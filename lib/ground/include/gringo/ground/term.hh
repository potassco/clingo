#pragma once

#include <gringo/core/symbol.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/util/enum.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

enum class RenameMode : uint8_t {
    rename_vars,       //!< Succesively rename variables in order of traversal.
    rename_projection, //!< Succesively introduce variables for projections in order of traversal.
    drop_projection,   //!< Drop projections from tuples and functions.
};

using VariableSet = Util::ordered_set<size_t>;
using VariableVec = VariableSet::values_container_type;

class Term;
using UTerm = std::unique_ptr<Term>;
using UTermVec = std::vector<UTerm>;

//! TODO: this interface with the default argument and function hiding is messy.
//! Using virtual function only for the implementation would fix this nicely.

//! Term interface.
class Term {
  public:
    using Key = Symbol;
    //! Destructor.
    virtual ~Term() = default;
    //! Compute an estimate how often the term can match size symbols given the bound variables.
    [[nodiscard]] virtual auto score(double size, std::vector<bool> const &bound) const -> double = 0;
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
    [[nodiscard]] virtual auto rename(SymbolStore &store, RenameMode mode, String const *name,
                                      size_t *vars) const -> UTerm = 0;
    //! Create a copy of the term renaming variables in order of occurrence.
    [[nodiscard]] virtual auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm = 0;
    //! Collect all variables in the term.
    virtual void vars(VariableSet &vars, bool provide = false) const = 0;
    //! Output the term.
    virtual void print(std::ostream &out) const = 0;
    //! Create a copy of the term.
    [[nodiscard]] virtual auto copy() const -> UTerm = 0;
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    [[nodiscard]] virtual auto equal_to(Term const &other) const -> bool = 0;
    [[nodiscard]] virtual auto compare_to(Term const &other) const -> std::strong_ordering = 0;

    //! Compute a siganture of the term.
    //!
    //! This renames variables in the term in ascending order and then return a pair of the term and the bound
    //! variables.
    [[nodiscard]] auto signature(VariableSet const &bound,
                                 VariableSet const &bind) const -> std::pair<UTerm, VariableVec> {
        auto names = Util::unordered_map<size_t, size_t>{};
        names.reserve(bind.size() + bound.size());
        auto sig_term = rename(names);
        auto sig_lookup = std::vector<size_t>{};
        sig_lookup.reserve(bound.size());
        for (auto const &var : bound) {
            sig_lookup.emplace_back(names[var]);
        }
        return {std::move(sig_term), std::move(sig_lookup)};
    }
    //! Collect all variables in the term.
    [[nodiscard]] auto vars() const -> VariableSet {
        VariableSet set;
        vars(set);
        return set;
    }

    friend auto operator==(Term const &a, Term const &b) -> bool { return a.equal_to(b); }
    friend auto operator<=>(Term const &a, Term const &b) -> std::strong_ordering { return a.compare_to(b); }
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.print(out);
        return out;
    }
};

class TermProjection : public Term {
  public:
    using Term::vars;

    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;
};

class TermSymbol : public Term {
  public:
    using Term::vars;

    TermSymbol(Symbol sym) : sym_{sym} {}
    [[nodiscard]] auto symbol() const -> Symbol { return sym_; }
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    Symbol sym_;
};

class TermVariable : public Term {
  public:
    using Term::vars;

    TermVariable(size_t var) : var_{var} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    size_t var_;
};

class TermLinear : public Term {
  public:
    using Term::vars;

    TermLinear(Number m, size_t var, Number n) : m_{std::move(m)}, n_{std::move(n)}, var_{var} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    Number m_;
    Number n_;
    size_t var_;
};

//! Available unary operations.
enum class UnaryOperator : uint8_t {
    minus = 0,  //!< The unary arithmetic minus operation.
    invert = 1, //!< The bitwise negation operation.
    abs = 2,    //!< The arithmetic absolute operation.
};

class TermUnary : public Term {
  public:
    using Term::vars;

    TermUnary(UnaryOperator op, UTerm rhs) : rhs_{std::move(rhs)}, op_{op} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    UTerm rhs_;
    UnaryOperator op_;
};

//! Available binary operations.
enum class BinaryOperator : uint8_t {
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
    using Term::vars;

    TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
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
    using Term::vars;

    TermTuple(UTermVec args) : args_{std::move(args)} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    UTermVec args_;
};

class TermFunction : public Term {
  public:
    using Term::vars;

    TermFunction(String name, UTermVec args) : name_{name}, args_{std::move(args)} {}
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name,
                              size_t *vars) const -> UTerm override;
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void vars(VariableSet &vars, bool provide) const override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto copy() const -> UTerm override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Term const &other) const -> std::strong_ordering override;

  private:
    String name_;
    UTermVec args_;
};

} // namespace Gringo::Ground
