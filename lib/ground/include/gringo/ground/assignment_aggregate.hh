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

class BaseAssignAggr : public BaseImpl<std::pair<size_t, Symbol>, BaseAssignAggr> {
  public:
    using BaseImpl::contains;
    using BaseImpl::Key;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomAssignAggr, Util::SpanHash, Util::SpanEqualTo>;
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

    //! Check whether all relevant elemens of the aggregate are domain.
    [[nodiscard]] auto domain_elems() const -> bool;
    //! Check whether all relevant elemens of the aggregate can be grounded in a single pass.
    [[nodiscard]] auto single_pass_elems() const -> bool;

  private:
    AtomMap atoms_;
    AtomSet derived_;
    bool domain_elems_;
    bool single_pass_elems_;
};

class StateAssignAggr {
  public:
    class AtomKey;
    // NOLINTBEGIN
    class ElementKey {
      public:
        static auto construct(auto &mbr, SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx,
                              UTermVec const &tuple, ElementKey *&target) -> bool;

        auto span() const -> SymbolSpan;
        auto hash() const -> size_t;
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

      private:
        ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx, UTermVec const &tuple,
                   bool &res);

        // Note that these two could be combined to save a little bit of memory.
        size_t n_;
        size_t atom_idx_;
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        Symbol syms_[0];
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
    };
    // NOLINTEND

    using AtomMap = BaseAssignAggr::AtomMap;
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateAssignAggr(VariableVec global, UTerm term, AggregateFunction fun, size_t index, bool domain_elems,
                    bool single_pass_elems)
        : base_{global.size(), domain_elems, single_pass_elems}, global_{std::move(global)}, term_{std::move(term)},
          index_{index}, fun_{fun} {}

    //! Get the global variables in the aggregate.
    //!
    //! This does not include the variables of the guard.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get the target term to assign values to.
    [[nodiscard]] auto term() const -> Term const &;
    //! Get the aggregate function.
    [[nodiscard]] auto fun() const -> AggregateFunction;
    //! Indicates that the elements are domain.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto domain_elems() const -> bool;
    //! Indicates that all necessary elemements can be grounded in a single
    //! pass.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate equeued aggregates.
    auto propagate(SymbolStore &store) -> bool;

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(Assignment &ass) -> AtomMap::iterator;

    //! Insert an aggregate element.
    void insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                     ElementKey *&elem_key, auto const &get_cond);

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
    //! Enqueu the given atom for propagation.
    void enqueue_(AtomMap::iterator it);

    std::pmr::monotonic_buffer_resource mbr_;
    BaseAssignAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    UTerm term_;
    std::vector<size_t> queue_;
    AtomKey *atom_key_ = nullptr;
    size_t index_;
    AggregateFunction fun_;
};

//! A term like object used to match conditional literals and their elements.
class MatchAssignAggr {
  public:
    //! The key to match against.
    using Key = BaseAssignAggr::Key;

    //! Construct the matcher.
    MatchAssignAggr(StateAssignAggr &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound,
                                 [[maybe_unused]] VariableSet const &bind) const -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Key key, Assignment &ass) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(SymbolStore &store, Assignment &ass) const -> std::optional<Key>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchAssignAggr const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateAssignAggr &;

  private:
    std::vector<Symbol> mutable eval_;
    StateAssignAggr *state_;
};

//! Literal representing an aggregate.
class LitAssignAggr : public Lit, private MatchAssignAggr {
  public:
    LitAssignAggr(StateAssignAggr &state) : MatchAssignAggr{state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    [[nodiscard]] auto do_domain() const -> bool override;

    //! Returns true if the aggregate needs only one grounding pass.
    [[nodiscard]] auto do_single_pass() const -> bool override;

    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;

    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;

    auto do_output([[maybe_unused]] InstantiationContext &ctx, OutputLit &out) const -> bool override;

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
    StmAssignAggrElem(StateAssignAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, size_t priority)
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

    StateAssignAggr *state_;
    StateAssignAggr::ElementKey *key_ = nullptr;
    UTermVec tuple_;
    ULitVec body_;
    size_t num_cond_;
    size_t priority_;
};

} // namespace Gringo::Ground
