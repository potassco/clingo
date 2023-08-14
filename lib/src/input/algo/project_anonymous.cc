#include <input/algo/check_type.hh>
#include <input/algo/project_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto is_anonymous(Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && var->is_anonymous;
}

struct ProjectAnonymous : Transformer<ProjectAnonymous> {

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &) const -> std::optional<T> = delete;

    // ignore

    auto operator()(std::string const &x) const -> std::optional<std::string> {
        static_cast<void>(x);
        return std::nullopt;
    }

    template <class T> auto operator()(T const &x) const -> std::enable_if_t<std::is_enum_v<T>, std::optional<T>> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(std::monostate x) const -> std::optional<Term> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (is_anonymous(std::get_if<Term>(&elem))) {
            return {std::monostate{}};
        }
        // Note: a tiny bit lazy. Because monostate always maps to nullopt, we
        // can safely convert the resulting optional term back into a tuple
        // elem.
        return std::visit(*this, elem);
    };

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(term.loc, tr(term.pool));
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        if (term.external) {
            return std::nullopt;
        }
        return transform_construct<TermFunction>(term.loc, term.name, tr(term.pool), term.external);
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct<TermUnary>(term.loc, term.op, tr(term.rhs));
        }
        return std::nullopt;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // theory term

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    auto operator()(Literal const &lit) const -> std::optional<Literal> { return std::visit(*this, lit); }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        if (lit.sign != Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.loc, lit.sign, tr(lit.term));
        }
        return std::nullopt;
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &lit) const -> std::optional<ConditionalLiteral> {
        return transform_construct<ConditionalLiteral>(lit.loc, tr(lit.lits), tr(lit.cond));
    }

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const
        -> std::optional<std::conditional_t<Conjunctive, BodyLiteral, HeadLiteral>> {
        return transform_construct<Junction<Conjunctive>>(lit.loc, tr(lit.elems));
    }

    // aggregate

    auto operator()(SetAggregate::Element const &elem) const -> std::optional<SetAggregate::Element> {
        return transform_construct<SetAggregate::Element>(tr(elem.lit), tr(elem.cond));
    }

    auto operator()(SetAggregate const &aggr) const -> std::optional<SetAggregate> {
        return transform_construct<SetAggregate>(aggr.loc, aggr.lhs, tr(aggr.elems), aggr.rhs);
    }

    // theory

    auto operator()(TheoryAtom const &atom) const -> std::optional<TheoryAtom> {
        return transform_construct<TheoryAtom>(atom.loc, atom.name, tr(atom.elems), atom.rhs);
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(HeadAggregate::Element const &lit) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(lit.tuple, tr(lit.lit), tr(lit.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc, lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(tr(lit.aggr));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(tr(lit.atom));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(elem.tuple, tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc, lit.sign, lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.sign, tr(lit.aggr));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.sign, tr(lit.atom));
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> std::optional<Statement> {
        return transform_construct<Rule>(stm.loc, tr(stm.head), tr(stm.body));
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(elem.first, tr(elem.second));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementOptimize>(stm.loc, stm.type, tr(stm.elems));
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc, tr(stm.body), stm.tuple);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc, stm.term, tr(stm.body));
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.loc, stm.term, tr(stm.body));
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementExternal>(stm.loc, stm.term, tr(stm.body), stm.type);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc, stm.edges, tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc, stm.atom, tr(stm.body), stm.type, stm.prio, stm.mod);
    }

    auto operator()(StatementScript const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementConst const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(Comment const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> { return ProjectAnonymous{}(term); }

auto project_anonymous(Literal const &lit) -> std::optional<Literal> { return ProjectAnonymous{}(lit); }

auto project_anonymous(HeadLiteral const &lit) -> std::optional<HeadLiteral> { return ProjectAnonymous{}(lit); }

auto project_anonymous(BodyLiteral const &lit) -> std::optional<BodyLiteral> { return ProjectAnonymous{}(lit); }

auto project_anonymous(Statement const &stm) -> std::optional<Statement> { return ProjectAnonymous{}(stm); }

} // namespace Gringo::Input
