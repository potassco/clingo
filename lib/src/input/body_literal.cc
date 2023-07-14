#include <input/body_literal.hh>

#include "cond_lits.hh"
#include "transform.hh"
#include "variables.hh"

namespace Gringo::Input {

////////// BodyLiteral //////////

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

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

auto tra(auto const &x, NameGen &gen) { return Trans(x, RewriteAnonymous{gen}); }

} // namespace

auto BodyLiteral::is_atom() const -> bool { return false; }

auto BodyLiteral::is_test() const -> bool { return false; }

////////// ConditionalLiteral //////////

void Conjunction::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void Conjunction::add_sign(Sign sign) {
    if (elems_.size() != 1 || elems_.front().first.size() != 1) {
        throw std::logic_error("there must be exactly one element");
    }
    elems_.front().first.front()->add_sign(sign);
}

auto Conjunction::unpool() const -> std::optional<SBodyLiteralVec> {
    return CondLits::unpool<Conjunction, BodyLiteral>(elems_);
}

void Conjunction::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    CondLits::visit_variables(elems_, fun, ctx);
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

auto Conjunction::is_atom() const -> bool { return CondLits::is_atom(elems_); }

auto Conjunction::is_test() const -> bool { return CondLits::is_test(elems_); }

auto Conjunction::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<Conjunction, BodyLiteral>(tra(elems_, gen));
}

////////// BodyAggregate //////////

void BodyAggregate::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

void BodyAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodyAggregate::set_left_guard(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

auto BodyAggregate::unpool() const -> std::optional<SBodyLiteralVec> {
    return unpool_crossproducts(
        [this](auto lhs, auto elem_lits, auto rhs) {
            return Util::construct_shared<BodyAggregate, BodyLiteral>(sign_, std::move(lhs), fun_, std::move(elem_lits),
                                                                      std::move(rhs));
        },
        Util::overloaded{
            [](ElementVec const &elem_lits) -> std::optional<std::vector<ElementVec>> {
                return map_opt(
                    unpool_union(elem_lits,
                                 [](auto elem) {
                                     return unpool_crossproducts(
                                         [](auto tuple, auto cond) {
                                             return Element{std::move(tuple), std::move(cond)};
                                         },
                                         Util::overloaded{
                                             [](STermVec const &tuple) { return unpool_crossproduct(tuple); },
                                             [](SLiteralVec const &lits) { return unpool_crossproduct(lits); }},
                                         std::get<0>(elem), std::get<1>(elem));
                                 }),
                    [](auto elem_lits) { return make_vec<ElementVec>(std::move(elem_lits)); });
            },
            [](LGuard const &lhs) -> std::optional<std::vector<LGuard>> {
                return and_then_opt(lhs, [](auto const &lhs) {
                    return Util::map_opt_vec(lhs.first->unpool(), [&lhs](auto term) {
                        return std::make_optional<LGuard::value_type>(std::move(term), lhs.second);
                    });
                });
            },
            [](RGuard const &rhs) -> std::optional<std::vector<RGuard>> {
                return and_then_opt(rhs, [](auto const &rhs) {
                    return Util::map_opt_vec(rhs.second->unpool(), [&rhs](auto term) {
                        return std::make_optional<RGuard::value_type>(rhs.first, std::move(term));
                    });
                });
            },
        },
        lhs_, elems_, rhs_);
}

void BodyAggregate::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(lhs_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto BodyAggregate::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    if (sign_ == Sign::none && !in_classical_scope && reduct_is_nonmonotone(lhs_, fun_, rhs_)) {
        return std::nullopt;
    }

    auto fun = [project](Element const &elem) -> std::optional<Element> {
        auto const &[lit, cond] = elem;

        // counts of local variables
        VarCounter counter{project.counts()};
        counter.add(lit, cond);
        auto sub_project = Projection{project.mode(), counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto BodyAggregate::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, tpa(elems_), rhs_);
}

auto BodyAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, tra(lhs_, gen), fun_, tra(elems_, gen),
                                                                  tra(rhs_, gen));
}

////////// BodySetAggregate //////////

void BodySetAggregate::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

auto BodySetAggregate::unpool() const -> std::optional<SBodyLiteralVec> {
    return Util::map_opt_vec(aggr_.unpool(), [this](auto aggr) {
        return Util::construct_shared<BodySetAggregate, BodyLiteral>(sign_, std::move(aggr));
    });
}

void BodySetAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodySetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void BodySetAggregate::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    aggr_.visit_variables(std::move(fun), ctx);
}

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

auto BodySetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodySetAggregate, BodyLiteral>(sign_, tra(aggr_, gen));
}

////////// BodyTheoryAtom //////////

void BodyTheoryAtom::accept(BodyLiteralVisitor const &visitor) const { visitor.visit(*this); }

auto BodyTheoryAtom::unpool() const -> std::optional<SBodyLiteralVec> {
    return Util::map_opt_vec(atom_.unpool(), [this](auto atom) {
        return Util::construct_shared<BodyTheoryAtom, BodyLiteral>(sign_, std::move(atom));
    });
}

void BodyTheoryAtom::add_sign(Sign sign) { sign_ += sign; }

void BodyTheoryAtom::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    atom_.visit_variables(std::move(fun), ctx);
}

auto BodyTheoryAtom::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    static_cast<void>(project);
    static_cast<void>(in_classical_scope);
    return std::nullopt;
}

auto BodyTheoryAtom::project_anonymous() const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyTheoryAtom, BodyLiteral>(sign_, tpa(atom_));
}

auto BodyTheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    return transform_construct_shared<BodyTheoryAtom, BodyLiteral>(sign_, tra(atom_, gen));
}

} // namespace Gringo::Input
