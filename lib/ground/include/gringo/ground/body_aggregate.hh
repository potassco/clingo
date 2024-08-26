#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/small_vector.hh>

#include <ostream>

namespace Gringo::Ground {

using GuardVec = std::vector<std::pair<Relation, UTerm>>;

enum class AtomBdAggrState : uint8_t {
    unknown = 0,
    derived = 1,
    fact = 2,
};

class AtomBdAggr {
  public:
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

    [[nodiscard]] auto uid() const -> std::optional<size_t>;

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

class BaseBdAggr : public BaseImpl<Symbol const *, BaseBdAggr> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomBdAggr, Util::SpanHash, Util::SpanEqualTo>;

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
    [[nodiscard]] auto atom_index_(AtomMap::const_iterator it) const -> size_t;

    AtomMap atoms_;
    Util::index_sequence<size_t> derived_;
};

class StateBdAggr {
  public:
    // NOLINTBEGIN
    struct ElementKey {
        ElementKey(SymbolStore &store, Assignment &ass, AggregateFunction fun, size_t atom_idx, UTermVec const &tuple,
                   bool &res);

        auto span() const -> SymbolSpan;
        auto hash() const -> size_t;
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

        // Note that these two could be combined to safe a little bit of memory.
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

    using AtomMap = BaseBdAggr::AtomMap;
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Initialize an aggregate state.
    StateBdAggr(VariableVec global, GuardVec guards, AggregateFunction fun, size_t index, bool domain, bool monotone,
                bool single_pass_elems)
        : base_{global.size()}, global_{std::move(global)}, guards_{std::move(guards)}, index_{index}, fun_{fun},
          domain_{domain}, monotone_{monotone}, single_pass_elems_{single_pass_elems} {}

    //! Get the global variables in the aggregate.
    [[nodiscard]] auto global() const -> VariableVec const &;
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
    //! Indicates that all necessary elemements can be grounded in a single
    //! pass.
    //!
    //! This does not take into account the body prefix of elements.
    [[nodiscard]] auto single_pass_elems() const -> bool;
    //! Get the update index.
    [[nodiscard]] auto index() const -> size_t;

    //! Propagate equeued aggregates.
    auto propagate() -> bool;

    //! Enequeue an atom for propgation.
    void enqueue(AtomMap::iterator it);

    //! Insert an aggregate atom (stemming from an aggregate element).
    //!
    //! This function also enqueues freshly inserted atoms to cover the case
    //! that the aggregate matches the empty element set.
    auto insert_atom(SymbolStore &store, Assignment &ass) -> std::optional<AtomMap::iterator>;

    //! Insert a previously evaluated atom.
    //!
    //! This functions can be used to ensure the presence of an atom that has not yet been derived.
    auto insert_atom(Symbol const *tuple) -> AtomMap::iterator;

    //! Insert an aggregate element.
    void insert_elem(SymbolStore &store, Assignment &ass, AtomMap::iterator it, UTermVec const &tuple,
                     auto const &get_cond);

    //! Get the index of an aggregate atom.
    //!
    //! This index also captures not yet derived atoms.
    auto index(AtomMap::iterator it) -> size_t;

    //! Get the index of an aggregate element.
    auto index(ElementMap::iterator it) -> size_t;

    //! Print a non-ground representation of the aggregate.
    void print(std::ostream &out);

    //! Get the underlying atom base.
    [[nodiscard]] auto base() -> BaseBdAggr &;

    //! Output all previously output aggregates.
    void output(OutputStm &out);

  private:
    Util::NodeStore<alignof(Symbol)> node_store_;
    BaseBdAggr base_;
    ElementMap tuples_;
    VariableVec global_;
    GuardVec guards_;
    std::vector<size_t> queue_;
    size_t index_;
    AggregateFunction fun_;
    bool domain_;
    bool monotone_;
    bool single_pass_elems_;
};

//! A term like object used to match conditional literals and their elements.
class MatchBdAggr {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchBdAggr(StateBdAggr &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound,
                                 [[maybe_unused]] VariableSet const &bind) const -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(SymbolStore &store, Assignment &ass) const -> std::optional<Symbol const *>;

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
    StmBdAggrElem(StateBdAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, size_t priority)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, num_cond_{num_cond}, priority_{priority} {}

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override;
    void do_propagate(Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;

    StateBdAggr *state_;
    UTermVec tuple_;
    ULitVec body_;
    size_t num_cond_;
    size_t priority_;
};

} // namespace Gringo::Ground
