#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>

namespace Gringo::Ground {

using GuardVec = std::vector<std::pair<Relation, UTerm>>;

class StateAggr {
  public:
    StateAggr(VariableVec global, GuardVec guards, AggregateFunction fun, size_t index, bool monotone, bool recursive)
        : global_{std::move(global)}, guards_{std::move(guards)}, index_{index}, fun_{fun}, monotone_{monotone},
          recursive_{recursive} {}

    [[nodiscard]] auto global() const -> VariableVec const & { return global_; }
    [[nodiscard]] auto guards() const -> GuardVec const & { return guards_; }
    [[nodiscard]] auto fun() const -> AggregateFunction { return fun_; }
    [[nodiscard]] auto monotone() const -> bool { return monotone_; }
    [[nodiscard]] auto recursive() const -> bool { return recursive_; }
    [[nodiscard]] auto index() const -> size_t { return index_; }

  private:
    VariableVec global_;
    GuardVec guards_;
    size_t index_;
    AggregateFunction fun_;
    bool monotone_;
    bool recursive_;
};

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
        throw std::logic_error("implement me: matcher for aggregate literal");
    }

    [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override {
        static_cast<void>(bound);
        return 0;
    }

    void do_print(std::ostream &out) const override {
        auto const &guards = state_->guards();
        auto it = guards.begin();
        if (guards.size() > 1) {
            out << *it->second << " " << flip(it->first) << " ";
            ++it;
        }
        out << state_->fun() << "("
            << Util::p_range(state_->global(), [](std::ostream &out, auto var) { out << "X_" << var; }) << ")";
        if (state_->index() != stratified_index) {
            out << "[" << state_->index() << "]";
        }
        for (auto ie = guards.end(); it != ie; ++it) {
            out << " " << it->first << " " << *it->second;
        }
    }

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
    StmAggrElem(StateAggr &state, UTermVec tuple, ULitVec body, size_t num_cond, VariableVec local, size_t priority,
                bool dom, bool rec)
        : state_{&state}, tuple_{std::move(tuple)}, body_{std::move(body)}, local_{std::move(local)},
          num_cond_{num_cond}, priority_{priority}, dom_{dom}, rec_{rec} {
        static_cast<void>(local_);
        static_cast<void>(num_cond_);
        static_cast<void>(dom_);
        static_cast<void>(rec_);
    }

  private:
    [[nodiscard]] auto do_body() const -> ULitVec const & override { return body_; }

    [[nodiscard]] auto do_important() const -> VariableSet override {
        auto res = VariableSet{};
        res.insert(state_->global().begin(), state_->global().end());
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

    [[nodiscard]] auto do_priority() const -> size_t override { return priority_; }

    void do_print_head(std::ostream &out) const override {
        auto p_var = [](std::ostream &out, auto const &x) { out << "X_" << x; };
        auto p_term = [](std::ostream &out, auto const &x) { out << *x; };
        out << "#elem(";
        out << "g(" << Util::p_range{state_->global(), p_var} << ")";
        out << ",l(" << Util::p_range{local_, p_var} << ")";
        out << ",t(" << Util::p_range{tuple_, p_term} << ")";
        out << ")";
    }

    void do_print(std::ostream &out) const override {
        out << priority_ << ": ";
        print_head(out);
        if (state_->index() != stratified_index) {
            out << "[" << state_->index() << "]";
        }
        out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
    }

    StateAggr *state_;
    UTermVec tuple_;
    ULitVec body_;
    VariableVec local_;
    size_t num_cond_;
    size_t priority_;
    bool dom_;
    bool rec_;
};

} // namespace Gringo::Ground
