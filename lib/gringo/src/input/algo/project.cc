#include <algorithm>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/project.hh>
#include <gringo/input/algo/project_anonymous.hh>
#include <gringo/input/algo/visit_variables.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto projectable(ProjectionMap project, Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && project.projectable(var->name(), var->anonymous());
}

auto get_counts(ProjectionMap project, auto const &elem) {
    Util::unordered_map<String, size_t> counts;
    visit_variables(elem, [&project, &counts](Location const &loc, String var) {
        static_cast<void>(loc);
        if (!project.counts().contains(var)) {
            ++counts[var];
        }
    });
    return counts;
}

struct Project : Transformer<Project> {

    Project(ProjectionMap project, bool in_classical_scope = true, bool project_lits = true)
        : project{std::move(project)}, in_classical_scope{in_classical_scope}, project_lits{project_lits} {}

    // protect ourselves -> no unintended overloads

    template <class T> [[nodiscard]] auto accept(T const &x) const -> std::optional<T> = delete;

    // term

    [[nodiscard]] auto accept(Argument const &elem) const -> std::optional<Argument> {
        if (auto const *term = std::get_if<Term>(&elem); projectable(project, term)) {
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
        return transform_construct2(term, a_pool);
    }

    [[nodiscard]] static auto accept(TermAbs const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct2(term, a_rhs);
        }
        return std::nullopt;
    }

    [[nodiscard]] static auto accept(TermBinary const &term) -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    [[nodiscard]] static auto accept(LitComparison const &lit) -> std::optional<Lit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LitSymbolic const &lit) const -> std::optional<Lit> {
        if (lit.sign() == Sign::none) {
            return transform_construct2(lit, a_term);
        }
        return std::nullopt;
    }

    // conditional literal

    [[nodiscard]] auto accept(CondLit const &elem) const -> std::optional<CondLit> {
        bool project_cond = in_classical_scope || !is_atom(elem.lit());
        // add counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};
        // project conclusion
        auto res_lit = std::optional<Lit>{};
        if (project_lits) {
            res_lit = sub_project.transform(elem.lit());
        }
        // project premise
        std::optional<LitArray> res_cond = std::nullopt;
        if (project_cond) {
            res_cond = sub_project.transform(elem.cond());
        }
        if (res_lit || res_cond) {
            return CondLit{elem.loc(), std::move(res_lit).value_or(elem.lit()),
                           std::move(res_cond).value_or(elem.cond())};
        }
        return std::nullopt;
    }

    // aggregate

    [[nodiscard]] auto accept(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        // add counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct2(elem, a_cond);
    }

    // head literal

    [[nodiscard]] static auto accept(HdLitSimple const &lit) -> std::optional<HdLit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(HdLitDisjunctionElement const &elem) const -> std::optional<HdLitDisjunctionElement> {
        return std::visit(
            [this](auto const &elem) -> std::optional<HdLitDisjunctionElement> {
                GRINGO_MATCH(elem, Lit) {
                    if (!project_lits) {
                        return std::nullopt;
                    }
                }
                return transform(elem);
            },
            elem);
    }
    [[nodiscard]] auto accept(HdLitDisjunction const &lit) const -> std::optional<HdLit> {
        // only projects variables in premise (almost body literals)
        auto sub_project = Project{project, true, false};
        return sub_project.transform_construct2(lit, a_elems);
    }

    [[nodiscard]] static auto accept(HdLitTheoryAtom const &lit) -> std::optional<HdLit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(HdLitAggregateElement const &elem) const -> std::optional<HdLitAggregateElement> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct2(elem, a_cond);
    }

    [[nodiscard]] auto accept(HdLitAggregate const &lit) const -> std::optional<HdLit> {
        return transform_construct2(lit, a_elems);
    }

    [[nodiscard]] auto accept(HdLitSetAggregate const &lit) const -> std::optional<HdLit> {
        // Note that we can always project in conditions. Semantic-wise a head
        // aggregate is a shortcut for a choice rule + a body aggregate in an
        // integrity constraint.
        return transform_construct2(lit, a_elems);
    }

    // body literal

    [[nodiscard]] auto accept(BdLitConjunction const &lit) const -> std::optional<BdLit> {
        // we project variables in premise if in classical scope,
        // we always project variables in conclusion.
        auto sub_project = Project{project, in_classical_scope, true};
        return sub_project.transform_construct2(lit, a_lit);
    }
    [[nodiscard]] auto accept(BdLitAggregateElement const &elem) const -> std::optional<BdLitAggregateElement> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct2(elem, a_cond);
    }

    [[nodiscard]] auto accept(BdLitAggregate const &lit) const -> std::optional<BdLit> {
        if (lit.sign() != Sign::none || in_classical_scope || !reduct_is_nonmonotone(lit.lhs(), lit.fun(), lit.rhs())) {
            return transform_construct2(lit, a_elems);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto accept(BdLitSetAggregate const &lit) const -> std::optional<BdLit> {
        if (lit.sign() == Sign::none && !in_classical_scope &&
            reduct_is_nonmonotone(lit.lhs(), AggregateFunction::count, lit.rhs())) {
            return std::nullopt;
        }
        return transform_construct2(lit, a_elems);
    }

    [[nodiscard]] static auto accept(BdLitTheoryAtom const &lit) -> std::optional<BdLit> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    // statement

    [[nodiscard]] auto accept(StmRule const &stm) const -> std::optional<Stm> {
        // do not project projection-like rules
        if (is_atom(stm.head())) {
            auto has_atom =
                std::any_of(stm.body().begin(), stm.body().end(), [](auto const &lit) { return is_atom(lit); });
            size_t n_test =
                std::count_if(stm.body().begin(), stm.body().end(), [](auto const &lit) { return is_test(lit); });
            if (has_atom && n_test == stm.body().size() - 1) {
                return std::nullopt;
            }
        }
        bool in_classical_scope = is_classical(stm.head());
        auto sub_project = Project{project, in_classical_scope};
        // Note that it would be nicest to be able to have two different
        // translators for head and body because the scope setting would
        // ideally just apply to the body. In the current implementation, the
        // head literals simply set the scope themselves.
        return sub_project.transform_construct2(stm, a_head, a_body);
    }

    [[nodiscard]] auto accept(OptimizeElement const &elem) const -> std::optional<OptimizeElement> {
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};
        return sub_project.transform_construct<OptimizeElement>(elem.first, tr(elem.second));
    }

    [[nodiscard]] auto accept(StmWeakConstraint const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmShow const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmProject const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmExternal const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmEdge const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    [[nodiscard]] auto accept(StmHeuristic const &stm) const -> std::optional<Stm> {
        return transform_construct2(stm, a_body);
    }

    ProjectionMap project;
    bool in_classical_scope;
    bool project_lits;
};

} // namespace

