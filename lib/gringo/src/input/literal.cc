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

auto operator==(Signed const &a, Signed const &b) -> bool { return a.sign_ == b.sign_; }

auto operator<(Signed const &a, Signed const &b) -> bool { return a.sign_ < b.sign_; }

auto operator==(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
    return std::tie(a.sign_, a.value_) == std::tie(b.sign_, b.value_);
}

auto operator==(LiteralRelation const &a, LiteralRelation const &b) -> bool {
    return std::tie(a.sign_, a.lhs_, a.rhs_) == std::tie(b.sign_, b.lhs_, b.rhs_);
}

auto operator==(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
    return std::tie(a.sign_, a.term_) == std::tie(b.sign_, b.term_);
}

auto operator==(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool {
    return std::tie(a.lit_, a.cond_) == std::tie(b.lit_, b.cond_);
}

auto operator<(LiteralBoolean const &a, LiteralBoolean const &b) -> bool {
    return std::tie(a.sign_, a.value_) < std::tie(b.sign_, b.value_);
}

auto operator<(LiteralRelation const &a, LiteralRelation const &b) -> bool {
    return std::tie(a.sign_, a.lhs_, a.rhs_) < std::tie(b.sign_, b.lhs_, b.rhs_);
}

auto operator<(LiteralSymbolic const &a, LiteralSymbolic const &b) -> bool {
    return std::tie(a.sign_, a.term_) < std::tie(b.sign_, b.term_);
}

auto operator<(ConditionalLiteral const &a, ConditionalLiteral const &b) -> bool {
    return std::tie(a.lit_, a.cond_) < std::tie(b.lit_, b.cond_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::LiteralBoolean>::operator()(Gringo::Input::LiteralBoolean const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralBoolean), x.sign_, x.value_);
}

auto value_hasher<Gringo::Input::LiteralRelation>::operator()(Gringo::Input::LiteralRelation const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralRelation), x.sign_, x.lhs_, x.rhs_);
}

auto value_hasher<Gringo::Input::LiteralSymbolic>::operator()(Gringo::Input::LiteralSymbolic const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LiteralSymbolic), x.sign_, x.term_);
}

auto value_hasher<Gringo::Input::ConditionalLiteral>::operator()(Gringo::Input::ConditionalLiteral const &x) const
    -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::ConditionalLiteral), x.lit_, x.cond_);
}

} // namespace Gringo::Util
