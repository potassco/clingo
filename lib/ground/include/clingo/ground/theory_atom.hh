#pragma once

#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>
#include <clingo/ground/term.hh>
#include <clingo/ground/theory_term.hh>

#include <clingo/util/macro.hh>
#include <clingo/util/small_vector.hh>

#include <memory_resource>

namespace CppClingo::Ground {

// Outline:
// - rules:
//      H :- L, B.
//   accu :- A, E.
//   - literal L simply matches everything
//   - atom A matches the theory atoms output while grounding
//   - statements elem gather the ground elements of theory atoms
//     whose grounding is delayed until output
// - propagate
//   - no propagation is necessary because theory atoms always match
// - output
//   - as with other aggregates, output is delayed until the end of a component
//   - unlike other aggregates, elements are grounded during output
// - classes
//   - AtomTheory       (ground representation for theory atoms)
//   - BaseTheory       (a base for theory atoms)
//   - StateTheoryAtom  (gathers state for grounding/output)
//   - MatchTheory      (match object for theory atoms)
//   - LitMatchTheory   (binder to ground theory elements)
//   - StmTheoryElement (statement to accumulate elements)
//   - LitTheoryAtom    (atom to capture theory atoms in bodies)
//   - StmTheoryAtom    (statement to capture theory atoms in heads)

//! @addtogroup ground_theory
//! @{

//! The right-hand-side of a theory atom.
using TheoryRGuard = std::optional<std::pair<String, UTheoryTerm>>;

//! Extensible ground representation for theory atoms.
class AtomTheory {
  public:
    //! Default construct the atom with an empty set of elements.
    AtomTheory(Symbol name, std::optional<size_t> rhs) : name_{name}, rhs_{rhs ? *rhs : invalid_offset} {}

    //! Add a new element.
    void add_elem(size_t idx);
    //! Get the theory atom's elements.
    [[nodiscard]] auto elems() const -> std::span<size_t const>;

    //! Get the unique id of the theory atom.
    [[nodiscard]] auto uid() const -> std::optional<size_t>;
    //! Set the unique id of the theory atom.
    void uid(size_t uid);

    //! Get the name of the atom.
    [[nodiscard]] auto name() const -> Symbol { return name_; }

    //! Get the right hand side theory term index.
    [[nodiscard]] auto rhs() const -> std::optional<size_t> {
        if (rhs_ != invalid_offset) {
            return rhs_;
        }
        return std::nullopt;
    }

  private:
    Util::small_vector<size_t> elems_;
    Symbol name_;
    size_t uid_ = invalid_offset;
    size_t rhs_;
};

//! The base capturing theory atoms encountered during grounding.
class BaseTheory : public BaseImpl<Symbol const *, BaseTheory> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using AtomMap = Util::ordered_map<Symbol const *, AtomTheory, Util::array_hash, Util::array_equal_to>;

    //! Construct an empty base.
    BaseTheory(size_t size) : atoms_{0, size, size} {}

    //! Add an atom.
    auto add(Symbol const *sym, Symbol name, std::optional<size_t> rhs) -> std::pair<AtomMap::iterator, bool>;

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

//! State storing all necessary information to ground theory atoms.
class StateTheory : public Ground::State {
  public:
    //! Keys for aggregate elements storing their tuple and their aggregate index.
    //!
    //! The atom index is used to store all elements in one big hash table.
    class ElementKey {
      private:
        struct priv_tag {};

      public:
        //! Constructor.
        ElementKey(priv_tag tag, EvalContext const &ctx, OutputTheory &out, size_t atom_idx,
                   UTheoryTermVec const &terms);
        //! Prevent copying and moving.
        ElementKey(ElementKey const &other) = delete;
        //! Construct an element key evaluating the given tuple.
        static void construct(std::pmr::monotonic_buffer_resource &mbr, EvalContext const &ctx, OutputTheory &out,
                              size_t atom_idx, UTheoryTermVec const &terms, ElementKey *&target);

        //! Get the terms.
        [[nodiscard]] auto terms() const -> UTheoryTermVec const &;
        //! Get the size of the tuple.
        [[nodiscard]] auto size() const -> size_t;
        //! Get the tuple.
        [[nodiscard]] auto span() const -> std::span<size_t const>;
        //! Compute a hash for the key.
        [[nodiscard]] auto hash() const -> size_t;
        //! Compare to element keys.
        friend auto operator==(ElementKey const &a, ElementKey const &b) -> bool;

      private:
        size_t n_;
        size_t atom_idx_;
        // NOLINTBEGIN
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
        size_t syms_[0];
        CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
        // NOLINTEND
    };

