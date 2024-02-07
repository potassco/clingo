#include <gringo/input/theory.hh>

namespace Gringo::Input {

auto operator==(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool { return a.name_ == b.name_; }

auto operator<(TheoryTermVariable const &a, TheoryTermVariable const &b) -> bool { return a.name_ < b.name_; }

auto operator==(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool { return a.value_ == b.value_; }

auto operator<(TheoryTermSymbol const &a, TheoryTermSymbol const &b) -> bool { return a.value_ < b.value_; }

auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
}

auto operator<(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return std::tie(a.type_, a.elems_) < std::tie(b.type_, b.elems_);
}

auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
}

auto operator<(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return std::tie(a.name_, a.args_) < std::tie(b.name_, b.args_);
}

auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool { return a.elems_ == b.elems_; }

auto operator<(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool { return a.elems_ < b.elems_; }

auto operator==(TheoryElement const &a, TheoryElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) == std::tie(b.tuple_, b.cond_);
}

auto operator<(TheoryElement const &a, TheoryElement const &b) -> bool {
    return std::tie(a.tuple_, a.cond_) < std::tie(b.tuple_, b.cond_);
}

template <> auto operator==(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool {
    return std::tie(a.sign_, a.name_, a.elems_, a.rhs_) == std::tie(b.sign_, b.name_, b.elems_, b.rhs_);
}

template <> auto operator<(TheoryAtom<true> const &a, TheoryAtom<true> const &b) -> bool {
    return std::tie(a.sign_, a.name_, a.elems_, a.rhs_) < std::tie(b.sign_, b.name_, b.elems_, b.rhs_);
}

template <> auto operator==(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool {
    return std::tie(a.name_, a.elems_, a.rhs_) == std::tie(b.name_, b.elems_, b.rhs_);
}

template <> auto operator<(TheoryAtom<false> const &a, TheoryAtom<false> const &b) -> bool {
    return std::tie(a.name_, a.elems_, a.rhs_) < std::tie(b.name_, b.elems_, b.rhs_);
}

} // namespace Gringo::Input

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
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtom<true>), x.sign_, x.name_, x.elems_, x.rhs_);
}

auto value_hasher<Gringo::Input::TheoryAtom<false>>::operator()(Gringo::Input::TheoryAtom<false> const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TheoryAtom<false>), x.name_, x.elems_, x.rhs_);
}

} // namespace Gringo::Util
