#include <gringo/input/literal.hh>

namespace Gringo::Input {

////////// Literal //////////

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

auto flip(Relation rel) -> Relation {
    switch (rel) {
        case Relation::equal:
        case Relation::inequal: {
            break;
        }
        case Relation::greater: {
            return Relation::less;
        }
        case Relation::greater_equal: {
            return Relation::less_equal;
        }
        case Relation::less: {
            return Relation::greater;
        }
        case Relation::less_equal: {
            return Relation::greater_equal;
        }
    }
    return rel;
}

auto complement(Relation rel) -> Relation {
    switch (rel) {
        case Relation::equal: {
            return Relation::inequal;
        }
        case Relation::inequal: {
            return Relation::equal;
        }
        case Relation::greater: {
            return Relation::less_equal;
        }
        case Relation::greater_equal: {
            return Relation::less;
        }
        case Relation::less: {
            return Relation::greater_equal;
        }
        case Relation::less_equal: {
            break;
        }
    }
    return Relation::greater;
}

auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
    return std::tie(a.sign, a.value) == std::tie(b.sign, b.value);
}

auto operator==(LiteralRelation const &a, LiteralRelation const &b) -> bool {
    return std::tie(a.sign, a.lhs, a.rhs) == std::tie(b.sign, b.lhs, b.rhs);
}

auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
    return std::tie(a.sign, a.term) == std::tie(b.sign, b.term);
}

auto operator==(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool {
    return std::tie(a.lit, a.cond) == std::tie(b.lit, b.cond);
}

auto operator<(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
    return std::tie(a.sign, a.value) < std::tie(b.sign, b.value);
}

auto operator<(LiteralRelation const &a, LiteralRelation const &b) -> bool {
    return std::tie(a.sign, a.lhs, a.rhs) < std::tie(b.sign, b.lhs, b.rhs);
}

auto operator<(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
    return std::tie(a.sign, a.term) < std::tie(b.sign, b.term);
}

auto operator<(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool {
    return std::tie(a.lit, a.cond) < std::tie(b.lit, b.cond);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::LiteralBoolean>::operator()(Gringo::Input::LiteralBoolean const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralBoolean), x.sign, x.value);
}

auto value_hasher<Gringo::Input::LiteralRelation>::operator()(Gringo::Input::LiteralRelation const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralRelation), x.sign, x.lhs, x.rhs);
}

auto value_hasher<Gringo::Input::LiteralSymbolic>::operator()(Gringo::Input::LiteralSymbolic const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralSymbolic), x.sign, x.term);
}

auto value_hasher<Gringo::Input::ConditionalLiteral>::operator()(Gringo::Input::ConditionalLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::ConditionalLiteral), x.lit, x.cond);
}

} // namespace Gringo::Util
