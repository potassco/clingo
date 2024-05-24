#pragma once

#include <gringo/core/symbol.hh>

#include <gringo/util/index_sequence.hh>
#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

// NOLINTNEXTLINE(performance-enum-size)
enum class AtomState : uint64_t {
    // Indicates that the atom is derived by a fact.
    fact = 0,
    // Indicates that the atom is derived by some rule but not a fact.
    derived = 1,
    // Indicates that the atom is derived by some external but not a rule or fact.
    external = 2,
    // At the time rule (1) is grounded, atom x is not yet defined.
    // Once (2) has been grounded, there is a definition for it.
    //
    //   a :- not x. (1)
    //   x :- a.     (2)
    //
    // The flag indicates atoms that have neither been derived by facts, rules, or externals.
    unknown = 3,
};

struct AtomInfo {
    // A unique id among all atoms.
    mutable uint64_t id : 63;
    mutable AtomState state : 2;
};

using Atom = std::pair<Symbol, AtomInfo>;

enum class AtomUpdate : uint8_t {
    added = 0,
    changed = 1,
    unchanged = 2,
};

enum class MatcherType : uint8_t { new_atoms, old_atoms, all_atoms };

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

// TODO:
// - used to store indices for the domain
// - implemented in an any-like fashion because we do not need details here
// - for base cleanup it would be easy to add a method
class BaseContext {
  public:
    virtual ~BaseContext() = default;
};

//! An atom base used to store derivable atoms and associated state.
//!
//! The base tracks the generation of atoms for semi-naive evaluation,
//! and the state of atoms.
//!
//! An atom base can also stores unknown atoms. For such atoms it is not yet
//! know whether there will be a rule deriving them. The only purpose is to
//! store them here is to associated them with a unique id.
class Base {
  public:
    //! Get the number of atoms in the base (including unknown ones).
    [[nodiscard]] auto size() const { return atoms_.size(); }

    //! Check if the base is domain.
    //!
    //! A base is domain if it contains facts only.
    [[nodiscard]] auto domain() const {
        for (auto n = atoms_.size(); domain_offset_ < n; ++domain_offset_) {
            if (atoms_.nth(domain_offset_)->second.state != AtomState::fact) {
                return false;
            }
        }
        return true;
    }

    //! Add an atom to the base.
    auto add(Symbol atom, AtomState state) -> AtomUpdate {
        auto [it, ins] = atoms_.try_emplace(atom, 0, state);
        if (ins) {
            if (state != AtomState::unknown) {
                derived_.add(atom_index_(it));
            }
            return AtomUpdate::added;
        }
        if (state < it->second.state) {
            // note transitions from external to derived are ignored
            // because there is no additional information for grounding
            auto prev = it.value().state;
            it.value().state = state;
            if (prev == AtomState::unknown) {
                derived_.add(atom_index_(it));
                return AtomUpdate::added;
            }
            if (state == AtomState::fact) {
                return AtomUpdate::changed;
            }
        }
        return AtomUpdate::unchanged;
    }

    //! Update the generation counts.
    void update(size_t generation) const { counts_.update(generation, derived_.size()); }
    //! Get the index of the first atom in the given generation.
    auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }
    //! Get the index plus one of the last atom in the given generation.
    auto end(MatcherType type) const -> size_t { return counts_.end(type); }
    //! Check if the base contains the given atom.
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool {
        auto it = atoms_.find(sym);
        return it != atoms_.end() && it->second.state != AtomState::unknown;
    }
    //! Check if the base contains the given atom with in the given generation.
    [[nodiscard]] auto contains(Symbol const &sym, MatcherType type) const -> bool {
        if (auto it = atoms_.find(sym); it != atoms_.end() && it->second.state != AtomState::unknown) {
            auto index = derived_.find(atom_index_(it));
            return counts_.contains(index, type);
        }
        return false;
    }
    //! Get the i-th atom in the base.
    auto nth(size_t i) const -> Util::ordered_map<Symbol, AtomInfo>::const_iterator { return atoms_.nth(derived_[i]); }
    //! Get the i-th atom in the base.
    auto nth(size_t i) -> Util::ordered_map<Symbol, AtomInfo>::iterator { return atoms_.nth(derived_[i]); }

    //! Check if the base contains at least one atom from the all generation.
    auto has_update() const -> bool { return derived_.size() > counts_.end(MatcherType::all_atoms); }
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

  private:
    [[nodiscard]] auto atom_index_(Util::ordered_map<Symbol, AtomInfo>::const_iterator it) const -> size_t {
        return static_cast<size_t>(std::distance(atoms_.cbegin(), it));
    }

    std::unique_ptr<BaseContext> context_;
    Util::ordered_map<Symbol, AtomInfo> atoms_;
    Util::index_sequence<size_t> derived_;
    size_t mutable domain_offset_ = 0;
    GenerationCounts mutable counts_;
};

using UBase = std::unique_ptr<Base>;

} // namespace Gringo::Ground
