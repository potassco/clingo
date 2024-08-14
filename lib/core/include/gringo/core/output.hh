#pragma once

#include <gringo/core/core.hh>
#include <gringo/core/symbol.hh>

namespace Gringo {

//! @addtogroup core_output
//! @{

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
    void cond_lit(size_t uid) { do_cond_lit(uid); }
    //! Delayed output of an aggregate.
    //!
    //! Outputs a previously added aggregate if uid is given or starts
    //! outputting a fresh aggregate atom.
    auto bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t { return do_bd_aggr(sign, uid); }

  private:
    virtual void do_lit(Sign sign, Symbol sym) = 0;
    virtual void do_boolean(bool value) = 0;
    virtual void do_cond_lit(size_t uid) = 0;
    virtual auto do_bd_aggr(Sign sign, std::optional<size_t> uid) -> size_t = 0;
};

//! Interface to output statements.
class OutputStm {
  public:
    using BdElems = std::span<std::pair<SymbolSpan, std::span<size_t const>> const>;
    using Guards = std::span<std::pair<Relation, Symbol> const>;

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
    void rule(std::optional<Symbol> head) { do_rule(head); }

    //! Return an output for conditional literals.
    //!
    //! This allows for adding literal to the premise or condition of the literal.
    auto cond() -> OutputLit & { return do_cond(); }
    //! Add elements to the premise of a conditional literal.
    void cond_lit_premise(size_t lit_uid, size_t elem_uid) { do_cond_lit_premise(lit_uid, elem_uid); }
    //! Add elements to the conclusion of a conditional literal.
    //!
    //! At most one element must be added to the conclusion.
    void cond_lit_conclusion(size_t lit_uid, size_t elem_uid) { do_cond_lit_conclusion(lit_uid, elem_uid); }

    //! Commit a condition of simple literals returning its id.
    auto cond_id() -> size_t { return do_cond_id(); }

    //! Complete a delayed body aggregate.
    void bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) {
        do_bd_aggr(uid, fun, elems, guards);
    }

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
    virtual void do_rule(std::optional<Symbol> head) = 0;

    virtual auto do_cond() -> OutputLit & = 0;
    virtual void do_cond_lit_premise(size_t lit_uid, size_t elem_uid) = 0;
    virtual void do_cond_lit_conclusion(size_t lit_uid, size_t elem_uid) = 0;

    virtual auto do_cond_id() -> size_t = 0;

    virtual void do_bd_aggr(size_t uid, AggregateFunction fun, BdElems elems, Guards guards) = 0;

    virtual void do_flush() = 0;
    virtual void do_end_step() = 0;

    virtual void do_mark(SymbolCollector &gc) = 0;
};

//! Unique pointer for statement output.
using UOutputStm = std::unique_ptr<OutputStm>;

//! @}

} // namespace Gringo
