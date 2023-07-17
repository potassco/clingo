#include <input/body_literal.hh>

namespace Gringo::Input {

namespace {

struct AddSign {

    void operator()(Conjunction const &lit) {
        if (lit.elems.size() != 1 || lit.elems.front().lits.size() != 1) {
            throw std::logic_error("there must be exactly one element");
        }
        add_sign(lit.elems.front().lits.front(), sign);
    }

    void operator()(BodyAggregate &lit) const { lit.sign += sign; }

    void operator()(BodySetAggregate &lit) const { lit.sign += sign; }

    void operator()(BodyTheoryAtom &lit) const { lit.sign += sign; }

    Sign sign;
};

} // namespace

void add_sign(BodyLiteral &lit, Sign sign) { std::visit(AddSign{sign}, lit); }

} // namespace Gringo::Input
