#pragma once

#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/small_vector.hh>

#include <memory_resource>
#include <ostream>

namespace CppClingo::Ground {

// Outline:
// - A :- B.
//   - atoms A are gathered in the aggregate domain
//     (on lower priority than the elements)
//   - there is no need to enqueue the aggregate
//     - propagation only derives heads
//     - element accumulation is independent
// - accu :- A, E.
//   - gathers aggregate elements
//   - enqueues aggregate A for propagation
// - propagate
//   - if A can be derived, propagate heads of aggregate elements
// - output
//   - it might happen that there are aggregates that have not been derived
//   - they are currently output as is but their heads are not propagated
//   - in principle, one could stop grounding if such an aggregate has a true body
// - classes
//   - AtomHdAggr    (to collect head aggregates)
//   - BaseHdAggr    (the base for grounding)
//   - StateHdAggr   (to gather state for grounding/output)
//   - StmHdAggr     (to derive atoms A)
//   - StmHdAggrElem (to accumulate elements)
//   - LitHdAggr     (to be used in StmHdAggrElem)

//! @addtogroup ground_hdaggr
//! @{

//! Extensible ground representation of head aggregates.
//!
//! Elements can be added to this representation and it can be enqueued for
//! later propagation. Propagation adjust the stored bounds capturing an
//! interval of possible values the aggregate can take.
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

    //! Check if the aggregate matches the guards.
    //!
    //! Only the relation of the given non-ground guards is accessed; the
    //! values for the terms are stored in the aggregate atom.
    auto propagate(GuardVec const &guards, Symbol const *vals) -> bool;

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
    bool enqueued_ = false;
    bool matched_ = false;
};

//! The base capturing derived head aggregate atoms.
class BaseHdAggr : public BaseImpl<Symbol const *, BaseHdAggr> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomHdAggr, Util::array_hash, Util::array_equal_to>;

    //! Construct an empty base.
    BaseHdAggr(size_t size) : atoms_{0, size, size} {}

    //! Add an atom to the current generation.
    auto add(Symbol const *sym, AggregateFunction fun) -> std::pair<AtomMap::iterator, bool>;

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t;

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    [[nodiscard]] auto index(Symbol const *sym) const -> size_t;
    //! Get the i-th atom in the base.
    [[nodiscard]] auto nth(size_t i) const -> AtomMap::const_iterator;
    //! Get the i-th atom in the base.
    [[nodiscard]] auto nth(size_t i) -> AtomMap::iterator;

    //! Get the underlying atom map.
    [[nodiscard]] auto atoms() -> AtomMap &;

  private:
    AtomMap atoms_;
};

// Note: this could as well be a small container class
class StmHdAggrElem;

//! State storing all necessary information to ground head aggregates.
class StateHdAggr : public State {
  public:
    class AtomKey;
    //! Keys for aggregate elements storing their tuple and their aggregate index.
    //!
    //! The atom index is used to store all elements in one big hash table.
    class ElementKey {
      private:
        struct priv_tag {};

