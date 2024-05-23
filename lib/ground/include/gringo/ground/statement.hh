#pragma once

#include <gringo/ground/literal.hh>

#include <gringo/ground/instantiator.hh>

namespace Gringo::Ground {

class Stm : public InstanceCallback {
  public:
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto body() const -> ULitVec const & = 0;
    [[nodiscard]] virtual auto important() const -> VariableSet = 0;
    friend auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

using UStm = std::unique_ptr<Stm>;
using UStmVec = std::vector<UStm>;

//! Helper class to prepare statements for grounding.
class Linearizer {
  public:
    //! Indicate that a new domain is being prepared.
    void start(Queue &queue, bool domain);
    //! Prepare a statement for grounding.
    void prepare(Stm &stm);

  private:
    //! Build the dependency graph among literals and variables.
    void build_(ULitVec const &lits);
    //! Create matchers for literals ordering them heuristically.
    auto order_(InstanceCallback &cb, std::vector<MatcherType> const &todo, VariableSet const &important,
                ULitVec const &lits) -> std::pair<Instantiator, std::optional<size_t>>;

    Queue *iqueue_ = nullptr;
    std::vector<size_t> rec_;
    std::vector<std::vector<MatcherType>> todos_;
    std::vector<std::pair<size_t, size_t>> queue_;
    //! A map from literal indices to provided variables.
    std::vector<std::tuple<size_t, std::vector<size_t>, std::vector<size_t>>> lit_map_;
    //! A map from variables to provided literals.
    std::vector<std::vector<size_t>> var_map_;
    bool domain_ = false;
};

class StmRule : public Stm {
  public:
    StmRule(Ground::UTerm head, Base *base, std::vector<size_t> indices, Ground::ULitVec body)
        : head_{std::move(head)}, base_{base}, indices_{std::move(indices)}, body_{std::move(body)} {
        if (head_) {
            body_.emplace_back(std::make_unique<LitFactCheck>(*base_, *head_));
        }
    }
    // Stm interface
    void print(std::ostream &out) const override;

    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // InstanceCallback interface
    void print_head(std::ostream &out) const override;
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override { return 0; }

  private:
    // TODO: how to handle head
    UTerm head_;
    Base *base_;
    //! A list of indices.
    //!
    //! Recursive body literals that unify with the rule head have matching indices.
    //! This allows for updating the indices of these literals while grounding.
    std::vector<size_t> indices_;
    Ground::ULitVec body_;
};

} // namespace Gringo::Ground
