#pragma once

#include <gringo/core/core.hh>

#include <gringo/ground/base.hh>
#include <gringo/ground/instantiator.hh>
#include <gringo/ground/term.hh>

#include <gringo/util/ordered_map.hh>

namespace Gringo::Ground {

//! @addtogroup ground_literal
//! @{

//! Available variable selection modes.
enum class VarSelectMode : uint8_t {
    depend = 1,  //!< Get variables a literal depends on.
    provide = 2, //!< Get variables provided by a literal.
    all = 3,     //!< Get all variables in a literal.
};

class Lit;
//! A unique pointer holding a literal.
using ULit = std::unique_ptr<Lit>;
//! A vector of literals.
using ULitVec = std::vector<ULit>;

//! The base class for groundable literals.
class Lit {
  public:
    //! Destroy the literal.
    virtual ~Lit() = default;

    //! Get the variables in the predicate.
    void vars(VariableSet &vars, VarSelectMode mode) const { do_vars(vars, mode); }

    //! Returns true if matching literals are always fact.
    //!
    //! For example, symbolic literals are domain if there are no negative
    //! cycles through them and their (current) base is domain.
    [[nodiscard]] auto domain() const -> bool { return do_domain(); }
    //! Returns true if the literal can be grounded in a single pass.
    //!
    //! Such literals are always have a complete state ready to be output.
    [[nodiscard]] auto single_pass() const -> bool { return do_single_pass(); }
    //! Returns true if the base of the literal is complete at the time of grounding.
    [[nodiscard]] auto matcher(MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
        return do_matcher(type, bound);
    }
    //! Compute a score used to order rule bodies.
    [[nodiscard]] auto score(std::vector<bool> const &bound) const -> double { return do_score(bound); }

    //! Output the literal.
    //!
    //! If the literal is a fact, it should not be output and the function
    //! should return false.
    [[nodiscard]] auto output(InstantiationContext &ctx, OutputLit &out) const -> bool { return do_output(ctx, out); }

    //! Copy the literal.
    [[nodiscard]] auto copy() const -> ULit { return do_copy(); }

    //! Compute a hash for the literal.
    [[nodiscard]] auto hash() const -> size_t { return do_hash(); }

    //! Compare two literals.
    friend auto operator==(Lit const &a, Lit const &b) -> bool { return a.do_equal_to(b); }
    //! Compare two literals.
    friend auto operator<=>(Lit const &a, Lit const &b) -> std::weak_ordering { return a.do_compare_to(b); }
    //! Print the literal.
    friend auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream & {
        lit.do_print(out);
        return out;
    }

  private:
    virtual void do_vars(VariableSet &vars, VarSelectMode mode) const = 0;
    [[nodiscard]] virtual auto do_domain() const -> bool = 0;
    [[nodiscard]] virtual auto do_single_pass() const -> bool { return false; }
    [[nodiscard]] virtual auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> = 0;
    [[nodiscard]] virtual auto do_score(std::vector<bool> const &bound) const -> double = 0;
    virtual void do_print(std::ostream &out) const = 0;
    virtual auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool = 0;
    [[nodiscard]] virtual auto do_copy() const -> ULit = 0;
    [[nodiscard]] virtual auto do_hash() const -> size_t = 0;
    [[nodiscard]] virtual auto do_equal_to(Lit const &other) const -> bool = 0;
    [[nodiscard]] virtual auto do_compare_to(Lit const &other) const -> std::weak_ordering = 0;
};

//! A literal representing a Boolean.
class LitBool : public Lit {
  public:
    //! Construct the literal.
    LitBool(bool value) : value_{value} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    bool value_;
};

//! A literal representing a comparison.
class LitComparison : public Lit {
  public:
    //! Construct the literal.
    LitComparison(UTerm lhs, Relation cmp, UTerm rhs) : lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, cmp_{cmp} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    UTerm lhs_;
    UTerm rhs_;
    Relation cmp_;
};

//! A literal representing an interval assignment.
class LitInterval : public Lit {
  public:
    //! Construct the literal.
    LitInterval(UTerm lhs, UTerm lower, UTerm upper)
        : lhs_{std::move(lhs)}, lower_{std::move(lower)}, upper_{std::move(upper)} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    UTerm lhs_;
    UTerm lower_;
    UTerm upper_;
};

//! Marker for stratified literals.
constexpr auto stratified_index = std::numeric_limits<size_t>::max();

//! Simple literal that discards whenever it matches to a fact.
//!
//! It is meant to prune rules whose heads have already been derived as facts.
class LitFactCheck : public Lit {
  public:
    //! Construct the literal.
    LitFactCheck(Base &base, Term const &atom, Symbol &target) : base_{&base}, atom_{&atom}, target_{&target} {}

