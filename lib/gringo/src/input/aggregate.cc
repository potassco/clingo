#include <gringo/input/aggregate.hh>

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
