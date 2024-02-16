#include <gringo/input/body_literal.hh>

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
