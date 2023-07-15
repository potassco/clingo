#pragma once

#include <functional>
#include <optional>

#include <input/literal.hh>

namespace Gringo::Input {

enum class AggregateFunction {
    count,
    sum,
    sump,
    min,
    max,
};

using LGuard = std::optional<std::pair<TermV2, Relation>>;
using RGuard = std::optional<std::pair<Relation, TermV2>>;

inline auto reduct_is_nonmonotone(LGuard const &lhs, AggregateFunction fun, RGuard const &rhs) -> bool {
    if (!lhs.has_value() && !rhs.has_value()) {
        return false;
    }
    if (lhs.has_value() && lhs->second == Relation::inequal) {
        return true;
    }
    if (rhs.has_value() && rhs->first == Relation::inequal) {
        return true;
    }
    return fun == AggregateFunction::sum;
}

class SetAggregate {
  public:
    using Element = std::pair<SLiteral, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    SetAggregate(ElementVec elems) : elems_{std::move(elems)} {}
    SetAggregate(LGuard lhs, ElementVec elems, RGuard rhs)
        : elems_{std::move(elems)}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    SetAggregate(ElementVec elems, Relation rel, TermV2 rhs)
        : elems_{std::move(elems)}, rhs_(std::make_pair(rel, std::move(rhs))) {}

    //! Get the aggregate elements.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }
    //! Get the left-hand-side.
    [[nodiscard]] auto lhs() const -> LGuard const & { return lhs_; }
    //! Get the right-hand-side.
    [[nodiscard]] auto rhs() const -> RGuard const & { return rhs_; }

    void set_rhs(TermV2 lhs, Relation rel);
    [[nodiscard]] auto unpool() const -> std::optional<std::vector<SetAggregate>>;
    void visit_variables(std::function<void(std::string const &var)> fun, VariableContext ctx) const;
    /// Projects pure variables in the condition if the aggregate is not
    /// nonmonotone or occurs in a negative scope.
    [[nodiscard]] auto project(Projection project, bool in_negative_scope) const -> std::optional<SetAggregate>;
    [[nodiscard]] auto project_anonymous() const -> std::optional<SetAggregate>;

  private:
    ElementVec elems_;
    LGuard lhs_;
    RGuard rhs_;
};

} // namespace Gringo::Input
