#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/visit_variables.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

struct RewriteAnonymous {
    RewriteAnonymous(NameGen &gen) : gen{gen} {}

    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    // term

    auto operator()(Term const &term) const { return std::visit(*this, term); }

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        if (term.is_anonymous) {
            return TermVariable{gen.new_name(), true};
        }
        return std::nullopt;
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        return transform_construct<TermFunction>(term.name, tr(term.pool), term.external);
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(tr(term.pool));
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        return transform_construct<TermAbs>(tr(term.pool));
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        return transform_construct<TermUnary>(term.op, tr(term.rhs));
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        return transform_construct<TermBinary>(tr(term.lhs), term.op, tr(term.rhs));
    }

    // theory

    auto operator()(TheoryTerm const &term) const -> std::optional<TheoryTerm> { return std::visit(*this, term); }

    auto operator()(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermUnparsed>(tr(term.elems));
    }

    auto operator()(TheoryTermSymbol const &term) const -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TheoryTermVariable const &term) const -> std::optional<TheoryTerm> {
        if (term.is_anonymous) {
            return TheoryTermVariable{gen.new_name(), true};
        }
        return std::nullopt;
    }

    auto operator()(TheoryTermTuple const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermTuple>(term.type, tr(term.elems));
    }

    auto operator()(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        return transform_construct<TheoryTermFunction>(term.name, tr(term.args));
    }

    // literal

    auto operator()(Literal const &lit) const { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralRelation>(lit.sign, tr(lit.lhs), tr(lit.rhs));
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        return transform_construct<LiteralSymbolic>(lit.sign, tr(lit.term));
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &lit) const -> std::optional<ConditionalLiteral> {
        return transform_construct<ConditionalLiteral>(tr(lit.lits), tr(lit.cond));
    }

    // set aggregate

    auto operator()(SetAggregate::Element const &elem) const -> std::optional<SetAggregate::Element> {
        return transform_construct<SetAggregate::Element>(tr(elem.lit), tr(elem.cond));
    }

    auto operator()(SetAggregate const &aggr) const -> std::optional<SetAggregate> {
        return transform_construct<SetAggregate>(tr(aggr.lhs), tr(aggr.elems), tr(aggr.rhs));
    }

    // theory

    auto operator()(TheoryAtom const &aggr) const -> std::optional<TheoryAtom> {
        return transform_construct<TheoryAtom>(tr(aggr.name), tr(aggr.elems), tr(aggr.rhs));
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<Disjunction>(tr(lit.elems));
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(tr(lit.aggr));
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(tr(elem.tuple), tr(elem.lit), tr(elem.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(tr(lit.atom));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<Conjunction>(tr(lit.elems));
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.sign, tr(lit.aggr));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(tr(elem.tuple), tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.sign, tr(lit.lhs), lit.fun, tr(lit.elems), tr(lit.rhs));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.sign, tr(lit.atom));
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        // return std::visit(*this, stm);
        throw std::logic_error("implement me!!!");
    }

    NameGen &gen;
};

/*
struct RewriteAnonymousOld {
    auto operator()(Term const &term) const { return rewrite_anonymous(term, gen); }
    auto operator()(TheoryTerm const &term) const { return rewrite_anonymous(term, gen); }
    auto operator()(Literal const &lit) const { return rewrite_anonymous(lit, gen); }
    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return rewrite_anonymous(lit, gen); };
    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return rewrite_anonymous(lit, gen); };
    NameGen &gen;
};

class StatementRewriter : public StatementVisitor {
  public:
    StatementRewriter(NameGen &gen, std::optional<SStatement> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymousOld{gen_}); }

    void visit(Rule const &stm) const override {
        result_ = transform_construct_shared<Rule, Statement>(tra(stm.head()), tra(stm.body()));
    }

    void visit(TheoryDefinition const &stm) const override { static_cast<void>(stm); }

    void visit(StatementOptimize const &stm) const override {
        result_ = transform_construct_shared<StatementOptimize, Statement>(stm.type(), tra(stm.elements()));
    }

    void visit(StatementWeakConstraint const &stm) const override {
        result_ = transform_construct_shared<StatementWeakConstraint, Statement>(tra(stm.body()), tra(stm.tuple()));
    }

    void visit(StatementShow const &stm) const override {
        result_ = transform_construct_shared<StatementShow, Statement>(tra(stm.term()), tra(stm.body()));
    }

    void visit(StatementShowSig const &stm) const override { static_cast<void>(stm); }

    void visit(StatementProject const &stm) const override {
        result_ = transform_construct_shared<StatementProject, Statement>(tra(stm.term()), tra(stm.body()));
    }

    void visit(StatementProjectSig const &stm) const override { static_cast<void>(stm); }

    void visit(StatementDefined const &stm) const override { static_cast<void>(stm); }

    void visit(StatementExternal const &stm) const override {
        result_ =
            transform_construct_shared<StatementExternal, Statement>(tra(stm.term()), tra(stm.body()), tra(stm.type()));
    }

    void visit(StatementEdge const &stm) const override {
        result_ = transform_construct_shared<StatementEdge, Statement>(tra(stm.edges()), tra(stm.body()));
    }

    void visit(StatementHeuristic const &stm) const override {
        result_ = transform_construct_shared<StatementHeuristic, Statement>(
            tra(stm.atom()), tra(stm.body()), tra(stm.type()), tra(stm.priority()), tra(stm.modifier()));
    }

    void visit(StatementScript const &stm) const override { static_cast<void>(stm); }

    void visit(StatementInclude const &stm) const override { static_cast<void>(stm); }

    void visit(StatementProgram const &stm) const override { static_cast<void>(stm); }

    void visit(StatementConst const &stm) const override { static_cast<void>(stm); }

  private:
    NameGen &gen_;
    std::optional<SStatement> &result_;
};
*/

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
    VariableSet vars;
    visit_variables(
        stm, [&vars](auto const &var) { vars.emplace(var); }, VariableContext::all);
    auto gen = NameGen{std::move(vars)};
    return RewriteAnonymous{gen}(stm);
}

} // namespace Gringo::Input
