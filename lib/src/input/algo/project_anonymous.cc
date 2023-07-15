#include <input/algo/check_type.hh>
#include <input/algo/project_anonymous.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto is_anonymous(TermV2 const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && var->is_anonymous;
}

struct TermProjectAnonymous {
    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    auto operator()(TermV2 const &term) const -> std::optional<TermV2> { return std::visit(*this, term); }

    auto operator()(TupleElemV2 const &elem) const -> std::optional<TupleElemV2> {
        if (is_anonymous(std::get_if<TermV2>(&elem))) {
            return {std::monostate{}};
        }
        return transform(*this, elem);
    };

    auto operator()(TermSymbol const &term) const -> std::optional<TermV2> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermVariable const &term) const -> std::optional<TermV2> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermTuple> const &term) const -> std::optional<TermV2> {
        return transform_construct_shared<TermTuple>(tr(term->pool));
    }

    auto operator()(Util::shared_ptr<TermFunction> const &term) const -> std::optional<TermV2> {
        if (term->external) {
            return std::nullopt;
        }
        return transform_construct_shared<TermFunction>(term->name, tr(term->pool), term->external);
    }

    auto operator()(Util::shared_ptr<TermAbs> const &term) const -> std::optional<TermV2> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermUnary> const &term) const -> std::optional<TermV2> {
        if (check_type(term, TermCheckType::atom, nullptr)) {
            return transform_construct_shared<TermUnary>(term->op, tr(term->rhs));
        }
        return std::nullopt;
    }

    auto operator()(Util::shared_ptr<TermBinary> const &term) const -> std::optional<TermV2> {
        static_cast<void>(term);
        return std::nullopt;
    }
};

} // namespace

[[nodiscard]] auto project_anonymous(TermV2 const &term) -> std::optional<TermV2> {
    return std::visit(TermProjectAnonymous{}, term);
}

} // namespace Gringo::Input
