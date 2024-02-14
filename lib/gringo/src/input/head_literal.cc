#include <gringo/input/head_literal.hh>

namespace Gringo::Input {

auto operator==(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(SimpleHeadLiteral const &a, SimpleHeadLiteral const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(Disjunction const &a, Disjunction const &b) -> bool { return a.elems_ == b.elems_; }

auto operator<(Disjunction const &a, Disjunction const &b) -> bool { return a.elems_ < b.elems_; }

auto operator==(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.lit_, a.cond_) == std::tie(b.tuple_, b.lit_, b.cond_);
}

auto operator<(HeadAggregateElement const &a, HeadAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.lit_, a.cond_) < std::tie(b.tuple_, b.lit_, b.cond_);
}

auto operator==(HeadAggregate const &a, HeadAggregate const &b) -> bool {
    return std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) == std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_);
}

auto operator<(HeadAggregate const &a, HeadAggregate const &b) -> bool {
    return std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) < std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SimpleHeadLiteral>::operator()(Gringo::Input::SimpleHeadLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SimpleHeadLiteral), x.lit_);
}

auto value_hasher<Gringo::Input::Disjunction>::operator()(Gringo::Input::Disjunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Disjunction), x.elems_);
}

auto value_hasher<Gringo::Input::HeadAggregateElement>::operator()(Gringo::Input::HeadAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HeadAggregateElement), x.tuple_, x.cond_);
}

auto value_hasher<Gringo::Input::HeadAggregate>::operator()(Gringo::Input::HeadAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HeadAggregate), x.fun_, x.lhs_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
