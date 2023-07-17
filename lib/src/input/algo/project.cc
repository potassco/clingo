#include <input/algo/check_type.hh>
#include <input/algo/project.hh>

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

struct Project {

    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

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

    template <class T, class B, class P>
    auto project(typename T::ElementVec const &elems, P prj, bool project_lits, bool in_classical_scope)
        -> std::optional<Util::shared_ptr<B>> {
        using Gringo::Input::is_atom;
        using Gringo::Input::project;
        using Elem = typename T::ElementVec::value_type;
        auto fun = [&](Elem const &elem) -> std::optional<Elem> {
            auto const &[lits, cond] = elem;
            bool project_cond = in_classical_scope ||
                                std::all_of(lits.begin(), lits.end(), [](auto const &lit) { return !is_atom(lit); });
            // project conclusion
            std::optional<LiteralVec> projected_lits = std::nullopt;
            if (project_lits) {
                auto fun = [prj](Literal const &lit) { return project(lit, prj); };
                projected_lits = transform(fun, lits);
            }
            // project premise
            std::optional<LiteralVec> projected_cond = std::nullopt;
            if (project_cond) {
                // add counts of local variables
                VarCounter counter{prj.counts()};
                counter.add(lits);
                counter.add(cond);
                // Note that there can be no global variables with just one
                // occurrence in a condition. However, we can project local
                // variables.
                auto sub_prj = Projection{prj.mode(), counter};
                auto fun = [sub_prj](Literal const &lit) { return project(lit, sub_prj); };
                projected_cond = transform(fun, cond);
            }
            if (projected_lits.has_value() || projected_cond.has_value()) {
                return Elem{std::move(projected_lits).value_or(lits), std::move(projected_cond).value_or(cond)};
            }
            return std::nullopt;
        };
        return transform_construct_shared<T, B>(Trans{elems, fun});
    }

    // aggregate

    auto SetAggregate::project(Projection prj, bool in_negative_scope) const -> std::optional<SetAggregate> {
        using Gringo::Input::project;
        if (!in_negative_scope && reduct_is_nonmonotone(lhs_, AggregateFunction::count, rhs_)) {
            return std::nullopt;
        }
        auto fun = [prj](Element const &elem) -> std::optional<Element> {
            auto const &[lit, cond] = elem;

            // add counts of local variables
            VarCounter counter{prj.counts()};
            counter.add(lit);
            counter.add(cond);
            auto sub_project = Projection{prj.mode(), counter};

            // project literals in condition
            auto fun = [sub_project](Literal const &lit) { return project(lit, sub_project); };
            return transform_construct<Element>(lit, Trans(cond, fun));
        };
        return transform_construct<SetAggregate>(lhs_, Trans{elems_, fun}, rhs_);
    }

    // head literal

    auto Disjunction::project(Projection project) const -> std::optional<SHeadLiteral> {
        // Note when to project:
        // - variables in conditions (almost body literals)
        return CondLits::project<Disjunction, HeadLiteral>(elems_, project, false, true);
    }

    auto HeadTheoryAtom::project(Projection project) const -> std::optional<SHeadLiteral> {
        static_cast<void>(project);
        return std::nullopt;
    }

    auto HeadAggregate::project(Projection prj) const -> std::optional<SHeadLiteral> {
        using Gringo::Input::project;
        auto fun = [prj](Element const &elem) -> std::optional<Element> {
            auto const &[tuple, lit, cond] = elem;

            // counts of local variables
            VarCounter counter{prj.counts()};
            counter.add(tuple, lit, cond);
            auto sub_project = Projection{prj.mode(), counter};

            // project literals in condition
            auto fun = [sub_project](Literal const &lit) { return project(lit, sub_project); };
            return transform_construct<Element>(tuple, lit, Trans(cond, fun));
        };
        return transform_construct_shared<HeadAggregate, HeadLiteral>(lhs_, fun_, Trans{elems_, fun}, rhs_);
    }

    auto HeadSetAggregate::project(Projection project) const -> std::optional<SHeadLiteral> {
        // Note that we can always project in conditions. Semantic-wise a head
        // aggregate is a shortcut for a choice rule + a body aggregate in an
        // integrity constraint.
        auto projected = aggr_.project(project, true);
        if (projected.has_value()) {
            return Util::construct_shared<HeadSetAggregate, HeadLiteral>(std::move(projected).value());
        }
        return std::nullopt;
    }

    // body literal

    auto Conjunction::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
        // Note when to project:
        // - variables in premise if in classical scope,
        // - varibales in conclusion.
        return CondLits::project<Conjunction, BodyLiteral>(elems_, project, true, in_classical_scope);
    }

    auto BodyAggregate::project(Projection prj, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
        using Gringo::Input::project;
        if (sign_ == Sign::none && !in_classical_scope && reduct_is_nonmonotone(lhs_, fun_, rhs_)) {
            return std::nullopt;
        }

        auto fun = [prj](Element const &elem) -> std::optional<Element> {
            auto const &[lit, cond] = elem;

            // counts of local variables
            VarCounter counter{prj.counts()};
            counter.add(lit, cond);
            auto sub_prj = Projection{prj.mode(), counter};

            // project literals in condition
            auto fun = [sub_prj](Literal const &lit) { return project(lit, sub_prj); };
            return transform_construct<Element>(lit, Trans(cond, fun));
        };
        return transform_construct_shared<BodyAggregate, BodyLiteral>(sign_, lhs_, fun_, Trans{elems_, fun}, rhs_);
    }

    auto BodySetAggregate::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
        auto projected = aggr_.project(project, in_classical_scope || sign_ != Sign::none);
        if (projected.has_value()) {
            return Util::construct_shared<BodySetAggregate, BodyLiteral>(sign_, std::move(projected).value());
        }
        return std::nullopt;
    }

    auto BodyTheoryAtom::project(Projection project, bool in_classical_scope) const -> std::optional<SBodyLiteral> {
        static_cast<void>(project);
        static_cast<void>(in_classical_scope);
        return std::nullopt;
    }

    Projection project;
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

[[nodiscard]] auto Projection::counts() const -> std::unordered_map<std::string, size_t> const & { return counts_; }

auto Projection::mode() const -> ProjectionMode { return mode_; }

[[nodiscard]] auto project(Term const &term, Projection project) -> std::optional<Term> {
    return std::visit(Project{project}, term);
}

[[nodiscard]] auto project(Literal const &lit, Projection project) -> std::optional<Literal> {
    return std::visit(Project{project}, lit);
}

} // namespace Gringo::Input
