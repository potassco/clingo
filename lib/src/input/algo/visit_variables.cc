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

    void operator()(Term const &term) const { std::visit(*this, term); }

    void operator()(TermSymbol const &term) const { static_cast<void>(term); }

    void operator()(TermVariable const &term) const { fun(term.name); }

    void operator()(TermTuple const &term) const { visit(*this, term.pool); }

    void operator()(TermFunction const &term) const { visit(*this, term.pool); }

    void operator()(TermAbs const &term) const { visit(*this, term.pool); }

    void operator()(TermUnary const &term) const { visit(*this, term.rhs); }

    void operator()(TermBinary const &term) const {
        visit(*this, term.lhs);
        visit(*this, term.rhs);
    }

    VarVisitFun fun;
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

void visit_variables(Term const &term, VarVisitFun fun) { std::visit(VisitVariables{std::move(fun)}, term); }

void visit_variables(TheoryTerm const &term, VarVisitFun fun) { term.visit_variables(std::move(fun)); }

void visit_variables(Literal const &lit, VarVisitFun fun) { lit.visit_variables(std::move(fun)); }

void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    lit.visit_variables(std::move(fun), ctx);
}

void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    lit.visit_variables(std::move(fun), ctx);
}

void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx) {
    stm.visit_variables(std::move(fun), ctx);
}

} // namespace Gringo::Input
