#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

class StateAggr {};

//! Literal representing an aggregate.
class LitAggr : public Lit {
  public:
    LitAggr(StateAggr &state) : state_{&state} {}

  private:
    void do_vars(VariableSet &vars, VarSelectMode mode) const override {
        static_cast<void>(vars);
        static_cast<void>(mode);
    }

    [[nodiscard]] auto do_domain() const -> bool override {
        // TODO: maybe the state will know
        return true;
    }

    [[nodiscard]] auto do_recursive() const -> bool override {
        // TODO: maybe the state will know
        return false;
    }

    [[nodiscard]] auto do_matcher(MatcherType type, std::vector<bool> const &bound)
        -> std::pair<UMatcher, std::optional<size_t>> override {
        static_cast<void>(type);
        static_cast<void>(bound);
        // TODO
        return {nullptr, 0};
    }

    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override {
        static_cast<void>(bound);
        return 0;
    }

    void do_print(std::ostream &out) const override { out << "TODO"; }

    auto do_output(InstantiationContext &ctx, OutputLit &out) const -> bool override {
        static_cast<void>(ctx);
        static_cast<void>(out);
        return true;
    }

    [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitAggr>(*state_); }

    [[nodiscard]] auto do_hash() const -> size_t override {
        // NOLINTNEXTLINE
        return Util::value_hash_record<LitAggr>(reinterpret_cast<uintptr_t>(this));
    }

    [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override { return this == &other; }

    [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override { return this <=> &other; }

    StateAggr *state_;
};

//! Gather aggregate elements.
//!
//! This class can also be used to derive empty aggregates. A tuple with a
//! neutral element has to be used, which is 0/\#sum/\#sup depending on the
//! type of the aggregate.
class StmAggrElem : public Stm {
  public:
    StmAggrElem(StateAggr &state, VariableVec global, UTermVec tuple, ULitVec body)
        : state_{&state}, global_{std::move(global)}, tuple_{std::move(tuple)}, body_{std::move(body)} {}

  private:
    void do_print(std::ostream &out) const override { out << "TODO"; }

    [[nodiscard]] auto do_body() const -> ULitVec const & override { return body_; }

    [[nodiscard]] auto do_important() const -> VariableSet override {
        auto res = VariableSet{};
        res.insert(global_.begin(), global_.end());
        for (auto const &term : tuple_) {
            term->vars(res);
        }
        return VariableSet{};
    }

    void do_init(size_t gen) override {
        // TODO: should be similar to condlit
        static_cast<void>(gen);
    }

    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override {
        static_cast<void>(ctx);
        return true;
    }

    void do_propagate(Queue &queue) override {
        static_cast<void>(state_);
        // TODO: should be similar to condlit
        static_cast<void>(queue);
    }

    [[nodiscard]] auto do_priority() const -> size_t override {
        // TODO: must be dynamic/should be similar to condlit
        return 0;
    }

    void do_print_head(std::ostream &out) const override { out << "TODO"; }

    StateAggr *state_;
    VariableVec global_;
    UTermVec tuple_;
    ULitVec body_;
};

} // namespace Gringo::Ground
