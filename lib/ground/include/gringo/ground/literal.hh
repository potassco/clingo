#pragma once

#include <gringo/ground/term.hh>

namespace Gringo::Ground {

class Lit {
  public:
    virtual ~Lit() = default;
};

class LitSymbolic : public Lit {
  public:
    Symbol sym;
};

} // namespace Gringo::Ground
