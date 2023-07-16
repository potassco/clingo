#include <input/algo/unpool.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct Unpool {

    auto operator()(Term const &term) const -> std::optional<TermVec> { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> std::optional<TermVec> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<TermVec> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<TermVec> {
        // unpool the elements
        auto elems = unpool_union(term.pool, [this](TermTuple::Element const &tuple_or_term) {
            return Util::visit_variant(
                tuple_or_term,
                [this](Term const &term) -> std::optional<TermTuple::ElementVec> {
                    return Util::map_opt_vec(std::visit(*this, term),
                                             [](auto term) { return TermTuple::Element{std::move(term)}; });
                },
                [this](TupleVec const &tuple) -> std::optional<TermTuple::ElementVec> {
                    return Util::map_opt_vec(
                        unpool_crossproduct(tuple,
                                            [this](TupleElem const &elem) {
                                                return Util::visit_variant(
                                                    elem,
                                                    [this](Term const &term) -> std::optional<TupleVec> {
                                                        return Util::map_opt_vec(
                                                            std::visit(*this, term),
                                                            [](auto term) { return TupleElem{std::move(term)}; });
                                                    },
                                                    [](std::monostate x) -> std::optional<TupleVec> {
                                                        static_cast<void>(x);
                                                        return std::nullopt;
                                                    });
                                            }),
                        [](auto tuple) { return TermTuple::Element{std::move(tuple)}; });
                });
        });

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
            return unpool_crossproduct(tuple, [this](TupleElem const &elem) {
                return Util::visit_variant(
                    elem,
                    [this](Term const &term) -> std::optional<TupleVec> {
                        return Util::map_opt_vec(std::visit(*this, term),
                                                 [](auto term) { return TupleElem{std::move(term)}; });
                    },
                    [](std::monostate x) -> std::optional<TupleVec> {
                        static_cast<void>(x);
                        return std::nullopt;
                    });
            });
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
        return Util::map_opt_vec(std::visit(*this, *term.rhs), [&term](auto rhs) -> Term {
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
        return unpool_crossproduct(guards, [](Guard const &guard) {
            return Util::map_opt_vec(Gringo::Input::unpool(guard.second), [&guard](auto term) {
                return Guard{guard.first, std::move(term)};
            });
        });
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
        return Util::map_opt_vec(std::visit(*this, lit.term), [&lit](auto term) -> Literal {
            return LiteralSymbolic{lit.sign, std::move(term)};
        });
    }
};

} // namespace

auto unpool(Term const &term) -> std::optional<TermVec> { return std::visit(Unpool{}, term); }

auto unpool(Literal const &lit) -> std::optional<LiteralVec> { return std::visit(Unpool{}, lit); }

} // namespace Gringo::Input
