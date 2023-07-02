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

////////// HeadLiteral //////////

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

auto HeadLiteral::unpool() const -> SHeadLiteralVec {
    SHeadLiteralVec head_lits;
    SLiteralVec lits;
    STermVec terms;
    PoolHeadLiteral pool{head_lits, lits, terms};
    unpool(pool);
    return head_lits;
}

auto HeadLiteral::is_atom() const -> bool { return false; }

auto HeadLiteral::is_test() const -> bool { return false; }

auto HeadLiteral::is_classical() const -> bool { return false; }

////////// Disjunction //////////

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

void Disjunction::print(std::ostream &out) const { CondLits::print(elems_, out, "#or", true); }

void Disjunction::unpool(PoolHeadLiteral &pool) const { CondLits::unpool(this, pool, elems_); }

void Disjunction::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    CondLits::visit_variables(elems_, fun, ctx);
}

auto Disjunction::project(Projection project) const -> std::optional<SHeadLiteral> {
    // Note when to project:
    // - variables in conditions (almost body literals)
    return CondLits::project<Disjunction, HeadLiteral>(elems_, project, false, true);
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
    auto fun = [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); };
    return transform_construct_shared<Disjunction, HeadLiteral>(Trans{elems_, fun});
}

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::print(std::ostream &out) const { out << atom_; }

void HeadTheoryAtom::unpool(PoolHeadLiteral &pool) const {
    atom_.unpool(pool, [&](std::optional<TheoryAtom> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadTheoryAtom>(std::move(aggr).value());
        }
    });
}

void HeadTheoryAtom::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    atom_.visit_variables(fun, ctx);
}

auto HeadTheoryAtom::project(Projection project) const -> std::optional<SHeadLiteral> { return std::nullopt; }

auto HeadTheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    auto fun = [&gen](TheoryAtom const &atom) { return atom.rewrite_anonymous(gen); };
    return transform_construct_shared<HeadTheoryAtom, HeadLiteral>(Trans{atom_, fun});
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

void HeadAggregate::unpool(PoolHeadLiteral &pool) const {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto const &elem : elems_) {
        unpool_with(
            [&](std::optional<STermVec> &tuple, std::optional<SLiteral> &lit, std::optional<SLiteralVec> &cond) {
                if (!tuple.has_value() && !lit.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto const &[e_tuple, e_lit, e_cond] = elem;
                elems->emplace_back(tuple.value_or(e_tuple), lit.value_or(e_lit), cond.value_or(e_cond));
            },
            unpool_crossproduct<PoolTerm>(pool, std::get<0>(elem)),
            unpool_element<PoolLiteral>(pool, std::get<1>(elem)),
            unpool_crossproduct<PoolLiteral>(pool, std::get<2>(elem)));
        ++i;
    }

    // unpool the guards and combine with the elements
    unpool_with(
        [&](std::optional<STerm> &lhs, std::optional<STerm> &rhs) {
            if (!lhs.has_value() && !rhs.has_value() && !elems.has_value()) {
                pool.append(this);
                return;
            }
            pool.append_shared<HeadAggregate>(
                lhs ? LGuard(std::in_place, lhs.value(), lhs_->second) : (lhs_ ? lhs_ : std::nullopt), fun_,
                elems.value_or(elems_),
                rhs ? RGuard(std::in_place, rhs_->first, rhs.value()) : (rhs_ ? rhs_ : std::nullopt));
        },
        unpool_element<PoolTerm, LGuard, UnpoolGuards>(pool, lhs_),
        unpool_element<PoolTerm, RGuard, UnpoolGuards>(pool, rhs_));
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
        auto sub_project = Projection{counter};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(tuple, lit, Trans(cond, fun));
    };
    return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, Trans{elems_, fun}, rhs_);
}

auto HeadAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    auto fun = overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return transform_construct_shared<HeadAggregate, HeadLiteral>(Trans{lhs_, fun}, fun_, Trans{elems_, fun},
                                                                  Trans{rhs_, fun});
}

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

void HeadSetAggregate::unpool(PoolHeadLiteral &pool) const {
    aggr_.unpool(pool, [&](std::optional<SetAggregate> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadSetAggregate>(std::move(aggr).value());
        }
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
        return construct_shared<HeadSetAggregate, HeadLiteral>(std::move(projected).value());
    }
    return std::nullopt;
}

auto HeadSetAggregate::rewrite_anonymous(NameGen &gen) const -> std::optional<SHeadLiteral> {
    auto fun = [&gen](SetAggregate const &aggr) { return aggr.rewrite_anonymous(gen); };
    return transform_construct_shared<HeadSetAggregate, HeadLiteral>(Trans{aggr_, fun});
}
