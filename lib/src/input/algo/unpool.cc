#include <input/algo/unpool.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct Unpool {

    // terms

    auto operator()(Term const &term) const -> std::optional<TermVec> { return std::visit(*this, term); }

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
        return Util::visit_variant(
            elem,
            [this](Term const &term) -> std::optional<TupleVec> {
                return Util::map_opt_vec(operator()(term), [](auto term) { return TupleElem{std::move(term)}; });
            },
            [](std::monostate x) -> std::optional<TupleVec> {
                static_cast<void>(x);
                return std::nullopt;
            });
    }

    auto operator()(TermTuple::Element const &tuple_or_term) const -> std::optional<TermTuple::ElementVec> {
        return Util::visit_variant(
            tuple_or_term,
            [this](Term const &term) -> std::optional<TermTuple::ElementVec> {
                return Util::map_opt_vec(operator()(term),
                                         [](auto term) { return TermTuple::Element{std::move(term)}; });
            },
            [this](TupleVec const &tuple) -> std::optional<TermTuple::ElementVec> {
                return Util::map_opt_vec(unpool_crossproduct(tuple, *this),
                                         [](auto tuple) { return TermTuple::Element{std::move(tuple)}; });
            });
    }

    auto operator()(TermTuple const &term) const -> std::optional<TermVec> {
        // unpool the elements
        auto elems = unpool_union(term.pool, *this);

        // turn the elements into individual tuple terms or terms
        if (!elems.has_value() && (term.pool.size() != 1 || std::holds_alternative<Term>(term.pool.front()))) {
            elems = term.pool;
        }
        return Util::map_opt_vec(std::move(elems), [](auto elem) -> Term {
            return Util::visit_variant(
                std::move(elem), [](Term term) { return term; },
                [](TupleVec tuple) -> Term { return TermTuple{TermTuple::ElementVec{std::move(tuple)}}; });
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
            return TermFunction{term.name, PoolVec{std::move(elem)}, term.external};
        });
    }

    auto operator()(TermAbs const &term) const -> std::optional<TermVec> {
        auto unpooled = unpool_union(term.pool, *this);
        if (!unpooled.has_value() && term.pool.size() != 1) {
            unpooled = term.pool;
        }
        return Util::map_opt_vec(std::move(unpooled),
                                 [](auto term) -> Term { return TermAbs{TermVec{std::move(term)}}; });
    }

    auto operator()(TermUnary const &term) const -> std::optional<TermVec> {
        return Util::map_opt_vec(operator()(*term.rhs), [&term](auto rhs) -> Term {
            return TermUnary{term.op, std::move(rhs)};
        });
    }

    auto operator()(TermBinary const &term) const -> std::optional<TermVec> {
        return unpool_crossproducts(
            [&term](auto lhs, auto rhs) -> Term {
                return TermBinary{std::move(lhs), term.op, std::move(rhs)};
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
                return LiteralRelation{lit.sign, std::move(lhs), std::move(rhs)};
            },
            *this, lit.lhs, lit.rhs);
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<LiteralVec> {
        return Util::map_opt_vec(operator()(lit.term), [&lit](auto term) -> Literal {
            return LiteralSymbolic{lit.sign, std::move(term)};
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
            return Util::map_opt_vec(unpool_crossproduct(elem.lits, *this), [](auto lits) {
                return ConditionalLiteral{std::move(lits), {}};
            });
        });

        // copy literals if conditions have been unpooled
        if (elem_conds.has_value() && !elem_lits.has_value()) {
            elem_lits = Util::make_vec<ConditionalLiteralVec>(ConditionalLiteralVec{});
            elem_lits->back().reserve(elems.size());
            for (auto const &elem : elems) {
                elem_lits->back().emplace_back(ConditionalLiteral{elem.lits, {}});
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
                        unpooled.emplace_back(elem_lits[i].lits, cond);
                    }
                } else {
                    unpooled.emplace_back(elem_lits[i].lits, elems[i].cond);
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

    auto operator()(SetAggregate::Element const &elem) -> std::optional<std::vector<SetAggregate::Element>> {
        return unpool_crossproducts(
            [](auto lit, auto cond) {
                return SetAggregate::Element{std::move(lit), std::move(cond)};
            },
            *this, elem.lit, elem.cond);
    }

    auto operator()(SetAggregate::ElementVec const &elems) const
        -> std::optional<std::vector<SetAggregate::ElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<SetAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(SetAggregate const &aggr) const -> std::optional<std::vector<SetAggregate>> {
        return unpool_crossproducts(
            [&](auto lhs, auto elems, auto rhs) {
                return SetAggregate{std::move(lhs), std::move(elems), std::move(rhs)};
            },
            *this, aggr.lhs, aggr.elems, aggr.rhs);
    }

    // theory

    auto operator()(TheoryAtom::Element const &elem) -> std::optional<std::vector<TheoryAtom::Element>> {
        return unpool_crossproducts(
            [&elem](auto cond) {
                return TheoryAtom::Element{std::get<0>(elem), std::move(cond)};
            },
            *this, std::get<1>(elem));
    }

    auto operator()(TheoryAtom::ElementVec const &elems) -> std::optional<std::vector<TheoryAtom::ElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<TheoryAtom::ElementVec>(std::move(elems)); });
    }

    auto operator()(TheoryAtom const &atom) const -> std::optional<std::vector<TheoryAtom>> {
        return unpool_crossproducts(
            [&atom](auto name, auto elems) {
                return TheoryAtom{std::move(name), std::move(elems), atom.rhs};
            },
            *this, atom.name, atom.elems);
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteralVec> {
        return Util::map_opt_vec(operator()(lit.elems),
                                 [](auto elem) -> HeadLiteral { return Disjunction{std::move(elem)}; });
    }

    auto operator()(HeadAggregate::Element const &elem) -> std::optional<HeadAggregate::ElementVec> {
        return unpool_crossproducts(
            [](auto tuple, auto lit, auto cond) {
                return HeadAggregate::Element{std::move(tuple), std::move(lit), std::move(cond)};
            },
            *this, std::get<0>(elem), std::get<1>(elem), std::get<2>(elem));
    }

    auto operator()(HeadAggregate::ElementVec const &elems) -> std::optional<std::vector<HeadAggregate::ElementVec>> {
        return Util::map_opt(unpool_union(elems, *this),
                             [](auto elems) { return Util::make_vec<HeadAggregate::ElementVec>(std::move(elems)); });
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        return unpool_crossproducts(
            [&lit](auto lhs, auto elems, auto rhs) -> HeadLiteral {
                return HeadAggregate{std::move(lhs), lit.fun, std::move(elems), std::move(rhs)};
            },
            *this, lit.lhs, lit.elems, lit.rhs);
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        return Util::map_opt_vec(operator()(lit.aggr),
                                 [](auto aggr) -> HeadLiteral { return HeadSetAggregate{std::move(aggr)}; });
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteralVec> {
        return Util::map_opt_vec(operator()(lit.atom),
                                 [](auto atom) -> HeadLiteral { return HeadTheoryAtom{std::move(atom)}; });
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteralVec> {
        return Util::map_opt_vec(operator()(lit.elems),
                                 [](auto elem) -> BodyLiteral { return Conjunction{std::move(elem)}; });
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
                return BodyAggregate{aggr.sign, std::move(lhs), aggr.fun, std::move(elems), std::move(rhs)};
            },
            *this, aggr.lhs, aggr.elems, aggr.rhs);
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteralVec> {
        return Util::map_opt_vec(operator()(lit.aggr), [&lit](auto aggr) -> BodyLiteral {
            return BodySetAggregate{lit.sign, std::move(aggr)};
        });
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteralVec> {
        return Util::map_opt_vec(operator()(lit.atom), [&lit](auto atom) -> BodyLiteral {
            return BodyTheoryAtom{lit.sign, std::move(atom)};
        });
    }
};

} // namespace

auto unpool(Term const &term) -> std::optional<TermVec> { return Unpool{}(term); }

auto unpool(Literal const &lit) -> std::optional<LiteralVec> { return Unpool{}(lit); }

auto unpool(HeadLiteral const &lit) -> std::optional<HeadLiteralVec> { return Unpool{}(lit); }

auto unpool(BodyLiteral const &lit) -> std::optional<BodyLiteralVec> { return Unpool{}(lit); }

} // namespace Gringo::Input
