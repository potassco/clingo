#include <input/algo/evaluate.hh>
#include <input/algo/rewrite_arithmetic.hh>

#include "checked_math.hh"
#include "transform.hh"

/*
// TODO: rename file!!!
whole process as in gringo atm
1. apply #const statements (partially done)
2. unpool (done)
3. init theory
4. simplify
  0. evaluate/linear terms
  1. extract atoms to project
  2. dots
  3. script
5. unpool comparison
6. rewrite
  1. aggregates
  2. arithmetics
  4. comparisons to intervals
  5. assignment aggregates
*/

namespace Gringo::Input {

namespace {

struct TermMap {};

//! Check if a term can be used for matching.
struct SimplifyTerm {
    enum class Type {
        numeric,
        symbolic,
        tuple,
        any,
    };
    using TermResult = std::pair<Type, std::optional<Term>>;
    using Result = std::optional<std::variant<Symbol, TermResult>>;

    auto operator()(Term const &term) const -> Result { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> Result { return term.value; }

    auto operator()(TermVariable const &term) const -> Result {
        static_cast<void>(term);
        // a variable can represent any term
        return TermResult{Type::any, std::nullopt};
    }

    auto operator()(TermFunction const &term) const -> Result {
        assert(term.pool.size() == 1);
        std::vector<Result::value_type> res_args;
        for (auto const &arg : term.pool.front()) {
            if (std::holds_alternative<std::monostate>(arg)) {
                // TODO: handle
                continue;
            }
            auto res_arg = operator()(std::get<Term>(arg));
            // early exit in case of failure
            if (!res_arg.has_value()) {
                return std::nullopt;
            }
            res_args.emplace_back(res_arg.value());
            // TODO: in case res_arg changes a new argument vector has to be constructed
        }
        static_cast<void>(term);
        throw std::runtime_error("implement me!!!");
    }

    auto operator()(TermTuple const &term) const -> Result {
        static_cast<void>(term);
        throw std::runtime_error("implement me!!!");
    }

    auto operator()(TermAbs const &term) const -> Result {
        assert(term.pool.size() == 1);
        auto opt_res = operator()(term.pool.front());
        if (!opt_res.has_value()) {
            return std::nullopt;
        }
        return std::visit(
            [&term](auto &res) -> Result {
                // evaluate symbol
                GRINGO_MATCH(res, Symbol) {
                    if (res.type() != SymbolType::number) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    auto res_val = check_abs(res.num());
                    if (!res_val.has_value()) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    return SymbolStore::num(res_val.value());
                }
                // handle term
                GRINGO_MATCH(res, TermResult) {
                    // handle invalid terms
                    if (res.first == Type::symbolic || res.first == Type::tuple) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    // construct a new term
                    if (res.second.has_value()) {
                        TermVec pool;
                        pool.emplace_back(std::move(*res.second));
                        return TermResult{Type::numeric, TermAbs{term.loc, std::move(pool)}};
                    }
                    // the term did not change
                    return TermResult{Type::numeric, std::nullopt};
                }
            },
            opt_res.value());
    }

    auto operator()(TermUnary const &term) const -> Result {
        auto opt_res_rhs = operator()(*term.rhs);
        if (!opt_res_rhs.has_value()) {
            return std::nullopt;
        }
        return std::visit(
            [&term](auto &res_rhs) -> Result {
                // evaluate symbol
                GRINGO_MATCH(res_rhs, Symbol) {
                    // we can always evaluate constants
                    auto res = evaluate(term.op, res_rhs);
                    if (!res.has_value()) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    return res.value();
                }
                GRINGO_MATCH(res_rhs, TermResult) {
                    // fail if term is not invertable/negatable
                    if (res_rhs.first == Type::tuple ||
                        (term.op == UnaryOperator::invert && res_rhs.first == Type::symbolic)) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    // ~term is always numeric
                    auto type = term.op == UnaryOperator::invert ? Type::numeric : res_rhs.first;
                    // simplify --symbolic to symbolic
                    // (we cannot simplify numeric terms because `-` can overflow)
                    auto fold = [&term, &type](Term const &rhs) -> Term const * {
                        if (auto const *rhs_unary = std::get_if<TermUnary>(&rhs);
                            rhs_unary != nullptr && term.op == UnaryOperator::invert &&
                            rhs_unary->op == UnaryOperator::invert && type == Type::symbolic) {
                            return rhs_unary->rhs.get();
                        }
                        return nullptr;
                    };
                    // the argument changed -> construct new term and fold if possible
                    if (res_rhs.second.has_value()) {
                        if (auto const *rhs_rhs = fold(res_rhs.second.value()); rhs_rhs != nullptr) {
                            return TermResult{type, *rhs_rhs};
                        }
                        return TermResult{type,
                                          TermUnary{term.loc, term.op,
                                                    Util::construct_shared<Term>(std::move(res_rhs.second.value()))}};
                    }
                    // the argument did not change but the term be foldable
                    if (auto const *rhs_rhs = fold(*term.rhs); rhs_rhs != nullptr) {
                        return TermResult{type, *rhs_rhs};
                    }
                    // the term did not change
                    return TermResult{type, std::nullopt};
                }
            },
            opt_res_rhs.value());
    }

    auto operator()(TermBinary const &term) const -> Result {
        auto res_lhs = operator()(*term.lhs);
        auto res_rhs = operator()(*term.rhs);
        static_cast<void>(res_lhs);
        static_cast<void>(res_rhs);
        throw std::runtime_error("implement me!!!");
    }
};

//! Check if a term can be used for matching.
struct TermCanMatch {
    auto operator()(Term const &term) const -> bool { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermVariable const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermFunction const &term) const -> bool { return !term.external; }

