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

using UStm = std::unique_ptr<Stm>;
using UStmVec = std::vector<UStm>;

class StmRule : public Stm {
  public:
    StmRule(Ground::UTerm atom, std::vector<size_t> provides, Ground::ULitVec body)
        : atom_{std::move(atom)}, provides_{std::move(provides)}, body_{std::move(body)} {}
    void print(std::ostream &out) const override;

  private:
    // TODO: how to handle head
    Ground::UTerm atom_;
    std::vector<size_t> provides_;
    Ground::ULitVec body_;
};

} // namespace Gringo::Ground
