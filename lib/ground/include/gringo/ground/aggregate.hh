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
    struct ElemState {
        bool is_fact;
        bool is_blocked;
    };
    // we can use here that the number of local variables is fixed
    using ElemMap = Util::ordered_map<Symbol const *, ElemState, Util::SpanHash, Util::SpanEqualTo>;

    struct AtomState {
        bool is_fact;
        size_t num_blocked;
        size_t lit_index_;
        std::vector<size_t> elems;
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
        AtomMap *atoms_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    struct BasePremise {
        BasePremise(ElemMap &elems) : elems_{&elems} {}

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = static_cast<size_t>(std::distance(elems_->begin(), elems_->find(sym)));
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> ElemMap::const_iterator { return elems_->nth(i); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> ElemMap::iterator { return elems_->nth(i); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, elems_->size()); }

      private:
        ElemMap *elems_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    struct BaseLit {
        BaseLit(AtomMap &atoms) : atoms_{&atoms} {}

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = atoms_->find(sym)->second.lit_index_;
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
        : local_{std::move(local)}, global_{std::move(global)},
          atoms_{0, Util::SpanHash{global_.size()}, Util::SpanEqualTo{global_.size()}},
          elems_{0, Util::SpanHash{local_.size() + 1}, Util::SpanEqualTo{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index} {}

    void vars(VariableSet &res, bool all) const {
        if (all) {
            res.insert(local_.begin(), local_.end());
        }
        res.insert(global_.begin(), global_.end());
    }

    [[nodiscard]] auto vars(bool all) const -> VariableSet {
        VariableSet res;
        res.reserve(all ? global_.size() + local_.size() : global_.size());
        vars(res, all);
        return res;
    }

    [[nodiscard]] auto index() const -> size_t { return index_; }

    void add_empty() {
        // TODO:
        // - add directly to atoms_
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
    VariableVec local_;
    VariableVec global_;
    AtomMap atoms_;
    ElemMap elems_;
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
