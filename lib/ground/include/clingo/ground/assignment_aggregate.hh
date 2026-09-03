#pragma once

#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/small_vector.hh>

#include <memory_resource>

namespace CppClingo::Ground {

//! @addtogroup ground_assignaggr
//! @{

//! Extensible ground representation of assignment aggregates.
class AtomAssignAggr {
  public:
    //! The possible values an assignment aggregate can take.
    //!
    //! Aggregates operating on numbers do not use symbols to avoid storing
    //! unnecessary intermediate symbols.
    using Values = std::variant<Util::small_vector<Number>, Util::small_vector<Symbol>>;

    //! Initialize for the given aggregate function.
    AtomAssignAggr(AggregateFunction fun) : values_{init_(fun)} {}

    //! Accumulate a tuple.
    void accumulate(AggregateFunction fun, SymbolSpan tup, bool fact);

    //! Check if the aggregate in its current state is a fact.
    [[nodiscard]] auto is_fact() const;
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

    [[nodiscard]] auto num_values_() const -> size_t;

    std::vector<size_t> elems_;
    Values values_;
    size_t propagated_vals_ = 0;
    size_t propagated_ = 0;
    bool enqueued_ = false;
};

//! The base capturing derived assignment aggregate atoms.
class BaseAssignAggr : public BaseImpl<std::pair<size_t, Symbol>, BaseAssignAggr> {
  public:
    using BaseImpl::contains;
    using BaseImpl::Key;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomAssignAggr, Util::array_hash, Util::array_equal_to>;
    //! Map containing the derived atoms and their values.
    using AtomSet = Util::ordered_map<Key, size_t>;

    //! Construct an empty base.
    BaseAssignAggr(size_t size, bool domain_elems, bool single_pass_elems)
        : atoms_{0, size, size}, domain_elems_{domain_elems}, single_pass_elems_{single_pass_elems} {}

    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    [[nodiscard]] auto is_fact(Key sym) const -> bool;
    //! Add an atom to the base.
    //!
    //! This function should be called during propagation if an aggregate can match.
    auto add(size_t idx, Symbol val) -> bool;

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
    //! Get the derived atoms.
    [[nodiscard]] auto derived() -> AtomSet &;

    //! Check whether all relevant elements of the aggregate are domain.
    [[nodiscard]] auto domain_elems() const -> bool;
    //! Check whether all relevant elements of the aggregate can be grounded in a single pass.
    [[nodiscard]] auto single_pass_elems() const -> bool;

  private:
    AtomMap atoms_;
    AtomSet derived_;
    bool domain_elems_;
    bool single_pass_elems_;
};

class StmAssignAggrElem;

//! State storing all necessary information to ground assignment aggregates.
class StateAssignAggr : public State {
  public:
    class AtomKey;
    //! Keys for aggregate elements storing their tuple and their aggregate atom index.
    //!
    //! The atom index is used to store all elements in one big hash table.
    class ElementKey {
      private:
        struct priv_tag {};

      public:
        //! Construct the element.
        ElementKey(priv_tag tag, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                   StmAssignAggrElem &elem, bool &res);

        //! Prevent copying and moving.
        ElementKey(ElementKey const &other) = delete;
        //! Construct an element key evaluating the given tuple.
        [[nodiscard]] static auto construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                            StmAssignAggrElem &elem) -> bool;

        //! Get the tuple.
        [[nodiscard]] auto span() const -> SymbolSpan;
        //! Compute a hash for the key.
        [[nodiscard]] auto hash() const -> size_t;
        //! Compare to element keys.
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

      private:
        // Note that these two could be combined to save a little bit of memory.
        size_t n_;
        size_t atom_idx_;
        // NOLINTBEGIN
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms_[0];
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };

    //! A map from global variables to the aggregate representation.
    using AtomMap = BaseAssignAggr::AtomMap;
    //! A map from tuples to their conditions.
    //!
    //! Each value in the map represents an aggregate element.
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateAssignAggr(std::pmr::monotonic_buffer_resource &mbr, VariableVec global, UTerm term, AggregateFunction fun,
                    size_t index, bool domain_elems, bool single_pass_elems)
        : base_{global.size(), domain_elems, single_pass_elems}, global_{std::move(global)}, term_{std::move(term)},
          mbr_{&mbr}, index_{index}, fun_{fun} {
        assert(fun_ != AggregateFunction::count);
    }

    //! Get the global variables in the aggregate.
    //!
    //! This does not include the variables of the guard.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get a buffer to store values for global variables.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Get the target term to assign values to.
    [[nodiscard]] auto term() const -> Term const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that the elements are domain.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto domain_elems() const -> bool;
    //! Indicates that all necessary elements can be grounded in a single
    //! pass.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate enqueued aggregates.
    auto propagate(SymbolStore &store) -> bool;

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(EvalContext const &ctx) -> std::pair<AtomMap::iterator, bool>;

