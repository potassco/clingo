#pragma once

#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

struct BaseCondLit {
    void vars(VariableSet &res, bool all) const {
        if (all) {
            res.insert(local.begin(), local.end());
        }
        res.insert(global.begin(), global.end());
    }
    [[nodiscard]] auto vars(bool all) const -> VariableSet {
        VariableSet res;
        res.reserve(all ? global.size() + local.size() : global.size());
        vars(res, all);
        return res;
    }
    // map local symbols -> to state
    // state:
    //   - set of global symbols in element
    //   - propagated or not
    //     - not yet propagated literals can still be blocked
    //     - a literal is blocked if one of its premises is true
    //       but the conclusion false or not yet derived
    //     - if the conclusion is false, the whole literal becomes false
    //       and does not need to be propagated anymore
    //   - determine if fact when condition is stratified
    //     - if the premise is stratified then the literal can be marked as fact
    //       if all associated conclusions are true
    //     - needs a flag in base
    //   - the literal has to be propagated either by the premise or conclusion statement
    //     - if the conclusion is false there is no corresponding statement
    //       and the premise statement can trigger propagation
    //     - needs flag in base
    VariableVec local;
    VariableVec global;
    size_t index;
};

enum class LitCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
    lit = 4,
};
auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream &;

class LitCondLit : public Lit {
  public:
    LitCondLit(LitCondLitType type, BaseCondLit &base, size_t index) : base_{&base}, index_{index}, type_{type} {}
    void vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto domain(bool domain) const -> bool override;
    [[nodiscard]] auto recursive() const -> bool override;
    [[nodiscard]] auto matcher(MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto score(std::vector<bool> const &bound) const -> double override;
    void print(std::ostream &out) const override;
    auto output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool override;
    [[nodiscard]] auto copy() const -> ULit override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Lit const &other) const -> std::weak_ordering override;

  private:
    BaseCondLit *base_;
    size_t index_;
    LitCondLitType type_;
};

enum class StmCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
};
auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream &;

class StmCondLit : public Stm {
  public:
    StmCondLit(StmCondLitType type, BaseCondLit &base, ULitVec body, size_t prio, size_t index)
        : base_{&base}, body_{std::move(body)}, prio_{prio}, index_{index}, type_{type} {}
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
    size_t index_;
    StmCondLitType type_;
};

} // namespace Gringo::Ground
