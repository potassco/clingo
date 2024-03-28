#pragma once

#include <gringo/ground/literal.hh>

#include <gringo/ground/instantiator.hh>

namespace Gringo::Ground {

class Stm {
  public:
    virtual ~Stm() = default;
    virtual void print(std::ostream &out) const = 0;
    virtual void linearize(InstantiatorVec &insts, bool domain) = 0;
    friend auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

using UStm = std::unique_ptr<Stm>;
using UStmVec = std::vector<UStm>;

class StmRule : public Stm, private InstanceCallback {
  public:
    StmRule(Ground::UTerm head, std::vector<size_t> indices, Ground::ULitVec body)
        : head_{std::move(head)}, indices_{std::move(indices)}, body_{std::move(body)} {}
    void print(std::ostream &out) const override;
    void linearize(InstantiatorVec &insts, bool domain) override;
    void init() override;
    void report(Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override { return 0; }

  private:
    // TODO: how to handle head
    Ground::UTerm head_;
    //! A list of indices.
    //!
    //! Recursize body literals that unify with the rule head have matching indices.
    //! This allows for updating the indices of these literals while grounding.
    std::vector<size_t> indices_;
    Ground::ULitVec body_;
};

} // namespace Gringo::Ground