      public:
        //! Private constructor.
        ElementKey(priv_tag tag, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx, StmHdAggrElem &elem,
                   bool &res);
        //! Prevent copying and moving.
        ElementKey(ElementKey const &other) = delete;
        //! Construct an element key evaluating the given tuple.
        [[nodiscard]] static auto construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                            StmHdAggrElem &elem) -> bool;

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
        // Note that these two could be combined to save a little bit of memory.
        mutable size_t n_;
        size_t atom_idx_;
        // NOLINTBEGIN
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms_[0];
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };

    //! A map from global variables (including the guards) to the aggregate representation.
    using AtomMap = BaseHdAggr::AtomMap;
    //! A map from tuples to their head atoms and conditions.
    //!
    //! Head atoms must be either true or symbolic atoms. We use \#sup to
    //! represent true. Whether an element is fact is stored in the key.
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<std::tuple<Symbol, size_t, size_t>, 1>>;

    //! Initialize an aggregate state.
    StateHdAggr(std::pmr::monotonic_buffer_resource &mbr, BaseVec bases, VariableVec global, GuardVec guards,
                AggregateFunction fun, size_t index, bool single_pass_body)
        : base_{global.size()}, global_{std::move(global)}, guards_{std::move(guards)}, bases_{std::move(bases)},
          mbr_{&mbr}, index_{index}, fun_{fun}, single_pass_body_{single_pass_body} {}

    //! Get the global variables in the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get a buffer to store values for global variables.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Get the non-ground guards of the aggregate.
    [[nodiscard]] auto guards() const -> GuardVec const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that all necessary elements can be grounded in a single
    //! pass.
    [[nodiscard]] auto single_pass_body() const -> bool;
    //! Get the update index for the aggregate.
    [[nodiscard]] auto index() const -> size_t;
    //! Get the update indices for the heads of the elements.
    //!
    //! Unoptimized and intended for debug printing.
    [[nodiscard]] auto indices() const -> std::vector<size_t>;

    //! Enqueue aggregate element rules.
    void enqueue(Queue &queue);

    //! Propagate enqueued aggregates.
    void propagate(OutputStm &out, Queue &queue);

    //! Insert an aggregate atom.
    auto insert_atom(EvalContext const &ctx) -> std::optional<std::pair<AtomMap::iterator, bool>>;

    //! Insert an aggregate element.
    void insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmHdAggrElem &elem);

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out, bool print_index);

    //! Output all previously output aggregates.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;

    //! Return the base of the aggregate.
    [[nodiscard]] auto base() -> BaseHdAggr &;

  private:
    //! Enqueue an atom for propagation.
    void enqueue_(AtomMap::iterator it);

    //! Get the index of an aggregate atom.
    //!
    //! This index also captures not yet derived atoms.
    auto atom_index_(AtomMap::iterator it) -> size_t;

    BaseHdAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    SymbolVec symbols_;
    GuardVec guards_;
    BaseVec bases_;
    std::vector<size_t> queue_;
    std::pmr::monotonic_buffer_resource *mbr_;
    AtomKey *atom_key_ = nullptr;
    size_t index_;
    AggregateFunction fun_;
    bool single_pass_body_;
};

//! A statement deriving head aggregate atoms to trigger grounding of elements.
class StmHdAggr : public Stm {
  public:
    //! Construct the statement.
    StmHdAggr(StateHdAggr &state, Ground::ULitVec body, size_t priority)
        : state_{&state}, body_{std::move(body)}, priority_{priority} {}

  private:
    // Stm interface
    void do_print(std::ostream &out) const override;

    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;

    StateHdAggr *state_;
    ULitVec body_;
    size_t priority_;
};

//! Gather aggregate elements.
class StmHdAggrElem : public Stm {
  public:
    //! Construct the statement.
    StmHdAggrElem(StateHdAggr &state, std::optional<std::pair<UTerm, AtomBase *>> head, Location loc_weight,
                  UTermVec tuple, ULitVec body)
        : loc_weight_{std::move(loc_weight)}, state_{&state}, head_{head ? std::move(head->first) : nullptr},
          base_{head ? head->second : nullptr}, tuple_{std::move(tuple)}, body_{std::move(body)} {}

  private:
    friend class StateHdAggr;
    friend class StateHdAggr::ElementKey;

    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;

    auto get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool>;

    Location loc_weight_;
    StateHdAggr *state_;
    StateHdAggr::ElementKey *elem_key_ = nullptr;
    UTerm head_;
    AtomBase *base_;
    UTermVec tuple_;
    ULitVec body_;
    bool logged_ = false;
};

//! A term like object used to match head aggregates.
class MatchHdAggr {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchHdAggr(StateHdAggr &state) : state_{&state} {
        eval_.reserve(state_->global().size() + state_->guards().size());
    }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound, VariableSet const &bind) const -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Symbol const *sym) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Symbol const *>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchHdAggr const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateHdAggr &;

  private:
    std::vector<Symbol> mutable eval_;
    StateHdAggr *state_;
};

//! Literal representing an aggregate.
class LitHdAggr : public Lit, private MatchHdAggr {
  public:
    //! Construct the aggregate literal.
    LitHdAggr(StateHdAggr &state) : MatchHdAggr{state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    [[nodiscard]] auto do_domain() const -> bool override;

    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override;

    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;

    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;

    auto do_output(EvalContext const &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t offset_ = invalid_offset;
};

//! @}

} // namespace CppClingo::Ground
