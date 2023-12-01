#include <algorithm>

#include <input/algo/print.hh>
#include <input/algo/simplify.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct LiteralToTuple {
    auto operator()(Literal const &orig, Literal const &lit) -> TermVec {
        return std::visit(*this, std::variant<std::reference_wrapper<Literal const>>{orig}, lit);
    }

    auto tuple_from_vars(Literal const &orig) -> TermVec {
        auto var_set = select_variables(orig);
        auto var_vec = std::vector(var_set.begin(), var_set.end());
        std::sort(var_vec.begin(), var_vec.end());
        TermVec res;
        res.reserve(var_vec.size() + 1);
        res.emplace_back(TermSymbol{location(orig), store.num(n)});
        for (auto const &var : var_vec) {
            res.emplace_back(TermVariable{location(orig), var});
        }
        return res;
    }

    auto operator()(Literal const &orig, LiteralBoolean const &lit) -> TermVec {
        static_cast<void>(lit);
        return tuple_from_vars(orig);
    }

    auto operator()(Literal const &orig, LiteralRelation const &lit) -> TermVec {
        static_cast<void>(lit);
        return tuple_from_vars(orig);
    }

    auto operator()(Literal const &orig, LiteralSymbolic const &lit) -> TermVec {
        static_cast<void>(orig);
        TermVec res;
        res.reserve(2);
        int i = 0;
        switch (lit.sign) {
            case Sign::none: {
                i = 0;
                break;
            }
            case Sign::once: {
                i = 1;
                break;
            }
            case Sign::twice: {
                i = 2;
                break;
            }
        }
        res.emplace_back(TermSymbol{lit.loc, store.num(i)});
        res.emplace_back(lit.term);
        return res;
    }

    void next() { ++n; }

    SymbolStore &store;
    int n = 2;
};

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
                GRINGO_MATCH(x, Term) {
                    return Util::map_opt_vec(operator()(x), [](auto term) { return TupleElem{std::move(term)}; });
                }
                GRINGO_MATCH(x, std::monostate) { return std::nullopt; }
            },
            elem);
    }

    auto operator()(TermTuple::Element const &tuple_or_term) const -> std::optional<TermTuple::ElementVec> {
        return std::visit(
            [this](auto const &x) -> std::optional<TermTuple::ElementVec> {
                GRINGO_MATCH(x, Term) {
                    return Util::map_opt_vec(operator()(x),
                                             [](auto term) { return TermTuple::Element{std::move(term)}; });
                }
                GRINGO_MATCH(x, TupleVec) {
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
                    GRINGO_MATCH(x, Term) { return x; }
                    GRINGO_MATCH(x, TupleVec) { return TermTuple{term.loc, TermTuple::ElementVec{std::move(x)}}; }
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

    //! Unpool a set aggregate element.
    //!
    //! Note that this function rewrites into a tuple aggregate
    //! because set aggregates cannot represent unpooled relation literals (nicely).
    //! To be able to rewrite, the literal of the element is simplified first.
    template <bool HasSign>
    void
    unpool_elem(LiteralToTuple &to_tuple, SetAggregateElement const &elem,
                std::vector<typename std::conditional_t<HasSign, BodyAggregate, HeadAggregate>::Element> &elems) const {
        auto set_elems = unpool_crossproducts(
            [&elem](auto lit, auto cond) {
                return SetAggregateElement{elem.loc, std::move(lit), std::move(cond)};
            },
            *this, elem.lit, elem.cond);
        auto simplify_lit = [this, &to_tuple, &elem, &elems](SetAggregateElement unpooled) {
            auto guard = ctx.push();
            auto res = simplify(HasSign ? SimplifyLiteralFlags::matchable
                                        : (SimplifyLiteralFlags::matchable | SimplifyLiteralFlags::unfailable),
                                ctx, unpooled.lit);
            auto lit = res.value.value_or(std::move(unpooled.lit));
            for (auto &[lhs, rhs] : ctx.aux()) {
                auto loc = location(lhs);
                auto rel = LiteralRelation{loc, Sign::none, std::move(lhs),
                                           Util::make_vec<Guard>(Guard{Relation::equal, std::move(rhs)})};
                unpooled.cond.emplace_back(std::move(rel));
            }
            auto tuple = to_tuple(elem.lit, lit);
            if constexpr (HasSign) {
                unpooled.cond.emplace_back(std::move(lit));
                elems.emplace_back(elem.loc, std::move(tuple), std::move(unpooled.cond));
            } else {
                elems.emplace_back(elem.loc, std::move(tuple), std::move(lit), std::move(unpooled.cond));
            }
        };
        if (set_elems.has_value()) {
            for (auto &unpooled : set_elems.value()) {
                simplify_lit(std::move(unpooled));
            }
        } else {
            simplify_lit(elem);
        }
    }

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &aggr) const
        -> std::optional<std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>> {
        auto convert = [this, &aggr](auto lhs, auto rhs) {
            std::vector<typename std::conditional_t<HasSign, BodyAggregate, HeadAggregate>::Element> elems;
            LiteralToTuple to_tuple{ctx.store()};
            for (auto &elem : aggr.elems) {
                to_tuple.next();
                unpool_elem<HasSign>(to_tuple, elem, elems);
            }
            if constexpr (HasSign) {
                return BodyLiteral{BodyAggregate{aggr.loc, aggr.sign, std::move(lhs), AggregateFunction::count,
                                                 std::move(elems), std::move(rhs)}};
            } else {
                return HeadLiteral{HeadAggregate{aggr.loc, std::move(lhs), AggregateFunction::count, std::move(elems),
                                                 std::move(rhs)}};
            }
        };
        auto ret = unpool_crossproducts(convert, *this, aggr.lhs, aggr.rhs);
        if (!ret.has_value()) {
            ret = std::vector<std::conditional_t<HasSign, BodyLiteral, HeadLiteral>>{};
            ret->emplace_back(convert(aggr.lhs, aggr.rhs));
        }
        return ret;
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
            [&elem](auto tuple, auto lit, auto cond) {
                return HeadAggregate::Element{elem.loc, std::move(tuple), std::move(lit), std::move(cond)};
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

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteralVec> {
        return Util::map_opt_vec(operator()(lit.lit),
                                 [](auto lit) -> BodyLiteral { return SimpleBodyLiteral{std::move(lit)}; });
    }

    auto operator()(BodyLiteralVec const &lits) const -> std::optional<std::vector<BodyLiteralVec>> {
        return unpool_crossproduct(lits, *this);
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::ElementVec> {
        return unpool_crossproducts(
            [&elem](auto tuple, auto cond) {
                return BodyAggregate::Element{elem.loc, std::move(tuple), std::move(cond)};
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
        StatementVec stms;
        stms.reserve(stm.elems.size());
        for (auto const &elem : stm.elems) {
            auto body = BodyLiteralVec{};
            body.reserve(elem.second.size());
            for (auto const &lit : elem.second) {
                body.emplace_back(SimpleBodyLiteral{lit});
            }
            auto cons = StatementWeakConstraint{stm.loc, std::move(body), elem.first};
            if (auto opt_stms = operator()(cons); opt_stms.has_value()) {
                stms.insert(stms.end(), std::make_move_iterator(opt_stms->begin()),
                            std::make_move_iterator(opt_stms->end()));
            } else {
                stms.emplace_back(std::move(cons));
            }
        }
        return stms;
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

    RewriteContext &ctx;
};

} // namespace

auto unpool(RewriteContext &ctx, Term const &term) -> std::optional<TermVec> { return Unpool{ctx}(term); }

auto unpool(RewriteContext &ctx, Literal const &lit) -> std::optional<LiteralVec> { return Unpool{ctx}(lit); }

auto unpool(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVec> { return Unpool{ctx}(lit); }

auto unpool(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVec> { return Unpool{ctx}(lit); }

template <class F> struct print {
    print(F const &f) : f{f} {}
    friend auto operator<<(std::ostream &out, print const &p) -> std::ostream & {
        p.f(out);
        return out;
    }
    F const &f;
};

auto unpool(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    auto stms = Unpool{ctx}(stm);
    if (stms.has_value()) {
        VariableSet old_global = select_variables(stm, VariableContext::global);
        for (auto &unpooled : stms.value()) {
            VariableSet new_global = select_variables(unpooled, VariableContext::global, old_global.size());
            std::vector<std::pair<Location, String>> unsafe;
            visit_variables(
                unpooled,
                [&](Location const &loc, String var) {
                    if (old_global.contains(var) != new_global.contains(var)) {
                        unsafe.emplace_back(loc, var);
                    }
                },
                VariableContext::all);
            if (!unsafe.empty()) {
                GRINGO_REPORT_LOC(ctx.logger(), error, location(stm))
                    << "unsafe variables in:\n"
                    << "  " << stm << "\n"
                    << print{[&unsafe](std::ostream &out) {
                           for (auto const &[loc, var] : unsafe) {
                               out << loc << ": note: '" << var << "' is unsafe";
                           }
                       }};
            }
        }
    }
    return stms;
}

} // namespace Gringo::Input
