#pragma once

#include <gringo/core/symbol.hh>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

class Term {
  public:
    virtual ~Term() = default;
    [[nodiscard]] virtual auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool = 0;
    [[nodiscard]] virtual auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> = 0;
};
using UTerm = std::unique_ptr<Term>;
using UTermVec = std::vector<UTerm>;

class TermSymbol : public Term {
  public:
    TermSymbol(Symbol sym) : sym_{sym} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;

  private:
    Symbol sym_;
};

class TermVariable : public Term {
  public:
    TermVariable(size_t var) : var_{var} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;

  private:
    size_t var_;
};

class TermLinear : public Term {
  public:
    TermLinear(Number m, size_t var, Number n) : m_{std::move(m)}, n_{std::move(n)}, var_{var} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;

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

  private:
    UTermVec args_;
};

class TermFunction : public Term {
  public:
    TermFunction(String name, UTermVec args) : name_{name}, args_{std::move(args)} {}
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;

  private:
    String name_;
    UTermVec args_;
};

} // namespace Gringo::Ground
