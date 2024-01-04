#include <gringo/input/term.hh>

namespace Gringo::Input {

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

auto operator==(Projection const &a, Projection const &b) -> bool {
    static_cast<void>(a);
    static_cast<void>(b);
    return true;
};

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TermVariable>::operator()(Gringo::Input::TermVariable const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name);
}

auto value_hasher<Gringo::Input::TermSymbol>::operator()(Gringo::Input::TermSymbol const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermSymbol), x.value);
}

auto value_hasher<Gringo::Input::TermTuple>::operator()(Gringo::Input::TermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermTuple), x.pool);
}

auto value_hasher<Gringo::Input::TermFunction>::operator()(Gringo::Input::TermFunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermFunction), x.name, x.pool, x.external);
}

auto value_hasher<Gringo::Input::TermAbs>::operator()(Gringo::Input::TermAbs const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermAbs), x.pool);
}

auto value_hasher<Gringo::Input::TermUnary>::operator()(Gringo::Input::TermUnary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermUnary), x.op, x.rhs);
}

auto value_hasher<Gringo::Input::TermBinary>::operator()(Gringo::Input::TermBinary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermBinary), x.op, x.lhs, x.rhs);
}

auto value_hasher<Gringo::Input::Projection>::operator()(Gringo::Input::Projection const &x) const -> size_t {
    static_cast<void>(x);
    return Gringo::Util::value_hash(typeid(Gringo::Input::Projection));
}

} // namespace Gringo::Util
