#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/small_vector.hh>

namespace Gringo::Ground {

using GuardVec = std::vector<std::pair<Relation, UTerm>>;

enum class AtomAssignAggrState : uint8_t {
    unknown = 0,
    derived = 1,
    fact = 2,
};

class AtomAssignAggr {
  public:
    using Bound = std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>>;

    //! Initialize for the given aggregate function.
    AtomAssignAggr() = default;

    //! Accumulate a tuple.
    void accumulate(AggregateFunction fun, SymbolSpan tup, bool fact);

    //! Check if the aggregate matches the guards (first) and is a fact
    //! (second).
    //!
    //! Only the relation of the given non-ground guards is accessed; the
    //! values for the terms are stored in the aggregate atom.
    auto propagate(GuardVec const &guards, Symbol const *vals) -> std::pair<bool, bool>;

    //! Get the index of the aggregate.
    //!
    //! It must be derived first.
    [[nodiscard]] auto derived_idx() const -> size_t;
    //! Set the derived index of the aggregate.
    //!
    //! It must be derived first.
    void derived_idx(size_t idx);

    //! Get the derived state of the aggregate atom.
    [[nodiscard]] auto state() const -> AtomAssignAggrState;
    //! Set the derived state of the aggregate atom.
    //!
    //! It must only be derived once.
    void state(AtomAssignAggrState state);

    //! Enqueue the atom for propagation.
    auto enqueue() -> bool;
    //! Dequeue the atom after propagation.
    //!
    //! Also marks elements as propagated.
    void dequeue();

    //! Add a new element.
    void add_elem(size_t idx);
    //! Get the aggregate elements.
    [[nodiscard]] auto elems() const -> std::span<size_t const>;
    //! Get the aggregate elements to propagate.
    [[nodiscard]] auto todo() -> std::span<size_t const>;

    [[nodiscard]] auto uid() const -> std::optional<size_t>;

    void uid(size_t uid);

  private:
    static auto init_(AggregateFunction fun) -> Bound;

    std::vector<size_t> elems_;
    // has to be an ordered set
    // propagated captures the number of elements already propagated
    // adding facts creates a new ordered set
    // propagated elements can be moved to front (won't really happen in practice)
    std::variant<std::monostate, std::pair<Number, Number>, std::pair<Symbol, Symbol>> bound_;
    size_t propagated_ = 0;
    // probably unnecessary
    size_t derived_idx_ = 0;
    size_t uid_ = invalid_offset;
    AtomAssignAggrState state_ = AtomAssignAggrState::unknown;
    bool enqueued_ = false;
};

} // namespace Gringo::Ground
