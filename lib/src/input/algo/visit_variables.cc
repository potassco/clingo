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

    void operator()(ConditionalLiteral const &cond_lit) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, cond_lit.cond);
        }
        visit_rec(*this, cond_lit.lits);
    }

    // aggregate

    void operator()(SetAggregate::Element const &elem) const { visit_rec(*this, elem.lit, elem.cond); }

    void operator()(SetAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, lit.elems);
        }
        visit_rec(*this, lit.lhs, lit.rhs);
    }

    // theory

    void operator()(TheoryAtom const &atom) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, atom.elems);
        }
        visit_rec(*this, atom.name);
    }

    // head literal

    void operator()(HeadLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Disjunction const &lit) const { visit_rec(*this, lit.elems); }

    void operator()(HeadAggregate::Element const &elem) const { visit_rec(*this, elem.tuple, elem.lit, elem.cond); }

    void operator()(HeadAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, lit.elems);
        }
        visit_rec(*this, lit.lhs, lit.rhs);
    }

    void operator()(HeadSetAggregate const &lit) const { visit_rec(*this, lit.aggr); }

    void operator()(HeadTheoryAtom const &lit) const { visit_rec(*this, lit.atom); }

    // body literal

    void operator()(BodyLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(Conjunction const &lit) const { visit_rec(*this, lit.elems); }

    void operator()(BodyAggregate::Element const &elem) const { visit_rec(*this, elem.tuple, elem.cond); }

    void operator()(BodyAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, lit.elems);
        }
        visit_rec(*this, lit.lhs, lit.rhs);
    }

    void operator()(BodySetAggregate const &lit) const { visit_rec(*this, lit.aggr); }

    void operator()(BodyTheoryAtom const &lit) const { visit_rec(*this, lit.atom); }

    // statement

    void operator()(Statement const &stm) const { return std::visit(*this, stm); }

    void operator()(Rule const &stm) const { visit_rec(stm.head, stm.body); }

    void operator()(TheoryDefinition const &stm) const { static_cast<void>(stm); }

    void operator()(StatementOptimize::Tuple const &tuple) const {
        visit_rec(tuple.weight, tuple.priority, tuple.terms);
    }

    void operator()(StatementOptimize const &stm) const {
        if (ctx == VariableContext::all) {
            visit_rec(*this, stm.elems);
        }
    }

    void operator()(StatementWeakConstraint const &stm) const { visit_rec(*this, stm.body, stm.tuple); }

    void operator()(StatementShow const &stm) const { visit_rec(*this, stm.term, stm.body); }

    void operator()(StatementShowSig const &stm) const { static_cast<void>(stm); }

    void operator()(StatementProject const &stm) const { visit_rec(*this, stm.term, stm.body); }

    void operator()(StatementProjectSig const &stm) const { static_cast<void>(stm); }

    void operator()(StatementDefined const &stm) const { static_cast<void>(stm); }

    void operator()(StatementExternal const &stm) const { visit_rec(*this, stm.term, stm.body, stm.type); }

    void operator()(StatementEdge::Edge const &edge) const { visit_rec(*this, edge.u, edge.v); }

    void operator()(StatementEdge const &stm) const { visit_rec(*this, stm.edges, stm.body); }

    void operator()(StatementHeuristic const &stm) const {
        visit_rec(*this, stm.atom, stm.body, stm.type, stm.prio, stm.mod);
    }

    void operator()(StatementScript const &stm) const { static_cast<void>(stm); }

    void operator()(StatementInclude const &stm) const { static_cast<void>(stm); }

    void operator()(StatementProgram const &stm) const { static_cast<void>(stm); }

    void operator()(StatementConst const &stm) const { static_cast<void>(stm); }

    VarVisitFun fun;
    VariableContext ctx = VariableContext::all;
};

} // namespace

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
    VisitVariables{std::move(fun), ctx}(stm);
}

} // namespace Gringo::Input
