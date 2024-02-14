#include <gringo/input/body_literal.hh>

namespace Gringo::Input {

auto operator==(BdLitSimple const &a, BdLitSimple const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(BdLitSimple const &a, BdLitSimple const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(BdLitConjunction const &a, BdLitConjunction const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(BdLitConjunction const &a, BdLitConjunction const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) == std::tie(b.tuple_, b.cond_);
}

auto operator<(BdLitAggregateElement const &a, BdLitAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) < std::tie(b.tuple_, b.cond_);
}

auto operator==(BdLitAggregate const &a, BdLitAggregate const &b) -> bool {
    return std::tie(a.sign_, a.fun_, a.lhs_, a.elems_, a.rhs_) == std::tie(b.sign_, b.fun_, b.lhs_, b.elems_, b.rhs_);
}

auto operator<(BdLitAggregate const &a, BdLitAggregate const &b) -> bool {
    return std::tie(a.sign_, a.fun_, a.lhs_, a.elems_, a.rhs_) < std::tie(b.sign_, b.fun_, b.lhs_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::BdLitSimple>::operator()(Gringo::Input::BdLitSimple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BdLitSimple), x.lit_);
}

auto value_hasher<Gringo::Input::BdLitConjunction>::operator()(Gringo::Input::BdLitConjunction const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BdLitConjunction), x.lit_);
}

auto value_hasher<Gringo::Input::BdLitAggregateElement>::operator()(Gringo::Input::BdLitAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BdLitAggregateElement), x.tuple_, x.cond_);
}

auto value_hasher<Gringo::Input::BdLitAggregate>::operator()(Gringo::Input::BdLitAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::BdLitAggregate), x.sign_, x.fun_, x.lhs_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
