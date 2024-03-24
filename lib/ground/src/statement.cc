#include <gringo/ground/statement.hh>

namespace Gringo::Ground {

void StmRule::print(std::ostream &out) const {
    out << *atom_ << " :- ";
    bool comma = false;
    for (auto const &lit : body_) {
        if (comma) {
            out << ", ";
        } else {
            comma = true;
        }
        out << *lit;
    }
    out << "." << std::endl;
}

} // namespace Gringo::Ground
