#include "add_sign.hh"

#include <clingo/util/type_traits.hh>

#include <utility>

namespace CppClingo::Input {

namespace {

struct AddSign {
    template <class T> void operator()(T const &lit) const = delete;

    template <class L>
        requires Util::is_among_v<L, LitBool, LitComparison, LitSymbolic>
    auto operator()(L const &lit) const -> Lit {
        return lit.update(a_loc = lit.loc() + pos, a_sign = lit.sign() + sign);
    }

    auto operator()(Lit const &lit) const -> Lit { return std::visit(*this, lit); }

    auto operator()(BdLitSimple const &lit) const -> BdLit { return BdLitSimple{operator()(lit.lit())}; }

    auto operator()(BdLitConjunction const &lit) const -> BdLit {
        auto const &clit = lit.lit();
        return BdLitConjunction{clit.update(a_loc = pos + clit.loc(), a_lit = operator()(clit.lit()))};
    }

    template <class L>
        requires Util::is_among_v<L, BdLitAggregate, BdLitSort, BdLitSetAggregate, BdLitTheoryAtom>
    auto operator()(L const &lit) const -> BdLit {
        return lit.update(a_loc = lit.loc() + pos, a_sign = lit.sign() + sign);
    }

    auto operator()(BdLit const &lit) const -> BdLit { return std::visit(*this, lit); }

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

} // namespace CppClingo::Input
