#include <gringo/input/aggregate.hh>

namespace Gringo::Input {

auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
    return std::tie(a.lit, a.cond) == std::tie(a.lit, b.cond);
}

auto operator<(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
    return std::tie(a.lit, a.cond) < std::tie(a.lit, b.cond);
}

template <> auto operator==(SetAggregate<true> const &a, SetAggregate<true> const &b) -> bool {
    return std::tie(a.sign, a.lhs, a.elems, a.rhs) == std::tie(b.sign, b.lhs, b.elems, b.rhs);
}

template <> auto operator<(SetAggregate<true> const &a, SetAggregate<true> const &b) -> bool {
    return std::tie(a.sign, a.lhs, a.elems, a.rhs) < std::tie(b.sign, b.lhs, b.elems, b.rhs);
}

template <> auto operator==(SetAggregate<false> const &a, SetAggregate<false> const &b) -> bool {
    return std::tie(a.lhs, a.elems, a.rhs) == std::tie(b.lhs, b.elems, b.rhs);
}

template <> auto operator<(SetAggregate<false> const &a, SetAggregate<false> const &b) -> bool {
    return std::tie(a.lhs, a.elems, a.rhs) < std::tie(b.lhs, b.elems, b.rhs);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SetAggregateElement>::operator()(Gringo::Input::SetAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregateElement), x.lit, x.cond);
}

auto value_hasher<Gringo::Input::SetAggregate<true>>::operator()(Gringo::Input::SetAggregate<true> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregate<true>), x.sign, x.lhs, x.elems, x.rhs);
}

auto value_hasher<Gringo::Input::SetAggregate<false>>::operator()(Gringo::Input::SetAggregate<false> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregate<false>), x.lhs, x.elems, x.rhs);
}

} // namespace Gringo::Util
