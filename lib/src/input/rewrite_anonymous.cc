#include <input/rewrite_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

struct RewriteAnonymous {
    auto operator()(STerm const &term) const { return rewrite_anonymous(*term, gen); }
    auto operator()(STheoryTerm const &term) const { return rewrite_anonymous(*term, gen); }
    auto operator()(SLiteral const &lit) const { return rewrite_anonymous(*lit, gen); }
    auto operator()(TheoryAtom const &aggr) const -> std::optional<TheoryAtom> {
        return transform_construct<TheoryAtom>(Trans{aggr.name(), *this}, Trans{aggr.elements(), *this},
                                               Trans{aggr.rhs(), *this});
    };
    auto operator()(SetAggregate const &aggr) const -> std::optional<SetAggregate> {
        return transform_construct<SetAggregate>(Trans{aggr.lhs(), *this}, Trans{aggr.elements(), *this},
                                                 Trans{aggr.rhs(), *this});
    };
    auto operator()(SHeadLiteral const &lit) const -> std::optional<SHeadLiteral> {
        return rewrite_anonymous(*lit, gen);
    };
    auto operator()(SBodyLiteral const &lit) const -> std::optional<SBodyLiteral> {
        return rewrite_anonymous(*lit, gen);
    };
    NameGen &gen;
};

class TermRewriter : public TermVisitor {
  public:
    TermRewriter(NameGen &gen, std::optional<STerm> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

    void visit(TermSymbol const &term) const override { static_cast<void>(term); }

    void visit(TermVariable const &term) const override {
        if (term.is_anonymous()) {
            result_ = Util::construct_shared<TermVariable, Term>(gen_.new_name(), true);
        }
    }

    void visit(TermFunction const &term) const override {
        result_ = transform_construct_shared<TermFunction, Term>(term.name(), tra(term.pool()), term.is_external());
    }

    void visit(TermTuple const &term) const override {
        result_ = transform_construct_shared<TermTuple, Term>(tra(term.pool()));
    }

    void visit(TermAbs const &term) const override {
        result_ = transform_construct_shared<TermAbs, Term>(tra(term.pool()));
    }

    void visit(TermUnary const &term) const override {
        result_ = transform_construct_shared<TermUnary, Term>(term.unary_operator(), tra(term.rhs()));
    }

    void visit(TermBinary const &term) const override {
        result_ =
            transform_construct_shared<TermBinary, Term>(tra(term.lhs()), term.binary_operator(), tra(term.rhs()));
    }

  private:
    NameGen &gen_;
    std::optional<STerm> &result_;
};

class TheoryTermRewriter : public TheoryTermVisitor {
  public:
    TheoryTermRewriter(NameGen &gen, std::optional<STheoryTerm> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

    void visit(TheoryTermUnparsed const &term) const override {
        result_ = transform_construct_shared<TheoryTermUnparsed, TheoryTerm>(tra(term.elements()));
    }

    void visit(TheoryTermSymbol const &term) const override { static_cast<void>(term); }

    void visit(TheoryTermVariable const &term) const override {
        if (term.is_anonymous()) {
            result_ = Util::construct_shared<TheoryTermVariable, TheoryTerm>(gen_.new_name(), true);
        }
    }

    void visit(TheoryTermTuple const &term) const override {
        result_ = transform_construct_shared<TheoryTermTuple, TheoryTerm>(term.type(), tra(term.elements()));
    }

    void visit(TheoryTermFunction const &term) const override {
        result_ = transform_construct_shared<TheoryTermFunction, TheoryTerm>(term.name(), tra(term.arguments()));
    }

  private:
    NameGen &gen_;
    std::optional<STheoryTerm> &result_;
};

class LiteralRewriter : public LiteralVisitor {
  public:
    LiteralRewriter(NameGen &gen, std::optional<SLiteral> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

    void visit(LiteralBoolean const &lit) const override { static_cast<void>(lit); }

    void visit(LiteralRelation const &lit) const override {
        result_ = transform_construct_shared<LiteralRelation, Literal>(lit.sign(), tra(lit.lhs()), tra(lit.rhs()));
    }

    void visit(LiteralSymbolic const &lit) const override {
        result_ = transform_construct_shared<LiteralSymbolic, Literal>(lit.sign(), tra(lit.term()));
    }

  private:
    NameGen &gen_;
    std::optional<SLiteral> &result_;
};

class HeadLiteralRewriter : public HeadLiteralVisitor {
  public:
    HeadLiteralRewriter(NameGen &gen, std::optional<SHeadLiteral> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

    void visit(Disjunction const &lit) const override {
        result_ = transform_construct_shared<Disjunction, HeadLiteral>(tra(lit.elements()));
    }

    void visit(HeadSetAggregate const &lit) const override {
        result_ = transform_construct_shared<HeadSetAggregate, HeadLiteral>(tra(lit.atom()));
    }

    void visit(HeadAggregate const &lit) const override {
        result_ = transform_construct_shared<HeadAggregate, HeadLiteral>(tra(lit.lhs()), lit.function(),
                                                                         tra(lit.elements()), tra(lit.rhs()));
    }

    void visit(HeadTheoryAtom const &lit) const override {
        result_ = transform_construct_shared<HeadTheoryAtom, HeadLiteral>(tra(lit.atom()));
    }

  private:
    NameGen &gen_;
    std::optional<SHeadLiteral> &result_;
};

class BodyLiteralRewriter : public BodyLiteralVisitor {
  public:
    BodyLiteralRewriter(NameGen &gen, std::optional<SBodyLiteral> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

    void visit(Conjunction const &lit) const override {
        result_ = transform_construct_shared<Conjunction, BodyLiteral>(tra(lit.elements()));
    }

    void visit(BodySetAggregate const &lit) const override {
        result_ = transform_construct_shared<BodySetAggregate, BodyLiteral>(lit.sign(), tra(lit.atom()));
    }

    void visit(BodyAggregate const &lit) const override {
        result_ = transform_construct_shared<BodyAggregate, BodyLiteral>(lit.sign(), tra(lit.lhs()), lit.function(),
                                                                         tra(lit.elements()), tra(lit.rhs()));
    }

    void visit(BodyTheoryAtom const &lit) const override {
        result_ = transform_construct_shared<BodyTheoryAtom, BodyLiteral>(lit.sign(), tra(lit.atom()));
    }

  private:
    NameGen &gen_;
    std::optional<SBodyLiteral> &result_;
};

class StatementRewriter : public StatementVisitor {
  public:
    StatementRewriter(NameGen &gen, std::optional<SStatement> &result) : gen_{gen}, result_{result} {}

