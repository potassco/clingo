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
    using Values = std::variant<Util::small_vector<Number>, Util::small_vector<Symbol>>;

    //! Initialize for the given aggregate function.
    AtomAssignAggr(AggregateFunction fun) : values_{init_(fun)} {}

    //! Accumulate a tuple.
    void accumulate(AggregateFunction fun, SymbolSpan tup, bool fact);

    //! Check if the aggregate in its current state is a fact.
    [[nodiscard]] auto is_fact() const;
    //! Enqueue the atom for propagation.
    auto enqueue_vals() -> bool;
    //! Dequeue the atom after propagation.
    //!
    //! Also marks elements as propagated.
    void dequeue_vals();
    //! Get the values to propagate.
    [[nodiscard]] auto todo_values() -> std::variant<NumberSpan, SymbolSpan>;

    //! Add a new element.
    void add_elem(size_t idx);
    //! Get the aggregate elements.
    [[nodiscard]] auto elems() const -> std::span<size_t const>;
    //! Enqueue aggregate to propagate its elements.
    [[nodiscard]] auto enqueue() -> bool;
    //! Dequeue an aggregate whose elements have been propagated.
    void dequeue();
    //! Get the elements that have to be propagated.
    [[nodiscard]] auto todo() -> std::span<size_t const>;

  private:
    static auto init_(AggregateFunction fun) -> Values;

    std::vector<size_t> elems_;
    Values values_;
    size_t propagated_vals_ = 0;
    size_t propagated_ = 0;
    bool enqueued_vals_ = false;
    bool enqueued_ = false;
};

class BaseAssignAggr : public BaseImpl<std::pair<size_t, Symbol>, BaseAssignAggr> {
  public:
    using BaseImpl::contains;
    using BaseImpl::Key;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomAssignAggr, Util::SpanHash, Util::SpanEqualTo>;
    //! Map containing the derived atoms and their values.
    using AtomSet = Util::ordered_set<Key>;

    //! Construct an empty base.
    BaseAssignAggr(size_t size, bool single_pass_elems)
        : atoms_{0, size, size}, single_pass_elems_{single_pass_elems} {}

    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    [[nodiscard]] auto is_fact(Key sym) const -> bool;
    //! Add an atom to the base.
    //!
    //! This function should be called during propagation if an aggregate can match.
    void add(size_t idx, Symbol val);

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t;

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    [[nodiscard]] auto index(Key sym) const -> size_t;
    //! Get the i-th atom in the base.
    [[nodiscard]] auto nth(size_t i) const -> AtomSet::const_iterator;
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> AtomSet::iterator;

    //! Get the underlying atoms.
    [[nodiscard]] auto atoms() -> AtomMap &;

  private:
    AtomMap atoms_;
    AtomSet derived_;
    bool single_pass_elems_;
};

class StateAssignAggr {
  public:
    // NOLINTBEGIN
    struct ElementKey {
        ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx, UTermVec const &tuple,
                   bool &res);

        auto span() const -> SymbolSpan;
        auto hash() const -> size_t;
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

        // Note that these two could be combined to save a little bit of memory.
        size_t n;
        size_t atom_idx;
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
    };

    struct AtomKey {
        AtomKey(SymbolStore &store, Assignment &ass, VariableVec const &global, GuardVec &guards, bool &res);

        AtomKey(Symbol const *tuple, size_t n);
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
    };
    // NOLINTEND

    using AtomMap = BaseAssignAggr::AtomMap;
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateAssignAggr(VariableVec global, UTerm term, AggregateFunction fun, size_t index, bool single_pass_elems)
        : base_{global.size(), single_pass_elems}, global_{std::move(global)}, term_{std::move(term)}, index_{index},
          fun_{fun} {}

    //! Get the global variables in the aggregate.
    //!
    //! This does not include the guard of the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get the target term to assign values to.
    [[nodiscard]] auto term() const -> Term const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that all necessary elemements can be grounded in a single
    //! pass.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate equeued aggregates.
    auto propagate() -> bool;

    //! Enqueue an atom for propgation.
    void enqueue(AtomMap::iterator it);

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(SymbolStore &store, Assignment &ass) -> std::optional<AtomMap::iterator>;

    //! Insert an aggregate element.
    void insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                     auto const &get_cond);

    //! Get the index of an aggregate atom.
    auto index(AtomMap::iterator it) -> size_t;

    //! Get the index of an aggregate element.
    auto index(ElementMap::iterator it) -> size_t;

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out);

    //! Get the underlying atom base.
    [[nodiscard]] auto base() -> BaseAssignAggr &;

    //! Output all previously grounded aggregates.
    void output(OutputStm &out);

  private:
    Util::NodeStore<alignof(Symbol)> node_store_;
    BaseAssignAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    UTerm term_;
    std::vector<size_t> queue_;
    size_t index_;
    AggregateFunction fun_;
};
} // namespace Gringo::Ground
