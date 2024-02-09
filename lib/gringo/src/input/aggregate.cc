#include <gringo/input/aggregate.hh>

namespace Gringo::Input {

auto operator==(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
    return std::tie(a.lit_, a.cond_) == std::tie(a.lit_, b.cond_);
}

auto operator<(SetAggregateElement const &a, SetAggregateElement const &b) -> bool {
    return std::tie(a.lit_, a.cond_) < std::tie(a.lit_, b.cond_);
}

template <> auto operator==(SetAggregate<true> const &a, SetAggregate<true> const &b) -> bool {
    return std::tie(static_cast<Signed const &>(a), a.lhs_, a.elems_, a.rhs_) ==
           std::tie(static_cast<Signed const &>(b), b.lhs_, b.elems_, b.rhs_);
}

template <> auto operator<(SetAggregate<true> const &a, SetAggregate<true> const &b) -> bool {
    return std::tie(static_cast<Signed const &>(a), a.lhs_, a.elems_, a.rhs_) <
           std::tie(static_cast<Signed const &>(b), b.lhs_, b.elems_, b.rhs_);
}

template <> auto operator==(SetAggregate<false> const &a, SetAggregate<false> const &b) -> bool {
    return std::tie(a.lhs_, a.elems_, a.rhs_) == std::tie(b.lhs_, b.elems_, b.rhs_);
}

template <> auto operator<(SetAggregate<false> const &a, SetAggregate<false> const &b) -> bool {
    return std::tie(a.lhs_, a.elems_, a.rhs_) < std::tie(b.lhs_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SetAggregateElement>::operator()(Gringo::Input::SetAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregateElement), x.lit_, x.cond_);
}

auto value_hasher<Gringo::Input::SetAggregate<true>>::operator()(Gringo::Input::SetAggregate<true> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregate<true>), x.sign(), x.lhs_, x.elems_, x.rhs_);
}

auto value_hasher<Gringo::Input::SetAggregate<false>>::operator()(Gringo::Input::SetAggregate<false> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SetAggregate<false>), x.lhs_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
