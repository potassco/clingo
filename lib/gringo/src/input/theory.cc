#include <gringo/input/theory.hh>

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TheoryTermVariable>::operator()(Gringo::Input::TheoryTermVariable const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name_);
}

auto value_hasher<Gringo::Input::TheoryTermSymbol>::operator()(Gringo::Input::TheoryTermSymbol const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermSymbol), x.value_);
}

auto value_hasher<Gringo::Input::TheoryTermTuple>::operator()(Gringo::Input::TheoryTermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermTuple), x.elems_, x.type_);
}

auto value_hasher<Gringo::Input::TheoryTermFunction>::operator()(Gringo::Input::TheoryTermFunction const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermFunction), x.name_, x.args_);
}

auto value_hasher<Gringo::Input::TheoryTermUnparsed>::operator()(Gringo::Input::TheoryTermUnparsed const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermUnparsed), x.elems_);
}

auto value_hasher<Gringo::Input::TheoryElement>::operator()(Gringo::Input::TheoryElement const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryElement), x.tuple_, x.cond_);
}

auto value_hasher<Gringo::Input::TheoryAtom<true>>::operator()(Gringo::Input::TheoryAtom<true> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtom<true>), x.sign(), x.name_, x.elems_, x.rhs_);
}

auto value_hasher<Gringo::Input::TheoryAtom<false>>::operator()(Gringo::Input::TheoryAtom<false> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtom<false>), x.name_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
