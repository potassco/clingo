#include <input/literal.hh>

namespace Gringo::Input {

////////// Literal //////////

namespace {

struct AddSign {
    void operator()(auto &lit) const { lit.sign += sign; }

    Sign sign;
};

} // namespace

auto operator-(Sign a) -> Sign {
    switch (a) {
        case Sign::none: {
            return Sign::once;
        }
        case Sign::once: {
            return Sign::twice;
        }
        case Sign::twice: {
            break;
        }
    }
    return Sign::once;
}

auto operator+(Sign a, Sign b) -> Sign {
    switch (a) {
        case Sign::none: {
            return b;
        }
        case Sign::once: {
            return -b;
        }
        case Sign::twice: {
            break;
        }
    }
    return -(-b);
}

auto operator+=(Sign &a, Sign b) -> Sign & {
    a = a + b;
    return a;
}

auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
    return Util::value_equal(a.sign, b.sign, a.value, b.value);
}

auto operator==(LiteralRelation const &a, LiteralRelation const &b) -> bool {
    return Util::value_equal(a.sign, b.sign, a.lhs, b.lhs, a.rhs, b.rhs);
}

auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
    return Util::value_equal(a.sign, b.sign, a.term, b.term);
}

void add_sign(Literal &lit, Sign sign) { std::visit(AddSign{sign}, lit); }

} // namespace Gringo::Input

namespace std {

auto hash<Gringo::Input::LiteralBoolean>::operator()(Gringo::Input::LiteralBoolean const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermVariable), x.sign, x.value);
}

auto hash<Gringo::Input::LiteralRelation>::operator()(Gringo::Input::LiteralRelation const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermSymbol), x.sign, x.lhs, x.rhs);
}

auto hash<Gringo::Input::LiteralSymbolic>::operator()(Gringo::Input::LiteralSymbolic const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::TermTuple), x.sign, x.term);
}

} // namespace std
