#pragma once

#include <gringo/core/core.hh>
#include <gringo/core/symbol.hh>

namespace Gringo {

//! @addtogroup core_output
//! @{

//! Interface to output literals.
class OutputTheory {
  public:
    using IndexSpan = std::span<size_t const>;
    using OptGuard = std::optional<std::pair<size_t, size_t>>;
    //! Destroy the output.
    virtual ~OutputTheory() = default;
    //! Output the given symbolic literal.
    auto str(String val) -> size_t { return do_str(val); }
    //! Output the given symbolic literal.
    auto num(Number const &num) -> size_t { return do_num(num); }
    //! Output the given symbolic literal.
    auto fun(String name, std::span<size_t const> args) -> size_t { return do_fun(name, args); }
    //! Output the given tuple.
    auto tup(TheoryTermTupleType type, IndexSpan args) -> size_t { return do_tup(type, args); }
    //! Output the given element.
    auto elem(IndexSpan tuple, size_t cond) -> size_t { return do_elem(tuple, cond); }
    //! Output the given atom.
    void atm(size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard = std::nullopt) {
        do_atm(atom_uid, name, elems, guard);
    }

  private:
    virtual auto do_str(String val) -> size_t = 0;
    virtual auto do_num(Number const &val) -> size_t = 0;
    virtual auto do_fun(String name, std::span<size_t const> args) -> size_t = 0;
    virtual auto do_tup(TheoryTermTupleType type, std::span<size_t const> args) -> size_t = 0;
    virtual auto do_elem(IndexSpan tuple, size_t cond) -> size_t = 0;
    virtual void do_atm(size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard) = 0;
};

//! Interface to output literals.
class OutputLit {
  public:
    //! Destroy the output.
    virtual ~OutputLit() = default;
    //! Output the given symbolic literal.
    void lit(Sign sign, Symbol sym) { do_lit(sign, sym); }
    //! Output the given boolean constant.
    void boolean(bool value) { do_boolean(value); }
    //! Output the given conditional literal.
    //!
    //! Note that its elemens have to be accumulated before using the statement output.
    auto cond_lit(std::optional<size_t> uid) -> size_t { return do_cond_lit(uid); }
    //! Delayed output of a body aggregate.
    //!
    //! Outputs a previously added aggregate if uid is given or starts
    //! outputting a fresh aggregate atom.
    auto bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t { return do_bd_aggr(sign, uid); }
    //! Delayed output of a theory atom.
    auto bd_theory(Sign sign, std::optional<size_t> uid) -> size_t { return do_bd_theory(sign, uid); }

  private:
    virtual void do_lit(Sign sign, Symbol sym) = 0;
    virtual void do_boolean(bool value) = 0;
    virtual auto do_cond_lit(std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_bd_theory(Sign sign, std::optional<size_t> uid) -> size_t = 0;
};

//! Interface to output statements.
class OutputStm {
  public:
    //! A body aggregate element.
    //!
    //! The span captures the ids of conditions.
    using BdElem = std::pair<SymbolSpan, std::span<size_t const>>;
    //! A span of body aggregate elements.
    using BdElems = std::span<BdElem const>;
    //! A head aggregate element.
    //!
    //! The span captures the heads (`#sup` is used to represent `#true`) and
    //! the ids of conditions.
    using HdElem = std::pair<SymbolSpan, std::span<std::pair<Symbol, size_t> const>>;
    //! A span of body aggregate elements.
    using HdElems = std::span<HdElem const>;
    //! The guards of an aggregate.
    using Guards = std::span<std::pair<Relation, Symbol> const>;
    //! A conditional literal.
    //!
    //! The two sizes correspond to indices of conditions.
    //! If the first one is not given, it is assumed false.
    using CondLit = std::pair<std::optional<size_t>, size_t>;
    //! A span of conditional literals.
    using CondLits = std::span<CondLit const>;
    //! A disjunction element.
    using DisjunctionElem = std::pair<Symbol, std::span<size_t const>>;
    //! A span of disjunction elements.
    using DisjunctionElems = std::span<DisjunctionElem const>;

