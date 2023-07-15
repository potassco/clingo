#include <input/algo/check_type.hh>
#include <input/algo/project.hh>

#include "transform.hh"

namespace Gringo::Input {

namespace {

auto projectable(Projection project, TermV2 const *term) -> bool {
    if (term == nullptr) {
        return false;
    }
    auto const *var = std::get_if<TermVariable>(term);
    return var != nullptr && project.projectable(var->name, var->is_anonymous);
}

struct TermProject {

    [[nodiscard]] auto tr(auto const &x) const { return Trans(x, *this); }

    auto operator()(TermV2 const &term) const -> std::optional<TermV2> { return std::visit(*this, term); }

    auto operator()(TupleElemV2 const &elem) const -> std::optional<TupleElemV2> {
        if (projectable(project, std::get_if<TermV2>(&elem))) {
            return {std::monostate{}};
        }
        return transform(*this, elem);
    };

    auto operator()(TermVariable const &term) const -> std::optional<TermV2> {
        static_cast<void>(term);
        return std::nullopt;
    }

    auto operator()(TermSymbol const &term) const -> std::optional<TermV2> {
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

[[nodiscard]] auto project(TermV2 const &term, Projection project) -> std::optional<TermV2> {
    return std::visit(TermProject{project}, term);
}

} // namespace Gringo::Input
