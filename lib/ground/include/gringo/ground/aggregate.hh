#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/span_stack.hh>

namespace Gringo::Ground {

// TODO:
// maybe rename to state
// map local symbols -> to state
// state:
//   - set of global symbols in element
//   - propagated or not
//     - not yet propagated literals can still be blocked
//     - a literal is blocked if one of its premises is true
//       but the conclusion false or not yet derived
//     - if the conclusion is false, the whole literal becomes false
//       and does not need to be propagated anymore
//   - determine if fact when condition is stratified
//     - if the premise is stratified then the literal can be marked as fact
//       if all associated conclusions are true
//     - needs a flag in base
//   - the literal has to be propagated either by the premise or conclusion statement
//     - if the conclusion is false there is no corresponding statement
//       and the premise statement can trigger propagation
//     - needs flag in base
// there are three associated bases
// - empty: any conditional literal encountered during grounding
// - premise:
//   - premises accumulated
//   - only necessary if there is a conclusion
//   - the key is the index of the atom and the global variables
//     (by misusing the representation of the symbol,
//     the index could be stored using to_rep/from_rep)
//   - adding an element would mean having to lookup the atom
//     (which should be fine)
// - lit:
//   - subset of empty
//   - gathers propagated atoms
//   - can be represented using a set of integers
//     (atoms are already stored in the atom table and can be addressed by index)
struct BaseCondLit {
  public:
    // NOLINTNEXTLINE(performance-enum-size)
    enum class TruthConclusion : size_t {
        true_ = 0,
        false_ = 1,
        derived = 2,
        unknown = 3,
    };
    struct ElemState {
      public:
        ElemState(bool premise_is_fact) : premise_is_fact_{static_cast<uint8_t>(premise_is_fact)} {}
        void mark_conclusion(bool fact) {
            assert(conclusion_truth_ == TruthConclusion::unknown);
            conclusion_truth_ = fact ? TruthConclusion::true_ : TruthConclusion::derived;
        }
        [[nodiscard]] auto is_fact() const -> bool {
            return premise_is_fact_ != 0 && conclusion_truth_ == TruthConclusion::true_;
        }
        [[nodiscard]] auto is_blocked() const -> bool {
            return premise_is_fact_ != 0 &&
                   (conclusion_truth_ == TruthConclusion::false_ || conclusion_truth_ == TruthConclusion::unknown);
        }
        void set_offset(size_t offset) { offset_ = offset; }
        [[nodiscard]] auto offset() const -> size_t { return offset_; }

      private:
        size_t offset_ : 56 = 0;
        TruthConclusion conclusion_truth_ : 7 = TruthConclusion::unknown;
        size_t premise_is_fact_ : 1;
    };
    // we can use here that the number of local variables is fixed
    using ElemMap = Util::ordered_map<Symbol const *, ElemState, Util::SpanHash, Util::SpanEqualTo>;

    struct AtomState {
      public:
        AtomState() = default;
        void add_elem(size_t index) { elems_.emplace_back(index); }
        [[nodiscard]] auto enqueue(ElemMap const &elems) -> bool {
            if (!enqueued_ && !propagated_ && !elems.nth(elems_propagated_).value().is_blocked()) {
                enqueued_ = true;
                return true;
            }
            return false;
        }
        [[nodiscard]] auto propagate(ElemMap const &elems) -> bool {
            assert(!propagated_);
            for (auto n = elems_.size(); elems_propagated_ < n; ++elems_propagated_) {
                if (elems.nth(elems_[n]).value().is_blocked()) {
                    return false;
                }
            }
            propagated_ = true;
            return true;
        }
        void set_offset(size_t offset) { offset_ = offset; }
        [[nodiscard]] auto offset() const -> size_t { return offset_; }

      private:
        std::vector<size_t> elems_;
        size_t offset_ = 0;
        size_t elems_propagated_ = 0;
        bool propagated_ = false;
        bool enqueued_ = false;
    };
    // we can use here that the number of global variables is fixed
    using AtomMap = Util::ordered_map<Symbol const *, AtomState, Util::SpanHash, Util::SpanEqualTo>;

    class BaseEmpty {
      public:
        BaseEmpty(AtomMap &atoms) : atoms_{&atoms} {}

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = static_cast<size_t>(std::distance(atoms_->begin(), atoms_->find(sym)));
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_->nth(i); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> AtomMap::iterator { return atoms_->nth(i); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, atoms_->size()); }

