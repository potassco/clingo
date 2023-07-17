#pragma once

#include <input/statement.hh>

namespace Gringo::Input {

// TODO: maybe specialize more

//! Variable selection scopes.
//!
//! @see Statement::visit_variables()
enum class VariableContext {
    global, //!< Visit variables occurring in global scope.
    all,    //!< Visit all variable occurrences.
};

//! A function to visit variable occurrences.
using VarVisitFun = std::function<void(std::string const &var)>;

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

//! Variable selection modes for select_variables().
enum VariableSelectMode {
    add, //!< Add variables to the set.
    del, //!< Remove variables from the set.
};

//! Add/remove variables to/from a set occuring in the given expression.
template <class E> void select_variables(E &expr, VariableSet &vars, VariableSelectMode mode) {
    if (mode == VariableSelectMode::add) {
        visit_variables(expr, [&vars](std::string const &var) { vars.emplace(var); });
    } else {
        visit_variables(expr, [&vars](std::string const &var) { vars.erase(var); });
    }
}

//! Convenience method for @ref select_variables(E, VariableSet &, VariableSelectMode) returning a set.
template <class E> auto select_variables(E &expr, VariableSelectMode mode) -> VariableSet {
    VariableSet vars;
    select_variables(expr, vars, mode);
    return vars;
}

} // namespace Gringo::Input
