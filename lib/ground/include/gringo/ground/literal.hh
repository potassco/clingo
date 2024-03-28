#pragma once

#include <gringo/ground/base.hh>
#include <gringo/ground/instantiator.hh>
#include <gringo/ground/literal.hh>
#include <gringo/ground/term.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

enum class Sign {
    none,
    once,
    twice,
};
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

enum class VarSelectMode {
    depend = 1,
    provide = 2,
    all = 3,
};

enum class MatcherType { new_atoms, old_atoms, all_atoms };

class Lit {
  public:
    virtual ~Lit() = default;

    //! Get the variables in the predicate.
    virtual void vars(VariableSet &vars, VarSelectMode mode) const = 0;
    //! Returns true if the literal is domain.
    //!
    //! A literal is considered domain if
    //! - it's base does not contain a non-domain value, and
    //!   - it occurs in a domain component, or
    //!   - it is stratified
    [[nodiscard]] virtual auto domain(bool domain) const -> bool = 0;
    //! Returns true if the literal is recursive.
    //!
    //! Recursive literals give rise to components that need more than one grounding pass.
    //! For example, incomplete positive symbolic literals are considered recursive.
    //! However, incomplete negative literals are not considered recursive.
    [[nodiscard]] virtual auto recursive() const -> bool { return false; }
    [[nodiscard]] virtual auto matcher(MatcherType type) -> UMatcher = 0;

    virtual void print(std::ostream &out) const = 0;

    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    [[nodiscard]] virtual auto equal_to(Lit const &other) const -> bool = 0;
    [[nodiscard]] virtual auto compare_to(Lit const &other) const -> std::weak_ordering = 0;

    friend auto operator==(Lit const &a, Lit const &b) -> bool { return a.equal_to(b); }
    friend auto operator<=>(Lit const &a, Lit const &b) -> std::weak_ordering { return a.compare_to(b); }
    friend auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream & {
        lit.print(out);
        return out;
    }
};
using ULit = std::unique_ptr<Lit>;
using ULitVec = std::vector<ULit>;

class LitSymbolic : public Lit {
  public:
    LitSymbolic(Base &base, Sign sign, UTerm atom, size_t index)
        : base_{&base}, atom_{std::move(atom)}, sign_{sign}, index_{index} {}

    void vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto domain(bool domain) const -> bool override;
    [[nodiscard]] auto recursive() const -> bool override;
    [[nodiscard]] auto matcher(MatcherType type) -> UMatcher override;

    void print(std::ostream &out) const override;

    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Lit const &other) const -> std::weak_ordering override;

  private:
    Base *base_;
    UTerm atom_;
    Sign sign_;
    //! The index of the literal.
    //!
    //! Note that only recursive literals have indices.
    size_t index_;
};

} // namespace Gringo::Ground
