#include <iterator>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <input/head_literal.hh>

#include <input/algo/project.hh>
#include <input/algo/project_anonymous.hh>

#include "algo/cond_lits.hh"
#include "algo/transform.hh"

namespace Gringo::Input {

////////// HeadLiteral //////////

namespace {

struct ProjectAnonymous {
    auto operator()(Literal const &lit) const { return project_anonymous(lit); }
    auto operator()(TheoryAtom const &aggr) -> std::optional<TheoryAtom> { return aggr.project_anonymous(); };
    auto operator()(SetAggregate const &aggr) -> std::optional<SetAggregate> { return aggr.project_anonymous(); };
};

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

} // namespace

[[nodiscard]] auto HeadLiteral::print_empty() const -> bool { return false; }

////////// Disjunction //////////

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

auto Disjunction::project(Projection project) const -> std::optional<SHeadLiteral> {
    // Note when to project:
    // - variables in conditions (almost body literals)
    return CondLits::project<Disjunction, HeadLiteral>(elems_, project, false, true);
}

auto Disjunction::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<Disjunction, HeadLiteral>(tpa(elems_));
}

////////// HeadTheoryAtom //////////

auto HeadTheoryAtom::project(Projection project) const -> std::optional<SHeadLiteral> {
    static_cast<void>(project);
    return std::nullopt;
}

auto HeadTheoryAtom::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<HeadTheoryAtom, HeadLiteral>(tpa(atom_));
}

////////// HeadAggregate //////////

auto HeadAggregate::project(Projection prj) const -> std::optional<SHeadLiteral> {
    using Gringo::Input::project;
    auto fun = [prj](Element const &elem) -> std::optional<Element> {
        auto const &[tuple, lit, cond] = elem;

        // counts of local variables
        VarCounter counter{prj.counts()};
        counter.add(tuple, lit, cond);
        auto sub_project = Projection{prj.mode(), counter};

        // project literals in condition
        auto fun = [sub_project](Literal const &lit) { return project(lit, sub_project); };
        return transform_construct<Element>(tuple, lit, Trans(cond, fun));
    };
    return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto HeadAggregate::project_anonymous() const -> std::optional<SHeadLiteral> {
    return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, tpa(elems_), rhs_);
}

////////// HeadSetAggregate //////////

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
