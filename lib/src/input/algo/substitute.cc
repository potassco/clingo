#include "transform.hh"

#include <input/program.hh>

namespace Gringo::Input {

namespace {

[[nodiscard]] auto is_identifier(Term const &term) -> bool {
    if (auto const *fun = std::get_if<TermFunction>(&term); fun != nullptr) {
        return !fun->external && fun->pool.size() == 1 && fun->pool.front().empty();
    }
    if (auto const *sym = std::get_if<TermSymbol>(&term); sym != nullptr) {
        return sym->value.type() == SymbolType::function && sym->value.args().empty();
    }
    return false;
}

[[nodiscard]] auto variable_for_param(RewriteContext &ctx, Location const &loc, size_t param) {
    return TermVariable{loc, ctx.store().string("$" + std::to_string(param))};
}

struct Substitute : Transformer<Substitute> {

    Substitute(RewriteContext &ctx) : ctx{ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // ignore

    auto operator()(Projection const &x) const -> std::optional<Projection> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(String const &x) const -> std::optional<String> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(Relation const &x) const -> std::optional<Relation> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(Location const &loc, SymbolSpan args) const -> std::optional<std::variant<SymbolVec, TupleVec>> {
        std::optional<std::vector<std::variant<Term, Symbol>>> res_args;
        bool constant = true;
        {
            size_t i = 0;
            for (auto arg : args) {
                auto res_arg = operator()(loc, arg);
                if (res_arg.has_value() || res_args.has_value()) {
                    if (!res_args.has_value()) {
                        res_args.emplace();
                        res_args->reserve(args.size());
                        res_args->insert(res_args->end(), args.begin(), args.begin() + i);
                    }
                    res_args->emplace_back(std::move(res_arg).value_or(arg));
                    if (constant && std::holds_alternative<Term>(res_args->back())) {
                        constant = false;
                    }
                }
                ++i;
            }
        }
        if (!res_args) {
            return std::nullopt;
        }
        if (constant) {
            auto tuple = SymbolVec{};
            tuple.reserve(res_args->size());
            for (auto &&arg : *res_args) {
                tuple.emplace_back(std::get<Symbol>(arg));
            }
            return tuple;
        }
        auto tuple = TupleVec{};
        tuple.reserve(res_args->size());
        for (auto &&arg : *res_args) {
            tuple.emplace_back(std::visit(
                [&loc](auto &&x) -> Term {
                    GRINGO_MATCH(x, Symbol) { return TermSymbol{loc, x}; }
                    GRINGO_MATCH(x, Term) { return x; }
                },
                std::move(arg)));
        }
        return tuple;
    }

    auto operator()(Location const &loc, Symbol const &sym) const -> std::optional<std::variant<Term, Symbol>> {
        switch (sym.type()) {
            case SymbolType::function: {
                if (sym.args().empty()) {
                    if (auto param = ctx.is_param(sym.name()); param) {
                        return variable_for_param(ctx, loc, param.value());
                    }
                    if (auto value = ctx.is_const(sym.name()); value) {
                        if (sym.has_sign()) {
                            switch (sym.type()) {
                                case SymbolType::function: {
                                    return value->flip_classical_sign();
                                }
                                case SymbolType::number: {
                                    return ctx.store().num(-*value->num());
                                }
                                case SymbolType::inf:
                                case SymbolType::sup:
                                case SymbolType::string:
                                case SymbolType::tuple: {
                                    break;
                                }
                            }
                            // Note: this will evaluated as empty pool later
                            return TermUnary{loc, UnaryOperator::negate, TermSymbol{loc, *value}};
                        }
                        return value;
                    }
                    break;
                }
                if (auto res_args = operator()(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc, &sym](auto &&tuple) -> std::variant<Term, Symbol> {
                            GRINGO_MATCH(tuple, SymbolVec) {
                                return ctx.store().fun(sym.name(), std::move(tuple), sym.has_sign());
                            }
                            GRINGO_MATCH(tuple, TupleVec) {
                                auto ret = Term{
                                    TermFunction{loc, sym.name(), Util::make_vec<TupleVec>(std::move(tuple)), false}};
                                if (sym.has_sign()) {
                                    ret = TermUnary{loc, UnaryOperator::negate, std::move(ret)};
                                }
                                return ret;
                            }
                        },
                        std::move(res_args).value());
                }
                break;
            }
            case SymbolType::tuple: {
                if (auto res_args = operator()(loc, sym.args()); res_args) {
                    return std::visit(
                        [this, &loc](auto &&tuple) -> std::variant<Term, Symbol> {
                            GRINGO_MATCH(tuple, SymbolVec) { return ctx.store().tup(std::move(tuple)); }
                            GRINGO_MATCH(tuple, TupleVec) {
                                return TermTuple{loc, Util::make_vec<TermTuple::Element>(std::move(tuple))};
                            }
                        },
                        std::move(res_args).value());
                }
                break;
            }
            case SymbolType::inf:
            case SymbolType::sup:
            case SymbolType::number:
            case SymbolType::string: {
                break;
            }
        }
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        auto sym = operator()(term.loc, term.value);
        if (sym.has_value()) {
            return std::visit(
                [&term](auto &&x) -> Term {
                    GRINGO_MATCH(x, Symbol) { return TermSymbol{term.loc, x}; }
                    GRINGO_MATCH(x, Term) { return x; }
                },
                sym.value());
        }
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        if (term.pool.size() != 1) {
            throw std::runtime_error("unpool has to be called before substituting parameters");
        }
        if (!term.pool.front().empty() || term.external) {
            return transform_construct<TermFunction>(term.loc, term.name, tr(term.pool), term.external);
        }
        if (auto param = ctx.is_param(term.name); param) {
            return TermVariable{term.loc, ctx.store().string("$" + std::to_string(param.value()))};
        }
        if (auto value = ctx.is_const(term.name); value) {
            return TermSymbol{term.loc, value.value()};
        }
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(term.loc, tr(term.pool));
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct<TermAbs>(term.loc, tr(term.pool));
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct<TermUnary>(term.loc, term.op, tr(term.rhs));
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct<TermBinary>(term.loc, tr(term.lhs), term.op, tr(term.rhs));
    }

