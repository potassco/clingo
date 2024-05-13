#pragma once

#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

struct BaseCondLit {
    VariableVec local;
    VariableVec global;
};

class StmCondLitEmpty : public Stm {
  public:
    StmCondLitEmpty(BaseCondLit *base, ULitVec body, size_t prio) : base_{base}, body_{std::move(body)}, prio_{prio} {}
    // statement interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // solution callback interface
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override;

  private:
    BaseCondLit *base_;
    ULitVec body_;
    size_t prio_;
};

class StmCondLitPremise : public Stm {
  public:
    StmCondLitPremise(BaseCondLit *base, ULitVec body, size_t prio)
        : base_{base}, body_{std::move(body)}, prio_{prio} {}
    // statement interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // solution callback interface
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override;

  private:
    BaseCondLit *base_;
    ULitVec body_;
    size_t prio_;
};

} // namespace Gringo::Ground
