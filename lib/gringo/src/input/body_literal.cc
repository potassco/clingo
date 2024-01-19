#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

auto operator==(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool { return a.lit == b.lit; }

auto operator<(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool { return a.lit < b.lit; }

auto operator==(Conjunction const &a, Conjunction const &b) -> bool { return a.lit == b.lit; }

auto operator<(Conjunction const &a, Conjunction const &b) -> bool { return a.lit < b.lit; }

auto operator==(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool {
    return std::tie(a.tuple, a.cond) == std::tie(b.tuple, b.cond);
}

auto operator<(BodyAggregate::Element const &a, BodyAggregate::Element const &b) -> bool {
    return std::tie(a.tuple, a.cond) < std::tie(b.tuple, b.cond);
}

auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool {
    return std::tie(a.sign, a.fun, a.lhs, a.elems, a.rhs) == std::tie(b.sign, b.fun, b.lhs, b.elems, b.rhs);
}

auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool {
    return std::tie(a.sign, a.fun, a.lhs, a.elems, a.rhs) < std::tie(b.sign, b.fun, b.lhs, b.elems, b.rhs);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SimpleBodyLiteral>::operator()(Gringo::Input::SimpleBodyLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SimpleBodyLiteral), x.lit);
}

auto value_hasher<Gringo::Input::Conjunction>::operator()(Gringo::Input::Conjunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Conjunction), x.lit);
}

auto value_hasher<Gringo::Input::BodyAggregate::Element>::operator()(
    Gringo::Input::BodyAggregate::Element const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BodyAggregate::Element), x.tuple, x.cond);
}

auto value_hasher<Gringo::Input::BodyAggregate>::operator()(Gringo::Input::BodyAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BodyAggregate), x.sign, x.fun, x.lhs, x.elems, x.rhs);
}

} // namespace Gringo::Util
