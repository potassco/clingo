#include <sstream>

#include <util/print.hh>

#include <body_literal.hh>

#include "cond_lits.hh"
#include "transform.hh"
#include "variables.hh"

////////// BodyLiteral //////////

auto BodyLiteral::unpool() -> SBodyLiteralVec {
    SBodyLiteralVec body_lits;
    SLiteralVec lits;
    STermVec terms;
    PoolBodyLiteral pool{body_lits, lits, terms};
    unpool(pool);
    return body_lits;
}

[[nodiscard]] auto BodyLiteral::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, BodyLiteral const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

auto BodyLiteral::is_atom() const -> bool { return false; }

auto BodyLiteral::is_test() const -> bool { return false; }

////////// ConditionalLiteral //////////

void Conjunction::add_sign(Sign sign) {
    if (elems_.size() != 1 || elems_.front().first.size() != 1) {
        throw std::logic_error("there must be exactly one element");
    }
    elems_.front().first.front()->add_sign(sign);
}

void Conjunction::print(std::ostream &out) const { CondLits::print(elems_, out, "#and", false); }

void Conjunction::unpool(PoolBodyLiteral &pool) { CondLits::unpool(this, pool, elems_); }

void Conjunction::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    CondLits::visit_variables(elems_, fun, ctx);
}

auto Conjunction::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    // Note when to project:
    // - variables in premise if in classical scope,
    // - varibales in conclusion.
    return CondLits::project<Conjunction, BodyLiteral>(elems_, project, true, in_classical_scope);
}

auto Conjunction::is_atom() const -> bool { return CondLits::is_atom(elems_); }

auto Conjunction::is_test() const -> bool { return CondLits::is_test(elems_); }

auto Conjunction::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    auto fun = [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); };
    return transform_construct_shared<Conjunction, BodyLiteral>(Trans{elems_, fun});
}

////////// BodyAggregate //////////

void BodyAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodyAggregate::set_left_guard(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

void BodyAggregate::print(std::ostream &out) const {
    out << sign_;
    if (lhs_) {
        out << *lhs_->first << " " << lhs_->second << " ";
    }
    out << fun_ << " { " << p_range_with(elems_, "; ", [](std::ostream &out, auto const &elem) {
        out << p_range{std::get<0>(elem), ","};
        if (!std::get<1>(elem).empty()) {
            out << ": " << p_range{std::get<1>(elem), ", "};
        }
    }) << (elems_.empty() ? "}" : " }");
    if (rhs_) {
        out << " " << rhs_->first << " " << *rhs_->second;
    }
}

void BodyAggregate::unpool(PoolBodyLiteral &pool) {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<STermVec> &tuple, std::optional<SLiteralVec> &cond) {
                if (!tuple.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[e_tuple, e_cond] = elem;
                elems->emplace_back(tuple.value_or(e_tuple), cond.value_or(e_cond));
            },
            unpool_crossproduct<PoolTerm>(pool, std::get<0>(elem)),
            unpool_crossproduct<PoolLiteral>(pool, std::get<1>(elem)));
        ++i;
    }

    // unpool the guards and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value() && !elems.has_value()) {
                pool.append(this);
                return;
            }
            pool.append_shared<BodyAggregate>(
                sign_, lhs ? LGuard(std::in_place, lhs.value(), lhs_->second) : (lhs_ ? lhs_ : std::nullopt), fun_,
                elems.value_or(elems_),
                rhs ? RGuard(std::in_place, rhs_->first, rhs.value()) : (rhs_ ? rhs_ : std::nullopt));
        },
        unpool_element<PoolTerm, LGuard, UnpoolGuards>(pool, lhs_),
        unpool_element<PoolTerm, RGuard, UnpoolGuards>(pool, rhs_));
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
        auto sub_project = Projection{counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(lit, Trans(cond, fun));
    };
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto BodyAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    auto fun = overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, Trans{lhs_, fun}, fun_, Trans{elems_, fun},
                                                                  Trans{rhs_, fun});
}

////////// BodySetAggregate //////////

void BodySetAggregate::unpool(PoolBodyLiteral &pool) {
    aggr_.unpool(pool, [&](std::optional<SetAggregate> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<BodySetAggregate>(std::move(aggr).value());
        }
    });
}

void BodySetAggregate::add_sign(Sign sign) { sign_ += sign; }

void BodySetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void BodySetAggregate::print(std::ostream &out) const { out << sign_ << aggr_; }

void BodySetAggregate::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    aggr_.visit_variables(std::move(fun), ctx);
}

auto BodySetAggregate::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    auto projected = aggr_.project(project, in_classical_scope || sign_ != Sign::none);
    if (projected.has_value()) {
        return construct_shared<BodySetAggregate, BodyLiteral>(std::move(projected).value());
    }
    return std::nullopt;
}

auto BodySetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    auto fun = [&gen](SetAggregate const &aggr) { return aggr.rewrite_anonymous(gen); };
    return transform_construct_shared<BodySetAggregate, BodyLiteral>(sign_, Trans{aggr_, fun});
}

////////// BodyTheoryAtom //////////

void BodyTheoryAtom::unpool(PoolBodyLiteral &pool) {
    atom_.unpool(pool, [&](std::optional<TheoryAtom> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<BodyTheoryAtom>(sign_, std::move(aggr).value());
        }
    });
}

void BodyTheoryAtom::add_sign(Sign sign) { sign_ += sign; }

void BodyTheoryAtom::print(std::ostream &out) const { out << sign_ << atom_; }

void BodyTheoryAtom::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    atom_.visit_variables(std::move(fun), ctx);
}

auto BodyTheoryAtom::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
    static_cast<void>(project);
    static_cast<void>(in_classical_scope);
    return std::nullopt;
}

auto BodyTheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<SBodyLiteral> {
    auto fun = [&gen](SetAggregate const &aggr) { return aggr.rewrite_anonymous(gen); };
    return transform_construct_shared<BodyTheoryAtom, BodyLiteral>(sign_, Trans{atom_, fun});
}
