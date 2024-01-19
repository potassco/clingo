#include <gringo/input/head_literal.hh>

namespace Gringo::Input {

auto operator==(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool { return a.lit == b.lit; }

auto operator<(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool { return a.lit < b.lit; }

auto operator==(Disjunction const &a, Disjunction const &b) -> bool { return a.elems == b.elems; }

auto operator<(Disjunction const &a, Disjunction const &b) -> bool { return a.elems < b.elems; }

auto operator==(HeadAggregate::Element const &a, HeadAggregate::Element const &b) -> bool {
    return std::tie(a.tuple, a.lit, a.cond) == std::tie(b.tuple, b.lit, b.cond);
}

auto operator<(HeadAggregate::Element const &a, HeadAggregate::Element const &b) -> bool {
    return std::tie(a.tuple, a.lit, a.cond) < std::tie(b.tuple, b.lit, b.cond);
}

auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool {
    return std::tie(a.fun, a.lhs, a.elems, a.rhs) == std::tie(b.fun, b.lhs, b.elems, b.rhs);
}

auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool {
    return std::tie(a.fun, a.lhs, a.elems, a.rhs) < std::tie(b.fun, b.lhs, b.elems, b.rhs);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SimpleHeadLiteral>::operator()(Gringo::Input::SimpleHeadLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SimpleHeadLiteral), x.lit);
}

auto value_hasher<Gringo::Input::Disjunction>::operator()(Gringo::Input::Disjunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Disjunction), x.elems);
}

auto value_hasher<Gringo::Input::HeadAggregate::Element>::operator()(
    Gringo::Input::HeadAggregate::Element const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HeadAggregate::Element), x.tuple, x.cond);
}

auto value_hasher<Gringo::Input::HeadAggregate>::operator()(Gringo::Input::HeadAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HeadAggregate), x.fun, x.lhs, x.elems, x.rhs);
}

} // namespace Gringo::Util
