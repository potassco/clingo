#pragma once

#include <gringo/core/symbol.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

enum class RenameMode {
    rename_vars,       //!< Succesively rename variables in order of traversal.
    rename_projection, //!< Succesively introduce variables for projections in order of traversal.
    drop_projection,   //!< Drop projections from tuples and functions.
};

using VariableSet = Util::ordered_set<size_t>;
using VariableVec = VariableSet::values_container_type;

class Term;
using UTerm = std::unique_ptr<Term>;
using UTermVec = std::vector<UTerm>;

//! Term interface.
class Term {
  public:
    //! Destructor.
    virtual ~Term() = default;
    //! Match a term with the given symbol.
    //!
    //! Returns true if the term can match the symbol.
    //! Variables in the term are assigned to symbols and stored in the given assignment.
    [[nodiscard]] virtual auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool = 0;
    //! Evaluates a term w.r.t. the given assignment.
    //!
    //! A term might fail to evaluate if a unary or binary operation is not defined for its arguments.
    [[nodiscard]] virtual auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> = 0;
    //! Create a copy of the term renaming/replacing parts of it.
    //!
    //! If a name is given, the name of the outermost function symbol is changed.
    //! Otherwise, variables and projection are replaced according to the given mode.
    [[nodiscard]] virtual auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const
        -> UTerm = 0;
    //! Collect all variables in the term.
    virtual void vars(VariableSet &vars, bool provide = false) const = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    [[nodiscard]] virtual auto equal_to(Term const &other) const -> bool = 0;
    [[nodiscard]] virtual auto compare_to(Term const &other) const -> std::strong_ordering = 0;

    friend auto operator==(Term const &a, Term const &b) -> bool { return a.equal_to(b); }
    friend auto operator<=>(Term const &a, Term const &b) -> std::strong_ordering { return a.compare_to(b); }
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.print(out);
        return out;
    }
};

class TermProjection : public Term {
  public:
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;
};

class TermSymbol : public Term {
  public:
    TermSymbol(Symbol sym) : sym_{sym} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    Symbol sym_;
};

class TermVariable : public Term {
  public:
    TermVariable(size_t var) : var_{var} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    size_t var_;
};

class TermLinear : public Term {
  public:
    TermLinear(Number m, size_t var, Number n) : m_{std::move(m)}, n_{std::move(n)}, var_{var} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    Number m_;
    Number n_;
    size_t var_;
};

//! Available unary operations.
enum class UnaryOperator : int {
    minus = 0,  //!< The unary arithmetic minus operation.
    invert = 1, //!< The bitwise negation operation.
    abs = 2,    //!< The arithmetic absolute operation.
};

class TermUnary : public Term {
  public:
    TermUnary(UnaryOperator op, UTerm rhs) : rhs_{std::move(rhs)}, op_{op} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    UTerm rhs_;
    UnaryOperator op_;
};

//! Available binary operations.
enum class BinaryOperator : int {
    and_,  //!< The AND bit operation.
    div,   //!< The (integer) divide arithmetic operation.
    minus, //!< The minus arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    times, //!< The multiply arithmetic operation.
    or_,   //!< The OR bit operation.
    plus,  //!< The plus arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
    xor_,  //!< The XOR bit operation.
};

class TermBinary : public Term {
  public:
    TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    UTerm lhs_;
    UTerm rhs_;
    BinaryOperator op_;
};

class TermTuple : public Term {
  public:
    TermTuple(UTermVec args) : args_{std::move(args)} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    UTermVec args_;
};

class TermFunction : public Term {
  public:
    TermFunction(String name, UTermVec args) : name_{name}, args_{std::move(args)} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String *name, size_t *vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    String name_;
    UTermVec args_;
};

} // namespace Gringo::Ground
