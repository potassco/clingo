#pragma once

#include <clingo/ground/instantiator.hh>
#include <clingo/ground/term.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_theory
//! @{

class TheoryTerm;
//! A unique pointer to a theory term.
using UTheoryTerm = std::unique_ptr<TheoryTerm>;
//! A vector of theory terms.
using UTheoryTermVec = std::vector<UTheoryTerm>;

//! The TheoryTerm interface.
class TheoryTerm {
  public:
    //! Destructor.
    virtual ~TheoryTerm() = default;
    //! Collect all variables in the term.
    void vars(VariableSet &vars) const { do_vars(vars); }
    //! Create a copy of the term.
    [[nodiscard]] auto copy() const -> UTheoryTerm { return do_copy(); }
    //! Compute a hash for the term.
    [[nodiscard]] auto hash() const -> size_t { return do_hash(); }

    //! Collect all variables in the term.
    [[nodiscard]] auto vars() const -> VariableSet {
        VariableSet set;
        vars(set);
        return set;
    }

    //! Output the term.
    auto output(EvalContext const &ctx, OutputTheory &out) const -> size_t { return do_output(ctx, out); }

    //! Compare two terms.
    friend auto operator==(TheoryTerm const &a, TheoryTerm const &b) -> bool { return a.do_equal_to(b); }
    //! Compare two terms.
    friend auto operator<=>(TheoryTerm const &a, TheoryTerm const &b) -> std::strong_ordering {
        return a.do_compare_to(b);
    }
    //! Print the term.
    friend auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
        term.do_print(out);
        return out;
    }

  private:
    virtual void do_vars(VariableSet &vars) const = 0;
    virtual void do_print(std::ostream &out) const = 0;
    virtual auto do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t = 0;
    [[nodiscard]] virtual auto do_copy() const -> UTheoryTerm = 0;
    [[nodiscard]] virtual auto do_hash() const -> size_t = 0;
    [[nodiscard]] virtual auto do_equal_to(TheoryTerm const &other) const -> bool = 0;
    [[nodiscard]] virtual auto do_compare_to(TheoryTerm const &other) const -> std::strong_ordering = 0;
};

//! A symbolic theory term.
class TheoryTermSymbol : public TheoryTerm {
  public:
    //! Construct a theory symbol.
    TheoryTermSymbol(Symbol sym) : sym_{sym} {}

  private:
    void do_vars(VariableSet &vars) const override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t override;
    [[nodiscard]] auto do_copy() const -> UTheoryTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(TheoryTerm const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(TheoryTerm const &other) const -> std::strong_ordering override;

    Symbol sym_;
};

//! A variable theory term.
class TheoryTermVariable : public TheoryTerm {
  public:
    //! Construct a theory variable.
    TheoryTermVariable(size_t var) : var_{var} {}

  private:
    void do_vars(VariableSet &vars) const override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t override;
    [[nodiscard]] auto do_copy() const -> UTheoryTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(TheoryTerm const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(TheoryTerm const &other) const -> std::strong_ordering override;

    size_t var_;
};

//! A tuple (set or list) theory term.
class TheoryTermTuple : public TheoryTerm {
  public:
    //! Construct a theory tuple/set/list.
    TheoryTermTuple(TheoryTermTupleType type, UTheoryTermVec args) : type_{type}, args_{std::move(args)} {}

  private:
    void do_vars(VariableSet &vars) const override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t override;
    [[nodiscard]] auto do_copy() const -> UTheoryTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(TheoryTerm const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(TheoryTerm const &other) const -> std::strong_ordering override;

    TheoryTermTupleType type_;
    UTheoryTermVec args_;
};

//! A function theory term.
class TheoryTermFunction : public TheoryTerm {
  public:
    //! Construct a theory function.
    TheoryTermFunction(String name, UTheoryTermVec args) : name_{name}, args_{std::move(args)} {}

  private:
    void do_vars(VariableSet &vars) const override;
    void do_print(std::ostream &out) const override;
    auto do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t override;
    [[nodiscard]] auto do_copy() const -> UTheoryTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(TheoryTerm const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(TheoryTerm const &other) const -> std::strong_ordering override;

    String name_;
    UTheoryTermVec args_;
};

//! @}

} // namespace CppClingo::Ground
