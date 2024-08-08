#pragma once

#include <gringo/ground/literal.hh>
#include <gringo/ground/matcher.hh>
#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>

#include <iostream>

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

    auto propagate() -> bool {
        static_cast<void>(this);
        std::cerr << "implement me: propagate aggregate" << '\n';
        return false;
    }

  private:
    VariableVec global_;
    GuardVec guards_;
    size_t index_;
    AggregateFunction fun_;
    bool monotone_;
    bool recursive_;
};

class MatcherAggr : public OnceMatcher {
  private:
    auto do_once([[maybe_unused]] InstantiationContext &ctx) -> bool override {
        throw std::logic_error("implement aggregate matcher");
    }
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
        // TODO: create proper matcher
        return {std::make_unique<MatcherAggr>(),
                state_->index() != stratified_index ? std::make_optional(state_->index()) : std::nullopt};
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
        return res;
    }
    [[nodiscard]] auto do_is_important(size_t index) const -> bool override {
        // Only the literals gathered by do_important and the ones in the
        // condition are important. The remaining ones in the body can be
        // backtracked.
        return index < num_cond_;
    }

    void do_init([[maybe_unused]] size_t gen) override {
        // by construction, this statement does not increment the generation
    }

    [[nodiscard]] auto do_report(InstantiationContext &ctx) -> bool override {
        // TODO:
        // - the element should be stored including tuple and condition
        // - storing the tuple is straightforward
        // - there are different ways to store conditions
        //   1. the local variables would be sufficient
        //      + conditions representable as fixed size arrays
        //      + conditions can be restored for output later on
        //      + straightforward to implement
        //      . some overhead to restore literals from assignment
        //      . does not permit to implement all simplifications
        //        - weights with equivalent conditions can be combined
        //          to get a better estimate for lower/upper bounds
        //        - interesting for the stratified case
        //        - cumbersome/expensive in the recursive case
        //        - probably not important in practice
        //      > should work well in practice
        //        - the combination of weights is something that can be deferred to the backend
        //          (at the expense of a slight worse estimate of bounds)
        //        - some overhead for deferred output
        //   2. the atom indices could be stored because conditions are always simple literals
        //      + conditions representable as fixed size arrays
        //      + signs can be represented using the upper 2 bits
        //      + efficient mapping from indices to atoms for deferred output
        //      . all simplifications can be implemented
        //      . unnecessary storage of ids for facts but at least domain literals can be omitted
        //      > should work well in practice
        //        - weight simplification would be expensive in the recursive case
        //        - nicely works for deferred output
        //   3. literal ids could be used
        //      + all simplifications can be implemented
        //      + no restrictions regarding literals in conditions
        //      . conditions can be kept as short possible but have dynamic size
        //      - additonal bookkeeping to map from ids to literals for deferred output
        //      > not a good choice due to bookkeeping
        // - either point 1 or 2 should be implemented
        // - notes for point 1
        //   - atoms: globals -> aggregate id
        //   - tuples: (aggregate id, tuple) -> (formula id, fact, [[element id, locals]])
        //     - fact can be made implicit by storing an empty list
        //     - the element id and locals can be combined into one vector
        //     - the elements should be unique
        //     - a simple scan should suffice in practice because it is common that lists have a length of at most one
        // - notes for point 2
        //   - very similar to the above
        //   - potentially, shorter formulas if two elements have the same condition
        //     (uncommon in practice)
        // - the first implementation will be variant 1 because it does not require any interface extensions
        auto n = state_->global().size();
        auto &ass = ctx.ass();
        auto global = Util::SpanStack<Symbol>{n}; // TODO: has to be become part of state
        auto syms = global.push_map(state_->global(), [&ass](auto var) { return *ass[var]; });
        auto atoms = Util::ordered_map<Symbol const *, std::vector<size_t>, Util::SpanHash, Util::SpanEqualTo>{
            0, Util::SpanHash{n}, Util::SpanEqualTo{n}};
        auto [it, ins] = atoms.emplace(syms.data(), std::vector<size_t>{});
        if (!ins) {
            global.pop();
        }
        auto atom_idx = it - atoms.begin();

        // TODO: Tuples can have different sizes. The state must store
        // different tables for different sizes. In practice, it is very likely
        // that there is just one table so a forward list + a pointer in the
        // element rule should be fine.
        auto m = tuple_.size();
        // TODO: could be avoided adding yet another function to the span_stack
        auto buf = SymbolVec{};
        buf.reserve(m + 2);
        // with just a little bit of hackery, memory can be safed here
        buf.emplace_back(Symbol::from_rep(m));
        buf.emplace_back(Symbol::from_rep(atom_idx));
        for (auto &term : tuple_) {
            buf.emplace_back(*term->eval(ctx.store(), ass));
        }
        auto tuples = Util::SpanStack<Symbol>{m + 2};
        syms = tuples.push(buf);
        // TODO: the length of the tuble is stored in the beginning of the symbol pointer
        // dedicated hash and equal to functions have to be used
        auto elems = Util::ordered_map<Symbol const *, SymbolVec, Util::SpanHash, Util::SpanEqualTo>{
            0, Util::SpanHash{m + 2}, Util::SpanEqualTo{m + 2}};
        auto [jt, jns] = elems.emplace(syms.data(), SymbolVec{});
        if (!jns) {
            tuples.pop();
        }
        it.value().emplace_back(jt - elems.begin());

        // NOTE: It does not have to be a pointer. It could also be an index.
        auto stm_idx = reinterpret_cast<uintptr_t>(this); // NOLINT
        jt.value().emplace_back(Symbol::from_rep(stm_idx));
        for (auto const &var : local_) {
            jt.value().emplace_back(ass[var].value());
        }

        std::cerr << "accumulate:";
        std::cerr << "\n  atom idx: " << atom_idx;
        std::cerr << "\n  element id: <integer or maybe address of stm>";
        std::cerr << "\n  global:";
        for (auto const &var : state_->global()) {
            if (auto sym = ctx.ass()[var]) {
                std::cerr << " " << *sym;
            }
        }
        std::cerr << "\n  local:";
        for (auto const &var : local_) {
            if (auto sym = ctx.ass()[var]) {
                std::cerr << " " << *sym;
            }
        }
        std::cerr << "\n  tuple:";
        for (auto const &term : tuple_) {
            if (auto sym = term->eval(ctx.store(), ctx.ass())) {
                std::cerr << " " << *sym;
            }
        }
        std::cerr << "\n";
        return true;
    }

    void do_propagate(Queue &queue) override {
        // This is called after all statements on the current priority have
        // been processed. Thus, all element aggregation rules have been
        // processed. Here, aggregates that can match are added to the base and
        // are enqueued.
        if (state_->propagate() && state_->index() != stratified_index) {
            queue.propagate(state_->index());
        }
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
