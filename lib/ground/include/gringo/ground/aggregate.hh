#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enumerate.hh>
#include <gringo/util/span_stack.hh>

#include <iostream>

namespace Gringo::Ground {

enum class LitCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    lit = 2,
};
auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream &;

template <class Base>
concept IsBase = requires(Base &b) {
    b.begin(std::declval<MatcherType>());
    b.end(std::declval<MatcherType>());
    b.contains(std::declval<typename Base::Key>(), std::declval<MatcherType>());
    { b.nth(std::declval<size_t>())->first } -> std::same_as<typename Base::Key const &>;
    b.update(size_t{0});
    { b.template context<int>() } -> std::same_as<int &>;
} && requires(Base const &b) {
    { b.nth(std::declval<size_t>())->first } -> std::same_as<typename Base::Key const &>;
};

template <class Match>
concept IsMatch = requires(Match const &m) {
    { m.vars() } -> std::same_as<VariableSet>;
    m.match(std::declval<SymbolStore &>(), std::declval<typename Match::Key>(), std::declval<Assignment &>());
    m.eval(std::declval<SymbolStore &>(), std::declval<Assignment &>());
    m.signature(std::declval<VariableSet const &>(), std::declval<VariableSet const &>());
    std::declval<std::ostream &>() << m;
};

// NOLINTNEXTLINE(performance-enum-size)
enum class TruthConclusion : uint64_t {
    true_ = 0,
    false_ = 1,
    derived = 2,
    unknown = 3,
};

struct StateCondLitElem {
  public:
    StateCondLitElem(bool premise_is_fact, bool has_conclusion)
        : conclusion_truth_{has_conclusion ? TruthConclusion::unknown : TruthConclusion::false_},
          premise_is_fact_{static_cast<uint8_t>(premise_is_fact)} {}
    void mark_conclusion(bool fact) {
        assert(conclusion_truth_ == TruthConclusion::unknown);
        conclusion_truth_ = fact ? TruthConclusion::true_ : TruthConclusion::derived;
    }
    [[nodiscard]] auto is_fact() const -> bool { return conclusion_truth_ == TruthConclusion::true_; }
    [[nodiscard]] auto is_blocked() const -> bool {
        return premise_is_fact_ != 0 &&
               (conclusion_truth_ == TruthConclusion::false_ || conclusion_truth_ == TruthConclusion::unknown);
    }
    void set_offset(size_t offset) { offset_ = offset; }
    [[nodiscard]] auto offset() const -> size_t { return offset_; }

  private:
    uint64_t offset_ : 56 = 0;
    TruthConclusion conclusion_truth_ : 7;
    uint64_t premise_is_fact_ : 1;
};

// we can use here that the number of local variables is fixed
using MapElemCondLit = Util::ordered_map<Symbol const *, StateCondLitElem, Util::SpanHash, Util::SpanEqualTo>;

class StateAtomCondLit {
  public:
    StateAtomCondLit() = default;
    void add_elem(size_t index) { elems_.emplace_back(index); }
    [[nodiscard]] auto enqueue(MapElemCondLit const &elems) -> bool {
        if (!enqueued_ && !propagated_ &&
            (elems_propagated_ == elems_.size() || !elems.nth(elems_[elems_propagated_]).value().is_blocked())) {
            enqueued_ = true;
            return true;
        }
        return false;
    }
    [[nodiscard]] auto propagate(MapElemCondLit const &elems) -> bool {
        assert(!propagated_ && enqueued_);
        enqueued_ = false;
        for (auto n = elems_.size(); elems_propagated_ < n; ++elems_propagated_) {
            if (elems.nth(elems_[elems_propagated_]).value().is_blocked()) {
                return false;
            }
        }
        propagated_ = true;
        return true;
    }
    [[nodiscard]] auto is_fact(MapElemCondLit const &elems) const -> bool {
        return std::all_of(elems_.begin(), elems_.end(),
                           [&elems](auto idx) { return elems.nth(idx).value().is_fact(); });
    }
    [[nodiscard]] auto is_false() const -> bool { return false_; }
    void set_offset(size_t offset) { offset_ = offset; }
    [[nodiscard]] auto offset() const -> size_t { return offset_; }

  private:
    std::vector<size_t> elems_;
    size_t offset_ = 0;
    size_t elems_propagated_ = 0;
    bool propagated_ = false;
    bool enqueued_ = false;
    bool false_ = false;
};
using MapAtomCondLit = Util::ordered_map<Symbol const *, StateAtomCondLit, Util::SpanHash, Util::SpanEqualTo>;

class BaseCondLitEmpty : public BaseImpl<Symbol const *, BaseCondLitEmpty> {
  public:
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

class BaseCondLitPremise : public BaseImpl<Symbol const *, BaseCondLitPremise> {
  public:
    using Key = Symbol const *;

    BaseCondLitPremise(MapElemCondLit &elems) : elems_{&elems} {}

    //! Add a blocked element to the base.
    void add(MapElemCondLit::iterator it) {
        assert(it.value().is_blocked());
        it.value().set_offset(base_.size());
        base_.emplace_back(std::distance(elems_->begin(), it));
    }

    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t { return elems_->find(key)->second.offset(); }
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

class BaseCondLit : public BaseImpl<Symbol const *, BaseCondLit> {
  public:
    BaseCondLit(MapAtomCondLit &atoms) : atoms_{&atoms} {}

    //! Add a propagated atom to the base.
    void add(MapAtomCondLit::iterator it) {
        it.value().set_offset(base_.size());
        base_.emplace_back(std::distance(atoms_->begin(), it));
    }

