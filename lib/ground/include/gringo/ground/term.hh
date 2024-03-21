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

class TermUnary : public Term {
  public:
    // TODO: operator
    UTerm rhs;
};

class TermBinary : public Term {
  public:
    // TODO: operator
    UTerm lhs;
    UTerm rhs;
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
