#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

auto operator==(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(SimpleBodyLiteral const &a, SimpleBodyLiteral const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(Conjunction const &a, Conjunction const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(Conjunction const &a, Conjunction const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(BodyAggregateElement const &a, BodyAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) == std::tie(b.tuple_, b.cond_);
}

auto operator<(BodyAggregateElement const &a, BodyAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) < std::tie(b.tuple_, b.cond_);
}

auto operator==(BodyAggregate const &a, BodyAggregate const &b) -> bool {
    return std::tie(a.sign_, a.fun_, a.lhs_, a.elems_, a.rhs_) == std::tie(b.sign_, b.fun_, b.lhs_, b.elems_, b.rhs_);
}

auto operator<(BodyAggregate const &a, BodyAggregate const &b) -> bool {
    return std::tie(a.sign_, a.fun_, a.lhs_, a.elems_, a.rhs_) < std::tie(b.sign_, b.fun_, b.lhs_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::SimpleBodyLiteral>::operator()(Gringo::Input::SimpleBodyLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::SimpleBodyLiteral), x.lit_);
}

auto value_hasher<Gringo::Input::Conjunction>::operator()(Gringo::Input::Conjunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Conjunction), x.lit_);
}

auto value_hasher<Gringo::Input::BodyAggregateElement>::operator()(Gringo::Input::BodyAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BodyAggregateElement), x.tuple_, x.cond_);
}

auto value_hasher<Gringo::Input::BodyAggregate>::operator()(Gringo::Input::BodyAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BodyAggregate), x.sign_, x.fun_, x.lhs_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
