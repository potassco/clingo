#pragma once

#include <gringo/core/symbol.hh>

namespace Gringo::Ground {

class Term {
  public:
    virtual ~Term() = default;
};
using UTerm = std::unique_ptr<Term>;
using UTermVec = std::vector<UTerm>;

class TermSymbol : public Term {
  public:
    Symbol sym;
};

class TermVariable : public Term {
  public:
    size_t var;
};

class TermLinear : public Term {
  public:
    Number m;
    Number n;
    size_t var;
};

//! Available unary operations.
enum class UnaryOperator : int {
    minus = 0,  //!< The unary arithmetic minus operation.
    invert = 1, //!< The bitwise negation operation.
    abs = 2,    //!< The arithmetic absolute operation.
};

class TermUnary : public Term {
  public:
    UTerm rhs;
    UnaryOperator op;
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
    UTerm lhs;
    UTerm rhs;
    BinaryOperator op;
};

class TermTuple : public Term {
  public:
    UTermVec args;
};

class TermFunction : public Term {
  public:
    String name;
    UTermVec args;
};

} // namespace Gringo::Ground
