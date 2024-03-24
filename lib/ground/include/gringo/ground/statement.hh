#pragma once

#include <gringo/ground/literal.hh>

namespace Gringo::Ground {

class Stm {
  public:
    virtual ~Stm() = default;
    virtual void print(std::ostream &out) const = 0;
    friend auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

class StmRule : public Stm {
  public:
    StmRule(Ground::UTerm atom, Ground::ULitVec body) : atom_{std::move(atom)}, body_{std::move(body)} {}
    void print(std::ostream &out) const override;

  private:
    // TODO: how to handle head
    Ground::UTerm atom_;
    Ground::ULitVec body_;
};

} // namespace Gringo::Ground
