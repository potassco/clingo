#pragma once

#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/small_vector.hh>

#include <memory_resource>
#include <ostream>

namespace CppClingo::Ground {

//! @addtogroup ground_bdaggr
//! @{

//! Derivation state of body aggregates.
enum class AtomBdAggrState : uint8_t {
    unknown = 0, //!< The aggregate has not yet been derived.
    derived = 1, //!< The aggregate has been derived.
    fact = 2,    //!< The aggregate has been derived as a fact.
};

//! Extensible ground representation of body aggregates.
class AtomBdAggr {
  public:
    //! The lower and upper bound for the value an aggregate can take.
    //!
    //! Aggregates operating on numbers do not use symbols to avoid storing
    //! unnecessary intermediate symbols.
    using Bound = std::variant<std::pair<Number, Number>, std::pair<Symbol, Symbol>>;

    //! Initialize for the given aggregate function.
    AtomBdAggr(AggregateFunction fun) : bound_{init_(fun)} {}

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
    [[nodiscard]] auto state() const -> AtomBdAggrState;
    //! Set the derived state of the aggregate atom.
    //!
    //! It must only be derived once.
    void state(AtomBdAggrState state);

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
    size_t derived_idx_ = 0;
    size_t uid_ = invalid_offset;
    AtomBdAggrState state_ = AtomBdAggrState::unknown;
    bool enqueued_ = false;
};

//! The base capturing derived body aggregate atoms.
class BaseBdAggr : public BaseImpl<Symbol const *, BaseBdAggr> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomBdAggr, Util::array_hash, Util::array_equal_to>;

    //! Construct an empty base.
    BaseBdAggr(size_t size) : atoms_{0, size, size} {}

    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol const *sym) const -> bool;
    //! Add an atom to the base.
    //!
    //! This function should be called during propagation if an aggregate can match.
    void add(AtomMap::iterator it);

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t;

    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    auto index(Symbol const *sym) const -> size_t;
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> AtomMap::const_iterator;
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> AtomMap::iterator;

    //! Get the underlying atom map (includes atoms not yet derived).
    [[nodiscard]] auto atoms() -> AtomMap &;

  private:
    AtomMap atoms_;
    Util::index_sequence<size_t> derived_;
};

// see node in head_aggregate.h
class StmBdAggrElem;

//! State storing all necessary information to ground body aggregates.
class StateBdAggr : public State {
  public:
    class AtomKey;
    //! Keys for aggregate elements storing their tuple and their aggregate index.
    //!
    //! The atom index is used to store all elements in one big hash table.
    class ElementKey {
      private:
        struct priv_tag {};

      public:
        //! Construct the element.
        ElementKey(priv_tag tag, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx, StmBdAggrElem &elem,
                   bool &res);
        //! Prevent copying and moving.
        ElementKey(ElementKey const &other) = delete;
        //! Construct an element key evaluating the given tuple.
        [[nodiscard]] static auto construct(auto &mbr, EvalContext const &ctx, AggregateFunction fun, size_t atom_idx,
                                            StmBdAggrElem &elem) -> bool;

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
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms_[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };

    //! A map from global variables (including the guards) to the aggregate representation.
    using AtomMap = BaseBdAggr::AtomMap;
    //! A map from tuples to their conditions.
    //!
    //! Each value in the map represents an aggregate element. The vector of
    //! condition ids is kept sorted and unique. A small vector is used because
    //! it should commonly have sizes of zero (a fact) or one (a single
    //! conditions).
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateBdAggr(std::pmr::monotonic_buffer_resource &mbr, VariableVec global, GuardVec guards, AggregateFunction fun,
                size_t index, bool domain, bool monotone, bool single_pass_elems)
        : base_{global.size()}, global_{std::move(global)}, guards_{std::move(guards)}, mbr_{&mbr}, index_{index},
          fun_{fun}, domain_{domain}, monotone_{monotone}, single_pass_elems_{single_pass_elems} {}

    //! Get the global variables in the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get a buffer to store values for global variables.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Get the non-ground guards of the aggregate.
    [[nodiscard]] auto guards() const -> GuardVec const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that all aggregate elements are domain.
    //!
    //! That is, all the bases of literals in conditions are domain and all
    //! negative literals are stratified.
    //!
    //! Only considers the elements of the aggregate.
    [[nodiscard]] auto domain() const -> bool;
    //! Indicates that the aggregate is monotone.
    //!
    //! Neither takes the sign of the aggregate nor its elements into account.
    [[nodiscard]] auto monotone() const -> bool;
    //! Indicates that all necessary elements can be grounded in a single
    //! pass.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate enqueued aggregates.
    auto propagate() -> bool;

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(EvalContext const &ctx) -> std::optional<std::pair<AtomMap::iterator, bool>>;

    //! Insert a previously evaluated atom.
    //!
    //! This functions can be used to ensure the presence of an atom that has not yet been derived.
    auto insert_atom(Symbol const *tuple) -> AtomMap::iterator;

