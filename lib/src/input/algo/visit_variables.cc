#include <input/algo/visit_variables.hh>

// TODO: this is bogus and should be removed
#include "variables.hh"
#include "visit.hh"

namespace Gringo::Input {

namespace {

// Note: no need for a std::function here. Once everything is in place, this
// part can also be hidden and only the more specific functions have to be
// visible.

struct VisitVariables {

    // terms

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()(TermSymbol const &term) const { static_cast<void>(term); }

    void operator()(TermVariable const &term) const { fun(term.name); }

    void operator()(TermTuple const &term) const { visit_rec(*this, term.pool); }

    void operator()(TermFunction const &term) const { visit_rec(*this, term.pool); }

    void operator()(TermAbs const &term) const { visit_rec(*this, term.pool); }

    void operator()(TermUnary const &term) const { visit_rec(*this, term.rhs); }

    void operator()(TermBinary const &term) const { visit_rec(*this, term.lhs, term.rhs); }

    // theory terms

    void operator()(TheoryTerm const &term) const { std::visit(*this, term); }

    void operator()(TheoryTermSymbol const &term) const { static_cast<void>(term); }

    void operator()(TheoryTermVariable const &term) const { fun(term.name); }

    void operator()(TheoryTermTuple const &term) const { visit_rec(*this, term.elems); }

    void operator()(TheoryTermFunction const &term) const { visit_rec(*this, term.args); }

    void operator()(TheoryTermUnparsed const &term) const { visit_rec(*this, term.elems); }

    // literals

    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(LiteralBoolean const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralRelation const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralSymbolic const &lit) const { visit_rec(*this, lit.term); }

    // conditional literal

    void operator()(ConditionalLiteral const &cond_lit) const { visit_rec(*this, cond_lit.lits, cond_lit.cond); }

    // aggregate

    void operator()(SetAggregate::Element const &elem) const { visit_rec(*this, elem.lit, elem.cond); }

    void operator()(SetAggregate const &lit) const { visit_rec(*this, lit.elems, lit.lhs, lit.rhs); }

    // theory

    void operator()(TheoryAtom const &atom) const { visit_rec(*this, atom.name, atom.elems); }

    // head literal

    void operator()(HeadLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Disjunction const &lit) const { visit_rec(*this, lit.elems); }

    void operator()(HeadAggregate const &lit) const { visit_rec(*this, lit.elems, lit.lhs, lit.rhs); }

    void operator()(HeadSetAggregate const &lit) const { visit_rec(*this, lit.aggr); }

    void operator()(HeadTheoryAtom const &lit) const { visit_rec(*this, lit.atom); }

    // body literal

    void operator()(BodyLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Conjunction const &lit) const { visit_rec(*this, lit.elems); }

    void operator()(BodyAggregate const &lit) const { visit_rec(*this, lit.elems, lit.lhs, lit.rhs); }

    void operator()(BodySetAggregate const &lit) const { visit_rec(*this, lit.aggr); }

    void operator()(BodyTheoryAtom const &lit) const { visit_rec(*this, lit.atom); }

    VarVisitFun fun;
    VariableContext ctx = VariableContext::all;
};

} // namespace

// TODO: remove from here!

void visit_variables(TermVariable const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermSymbol const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermTuple const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermFunction const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermAbs const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermUnary const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TermBinary const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

// TODO: remove until here!

void visit_variables(Term const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(TheoryTerm const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}(term); }

void visit_variables(Literal const &lit, VarVisitFun fun) { VisitVariables{std::move(fun)}(lit); }

void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}(lit);
}

void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}(lit);
}

void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx) {
    stm.visit_variables(std::move(fun), ctx);
}

} // namespace Gringo::Input
