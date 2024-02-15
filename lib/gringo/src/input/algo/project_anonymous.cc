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
    return var != nullptr && var->anonymous();
}

struct ProjectAnonymous : Transformer<ProjectAnonymous> {

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(Argument const &elem) const -> std::optional<Argument> {
        if (auto const *term = std::get_if<Term>(&elem); is_anonymous(term)) {
            return {Projection{location(*term)}};
        }
        return std::visit(
            [this](auto const &x) -> std::optional<Argument> {
                return Util::transform(transform(x), [](auto &&y) -> Argument { return {GRINGO_FWD(y)}; });
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

    [[nodiscard]] static auto accept(LitComparison const &lit) -> std::optional<Lit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LitSymbolic const &lit) const -> std::optional<Lit> {
        if (lit.sign() != Sign::none) {
            return transform_construct<LitSymbolic>(lit.loc(), lit.sign(), tr(lit.term()));
        }
        return std::nullopt;
    }

    // head literal

    [[nodiscard]] auto accept(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return transform_construct<HdLitAggregateElement>(elem.loc(), elem.tuple(), tr(elem.lit()), tr(elem.cond()));
    }

    [[nodiscard]] auto accept(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitAggregate>(lit.loc(), lit.lhs(), lit.fun(), tr(lit.elems()), lit.rhs());
    }

    [[nodiscard]] auto accept(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitSetAggregate>(lit.loc(), lit.lhs(), tr(lit.elems()), lit.rhs());
    }

    [[nodiscard]] auto accept(HdLitTheoryAtom const &lit) const -> std::optional<HdLit> {
        return transform_construct<HdLitTheoryAtom>(lit.loc(), lit.name(), tr(lit.elems()), lit.rhs());
    }

    // body literal

    [[nodiscard]] auto accept(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        return transform_construct<BdLitAggregateElement>(elem.loc(), elem.tuple(), tr(elem.cond()));
    }

    [[nodiscard]] auto accept(BdLitAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitAggregate>(lit.loc(), lit.sign(), lit.lhs(), lit.fun(), tr(lit.elems()),
                                                   lit.rhs());
    }

    [[nodiscard]] auto accept(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitSetAggregate>(lit.loc(), lit.sign(), lit.lhs(), tr(lit.elems()), lit.rhs());
    }

    // theory

    [[nodiscard]] auto accept(BdLitTheoryAtom const &lit) const -> std::optional<BdLit> {
        return transform_construct<BdLitTheoryAtom>(lit.loc(), lit.sign(), lit.name(), tr(lit.elems()), lit.rhs());
    }

    // statement

    [[nodiscard]] auto accept(OptimizeElement const &elem) const -> std::optional<OptimizeElement> {
        return transform_construct<OptimizeElement>(elem.first, tr(elem.second));
    }

    [[nodiscard]] auto accept(StmWeakConstraint const &stm) const -> std::optional<Stm> {
        return transform_construct<StmWeakConstraint>(stm.loc(), tr(stm.body()), stm.tuple());
    }

    [[nodiscard]] auto accept(StmShow const &stm) const -> std::optional<Stm> {
        return transform_construct<StmShow>(stm.loc(), stm.term(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StmProject const &stm) const -> std::optional<Stm> {
        return transform_construct<StmProject>(stm.loc(), stm.term(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StmExternal const &stm) const -> std::optional<Stm> {
        return transform_construct<StmExternal>(stm.loc(), stm.term(), tr(stm.body()), stm.type());
    }

    [[nodiscard]] auto accept(StmEdge const &stm) const -> std::optional<Stm> {
        return transform_construct<StmEdge>(stm.loc(), stm.edges(), tr(stm.body()));
    }

    [[nodiscard]] auto accept(StmHeuristic const &stm) const -> std::optional<Stm> {
        return transform_construct<StmHeuristic>(stm.loc(), stm.atom(), tr(stm.body()), stm.weight(), stm.prio(),
                                                 stm.type());
    }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> { return ProjectAnonymous{}.transform(term); }

auto project_anonymous(Lit const &lit) -> std::optional<Lit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(HdLit const &lit) -> std::optional<HdLit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(BdLit const &lit) -> std::optional<BdLit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(Stm const &stm) -> std::optional<Stm> { return ProjectAnonymous{}.transform(stm); }

} // namespace Gringo::Input
