#include <gringo/input/term.hh>
#include <gringo/util/algorithm.hh>

namespace Gringo::Input {} // namespace Gringo::Input

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

auto value_hasher<Gringo::Input::TermAbs>::operator()(Gringo::Input::TermAbs const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermAbs), x.pool_);
}

auto value_hasher<Gringo::Input::TermUnary>::operator()(Gringo::Input::TermUnary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermUnary), x.op_, x.rhs_);
}

auto value_hasher<Gringo::Input::TermBinary>::operator()(Gringo::Input::TermBinary const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermBinary), x.op_, x.lhs_, x.rhs_);
}

} // namespace Gringo::Util
