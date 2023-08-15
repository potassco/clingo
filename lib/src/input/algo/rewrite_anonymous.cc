#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/visit_variables.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

struct RewriteAnonymous : Transformer<RewriteAnonymous> {
    RewriteAnonymous(NameGen &gen) : gen{gen} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // ignore

    auto operator()(std::monostate const &x) const -> std::optional<std::monostate> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(std::string const &x) const -> std::optional<std::string> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(Relation const &x) const -> std::optional<Relation> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        if (term.is_anonymous) {
            return TermVariable{term.loc, gen.new_name(), true};
        }
        return std::nullopt;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        return transform_construct<TermFunction>(term.loc, term.name, tr(term.pool), term.external);
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(term.loc, tr(term.pool));
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct<TermAbs>(term.loc, tr(term.pool));
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct<TermUnary>(term.loc, term.op, tr(term.rhs));
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct<TermBinary>(term.loc, tr(term.lhs), term.op, tr(term.rhs));
    }

    // theory

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> { return std::visit(*this, term); }

    auto operator()(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermUnparsed>(term.loc, tr(term.elems));
    }

    auto operator()(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        if (term.is_anonymous) {
            return TheoryTermVariable{term.loc, gen.new_name(), true};
        }
        return std::nullopt;
    }

    auto operator()(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermTuple>(term.loc, term.type, tr(term.elems));
    }

    auto operator()(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermFunction>(term.loc, term.name, tr(term.args));
    }

    // literal

    auto operator()(Literal const &lit) const { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralRelation>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.rhs));
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralSymbolic>(lit.loc, lit.sign, tr(lit.term));
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

    // set aggregate

    auto operator()(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        return transform_construct<SetAggregateElement>(tr(elem.lit), tr(elem.cond));
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteral> { return operator()(lit.lit); }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(tr(elem.tuple), tr(elem.lit), tr(elem.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(lit.loc, tr(lit.name), tr(lit.elems), tr(lit.rhs));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.loc, lit.sign, tr(lit.lhs), tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(tr(elem.tuple), tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc, lit.sign, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.loc, lit.sign, tr(lit.name), tr(lit.elems), tr(lit.rhs));
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

    auto operator()(StatementOptimize::Tuple const &elem) const -> std::optional<StatementOptimize::Tuple> {
        return transform_construct<StatementOptimize::Tuple>(tr(elem.weight), tr(elem.priority), tr(elem.terms));
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(tr(elem.first), tr(elem.second));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementOptimize>(stm.loc, stm.type, tr(stm.elems));
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc, tr(stm.body), tr(stm.tuple));
    }

    auto operator()(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc, tr(stm.term), tr(stm.body));
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.loc, tr(stm.term), tr(stm.body));
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
        return transform_construct<StatementExternal>(stm.loc, tr(stm.term), tr(stm.body), tr(stm.type));
    }

    auto operator()(StatementEdge::Edge const &edge) const -> std::optional<StatementEdge::Edge> {
        return transform_construct<StatementEdge::Edge>(tr(edge.u), tr(edge.v));
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc, tr(stm.edges), tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc, tr(stm.atom), tr(stm.body), tr(stm.type), tr(stm.prio),
                                                       tr(stm.mod));
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

    NameGen &gen;
};

} // namespace

auto NameGen::new_name() -> std::string {
    while (true) {
        std::string name = "__Aux_" + std::to_string(num_);
        ++num_;
        if (!vars_.contains(name)) {
            return name;
        }
    }
}

[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<Term> {
    return RewriteAnonymous{gen}(term);
}

[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<TheoryTerm> {
    return RewriteAnonymous{gen}(term);
}

[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<Literal> {
    return RewriteAnonymous{gen}(lit);
}

[[nodiscard]] auto rewrite_anonymous(HeadLiteral const &lit, NameGen &gen) -> std::optional<HeadLiteral> {
    return RewriteAnonymous{gen}(lit);
}

[[nodiscard]] auto rewrite_anonymous(BodyLiteral const &lit, NameGen &gen) -> std::optional<BodyLiteral> {
    return RewriteAnonymous{gen}(lit);
}

[[nodiscard]] auto rewrite_anonymous(Statement const &stm) -> std::optional<Statement> {
    auto gen = NameGen{select_variables(stm, VariableContext::all)};
    return RewriteAnonymous{gen}(stm);
}

} // namespace Gringo::Input
