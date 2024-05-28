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
    [[nodiscard]] auto is_false() const {
        return premise_is_fact_ != 0 && conclusion_truth_ == TruthConclusion::false_;
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
        if (enqueued_ == 0 && propagated_ == 0 &&
            (elems_propagated_ == elems_.size() || !elems.nth(elems_[elems_propagated_]).value().is_blocked())) {
            enqueued_ = 1;
            return true;
        }
        return false;
    }
    [[nodiscard]] auto propagate(MapElemCondLit const &elems) -> bool {
        assert(propagated_ == 0 && enqueued_ != 0);
        enqueued_ = 0;
        for (auto n = elems_.size(); elems_propagated_ < n; ++elems_propagated_) {
            auto const &elem = elems.nth(elems_[elems_propagated_]).value();
            if (elem.is_blocked()) {
                if (elem.is_false()) {
                    false_ = 1;
                }
                return false;
            }
        }
        propagated_ = 1;
        return true;
    }
    [[nodiscard]] auto is_blocked() const -> bool { return elems_propagated_ < elems_.size(); }
    [[nodiscard]] auto is_fact(MapElemCondLit const &elems) const -> bool {
        return std::all_of(elems_.begin(), elems_.end(),
                           [&elems](auto idx) { return elems.nth(idx).value().is_fact(); });
    }
    [[nodiscard]] auto is_false() const -> bool { return false_ != 0; }
    void set_offset(size_t offset) { offset_ = offset; }
    [[nodiscard]] auto offset() const -> size_t { return offset_; }

  private:
    std::vector<size_t> elems_;
    uint64_t elems_propagated_ : 61 = 0;
    uint64_t propagated_ : 1 = 0;
    uint64_t enqueued_ : 1 = 0;
    uint64_t false_ : 1 = 0;
    size_t offset_ = 0;
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
    StateCondLit(VariableVec local, VariableVec global, size_t index, bool has_conclusion, bool rec_premise,
                 bool domain)
        : local_{std::move(local)}, global_{std::move(global)}, syms_elems_{local_.size() + 1},
          syms_atoms_{global_.size()}, atoms_{0, Util::SpanHash{global_.size()}, Util::SpanEqualTo{global_.size()}},
          elems_{0, Util::SpanHash{local_.size() + 1}, Util::SpanEqualTo{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index}, has_conclusion_{has_conclusion},
          rec_premise_{rec_premise}, domain_{domain} {
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
    auto add_empty(Assignment const &ass) -> std::pair<MapAtomCondLit::iterator, bool>;

    //! Add a new cond lit element.
    void add_premise(Assignment const &ass, bool fact);

    //! Add a conclusion to an element.
    void add_conclusion(Assignment const &ass, bool fact);

    //! Propagate enqueued conditional literals whose elements are not blocked.
    [[nodiscard]] auto propagate() -> bool;

    //! Return true if all contained literals are domain and the premise is not recursive.
    [[nodiscard]] auto domain() const -> bool;

    [[nodiscard]] auto base_empty() -> BaseCondLitEmpty &;
    [[nodiscard]] auto base_premise() -> BaseCondLitPremise &;
    [[nodiscard]] auto base_lit() -> BaseCondLit &;

    [[nodiscard]] auto lit_is_fact(Assignment const &ass) -> bool;

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
    bool domain_;
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
    LitCondLit(LitCondLitType type, StateCondLit &state, size_t index) : MatchCondLit{state, type}, index_{index} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_recursive() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_output(InstantiationContext &ctx) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    size_t index_;
};

class LitCondLitStrat : public Lit, private InstanceCallback {
  public:
    LitCondLitStrat(StateCondLit &state, ULitVec premise) : state_{&state}, premise_{std::move(premise)} {}

  private:
    // lit interface
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_recursive() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_output(InstantiationContext &ctx) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    // cb interface
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override;
    void do_propagate(Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;
    void do_print_head(std::ostream &out) const override;

    StateCondLit *state_;
    ULitVec premise_;
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

  private:
    // statement interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // solution callback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override;
    void do_propagate(Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override;

    StateCondLit *base_;
    ULitVec body_;
    size_t prio_;
    size_t index_;
    StmCondLitType type_;
};

} // namespace Gringo::Ground
