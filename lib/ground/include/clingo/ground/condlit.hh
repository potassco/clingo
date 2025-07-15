#pragma once

#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/enumerate.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_bdcondlit
//! @{

//! The type of the literals involved in grounding conditional literals.
enum class LitCondLitType : uint8_t {
    empty = 0,   //!< Literals encountered during grounding.
    premise = 1, //!< Premises encountered during grounding.
    lit = 2,     //!< Conditional literals derived during grounding.
};
//! Print the type of a conditional literal grounding literal.
auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream &;

//! The 4-valued truth value of a conclusion.
// NOLINTNEXTLINE(performance-enum-size)
enum class TruthConclusion : uint64_t {
    true_ = 0,   //!< The conclusion is true.
    false_ = 1,  //!< The conclusion is false.
    derived = 2, //!< There is rule that can derived the conclusion.
    unknown = 3, //!< There is no rule yet that could derive the conclusion.
};

//! Capture (the state of) an element of a conditional literal.
struct StateCondLitElem {
  public:
    //! Initialize the element.
    StateCondLitElem(size_t premise, bool premise_is_fact, bool has_conclusion)
        : premise_{premise}, conclusion_truth_{has_conclusion ? TruthConclusion::unknown : TruthConclusion::false_},
          premise_is_fact_{static_cast<uint8_t>(premise_is_fact)} {}
    //! Mark a previously unknown conclusion either as derived or fact.
    void set_conclusion(size_t conclusion, bool fact) {
        conclusion_ = conclusion;
        assert(conclusion_truth_ == TruthConclusion::unknown);
        conclusion_truth_ = fact ? TruthConclusion::true_ : TruthConclusion::derived;
    }
    //! Check if the element is true, i.e., its conclusion is true.
    [[nodiscard]] auto is_fact() const -> bool { return conclusion_truth_ == TruthConclusion::true_; }
    //! Check if the element is blocked.
    //!
    //! An element is blocked if its premise is true and its conclusion is
    //! false or has not yet been derived.
    [[nodiscard]] auto is_blocked() const -> bool {
        return premise_is_fact_ != 0 &&
               (conclusion_truth_ == TruthConclusion::false_ || conclusion_truth_ == TruthConclusion::unknown);
    }
    //! Check if an element is false.
    //!
    //! An element is false if its premise is true and its conclusion false.
    [[nodiscard]] auto is_false() const {
        return premise_is_fact_ != 0 && conclusion_truth_ == TruthConclusion::false_;
    }
    //! Check if the index of the element has already been set.
    [[nodiscard]] auto has_offset() const -> bool { return offset_ > 0; }
    //! Set the index of the element.
    void set_offset(size_t offset) { offset_ = offset + 1; }
    //! The index of the element in the vector of derived elements.
    //!
    //! The index is set once an element is propagated and added to the set of derived elements
    [[nodiscard]] auto offset() const -> size_t { return offset_ - 1; }

    //! Get the premise of the element.
    [[nodiscard]] auto premise() const -> size_t { return premise_; }
    //! Get the conclusion index of the literal if there is one.
    [[nodiscard]] auto conclusion() const -> std::optional<size_t> {
        return conclusion_ != invalid_offset ? std::make_optional(conclusion_) : std::nullopt;
    }

  private:
    size_t conclusion_ = invalid_offset;
    size_t premise_;
    uint64_t offset_ : 61 = 1;
    TruthConclusion conclusion_truth_ : 2;
    uint64_t premise_is_fact_ : 1;
};

//! A map from an atom + local variables to an element of a conditional literal.
//!
//! We can use here that the number of local variables is fixed.
using MapElemCondLit = Util::ordered_map<Symbol const *, StateCondLitElem, Util::array_hash, Util::array_equal_to>;

