#pragma once

#include <clingo/ground/literal.hh>
#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

namespace CppClingo::Ground {

class StmSortElem;

//! Base storing the derived adjacency relation for `#sort` literals.
//!
//! Values are partitioned by assignments to the global grouping variables.
//! For each group, values are sorted and duplicates removed before
//! propagation. The atom base then stores keys `(group, prev, next)` for
//! adjacent pairs in that sorted sequence.
class BaseSort : public BaseImpl<std::tuple<size_t, Symbol, Symbol>, BaseSort> {
  public:
    using BaseImpl::contains;
    using Key = BaseImpl::Key;
    using GroupMap = Util::ordered_map<Symbol const *, SymbolVec, Util::array_hash, Util::array_equal_to>;
    using AtomSet = Util::ordered_map<Key, size_t>;

    //! Construct the base with the number of grouping variables.
    BaseSort(size_t size) : groups_{0, size, size} {}

    //! Get the number of propagated adjacency atoms.
    [[nodiscard]] auto size() const -> size_t;
    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t;
    //! Get the n-th adjacency atom.
    [[nodiscard]] auto nth(size_t index) const -> AtomSet::const_iterator;
    //! Get the n-th adjacency atom.
    auto nth(size_t index) -> AtomSet::iterator;
    //! Access groups keyed by global variable assignments.
    [[nodiscard]] auto groups() -> GroupMap &;
    //! Access stored adjacency atoms.
    [[nodiscard]] auto atoms() -> AtomSet &;

  private:
    GroupMap groups_;
    AtomSet atoms_;
};

//! Grounding state shared by `#sort` statements and literals.
//!
//! The state collects values per global-variable group and, during
//! propagation, derives the adjacency relation between consecutive sorted
//! values in each group.
class StateSort : public State {
  public:
    class GroupKey;
    using GroupMap = BaseSort::GroupMap;

    //! Construct the state with global grouping variables and adjacency terms.
    StateSort(std::pmr::monotonic_buffer_resource &mbr, VariableVec global, UTerm prev, UTerm next)
        : base_{global.size()}, global_{std::move(global)}, prev_{std::move(prev)}, next_{std::move(next)}, mbr_{&mbr} {
    }

    //! Get the global variables used to form grouping keys.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get temporary storage used while instantiating newly seen groups.
    [[nodiscard]] auto symbols() -> SymbolVec &;
    //! Get the term matching the predecessor in an adjacency pair.
    [[nodiscard]] auto prev() const -> Term const &;
    //! Get the term matching the successor in an adjacency pair.
    [[nodiscard]] auto next() const -> Term const &;
    //! Access the propagated base.
    [[nodiscard]] auto base() -> BaseSort &;
    //! Insert or find the group for the current global assignment.
    auto insert_group(EvalContext const &ctx) -> std::pair<GroupMap::iterator, bool>;
    //! Evaluate and append one value to a group's raw value list.
    void insert_value(EvalContext const &ctx, GroupMap::iterator group, Term const &term);
    //! Finalize a group by deriving adjacency atoms from sorted unique values.
    void propagate(GroupMap::iterator group);
    void output(Logger &, SymbolStore &, OutputStm &) override {}

  private:
    BaseSort base_;
    VariableVec global_;
    SymbolVec symbols_;
    UTerm prev_;
    UTerm next_;
    std::pmr::monotonic_buffer_resource *mbr_;
    GroupKey *group_key_ = nullptr;
};

//! Matcher helper for `#sort` literals.
//!
//! It maps between assignment contexts and sort keys `(group, prev, next)` and
//! ensures that group symbols are unified with the current assignment.
class MatchSort {
  public:
    using Key = BaseSort::Key;
    //! Construct the matcher view for the given sort state.
    MatchSort(StateSort &state) : state_{&state} {}
    //! Get all variables referenced by matching.
    [[nodiscard]] auto vars() const -> VariableSet;
    //! Return the matcher signature based on currently bound variables.
    [[nodiscard]] auto signature(VariableSet const &bound, VariableSet const &bind) const -> VariableVec;
    //! Check whether the key matches and/or extends the current assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Key const &key) const -> bool;
    //! Evaluate the current assignment into a key, if fully defined.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Key>;
    //! Access the underlying sort state.
    [[nodiscard]] auto state() const -> StateSort & { return *state_; }
    friend auto operator<<(std::ostream &out, MatchSort const &match) -> std::ostream &;

  private:
    StateSort *state_;
};

//! Statement collecting one value candidate for a `#sort` group.
//!
//! During reporting, the statement inserts the current group and adds the
//! evaluated value to that group's pool.
class StmSortElem : public Stm {
  public:
    //! Construct a sort-element statement.
    StmSortElem(StateSort &state, UTerm value, ULitVec body, size_t num_cond, size_t priority,
                ProfileNodeInternal *node)
        : state_{&state}, value_{std::move(value)}, body_{std::move(body)}, node_{node}, num_cond_{num_cond},
          priority_{priority} {}
    //! Copy while deep-copying owned terms and literals.
    StmSortElem(StmSortElem const &other)
        : state_{other.state_}, value_{other.value_->copy()}, body_{copy_uvec(other.body_)}, node_{other.node_},
          num_cond_{other.num_cond_}, priority_{other.priority_} {}
    StmSortElem(StmSortElem &&) noexcept = default;

  private:
    auto do_body() const -> ULitVec const & override;
    auto do_important() const -> VariableSet override;
    auto do_is_important(size_t index) const -> bool override;
    void do_init(size_t gen) override;
    auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &, OutputStm &, Queue &) override;
    auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    StateSort *state_;
    UTerm value_;
    ULitVec body_;
    ProfileNodeInternal *node_;
    size_t num_cond_;
    size_t priority_;
};

//! Literal strategy for `#sort`.
//!
//! This literal integrates sort-element instantiation with atom matching over
//! the propagated adjacency base.
class LitSortStrat : public Lit, private MatchSort {
  public:
    //! Construct the strategy from state and its sort-element statements.
    LitSortStrat(StateSort &state, std::vector<StmSortElem> elems) : MatchSort{state}, elems_{std::move(elems)} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type, std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &, OutputLit &) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    std::vector<StmSortElem> elems_;
    size_t offset_ = invalid_offset;
};

} // namespace CppClingo::Ground