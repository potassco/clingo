#include <sstream>

#include <util/print.hh>

#include <body_literal.hh>

#include "cond_lits.hh"
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

////////// ConditionalLiteral //////////

void Conjunction::add_sign(Sign sign) {
    if (elems_.size() != 1 || elems_.front().first.size() != 1) {
        throw std::logic_error("there must be exactly one element");
    }
    elems_.front().first.front()->add_sign(sign);
}

void Conjunction::print(std::ostream &out) const { print_cond_lits(elems_, global_, out, "#and", false); }

void Conjunction::unpool(PoolBodyLiteral &pool) { unpool_cond_lits(this, pool, global_, elems_); }

void Conjunction::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    cond_visit_variables(elems_, global_, std::move(fun), ctx);
}

auto Conjunction::project(Projection project) -> SBodyLiteral {
    // Note: that we can only project global variables in succeedents here.
    std::optional<ElementVec> elems;
    size_t n = 0;
    for (auto const &[lits, cond] : elems_) {
        // project literals in succeedent
        size_t m = 0;
        if (elems.has_value()) {
            elems->emplace_back(copy_n(lits, m), cond);
        }
        for (auto const &lit : lits) {
            auto projected_lit = lit->project(project);
            if (projected_lit != lit && !elems.has_value()) {
                elems = copy_n(elems_, n);
                elems->emplace_back(copy_n(lits, m), cond);
            }
            if (elems.has_value()) {
                elems->back().first.emplace_back(projected_lit);
            }
            ++m;
        }
        ++n;
    }
    if (elems.has_value()) {
        return construct_shared<Conjunction, BodyLiteral>(global_, std::move(elems).value());
    }
    return SBodyLiteral{this};
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

auto BodyAggregate::project(Projection project) -> SBodyLiteral {
    // FIXME: only project convex aggregates/or aggregates in scope of negation:
    //   :- #aggr {...} != n
    //   H :- not #aggr {...} != n
    //   H :- not not #aggr {...} != n
    //   H :- #sum+ {...} > n.
    // TODO: same for body set aggregates
    // TODO: do not project in `not not p(X)`
    std::optional<ElementVec> elems;
    size_t n = 0;
    for (auto const &[tuple, cond] : elems_) {
        // counts of local variables
        VarCounter counter{project.counts()};
        counter.add(tuple, cond);
        auto sub_project = Projection{counter};

        // project literals in condition
        size_t m = 0;
        if (elems.has_value()) {
            elems->emplace_back(tuple, copy_n(cond, m));
        }
        for (auto const &lit : cond) {
            auto projected_lit = lit->project(sub_project);
            if (projected_lit != lit && !elems.has_value()) {
                elems = copy_n(elems_, n);
                elems->emplace_back(tuple, copy_n(cond, m));
            }
            if (elems.has_value()) {
                std::get<1>(elems->back()).emplace_back(projected_lit);
            }
            ++m;
        }
        ++n;
    }
    if (elems.has_value()) {
        return construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, std::move(elems).value(), rhs_);
    }
    return SBodyLiteral{this};
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

auto BodySetAggregate::project(Projection project) -> SBodyLiteral {
    auto projected = aggr_.project(project, true);
    if (projected.has_value()) {
        return construct_shared<BodySetAggregate, BodyLiteral>(sign_, std::move(projected).value());
    }
    return SBodyLiteral{this};
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

auto BodyTheoryAtom::project(Projection project) -> SBodyLiteral { return SBodyLiteral{this}; }
