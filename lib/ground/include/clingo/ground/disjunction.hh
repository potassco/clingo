#pragma once

#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/small_vector.hh>

#include <memory_resource>
#include <ostream>

namespace CppClingo::Ground {

//! @addtogroup ground_hdcondlit
//! @{

// Outline:
// - grounding of disjunctions is very similar to the grounding of head aggregates
// - the differences are small
//   - grounding can be stopped if one element becomes true
//   - data structures do not have to store term tuples

//! Extensible ground representation of disjunctions.
//!
//! Elements can be added to this representation and it can be enqueued for
//! later propagation. Propagation derives not yet propagated head atoms as
//! long as the disjunction is not marked as fact.
class AtomDisjunction {
  public:
    //! Check if the disjunction is a fact.
    [[nodiscard]] auto is_fact() const -> bool;
    //! Mark the disjunction as fact.
    void mark_fact();
    //! Enqueue the atom for propagation.
    auto enqueue() -> bool;
    //! Dequeue the atom after propagation.
    //!
    //! Also marks elements as propagated.
    void dequeue();
    //! Add a new element.
    void add_elem(size_t idx);
    //! Get the disjunction elements.
    [[nodiscard]] auto elems() const -> std::span<size_t const>;
    //! Get the disjunction elements to propagate.
    [[nodiscard]] auto todo() -> std::span<size_t const>;

    //! Get the unique id of the disjunction atom.
    [[nodiscard]] auto uid() const -> std::optional<size_t>;
    //! Set the unique id of the disjunction atom.
    void uid(size_t uid);

  private:
    Util::small_vector<size_t> elems_;
    size_t uid_ = invalid_offset;
    uint64_t propagated_ : 62 = 0;
    // TODO: maybe bit set
    uint64_t enqueued_ : 1 = 0;
    uint64_t fact_ : 1 = 0;
};

//! The base capturing derived disjunction atoms.
class BaseDisjunction : public BaseImpl<Symbol const *, BaseDisjunction> {
  public:
    using BaseImpl::contains;
    //! Mapping from global variables to disjunction atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomDisjunction, Util::array_hash, Util::array_equal_to>;

    //! Construct an empty base.
    BaseDisjunction(size_t size) : atoms_{0, size, size} {}

    //! Add an atom to the current generation.
    auto add(Symbol const *sym) -> std::pair<AtomMap::iterator, bool>;

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

//! A vector of signatures, bases, and indices.
//!
//! Whenever a base has an update, its indices have to be propagated. The base
//! is identified by the signature and the vector sorted by this signature.
using DisjunctionBaseVec = std::vector<std::tuple<std::tuple<String, size_t, bool>, AtomBase *, std::vector<size_t>>>;

//! State storing all necessary information to ground disjunctions.
class StateDisjunction : public State {
  public:
    //! A map from global variables (including the guards) to the disjunction representation.
    using AtomMap = BaseDisjunction::AtomMap;
    //! A key consisting of a head atom and a disjunction atom index.
    using ElementKey = std::pair<Symbol, size_t>;
    //! A map from disjunction atoms and their heads to conditions.
    using ElementMap = Util::ordered_map<ElementKey, std::pair<size_t, Util::small_vector<size_t>>>;

    //! Initialize a disjunction state.
    StateDisjunction(std::pmr::monotonic_buffer_resource &mbr, DisjunctionBaseVec bases, VariableVec global,
                     size_t index, bool single_pass_body)
        : base_{global.size()}, global_{std::move(global)}, bases_{std::move(bases)}, mbr_{&mbr}, index_{index},
          single_pass_body_{single_pass_body} {}

    //! Get the global variables in the disjunction.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get a buffer to store values for global variables.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Indicates that all necessary elements can be grounded in a single
    //! pass.
    [[nodiscard]] auto single_pass_body() const -> bool;
    //! Get the update index for the disjunction.
    [[nodiscard]] auto index() const -> size_t;
    //! Get the update indices for the heads of the elements.
    //!
    //! Unoptimized and intended for debug printing.
    [[nodiscard]] auto indices() const -> std::vector<size_t>;

    //! Enqueue disjunction element rules.
    void enqueue(Queue &queue);

    //! Propagate enqueued disjunction atoms.
    void propagate(OutputStm &out, Queue &queue);

    //! Insert a disjunction atom.
    auto insert_atom(Assignment &ass) -> std::pair<AtomMap::iterator, bool>;

    //! Insert an disjunction element.
    void insert_elem(EvalContext const &ctx, AtomMap::iterator it, UTerm const &head, auto const &get_cond);

    //! Print a non-ground representation of the disjunction.
    void print(std::ostream &out, bool print_index);

    //! Output all previously output disjunction atoms.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;

    //! Return the base of the disjunction.
    [[nodiscard]] auto base() -> BaseDisjunction &;

  private:
    //! Enqueue an atom for propagation.
    void enqueue_(AtomMap::iterator it);

    //! Get the index of a disjunction atom.
    //!
    //! This index also captures not yet derived atoms.
    auto atom_index_(AtomMap::iterator it) -> size_t;

    BaseDisjunction base_;
    ElementMap elems_;
    VariableVec global_;
    SymbolVec symbols_;
    DisjunctionBaseVec bases_;
    std::vector<size_t> queue_;
    std::pmr::monotonic_buffer_resource *mbr_;
    Symbol *atom_key_ = nullptr;
    size_t index_;
    bool single_pass_body_;
};

//! A statement deriving disjunction atoms to trigger grounding of elements.
class StmDisjunction : public Stm {
  public:
    //! Construct the statement.
    StmDisjunction(StateDisjunction &state, Ground::ULitVec body, size_t priority)
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

    StateDisjunction *state_;
    ULitVec body_;
    size_t priority_;
};

//! Gather disjunction elements.
class StmDisjunctionElem : public Stm {
  public:
    //! Construct the statement.
    StmDisjunctionElem(StateDisjunction &state, UTerm head, AtomBase &base, ULitVec body)
        : state_{&state}, head_{std::move(head)}, base_{&base}, body_{std::move(body)} {}

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;

    StateDisjunction *state_;
    UTerm head_;
    AtomBase *base_;
    ULitVec body_;
};

//! A term like object used to match disjunction atoms.
class MatchDisjunction {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchDisjunction(StateDisjunction &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound, VariableSet const &bind) const -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Symbol const *sym) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Symbol const *>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchDisjunction const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateDisjunction &;

  private:
    std::vector<Symbol> mutable eval_;
    StateDisjunction *state_;
};

//! Literal representing a disjunction.
class LitDisjunction : public Lit, private MatchDisjunction {
  public:
    //! Construct the disjunction literal.
    LitDisjunction(StateDisjunction &state) : MatchDisjunction{state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;

    [[nodiscard]] auto do_domain() const -> bool override;

    //! Returns true if the disjunction needs only one grounding pass.
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
