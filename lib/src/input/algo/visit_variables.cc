#include <input/algo/visit_variables.hh>

namespace Gringo::Input {

namespace {}

void visit_variables(TermVariable const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermSymbol const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermTuple const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermFunction const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermAbs const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermUnary const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(TermBinary const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("remove me!!!");
}

void visit_variables(Term const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("implement me!!!");
}

void visit_variables(TheoryTerm const &term, VarVisitFun fun) {
    static_cast<void>(term);
    static_cast<void>(fun);
    throw std::logic_error("implement me!!!");
}

void visit_variables(Literal const &lit, VarVisitFun fun) {
    static_cast<void>(lit);
    static_cast<void>(fun);
    throw std::logic_error("implement me!!!");
}

void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    static_cast<void>(lit);
    static_cast<void>(fun);
    static_cast<void>(ctx);
    throw std::logic_error("implement me!!!");
}

void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    static_cast<void>(lit);
    static_cast<void>(fun);
    static_cast<void>(ctx);
    throw std::logic_error("implement me!!!");
}

void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx) {
    static_cast<void>(stm);
    static_cast<void>(fun);
    static_cast<void>(ctx);
    throw std::logic_error("implement me!!!");
}

} // namespace Gringo::Input
