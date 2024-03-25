#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>

namespace Gringo::Ground {

void StmRule::print(std::ostream &out) const {
    out << *atom_;
    if (!provides_.empty()) {
        out << "[" << Util::p_range(provides_, ",") << "]";
    }
    out << " :- " << Util::p_range(body_, ", ", [](auto const &lit) -> decltype(auto) { return *lit; }) << ".";
}

} // namespace Gringo::Ground
