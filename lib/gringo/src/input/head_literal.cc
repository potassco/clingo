#include <gringo/input/head_literal.hh>

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