    auto operator()(TermTuple const &term) const -> bool {
        static_cast<void>(term);
        return true;
    }

    auto operator()(TermAbs const &term) const -> bool {
        static_cast<void>(term);
        return false;
    }

    auto operator()(TermUnary const &term) const -> bool { return term.op == UnaryOperator::negate; }

    auto operator()(TermBinary const &term) const -> bool {
        static_cast<void>(term);
        // TODO
        return true;
    }
};

struct RewriteArithmetics : Transformer<RewriteArithmetics> {
    RewriteArithmetics(TermMap &map) : map{map} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // ignore

    auto operator()(std::monostate const &x) const -> std::optional<std::monostate> {
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

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        return transform_construct<TermFunction>(term.loc, term.name, tr(term.pool), term.external);
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

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> { return std::visit(*this, term); }

    auto operator()(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermUnparsed>(term.loc, tr(term.elems));
    }

    auto operator()(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermTuple>(term.loc, term.type, tr(term.elems));
    }

    auto operator()(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermFunction>(term.loc, term.name, tr(term.args));
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
        return transform_construct<LiteralSymbolic>(lit.loc, lit.sign, tr(lit.term));
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
        return transform_construct<SetAggregateElement>(tr(elem.lit), tr(elem.cond));
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> { return operator()(lit.lit); }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(tr(elem.tuple), tr(elem.lit), tr(elem.cond));
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
        return transform_construct<BodyAggregate::Element>(tr(elem.tuple), tr(elem.cond));
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
        return transform_construct<StatementProject>(stm.loc, tr(stm.term), tr(stm.body));
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
        return transform_construct<StatementExternal>(stm.loc, tr(stm.term), tr(stm.body), tr(stm.type));
    }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::Edge> {
        return transform_construct<StatementEdge::Edge>(tr(edge.u), tr(edge.v));
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc, tr(stm.edges), tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc, tr(stm.atom), tr(stm.body), tr(stm.type), tr(stm.prio),
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

    TermMap &map;
};

} // namespace

[[nodiscard]] auto rewrite_arthimetics(SymbolStore &store, Statement const &stm) -> std::optional<Statement> {
    static_cast<void>(store);
    auto map = TermMap{};
    return RewriteArithmetics{map}(stm);
}

} // namespace Gringo::Input
