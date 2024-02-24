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
        return rewrite(term, a_pool);
    }

    [[nodiscard]] static auto accept(TermAbs const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return rewrite(term, a_rhs);
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
            return rewrite(lit, a_term);
        }
        return std::nullopt;
    }

    // head literal

    [[nodiscard]] auto accept(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return rewrite(elem, a_lit, a_cond);
    }

    [[nodiscard]] auto accept(HdLitAggregate const &lit) const -> std::optional<HdLit> { return rewrite(lit, a_elems); }

    [[nodiscard]] auto accept(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_elems);
    }

    [[nodiscard]] auto accept(HdLitTheoryAtom const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_elems);
    }

    // body literal

    [[nodiscard]] auto accept(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        return rewrite(elem, a_cond);
    }

    [[nodiscard]] auto accept(BdLitAggregate const &lit) const -> std::optional<BdLit> { return rewrite(lit, a_elems); }

    [[nodiscard]] auto accept(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_elems);
    }

    // theory

    [[nodiscard]] auto accept(BdLitTheoryAtom const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_elems);
    }

    // statement

    [[nodiscard]] auto accept(OptimizeElement const &elem) const -> std::optional<OptimizeElement> {
        return rewrite(elem, a_cond);
    }

    [[nodiscard]] auto accept(StmWeakConstraint const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }

    [[nodiscard]] auto accept(StmShow const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }

    [[nodiscard]] auto accept(StmProject const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }

    [[nodiscard]] auto accept(StmExternal const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }

    [[nodiscard]] auto accept(StmEdge const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }

    [[nodiscard]] auto accept(StmHeuristic const &stm) const -> std::optional<Stm> { return rewrite(stm, a_body); }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> { return ProjectAnonymous{}.transform(term); }

auto project_anonymous(Lit const &lit) -> std::optional<Lit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(HdLit const &lit) -> std::optional<HdLit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(BdLit const &lit) -> std::optional<BdLit> { return ProjectAnonymous{}.transform(lit); }

auto project_anonymous(Stm const &stm) -> std::optional<Stm> { return ProjectAnonymous{}.transform(stm); }

} // namespace Gringo::Input
