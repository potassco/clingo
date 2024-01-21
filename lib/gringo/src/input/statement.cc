#include <gringo/input/statement.hh>

namespace Gringo::Input {

auto operator==(Rule const &a, Rule const &b) -> bool { return std::tie(a.head, a.body) == std::tie(b.head, b.body); }

auto operator<(Rule const &a, Rule const &b) -> bool { return std::tie(a.head, a.body) < std::tie(b.head, b.body); }

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::Rule>::operator()(Gringo::Input::Rule const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::Rule), x.head, x.body);
}

} // namespace Gringo::Util
