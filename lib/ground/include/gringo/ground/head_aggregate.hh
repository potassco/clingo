#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/small_vector.hh>

#include <memory_resource>
#include <ostream>

namespace Gringo::Ground {

//! Outline:
//! - A :- B.
//!   - atoms A are gathered in the aggregate domain
//!     (on lower priority than the elements)
//!   - there is no need to enqueue the aggregate
//!     (stratified aggregates should be handled specially later)
//! - accu :- A, E.
//!   - gathers aggregate elements
//!   - enqueues aggregate A for propagation
//! - propagate
//!   - if A can be derived, propagate heads of aggregate elements
//! - output
//!   - it might happen that there are empty aggregates that have not been derived
//!   - they should be handled here
//! - requirements
//!   - AtomHdAggr    (to collect head aggregates)
//!   - StateHdAggr   (to gather state for grounding/output)
//!   - StmHdAggr     (to derive atoms A)
//!   - StmHdAggrElem (to accumulate elements)
//!   - LitHdAggr     (to be used in StmHdAggrElem)

//! Derivation state of head aggregates.
enum class AtomHdAggrState : uint8_t {
    unknown = 0, //!< The aggregate has not yet been derived.
    derived = 1, //!< The aggregate has been derived.
};

//! Extensible ground representation of head aggregates.
class AtomHdAggr {
  public:
    //! The lower and upper bound for the value an aggregate can take.
    //!
    //! Aggregates operating on numbers do not use symbols to avoid storing
    //! unnecessary intermediate symbols.
    using Bound = std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>>;

    //! Initialize for the given aggregate function.
    AtomHdAggr(AggregateFunction fun) : bound_{init_(fun)} {}

    //! Accumulate a tuple.
    void accumulate(AggregateFunction fun, SymbolSpan tup, bool fact);

    //! Check if the aggregate matches the guards (first) and is a fact
    //! (second).
    //!
    //! Only the relation of the given non-ground guards is accessed; the
    //! values for the terms are stored in the aggregate atom.
    auto propagate(GuardVec const &guards, Symbol const *vals) -> std::pair<bool, bool>;

    //! Get the derived state of the aggregate atom.
    [[nodiscard]] auto state() const -> AtomHdAggrState;
    //! Set the derived state of the aggregate atom.
    //!
    //! It must only be derived once.
    void state(AtomHdAggrState state);

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

    //! Get the unique id of the aggregate.
    [[nodiscard]] auto uid() const -> std::optional<size_t>;
    //! Set the unique id of the aggregate.
    void uid(size_t uid);

  private:
    static auto init_(AggregateFunction fun) -> Bound;

    std::vector<size_t> elems_;
    std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>> bound_;
    size_t propagated_ = 0;
    size_t uid_ = invalid_offset;
    AtomHdAggrState state_ = AtomHdAggrState::unknown;
    bool enqueued_ = false;
};

//! A vector of signatures, bases, and indices.
//!
//! Whenever a base has an update, its indices have to be propagated. The base
//! is identified by the signature and the vector sorted by this signature.
using HdAggrBaseVec = std::vector<std::tuple<std::tuple<String, size_t, bool>, Base *, std::vector<size_t>>>;

//! State storing all necessary information to ground body aggregates.
class StateHdAggr {
  public:
    class AtomKey;
    //! Keys for aggregate elements storing their tuple and their aggregate index.
    //!
    //! The atom index is used to store all elements in one big hash table.
    class ElementKey {
      public:
        //! Prevent copying and moving.
        ElementKey(ElementKey const &other) = delete;
        //! Construct an element key evaluating the given tuple.
        [[nodiscard]] static auto construct(auto &mbr, SymbolStore &store, Assignment &ass, AggregateFunction fun,
                                            size_t atom_idx, UTermVec const &tuple, ElementKey *&target) -> bool;

        //! Mark as fact.
        void mark_fact() const;
        //! Check if is fact.
        [[nodiscard]] auto fact() const -> bool;

        //! The number of elements in the tuple.
        [[nodiscard]] auto size() const -> size_t;
        //! Get the tuple.
        [[nodiscard]] auto span() const -> SymbolSpan;
        //! Compute a hash for the key.
        [[nodiscard]] auto hash() const -> size_t;
        //! Compare to element keys.
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

      private:
        ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx, UTermVec const &tuple,
                   bool &res);

        // Note that these two could be combined to save a little bit of memory.
        mutable size_t n_;
        size_t atom_idx_;
        // NOLINTBEGIN
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms_[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };

    //! A map from global variables (including the guards) to the aggregate representation.
    using AtomMap = Util::ordered_map<Symbol const *, AtomHdAggr, Util::array_hash, Util::array_equal_to>;
    //! A map from tuples to their head atoms and conditions.
    //!
    //! Head atoms must be either true or symbolic atoms. We use \#sup to
    //! represent true. Whether an element is fact is stored in the key.
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<std::pair<Symbol, size_t>>>;

    //! Initialize an aggregate state.
    StateHdAggr(std::pmr::monotonic_buffer_resource &mbr, HdAggrBaseVec bases, VariableVec global, GuardVec guards,
                AggregateFunction fun, size_t index, bool single_pass_elems)
        : atoms_{0, global.size(), global.size()}, global_{std::move(global)}, guards_{std::move(guards)},
          bases_{std::move(bases)}, mbr_{&mbr}, index_{index}, fun_{fun}, single_pass_elems_{single_pass_elems} {}

    //! Get the global variables in the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get a buffer to store values for global variables.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Get the non-ground guards of the aggregate.
    [[nodiscard]] auto guards() const -> GuardVec const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that all necessary elemements can be grounded in a single
    //! pass.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate equeued aggregates.
    auto propagate() -> bool;

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(SymbolStore &store, Assignment &ass) -> std::optional<std::pair<AtomMap::iterator, bool>>;

    //! Insert a previously evaluated atom.
    //!
    //! This functions can be used to ensure the presence of an atom that has not yet been derived.
    auto insert_atom(Symbol const *tuple) -> AtomMap::iterator;

    //! Insert an aggregate element.
    void insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                     ElementKey *&elem_key, auto const &get_cond);

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out);

    //! Output all previously output aggregates.
    void output(OutputStm &out);

  private:
    //! Enequeue an atom for propgation.
    void enqueue_(AtomMap::iterator it);

    //! Get the index of an aggregate atom.
    //!
    //! This index also captures not yet derived atoms.
    auto atom_index_(AtomMap::iterator it) -> size_t;

    AtomMap atoms_;
    ElementMap tuples_;
    VariableVec global_;
    SymbolVec symbols_;
    GuardVec guards_;
    HdAggrBaseVec bases_;
    std::vector<size_t> queue_;
    std::pmr::monotonic_buffer_resource *mbr_;
    AtomKey *atom_key_ = nullptr;
    size_t index_;
    AggregateFunction fun_;
    bool single_pass_elems_;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate. Count aggregates have to be translated to sum+
//! aggregates beforehand.
class StmHdAggrElem : public Stm {
  public:
    //! Construct the statement.
    //!
    //! The first num_cond literals of the body must form the aggregate
    //! element's condition. The following literals are just used for grounding
    //! binding global variables of the aggregate and ensuring safety.
    StmHdAggrElem(StateHdAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, size_t priority)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, num_cond_{num_cond}, priority_{priority} {}

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override;
    void do_propagate(SymbolStore &store, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;

    StateHdAggr *state_;
    StateHdAggr::ElementKey *elem_key_ = nullptr;
    UTermVec tuple_;
    ULitVec body_;
    size_t num_cond_;
    size_t priority_;
};

} // namespace Gringo::Ground
