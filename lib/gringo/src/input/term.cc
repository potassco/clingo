#include <gringo/input/term.hh>
#include <gringo/util/algorithm.hh>

namespace Gringo::Input {

auto operator==(TermVariable const &a, TermVariable const &b) -> bool { return a.name_ == b.name_; }

auto operator<(TermVariable const &a, TermVariable const &b) -> bool { return a.name_ < b.name_; }

auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool { return a.value_ == b.value_; }

auto operator<(TermSymbol const &a, TermSymbol const &b) -> bool { return a.value_ < b.value_; }

auto operator==(TermTuple const &a, TermTuple const &b) -> bool { return a.pool_ == b.pool_; }

auto operator<(TermTuple const &a, TermTuple const &b) -> bool { return a.pool_ < b.pool_; }

auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return std::tie(a.name_, a.pool_, a.external_) == std::tie(b.name_, b.pool_, b.external_);
}

auto operator<(TermFunction const &a, TermFunction const &b) -> bool {
    return std::tie(a.name_, a.pool_, a.external_) < std::tie(b.name_, b.pool_, b.external_);
}

auto operator==(TermAbs const &a, TermAbs const &b) -> bool { return a.pool_ == b.pool_; }

auto operator<(TermAbs const &a, TermAbs const &b) -> bool { return a.pool_ < b.pool_; }

auto operator==(TermUnary const &a, TermUnary const &b) -> bool {
    return std::tie(a.op_, a.rhs_) == std::tie(b.op_, b.rhs_);
};

auto operator<(TermUnary const &a, TermUnary const &b) -> bool {
    return std::tie(a.op_, a.rhs_) < std::tie(b.op_, b.rhs_);
}

auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return std::tie(*a.lhs_, a.op_, a.rhs_) == std::tie(*b.lhs_, b.op_, b.rhs_);
};

auto operator<(TermBinary const &a, TermBinary const &b) -> bool {
    return std::tie(*a.lhs_, a.op_, a.rhs_) < std::tie(*b.lhs_, b.op_, b.rhs_);
}

auto operator==(Projection const &a, Projection const &b) -> bool {
    static_cast<void>(a);
    static_cast<void>(b);
    return true;
};

auto operator<(Projection const &a, Projection const &b) -> bool {
    static_cast<void>(a);
    static_cast<void>(b);
    return false;
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::TermVariable>::operator()(Gringo::Input::TermVariable const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.name_);
}

auto value_hasher<Gringo::Input::TermSymbol>::operator()(Gringo::Input::TermSymbol const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermSymbol), x.value_);
}

auto value_hasher<Gringo::Input::TermTuple>::operator()(Gringo::Input::TermTuple const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermTuple), x.pool_);
}

auto value_hasher<Gringo::Input::TermFunction>::operator()(Gringo::Input::TermFunction const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermFunction), x.name_, x.pool_, x.external_);
}

auto value_hasher<Gringo::Input::TermAbs>::operator()(Gringo::Input::TermAbs const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermAbs), x.pool_);
}

auto value_hasher<Gringo::Input::TermUnary>::operator()(Gringo::Input::TermUnary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermUnary), x.op_, x.rhs_);
}

auto value_hasher<Gringo::Input::TermBinary>::operator()(Gringo::Input::TermBinary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermBinary), x.op_, x.lhs_, x.rhs_);
}

auto value_hasher<Gringo::Input::Projection>::operator()(Gringo::Input::Projection const &x) const -> size_t {
    static_cast<void>(x);
    return Gringo::Util::value_hash(typeid(Gringo::Input::Projection));
}

} // namespace Gringo::Util