    [[nodiscard]] auto tra(auto const &x) const { return Trans(x, RewriteAnonymous{gen_}); }

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

[[nodiscard]] auto rewrite_anonymous(Term const &term, NameGen &gen) -> std::optional<STerm> {
    std::optional<STerm> result;
    term.accept(TermRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(TheoryTerm const &term, NameGen &gen) -> std::optional<STheoryTerm> {
    std::optional<STheoryTerm> result;
    term.accept(TheoryTermRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(Literal const &lit, NameGen &gen) -> std::optional<SLiteral> {
    std::optional<SLiteral> result;
    lit.accept(LiteralRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(HeadLiteral const &lit, NameGen &gen) -> std::optional<SHeadLiteral> {
    std::optional<SHeadLiteral> result;
    lit.accept(HeadLiteralRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(BodyLiteral const &lit, NameGen &gen) -> std::optional<SBodyLiteral> {
    std::optional<SBodyLiteral> result;
    lit.accept(BodyLiteralRewriter{gen, result});
    return result;
}

[[nodiscard]] auto rewrite_anonymous(Statement const &stm) -> std::optional<SStatement> {
    std::optional<SStatement> result;
    VariableSet vars;
    stm.visit_variables([&vars](std::string const &var) { vars.emplace(var); }, VariableContext::all);
    auto gen = NameGen{std::move(vars)};
    stm.accept(StatementRewriter{gen, result});
    return result;
}

} // namespace Gringo::Input
