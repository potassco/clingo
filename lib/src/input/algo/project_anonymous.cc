#include <input/algo/check_type.hh>
#include <input/algo/project_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto is_anonymous(Term const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && var->is_anonymous;
}

struct ProjectAnonymous {
    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    // term

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(std::monostate x) const -> std::optional<Term> {
        static_cast<void>(x);
        return std::nullopt;
    }

    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (is_anonymous(std::get_if<Term>(&elem))) {
            return {std::monostate{}};
        }
        // Note: a tiny bit lazy. Because monostate always maps to nullopt, we
        // can safely convert the resulting optional term back into a tuple
        // elem.
        return std::visit(*this, elem);
    };

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
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
        if (lit.sign != Sign::none) {
            return transform_construct<LiteralSymbolic>(lit.sign, tr(lit.term));
        }
        return std::nullopt;
    }

    // conditional literal

    auto operator()(ConditionalLiteral const &lit) const -> std::optional<ConditionalLiteral> {
        return transform_construct<ConditionalLiteral>(tr(lit.lits), tr(lit.lits));
    }

    // aggregate

    auto operator()(SetAggregate::Element const &elem) const -> std::optional<SetAggregate::Element> {
        return transform_construct<SetAggregate::Element>(tr(elem.lit), tr(elem.cond));
    }

    auto operator()(SetAggregate const &aggr) const -> std::optional<SetAggregate> {
        return transform_construct<SetAggregate>(aggr.lhs, tr(aggr.elems), aggr.rhs);
    }

    // theory

    auto operator()(TheoryAtom const &atom) const -> std::optional<TheoryAtom> {
        return transform_construct<TheoryAtom>(atom.name, tr(atom.elems), atom.rhs);
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteral> { return std::visit(*this, lit); }

    auto operator()(Disjunction const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<Disjunction>(tr(lit.elems));
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadAggregate>(lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(HeadSetAggregate const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadSetAggregate>(tr(lit.aggr));
    }

    auto operator()(HeadTheoryAtom const &lit) const -> std::optional<HeadLiteral> {
        return transform_construct<HeadTheoryAtom>(tr(lit.atom));
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteral> { return std::visit(*this, lit); }

    auto operator()(Conjunction const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<Conjunction>(tr(lit.elems));
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyAggregate>(lit.sign, lit.lhs, lit.fun, tr(lit.elems), lit.rhs);
    }

    auto operator()(BodySetAggregate const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodySetAggregate>(lit.sign, tr(lit.aggr));
    }

    auto operator()(BodyTheoryAtom const &lit) const -> std::optional<BodyLiteral> {
        return transform_construct<BodyTheoryAtom>(lit.sign, tr(lit.atom));
    }
};

} // namespace

auto project_anonymous(Term const &term) -> std::optional<Term> { return ProjectAnonymous{}(term); }

auto project_anonymous(Literal const &lit) -> std::optional<Literal> { return ProjectAnonymous{}(lit); }

auto project_anonymous(HeadLiteral const &lit) -> std::optional<HeadLiteral> { return ProjectAnonymous{}(lit); }

auto project_anonymous(BodyLiteral const &lit) -> std::optional<BodyLiteral> { return ProjectAnonymous{}(lit); }

} // namespace Gringo::Input
