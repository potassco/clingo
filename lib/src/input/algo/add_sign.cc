#include "add_sign.hh"

namespace Gringo::Input {

namespace {

struct AddSign {
    void operator()(auto &lit) const {
        lit.sign += sign;
        location(lit) += std::move(pos);
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