    //! Insert an aggregate element.
    void insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmAssignAggrElem &elem);

    //! Get the index of an aggregate atom.
    auto atom_index(AtomMap::iterator it) -> size_t;

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out, bool print_index);

    //! Get the underlying atom base.
    [[nodiscard]] auto base() -> BaseAssignAggr &;

    //! Output all previously grounded aggregates.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;

  private:
    //! Enqueue the given atom for propagation.
    void enqueue_(AtomMap::iterator it);

    BaseAssignAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    SymbolVec symbols_;
    UTerm term_;
    std::vector<size_t> queue_;
    std::pmr::monotonic_buffer_resource *mbr_;
    AtomKey *atom_key_ = nullptr;
    size_t index_;
    AggregateFunction fun_;
};

//! A term like object to match assignment aggregates.
class MatchAssignAggr {
  public:
    //! The key to match against.
    using Key = BaseAssignAggr::Key;

    //! Construct the matcher.
    MatchAssignAggr(StateAssignAggr &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const
        -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Key key) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Key>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchAssignAggr const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateAssignAggr &;

  private:
    std::vector<Symbol> mutable eval_;
    StateAssignAggr *state_;
};

//! Literal representing an assignment aggregate.
class LitAssignAggr : public Lit, private MatchAssignAggr {
  public:
    //! Construct an assignment aggregate literal.
    LitAssignAggr(StateAssignAggr &state) : MatchAssignAggr{state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    [[nodiscard]] auto do_domain() const -> bool override;

    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override;

    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;

    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound,
                                [[maybe_unused]] double recursive_estimate) const -> double override;

    void do_print(std::ostream &out) const override;

    auto do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t offset_ = invalid_offset;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate. Count aggregates have to be translated to sum+
//! aggregates beforehand.
class StmAssignAggrElem : public Stm {
  public:
    //! Construct the statement.
    //!
    //! The first num_cond literals of the body must form the aggregate
    //! element's condition. The following literals are just used for grounding
    //! binding global variables of the aggregate and ensuring safety.
    StmAssignAggrElem(StateAssignAggr &state, Location loc_weight, UTermVec tuple, ULitVec body, size_t num_cond,
                      size_t priority, ProfileNodeInternal *node)
        : state_{&state}, loc_weight_{std::move(loc_weight)}, tuple_{std::move(tuple)}, body_{std::move(body)},
          node_{node}, num_cond_{num_cond}, priority_{priority} {}

    //! Copy constructor.
    StmAssignAggrElem(StmAssignAggrElem const &other)
        : state_{other.state_}, loc_weight_{other.loc_weight_}, tuple_{copy_uvec(other.tuple_)},
          body_{copy_uvec(other.body_)}, node_{other.node_}, num_cond_{other.num_cond_}, priority_{other.priority_} {};
    //! Move constructor.
    StmAssignAggrElem(StmAssignAggrElem &&other) noexcept = default;
    //! Copy assignment.
    auto operator=(StmAssignAggrElem const &other) -> StmAssignAggrElem & = default;
    //! Move assignment.
    auto operator=(StmAssignAggrElem &&other) noexcept -> StmAssignAggrElem & = default;

  private:
    friend class StateAssignAggr;
    friend class StateAssignAggr::ElementKey;

    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    auto get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool>;

    StateAssignAggr *state_;
    StateAssignAggr::ElementKey *key_ = nullptr;
    Location loc_weight_;
    UTermVec tuple_;
    ULitVec body_;
    ProfileNodeInternal *node_;
    size_t num_cond_;
    size_t priority_;
    bool logged_ = false;
};

static_assert(std::is_nothrow_move_constructible_v<StmAssignAggrElem>);
static_assert(std::is_nothrow_move_assignable_v<StmAssignAggrElem>);
static_assert(std::is_copy_assignable_v<StmAssignAggrElem>);

//! Literal representing a stratified assignment aggregate.
class LitAssignAggrStrat : public Lit, private MatchAssignAggr {
  public:
    //! Construct an assignment aggregate literal.
    LitAssignAggrStrat(StateAssignAggr &state, std::vector<StmAssignAggrElem> elems)
        : MatchAssignAggr{state}, elems_{std::move(elems)} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    [[nodiscard]] auto do_domain() const -> bool override;

    [[nodiscard]] auto do_single_pass() const -> bool override;

    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;

    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound,
                                [[maybe_unused]] double recursive_estimate) const -> double override;

    void do_print(std::ostream &out) const override;

    auto do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    std::vector<StmAssignAggrElem> elems_;
    size_t offset_ = invalid_offset;
};

//! @}

} // namespace CppClingo::Ground
