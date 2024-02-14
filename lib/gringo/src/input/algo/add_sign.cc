#include "add_sign.hh"

namespace Gringo::Input {

namespace {

struct AddSign {
    template <class T> void operator()(T const &lit) const = delete;

    auto operator()(Lit const &lit) const -> Lit { return std::visit(*this, lit); }

    auto operator()(LitBool const &lit) const -> Lit {
        return LitBool{lit.loc() + pos, lit.sign() + sign, lit.value()};
    }
    auto operator()(LitComparison const &lit) const -> Lit {
        return LitComparison{lit.loc() + pos, lit.sign() + sign, lit.lhs(), lit.rhs()};
    }
    auto operator()(LitSymbolic const &lit) const -> Lit {
        return LitSymbolic{lit.loc() + pos, lit.sign() + sign, lit.term()};
    }

    auto operator()(BdLit const &lit) const -> BdLit { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> BdLit { return BdLitSimple{operator()(lit.lit())}; }
    auto operator()(BdLitConjunction const &lit) const -> BdLit {
        return BdLitConjunction{CondLit{pos + lit.lit().loc(), operator()(lit.lit().lit()), lit.lit().cond()}};
    }
    auto operator()(BdLitAggregate const &lit) const -> BdLit {
        return BdLitAggregate{lit.loc() + pos, lit.sign() + sign, lit.lhs(), lit.fun(), lit.elems(), lit.rhs()};
    }
    auto operator()(BdLitSetAggregate const &lit) const -> BdLit {
        return BdLitSetAggregate{lit.loc() + pos, lit.sign() + sign, lit.lhs(), lit.elems(), lit.rhs()};
    }
    auto operator()(BdLitTheoryAtom const &lit) const -> BdLit {
        return BdLitTheoryAtom{lit.loc() + pos, lit.sign() + sign, lit.name(),
                               TheoryElementArray{lit.elems().begin(), lit.elems().end()}, lit.rhs()};
    }

    Sign sign;
    std::optional<Position> pos;
};

} // namespace

auto add_sign(Lit const &lit, Sign sign, std::optional<Position> pos) -> std::optional<Lit> {
    if (sign != Sign::none) {
        return AddSign{sign, std::move(pos)}(lit);
    }
    return std::nullopt;
}

auto add_sign(BdLit const &lit, Sign sign, std::optional<Position> pos) -> std::optional<BdLit> {
    if (sign != Sign::none) {
        return AddSign{sign, std::move(pos)}(lit);
    }
    return std::nullopt;
}

} // namespace Gringo::Input
