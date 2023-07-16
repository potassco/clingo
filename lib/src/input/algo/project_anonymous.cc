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
};

} // namespace

[[nodiscard]] auto project_anonymous(Term const &term) -> std::optional<Term> {
    return std::visit(ProjectAnonymous{}, term);
}

[[nodiscard]] auto project_anonymous(Literal const &lit) -> std::optional<Literal> {
    return std::visit(ProjectAnonymous{}, lit);
}

} // namespace Gringo::Input
