#include "add_sign.hh"

namespace Gringo::Input {

namespace {

template <typename T, typename V = void> constexpr bool has_loc_ = false;

template <typename T> constexpr bool has_loc_<T, std::void_t<decltype(std::declval<T>().loc)>> = true;

struct AddSign {
    void operator()(auto &lit) const {
        lit.sign += sign;
        // TODO: remove once bodyliterals have signs!
        if constexpr (has_loc_<decltype(lit)>) {
            lit.loc += std::move(pos);
        }
    }

    void operator()(Conjunction &lit) const {
        if (lit.elems.size() != 1 || lit.elems.front().lits.size() != 1) {
            throw std::logic_error("there must be exactly one element");
        }
        std::visit(*this, lit.elems.front().lits.front());
    }

    Sign sign;
    std::optional<Position> pos;
};

} // namespace

void add_sign(Literal &lit, Sign sign, std::optional<Position> pos) {
    if (sign != Sign::none) {
        std::visit(AddSign{sign, std::move(pos)}, lit);
    }
}

void add_sign(BodyLiteral &lit, Sign sign, std::optional<Position> pos) {
    if (sign != Sign::none) {
        std::visit(AddSign{sign, std::move(pos)}, lit);
    }
}

} // namespace Gringo::Input
