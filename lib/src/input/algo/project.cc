#include <input/algo/check_type.hh>
#include <input/algo/project.hh>
#include <input/algo/visit_variables.hh>

#include "transform.hh"
#include "variables.hh"

namespace Gringo::Input {

namespace {

auto projectable(Projection project, Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && project.projectable(var->name, var->is_anonymous);
}

struct Project {

    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(std::monostate x) const -> std::optional<Term> {
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
            projected_lits = transform(*this, lits);
        }
        // project premise
        std::optional<LiteralVec> projected_cond = std::nullopt;
        if (project_cond) {
            // add counts of local variables
            VarCounter counter{project.counts()};
            counter.add(lits);
            counter.add(cond);
            auto sub_project = Project{Projection{project.mode(), counter}};

            // Note that there can be no global variables with just one
            // occurrence in a condition. However, we can project local
            // variables.
            projected_cond = transform(sub_project, cond);
        }
        if (projected_lits.has_value() || projected_cond.has_value()) {
            return ConditionalLiteral{std::move(projected_lits).value_or(lits),
                                      std::move(projected_cond).value_or(cond)};
        }
        return std::nullopt;
    }

    // aggregate

    auto operator()(SetAggregate::Element const &elem) const -> std::optional<SetAggregate::Element> {
        auto const &[lit, cond] = elem;

        // add counts of local variables
        VarCounter counter{project.counts()};
        counter.add(lit);
        counter.add(cond);
        auto sub_project = Project{Projection{project.mode(), counter}};

        // project literals in condition
        return transform_construct<SetAggregate::Element>(lit, sub_project.tr(cond));
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
        return transform_construct<Disjunction>(sub_project.tr(lit.elems));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(HeadAggregate::Element const &elem) const -> std::optional<HeadAggregate::Element> {
        auto const &[tuple, lit, cond] = elem;

        // counts of local variables
        VarCounter counter{project.counts()};
        counter.add(tuple, lit, cond);
        auto sub_project = Project{Projection{project.mode(), counter}};

        // project literals in condition
        return transform_construct<HeadAggregate::Element>(tuple, lit, sub_project.tr(cond));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        // Note that we can always project in conditions. Semantic-wise a head
        // aggregate is a shortcut for a choice rule + a body aggregate in an
        // integrity constraint.
        auto sub_project = Project{project, true};
        return transform_construct<HeadSetAggregate>(sub_project.tr(lit.aggr));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteral> {
        // Note when to project:
        // - variables in premise if in classical scope,
        // - varibales in conclusion.
        auto sub_project = Project{project, in_classical_scope, true};
        return transform_construct<Conjunction>(sub_project.tr(lit.elems));
    }

    auto operator()(BodyAggregate::Element const &elem) const -> std::optional<BodyAggregate::Element> {
        auto const &[lit, cond] = elem;

        // counts of local variables
        VarCounter counter{project.counts()};
        counter.add(lit, cond);
        auto sub_prj = Project{Projection{project.mode(), counter}};

        // project literals in condition
        return transform_construct<BodyAggregate::Element>(lit, sub_prj.tr(cond));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        if (lit.sign != Sign::none || in_classical_scope || !reduct_is_nonmonotone(lit.lhs, lit.fun, lit.rhs)) {
            return transform_construct<BodyAggregate>(lit.sign, lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
        }
        return std::nullopt;
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        auto sub_project = Project{project, in_classical_scope || lit.sign != Sign::none};
        return transform_construct<BodySetAggregate>(sub_project.tr(lit.aggr));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        static_cast<void>(lit);
        return std::nullopt;
    }

    Projection project;
    bool in_classical_scope = true;
    bool project_lits = true;
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

} // namespace Gringo::Input