//! Capture (the state of) a conditional literal.
//!
//! This is referred to as atom in the code.
class StateAtomCondLit {
  public:
    StateAtomCondLit() = default;
    //! Add an element with the given index to the atom.
    void add_elem(size_t index) { elems_.emplace_back(index); }
    //! Enqueue the atom for grounding.
    //!
    //! Atom that are already enqueued or have already been propagated are not enqueued.
    //! As a consequence, an atom that has been propagated cannot be marked as false later on.
    [[nodiscard]] auto enqueue(MapElemCondLit const &elems) -> bool;
    //! Propagate a previously enqueued atom.
    [[nodiscard]] auto propagate(MapElemCondLit const &elems) -> bool;
    //! Check if all elements of the atom are facts.
    //!
    //! Note that this is not sufficient to check whether the atom is fact in
    //! case the premise is multipass.
    [[nodiscard]] auto is_fact(MapElemCondLit const &elems) const -> bool;
    //! Check if the atom has been marked false.
    [[nodiscard]] auto is_false() const -> bool { return false_ != 0; }
    //! Check if the atom has been derived.
    [[nodiscard]] auto has_offset() const -> bool { return offset_ > 0; }
    //! Mark the atom as derived setting its derived index.
    void set_offset(size_t offset) { offset_ = offset + 1; }
    //! The index of the atom in the vector of derived atoms.
    //!
    //! The index is set once an atom is propagated and added to the set of derived atoms.
    [[nodiscard]] auto offset() const -> size_t { return offset_ - 1; }

    //! Get the unique id of the atom.
    //!
    //! This id is used by the output to uniquely identify conditional literals.
    [[nodiscard]] auto uid() const -> std::optional<size_t> {
        return uid_ != invalid_offset ? std::make_optional(uid_) : std::nullopt;
    }
    //! Set the unique id of the atom.
    void uid(size_t uid) {
        assert(uid_ == invalid_offset || uid_ == uid);
        uid_ = uid;
    }
    //! Get the elements of the conditional literal.
    [[nodiscard]] auto elems() const -> std::span<size_t const> { return elems_; }

  private:
    std::vector<size_t> elems_;
    uint64_t elems_propagated_ : 61 = 0;
    uint64_t propagated_ : 1 = 0;
    uint64_t enqueued_ : 1 = 0;
    uint64_t false_ : 1 = 0;
    size_t uid_ = invalid_offset;
    size_t offset_ = 0;
};
//! A map from the global variables to a conditional literal.
using MapAtomCondLit = Util::ordered_map<Symbol const *, StateAtomCondLit, Util::array_hash, Util::array_equal_to>;

//! A base for not yet propagated conditional literals.
class BaseCondLitEmpty : public BaseImpl<Symbol const *, BaseCondLitEmpty> {
  public:
    //! Construct the base.
    //!
    //! The given atoms form the base. They are managed externally and the base
    //! just provides a view on them.
    BaseCondLitEmpty(MapAtomCondLit &atoms) : atoms_{&atoms} {}

    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t {
        return std::distance(atoms_->begin(), atoms_->find(key));
    }
    //! Get the number of atoms in the base.
    [[nodiscard]] auto size() const -> size_t { return atoms_->size(); }

    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) const -> MapAtomCondLit::const_iterator { return atoms_->nth(i); }
    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) -> MapAtomCondLit::iterator { return atoms_->nth(i); }

  private:
    MapAtomCondLit *atoms_;
};

//! A base for premises of conditional literals.
class BaseCondLitPremise : public BaseImpl<Symbol const *, BaseCondLitPremise> {
  public:
    //! Key to identify atoms.
    using Key = Symbol const *;

    //! Construct the base.
    //!
    //! The given elements form the base. They are managed externally and the
    //! base just provides a view on them. Elements must be explicitly added
    //! to the view using the add method.
    BaseCondLitPremise(MapElemCondLit &elems) : elems_{&elems} {}

    //! Add a blocked element to the base.
    void add(MapElemCondLit::iterator it) {
        // Note: see note in add_premise
        // assert(it.value().is_blocked());
        it.value().set_offset(base_.size());
        base_.emplace_back(std::distance(elems_->begin(), it));
    }

    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t {
        if (auto it = elems_->find(key); it != elems_->end() && it->second.has_offset()) {
            return it->second.offset();
        }
        return size();
    }
    //! Get the number of atoms in the base.
    [[nodiscard]] auto size() const -> size_t { return base_.size(); }

    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) const -> MapElemCondLit::const_iterator { return elems_->nth(base_[i]); }
    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) -> MapElemCondLit::iterator { return elems_->nth(base_[i]); }

  private:
    MapElemCondLit *elems_;
    std::vector<size_t> base_;
};

//! A base for conditional literals.
class BaseCondLit : public BaseImpl<Symbol const *, BaseCondLit> {
  public:
    //! Construct the base.
    //!
    //! The given atoms form the base. They are managed externally and the
    //! base just provides a view on them. Atoms must be explicitly added
    //! to the view using the add method.
    BaseCondLit(MapAtomCondLit &atoms) : atoms_{&atoms} {}

