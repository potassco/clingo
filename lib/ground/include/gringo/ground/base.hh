#pragma once

#include <gringo/core/symbol.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

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

enum class AtomUpdate {
    added = 0,
    changed = 1,
    unchanged = 2,
};

enum class MatcherType { new_atoms, old_atoms, all_atoms };

class Base {
  public:
    [[nodiscard]] auto contains(Symbol const &sym) const -> bool;
    [[nodiscard]] auto operator[](size_t pos) -> Atom &;
    [[nodiscard]] auto operator[](size_t pos) const -> Atom const &;
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

    auto atoms() const -> std::span<Atom const> { return {atoms_.values_container().data(), all_offset_}; }
    auto has_update() const -> bool { return atoms_.size() > all_offset_; }
    auto begin(MatcherType type) const -> size_t {
        if (type == MatcherType::new_atoms) {
            return old_offset_;
        }
        return 0;
    }
    auto end(MatcherType type) const -> size_t {
        if (type == MatcherType::old_atoms) {
            return old_offset_;
        }
        return all_offset_;
    }
    auto nth(size_t i) const -> Util::ordered_map<Symbol, AtomInfo>::const_iterator { return atoms_.nth(i); }
    auto nth(size_t i) -> Util::ordered_map<Symbol, AtomInfo>::iterator { return atoms_.nth(i); }

  private:
    Util::ordered_map<Symbol, AtomInfo> atoms_;
    size_t mutable domain_offset_ = 0;
    //! Symbols before this offset are considered old.
    size_t mutable old_offset_ = 0;
    //! Symbols before this offset are considered new or old.
    size_t mutable all_offset_ = 0;
    //! The last generation at which the domain has been updated.
    size_t mutable generation_ = 0;
};

} // namespace Gringo::Ground