      private:
        // TODO: derived and unknown atoms might have to be distinguished
        AtomMap *atoms_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    struct BasePremise {
        BasePremise(ElemMap &elems) : elems_{&elems} {}

        //! Add a blocked element to the base.
        void add(ElemMap::iterator it) {
            assert(it.value().is_blocked());
            it.value().set_offset(base_.size());
            base_.emplace_back(std::distance(elems_->begin(), it));
        }

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = elems_->find(sym)->second.offset();
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> ElemMap::const_iterator { return elems_->nth(base_[i]); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> ElemMap::iterator { return elems_->nth(base_[i]); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, elems_->size()); }

      private:
        ElemMap *elems_;
        std::vector<size_t> base_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    struct BaseLit {
        BaseLit(AtomMap &atoms) : atoms_{&atoms} {}

        //! Add a propagated atom to the base.
        void add(AtomMap::iterator it) {
            it.value().set_offset(base_.size());
            base_.emplace_back(std::distance(atoms_->begin(), it));
        }

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = atoms_->find(sym)->second.offset();
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_->nth(base_[i]); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> AtomMap::iterator { return atoms_->nth(base_[i]); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, base_.size()); }

      private:
        AtomMap *atoms_;
        std::vector<size_t> base_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    BaseCondLit(VariableVec local, VariableVec global, size_t index)
        : local_{std::move(local)}, global_{std::move(global)}, syms_elems_{local_.size() + 1},
          syms_atoms_{global_.size()}, atoms_{0, Util::SpanHash{global_.size()}, Util::SpanEqualTo{global_.size()}},
          elems_{0, Util::SpanHash{local_.size() + 1}, Util::SpanEqualTo{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index} {
        temp_syms_.reserve(std::max(global_.size(), local_.size() + 1));
    }

    //! Get the variables occuring in the conditional literal.
    void vars(VariableSet &res, bool all) const {
        if (all) {
            res.insert(local_.begin(), local_.end());
        }
        res.insert(global_.begin(), global_.end());
    }

    //! Get the variables occuring in the conditional literal.
    [[nodiscard]] auto vars(bool all) const -> VariableSet {
        VariableSet res;
        res.reserve(all ? global_.size() + local_.size() : global_.size());
        vars(res, all);
        return res;
    }

    //! Get the update index of the conditional literal.
    [[nodiscard]] auto index() const -> size_t { return index_; }

    //! Add a new cond lit atom.
    void add_empty(Assignment const &ass) {
        auto const syms = syms_atoms_.push_map(global_, [&ass](auto var) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return ass[var].value();
        });
        if (!atoms_.try_emplace(syms.data()).second) {
            syms_atoms_.pop();
        }
    }

    //! Add a new cond lit element.
    void add_premise(Assignment const &ass, bool fact) {
        // TODO:
        // - if the conclusion is fixed to false, set the truth member accordingly
        auto it = find_atom(ass);
        auto syms_elem = syms_elems_.push_imap([this, it, &ass](size_t i) {
            if (i == 0) {
                return Symbol::from_rep(std::distance(atoms_.begin(), it));
            }
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return ass[global_[i - 1]].value();
        });

        auto [jt, ins] = elems_.try_emplace(syms_elem.data(), fact);
        // an element can only be added once
        assert(ins);

        auto &atom = it.value();
        auto &elem = jt.value();

        // TODO:
        // - in the stratified case, unblock the element right away if the premise is true and the conclusion is not
        // false
        // - if the conclusion is false, mark the element and do not add it to the base because it cannot become true

        atom.add_elem(std::distance(elems_.begin(), jt));
        if (elem.is_blocked()) {
            base_premise_.add(jt);
        } else if (atom.enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    }

    //! Add a conclusion to an element.
    void add_conclusion(Assignment const &ass, bool fact) {
        auto it = find_atom(ass);
        auto jt = find_elem(ass, it);
        auto &atom = it.value();
        auto &elem = jt.value();
        elem.mark_conclusion(fact);
        if (atom.enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    }

    //! Propagate enqueued conditional literals whose elements are not blocked.
    auto propagate() -> bool {
        bool res = false;
        for (auto atom_index : propagate_) {
            auto it = atoms_.nth(atom_index);
            auto &atom = it.value();
            if (atom.propagate(elems_)) {
                base_lit_.add(it);
                res = true;
            }
        }
        propagate_.clear();
        return res;
    }

    /*
    // Base interface
    //! Get the number of atoms in the base.
    [[nodiscard]] auto size() const { return atoms_.size(); }

    //! Check if the base is domain.
    //!
    //! A base is domain if it contains facts only.
    [[nodiscard]] auto domain() const {
        for (auto n = atoms_.size(); domain_offset_ < n; ++domain_offset_) {
            if (!atoms_.nth(domain_offset_)->second.is_fact) {
                return false;
            }
        }
        return true;
    }

    //! Add an atom to the base.
    auto add(Symbol atom, AtomState state) -> AtomUpdate {
        // turning a delayed atom into an active one is tricky
        // suppose p(2) below has been inserted as delayed on a previous generation
        //   p(1) *p(2) p(3)
        // It now has to be added as active and should also become part of the new generation.
        // The easiest way to implement this is to delay inserting into atoms_ adding it to a separate set first.
        //   active:  p(1) p(3)
        //   delayed: p(2)
        // Downside: each insertion has to check the delayed set first (which should however be empty in most cases).
        if (auto [it, ins] = atoms_.try_emplace(atom, 0, state); !ins) {
            if (state < it->second.state) {
                // note transitions from external to unknown are ignored
                // because there is no additional information for grounding
                auto prev = it.value().state;
                it.value().state = state;
                if (prev == AtomState::derived) {
                    return AtomUpdate::added;
                }
                if (state == AtomState::fact) {
                    return AtomUpdate::changed;
                }
            }
            return AtomUpdate::unchanged;
        }
        return AtomUpdate::added;
    }

    //! Return a span with all atoms in the base.
    auto atoms() const -> std::span<Atom const> { return {atoms_.values_container().data(), all_offset_}; }
    //! Check if the base contains at least one atom from the all generation.
    auto has_update() const -> bool { return atoms_.size() > all_offset_; }
    //! Check if the base contains the given atom.
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool { return atoms_.find(sym) != atoms_.end(); }
    //! Get the index of the first atom in the given generation.
    auto begin(MatcherType type) const -> size_t {
        if (type == MatcherType::new_atoms) {
            return old_offset_;
        }
        return 0;
    }
    //! Get the index plus one of the last atom in the given generation.
    auto end(MatcherType type) const -> size_t {
        if (type == MatcherType::old_atoms) {
            return old_offset_;
        }
        return all_offset_;
    }
    //! Check if the base contains the given atom with in the given generation.
    [[nodiscard]] auto contains(Symbol const &sym, MatcherType type) const -> bool {
        auto index = static_cast<size_t>(std::distance(atoms_.begin(), atoms_.find(sym)));
        return begin(type) <= index && index < end(type);
    }
    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state == AtomState::fact;
    }

    //! Get the context of the base.
    template <class T> auto context() -> T & {
        if (context_ != nullptr) {
            if (auto res = dynamic_cast<T *>(context_.get()); res != nullptr) {
                return *res;
            }
            throw std::bad_cast();
        }
        context_ = std::make_unique<T>();
        return static_cast<T &>(*context_);
    }
    */

  private:
    [[nodiscard]] auto find_atom(Assignment const &ass) -> AtomMap::iterator {
        temp_syms_.clear();
        for (auto var : global_) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            temp_syms_.emplace_back(ass[var].value());
        }
        return atoms_.find(temp_syms_.data());
    }

    [[nodiscard]] auto find_elem(Assignment const &ass, AtomMap::iterator it) -> ElemMap::iterator {
        temp_syms_.clear();
        temp_syms_.emplace_back(Symbol::from_rep(std::distance(atoms_.begin(), it)));
        for (auto var : global_) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            temp_syms_.emplace_back(ass[var].value());
        }
        return elems_.find(temp_syms_.data());
    }

    VariableVec local_;
    VariableVec global_;
    std::vector<Symbol> temp_syms_;
    Util::SpanStack<Symbol> syms_elems_;
    Util::SpanStack<Symbol> syms_atoms_;
    AtomMap atoms_;
    ElemMap elems_;
    std::vector<size_t> propagate_;
    BaseEmpty base_empty_;
    BasePremise base_premise_;
    BaseLit base_lit_;
    size_t index_ = 0;
};

enum class LitCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
    lit = 4,
};
auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream &;

class LitCondLit : public Lit {
  public:
    LitCondLit(LitCondLitType type, BaseCondLit &base, size_t index) : base_{&base}, index_{index}, type_{type} {}
    void vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto domain(bool domain) const -> bool override;
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
    BaseCondLit *base_;
    size_t index_;
    LitCondLitType type_;
};

enum class StmCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
};
auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream &;

class StmCondLit : public Stm {
  public:
    StmCondLit(StmCondLitType type, BaseCondLit &base, ULitVec body, size_t prio, size_t index)
        : base_{&base}, body_{std::move(body)}, prio_{prio}, index_{index}, type_{type} {}
    // statement interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // solution callback interface
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override;

  private:
    BaseCondLit *base_;
    ULitVec body_;
    size_t prio_;
    size_t index_;
    StmCondLitType type_;
};

} // namespace Gringo::Ground
