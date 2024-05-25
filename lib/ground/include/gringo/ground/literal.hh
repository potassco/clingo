#pragma once

#include <gringo/core/core.hh>

#include <gringo/ground/base.hh>
#include <gringo/ground/instantiator.hh>
#include <gringo/ground/term.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

enum class VarSelectMode : uint8_t {
    depend = 1,
    provide = 2,
    all = 3,
};

class Lit;
using ULit = std::unique_ptr<Lit>;
using ULitVec = std::vector<ULit>;

class Lit {
  public:
    virtual ~Lit() = default;

    //! Get the variables in the predicate.
    virtual void vars(VariableSet &vars, VarSelectMode mode) const = 0;
    //! Check that all elements in the base of the literal are domain.
    //!
    //! Does not return true for incomplete negative literals.
    [[nodiscard]] virtual auto domain() const -> bool = 0;
    //! Returns true if the literal is recursive.
    //!
    //! Recursive literals give rise to components that need more than one grounding pass.
    //! For example, incomplete positive symbolic literals are considered recursive.
    //! However, incomplete negative literals are not considered recursive.
    [[nodiscard]] virtual auto recursive() const -> bool { return false; }
    //! Returns true if the base of the literal is complete at the time of grounding.
    [[nodiscard]] virtual auto
    matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> = 0;
    [[nodiscard]] virtual auto score(std::vector<bool> const &bound) const -> double = 0;

    virtual void print(std::ostream &out) const = 0;
    // Note: I did not make up my mind how to handle the text output yet
    // It might get it's own representation or a way to be output directly to a stream.
    virtual auto output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool = 0;

    [[nodiscard]] virtual auto copy() const -> ULit = 0;

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

class LitBool : public Lit {
  public:
    LitBool(bool value) : value_{value} {}

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
    bool value_;
};

class LitComparison : public Lit {
  public:
    LitComparison(UTerm lhs, Relation cmp, UTerm rhs) : lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, cmp_{cmp} {}

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
    UTerm lhs_;
    UTerm rhs_;
    Relation cmp_;
};

class LitInterval : public Lit {
  public:
    LitInterval(UTerm lhs, UTerm lower, UTerm upper)
        : lhs_{std::move(lhs)}, lower_{std::move(lower)}, upper_{std::move(upper)} {}

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
    UTerm lhs_;
    UTerm lower_;
    UTerm upper_;
};

constexpr auto stratified_index = std::numeric_limits<size_t>::max();

//! Simple literal that discards whenever it matches to a fact.
//!
//! It is meant to prune rules whose heads have already been derived as facts.
class LitFactCheck : public Lit {
  public:
    LitFactCheck(Base &base, Term const &atom) : base_{&base}, atom_{&atom} {}

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
    Base *base_;
    Term const *atom_;
};

class LitSymbolic : public Lit {
  public:
    LitSymbolic(Base &base, Sign sign, UTerm atom, size_t index)
        : base_{&base}, atom_{std::move(atom)}, sign_{sign}, index_{index} {}

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
    Base *base_;
    UTerm atom_;
    Sign sign_;
    //! The index of the literal.
    //!
    //! Note that only recursive literals have indices.
    size_t index_;
};

//! A literal similar to a symbolic literal.
//!
//! This literal takes care of projection during matching.
class LitProject : public Lit {
  public:
    class State {
      public:
        State(String name, size_t vars, Base &base, UTerm p_head, UTerm p_body)
            : name_{name}, base_{&base}, p_head_{std::move(p_head)}, p_body_{std::move(p_body)} {
            ass_.resize(vars);
        }
        [[nodiscard]] auto base() const -> Base & { return *base_; }
        [[nodiscard]] auto p_base() -> Base & { return p_base_; }
        [[nodiscard]] auto name() const -> String const & { return name_; }
        void init(SymbolStore &store, size_t gen);

      private:
        String name_;
        Base *base_;
        Base p_base_;
        UTerm p_head_;
        UTerm p_body_;
        Assignment ass_;
        size_t imported_ = 0;
    };
    LitProject(State &state, Sign sign, UTerm atom, UTerm p_atom, size_t index)
        : state_{&state}, atom_{std::move(atom)}, p_atom_{std::move(p_atom)}, index_{index}, sign_{sign} {}

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
    State *state_;
    UTerm atom_;
    UTerm p_atom_;
    size_t index_;
    Sign sign_;
};

} // namespace Gringo::Ground