    //! Map a key to its index in the base.
    [[nodiscard]] auto index(Key const &key) const -> size_t { return atoms_->find(key)->second.offset(); }
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

struct StateCondLit {
  public:
    StateCondLit(VariableVec local, VariableVec global, size_t index, bool has_conclusion, bool rec_premise)
        : local_{std::move(local)}, global_{std::move(global)}, syms_elems_{local_.size() + 1},
          syms_atoms_{global_.size()}, atoms_{0, Util::SpanHash{global_.size()}, Util::SpanEqualTo{global_.size()}},
          elems_{0, Util::SpanHash{local_.size() + 1}, Util::SpanEqualTo{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index}, has_conclusion_{has_conclusion},
          rec_premise_{rec_premise} {
        temp_syms_.reserve(std::max(global_.size(), local_.size() + 1));
    }

    //! Get the variables occuring in the conditional literal.
    void vars(VariableSet &res, bool all) const;

    //! Get the variables occuring in the conditional literal.
    [[nodiscard]] auto vars(bool all) const -> VariableSet;

    //! Get the global variables of the literal.
    [[nodiscard]] auto vars_global() const -> VariableVec const &;

    //! Get the local variables of the literal.
    [[nodiscard]] auto vars_local() const -> VariableVec const &;

    //! Get the update index of the conditional literal.
    [[nodiscard]] auto index() const -> size_t;

    //! Add a new cond lit atom.
    void add_empty(Assignment const &ass);

    //! Add a new cond lit element.
    void add_premise(Assignment const &ass, bool fact);

    //! Add a conclusion to an element.
    void add_conclusion(Assignment const &ass, bool fact);

    //! Propagate enqueued conditional literals whose elements are not blocked.
    auto propagate() -> bool;

    auto base_empty() -> BaseCondLitEmpty &;
    auto base_premise() -> BaseCondLitPremise &;
    auto base_lit() -> BaseCondLit &;

    auto lit_is_fact(Assignment const &ass);

    [[nodiscard]] auto atom_index(Assignment &ass) const -> std::optional<size_t>;

    [[nodiscard]] auto atom_nth(size_t index) -> MapAtomCondLit::iterator;

  private:
    [[nodiscard]] auto atom_index(MapAtomCondLit::const_iterator it) const -> size_t;

    [[nodiscard]] auto atom_find(Assignment const &ass) const -> MapAtomCondLit::const_iterator;

    [[nodiscard]] auto atom_find(Assignment const &ass) -> MapAtomCondLit::iterator;

    [[nodiscard]] auto elem_find(Assignment const &ass, MapAtomCondLit::iterator it) -> MapElemCondLit::iterator;

    VariableVec local_;
    VariableVec global_;
    std::vector<Symbol> mutable temp_syms_;
    Util::SpanStack<Symbol> syms_elems_;
    Util::SpanStack<Symbol> syms_atoms_;
    MapAtomCondLit atoms_;
    MapElemCondLit elems_;
    std::vector<size_t> propagate_;
    BaseCondLitEmpty base_empty_;
    BaseCondLitPremise base_premise_;
    BaseCondLit base_lit_;
    size_t index_;
    bool has_conclusion_;
    bool rec_premise_;
};

class MatchCondLit {
  public:
    using Key = Symbol const *;
    MatchCondLit(StateCondLit &state, LitCondLitType type) : state_{&state}, type_{type} {
        eval_.reserve(type_ == LitCondLitType::premise
                          ? std::max(state_->vars_global().size(), state_->vars_local().size() + 1)
                          : state_->vars_global().size());
    }

    [[nodiscard]] auto vars() const -> VariableSet;

    [[nodiscard]] auto signature(VariableSet const &bound,
                                 [[maybe_unused]] VariableSet const &bind) const -> VariableVec;

    [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool;

    [[nodiscard]] auto eval([[maybe_unused]] SymbolStore &store,
                            Assignment &ass) const -> std::optional<Symbol const *>;

    friend auto operator<<(std::ostream &out, MatchCondLit const &m) -> std::ostream &;

    [[nodiscard]] auto state() const -> StateCondLit &;
    [[nodiscard]] auto type() const -> LitCondLitType;

  private:
    [[nodiscard]] static auto match_(Assignment &ass, Symbol const *sym, VariableVec const &vars) -> bool;

    std::vector<Symbol> mutable eval_;
    StateCondLit *state_;
    LitCondLitType type_;
};

class LitCondLit : public Lit, private MatchCondLit {
  public:
    LitCondLit(LitCondLitType type, StateCondLit &base, size_t index) : MatchCondLit{base, type}, index_{index} {}
    void vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto domain() const -> bool override;
    [[nodiscard]] auto recursive() const -> bool override;
    [[nodiscard]] auto matcher(MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto score(std::vector<bool> const &bound) const -> double override;
    void print(std::ostream &out) const override;
    auto output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool override;
    [[nodiscard]] auto copy() const -> ULit override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Lit const &other) const -> std::weak_ordering override;

  private:
    size_t index_;
};

enum class StmCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
};
auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream &;

class StmCondLit : public Stm {
  public:
    StmCondLit(StmCondLitType type, StateCondLit &base, ULitVec body, size_t prio, size_t index)
        : base_{&base}, body_{std::move(body)}, prio_{prio}, index_{index}, type_{type} {}
    // statement interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // solution callback interface
    void print_head(std::ostream &out) const override;
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override;

  private:
    StateCondLit *base_;
    ULitVec body_;
    size_t prio_;
    size_t index_;
    StmCondLitType type_;
};

} // namespace Gringo::Ground
