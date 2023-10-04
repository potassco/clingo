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
    using TupleVecRes = std::vector<std::variant<std::monostate, Symbol, Term>>;

    //! Helper to simplify the arguments of the tuple.
    //!
    //! The resulting vector is nullopt if there were no simplifications.
    //! Otherwise, each element is either a monostate in case of projection, a
    //! symbol if it could evaluated right away, or a term in case of some
    //! other simplification.
    auto simplify_tuple(TupleVec const &tuple, bool &constant, std::optional<TupleVecRes> &res_tuple) const {
        size_t n = 0;

        // helper to initialize the arguments
        auto init = [&]() {
            if (!res_tuple.has_value()) {
                res_tuple = std::vector<std::variant<std::monostate, Symbol, Term>>{};
                res_tuple->reserve(tuple.size());
                for (auto it = tuple.begin(), ie = it + n; it != ie; ++it) {
                    std::visit([&res_tuple](auto &&res) { res_tuple->emplace_back(res); }, *it);
                }
            }
        };

        // evaluate arguments
        for (auto const &var_arg : tuple) {
            if (!std::visit(
                    [&, this](auto const &arg) {
                        GRINGO_MATCH(arg, std::monostate) {
                            constant = false;
                            init();
                            res_tuple->emplace_back();
                            return true;
                        }
                        GRINGO_MATCH(arg, Term) {
                            auto opt_res = operator()(arg);
                            // early exit in case of failure
                            if (!opt_res.has_value()) {
                                return false;
                            }
                            std::visit(
                                [&](auto &res) {
                                    GRINGO_MATCH(res, Symbol) {
                                        init();
                                        res_tuple->emplace_back(res);
                                    }
                                    GRINGO_MATCH(res, TermResult) {
                                        constant = false;
                                        if (!res_tuple.has_value() && !res.second.has_value()) {
                                            return;
                                        }
                                        init();
                                        res_tuple->emplace_back(res.second.has_value() ? std::move(res.second.value())
                                                                                       : arg);
                                    }
                                },
                                opt_res.value());
                        }
                        return true;
                    },
                    var_arg)) {
                return false;
            }
            ++n;
        }
        return true;
    }

    //! Convert the given simplified arguments to a symbol vector.
    //!
    //! The result vector must only store symbols.
    static auto args_symbol(std::optional<TupleVecRes> const &res_tuple) {
        std::vector<Symbol> args;
        if (res_tuple.has_value()) {
            args.reserve(res_tuple->size());
            for (auto const &arg : res_tuple.value()) {
                args.emplace_back(std::get<Symbol>(arg));
            }
        }
        return args;
    }

    //! Convert the given simplified arguments to term tuple.
    static auto args_term(TupleVec const &tuple, TupleVecRes const &res_tuple) {
        TupleVec args;
        auto it = tuple.begin();
        for (auto const &arg : res_tuple) {
            std::visit(
                [&](auto &&val) {
                    GRINGO_MATCH(val, Symbol) { args.emplace_back(TermSymbol{location(std::get<Term>(*it)), val}); }
                    else {
                        args.emplace_back(val);
                    }
                },
                arg);
            ++it;
        }
        return args;
    }

    auto operator()(Term const &term) const -> Result { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> Result { return term.value; }

    auto operator()(TermVariable const &term) const -> Result {
        static_cast<void>(term);
        // a variable can represent any term
        return TermResult{Type::any, std::nullopt};
    }

    auto operator()(TermFunction const &term) const -> Result {
        assert(term.pool.size() == 1);

        bool constant = !term.external;
        auto type = term.external ? Type::any : Type::symbolic;
        auto const &tuple = term.pool.front();
        std::optional<std::vector<std::variant<std::monostate, Symbol, Term>>> res_tuple;

        // simplify arguments
        if (!simplify_tuple(tuple, constant, res_tuple)) {
            return std::nullopt;
        }

        // none of the arguments changed
        if (!res_tuple.has_value() && !constant) {
            return TermResult{type, std::nullopt};
        }

        // the term can be evaluated to a symbol
        if (constant) {
            return store.fun(term.name, args_symbol(res_tuple), false);
        }

        // the term cannot be evaluated to a symbol
        return TermResult{type,
                          TermFunction{term.loc, term.name,
                                       Util::make_vec<TupleVec>(args_term(tuple, res_tuple.value())), term.external}};
    }

    auto operator()(TermTuple const &term) const -> Result {
        assert(term.pool.size() == 1 && std::holds_alternative<TupleVec>(term.pool.front()));

        bool constant = true;
        auto type = Type::tuple;
        auto const &tuple = std::get<TupleVec>(term.pool.front());
        std::optional<TupleVecRes> res_tuple;

        // simplify arguments
        if (!simplify_tuple(tuple, constant, res_tuple)) {
            return std::nullopt;
        }

        // none of the arguments changed
        if (!res_tuple.has_value() && !constant) {
            return TermResult{type, std::nullopt};
        }

        // the term can be evaluated to a symbol
        if (constant) {
            return store.tup(args_symbol(res_tuple));
        }

        // the term cannot be evaluated to a symbol
        return TermResult{type,
                          TermTuple{term.loc, Util::make_vec<TermTuple::Element>(args_term(tuple, res_tuple.value()))}};
    }

    auto operator()(TermAbs const &term) const -> Result {
        assert(term.pool.size() == 1);

        // simplify argument
        auto opt_res = operator()(term.pool.front());
        if (!opt_res.has_value()) {
            return std::nullopt;
        }

        // construct result
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
        // evaluate arguments
        auto opt_res_lhs = operator()(*term.lhs);
        auto opt_res_rhs = operator()(*term.rhs);
        if (!opt_res_lhs.has_value() || !opt_res_rhs.has_value()) {
            return std::nullopt;
        }

        auto is_numeric = [](auto &&res) {
            // TODO: error message
            GRINGO_MATCH(res, TermResult) { return res.first == Type::any || res.first == Type::numeric; }
            GRINGO_MATCH(res, Symbol) { return res.type() == SymbolType::number; }
        };

        auto as_term = [](auto const &term, auto &&res) -> Util::shared_ptr<Term> {
            GRINGO_MATCH(res, TermResult) {
                if (res.second.has_value()) {
                    return Util::construct_shared<Term>(std::move(res.second).value());
                }
                return term;
            }
            GRINGO_MATCH(res, Symbol) { return Util::construct_shared<Term>(TermSymbol{location(*term), res}); }
        };

        // construct result
        return std::visit(
            [&](auto &&res_lhs, auto &&res_rhs) -> Result {
                // check arguments
                if (!is_numeric(res_lhs) || !is_numeric(res_rhs)) {
                    return std::nullopt;
                }

                // evaluate to symbol
                GRINGO_MATCH2(res_lhs, Symbol, res_rhs, Symbol) {
                    auto res = evaluate(res_lhs, term.op, res_rhs);
                    if (!res.has_value()) {
                        // TODO: info message???
                        return std::nullopt;
                    }
                    return res.value();
                }

                // there was no change
                GRINGO_MATCH2(res_lhs, TermResult, res_rhs, TermResult) {
                    if (!res_lhs.second.has_value() && !res_rhs.second.has_value()) {
                        return TermResult{Type::numeric, std::nullopt};
                    }
                }

                // construct simplified term
                return TermResult{Type::numeric, TermBinary(term.loc, as_term(term.lhs, res_lhs), term.op,
                                                            as_term(term.rhs, res_rhs))};
            },
            opt_res_lhs.value(), opt_res_rhs.value());
    }

    SymbolStore &store;
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

[[nodiscard]] auto simplify(SymbolStore &store, Term const &term)
    -> std::variant<std::monostate, std::nullopt_t, Symbol, Term> {
    auto res = SimplifyTerm{store}(term);
    if (!res.has_value()) {
        return std::monostate{};
    }
    return std::visit(
        [](auto &&res) -> std::variant<std::monostate, std::nullopt_t, Symbol, Term> {
            GRINGO_MATCH(res, Symbol) { return res; }
            GRINGO_MATCH(res, SimplifyTerm::TermResult) {
                if (res.second.has_value()) {
                    return res.second.value();
                }
                return std::nullopt;
            }
        },
        res.value());
}

[[nodiscard]] auto rewrite_arthimetics(SymbolStore &store, Statement const &stm) -> std::optional<Statement> {
    static_cast<void>(store);
    auto map = TermMap{};
    return RewriteArithmetics{map}(stm);
}

} // namespace Gringo::Input
