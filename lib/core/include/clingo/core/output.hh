#pragma once

#include <clingo/core/backend.hh>
#include <clingo/core/core.hh>
#include <clingo/core/symbol.hh>

#include <clingo/util/small_vector.hh>

namespace CppClingo {

//! @addtogroup core_output
//! @{

//! A span of indices.
using IndexSpan = std::span<size_t const>;
//! A vector of indices.
using IndexVec = std::vector<size_t>;

//! Interface to output literals.
class OutputTheory {
  public:
    //! The type of a theory atom.
    enum class AtomType : uint8_t {
        head,      //!< The atom occurs in the head.
        body,      //!< The atom occurs in the body.
        directive, //!< The atom is a body.
    };

    //! An optional guard of string and term indices.
    using OptGuard = std::optional<std::pair<String, size_t>>;
    //! Destroy the output.
    virtual ~OutputTheory() = default;
    //! Output the given symbolic literal.
    auto str(String val) -> size_t { return do_str(val); }
    //! Output the given symbolic literal.
    auto num(Number const &num) -> size_t { return do_num(num); }
    //! Output the given symbolic literal.
    auto fun(String name, IndexSpan args) -> size_t { return do_fun(name, args); }
    //! Output the given tuple.
    auto tup(TheoryTermTupleType type, IndexSpan args) -> size_t { return do_tup(type, args); }
    //! Output the given element.
    auto elem(IndexSpan tuple, size_t cond) -> size_t { return do_elem(tuple, cond); }
    //! Output the given atom.
    void atom(AtomType type, size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard = std::nullopt) {
        do_atom(type, atom_uid, name, elems, guard);
    }
    //! Output the given symbol.
    auto sym(Symbol sym) -> size_t { return do_sym(sym); }

  private:
    virtual auto do_str(String val) -> size_t = 0;
    virtual auto do_num(Number const &val) -> size_t = 0;
    virtual auto do_fun(String name, std::span<size_t const> args) -> size_t = 0;
    virtual auto do_tup(TheoryTermTupleType type, std::span<size_t const> args) -> size_t = 0;
    virtual auto do_sym(Symbol sym) -> size_t = 0;
    virtual auto do_elem(IndexSpan tuple, size_t cond) -> size_t = 0;
    virtual void do_atom(AtomType type, size_t atom_uid, Symbol name, IndexSpan elems, OptGuard guard) = 0;
};

//! Interface to output literals.
class OutputLit {
  public:
    //! Destroy the output.
    virtual ~OutputLit() = default;
    //! Output the given symbolic literal.
    void lit(Sign sign, Symbol sym, size_t uid) { do_lit(sign, sym, uid); }
    //! Output the given boolean constant.
    void boolean(bool value) { do_boolean(value); }
    //! Output the given conditional literal.
    //!
    //! Note that its elements have to be accumulated before using the statement output.
    auto cond_lit(std::optional<size_t> uid) -> size_t { return do_cond_lit(uid); }
    //! Delayed output of a body aggregate.
    //!
    //! Outputs a previously added aggregate if uid is given or starts
    //! outputting a fresh aggregate atom.
    auto bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t { return do_bd_aggr(sign, uid); }
    //! Delayed output of a theory atom.
    auto bd_theory(Sign sign, std::optional<size_t> uid) -> size_t { return do_bd_theory(sign, uid); }

  private:
    virtual void do_lit(Sign sign, Symbol sym, size_t uid) = 0;
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
    using BdElem = std::pair<SymbolSpan, IndexSpan>;
    //! A span of body aggregate elements.
    using BdElemSpan = std::span<BdElem const>;
    //! A head aggregate element.
    //!
    //! The span captures the heads (`#sup` is used to represent `#true`) and
    //! the ids of conditions.
    using HdElem = std::pair<SymbolSpan, std::span<std::tuple<Symbol, size_t, size_t> const>>;
    //! A span of body aggregate elements.
    using HdElemSpan = std::span<HdElem const>;
    //! A rhs guard.
    using Guard = std::pair<Relation, Symbol>;
    //! The guards of an aggregate.
    using GuardSpan = std::span<Guard const>;
    //! A conditional literal.
    //!
    //! The two sizes correspond to indices of conditions.
    //! If the first one is not given, it is assumed false.
    using CondLit = std::pair<std::optional<size_t>, size_t>;
    //! A span of conditional literals.
    using CondLitSpan = std::span<CondLit const>;
    //! A disjunction element.
    using DisjElem = std::tuple<Symbol, size_t, IndexSpan>;
    //! A span of disjunction elements.
    using DisjElemSpan = std::span<DisjElem const>;

    //! Destroy the output.
    virtual ~OutputStm() = default;

    //! Generate a new unique id for a literal.
    //!
    //! Parameter fact can be set to true. The output is free to map all facts to the same id.
    auto uid(bool fact = false) -> size_t { return do_uid(fact); }

    //! Output the given fact.
    void fact(Symbol sym, size_t uid) { do_fact(sym, uid); }

    //! Adds a projection rule for the given atom.
    void project_atom(size_t p_atom, size_t atom) { do_project_atom(p_atom, atom); }

