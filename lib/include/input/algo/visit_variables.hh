#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

// Note: temporary to make progress.
// Internally, a visitor should be used.
void visit_variables(TermVariable const &term, VarVisitFun fun);
void visit_variables(TermSymbol const &term, VarVisitFun fun);
void visit_variables(TermTuple const &term, VarVisitFun fun);
void visit_variables(TermFunction const &term, VarVisitFun fun);
void visit_variables(TermAbs const &term, VarVisitFun fun);
void visit_variables(TermUnary const &term, VarVisitFun fun);
void visit_variables(TermBinary const &term, VarVisitFun fun);

void visit_variables(TermV2 const &term, VarVisitFun fun);
void visit_variables(TheoryTerm const &term, VarVisitFun fun);
void visit_variables(Literal const &lit, VarVisitFun fun);
void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx);
void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx);
void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx);

} // namespace Gringo::Input
