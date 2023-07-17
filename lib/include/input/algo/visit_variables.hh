#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

//! Visit variables with the given function.
void visit_variables(Term const &term, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(TheoryTerm const &term, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(Literal const &lit, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables in the given scope with the given function.
void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables in the given scope with the given function.
void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx);

} // namespace Gringo::Input
