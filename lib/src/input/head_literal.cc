#include <iterator>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <input/head_literal.hh>

#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "algo/cond_lits.hh"
#include "algo/transform.hh"

namespace Gringo::Input {

////////// HeadLiteral //////////

namespace {

struct ProjectAnonymous {
    auto operator()(SLiteral const &lit) const { return lit->project_anonymous(); }
    auto operator()(TheoryAtom const &aggr) -> std::optional<TheoryAtom> { return aggr.project_anonymous(); };
    auto operator()(SetAggregate const &aggr) -> std::optional<SetAggregate> { return aggr.project_anonymous(); };
};

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

} // namespace

[[nodiscard]] auto HeadLiteral::print_empty() const -> bool { return false; }

auto HeadLiteral::is_atom() const -> bool { return false; }

auto HeadLiteral::is_test() const -> bool { return false; }

auto HeadLiteral::is_classical() const -> bool { return false; }

////////// Disjunction //////////

void Disjunction::accept(HeadLiteralVisitor const &visitor) const { visitor.visit(*this); }

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

auto Disjunction::unpool() const -> std::optional<SHeadLiteralVec> {
    return CondLits::unpool<Disjunction, HeadLiteral>(elems_);
}

void Disjunction::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    CondLits::visit_variables(elems_, fun, ctx);
}

auto Disjunction::project(Projection project) const -> std::optional<SHeadLiteral> {
    // Note when to project:
    // - variables in conditions (almost body literals)
    return CondLits::project<Disjunction, HeadLiteral>(elems_, project, false, true);
}

auto Disjunction::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<Disjunction, HeadLiteral>(tpa(elems_));
}

auto Disjunction::is_atom() const -> bool { return CondLits::is_atom(elems_); }

auto Disjunction::is_test() const -> bool { return CondLits::is_test(elems_); }

auto Disjunction::is_classical() const -> bool {
    for (auto const &elem : elems_) {
        for (auto const &lit : elem.first) {
            if (lit->is_atom()) {
                return false;
            }
        }
    }
    return true;
}

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::accept(HeadLiteralVisitor const &visitor) const { visitor.visit(*this); }

auto HeadTheoryAtom::unpool() const -> std::optional<SHeadLiteralVec> {
    return Util::map_opt_vec(
        atom_.unpool(), [](auto atom) { return Util::construct_shared<HeadTheoryAtom, HeadLiteral>(std::move(atom)); });
}

void HeadTheoryAtom::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    atom_.visit_variables(fun, ctx);
}

auto HeadTheoryAtom::project(Projection project) const -> std::optional<SHeadLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto HeadTheoryAtom::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<HeadTheoryAtom, HeadLiteral>(tpa(atom_));
}

////////// HeadAggregate //////////

void HeadAggregate::accept(HeadLiteralVisitor const &visitor) const { visitor.visit(*this); }

void HeadAggregate::set_left_guard(Term lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

auto HeadAggregate::unpool() const -> std::optional<SHeadLiteralVec> {
    using Gringo::Input::unpool;
    return unpool_crossproducts(
        [this](auto lhs, auto elem_lits, auto rhs) {
            return Util::construct_shared<HeadAggregate, HeadLiteral>(std::move(lhs), fun_, std::move(elem_lits),
                                                                      std::move(rhs));
        },
        Util::overloaded{
            [](ElementVec const &elem_lits) -> std::optional<std::vector<ElementVec>> {
                return map_opt(
                    unpool_union(elem_lits,
                                 [](auto elem) {
                                     return unpool_crossproducts(
                                         [](auto tuple, auto lit, auto cond) {
                                             return Element{std::move(tuple), std::move(lit), std::move(cond)};
                                         },
                                         Util::overloaded{
                                             [](TermVec const &tuple) {
                                                 return unpool_crossproduct(
                                                     tuple, [](auto const &term) { return unpool(term); });
                                             },
                                             [](SLiteral const &lit) { return lit->unpool(); },
                                             [](SLiteralVec const &lits) { return unpool_crossproduct(lits); }},
                                         std::get<0>(elem), std::get<1>(elem), std::get<2>(elem));
                                 }),
                    [](auto elem_lits) { return make_vec<ElementVec>(std::move(elem_lits)); });
            },
            [](LGuard const &lhs) -> std::optional<std::vector<LGuard>> {
                return Util::and_then_opt(lhs, [](auto const &lhs) {
                    return Util::map_opt_vec(unpool(lhs.first), [&lhs](auto term) {
                        return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
                    });
                });
            },
            [](RGuard const &rhs) -> std::optional<std::vector<RGuard>> {
                return Util::and_then_opt(rhs, [](auto const &rhs) {
                    return Util::map_opt_vec(unpool(rhs.second), [&rhs](auto term) {
                        return std::make_optional<RGuard::value_type>(rhs.first, std::move(term));
                    });
                });
            },
        },
        lhs_, elems_, rhs_);
}

void HeadAggregate::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(lhs_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto HeadAggregate::project(Projection project) const -> std::optional<SHeadLiteral> {
    auto fun = [project](Element const &elem) -> std::optional<Element> {
        auto const &[tuple, lit, cond] = elem;

        // counts of local variables
        VarCounter counter{project.counts()};
        counter.add(tuple, lit, cond);
        auto sub_project = Projection{project.mode(), counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(tuple, lit, Trans(cond, fun));
    };
    return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto HeadAggregate::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, tpa(elems_), rhs_);
}

////////// HeadSetAggregate //////////

void HeadSetAggregate::accept(HeadLiteralVisitor const &visitor) const { visitor.visit(*this); }

void HeadSetAggregate::set_left_guard(Term lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

auto HeadSetAggregate::unpool() const -> std::optional<SHeadLiteralVec> {
    return Util::map_opt_vec(aggr_.unpool(), [](auto aggr) {
        return Util::construct_shared<HeadSetAggregate, HeadLiteral>(std::move(aggr));
    });
}

void HeadSetAggregate::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    aggr_.visit_variables(std::move(fun), ctx);
}

auto HeadSetAggregate::project(Projection project) const -> std::optional<SHeadLiteral> {
    // Note that we can always project in conditions. Semantic-wise a head
    // aggregate is a shortcut for a choice rule + a body aggregate in an
    // integrity constraint.
    auto projected = aggr_.project(project, true);
    if (projected.has_value()) {
        return Util::construct_shared<HeadSetAggregate, HeadLiteral>(std::move(projected).value());
    }
    return std::nullopt;
}

auto HeadSetAggregate::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<HeadSetAggregate, HeadLiteral>(tpa(aggr_));
}

} // namespace Gringo::Input
