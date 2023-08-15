#include <sstream>

#include <input/algo/print.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct Unpool {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<std::vector<T>> = delete;

    // terms

    auto operator()(Term const &term) const -> std::optional<TermVec> { return std::visit(*this, term); }

    auto operator()(std::optional<Term> const &term) const -> std::optional<std::vector<std::optional<Term>>> {
        return Util::and_then_opt(term, [this](Term const &term) {
            return Util::map_opt_vec(operator()(term), [](Term term) { return std::make_optional(std::move(term)); });
        });
    }

    auto operator()(TermVec const &terms) const -> std::optional<std::vector<TermVec>> {
        return unpool_crossproduct(terms, *this);
    }

    auto operator()(TermSymbol const &term) const -> std::optional<TermVec> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<TermVec> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<TupleVec> {
        return std::visit(
            [this](auto const &x) -> std::optional<TupleVec> {
                if constexpr (GRINGO_IS_OF_TYPE(x, Term)) {
                    return Util::map_opt_vec(operator()(x), [](auto term) { return TupleElem{std::move(term)}; });
                }
                if constexpr (GRINGO_IS_OF_TYPE(x, std::monostate)) {
                    return std::nullopt;
                }
            },
            elem);
    }

    auto operator()(TermTuple::Element const &tuple_or_term) const -> std::optional<TermTuple::ElementVec> {
        return std::visit(
            [this](auto const &x) -> std::optional<TermTuple::ElementVec> {
                if constexpr (GRINGO_IS_OF_TYPE(x, Term)) {
                    return Util::map_opt_vec(operator()(x),
                                             [](auto term) { return TermTuple::Element{std::move(term)}; });
                }
                if constexpr (GRINGO_IS_OF_TYPE(x, TupleVec)) {
                    return Util::map_opt_vec(unpool_crossproduct(x, *this),
                                             [](auto tuple) { return TermTuple::Element{std::move(tuple)}; });
                }
            },
            tuple_or_term);
    }

    auto operator()(TermTuple const &term) const -> std::optional<TermVec> {
        // unpool the elements
        auto elems = unpool_union(term.pool, *this);

        // turn the elements into individual tuple terms or terms
        if (!elems.has_value() && (term.pool.size() != 1 || std::holds_alternative<Term>(term.pool.front()))) {
            elems = term.pool;
        }
        return Util::map_opt_vec(std::move(elems), [&term](auto elem) -> Term {
            return std::visit(
                [&term](auto x) -> Term {
                    if constexpr (GRINGO_IS_OF_TYPE(x, Term)) {
                        return x;
                    }
                    if constexpr (GRINGO_IS_OF_TYPE(x, TupleVec)) {
                        return TermTuple{term.loc, TermTuple::ElementVec{std::move(x)}};
                    }
                },
                std::move(elem));
        });
    }

    auto operator()(TermFunction const &term) const -> std::optional<TermVec> {
        auto elems = unpool_union(term.pool, [this](TupleVec const &tuple) {
            // unpool the elements
            return unpool_crossproduct(tuple, *this);
        });

        if (!elems.has_value() && term.pool.size() != 1) {
            elems = term.pool;
        }

        return Util::map_opt_vec(std::move(elems), [&term](auto elem) -> Term {
            // turn individual elements into function terms
            return TermFunction{term.loc, term.name, PoolVec{std::move(elem)}, term.external};
        });
    }

    auto operator()(TermAbs const &term) const -> std::optional<TermVec> {
        auto unpooled = unpool_union(term.pool, *this);
        if (!unpooled.has_value() && term.pool.size() != 1) {
            unpooled = term.pool;
        }
        return Util::map_opt_vec(std::move(unpooled), [&term](auto arg) -> Term {
            return TermAbs{term.loc, TermVec{std::move(arg)}};
        });
    }

    auto operator()(TermUnary const &term) const -> std::optional<TermVec> {
        return Util::map_opt_vec(operator()(*term.rhs), [&term](auto rhs) -> Term {
            return TermUnary{term.loc, term.op, std::move(rhs)};
        });
    }

    auto operator()(TermBinary const &term) const -> std::optional<TermVec> {
        return unpool_crossproducts(
            [&term](auto lhs, auto rhs) -> Term {
                return TermBinary{term.loc, std::move(lhs), term.op, std::move(rhs)};
            },
            *this, *term.lhs, *term.rhs);
    }

    auto operator()(GuardVec const &guards) const -> std::optional<std::vector<GuardVec>> {
        return unpool_crossproduct(guards, [this](Guard const &guard) {
            return Util::map_opt_vec(operator()(guard.second), [&guard](auto term) {
                return Guard{guard.first, std::move(term)};
            });
        });
    }

    // literal

    auto operator()(Literal const &lit) const -> std::optional<LiteralVec> { return std::visit(*this, lit); }

    auto operator()(LiteralVec const &lits) const -> std::optional<std::vector<LiteralVec>> {
        return unpool_crossproduct(lits, *this);
    }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<LiteralVec> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<LiteralVec> {
        return unpool_crossproducts(
            [&lit](auto lhs, auto rhs) -> Literal {
                return LiteralRelation{lit.loc, lit.sign, std::move(lhs), std::move(rhs)};
            },
            *this, lit.lhs, lit.rhs);
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<LiteralVec> {
        return Util::map_opt_vec(operator()(lit.term), [&lit](auto term) -> Literal {
            return LiteralSymbolic{lit.loc, lit.sign, std::move(term)};
        });
    }

    // conjunction/disjunction

    auto operator()(ConditionalLiteralVec const &elems) const -> std::optional<std::vector<ConditionalLiteralVec>> {
        using Conds = std::vector<LiteralVec>;
        using OConds = std::optional<Conds>;
        using ElemConds = std::vector<OConds>;
        using OElemConds = std::optional<ElemConds>;

        // unpool the conditions
        OElemConds elem_conds;
        size_t i = 0;
        for (auto const &elem : elems) {
            auto conds = unpool_crossproduct(elem.cond, *this);
            if (conds.has_value()) {
                if (!elem_conds.has_value()) {
                    elem_conds = ElemConds(elems.size());
                }
                elem_conds->at(i) = std::move(conds).value();
            }
            ++i;
        }

        // unpool literals
        auto elem_lits = unpool_crossproduct(elems, [this](auto const &elem) {
            return Util::map_opt_vec(unpool_crossproduct(elem.lits, *this), [&elem](auto lits) {
                return ConditionalLiteral{elem.loc, std::move(lits), {}};
            });
        });

        // copy literals if conditions have been unpooled
        if (elem_conds.has_value() && !elem_lits.has_value()) {
            elem_lits = Util::make_vec<ConditionalLiteralVec>(ConditionalLiteralVec{});
            elem_lits->back().reserve(elems.size());
            for (auto const &elem : elems) {
                elem_lits->back().emplace_back(ConditionalLiteral{elem.loc, elem.lits, {}});
            }
        }

        // set conditions of unpooled literals and build disjunctions
        return Util::map_opt_vec(std::move(elem_lits), [&elem_conds, &elems](ConditionalLiteralVec elem_lits) {
            if (!elem_conds.has_value()) {
                size_t i = 0;
                for (auto &elem : elem_lits) {
                    elem.cond = elems[i].cond;
                    ++i;
                }
                return elem_lits;
            }
            ConditionalLiteralVec unpooled;
            for (size_t i = 0; i < elem_conds->size(); ++i) {
                if (elem_conds->at(i).has_value()) {
                    for (auto &cond : elem_conds->at(i).value()) {
                        unpooled.emplace_back(elems[i].loc, elem_lits[i].lits, cond);
                    }
                } else {
                    unpooled.emplace_back(elems[i].loc, elem_lits[i].lits, elems[i].cond);
                }
            }
            return unpooled;
        });
    }

    // set aggregate

    auto operator()(LGuard const &lhs) const -> std::optional<std::vector<LGuard>> {
        return Util::and_then_opt(lhs, [this](auto const &lhs) {
            return Util::map_opt_vec(operator()(lhs.first), [&lhs](auto term) {
                return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
            });
        });
    }

    auto operator()(RGuard const &rhs) const -> std::optional<std::vector<RGuard>> {
        return Util::and_then_opt(rhs, [this](auto const &rhs) {
            return Util::map_opt_vec(operator()(rhs.second), [&rhs](auto term) {
                return std::make_optional<RGuard::value_type>(rhs.first, std::move(term));
            });
        });
    }

    auto operator()(SetAggregateElement const &elem) const -> std::optional<std::vector<SetAggregateElement>> {
        return unpool_crossproducts(
            [](auto lit, auto cond) {
                return SetAggregateElement{std::move(lit), std::move(cond)};
            },
            *this, elem.lit, elem.cond);
    }

    auto operator()(SetAggregateElementVec const &elems) const -> std::optional<std::vector<SetAggregateElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<SetAggregateElementVec>(std::move(elems)); });
    }

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &aggr) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>> {
        return unpool_crossproducts(
            [&aggr](auto lhs, auto elems, auto rhs) {
                if constexpr (HasSign) {
                    return BodyLiteral{
                        SetAggregate<HasSign>{aggr.loc, aggr.sign, std::move(lhs), std::move(elems), std::move(rhs)}};
                } else {
                    return HeadLiteral{
                        SetAggregate<HasSign>{aggr.loc, std::move(lhs), std::move(elems), std::move(rhs)}};
                }
            },
            *this, aggr.lhs, aggr.elems, aggr.rhs);
    }

    // theory

    auto operator()(TheoryElement const &elem) const -> std::optional<std::vector<TheoryElement>> {
        return unpool_crossproducts(
            [&elem](auto cond) {
                return TheoryElement{std::get<0>(elem), std::move(cond)};
            },
            *this, std::get<1>(elem));
    }

    auto operator()(TheoryElementVec const &elems) const -> std::optional<std::vector<TheoryElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<TheoryElementVec>(std::move(elems)); });
    }

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>> {
        return unpool_crossproducts(
            [&atom](auto name, auto elems) {
                if constexpr (HasSign) {
                    return BodyLiteral{
                        TheoryAtom<HasSign>{atom.loc, atom.sign, std::move(name), std::move(elems), atom.rhs}};
                } else {
                    return HeadLiteral{TheoryAtom<HasSign>{atom.loc, std::move(name), std::move(elems), atom.rhs}};
                }
            },
            *this, atom.name, atom.elems);
    }

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> std::optional<std::conditional_t<Conjunctive, BodyLiteralVec, HeadLiteralVec>> {
        return Util::map_opt_vec(operator()(lit.elems),
                                 [&lit](auto elem) -> std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral> {
                                     return Junction<Conjunctive>{lit.loc, std::move(elem)};
                                 });
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        return Util::map_opt_vec(operator()(lit.lit),
                                 [](auto lit) -> HeadLiteral { return SimpleHeadLiteral{std::move(lit)}; });
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::ElementVec> {
        return unpool_crossproducts(
            [](auto tuple, auto lit, auto cond) {
                return HeadAggregate::Element{std::move(tuple), std::move(lit), std::move(cond)};
            },
            *this, elem.tuple, elem.lit, elem.cond);
    }

    auto operator()(HeadAggregate::ElementVec const &elems) const
        -> std::optional<std::vector<HeadAggregate::ElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<HeadAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        return unpool_crossproducts(
            [&lit](auto lhs, auto elems, auto rhs) -> HeadLiteral {
                return HeadAggregate{lit.loc, std::move(lhs), lit.fun, std::move(elems), std::move(rhs)};
            },
            *this, lit.lhs, lit.elems, lit.rhs);
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(BodyLiteralVec const &lits) const -> std::optional<std::vector<BodyLiteralVec>> {
        return unpool_crossproduct(lits, *this);
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::ElementVec> {
        return unpool_crossproducts(
            [](auto tuple, auto cond) {
                return BodyAggregate::Element{std::move(tuple), std::move(cond)};
            },
            *this, elem.tuple, elem.cond);
    }

    auto operator()(BodyAggregate::ElementVec const &elems) const
        -> std::optional<std::vector<BodyAggregate::ElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<BodyAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(BodyAggregate const &aggr) const -> std::optional<BodyLiteralVec> {
        return unpool_crossproducts(
            [&aggr](auto lhs, auto elems, auto rhs) -> BodyLiteral {
                return BodyAggregate{aggr.loc, aggr.sign, std::move(lhs), aggr.fun, std::move(elems), std::move(rhs)};
            },
            *this, aggr.lhs, aggr.elems, aggr.rhs);
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> { return std::visit(*this, stm); }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::EdgeVec> {
        return unpool_crossproducts(
            [](auto u, auto v) {
                return StatementEdge::Edge{std::move(u), std::move(v)};
            },
            *this, edge.u, edge.v);
    }

    auto operator()(StatementEdge::EdgeVec const &edges) const { return unpool_union(edges, *this); }

    auto operator()(Rule const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto head, auto body) -> Statement {
                return Rule{stm.loc, std::move(head), std::move(body)};
            },
            *this, stm.head, stm.body);
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize::Tuple const &tuple) const
        -> std::optional<std::vector<StatementOptimize::Tuple>> {
        return unpool_crossproducts(
            [](auto weight, auto prio, auto terms) {
                return StatementOptimize::Tuple{std::move(weight), std::move(prio), std::move(terms)};
            },
            *this, tuple.weight, tuple.priority, tuple.terms);
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::ElementVec> {
        return unpool_crossproducts(
            [](auto tuple, auto cond) {
                return StatementOptimize::Element{std::move(tuple), std::move(cond)};
            },
            *this, std::get<0>(elem), std::get<1>(elem));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        // TODO: consider turning into weak constraint
        return Util::map_opt(unpool_union(stm.elems, *this), [&stm](auto elems) {
            return Util::make_vec<Statement>(StatementOptimize{stm.loc, stm.type, std::move(elems)});
        });
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto body, auto tuple) -> Statement {
                return StatementWeakConstraint{stm.loc, std::move(body), std::move(tuple)};
            },
            *this, stm.body, stm.tuple);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto term, auto body) -> Statement {
                return StatementShow{stm.loc, std::move(term), std::move(body)};
            },
            *this, stm.term, stm.body);
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto term, auto body) -> Statement {
                return StatementProject{stm.loc, std::move(term), std::move(body)};
            },
            *this, stm.term, stm.body);
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto term, auto body, auto type) -> Statement {
                return StatementExternal{stm.loc, std::move(term), std::move(body), std::move(type)};
            },
            *this, stm.term, stm.body, stm.type);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        auto edges = operator()(stm.edges);
        auto bodies = operator()(stm.body);
        if (stm.edges.size() != 1 || edges.has_value() || bodies.has_value()) {
            StatementVec ret;
            for (auto &body : bodies.value_or(Util::make_vec<BodyLiteralVec>(stm.body))) {
                for (auto &edge : edges.value_or(stm.edges)) {
                    ret.emplace_back(StatementEdge{stm.loc, Util::make_vec<StatementEdge::Edge>(edge), body});
                }
            }
            return ret;
        }
        return std::nullopt;
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        return unpool_crossproducts(
            [&stm](auto atom, auto body, auto type, auto prio, auto mod) -> Statement {
                return StatementHeuristic{stm.loc,         std::move(atom), std::move(body),
                                          std::move(type), std::move(prio), std::move(mod)};
            },
            *this, stm.atom, stm.body, stm.type, stm.prio, stm.mod);
    }

    auto operator()(StatementScript const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<StatementVec> {
        auto ret = unpool_crossproducts(
            [&stm](auto value) -> Statement {
                return StatementConst{stm.loc, stm.type, stm.name, std::move(value)};
            },
            *this, stm.value);
        if (ret.has_value() && ret->size() != 1) {
            throw std::runtime_error("const statements must not contain pools");
        }
        return ret;
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        return std::nullopt;
    }
};

} // namespace

auto unpool(Term const &term) -> std::optional<TermVec> { return Unpool{}(term); }

auto unpool(Literal const &lit) -> std::optional<LiteralVec> { return Unpool{}(lit); }

auto unpool(HeadLiteral const &lit) -> std::optional<HeadLiteralVec> { return Unpool{}(lit); }

auto unpool(BodyLiteral const &lit) -> std::optional<BodyLiteralVec> { return Unpool{}(lit); }

auto unpool(Statement const &stm) -> std::optional<StatementVec> {
    auto stms = Unpool{}(stm);
    if (stms.has_value()) {
        VariableSet old_global = select_variables(stm, VariableContext::global);
        for (auto &unpooled : stms.value()) {
            VariableSet new_global = select_variables(unpooled, VariableContext::global, old_global.size());
            visit_variables(
                unpooled,
                [&](std::string const &var) {
                    if (old_global.contains(var) != new_global.contains(var)) {
                        std::ostringstream oss;
                        oss << "variable " << var << " in\n"
                            << "  " << stm << "\n"
                            << "is unsafe";
                        throw std::runtime_error(oss.str());
                    }
                },
                VariableContext::all);
        }
    }
    return stms;
}

} // namespace Gringo::Input