auto ProjectionMap::projectable(String const &var, bool anonymous) const -> bool {
    if (mode_ == ProjectionMode::disabled) {
        return false;
    }
    if (mode_ == ProjectionMode::anonymous && !anonymous) {
        return false;
    }
    auto it = counts_.find(var);
    return it != counts_.end() && it->second == 1;
}

auto ProjectionMap::counts() const -> Util::unordered_map<String, size_t> const & { return counts_; }

auto ProjectionMap::mode() const -> ProjectionMode { return mode_; }

auto project(Term const &term, ProjectionMap project) -> std::optional<Term> {
    return Project{project}.transform(term);
}

auto project(Lit const &lit, ProjectionMap project) -> std::optional<Lit> { return Project{project}.transform(lit); }

auto project(HdLit const &lit, ProjectionMap project) -> std::optional<HdLit> {
    return Project{project}.transform(lit);
}

auto project(BdLit const &lit, ProjectionMap project, bool in_classical_scope) -> std::optional<BdLit> {
    return Project{project, in_classical_scope}.transform(lit);
}

auto project(Stm const &stm, ProjectionMode mode, bool project_anonymous) -> std::optional<Stm> {
    std::optional<Stm> res;
    if (mode != ProjectionMode::disabled) {
        VariableSet vars = select_variables(stm, VariableContext::global);
        Util::unordered_map<String, size_t> counts;
        counts.reserve(vars.size());
        visit_variables(
            stm,
            [&vars, &counts](Location const &loc, String var) {
                static_cast<void>(loc);
                if (vars.contains(var)) {
                    ++counts[var];
                }
            },
            VariableContext::all);

        res = Project{ProjectionMap{mode, counts}}.transform(stm);
    }
    if (project_anonymous) {
        if (res.has_value()) {
            auto tmp = Gringo::Input::project_anonymous(res.value());
            if (tmp.has_value()) {
                res = std::move(tmp);
            }
        } else {
            res = Gringo::Input::project_anonymous(stm);
        }
    }
    return res;
}

} // namespace Gringo::Input
