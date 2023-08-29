#pragma once

#include <functional>

#include <input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_visit_variables Visit Variables
//! @ingroup input_algo
//!
//! Functions to visit variables in expressions.
//! @{

//! Variable selection scopes.
//!
//! @see Statement::visit_variables()
enum class VariableContext {
    global, //!< Visit variables occurring in global scope.
    all,    //!< Visit all variable occurrences.
};

//! A function to visit variable occurrences.
using VarVisitFun = std::function<void(Location const &loc, String var)>;

//! Visit variables with the given function.
void visit_variables(Term const &term, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(TheoryTerm const &term, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(Literal const &lit, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(ConditionalLiteral const &lit, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(SetAggregateElement const &elem, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(HeadAggregate::Element const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables with the given function.
void visit_variables(BodyAggregate::Element const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables with the given function.
void visit_variables(StatementOptimize::Element const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx);

//! Get all variables in an expression.
template <class T, class C = decltype(static_cast<void (*)(T const &, VarVisitFun)>(visit_variables))>
inline auto select_variables(T const &x, size_t size_hint = 0) -> VariableSet {
    VariableSet vars;
    if (size_hint > 0) {
        vars.reserve(size_hint);
    }
    visit_variables(x, [&](Location const &loc, String const &var) {
        static_cast<void>(loc);
        vars.emplace(var);
    });
    return vars;
}

//! Get all variables in an expression in the given context.
template <class T, class C = decltype(static_cast<void (*)(T const &, VarVisitFun, VariableContext)>(visit_variables))>
inline auto select_variables(T const &x, VariableContext context = VariableContext::all, size_t size_hint = 0)
    -> VariableSet {
    VariableSet vars;
    if (size_hint > 0) {
        vars.reserve(size_hint);
    }
    visit_variables(
        x,
        [&](Location const &loc, String const &var) {
            static_cast<void>(loc);
            vars.emplace(var);
        },
        context);
    return vars;
}

//! @}

} // namespace Gringo::Input
