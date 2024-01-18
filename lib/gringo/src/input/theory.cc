#include <gringo/input/theory.hh>

namespace Gringo::Input {
// TODO
} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TheoryTermVariable>::operator()(Gringo::Input::TheoryTermVariable const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name);
}

auto value_hasher<Gringo::Input::TheoryTermSymbol>::operator()(Gringo::Input::TheoryTermSymbol const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermSymbol), x.value);
}

auto value_hasher<Gringo::Input::TheoryTermTuple>::operator()(Gringo::Input::TheoryTermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermTuple), x.elems, x.type);
}

auto value_hasher<Gringo::Input::TheoryTermFunction>::operator()(Gringo::Input::TheoryTermFunction const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermFunction), x.name, x.args);
}

auto value_hasher<Gringo::Input::TheoryTermUnparsed>::operator()(Gringo::Input::TheoryTermUnparsed const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryTermUnparsed), x.elems);
}

} // namespace Gringo::Util
