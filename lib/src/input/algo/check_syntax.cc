#include <algorithm>
#include <type_traits>

#include <util/algorithm.hh>

#include <input/statement.hh>

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

    auto operator()(std::monostate projected, ProjectionCheck check) const -> bool {
        static_cast<void>(projected);
        return check != ProjectionCheck::nerver;
    }

    auto operator()(TupleElem const &elem, ProjectionCheck check) const -> bool {
        return std::visit(*this, elem, std::variant<ProjectionCheck>{check});
    }

    auto operator()(TupleVec const &tuple, ProjectionCheck check) const -> bool {
        return std::all_of(tuple.begin(), tuple.end(),
                           [this, check](auto const &project_or_term) { return operator()(project_or_term, check); });
    }

    auto operator()(TermTuple::Element const &elem, ProjectionCheck check) const -> bool {
        if (check < ProjectionCheck::always) {
            check = ProjectionCheck::nerver;
        }
        return std::visit(*this, elem, std::variant<ProjectionCheck>{check});
    }

    auto operator()(TermTuple const &term, ProjectionCheck check) const -> bool {
        return std::all_of(term.pool.begin(), term.pool.end(),
                           [this, check](auto const &tuple_or_term) { return operator()(tuple_or_term, check); });
    }

    auto operator()(TermFunction const &term, ProjectionCheck check) const -> bool {
        return std::all_of(term.pool.begin(), term.pool.end(),
                           [this, check](auto const &tuple) { return operator()(tuple, check); });
    }

    auto operator()(TermAbs const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(check);
        return std::all_of(term.pool.begin(), term.pool.end(),
                           [this](auto const &term) { return operator()(term, ProjectionCheck::nerver); });
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

    // theory terms

    auto operator()(TheoryTerm const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermSymbol const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermVariable const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermTuple const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermFunction const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    auto operator()(TheoryTermUnparsed const &term, ProjectionCheck check) const -> bool {
        static_cast<void>(term);
        static_cast<void>(check);
        return true;
    }

    // literals

    auto operator()(Literal const &lit, ProjectionCheck check) const -> bool {
        return std::visit(*this, lit, std::variant<ProjectionCheck>{check});
    }

    auto operator()(LiteralBoolean const &lit, ProjectionCheck check) const -> bool {
        static_cast<void>(lit);
        static_cast<void>(check);
        return true;
    }

    auto operator()(LiteralRelation const &lit, ProjectionCheck check) const -> bool {
        static_cast<void>(check);
        return operator()(lit.lhs, ProjectionCheck::nerver) &&
               std::all_of(lit.rhs.begin(), lit.rhs.end(),
                           [this](auto &guard) { return operator()(guard.second, ProjectionCheck::nerver); });
    }

    auto operator()(LiteralSymbolic const &lit, ProjectionCheck check) const -> bool {
        static_cast<void>(lit);
        static_cast<void>(check);
        return true;
    }

    // conditional literal

    auto operator()(LiteralVec const &lits, ProjectionCheck check = ProjectionCheck::always) const -> bool {
        return std::all_of(lits.begin(), lits.end(), [this, check](auto const &lit) { return operator()(lit, check); });
    }

    template <bool Conjunctive> auto operator()(Junction<Conjunctive> const &lit) const -> bool {
        return std::all_of(lit.elems.begin(), lit.elems.end(), [this](auto const &elem) {
            return this->operator()(elem.lits, Conjunctive ? ProjectionCheck::always : ProjectionCheck::nerver) &&
                   operator()(elem.cond);
        });
    }

    // aggregate

    auto operator()(LGuard guard) const -> bool {
        return !guard.has_value() || operator()(guard->first, ProjectionCheck::nerver);
    }

    auto operator()(RGuard guard) const -> bool {
        return !guard.has_value() || operator()(guard->second, ProjectionCheck::nerver);
    }

    auto operator()(TermVec const &terms) const -> bool {
        return std::all_of(terms.begin(), terms.end(),
                           [this](auto const &term) { return operator()(term, ProjectionCheck::nerver); });
    }

    template <bool HasSign> auto operator()(SetAggregate<HasSign> const &lit) const -> bool {
        return std::all_of(lit.elems.begin(), lit.elems.end(),
                           [this](auto const &elem) {
                               return this->operator()(elem.lit,
                                                       HasSign ? ProjectionCheck::always : ProjectionCheck::nerver) &&
                                      operator()(elem.cond);
                           }) &&
               operator()(lit.lhs) && operator()(lit.rhs);
    }

    // theory

    template <bool HasSign> auto operator()(TheoryAtom<HasSign> const &atom) const -> bool {
        return operator()(atom.name, ProjectionCheck::nerver) &&
               std::all_of(atom.elems.begin(), atom.elems.end(),
                           [this](auto const &elem) { return this->operator()(elem.second); });
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> bool { return operator()(lit.lit, ProjectionCheck::nerver); }

    auto operator()(HeadAggregate const &lit) const -> bool {
        return std::all_of(lit.elems.begin(), lit.elems.end(),
                           [this](auto const &elem) {
                               return operator()(elem.tuple) && operator()(elem.lit, ProjectionCheck::nerver) &&
                                                                operator()(elem.cond);
                           }) &&
               operator()(lit.lhs) && operator()(lit.rhs);
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> bool { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> bool { return operator()(lit.lit, ProjectionCheck::always); }

    auto operator()(BodyAggregate const &lit) const -> bool {
        return std::all_of(lit.elems.begin(), lit.elems.end(),
                           [this](auto const &elem) { return operator()(elem.tuple) && operator()(elem.cond); }) &&
               operator()(lit.lhs) && operator()(lit.rhs);
    }

    /*
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
