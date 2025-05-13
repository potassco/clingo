#pragma once

#include <clingo/input/statement.hh>

#include <functional>

namespace CppClingo::Input {

//! @addtogroup input_visit_variables
//! @{

//! Variable selection scopes.
//!
//! @see Statement::visit_variables()
enum class VariableContext : uint8_t {
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
void visit_variables(TheoryElement const &term, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(Lit const &lit, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(CondLit const &lit, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(SetAggregateElement const &elem, VarVisitFun fun);
//! Visit variables with the given function.
void visit_variables(HdLitAggregateElement const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(HdLit const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables with the given function.
void visit_variables(BdLitAggregateElement const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(BdLit const &lit, VarVisitFun fun, VariableContext ctx);
//! Visit variables with the given function.
void visit_variables(OptimizeElement const &elem, VarVisitFun fun);
//! Visit variables in the given scope with the given function.
void visit_variables(Stm const &stm, VarVisitFun fun, VariableContext ctx);

//! Get all variables in an expression.
template <class T> [[nodiscard]] inline auto select_variables(T const &x, size_t size_hint = 0) -> VariableSet {
    VariableSet vars;
    if (size_hint > 0) {
        vars.reserve(size_hint);
    }
    visit_variables(x, [&]([[maybe_unused]] Location const &loc, String const &var) { vars.emplace(var); });
    return vars;
}

//! Get all variables in an expression in the given context.
template <class T>
[[nodiscard]] inline auto select_variables(T const &x, VariableContext context, size_t size_hint = 0) -> VariableSet {
    VariableSet vars;
    if (size_hint > 0) {
        vars.reserve(size_hint);
    }
    visit_variables(x, [&]([[maybe_unused]] Location const &loc, String const &var) { vars.emplace(var); }, context);
    return vars;
}

//! @}

} // namespace CppClingo::Input