    //! Insert an aggregate element.
    void insert_elem(EvalContext const &ctx, AtomMap::iterator it, StmBdAggrElem &elem);

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out, bool print_index);

    //! Get the underlying atom base.
    [[nodiscard]] auto base() -> BaseBdAggr &;

    //! Output all previously output aggregates.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;

  private:
    //! Enqueue an atom for propagation.
    void enqueue_(AtomMap::iterator it);

    //! Get the index of an aggregate atom.
    //!
    //! This index also captures not yet derived atoms.
    auto atom_index_(AtomMap::iterator it) -> size_t;

    BaseBdAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    SymbolVec symbols_;
    GuardVec guards_;
    std::vector<size_t> queue_;
    std::pmr::monotonic_buffer_resource *mbr_;
    AtomKey *atom_key_ = nullptr;
    size_t index_;
    AggregateFunction fun_;
    bool domain_;
    bool monotone_;
    bool single_pass_elems_;
};

//! A term like object used to match body aggregates.
class MatchBdAggr {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchBdAggr(StateBdAggr &state) : state_{&state} {
        eval_.reserve(state_->global().size() + state_->guards().size());
    }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const
        -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Symbol const *sym) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Symbol const *>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchBdAggr const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateBdAggr &;

  private:
    std::vector<Symbol> mutable eval_;
    StateBdAggr *state_;
};

//! Literal representing an aggregate.
class LitBdAggr : public Lit, private MatchBdAggr {
  public:
    //! Construct the aggregate literal.
    LitBdAggr(StateBdAggr &state, Sign sign) : MatchBdAggr{state}, sign_{sign} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    //! Returns true if matching aggregates are always facts.
    //!
    //! The function can only return true if all literals in elements are
    //! domain. Furthermore, the aggregate must be either monotone or there is
    //! no recursion through it.
    //!
    //! Note that we do not need a stratified index for the latter case. There
    //! can be recursion through the body prefix.
    [[nodiscard]] auto do_domain() const -> bool override;

    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override;

    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;

    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;

    auto do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t offset_ = invalid_offset;
    Symbol const *symbol_ = nullptr;
    Sign sign_;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate. Count aggregates have to be translated to sum+
//! aggregates beforehand.
class StmBdAggrElem : public Stm {
  public:
    //! Construct the statement.
    //!
    //! The first num_cond literals of the body must form the aggregate
    //! element's condition. The following literals are just used for grounding
    //! binding global variables of the aggregate and ensuring safety.
    StmBdAggrElem(StateBdAggr &state, Location loc_weight, UTermVec tuple, ULitVec body, size_t num_cond,
                  size_t priority)
        : state_{&state}, loc_weight_{std::move(loc_weight)}, tuple_{std::move(tuple)}, body_{std::move(body)},
          num_cond_{num_cond}, priority_{priority} {}

    //! Copy constructor.
    StmBdAggrElem(StmBdAggrElem const &other)
        : state_{other.state_}, loc_weight_{other.loc_weight_}, tuple_{copy_uvec(other.tuple_)},
          body_{copy_uvec(other.body_)}, num_cond_{other.num_cond_}, priority_{other.priority_} {};
    //! Move constructor.
    StmBdAggrElem(StmBdAggrElem &&other) noexcept = default;
    //! Copy assignment.
    auto operator=(StmBdAggrElem const &other) -> StmBdAggrElem & = default;
    //! Move assignment.
    auto operator=(StmBdAggrElem &&other) noexcept -> StmBdAggrElem & = default;

  private:
    friend class StateBdAggr;
    friend class StateBdAggr::ElementKey;

    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;

    auto get_cond_(EvalContext const &ctx) -> std::pair<size_t, bool>;

    StateBdAggr *state_;
    StateBdAggr::ElementKey *elem_key_ = nullptr;
    Location loc_weight_;
    UTermVec tuple_;
    ULitVec body_;
    size_t num_cond_;
    size_t priority_;
    bool logged_ = false;
};

static_assert(std::is_nothrow_move_constructible_v<StmBdAggrElem>);
static_assert(std::is_nothrow_move_assignable_v<StmBdAggrElem>);
static_assert(std::is_copy_assignable_v<StmBdAggrElem>);

//! Literal representing a stratified body aggregate.
class LitBdAggrStrat : public Lit {
  public:
    //! Construct the aggregate literal.
    LitBdAggrStrat(StateBdAggr &state, std::vector<StmBdAggrElem> elems, Sign sign)
        : state_{&state}, elems_{std::move(elems)}, sign_{sign} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    //! Returns true if matching aggregates are always facts.
    //!
    //! The function can only return true if all literals in elements are
    //! domain.
    //!
    //! Note that there is no need for a stratified index. There can be
    //! recursion through the body prefix.
    [[nodiscard]] auto do_domain() const -> bool override;
    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output([[maybe_unused]] EvalContext const &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    StateBdAggr *state_;
    std::vector<StmBdAggrElem> elems_;
    size_t offset_ = invalid_offset;
    Sign sign_;
};

//! @}

} // namespace CppClingo::Ground
