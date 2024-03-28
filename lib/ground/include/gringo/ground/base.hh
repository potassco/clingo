#pragma once

#include <gringo/core/symbol.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

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
    mutable uint64_t id;
    // Indicates at which iteration an atom has been grounded.
    // It would be ideal if it were possible to get rid of this field.
    mutable uint64_t gen : 62;
    mutable AtomState state : 2;
};

using Atom = std::pair<Symbol, AtomInfo>;

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
            if (atoms_.nth(n)->second.state != AtomState::fact) {
                return false;
            }
        }
        return true;
    }

  private:
    Util::ordered_map<Symbol, AtomInfo> atoms_;
    size_t gen_ = 0;
    size_t mutable domain_offset_ = 0;
};

} // namespace Gringo::Ground
