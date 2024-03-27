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

void LitSymbolic::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

auto LitSymbolic::recursive() const -> bool {
    return sign_ == Sign::none && index_ != std::numeric_limits<size_t>::max();
}

void LitSymbolic::vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

} // namespace Gringo::Ground