    //! Destroy the output.
    virtual ~OutputStm() = default;

    //! Generate a new unique id.
    auto uid() -> size_t { return do_uid(); }

    //! Output the given fact.
    void fact(Symbol sym) { do_fact(sym); }

    //! Get an output for body literals.
    auto body() -> OutputLit & { return do_body(); }
    //! Output the given rule.
    //!
    //! The body of the rule has to be output first.
    void rule(std::optional<std::pair<Symbol, bool>> head) { do_rule(head); }
    //! Output a head aggregate rule.
    auto aggr_rule(std::optional<size_t> uid) -> size_t { return do_aggr_rule(uid); }
    //! Output a theory atom rule.
    auto theory_rule(std::optional<size_t> uid) -> size_t { return do_theory_rule(uid); }
    //! Output a head aggregate rule.
    auto disjunctive_rule(std::optional<size_t> uid) -> size_t { return do_disjunctive_rule(uid); }
    //! Output the given weak constraint.
    //!
    //! The body of the rule has to be output first.
    void weak_constraint(Number const &weight, std::optional<Symbol> prio, SymbolSpan terms) {
        do_weak_constraint(weight, prio, terms);
    }
    //! Output the given heuristic statement.
    //!
    //! The body of the rule has to be output first.
    void heuristic(Symbol atom, Number const &weight, Number const *prio, HeuristicType type) {
        do_heuristic(atom, weight, prio, type);
    }

    //! Return an output for a condition.
    auto cond() -> OutputLit & { return do_cond(); }
    //! Commit a condition of simple literals returning its id.
    auto cond_id() -> size_t { return do_cond_id(); }

    //! Complete a delayed body aggregate.
    void bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) {
        do_bd_aggr(uid, fun, elems, guards);
    }
    //! Complete a delayed head aggregate.
    void hd_aggr(size_t uid, AggregateFunction fun, HdElems elems, Guards guards) {
        do_hd_aggr(uid, fun, elems, guards);
    }
    //! Complete a delayed disjunction.
    void disjunction(size_t uid, DisjunctionElems elems) { do_disjunction(uid, elems); }

    //! Complete a delayed conditional literal.
    void cond_lit(size_t uid, CondLits elems) { do_cond_lit(uid, elems); }

    //! Return a theory output.
    auto theory() -> OutputTheory & { return do_theory(); }

    //! Flush all delayed rule assuming they are completely defined.
    //!
    //! Should be called after grounding a component.
    void flush() { do_flush(); }
    //! End the current (incremental) grounding step.
    void end_step() { do_end_step(); }

    //! Mark owned symbols.
    void mark(SymbolCollector &gc) { do_mark(gc); }

  private:
    virtual auto do_uid() -> size_t = 0;

    virtual void do_fact(Symbol sym) = 0;

    virtual auto do_body() -> OutputLit & = 0;
    virtual void do_rule(std::optional<std::pair<Symbol, bool>> head) = 0;
    virtual auto do_aggr_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_theory_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual void do_weak_constraint(Number const &weight, std::optional<Symbol> prio, SymbolSpan terms) = 0;
    virtual void do_heuristic(Symbol atom, Number const &weight, Number const *prio, HeuristicType type) = 0;

    virtual auto do_cond() -> OutputLit & = 0;
    virtual auto do_cond_id() -> size_t = 0;

    virtual void do_cond_lit(size_t uid, CondLits elems) = 0;
    virtual void do_bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) = 0;
    virtual void do_hd_aggr(size_t uid, AggregateFunction fun, HdElems elems, Guards guards) = 0;
    virtual void do_disjunction(size_t uid, DisjunctionElems elems) = 0;

    virtual auto do_theory() -> OutputTheory & = 0;

    virtual void do_flush() = 0;
    virtual void do_end_step() = 0;

    virtual void do_mark(SymbolCollector &gc) = 0;
};

//! Unique pointer for statement output.
using UOutputStm = std::unique_ptr<OutputStm>;

//! @}

} // namespace Gringo
