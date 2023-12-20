#include "add_sign.hh"

namespace Gringo::Input {

namespace {

struct AddSign {
    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc + pos, lit.sign + sign, lit.value};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        return LiteralRelation{lit.loc + pos, lit.sign + sign, lit.lhs, lit.rhs};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc + pos, lit.sign + sign, lit.term};
    }

    auto operator()(BodyLiteral const &lit) const -> BodyLiteral { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> BodyLiteral {
        return SimpleBodyLiteral{operator()(lit.lit)};
    }

    auto operator()(ConditionalLiteral const &lit) const -> BodyLiteral {
        return ConditionalLiteral{lit.loc + pos, operator()(lit.lit), lit.cond};
    }
    auto operator()(BodyAggregate const &lit) const -> BodyLiteral {
        return BodyAggregate{lit.loc + pos, lit.sign + sign, lit.lhs, lit.fun, lit.elems, lit.rhs};
    }
    auto operator()(BodySetAggregate const &lit) const -> BodyLiteral {
        return BodySetAggregate{lit.loc + pos, lit.sign + sign, lit.lhs, lit.elems, lit.rhs};
    }
    auto operator()(BodyTheoryAtom const &lit) const -> BodyLiteral {
        return BodyTheoryAtom{lit.loc + pos, lit.sign + sign, lit.name, lit.elems, lit.rhs};
    }

    Sign sign;
    std::optional<Position> pos;
};

} // namespace

auto add_sign(Literal const &lit, Sign sign, std::optional<Position> pos) -> std::optional<Literal> {
    if (sign != Sign::none) {
        return AddSign{sign, std::move(pos)}(lit);
    }
    return std::nullopt;
}

auto add_sign(BodyLiteral const &lit, Sign sign, std::optional<Position> pos) -> std::optional<BodyLiteral> {
    if (sign != Sign::none) {
        return AddSign{sign, std::move(pos)}(lit);
    }
    return std::nullopt;
}

} // namespace Gringo::Input
