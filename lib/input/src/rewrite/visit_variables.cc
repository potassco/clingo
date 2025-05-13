#include "visit.hh"

#include <clingo/input/rewrite/visit_variables.hh>

namespace CppClingo::Input {

namespace {

class VisitVariables : public Visitor<VisitVariables> {
  public:
    VisitVariables(VarVisitFun fun, VariableContext ctx = VariableContext::all) : fun{std::move(fun)}, ctx{ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> void accept(T const &x) const = delete;

    // terms

    void accept(TermVariable const &term) const { fun(term.loc(), term.name()); }

    // theory terms

    void accept(TheoryTermVariable const &term) const { fun(term.loc(), term.name()); }

    // conditional literal

    void accept(CondLit const &cond_lit) const {
        if (ctx == VariableContext::all) {
            visit(cond_lit.cond(), cond_lit.lit());
        }
    }

    // aggregate

    void accept(SetAggregateElement const &elem) const { visit(elem.lit(), elem.cond()); }

    template <bool HasSign> void accept(SetAggregate<HasSign> const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems());
        }
        visit(lit.lhs(), lit.rhs());
    }

    // theory

    template <bool HasSign> void accept(TheoryAtom<HasSign> const &atom) const {
        if (ctx == VariableContext::all) {
            visit(atom.elems());
        }
        visit(atom.name(), atom.rhs());
    }

    // head literal

    void accept(HdLitAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems());
        }
        visit(lit.lhs(), lit.rhs());
    }

    // body literal

    void accept(BdLitAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems());
        }
        visit(lit.lhs(), lit.rhs());
    }

    // statement

    void accept(StmOptimize const &stm) const {
        if (ctx == VariableContext::all) {
            visit(stm.elems());
        }
    }

    static void accept([[maybe_unused]] StmConst const &stm) {}

  private:
    VarVisitFun fun;
    VariableContext ctx;
};

} // namespace

void visit_variables(Term const &term, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(term);
}

void visit_variables(TheoryTerm const &term, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(term);
}

void visit_variables(TheoryElement const &term, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(term);
}

void visit_variables(Lit const &lit, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(lit);
}

void visit_variables(CondLit const &lit, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(lit);
}

void visit_variables(SetAggregateElement const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(HdLitAggregateElement const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(HdLit const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(lit);
}

void visit_variables(BdLitAggregateElement const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(BdLit const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(lit);
}

void visit_variables(OptimizeElement const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(Stm const &stm, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(stm);
}

} // namespace CppClingo::Input
