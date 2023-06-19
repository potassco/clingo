#include <iterator>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

#include <util/print.hh>

#include <head_literal.hh>

#include "unpool_cond_lit.hh"

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

auto HeadLiteral::unpool() -> SHeadLiteralVec {
    SHeadLiteralVec head_lits;
    SLiteralVec lits;
    STermVec terms;
    PoolHeadLiteral pool{head_lits, lits, terms};
    unpool(pool);
    return head_lits;
}

////////// Disjunction //////////

auto Disjunction::is_simple_() const -> bool {
    auto lit_vars = VariableSet{};
    auto cond_vars = VariableSet{};
    return std::all_of(elems_.begin(), elems_.end(), [&](auto const &elem) {
        if (elem.first.size() != 1) {
            return false;
        }
        lit_vars.clear();
        cond_vars.clear();
        elem.first.front()->variables(lit_vars, VariableSelectMode::add);
        for (auto const &lit : elems_.front().second) {
            lit->variables(cond_vars, VariableSelectMode::add);
        }
        if (std::any_of(global_.begin(), global_.end(), [&](auto const &var) { return cond_vars.contains(var); })) {
            return false;
        }
        std::erase_if(lit_vars, [&](auto const &var) { return cond_vars.contains(var); });
        for (auto const &var : global_) {
            lit_vars.erase(var);
        }
        return lit_vars.empty();
    });
}

auto Disjunction::print_empty() const -> bool { return elems_.empty(); }

void Disjunction::print(std::ostream &out) const {
    // Note: almost the same as for conjunctions
    if (is_simple_()) {
        out << p_range_with(elems_, "; ", [](std::ostream &out, auto const &elem) {
            auto cs = elem.second.empty() ? "" : ": ";
            out << *elem.first.front() << cs << p_range(elem.second, ", ");
        });
    } else {
        char const *lp = global_.empty() ? "" : "(";
        char const *rp = global_.empty() ? "" : ")";
        char const *sp = elems_.empty() ? "" : " ";
        out << "#or" << lp << p_range(global_) << rp << " { "
            << p_range_with(elems_, "; ",
                            [&](std::ostream &out, Element const &elem) {
                                char const *cs = !elem.second.empty() ? ": " : elem.first.empty() ? ":" : "";
                                out << p_range(elem.first, ", ") << cs << p_range(elem.second, ", ");
                            })
            << sp << "}";
    }
}

void Disjunction::unpool(PoolHeadLiteral &pool) { unpool_cond_lits(this, pool, global_, elems_); }

////////// HeadTheoryAtom //////////

void HeadTheoryAtom::print(std::ostream &out) const { out << atom_; }

void HeadTheoryAtom::unpool(PoolHeadLiteral &pool) {
    atom_.unpool(pool, [&](std::optional<TheoryAtom> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadTheoryAtom>(std::move(aggr).value());
        }
    });
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

void HeadAggregate::unpool(PoolHeadLiteral &pool) {
    // unpool the aggregate elements
    std::optional<ElementVec> elems;
    size_t i = 0;
    for (auto &elem : elems_) {
        unpool_with(
            [&](std::optional<STermVec> &tuple, std::optional<SLiteral> &lit, std::optional<SLiteralVec> &cond) {
                if (!tuple.has_value() && !lit.has_value() && !cond.has_value() && !elems.has_value()) {
                    return;
                }
                if (!elems.has_value()) {
                    elems = ElementVec{elems_.begin(), elems_.begin() + i};
                }
                auto &[e_tuple, e_lit, e_cond] = elem;
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

////////// HeadSetAggregate //////////

void HeadSetAggregate::set_left_guard(STerm lhs, Relation rel) { aggr_.set_rhs(std::move(lhs), rel); }

void HeadSetAggregate::print(std::ostream &out) const { out << aggr_; }

void HeadSetAggregate::unpool(PoolHeadLiteral &pool) {
    aggr_.unpool(pool, [&](std::optional<SetAggregate> aggr) {
        if (!aggr.has_value()) {
            pool.append(this);
        } else {
            pool.append_shared<HeadSetAggregate>(std::move(aggr).value());
        }
    });
}
