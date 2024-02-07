#include "add_sign.hh"

namespace Gringo::Input {

namespace {

struct AddSign {
    template <class T> void operator()(T const &lit) const = delete;

    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }

    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc_ + pos, lit.sign_ + sign, lit.value_};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        return LiteralRelation{lit.loc_ + pos, lit.sign_ + sign, lit.lhs_, lit.rhs_};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc_ + pos, lit.sign_ + sign, lit.term_};
    }

    auto operator()(BodyLiteral const &lit) const -> BodyLiteral { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> BodyLiteral {
        return SimpleBodyLiteral{operator()(lit.lit_)};
    }
    auto operator()(Conjunction const &lit) const -> BodyLiteral {
        return Conjunction{ConditionalLiteral{lit.lit_.loc_ + pos, operator()(lit.lit_.lit_), lit.lit_.cond_}};
    }
    auto operator()(BodyAggregate const &lit) const -> BodyLiteral {
        return BodyAggregate{lit.loc_ + pos, lit.sign_ + sign, lit.lhs_, lit.fun_, lit.elems_, lit.rhs_};
    }
    auto operator()(BodySetAggregate const &lit) const -> BodyLiteral {
        return BodySetAggregate{lit.loc_ + pos, lit.sign_ + sign, lit.lhs_, lit.elems_, lit.rhs_};
    }
    auto operator()(BodyTheoryAtom const &lit) const -> BodyLiteral {
        return BodyTheoryAtom{lit.loc_ + pos, lit.sign_ + sign, lit.name_, lit.elems_, lit.rhs_};
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
