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
    return var != nullptr && var->is_anonymous_;
}

struct ProjectAnonymous : Transformer<ProjectAnonymous> {

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (auto const *term = std::get_if<Term>(&elem); is_anonymous(term)) {
            return {Projection{location(*term)}};
        }
        return std::visit(
            [this](auto const &x) -> std::optional<TupleElem> {
                return Util::transform(transform(x), [](auto &&y) -> TupleElem { return {GRINGO_FWD(y)}; });
            },
            elem);
    };

    [[nodiscard]] auto accept(TermFunction const &term) const -> std::optional<Term> {
        if (term.external_) {
            return std::nullopt;
        }
        return transform_construct<TermFunction>(term.loc(), term.name_, tr(term.pool_), term.external_);
    }

    [[nodiscard]] static auto accept(TermAbs const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct<TermUnary>(term.loc(), term.op_, tr(term.rhs_));
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
        if (lit.sign_ != Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.loc(), lit.sign_, tr(lit.term_));
        }
        return std::nullopt;
    }

    // head literal

    [[nodiscard]] auto accept(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        return transform_construct<HeadAggregate::Element>(elem.loc(), elem.tuple_, tr(elem.lit_), tr(elem.cond_));
    }

    [[nodiscard]] auto accept(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc(), lit.lhs_, lit.fun_, tr(lit.elems_), lit.rhs_);
    }

    [[nodiscard]] auto accept(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(lit.loc(), lit.lhs_, tr(lit.elems_), lit.rhs_);
    }

    [[nodiscard]] auto accept(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(lit.loc(), lit.name_, tr(lit.elems_), lit.rhs_);
    }

    // body literal

    [[nodiscard]] auto accept(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        return transform_construct<BodyAggregate::Element>(elem.loc(), elem.tuple_, tr(elem.cond_));
    }

    [[nodiscard]] auto accept(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.loc(), lit.sign_, lit.lhs_, lit.fun_, tr(lit.elems_), lit.rhs_);
    }

    [[nodiscard]] auto accept(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.loc(), lit.sign_, lit.lhs_, tr(lit.elems_), lit.rhs_);
    }

    // theory

    [[nodiscard]] auto accept(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.loc(), lit.sign_, lit.name_, tr(lit.elems_), lit.rhs_);
    }

    // statement

    [[nodiscard]] auto accept(StatementOptimize::Element const &elem) const
        -> std::optional<StatementOptimize::Element> {
        return transform_construct<StatementOptimize::Element>(elem.first, tr(elem.second));
    }

    [[nodiscard]] auto accept(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(stm.loc(), tr(stm.body_), stm.tuple_);
    }

    [[nodiscard]] auto accept(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.loc(), stm.term_, tr(stm.body_));
    }

    [[nodiscard]] auto accept(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.loc(), stm.term_, tr(stm.body_));
    }

    [[nodiscard]] auto accept(StatementExternal const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementExternal>(stm.loc(), stm.term_, tr(stm.body_), stm.type_);
    }

    [[nodiscard]] auto accept(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.loc(), stm.edges_, tr(stm.body_));
    }

    [[nodiscard]] auto accept(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.loc(), stm.atom_, tr(stm.body_), stm.type_, stm.prio_,
                                                       stm.mod_);
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
