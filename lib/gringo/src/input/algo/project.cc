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
    return var != nullptr && project.projectable(var->name(), var->is_anonymous());
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

    [[nodiscard]] auto accept(ArgumentTuple::Element const &elem) const -> std::optional<ArgumentTuple::Element> {
        if (auto const *term = std::get_if<Term>(&elem); projectable(project, term)) {
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

    // literal

    [[nodiscard]] static auto accept(LiteralRelation const &lit) -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        if (lit.sign_ == Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.loc(), lit.sign_, tr(lit.term_));
        }
        return std::nullopt;
    }

    // conditional literal

    [[nodiscard]] auto accept(ConditionalLiteral const &elem) const -> std::optional<ConditionalLiteral> {
        bool project_cond = in_classical_scope || !is_atom(elem.lit_);
        // add counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};
        // project conclusion
        auto res_lit = std::optional<Literal>{};
        if (project_lits) {
            res_lit = sub_project.transform(elem.lit_);
        }
        // project premise
        std::optional<LiteralVec> res_cond = std::nullopt;
        if (project_cond) {
            res_cond = sub_project.transform(elem.cond_);
        }
        if (res_lit || res_cond) {
            return ConditionalLiteral{elem.loc(), std::move(res_lit).value_or(elem.lit_),
                                      std::move(res_cond).value_or(elem.cond_)};
        }
        return std::nullopt;
    }

    // aggregate

    [[nodiscard]] auto accept(SetAggregateElement const &elem) const -> std::optional<SetAggregateElement> {
        // add counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<SetAggregateElement>(elem.loc(), elem.lit_, tr(elem.cond_));
    }

    // head literal

    [[nodiscard]] static auto accept(SimpleHeadLiteral const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(Disjunction::Element const &elem) const -> std::optional<Disjunction::Element> {
        return std::visit(
            [this](auto const &elem) -> std::optional<Disjunction::Element> {
                GRINGO_MATCH(elem, Literal) {
                    if (!project_lits) {
                        return std::nullopt;
                    }
                }
                return transform(elem);
            },
            elem);
    }
    [[nodiscard]] auto accept(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        // only projects variables in premise (almost body literals)
        auto sub_project = Project{project, true, false};
        return sub_project.transform_construct<Disjunction>(lit.loc(), tr(lit.elems_));
    }

    [[nodiscard]] static auto accept(HeadTheoryAtom const &lit) -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    [[nodiscard]] auto accept(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<HeadAggregate::Element>(elem.loc(), elem.tuple_, elem.lit_,
                                                                       tr(elem.cond_));
    }

    [[nodiscard]] auto accept(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.loc(), lit.lhs_, lit.fun_, tr(lit.elems_), lit.rhs_);
    }

    [[nodiscard]] auto accept(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        // Note that we can always project in conditions. Semantic-wise a head
        // aggregate is a shortcut for a choice rule + a body aggregate in an
        // integrity constraint.
        return transform_construct<HeadSetAggregate>(lit.loc(), lit.lhs_, tr(lit.elems_), lit.rhs_);
    }

    // body literal

    [[nodiscard]] auto accept(Conjunction const &lit) const -> std::optional<BodyLiteral> {
        // we project variables in premise if in classical scope,
        // we always project variables in conclusion.
        auto sub_project = Project{project, in_classical_scope, true};
        return sub_project.transform_construct<ConditionalLiteral>(tr(lit.lit_));
    }
    [[nodiscard]] auto accept(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<BodyAggregate::Element>(elem.loc(), elem.tuple_, tr(elem.cond_));
    }

    [[nodiscard]] auto accept(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        if (lit.sign_ != Sign::none || in_classical_scope || !reduct_is_nonmonotone(lit.lhs_, lit.fun_, lit.rhs_)) {
            return transform_construct<BodyAggregate>(lit.loc(), lit.sign_, lit.lhs_, lit.fun_, tr(lit.elems_),
                                                      lit.rhs_);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto accept(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        if (lit.sign() == Sign::none && !in_classical_scope &&
            reduct_is_nonmonotone(lit.lhs_, AggregateFunction::count, lit.rhs_)) {
            return std::nullopt;
        }
        return transform_construct<BodySetAggregate>(lit.loc(), lit.sign(), lit.lhs_, tr(lit.elems_), lit.rhs_);
    }

    [[nodiscard]] static auto accept(BodyTheoryAtom const &lit) -> std::optional<BodyLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    // statement

    [[nodiscard]] auto accept(Rule const &stm) const -> std::optional<Statement> {
        // do not project projection-like rules
        if (is_atom(stm.head_)) {
            auto has_atom =
                std::any_of(stm.body_.begin(), stm.body_.end(), [](auto const &lit) { return is_atom(lit); });
            size_t n_test =
                std::count_if(stm.body_.begin(), stm.body_.end(), [](auto const &lit) { return is_test(lit); });
            if (has_atom && n_test == stm.body_.size() - 1) {
                return std::nullopt;
            }
        }
        bool in_classical_scope = is_classical(stm.head_);
        auto sub_project = Project{project, in_classical_scope};
        // Note that it would be nicest to be able to have two different
        // translators for head and body because the scope setting would
        // ideally just apply to the body. In the current implementation, the
        // head literals simply set the scope themselves.
        return sub_project.transform_construct<Rule>(stm.loc(), tr(stm.head_), tr(stm.body_));
    }

    [[nodiscard]] auto accept(StatementOptimize::Element const &elem) const
        -> std::optional<StatementOptimize::Element> {
        auto counts = get_counts(project, elem);
        auto sub_project = Project{ProjectionMap{project.mode(), counts}};
        return sub_project.transform_construct<StatementOptimize::Element>(elem.first, tr(elem.second));
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

auto project(Literal const &lit, ProjectionMap project) -> std::optional<Literal> {
    return Project{project}.transform(lit);
}

auto project(HeadLiteral const &lit, ProjectionMap project) -> std::optional<HeadLiteral> {
    return Project{project}.transform(lit);
}

auto project(BodyLiteral const &lit, ProjectionMap project, bool in_classical_scope) -> std::optional<BodyLiteral> {
    return Project{project, in_classical_scope}.transform(lit);
}

auto project(Statement const &stm, ProjectionMode mode, bool project_anonymous) -> std::optional<Statement> {
    std::optional<Statement> res;
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
