#include <cmath>
#include <optional>
#include <tuple>
#include <utility>

#include <input/term.hh>

// TODO: remove
#include <input/algo/check_type.hh>
#include <input/algo/project.hh>
#include <input/algo/project_anonymous.hh>

namespace Gringo::Input {

// TODO: move/remove this

auto Term::check_type(TermCheckType type, CheckTypeResult *res) const -> bool {
    return Gringo::Input::check_type(term, type, res);
}

auto Term::is_equal(Term const &other) const -> bool { return value_equal(term, other.term); }

auto Term::project(Projection project) const -> std::optional<STerm> {
    if (auto x = Gringo::Input::project(term, project); x.has_value()) {
        return construct_shared<Term>(std::move(x).value());
    }
    return std::nullopt;
}

auto Term::project_anonymous() const -> std::optional<STerm> {
    if (auto x = Gringo::Input::project_anonymous(term); x.has_value()) {
        return construct_shared<Term>(std::move(x).value());
    }
    return std::nullopt;
}

// until here

auto operator==(TermVariable const &a, TermVariable const &b) -> bool { return Util::value_equal(a.name, b.name); }

auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool { return Util::value_equal(a.value, b.value); }

auto operator==(TermTuple const &a, TermTuple const &b) -> bool { return Util::value_equal(a.pool, b.pool); }

auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return Util::value_equal(a.name, b.name, a.pool, b.pool, a.external, b.external);
}

auto operator==(TermAbs const &a, TermAbs const &b) -> bool { return Util::value_equal(a.pool, b.pool); }

auto operator==(TermUnary const &a, TermUnary const &b) -> bool { return Util::value_equal(a.op, b.op, a.rhs, b.rhs); };

auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return Util::value_equal(a.op, b.op, a.lhs, b.lhs, a.rhs, b.rhs);
};

} // namespace Gringo::Input

namespace std {

auto hash<Gringo::Input::TermVariable>::operator()(Gringo::Input::TermVariable const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name);
}

auto hash<Gringo::Input::TermSymbol>::operator()(Gringo::Input::TermSymbol const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermSymbol), x.value);
}

auto hash<Gringo::Input::TermTuple>::operator()(Gringo::Input::TermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermTuple), x.pool);
}

auto hash<Gringo::Input::TermFunction>::operator()(Gringo::Input::TermFunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermFunction), x.name, x.pool, x.external);
}

auto hash<Gringo::Input::TermAbs>::operator()(Gringo::Input::TermAbs const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermAbs), x.pool);
}

auto hash<Gringo::Input::TermUnary>::operator()(Gringo::Input::TermUnary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermUnary), x.op, x.rhs);
}

auto hash<Gringo::Input::TermBinary>::operator()(Gringo::Input::TermBinary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermBinary), x.op, x.lhs, x.rhs);
}

} // namespace std