  private:
    //! Construct the literal.
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    Base *base_;
    Term const *atom_;
    Symbol *target_;
};

//! A symbolic literal.
class LitSymbolic : public Lit {
  public:
    //! Construct the literal.
    LitSymbolic(Base &base, Sign sign, UTerm atom, size_t index, bool domain)
        : base_{&base}, atom_{std::move(atom)}, index_{index}, sign_{sign}, domain_{domain} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;

    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    Base *base_;
    UTerm atom_;
    //! The index of the literal.
    //!
    //! Note that only recursive literals have indices.
    size_t index_;
    size_t offset_ = 0;
    Symbol symbol_;
    Sign sign_;
    //! Whether the literal occurs in a domain component.
    bool domain_;
};

//! A literal similar to a symbolic literal.
//!
//! This literal takes care of projection during matching.
class LitProject : public Lit {
  public:
    //! The state capturing the base of a projection.
    class State {
      public:
        //! Initialize the state.
        State(String name, size_t vars, Base &base, UTerm p_head, UTerm p_body)
            : name_{name}, base_{&base}, p_head_{std::move(p_head)}, p_body_{std::move(p_body)} {
            ass_.resize(vars);
        }
        //! Get the base of the unprojected literal.
        [[nodiscard]] auto base() const -> Base & { return *base_; }
        //! Get the base of the projected literal.
        [[nodiscard]] auto p_base() -> Base & { return p_base_; }
        //! Get the auxiliary name of projected literal.
        [[nodiscard]] auto name() const -> String const & { return *name_; }
        //! Initialize the projected base.
        //!
        //! This populates the projected base.
        void init(SymbolStore &store, size_t gen);

      private:
        SharedString name_;
        Base *base_;
        Base p_base_;
        UTerm p_head_;
        UTerm p_body_;
        Assignment ass_;
        size_t imported_ = 0;
    };
    //! Construct the literal.
    LitProject(State &state, Sign sign, UTerm atom, UTerm p_atom, size_t index, bool domain)
        : state_{&state}, atom_{std::move(atom)}, p_atom_{std::move(p_atom)}, index_{index}, sign_{sign},
          domain_{domain} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override;

    [[nodiscard]] auto do_copy() const -> ULit override;

    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    State *state_;
    UTerm atom_;
    UTerm p_atom_;
    size_t index_;
    size_t offset_ = 0;
    Symbol symbol_;
    Sign sign_;
    bool domain_;
};

//! An auxiliary literal binding variables to the given symbols upon first match.
class LitTuple : public Lit {
  public:
    //! Construct the literal.
    LitTuple(VariableVec vars, SymbolVec &syms) : vars_{std::move(vars)}, syms_{&syms} {
        assert(vars_.size() == syms_->size());
    }

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto do_domain() const -> bool override;
    [[nodiscard]] auto do_single_pass() const -> bool override;
    [[nodiscard]] auto
    do_matcher(MatcherType type, std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto do_score([[maybe_unused]] std::vector<bool> const &bound) const -> double override;
    void do_print(std::ostream &out) const override;
    auto do_output([[maybe_unused]] InstantiationContext &ctx, OutputLit &out) const -> bool override;
    [[nodiscard]] auto do_copy() const -> ULit override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override;

    VariableVec vars_;
    SymbolVec *syms_;
};

//! @}

} // namespace Gringo::Ground
