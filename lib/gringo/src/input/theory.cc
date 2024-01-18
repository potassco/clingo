#include <gringo/input/theory.hh>

namespace Gringo::Input {

auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool { return a.name == b.name; }

auto operator<(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool { return a.name < b.name; }

auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool { return a.value == b.value; }

auto operator<(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool { return a.value < b.value; }

auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type, a.elems) == std::tie(b.type, b.elems);
}

auto operator<(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type, a.elems) < std::tie(b.type, b.elems);
}

auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name, a.args) == std::tie(b.name, b.args);
}

auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name, a.args) < std::tie(b.name, b.args);
}

auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool { return a.elems == b.elems; }

auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool { return a.elems < b.elems; }

auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool {
    return std::tie(a.tuple, a.cond) == std::tie(b.tuple, b.cond);
}

auto operator<(TheoryElement const &a, TheoryElement const &b) -> bool {
    return std::tie(a.tuple, a.cond) < std::tie(b.tuple, b.cond);
}

template <> auto operator==(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool {
    return std::tie(a.sign, a.name, a.elems, a.rhs) == std::tie(b.sign, b.name, b.elems, b.rhs);
}

template <> auto operator<(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool {
    return std::tie(a.sign, a.name, a.elems, a.rhs) < std::tie(b.sign, b.name, b.elems, b.rhs);
}

template <> auto operator==(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool {
    return std::tie(a.name, a.elems, a.rhs) == std::tie(b.name, b.elems, b.rhs);
}

template <> auto operator<(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool {
    return std::tie(a.name, a.elems, a.rhs) < std::tie(b.name, b.elems, b.rhs);
}

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

auto value_hasher<Gringo::Input::TheoryElement>::operator()(Gringo::Input::TheoryElement const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryElement), x.tuple, x.cond);
}

template <bool HasSign>
auto value_hasher<Gringo::Input::TheoryAtom<HasSign>>::operator()(Gringo::Input::TheoryAtom<HasSign> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtom<HasSign>), x.name, x.elems, x.rhs);
}

template <> struct value_hasher<Gringo::Input::TheoryAtom<true>>;

template <> struct value_hasher<Gringo::Input::TheoryAtom<false>>;

} // namespace Gringo::Util
