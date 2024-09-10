#pragma once

#include <gringo/ground/instantiator.hh>

#include <gringo/core/symbol.hh>

#include <gringo/util/index_sequence.hh>
#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

namespace Gringo::Ground {

//! @addtogroup ground_base
//! @{

//! A set of variables.
using VariableSet = Util::ordered_set<size_t>;
//! A vector of variables.
using VariableVec = VariableSet::values_container_type;

//! Enumeration to capture the state of an atom.
// NOLINTNEXTLINE(performance-enum-size)
enum class StateAtom : uint64_t {
    //! Indicates that the atom is derived by a fact.
    fact = 0,
    //! Indicates that the atom is derived by some rule but not a fact.
    derived = 1,
    //! Indicates that the atom has not yet been derived by a rule.
    //!
    //! At the time rule (1) is grounded, atom x is not yet defined.
    //! Once (2) has been grounded, there is a definition for it.
    //!
    //!   a :- not x. (1)
    //!   x :- a.     (2)
    //!
    //! The flag indicates atoms that have neither been derived by facts, rules, or externals.
    unknown = 2,
};

//! Capture the state of an atom.
struct AtomInfo {
    uint64_t id : 63;    //!< A unique id among all atoms.
    StateAtom state : 2; //!< The atom state.
};

//! An atom consisting of a symbol and its (mutable) state.
using SymbolicAtom = std::pair<Symbol, AtomInfo>;

//! Enumeration indicating state updates of atoms.
enum class AtomUpdate : uint8_t {
    added = 0,     //!< A freshly added atom.
    changed = 1,   //!< An update atom.
    unchanged = 2, //!< An atom whose state did not change.
};

//! Helper class to manage generation counts.
//!
//! Old, new, and the all generation are considered. The latter is the union of
//! the prior.
class GenerationCounts {
  public:
    //! Update the generation counts.
    //!
    //! Calling with generation zero resets the counts. Calling with a
    //! generation greater than the current generation adds all atoms from the
    //! current all generation to the old generation.
    //!
    //! Calling the function multiple times for the same generation has no
    //! effect.

    //! Get the index of the first atom in the given generation.
    [[nodiscard]] auto begin(MatcherType type) const -> size_t {
        if (type == MatcherType::new_atoms) {
            return old_offset_;
        }
        return 0;
    }

    //! Get the index plus one of the last atom in the given generation.
    [[nodiscard]] auto end(MatcherType type) const -> size_t {
        if (type == MatcherType::old_atoms) {
            return old_offset_;
        }
        return all_offset_;
    }

    //! Check if the given index belongs to the given generation.
    [[nodiscard]] auto contains(size_t index, MatcherType type) const -> bool {
        return begin(type) <= index && index < end(type);
    }

    //! Check if there is an update.
    [[nodiscard]] auto has_update(size_t size) const -> bool { return all_offset_ < size; }

    //! Update the current generations.
    //!
    //! Generations should always increase. They can however be reset by
    //! setting the generation to 0.
    void update(size_t generation, size_t size) {
        // initialize the domain
        // (all atoms are marked as new)
        if (generation == 0) {
            generation_ = 0;
            old_offset_ = 0;
            all_offset_ = size;
        }
        // the generation has been incremented by one
        // (freshly added atoms are marked new)
        else if (generation_ + 1 == generation) {
            generation_ = generation;
            old_offset_ = all_offset_;
            all_offset_ = size;
            // the generation has been incremented by more than one
            // (all atoms are marked old)
        } else if (generation_ + 1 < generation) {
            generation_ = generation;
            old_offset_ = size;
            all_offset_ = size;
        }
    }

