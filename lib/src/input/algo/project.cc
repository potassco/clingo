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

struct TermProject {

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
    return std::visit(TermProject{project}, term);
}

} // namespace Gringo::Input
