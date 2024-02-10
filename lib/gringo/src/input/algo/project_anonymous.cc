#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/project_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto is_anonymous(Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && var->is_anonymous();
}

struct ProjectAnonymous : Transformer<ProjectAnonymous> {

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(ArgumentTuple::Element const &elem) const -> std::optional<ArgumentTuple::Element> {
        if (auto const *term = std::get_if<Term>(&elem); is_anonymous(term)) {
            return {Projection{location(*term)}};
        }
        return std::visit(
            [this](auto const &x) -> std::optional<ArgumentTuple::Element> {
                return Util::transform(transform(x),
                                       [](auto &&y) -> ArgumentTuple::Element { return {GRINGO_FWD(y)}; });
            },
            elem);
    };

    [[nodiscard]] auto accept(TermFunction const &term) const -> std::optional<Term> {
        if (term.external()) {
            return std::nullopt;
        }
        return transform_construct<TermFunction>(term.loc(), term.name(), tr(term.pool()), term.external());
    }

    [[nodiscard]] static auto accept(TermAbs const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct<TermUnary>(term.loc(), term.op(), tr(term.rhs()));
        }
        return std::nullopt;
    }

    [[nodiscard]] static auto accept(TermBinary const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // theory term

    [[nodiscard]] static auto accept(TheoryTerm const &term) -> std::optional<TheoryTerm> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    [[nodiscard]] static auto accept(LiteralRelation const &lit) -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        if (lit.sign() != Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.loc(), lit.sign(), tr(lit.term()));
        }
        return std::nullopt;
    }

    // head literal

    [[nodiscard]] auto accept(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(elem.loc(), elem.tuple(), tr(elem.lit()), tr(elem.cond()));
    }

    [[nodiscard]] auto accept(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc(), lit.lhs(), lit.fun(), tr(lit.elems()), lit.rhs());
    }

    [[nodiscard]] auto accept(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc(), lit.lhs(), tr(lit.elems()), lit.rhs());
    }

    [[nodiscard]] auto accept(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(lit.loc(), lit.name(), tr(lit.elems()), lit.rhs());
    }

    // body literal

    [[nodiscard]] auto accept(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(elem.loc(), elem.tuple(), tr(elem.cond()));
    }

    [[nodiscard]] auto accept(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc(), lit.sign(), lit.lhs(), lit.fun(), tr(lit.elems()),
                                                  lit.rhs());
    }

    [[nodiscard]] auto accept(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.loc(), lit.sign(), lit.lhs(), tr(lit.elems()), lit.rhs());
    }

    // theory

    [[nodiscard]] auto accept(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.loc(), lit.sign(), lit.name(), tr(lit.elems()), lit.rhs());
    }

    // statement

    [[nodiscard]] auto accept(StatementOptimize::Element const &elem) const
        -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(elem.first, tr(elem.second));
    }

    [[nodiscard]] auto accept(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc(), tr(stm.body()), stm.tuple());
    }

    [[nodiscard]] auto accept(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc(), stm.term(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.loc(), stm.term(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StatementExternal const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementExternal>(stm.loc(), stm.term(), tr(stm.body()), stm.type());
    }

    [[nodiscard]] auto accept(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc(), stm.edges(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc(), stm.atom(), tr(stm.body()), stm.type(), stm.prio(),
                                                       stm.mod());
    }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> { return ProjectAnonymous{}.transform(term); }

auto project_anonymous(Literal const &lit) -> std::optional<Literal> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(HeadLiteral const &lit) -> std::optional<HeadLiteral> {
    return ProjectAnonymous{}.transform(lit);
}

auto project_anonymous(BodyLiteral const &lit) -> std::optional<BodyLiteral> {
    return ProjectAnonymous{}.transform(lit);
}

auto project_anonymous(Statement const &stm) -> std::optional<Statement> { return ProjectAnonymous{}.transform(stm); }

} // namespace Gringo::Input
