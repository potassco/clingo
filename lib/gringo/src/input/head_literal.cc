#include <gringo/input/head_literal.hh>

namespace Gringo::Input {

auto operator==(HdLitSimple const &a, HdLitSimple const &b) -> bool { return a.lit_ == b.lit_; }

auto operator<(HdLitSimple const &a, HdLitSimple const &b) -> bool { return a.lit_ < b.lit_; }

auto operator==(HdLitDisjunction const &a, HdLitDisjunction const &b) -> bool { return a.elems_ == b.elems_; }

auto operator<(HdLitDisjunction const &a, HdLitDisjunction const &b) -> bool { return a.elems_ < b.elems_; }

auto operator==(HdLitAggregateElement const &a, HdLitAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.lit_, a.cond_) == std::tie(b.tuple_, b.lit_, b.cond_);
}

auto operator<(HdLitAggregateElement const &a, HdLitAggregateElement const &b) -> bool {
    return std::tie(a.tuple_, a.lit_, a.cond_) < std::tie(b.tuple_, b.lit_, b.cond_);
}

auto operator==(HdLitAggregate const &a, HdLitAggregate const &b) -> bool {
    return std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) == std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_);
}

auto operator<(HdLitAggregate const &a, HdLitAggregate const &b) -> bool {
    return std::tie(a.fun_, a.lhs_, a.elems_, a.rhs_) < std::tie(b.fun_, b.lhs_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::HdLitSimple>::operator()(Gringo::Input::HdLitSimple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HdLitSimple), x.lit_);
}

auto value_hasher<Gringo::Input::HdLitDisjunction>::operator()(Gringo::Input::HdLitDisjunction const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HdLitDisjunction), x.elems_);
}

auto value_hasher<Gringo::Input::HdLitAggregateElement>::operator()(Gringo::Input::HdLitAggregateElement const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HdLitAggregateElement), x.tuple_, x.cond_);
}

auto value_hasher<Gringo::Input::HdLitAggregate>::operator()(Gringo::Input::HdLitAggregate const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::HdLitAggregate), x.fun_, x.lhs_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