    //! Get an output for body literals.
    auto body() -> OutputLit & { return do_body(); }
    //! Output the given rule.
    //!
    //! The body of the rule has to be output first.
    void rule(std::optional<std::tuple<Symbol, size_t, bool>> head) { do_rule(head); }
    //! Output the given external.
    void external(Symbol atom, size_t uid, ExternalType type) { do_external(atom, uid, type); }
    //! Output the given external.
    void project(Symbol atom, size_t uid) { do_project(atom, uid); }
    //! Output a head aggregate rule.
    auto aggr_rule(std::optional<size_t> uid) -> size_t { return do_aggr_rule(uid); }
    //! Output a theory atom rule.
    auto theory_rule(std::optional<size_t> uid) -> size_t { return do_theory_rule(uid); }
    //! Output a disjunctive rule.
    auto disjunctive_rule(std::optional<size_t> uid) -> size_t { return do_disjunctive_rule(uid); }
    //! Output the given weak constraint.
    //!
    //! The body of the rule has to be output first.
    void weak_constraint(Number const &weight, Number const *prio, SymbolSpan terms) {
        do_weak_constraint(weight, prio, terms);
    }
    //! Output the given heuristic statement.
    //!
    //! The body of the rule has to be output first.
    void heuristic(Symbol atom, size_t uid, Number const &weight, Number const *prio, HeuristicType type) {
        do_heuristic(atom, uid, weight, prio, type);
    }
    //! Output the given edge statement.
    //!
    //! The body of the rule has to be output first.
    void edge(Symbol src, Symbol dst) { do_edge(src, dst); }

    //! Output the given atom.
    void show_atom(Symbol atom, size_t uid) { do_show_atom(atom, uid); }
    //! Output the given term.
    //!
    //! The body of the corresponding statement has to be output first.
    void show_term(Symbol term) { do_show_term(term); }

    //! Return an output for a condition.
    auto cond() -> OutputLit & { return do_cond(); }
    //! Commit a condition of simple literals returning its id.
    auto cond_id() -> size_t { return do_cond_id(); }

    //! Complete a delayed body aggregate.
    void bd_aggr(size_t uid, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) {
        do_bd_aggr(uid, fun, elems, guards);
    }
    //! Complete a delayed head aggregate.
    void hd_aggr(size_t uid, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) {
        do_hd_aggr(uid, fun, elems, guards);
    }
    //! Complete a delayed disjunction.
    void disjunction(size_t uid, DisjElemSpan elems) { do_disjunction(uid, elems); }

    //! Complete a delayed conditional literal.
    void cond_lit(size_t uid, CondLitSpan elems) { do_cond_lit(uid, elems); }

    //! Return a theory output.
    auto theory() -> OutputTheory & { return do_theory(); }

    //! Flush all delayed rule assuming they are completely defined.
    //!
    //! Should be called after grounding a component.
    void flush() { do_flush(); }
    //! Handle classical negation of two atoms.
    void classical_negation(size_t atom_a, size_t atom_b) { do_classical_negation(atom_a, atom_b); }
    //! End the current (incremental) grounding step.
    void end_step() { do_end_step(); }

    //! Mark owned symbols.
    void mark(SymbolCollector &gc) { do_mark(gc); }

    //! Simplify stored state in the output.
    void simplify(std::function<TruthValue(prg_lit_t)> const &pred) { do_simplify(pred); }

  private:
    virtual auto do_uid(bool fact) -> size_t = 0;

    virtual void do_fact(Symbol sym, size_t uid) = 0;

    virtual void do_project_atom(size_t p_atom, size_t atom) = 0;

    virtual auto do_body() -> OutputLit & = 0;
    virtual void do_rule(std::optional<std::tuple<Symbol, size_t, bool>> head) = 0;
    virtual void do_external(Symbol atom, size_t uid, ExternalType type) = 0;
    virtual void do_project(Symbol atom, size_t uid) = 0;
    virtual auto do_aggr_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_theory_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual auto do_disjunctive_rule(std::optional<size_t> uid) -> size_t = 0;
    virtual void do_weak_constraint(Number const &weight, Number const *prio, SymbolSpan terms) = 0;
    virtual void do_heuristic(Symbol atom, size_t uid, Number const &weight, Number const *prio,
                              HeuristicType type) = 0;
    virtual void do_edge(Symbol src, Symbol dst) = 0;

    virtual void do_show_atom(Symbol atom, size_t uid) = 0;
    virtual void do_show_term(Symbol term) = 0;

    virtual auto do_cond() -> OutputLit & = 0;
    virtual auto do_cond_id() -> size_t = 0;

    virtual void do_cond_lit(size_t uid, CondLitSpan elems) = 0;
    virtual void do_bd_aggr(size_t uid, AggregateFunction fun, BdElemSpan elems, GuardSpan guards) = 0;
    virtual void do_hd_aggr(size_t uid, AggregateFunction fun, HdElemSpan elems, GuardSpan guards) = 0;
    virtual void do_disjunction(size_t uid, DisjElemSpan elems) = 0;

    virtual auto do_theory() -> OutputTheory & = 0;

    virtual void do_flush() = 0;
    virtual void do_classical_negation(size_t atom_a, size_t atom_b) = 0;
    virtual void do_end_step() = 0;

    virtual void do_mark(SymbolCollector &gc) = 0;
    virtual void do_simplify(std::function<TruthValue(prg_lit_t)> const &pred) = 0;
};

//! Unique pointer for statement output.
using UOutputStm = std::unique_ptr<OutputStm>;

//! @}

} // namespace CppClingo
