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

struct TermProjectAnonymous {
    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    auto operator()(Term const &term) const -> std::optional<Term> { return std::visit(*this, term); }

    auto operator()(TupleElem const &elem) const -> std::optional<TupleElem> {
        if (is_anonymous(std::get_if<Term>(&elem))) {
            return {std::monostate{}};
        }
        return transform(*this, elem);
    };

    auto operator()(TermSymbol const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermTuple> const &term) const -> std::optional<Term> {
        return transform_construct_shared<TermTuple>(tr(term->pool));
    }

    auto operator()(Util::shared_ptr<TermFunction> const &term) const -> std::optional<Term> {
        if (term->external) {
            return std::nullopt;
        }
        return transform_construct_shared<TermFunction>(term->name, tr(term->pool), term->external);
    }

    auto operator()(Util::shared_ptr<TermAbs> const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermUnary> const &term) const -> std::optional<Term> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct_shared<TermUnary>(term->op, tr(term->rhs));
        }
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermBinary> const &term) const -> std::optional<Term> {
        static_cast<void>(term);
        return std::nullopt;
    }
};

} // namespace

[[nodiscard]] auto project_anonymous(Term const &term) -> std::optional<Term> {
    return std::visit(TermProjectAnonymous{}, term);
}

} // namespace Gringo::Input
