#include <gringo/ground/literal.hh>

namespace Gringo::Ground {

auto operator<<(std::ostream &out, Sign sign) -> std::ostream & {
    switch (sign) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
    return out;
}

void LitSymbolic::print(std::ostream &out) const { out << sign_ << *atom_ << "[" << provided_by_ << "]"; }

} // namespace Gringo::Ground