  private:
    //! Symbols before this offset are considered old.
    size_t old_offset_ = 0;
    //! Symbols before this offset are considered new or old.
    size_t all_offset_ = 0;
    //! The last generation at which the domain has been updated.
    size_t generation_ = 0;
};

//! A context object.
//!
//! Currently, the interface is empty and one could use a std::any as well.
//! The class exists for easy extensibility.
class BaseContext {
  public:
    //! Destroy the context.
    virtual ~BaseContext() = default;
};

//! The base implementation of an atom base.
//!
//! It implements generation counting and support for adding a context object.
template <class KeyType, class BaseType> class BaseImpl {
  public:
    //! The key identifies an atom and is usually associated with further state.
    using Key = KeyType;

    //! Get the index of the first atom in the given generation.
    [[nodiscard]] auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

    //! Get the index plus one of the last atom in the given generation.
    [[nodiscard]] auto end(MatcherType type) const -> size_t { return counts_.end(type); }

    //! Check if the base contains the given atom with in the given generation.
    [[nodiscard]] auto contains(Key const &sym, MatcherType type) const -> std::optional<size_t> {
        if (auto idx = base().index(sym); counts_.contains(idx, type)) {
            return idx;
        }
        return std::nullopt;
    }

    //! Update the generation counts.
    void update(size_t generation) { counts_.update(generation, base().size()); }

    //! Get the context of the base with the desired type.
    //!
    //! Creates one if the base has none yet.
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

    //! Clear the current context.
    void clear_context() { context_ = nullptr; }

    //! Check if the base has an update.
    [[nodiscard]] auto has_update() const -> bool { return counts_.has_update(base().size()); }

  private:
    [[nodiscard]] auto base() -> BaseType & { return *static_cast<BaseType *>(this); }
    [[nodiscard]] auto base() const -> BaseType const & { return *static_cast<BaseType const *>(this); }

    GenerationCounts counts_;
    std::unique_ptr<BaseContext> context_;
};

//! An atom base used to store derivable atoms and associated state.
//!
//! The base tracks the generation of atoms for semi-naive evaluation,
//! and the state of atoms.
//!
//! An atom base can also stores unknown atoms. For such atoms it is not yet
//! know whether there will be a rule deriving them. The only purpose is to
//! store them here is to associated them with a unique id.
class Base : public BaseImpl<Symbol, Base> {
  public:
    using BaseImpl::contains;
    //! Map containing the atoms.
    using MapAtom = Util::ordered_map<Symbol, AtomInfo>;

    //! Check if the base is domain.
    //!
    //! A base is domain if it contains facts only.
    [[nodiscard]] auto domain() const {
        for (auto n = derived_.size(); domain_offset_ < n; ++domain_offset_) {
            if (atoms_.nth(derived_[domain_offset_])->second.state != StateAtom::fact) {
                return false;
            }
        }
        return true;
    }
    //! Check if the given atom is a fact.
    //!
    //! This function does not take into account to which generation an atom belongs.
    //! It can also return true for atoms added to upcoming generations.
    auto is_fact(Symbol sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state == StateAtom::fact;
    }
    //! Check if the base contains the given atom.
    //!
    //! This might includes atoms that have not (yet) been derived.
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end();
    }

    //! Add an atom to the base.
    auto add(Symbol atom, StateAtom state) -> AtomUpdate {
        auto [it, ins] = atoms_.try_emplace(atom, 0, state);
        if (ins) {
            if (state != StateAtom::unknown) {
                derived_.add(atom_index_(it));
            }
            return AtomUpdate::added;
        }
        if (state < it->second.state) {
            // note transitions from external to derived are ignored
            // because there is no additional information for grounding
            auto prev = it.value().state;
            it.value().state = state;
            if (prev == StateAtom::unknown) {
                derived_.add(atom_index_(it));
                return AtomUpdate::added;
            }
            if (state == StateAtom::fact) {
                return AtomUpdate::changed;
            }
        }
        return AtomUpdate::unchanged;
    }

    //! Get the number of derived atoms.
    [[nodiscard]] auto size() const -> size_t { return derived_.size(); }
    //! Get the atom index of the given symbol.
    //!
    //! Note that only derived atoms have indices.
    auto index(Symbol const &sym) const -> size_t {
        if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state != StateAtom::unknown) {
            return derived_.find(atom_index_(it));
        }
        return size();
    }
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> MapAtom::const_iterator { return atoms_.nth(derived_[i]); }
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> MapAtom::iterator { return atoms_.nth(derived_[i]); }

    //! Mark all symbols held by the base.
    void mark(SymbolCollector &gc) {
        clear_context();
        for (auto const &[sym, atom] : atoms_) {
            gc.mark(sym);
        }
    }

  private:
    [[nodiscard]] auto atom_index_(MapAtom::const_iterator it) const -> size_t {
        return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
    }

    MapAtom atoms_;
    Util::index_sequence<size_t> derived_;
    size_t mutable domain_offset_ = 0;
};

//! A unique pointer holding a base.
using UBase = std::unique_ptr<Base>;

//! @}

} // namespace Gringo::Ground
