#pragma once

#include <gringo/ground/literal.hh>

#include <gringo/ground/instantiator.hh>

namespace Gringo::Ground {

//! @addtogroup ground_stm
//! @{

//! Base class for groundable statements.
class Stm : public InstanceCallback {
  public:
    //! Get the body of the statement.
    [[nodiscard]] auto body() const -> ULitVec const & { return do_body(); }
    //! Get the important variables in the statement.
    //!
    //! This comprises all variables that have to be bound to provide relevant
    //! instances. Relevant important variables from the body are gathered
    //! separately. For example, normal rules should provide the variables in
    //! the rule head, and integrity constraint can return an empty set.
    [[nodiscard]] auto important() const -> VariableSet { return do_important(); }

    //! Print the statement.
    friend auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
        stm.do_print(out);
        return out;
    }

  private:
    virtual void do_print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_body() const -> ULitVec const & = 0;
    [[nodiscard]] virtual auto do_important() const -> VariableSet = 0;
};

//! A unique pointer holding a statement.
using UStm = std::unique_ptr<Stm>;
//! A vector of statements.
using UStmVec = std::vector<UStm>;

//! Helper class to prepare statements for grounding.
class Linearizer {
  public:
    //! Indicate that a new domain is being prepared.
    void start(Queue &queue);
    //! Prepare a statement for grounding.
    void prepare(InstanceCallback &cb, ULitVec const &body, VariableSet important);

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
};

//! A statement capturing normal rules and integrity constraints.
class StmRule : public Stm {
  public:
    //! Construct the statement.
    StmRule(std::optional<std::pair<Ground::UTerm, Base &>> head, std::vector<size_t> indices, Ground::ULitVec body)
        : head_{head ? std::move(head->first) : nullptr}, base_{head ? &head->second : nullptr},
          indices_{std::move(indices)}, body_{std::move(body)} {
        if (head_) {
            body_.emplace_back(std::make_unique<LitFactCheck>(*base_, *head_, atom_));
        }
    }

  private:
    // Stm interface
    void do_print(std::ostream &out) const override;

    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override;
    void do_propagate(Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    //! The head of the rule.
    //!
    //! Note that this unique pointer is zero in case of constraints.
    UTerm head_;
    Base *base_;
    //! A list of indices.
    //!
    //! Recursive body literals that unify with the rule head have matching indices.
    //! This allows for updating the indices of these literals while grounding.
    std::vector<size_t> indices_;
    Ground::ULitVec body_;
    Symbol atom_ = SymbolStore::sup();
};

//! @}

} // namespace Gringo::Ground
