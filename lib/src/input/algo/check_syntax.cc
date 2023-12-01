#include <input/statement.hh>
#include <util/algorithm.hh>

#include <type_traits>

#include "visit.hh"

namespace Gringo::Input {

namespace {

enum ProjectionCheck {
    nerver = 0,
    function = 1,
    always = 2,
};

struct CheckSyntax {
    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &, ProjectionCheck check) const -> bool = delete;

    // terms

    auto operator()(Term const &term, ProjectionCheck check) const -> bool {
        return std::visit(*this, term, std::variant<ProjectionCheck>{check});
    }

    auto operator()(TermSymbol const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TermVariable const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(std::monostate /*unused*/, ProjectionCheck check) const -> bool {
        return check != ProjectionCheck::nerver;
    }

    auto operator()(TupleElem const &elem, ProjectionCheck check) const -> bool {
        return std::visit(*this, elem, std::variant<ProjectionCheck>{check});
    }

    auto operator()(TupleVec const &tuple, ProjectionCheck check) const -> bool {
        for (auto const &project_or_term : tuple) {
            if (!operator()(project_or_term, check)) {
                return false;
            }
        }
        return true;
    }

    auto operator()(TermTuple::Element const &elem, ProjectionCheck check) const -> bool {
        if (check < ProjectionCheck::always) {
            check = ProjectionCheck::nerver;
        }
        return std::visit(*this, elem, std::variant<ProjectionCheck>{check});
    }

    auto operator()(TermTuple const &term, ProjectionCheck check) const -> bool {
        for (auto const &tuple_or_term : term.pool) {
            if (!operator()(tuple_or_term, check)) {
                return false;
            }
        }
        return true;
    }

    auto operator()(TermFunction const &term, ProjectionCheck check) const -> bool {
        for (auto const &tuple : term.pool) {
            if (!operator()(tuple, check)) {
                return false;
            }
        }
        return true;
    }

    auto operator()(TermAbs const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(check);
        for (auto const &term : term.pool) {
            if (!operator()(term, ProjectionCheck::nerver)) {
                return false;
            }
        }
        return true;
    }

    auto operator()(TermUnary const &term, ProjectionCheck check) const -> bool {
        switch (term.op) {
            case UnaryOperator::invert: {
                if (check > ProjectionCheck::function) {
                    check = ProjectionCheck::function;
                }
                break;
            }
            case UnaryOperator::negate: {
                check = ProjectionCheck::nerver;
                break;
            }
        }
        return operator()(*term.rhs, check);
    }

    auto operator()(TermBinary const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(check);
        return operator()(*term.lhs, ProjectionCheck::nerver) && operator()(*term.rhs, ProjectionCheck::nerver);
    }

    /*
    // theory terms

    void operator()(TheoryTerm const &term) const { std::visit(*this, term); }

    void operator()(TheoryTermSymbol const &term) const { static_cast<void>(term); }

    void operator()(TheoryTermVariable const &term) const { fun(term.loc, term.name); }

    void operator()(TheoryTermTuple const &term) const { visit(term.elems); }

    void operator()(TheoryTermFunction const &term) const { visit(term.args); }

    void operator()(TheoryTermUnparsed const &term) const { visit(term.elems); }

    // literals

    void operator()(Literal const &lit) const { std::visit(*this, lit); }

    void operator()(LiteralBoolean const &lit) const { static_cast<void>(lit); }

    void operator()(LiteralRelation const &lit) const { visit(lit.lhs, lit.rhs); }

    void operator()(LiteralSymbolic const &lit) const { visit(lit.term); }

    // conditional literal

    void operator()(ConditionalLiteral const &cond_lit) const {
        if (ctx == VariableContext::all) {
            visit(cond_lit.cond);
        }
        visit(cond_lit.lits);
    }

    template <bool Conjunctive> void operator()(Junction<Conjunctive> const &lit) const { visit(lit.elems); }

    // aggregate

    void operator()(SetAggregateElement const &elem) const { visit(elem.lit, elem.cond); }

    template <bool HasSign> void operator()(SetAggregate<HasSign> const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // theory

    template <bool HasSign> void operator()(TheoryAtom<HasSign> const &atom) const {
        if (ctx == VariableContext::all) {
            visit(atom.elems);
        }
        visit(atom.name);
    }

    // head literal

    void operator()(HeadLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(SimpleHeadLiteral const &lit) const { operator()(lit.lit); }

    void operator()(HeadAggregate::Element const &elem) const { visit(elem.tuple, elem.lit, elem.cond); }

    void operator()(HeadAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // body literal

    void operator()(BodyLiteral const &lit) const { std::visit(*this, lit); }

    void operator()(SimpleBodyLiteral const &lit) const { operator()(lit.lit); }

    void operator()(BodyAggregate::Element const &elem) const { visit(elem.tuple, elem.cond); }

    void operator()(BodyAggregate const &lit) const {
        if (ctx == VariableContext::all) {
            visit(lit.elems);
        }
        visit(lit.lhs, lit.rhs);
    }

    // statement

    void operator()(Statement const &stm) const { return std::visit(*this, stm); }

    void operator()(Rule const &stm) const { visit(stm.head, stm.body); }

    void operator()(TheoryDefinition const &stm) const { static_cast<void>(stm); }

    void operator()(StatementOptimize::Tuple const &tuple) const { visit(tuple.weight, tuple.priority, tuple.terms); }

    void operator()(StatementOptimize const &stm) const {
        if (ctx == VariableContext::all) {
            visit(stm.elems);
        }
    }

    void operator()(StatementWeakConstraint const &stm) const { visit(stm.body, stm.tuple); }

    void operator()(StatementShow const &stm) const { visit(stm.term, stm.body); }

    void operator()(StatementShowSig const &stm) const { static_cast<void>(stm); }

    void operator()(StatementProject const &stm) const { visit(stm.term, stm.body); }

    void operator()(StatementProjectSig const &stm) const { static_cast<void>(stm); }

    void operator()(StatementDefined const &stm) const { static_cast<void>(stm); }

    void operator()(StatementExternal const &stm) const { visit(stm.term, stm.body, stm.type); }

    void operator()(StatementEdge::Edge const &edge) const { visit(edge.u, edge.v); }

    void operator()(StatementEdge const &stm) const { visit(stm.edges, stm.body); }

    void operator()(StatementHeuristic const &stm) const { visit(stm.atom, stm.body, stm.type, stm.prio, stm.mod); }

    void operator()(StatementScript const &stm) const { static_cast<void>(stm); }

    void operator()(StatementInclude const &stm) const { static_cast<void>(stm); }

    void operator()(StatementProgram const &stm) const { static_cast<void>(stm); }

    void operator()(StatementConst const &stm) const { static_cast<void>(stm); }

    void operator()(Comment const &stm) const { static_cast<void>(stm); }
    */
};

} // namespace

} // namespace Gringo::Input