    //! Add a propagated atom to the base.
    void add(MapAtomCondLit::iterator it) {
        it.value().set_offset(base_.size());
        base_.emplace_back(std::distance(atoms_->begin(), it));
    }

    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t {
        if (auto it = atoms_->find(key); it != atoms_->end() && it->second.has_offset()) {
            return it->second.offset();
        }
        return size();
    }
    //! Get the number of atoms in the base.
    [[nodiscard]] auto size() const -> size_t { return base_.size(); }

    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) const -> MapAtomCondLit::const_iterator { return atoms_->nth(base_[i]); }
    //! Get the n-th atom in the base.
    [[nodiscard]] auto nth(size_t i) -> MapAtomCondLit::iterator { return atoms_->nth(base_[i]); }

  private:
    MapAtomCondLit *atoms_;
    std::vector<size_t> base_;
};

//! State to capture a set of conditional literals.
class StateCondLit : public State {
  public:
    //! Construct an empty state.
    StateCondLit(std::pmr::monotonic_buffer_resource &mbr, VariableVec local, VariableVec global, size_t index,
                 bool has_conclusion, bool sp_premise, bool domain)
        : local_{std::move(local)}, global_{std::move(global)}, mbr_{&mbr},
          atoms_{0, Util::array_hash{global_.size()}, Util::array_equal_to{global_.size()}},
          elems_{0, Util::array_hash{local_.size() + 1}, Util::array_equal_to{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index}, has_conclusion_{has_conclusion},
          sp_premise_{sp_premise}, domain_{domain} {
        temp_syms_.reserve(std::max(global_.size(), local_.size() + 1));
    }

    //! Get the variables occurring in the conditional literal.
    void vars(VariableSet &res, bool all) const;

    //! Get the variables occurring in the conditional literal.
    [[nodiscard]] auto vars(bool all) const -> VariableSet;

    //! Get the global variables of the literal.
    [[nodiscard]] auto vars_global() const -> VariableVec const &;

    //! Get the local variables of the literal.
    [[nodiscard]] auto vars_local() const -> VariableVec const &;

    //! Get the update index of the conditional literal.
    [[nodiscard]] auto index() const -> size_t;

    //! Add a new cond lit atom.
    auto add_empty(Assignment const &ass) -> std::pair<MapAtomCondLit::iterator, bool>;

    //! Add a new cond lit element with the given premise.
    //!
    //! If the function returns false the corresponding conditional literal is false.
    auto add_premise(EvalContext const &ctx, ULitVec const &premise) -> bool;

    //! Add a conclusion to an element.
    void add_conclusion(Assignment const &ass, MapAtomCondLit::iterator it, size_t conclusion, bool fact);

    //! Propagate enqueued conditional literals whose elements are not blocked.
    [[nodiscard]] auto propagate() -> bool;

    //! Return true if all contained literals are domain.
    [[nodiscard]] auto domain() const -> bool;

    //! Get the base containing all conditional literals encountered during grounding.
    [[nodiscard]] auto base_empty() -> BaseCondLitEmpty &;
    //! Get the base containing all premises of conditional literals.
    [[nodiscard]] auto base_premise() -> BaseCondLitPremise &;
    //! Get the base containing all conditional literals that have been derived.
    //!
    //! This is a subset of the empty base.
    [[nodiscard]] auto base_lit() -> BaseCondLit &;

    //! Find an atom given an assignment and return an iterator to it.
    //!
    //! Assumes that all global variables are bound.
    //! Returns an end iterator if the atom does not exist.
    [[nodiscard]] auto atom_find(Assignment const &ass) -> MapAtomCondLit::iterator;

    //! Find an atom given an assignment and return its index.
    //!
    //! Assumes that all global variables are bound.
    [[nodiscard]] auto atom_index(Assignment const &ass) -> std::optional<size_t>;

    //! Turn an iterator into an atom index.
    [[nodiscard]] auto atom_index(MapAtomCondLit::const_iterator it) const -> size_t;

    //! Turn an atom index into an iterator.
    [[nodiscard]] auto atom_nth(size_t index) -> MapAtomCondLit::iterator;

    //! Check if the given atom is a fact.
    //!
    //! Note that it is not sufficient to check the fact state of the atom.
    [[nodiscard]] auto atom_is_fact(MapAtomCondLit::iterator it) -> bool;

    //! Get an iterator to an element given an assignment.
    //!
    //! Assumes that the given atom iterator points to a valid atom and all
    //! local and global variables are bound. Returns an end iterator if the
    //! atom does not exist.
    [[nodiscard]] auto elem_find(Assignment const &ass, MapAtomCondLit::iterator it) -> MapElemCondLit::iterator;

    //! Turn an iterator into an element index.
    [[nodiscard]] auto elem_index(MapElemCondLit::const_iterator it) const -> size_t;

    //! Output the now complete conditional literal.
    void output(Logger &log, SymbolStore &store, OutputStm &out) override;

  private:
    VariableVec local_;
    VariableVec global_;
    std::vector<Symbol> mutable temp_syms_;
    std::pmr::monotonic_buffer_resource *mbr_;
    Symbol *syms_atom_ = nullptr;
    MapAtomCondLit atoms_;
    MapElemCondLit elems_;
    std::vector<size_t> propagate_;
    BaseCondLitEmpty base_empty_;
    BaseCondLitPremise base_premise_;
    BaseCondLit base_lit_;
    size_t index_;
    bool has_conclusion_;
    bool sp_premise_;
    bool domain_;
};

//! A term like object used to match conditional literals and their elements.
class MatchCondLit {
  public:
    //! The key to match against.
    using Key = Symbol const *;

