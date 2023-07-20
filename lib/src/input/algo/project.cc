#include <unordered_map>

#include <input/algo/check_type.hh>
#include <input/algo/project.hh>
#include <input/algo/project_anonymous.hh>
#include <input/algo/visit_variables.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto projectable(Projection project, Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && project.projectable(var->name, var->is_anonymous);
}

auto get_counts(Projection project, auto const &elem) {
    std::unordered_map<std::string, size_t> counts;
    visit_variables(elem, [&project, &counts](auto const &var) {
        if (!project.counts().contains(var)) {
            ++counts[var];
        }
    });
    return counts;
}

struct Project : Transformer<Project> {

    Project(Projection project, bool in_classical_scope = true, bool project_lits = true)
        : project{std::move(project)}, in_classical_scope{in_classical_scope}, project_lits{project_lits} {}

    // protect ourselves -> no unintended overloads

    template <class T> auto operator()(T const &x) const -> std::optional<T> = delete;

    // ignore

    auto operator()(std::string const &x) const -> std::optional<std::string> {
        static_cast<void>(x);
        return std::nullopt;
    }

    template <class T> auto operator()(T const &x) const -> std::enable_if_t<std::is_enum_v<T>, std::optional<T>> {
        static_cast<void>(x);
        return std::nullopt;
    }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(std::monostate const &x) const -> std::optional<Term> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (projectable(project, std::get_if<Term>(&elem))) {
            return {std::monostate{}};
        }
        // Note: a tiny bit lazy. Because monostate always maps to nullopt, we
        // can safely convert the resulting optional term back into a tuple
        // elem.
        return std::visit(*this, elem);
    };

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermTuple const &term) const -> std::optional<Term> {
        return transform_construct<TermTuple>(tr(term.pool));
    }

    auto operator()(TermFunction const &term) const -> std::optional<Term> {
        if (term.external) {
            return std::nullopt;
        }
        return transform_construct<TermFunction>(term.name, tr(term.pool), term.external);
    }

    auto operator()(TermAbs const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermUnary const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct<TermUnary>(term.op, tr(term.rhs));
        }
        return std::nullopt;
    }

    auto operator()(TermBinary const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    // literal

    auto operator()(Literal const &lit) const -> std::optional<Literal> { return std::visit(*this, lit); }

    auto operator()(LiteralRelation const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralBoolean const &lit) const -> std::optional<Literal> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit) const -> std::optional<Literal> {
        if (lit.sign == Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.sign, tr(lit.term));
        }
        return std::nullopt;
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &elem) const -> std::optional<ConditionalLiteral> {
        auto const &[lits, cond] = elem;
        bool project_cond =
            in_classical_scope || std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return !is_atom(lit); });
        // project conclusion
        std::optional<LiteralVec> projected_lits = std::nullopt;
        if (project_lits) {
            projected_lits = transform(lits);
        }
        // project premise
        std::optional<LiteralVec> projected_cond = std::nullopt;
        if (project_cond) {
            // add counts of local variables
            auto counts = get_counts(project, elem);
            auto sub_project = Project{Projection{project.mode(), counts}};

            // Note that there can be no global variables with just one
            // occurrence in a condition. However, we can project local
            // variables.
            projected_cond = sub_project.transform(cond);
        }
        if (projected_lits.has_value() || projected_cond.has_value()) {
            return ConditionalLiteral{std::move(projected_lits).value_or(lits),
                                      std::move(projected_cond).value_or(cond)};
        }
        return std::nullopt;
    }

    // aggregate

    auto operator()(SetAggregate::Element const &elem) const -> std::optional<SetAggregate::Element> {
        // add counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{Projection{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<SetAggregate::Element>(elem.lit, tr(elem.cond));
    }

    auto operator()(SetAggregate const &aggr) const -> std::optional<SetAggregate> {
        if (!in_classical_scope && reduct_is_nonmonotone(aggr.lhs, AggregateFunction::count, aggr.rhs)) {
            return std::nullopt;
        }
        return transform_construct<SetAggregate>(aggr.lhs, tr(aggr.elems), aggr.rhs);
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        // Note when to project:
        // - variables in conditions (almost body literals)
        auto sub_project = Project{project, true, false};
        return sub_project.transform_construct<Disjunction>(tr(lit.elems));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{Projection{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<HeadAggregate::Element>(elem.tuple, elem.lit, tr(elem.cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        // Note that we can always project in conditions. Semantic-wise a head
        // aggregate is a shortcut for a choice rule + a body aggregate in an
        // integrity constraint.
        auto sub_project = Project{project, true, true};
        return sub_project.transform_construct<HeadSetAggregate>(tr(lit.aggr));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteral> {
        // Note when to project:
        // - variables in premise if in classical scope,
        // - varibales in conclusion.
        auto sub_project = Project{project, in_classical_scope, true};
        return sub_project.transform_construct<Conjunction>(tr(lit.elems));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        // counts of local variables
        auto counts = get_counts(project, elem);
        auto sub_project = Project{Projection{project.mode(), counts}};

        // project literals in condition
        return sub_project.transform_construct<BodyAggregate::Element>(elem.tuple, tr(elem.cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        if (lit.sign != Sign::none || in_classical_scope || !reduct_is_nonmonotone(lit.lhs, lit.fun, lit.rhs)) {
            return transform_construct<BodyAggregate>(lit.sign, lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
        }
        return std::nullopt;
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        auto sub_project = Project{project, in_classical_scope || lit.sign != Sign::none};
        return sub_project.transform_construct<BodySetAggregate>(lit.sign, tr(lit.aggr));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<Statement> { return std::visit(*this, stm); }

    auto operator()(Rule const &stm) const -> std::optional<Statement> {
        // do not project projection-like rules
        if (is_atom(stm.head)) {
            auto has_atom = std::any_of(stm.body.begin(), stm.body.end(), [](auto const &lit) { return is_atom(lit); });
            size_t n_test =
                std::count_if(stm.body.begin(), stm.body.end(), [](auto const &lit) { return is_test(lit); });
            if (has_atom && n_test == stm.body.size() - 1) {
                return std::nullopt;
            }
        }
        bool in_classical_scope = is_classical(stm.head);
        auto sub_project = Project{project, in_classical_scope};
        // Note that it would be nicest to be able to have to different
        // translators for head and body because the scope setting would
        // ideally just apply to the body. In the current implementation, the
        // head literals simply set the scope themselves.
        return sub_project.transform_construct<Rule>(tr(stm.head), tr(stm.body));
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementOptimize::Element const &elem) const -> std::optional<StatementOptimize::Element> {
        auto counts = get_counts(project, elem);
        auto sub_project = Project{Projection{project.mode(), counts}};
        return sub_project.transform_construct<StatementOptimize::Element>(elem.first, tr(elem.second));
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementOptimize>(stm.type, tr(stm.elems));
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementWeakConstraint>(tr(stm.body), stm.tuple);
    }

    auto operator()(StatementShow const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementShow>(stm.term, tr(stm.body));
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<Statement> {
        static_cast<void>(stm);
        return std::nullopt;
    }

    auto operator()(StatementProject const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementProject>(stm.term, tr(stm.body));
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
        return transform_construct<StatementExternal>(stm.term, tr(stm.body), stm.type);
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementEdge>(stm.edges, tr(stm.body));
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<Statement> {
        return transform_construct<StatementHeuristic>(stm.atom, tr(stm.body), stm.type, stm.prio, stm.mod);
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

    Projection project;
    bool in_classical_scope;
    bool project_lits;
};

} // namespace

auto Projection::projectable(std::string const &var, bool anonymous) const -> bool {
    if (mode_ == ProjectionMode::disabled) {
        return false;
    }
    if (mode_ == ProjectionMode::anonymous && !anonymous) {
        return false;
    }
    auto it = counts_.find(var);
    return it != counts_.end() && it->second == 1;
}

auto Projection::counts() const -> std::unordered_map<std::string, size_t> const & { return counts_; }

auto Projection::mode() const -> ProjectionMode { return mode_; }

auto project(Term const &term, Projection project) -> std::optional<Term> { return Project{project}(term); }

auto project(Literal const &lit, Projection project) -> std::optional<Literal> { return Project{project}(lit); }

auto project(HeadLiteral const &lit, Projection project) -> std::optional<HeadLiteral> { return Project{project}(lit); }

auto project(BodyLiteral const &lit, Projection project, bool in_classical_scope) -> std::optional<BodyLiteral> {
    return Project{project, in_classical_scope}(lit);
}

auto project(Statement const &stm, ProjectionMode mode, bool project_anonymous) -> std::optional<Statement> {
    std::optional<Statement> res;
    if (mode != ProjectionMode::disabled) {
        VariableSet vars = select_variables(stm, VariableContext::global);
        std::unordered_map<std::string, size_t> counts;
        counts.reserve(vars.size());
        visit_variables(
            stm,
            [&vars, &counts](auto const &var) {
                if (vars.contains(var)) {
                    ++counts[var];
                }
            },
            VariableContext::all);

        res = Project{Projection{mode, counts}}(stm);
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
