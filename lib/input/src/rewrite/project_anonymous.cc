#include "transform.hh"

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/project_anonymous.hh>

namespace CppClingo::Input {

namespace {

auto is_anonymous(Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && var->anonymous();
}

class ProjectAnonymous : public Transformer<ProjectAnonymous> {
  public:
    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &expr) const = delete;

    // term

    [[nodiscard]] auto accept(Argument const &elem) const -> std::optional<Argument> {
        if (auto const *term = std::get_if<Term>(&elem); is_anonymous(term)) {
            return {Projection{location(*term)}};
        }
        return std::visit(
            [this](auto const &x) -> std::optional<Argument> {
                return Util::transform(transform(x), [](auto y) -> Argument { return {std::move(y)}; });
            },
            elem);
    };

    [[nodiscard]] auto accept(TermFunction const &term) const -> std::optional<Term> {
        if (term.external()) {
            return std::nullopt;
        }
        return rewrite(term, a_pool);
    }

    [[nodiscard]] auto accept(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return rewrite(term, a_rhs);
        }
        return std::nullopt;
    }

    template <class T>
        requires Util::is_among_v<T, TermAbs, TermBinary>
    [[nodiscard]] static auto accept([[maybe_unused]] TermAbs const &term) -> std::optional<Term> {
        return std::nullopt;
    }

    // theory term

    [[nodiscard]] static auto accept([[maybe_unused]] TheoryTerm const &term) -> std::optional<TheoryTerm> {
        return std::nullopt;
    }

    // literal

    [[nodiscard]] static auto accept([[maybe_unused]] LitComparison const &lit) -> std::optional<Lit> {
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LitSymbolic const &lit) const -> std::optional<Lit> {
        if (lit.sign() != Sign::none) {
            return rewrite(lit, a_term);
        }
        return std::nullopt;
    }

    // elements

    template <class T>
        requires Util::is_among_v<T, BdLitAggregateElement, OptimizeElement>
    [[nodiscard]] auto accept(T const &elem) const -> std::optional<T> {
        return rewrite(elem, a_cond);
    }

    [[nodiscard]] auto accept(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        return rewrite(elem, a_lit, a_cond);
    }

    // head literal

    template <class T>
        requires Util::is_among_v<T, HdLitAggregate, HdLitSetAggregate, HdLitTheoryAtom>
    [[nodiscard]] auto accept(T const &lit) const -> std::optional<HdLit> {
        return rewrite(lit, a_elems);
    }

    // body literal

    template <class T>
        requires Util::is_among_v<T, BdLitAggregate, BdLitSetAggregate, BdLitTheoryAtom>
    [[nodiscard]] auto accept(T const &lit) const -> std::optional<BdLit> {
        return rewrite(lit, a_elems);
    }

    // statement

    template <class T>
        requires Util::is_among_v<T, StmWeakConstraint, StmShow, StmProject, StmExternal, StmEdge, StmHeuristic>
    [[nodiscard]] auto accept(T const &stm) const -> std::optional<Stm> {
        return rewrite(stm, a_body);
    }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> {
    return ProjectAnonymous{}.transform(term);
}

auto project_anonymous(Lit const &lit) -> std::optional<Lit> {
    return ProjectAnonymous{}.transform(lit);
}

auto project_anonymous(HdLit const &lit) -> std::optional<HdLit> {
    return ProjectAnonymous{}.transform(lit);
}

auto project_anonymous(BdLit const &lit) -> std::optional<BdLit> {
    return ProjectAnonymous{}.transform(lit);
}

auto project_anonymous(Stm const &stm) -> std::optional<Stm> {
    return ProjectAnonymous{}.transform(stm);
}

} // namespace CppClingo::Input
