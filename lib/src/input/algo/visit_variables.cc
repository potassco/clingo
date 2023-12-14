#include <input/algo/visit_variables.hh>

#include "visit.hh"

namespace Gringo::Input {

namespace {

struct CollectVariables : Visitor<CollectVariables> {
    CollectVariables(VariableSet &vars) : vars{vars} {}
    ~CollectVariables() { vars.clear(); }

    template <class T> void accept(T const &x) const = delete;
    void accept(TermVariable const &term) const { vars.emplace(term.name); }

    VariableSet &vars;
};

struct VisitVariables : Visitor<VisitVariables> {
    VisitVariables(VarVisitFun fun, VariableContext ctx = VariableContext::all) : fun{std::move(fun)}, ctx{ctx} {}

    // protect ourselves -> no unintended overloads

    template <class T> void accept(T const &x) const = delete;

    // terms

    void accept(TermVariable const &term) const {
        if (!blocked.contains(term.name)) {
            fun(term.loc, term.name);
        }
    }

    // theory terms

    void accept(TheoryTermVariable const &term) const { fun(term.loc, term.name); }

    // conditional literal

    void accept(ConditionalLiteral const &cond_lit) const {
        if (ctx == VariableContext::all) {
            visit(cond_lit.cond, cond_lit.lits);
        } else {
            CollectVariables cv{blocked};
            cv.visit(cond_lit.cond);
            visit(cond_lit.lits);
        }
    }

    // aggregate

    void accept(SetAggregateElement const &elem) const { visit(elem.lit, elem.cond); }

    template <bool HasSign> void accept(SetAggregate<HasSign> const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // theory

    template <bool HasSign> void accept(TheoryAtom<HasSign> const &atom) const {
        if (ctx == VariableContext::all) {
            visit(atom.elems);
        }
        visit(atom.name);
    }

    // head literal

    void accept(HeadAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // body literal

    void accept(BodyAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // statement

    void accept(StatementOptimize const &stm) const {
        if (ctx == VariableContext::all) {
            visit(stm.elems);
        }
    }

    static void accept(StatementConst const &stm) { static_cast<void>(stm); }

    VarVisitFun fun;
    VariableContext ctx;
    mutable VariableSet blocked;
};

} // namespace

void visit_variables(Term const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}.visit(term); }

void visit_variables(TheoryTerm const &term, VarVisitFun fun) { VisitVariables{std::move(fun)}.visit(term); }

void visit_variables(Literal const &lit, VarVisitFun fun) { VisitVariables{std::move(fun)}.visit(lit); }

void visit_variables(ConditionalLiteral const &lit, VarVisitFun fun) { VisitVariables{std::move(fun)}.visit(lit); }

void visit_variables(SetAggregateElement const &elem, VarVisitFun fun) { VisitVariables{std::move(fun)}.visit(elem); }

void visit_variables(HeadAggregate::Element const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(HeadLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(lit);
}

void visit_variables(BodyAggregate::Element const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(BodyLiteral const &lit, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(lit);
}

void visit_variables(StatementOptimize::Element const &elem, VarVisitFun fun) {
    VisitVariables{std::move(fun)}.visit(elem);
}

void visit_variables(Statement const &stm, VarVisitFun fun, VariableContext ctx) {
    VisitVariables{std::move(fun), ctx}.visit(stm);
}

} // namespace Gringo::Input
