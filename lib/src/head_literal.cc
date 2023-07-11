#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <util/print.hh>

#include <head_literal.hh>

#include "cond_lits.hh"
#include "transform.hh"

namespace Gringo::Input {

////////// HeadLiteral //////////

namespace {

struct RewriteAnonymous {
    auto operator()(STerm const &term) const { return term->rewrite_anonymous(gen); }
    auto operator()(SLiteral const &lit) const { return lit->rewrite_anonymous(gen); }
    auto operator()(TheoryAtom const &aggr) -> std::optional<TheoryAtom> { return aggr.rewrite_anonymous(gen); };
    auto operator()(SetAggregate const &aggr) -> std::optional<SetAggregate> { return aggr.rewrite_anonymous(gen); };
    NameGen &gen;
};

struct ProjectAnonymous {
    auto operator()(SLiteral const &lit) const { return lit->project_anonymous(); }
    auto operator()(TheoryAtom const &aggr) -> std::optional<TheoryAtom> { return aggr.project_anonymous(); };
    auto operator()(SetAggregate const &aggr) -> std::optional<SetAggregate> { return aggr.project_anonymous(); };
};

auto tpa(auto const &x) { return Util::Trans(x, ProjectAnonymous{}); }

auto tra(auto const &x, NameGen &gen) { return Util::Trans(x, RewriteAnonymous{gen}); }

} // namespace

[[nodiscard]] auto HeadLiteral::print_empty() const -> bool { return false; }

[[nodiscard]] auto HeadLiteral::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, HeadLiteral const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

auto HeadLiteral::is_atom() const -> bool { return false; }

auto HeadLiteral::is_test() const -> bool { return false; }

auto HeadLiteral::is_classical() const -> bool { return false; }

////////// Disjunction //////////

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

void Disjunction::print(std::ostream &out) const { CondLits::print(elems_, out, "#or", true); }

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
    return Util::transform_construct_shared<Disjunction, HeadLiteral>(tpa(elems_));
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

auto Disjunction::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<Disjunction, HeadLiteral>(tra(elems_, gen));
}

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::print(std::ostream &out) const { out << atom_; }

auto HeadTheoryAtom::unpool() const -> std::optional<SHeadLiteralVec> {
    return map_opt_vec(atom_.unpool(),
                       [](auto atom) { return construct_shared<HeadTheoryAtom, HeadLiteral>(std::move(atom)); });
}

void HeadTheoryAtom::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    atom_.visit_variables(fun, ctx);
}

auto HeadTheoryAtom::project(Projection project) const -> std::optional<SHeadLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto HeadTheoryAtom::project_anonymous() const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadTheoryAtom, HeadLiteral>(tpa(atom_));
}

auto HeadTheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadTheoryAtom, HeadLiteral>(tra(atom_, gen));
}

////////// HeadAggregate //////////

void HeadAggregate::set_left_guard(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

void HeadAggregate::print(std::ostream &out) const {
    if (lhs_) {
        out << *lhs_->first << " " << lhs_->second << " ";
    }
    out << fun_ << " { " << p_range_with(elems_, "; ", [](std::ostream &out, auto const &elem) {
        out << p_range{std::get<0>(elem), ","} << ": " << *std::get<1>(elem);
        if (!std::get<2>(elem).empty()) {
            out << ": " << p_range{std::get<2>(elem), ", "};
        }
    }) << (elems_.empty() ? "}" : " }");
    if (rhs_) {
        out << " " << rhs_->first << " " << *rhs_->second;
    }
}

auto HeadAggregate::unpool() const -> std::optional<SHeadLiteralVec> {
    return unpool_crossproducts(
        [this](auto lhs, auto elem_lits, auto rhs) {
            return construct_shared<HeadAggregate, HeadLiteral>(std::move(lhs), fun_, std::move(elem_lits),
                                                                std::move(rhs));
        },
        overloaded{
            [](ElementVec const &elem_lits) -> std::optional<std::vector<ElementVec>> {
                return map_opt(
                    unpool_union(elem_lits,
                                 [](auto elem) {
                                     return unpool_crossproducts(
                                         [](auto tuple, auto lit, auto cond) {
                                             return Element{std::move(tuple), std::move(lit), std::move(cond)};
                                         },
                                         overloaded{[](STermVec const &tuple) { return unpool_crossproduct(tuple); },
                                                    [](SLiteral const &lit) { return lit->unpool(); },
                                                    [](SLiteralVec const &lits) { return unpool_crossproduct(lits); }},
                                         std::get<0>(elem), std::get<1>(elem), std::get<2>(elem));
                                 }),
                    [](auto elem_lits) { return make_vec<ElementVec>(std::move(elem_lits)); });
            },
            [](LGuard const &lhs) -> std::optional<std::vector<LGuard>> {
                return and_then_opt(lhs, [](auto const &lhs) {
                    return map_opt_vec(lhs.first->unpool(), [&lhs](auto term) {
                        return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
                    });
                });
            },
            [](RGuard const &rhs) -> std::optional<std::vector<RGuard>> {
                return and_then_opt(rhs, [](auto const &rhs) {
                    return map_opt_vec(rhs.second->unpool(), [&rhs](auto term) {
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
        return transform_construct<Element>(tuple, lit, Util::Trans(cond, fun));
    };
    return Util::transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, Util::Trans{elems_, fun}, rhs_);
}

auto HeadAggregate::project_anonymous() const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, tpa(elems_), rhs_);
}

auto HeadAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadAggregate, HeadLiteral>(tra(lhs_, gen), fun_, tra(elems_, gen),
                                                                        tra(rhs_, gen));
}

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

auto HeadSetAggregate::unpool() const -> std::optional<SHeadLiteralVec> {
    return map_opt_vec(aggr_.unpool(),
                       [](auto aggr) { return construct_shared<HeadSetAggregate, HeadLiteral>(std::move(aggr)); });
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
        return construct_shared<HeadSetAggregate, HeadLiteral>(std::move(projected).value());
    }
    return std::nullopt;
}

auto HeadSetAggregate::project_anonymous() const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadSetAggregate, HeadLiteral>(tpa(aggr_));
}

auto HeadSetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    return Util::transform_construct_shared<HeadSetAggregate, HeadLiteral>(tra(aggr_, gen));
}

} // namespace Gringo::Input
