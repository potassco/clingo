#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/term.hh>

namespace Gringo::Ground {

enum class Sign {
    none,
    once,
    twice,
};
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

class Lit {
  public:
    virtual ~Lit() = default;
    virtual void print(std::ostream &out) const = 0;
    friend auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream & {
        lit.print(out);
        return out;
    }
};
using ULit = std::unique_ptr<Lit>;
using ULitVec = std::vector<ULit>;

class LitSymbolic : public Lit {
  public:
    LitSymbolic(Sign sign, UTerm atom) : sign_{sign}, atom_{std::move(atom)} {}
    void print(std::ostream &out) const override;

  private:
    Sign sign_;
    UTerm atom_;
};

} // namespace Gringo::Ground
