#include <input/body_literal.hh>

#include <input/algo/project_anonymous.hh>

#include "algo/cond_lits.hh"
#include "algo/transform.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

////////// BodyLiteral //////////

namespace {

struct ProjectAnonymous {
    auto operator()(Literal const &lit) const { return project_anonymous(lit); }
    auto operator()(TheoryAtom const &aggr) -> std::optional<TheoryAtom> { return aggr.project_anonymous(); };
    auto operator()(SetAggregate const &aggr) -> std::optional<SetAggregate> { return aggr.project_anonymous(); };
};

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

} // namespace

////////// ConditionalLiteral //////////

void Conjunction::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void Conjunction::add_sign(Sign sign) {
    using Gringo::Input::add_sign;
    if (elems_.size() != 1 || elems_.front().first.size() != 1) {
        throw std::logic_error("there must be exactly one element");
    }
    add_sign(elems_.front().first.front(), sign);
}

auto Conjunction::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    // Note when to project:
    // - variables in premise if in classical scope,
    // - varibales in conclusion.
    return CondLits::project<Conjunction, BodyLiteral>(elems_, project, true, in_classical_scope);
}

auto Conjunction::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<Conjunction, BodyLiteral>(tpa(elems_));
}

////////// BodyAggregate //////////

void BodyAggregate::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void BodyAggregate::add_sign(Sign sign) { sign_ += sign; }

auto BodyAggregate::project(Projection prj, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    using Gringo::Input::project;
    if (sign_ == Sign::none && !in_classical_scope && reduct_is_nonmonotone(lhs_, fun_, rhs_)) {
        return std::nullopt;
    }

    auto fun = [prj](Element const &elem) -> std::optional<Element> {
        auto const &[lit, cond] = elem;

        // counts of local variables
        VarCounter counter{prj.counts()};
        counter.add(lit, cond);
        auto sub_prj = Projection{prj.mode(), counter};

        // project literals in condition
        auto fun = [sub_prj](Literal const &lit) { return project(lit, sub_prj); };
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto BodyAggregate::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, tpa(elems_), rhs_);
}

////////// BodySetAggregate //////////

void BodySetAggregate::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void BodySetAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodySetAggregate::set_left_guard(Term lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

auto BodySetAggregate::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    auto projected = aggr_.project(project, in_classical_scope || sign_ != Sign::none);
    if (projected.has_value()) {
        return Util::construct_shared<BodySetAggregate, BodyLiteral>(sign_, std::move(projected).value());
    }
    return std::nullopt;
}

auto BodySetAggregate::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodySetAggregate, BodyLiteral>(sign_, tpa(aggr_));
}

////////// BodyTheoryAtom //////////

void BodyTheoryAtom::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void BodyTheoryAtom::add_sign(Sign sign) { sign_ += sign; }

auto BodyTheoryAtom::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    static_cast<void>(project);
    static_cast<void>(in_classical_scope);
    return std::nullopt;
}

auto BodyTheoryAtom::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyTheoryAtom, BodyLiteral>(sign_, tpa(atom_));
}

} // namespace Gringo::Input
