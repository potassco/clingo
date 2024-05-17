#pragma once

#include <gringo/core/symbol.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

// NOLINTNEXTLINE(performance-enum-size)
enum class AtomState : uint64_t {
    // Indicates that the atom is derived by a fact.
    fact = 0,
    // At the time rule (1) is grounded, atom x is not yet defined.
    // Once (2) has been grounded, there is a definition for it.
    //
    //   a :- not x. (1)
    //   x :- a.     (2)
    //
    // The flag indicates atoms that have neither been derived by facts, rules, or externals.
    unknown = 1,
    // Indicates that the atom is derived by some external but not a rule or fact.
    external = 2,
    // Indicates that the atom is derived by some rule but not a fact.
    derived = 3,
};

struct AtomInfo {
    // A unique id among all atoms.
    mutable uint64_t id : 62;
    mutable AtomState state : 2;
};

using Atom = std::pair<Symbol, AtomInfo>;

enum class AtomUpdate : uint8_t {
    added = 0,
    changed = 1,
    unchanged = 2,
};

enum class MatcherType : uint8_t { new_atoms, old_atoms, all_atoms };

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
class Base {
  public:
    //! Get the number of atoms in the base.
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

    //! Update the generation counts.
    //!
    //! Calling with generation zero resets the counts. Calling with a
    //! generation greater than the current generation adds all atoms from the
    //! current all generation to the old generation.
    //!
    //! Calling the function multiple times for the same generation has no
    //! effect.
    void update(size_t generation) const {
        // initialize the domain
        // (all atoms are marked as new)
        if (generation == 0) {
            generation_ = 0;
            old_offset_ = 0;
            all_offset_ = atoms_.size();
        }
        // the generation has been incremented by one
        // (freshly added atoms are marked new)
        else if (generation_ + 1 == generation) {
            generation_ = generation;
            old_offset_ = all_offset_;
            all_offset_ = atoms_.size();
            // the generation has been incremented by more than one
            // (all atoms are marked old)
        } else if (generation_ + 1 < generation) {
            generation_ = generation;
            old_offset_ = atoms_.size();
            all_offset_ = atoms_.size();
        }
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
    //! Get the n-th atom in the base.
    auto nth(size_t i) const -> Util::ordered_map<Symbol, AtomInfo>::const_iterator { return atoms_.nth(i); }
    //! Get the n-th atom in the base.
    auto nth(size_t i) -> Util::ordered_map<Symbol, AtomInfo>::iterator { return atoms_.nth(i); }

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
    std::unique_ptr<BaseContext> context_;
    Util::ordered_map<Symbol, AtomInfo> atoms_;
    size_t mutable domain_offset_ = 0;
    //! Symbols before this offset are considered old.
    size_t mutable old_offset_ = 0;
    //! Symbols before this offset are considered new or old.
    size_t mutable all_offset_ = 0;
    //! The last generation at which the domain has been updated.
    size_t mutable generation_ = 0;
};

using UBase = std::unique_ptr<Base>;

} // namespace Gringo::Ground