    //! Construct the matcher.
    MatchCondLit(StateCondLit &state, LitCondLitType type) : state_{&state}, type_{type} {
        eval_.reserve(type_ == LitCondLitType::premise
                          ? std::max(state_->vars_global().size(), state_->vars_local().size() + 1)
                          : state_->vars_global().size());
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
    friend auto operator<<(std::ostream &out, MatchCondLit const &m) -> std::ostream &;

    //! Get the associated state.
    [[nodiscard]] auto state() const -> StateCondLit &;
    //! Get the type of the matcher.
    [[nodiscard]] auto type() const -> LitCondLitType;

  private:
    [[nodiscard]] static auto match_(Assignment &ass, Symbol const *sym, VariableVec const &vars) -> bool;

    std::vector<Symbol> mutable eval_;
    StateCondLit *state_;
    LitCondLitType type_;
};

//! Helper literals to ground conditional literals.
class LitCondLit : public Lit, private MatchCondLit {
  public:
    //! Construct the literal.
    LitCondLit(LitCondLitType type, StateCondLit &state, size_t index) : MatchCondLit{state, type}, index_{index} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_output(EvalContext const &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t index_;
    size_t offset_ = 0;
};

//! Helper literal to ground stratified conditional literals.
class LitCondLitStrat : public Lit, private InstanceCallback {
  public:
    //! Construct the literal.
    LitCondLitStrat(StateCondLit &state, ULitVec premise, ProfileNodeInternal *node)
        : state_{&state}, premise_{std::move(premise)}, node_{node} {}

  private:
    // lit interface
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                                  std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_output(EvalContext const &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    // cb interface
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;

    StateCondLit *state_;
    ULitVec premise_;
    ProfileNodeInternal *node_;
};

//! Type of the helper statement to ground conditional literals.
enum class StmCondLitType : uint8_t {
    empty = 0,      //!< Gather conditional literals for grounding.
    premise = 1,    //!< Gather premises of conditional literals.
    conclusion = 2, //!< Gather conclusions of conditional literals.
};
//! Print the type.
auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream &;

//! Helper statement to ground conditional literals.
class StmCondLit : public Stm {
  public:
    //! Construct the statement.
    StmCondLit(StmCondLitType type, StateCondLit &base, ULitVec body, size_t prio, size_t index,
               ProfileNodeInternal *node)
        : state_{&base}, body_{std::move(body)}, prio_{prio}, index_{index}, node_{node}, type_{type} {}

  private:
    // statement interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // solution callback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    [[nodiscard]] auto do_profile_node() const -> ProfileNodeInternal * override { return node_; }

    StateCondLit *state_;
    ULitVec body_;
    size_t prio_;
    size_t index_;
    ProfileNodeInternal *node_;
    StmCondLitType type_;
};

//! @}

} // namespace CppClingo::Ground