    //! Map containing the atoms.
    using AtomMap = BaseTheory::AtomMap;
    //! Map capturing the elements of theory atoms.
    using ElementMap = Util::ordered_map<ElementKey *, Util::small_vector<size_t>>;

    //! Construct the state.
    StateTheory(std::pmr::monotonic_buffer_resource &mbr, VariableVec global, UTerm name, TheoryRGuard guard,
                OutputTheory::AtomType type)
        : base_{global.size()}, mbr_{&mbr}, global_{std::move(global)}, name_{std::move(name)},
          guard_{std::move(guard)}, type_{type} {}

    //! Find a previously grounded theory atom.
    //!
    //! Assumes that the assignment binds the global variables of the atom.
    auto find_atom(Assignment &ass) -> AtomMap::iterator;
    //! Insert a theory atom.
    //!
    //! Assumes that the assignment binds the global variables of the atom.
    auto insert_atom(Symbol name, std::optional<size_t> rhs, Assignment &ass) -> std::pair<AtomMap::iterator, bool>;
    //! Insert a theory atom element.
    //!
    //! Assumes that the assignment binds the global/local variables of the element.
    void insert_elem(EvalContext const &ctx, AtomMap::iterator it, UTheoryTermVec const &tuple, ElementKey *&elem_key,
                     auto const &get_cond);
    //! Print a debug representation of the theory atom.
    void print(std::ostream &out);
    //! Output all previously output theory atoms.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;
    //! Get the global variables of the theory atom.
    [[nodiscard]] auto global() const -> VariableVec const &;
    //! Get the name of the theory atom.
    [[nodiscard]] auto name() const -> UTerm const &;
    //! Get the guard of the theory atom.
    [[nodiscard]] auto guard() const -> TheoryRGuard const &;
    //! Get the associated base.
    [[nodiscard]] auto base() -> BaseTheory &;
    //! Set the theory elements.
    void elems(UStmVec elems);

  private:
    BaseTheory base_;
    ElementMap tuples_;
    std::pmr::monotonic_buffer_resource *mbr_;
    VariableVec global_;
    UTerm name_;
    TheoryRGuard guard_;
    UStmVec elems_;
    Symbol *atom_key_ = nullptr;
    OutputTheory::AtomType type_;
};

//! A term like object used to match theory atoms.
class MatchTheory {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchTheory(StateTheory &state) : state_{&state} { eval_.reserve(state_->global().size()); }

    //! Get the variables of the matcher.
    [[nodiscard]] auto vars() const -> VariableSet;

    //! Get the signature of the matcher.
    [[nodiscard]] auto signature(VariableSet const &bound, VariableSet const &bind) const -> VariableVec;

    //! Match a span of symbols representing an atom or element with the assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Symbol const *sym) const -> bool;

    //! Evaluate w.r.t. the given assignment and return a span representing an atom or element.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Symbol const *>;

    //! Print a string representation of the matcher.
    friend auto operator<<(std::ostream &out, MatchTheory const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateTheory &;

  private:
    std::vector<Symbol> mutable eval_;
    StateTheory *state_;
};

//! Literal to match a theory atom.
class LitMatchTheory : public Lit, private MatchTheory {
  public:
    //! Construct the matcher.
    LitMatchTheory(StateTheory &state) : MatchTheory{state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound, [[maybe_unused]] double recursive_estimate) const
        -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t offset_ = invalid_offset;
};

//! Gather theory atom elements.
class StmTheoryElement : public Stm {
  public:
    //! Construct the statement.
    StmTheoryElement(StateTheory &state, UTheoryTermVec tuple, ULitVec body, ProfileNodeInternal *node)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, node_{node} {}

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    StateTheory *state_;
    StateTheory::ElementKey *elem_key_ = nullptr;
    UTheoryTermVec tuple_;
    ULitVec body_;
    ProfileNodeInternal *node_;
};

//! Literal to match a theory atom.
class LitBdTheory : public Lit {
  public:
    //! Construct the matcher.
    LitBdTheory(StateTheory &state, Sign sign) : state_{&state}, sign_{sign} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound, [[maybe_unused]] double recursive_estimate) const
        -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    StateTheory *state_;
    Symbol name_;
    Sign sign_;
};

//! Ground a rule with a theory atom in the head.
class StmHdTheory : public Stm {
  public:
    //! Construct the statement.
    StmHdTheory(StateTheory &state, ULitVec body, ProfileNodeInternal *node)
        : state_{&state}, body_{std::move(body)}, node_{node} {}

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;
    void do_init([[maybe_unused]] size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    StateTheory *state_;
    ULitVec body_;
    ProfileNodeInternal *node_;
};

//! @}

} // namespace CppClingo::Ground
