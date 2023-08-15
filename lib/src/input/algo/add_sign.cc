#include "add_sign.hh"

namespace Gringo::Input {

namespace {

struct AddSign {
    template <class... T> void operator()(std::variant<T...> &lit) const { std::visit(*this, lit); }

    template <class T> void operator()(T &lit) const {
        lit.sign += sign;
        location(lit) += std::move(pos);
    }

    void operator()(SimpleBodyLiteral &lit) const { operator()(lit.lit); }

    void operator()(Conjunction &lit) const {
        if (lit.elems.size() != 1 || lit.elems.front().lits.size() != 1) {
            throw std::logic_error("there must be exactly one element");
        }
        operator()(lit.elems.front().lits.front());
    }

    Sign sign;
    std::optional<Position> pos;
};

} // namespace

void add_sign(Literal &lit, Sign sign, std::optional<Position> pos) {
    if (sign != Sign::none) {
        AddSign{sign, std::move(pos)}(lit);
    }
}

void add_sign(BodyLiteral &lit, Sign sign, std::optional<Position> pos) {
    if (sign != Sign::none) {
        AddSign{sign, std::move(pos)}(lit);
    }
}

} // namespace Gringo::Input