    // theory

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    auto operator()(Literal const &lit) const { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralRelation>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.rhs));
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        if (!is_identifier(lit.term)) {
            return transform_construct<LiteralSymbolic>(lit.loc, lit.sign, tr(lit.term));
        }
        return std::nullopt;
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &lit) const -> std::optional<ConditionalLiteral> {
        return transform_construct<ConditionalLiteral>(lit.loc, tr(lit.lits), tr(lit.cond));
    }

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> std::optional<std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral>> {
        return transform_construct<Junction<Conjunctive>>(lit.loc, tr(lit.elems));
    }

    // set aggregate

    auto operator()(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return transform_construct<SetAggregateElement>(elem.loc, tr(elem.lit), tr(elem.cond));
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> { return operator()(lit.lit); }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(elem.loc, tr(elem.tuple), tr(elem.lit), tr(elem.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(lit.loc, tr(lit.name), tr(lit.elems), tr(lit.rhs));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteral> { return operator()(lit.lit); }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(elem.loc, tr(elem.tuple), tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc, lit.sign, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.loc, lit.sign, tr(lit.name), tr(lit.elems), tr(lit.rhs));
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> std::optional<Statement> {
        return transform_construct<Rule>(stm.loc, tr(stm.head), tr(stm.body));
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize::Tuple const &elem) const -> std::optional<StatementOptimize::Tuple> {
        return transform_construct<StatementOptimize::Tuple>(tr(elem.weight), tr(elem.priority), tr(elem.terms));
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(tr(elem.first), tr(elem.second));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementOptimize>(stm.loc, stm.type, tr(stm.elems));
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc, tr(stm.body), tr(stm.tuple));
    }

    auto operator()(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc, tr(stm.term), tr(stm.body));
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<Statement> {
        if (!is_identifier(stm.term)) {
            return transform_construct<StatementProject>(stm.loc, tr(stm.term), tr(stm.body));
        }
        return transform_construct<StatementProject>(stm.loc, stm.term, tr(stm.body));
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<Statement> {
        if (!is_identifier(stm.term)) {
            return transform_construct<StatementExternal>(stm.loc, tr(stm.term), tr(stm.body), tr(stm.type));
        }
        return transform_construct<StatementExternal>(stm.loc, stm.term, tr(stm.body), tr(stm.type));
    }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::Edge> {
        return transform_construct<StatementEdge::Edge>(tr(edge.u), tr(edge.v));
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc, tr(stm.edges), tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        if (!is_identifier(stm.atom)) {
            return transform_construct<StatementHeuristic>(stm.loc, tr(stm.atom), tr(stm.body), tr(stm.type),
                                                           tr(stm.prio), tr(stm.mod));
        }
        return transform_construct<StatementHeuristic>(stm.loc, stm.atom, tr(stm.body), tr(stm.type), tr(stm.prio),
                                                       tr(stm.mod));
    }

    auto operator()(StatementScript const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(Comment const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    RewriteContext &ctx;
};

} // namespace

[[nodiscard]] auto substitute(RewriteContext &ctx, Term const &term) -> std::optional<Term> {
    if (ctx.has_params()) {
        return Substitute{ctx}(term);
    }
    return std::nullopt;
}

[[nodiscard]] auto substitute(RewriteContext &ctx, Literal const &lit) -> std::optional<Literal> {
    if (ctx.has_params()) {
        return Substitute{ctx}(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto substitute(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteral> {
    if (ctx.has_params()) {
        return Substitute{ctx}(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto substitute(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteral> {
    if (ctx.has_params()) {
        return Substitute{ctx}(lit);
    }
    return std::nullopt;
}

[[nodiscard]] auto substitute(RewriteContext &ctx, Statement const &stm) -> std::optional<Statement> {
    if (ctx.has_params()) {
        return Substitute{ctx}(stm);
    }
    return std::nullopt;
}

} // namespace Gringo::Input
