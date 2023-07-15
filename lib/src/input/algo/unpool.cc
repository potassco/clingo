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

    auto operator()(Util::shared_ptr<TermTuple> const &term) const -> std::optional<TermVec> {
        // unpool the elements
        auto elems = unpool_union(term->pool, [this](TermTuple::Element const &tuple_or_term) {
            return Util::visit_variant(
                tuple_or_term,
                [this](Term const &term) -> std::optional<TermTuple::ElementVec> {
                    return map_opt_vec(std::visit(*this, term),
                                       [](auto term) { return TermTuple::Element{std::move(term)}; });
                },
                [this](TupleVec const &tuple) -> std::optional<TermTuple::ElementVec> {
                    return map_opt_vec(unpool_crossproduct(tuple,
                                                           [this](TupleElem const &elem) {
                                                               return Util::visit_variant(
                                                                   elem,
                                                                   [this](Term const &term) -> std::optional<TupleVec> {
                                                                       return map_opt_vec(
                                                                           std::visit(*this, term), [](auto term) {
                                                                               return TupleElem{std::move(term)};
                                                                           });
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
        if (!elems.has_value() && (term->pool.size() != 1 || std::holds_alternative<Term>(term->pool.front()))) {
            elems = term->pool;
        }
        return map_opt_vec(std::move(elems), [](auto elem) -> Term {
            return Util::visit_variant(
                std::move(elem), [](Term term) { return term; },
                [](TupleVec tuple) -> Term {
                    return Util::construct_shared<TermTuple>(TermTuple::ElementVec{std::move(tuple)});
                });
        });
    }

    auto operator()(Util::shared_ptr<TermAbs> const &term) const -> std::optional<TermVec> {
        auto unpooled = unpool_union(term->pool, *this);
        if (!unpooled.has_value() && term->pool.size() != 1) {
            unpooled = term->pool;
        }
        return map_opt_vec(std::move(unpooled),
                           [](auto term) -> Term { return Util::construct_shared<TermAbs>(TermVec{std::move(term)}); });
    }

    auto operator()(Util::shared_ptr<TermFunction> const &term) const -> std::optional<TermVec> {
        auto elems = unpool_union(term->pool, [this](TupleVec const &tuple) {
            // unpool the elements
            return unpool_crossproduct(tuple, [this](TupleElem const &elem) {
                return Util::visit_variant(
                    elem,
                    [this](Term const &term) -> std::optional<TupleVec> {
                        return map_opt_vec(std::visit(*this, term),
                                           [](auto term) { return TupleElem{std::move(term)}; });
                    },
                    [](std::monostate x) -> std::optional<TupleVec> {
                        static_cast<void>(x);
                        return std::nullopt;
                    });
            });
        });

        if (!elems.has_value() && term->pool.size() != 1) {
            elems = term->pool;
        }

        return map_opt_vec(std::move(elems), [&term](auto elem) -> Term {
            // turn individual elements into function terms
            return Util::construct_shared<TermFunction>(term->name, PoolVec{std::move(elem)}, term->external);
        });
    }

    auto operator()(Util::shared_ptr<TermUnary> const &term) const -> std::optional<TermVec> {
        return map_opt_vec(std::visit(*this, term->rhs), [&term](auto rhs) -> Term {
            return Util::construct_shared<TermUnary>(term->op, std::move(rhs));
        });
    }

    auto operator()(Util::shared_ptr<TermBinary> const &term) const -> std::optional<TermVec> {
        return unpool_crossproducts(
            [&term](Term lhs, Term rhs) -> Term {
                return Util::construct_shared<TermBinary>(std::move(lhs), term->op, std::move(rhs));
            },
            *this, term->lhs, term->rhs);
    }
};

} // namespace

auto unpool(Term const &term) -> std::optional<TermVec> { return std::visit(Unpool{}, term); }

} // namespace Gringo::Input
