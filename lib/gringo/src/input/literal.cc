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

auto operator==(LitBool const &a, LitBool const &b) -> bool {
    return std::tie(a.sign_, a.value_) == std::tie(b.sign_, b.value_);
}

auto operator==(LitComparison const &a, LitComparison const &b) -> bool {
    return std::tie(a.sign_, a.lhs_, a.rhs_) == std::tie(b.sign_, b.lhs_, b.rhs_);
}

auto operator==(LitSymbolic const &a, LitSymbolic const &b) -> bool {
    return std::tie(a.sign_, a.term_) == std::tie(b.sign_, b.term_);
}

auto operator==(CondLit const &a, CondLit const &b) -> bool {
    return std::tie(a.lit_, a.cond_) == std::tie(b.lit_, b.cond_);
}

auto operator<(LitBool const &a, LitBool const &b) -> bool {
    return std::tie(a.sign_, a.value_) < std::tie(b.sign_, b.value_);
}

auto operator<(LitComparison const &a, LitComparison const &b) -> bool {
    return std::tie(a.sign_, a.lhs_, a.rhs_) < std::tie(b.sign_, b.lhs_, b.rhs_);
}

auto operator<(LitSymbolic const &a, LitSymbolic const &b) -> bool {
    return std::tie(a.sign_, a.term_) < std::tie(b.sign_, b.term_);
}

auto operator<(CondLit const &a, CondLit const &b) -> bool {
    return std::tie(a.lit_, a.cond_) < std::tie(b.lit_, b.cond_);
}

} // namespace Gringo::Input

namespace Gringo::Util {

auto value_hasher<Gringo::Input::LitBool>::operator()(Gringo::Input::LitBool const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LitBool), x.sign_, x.value_);
}

auto value_hasher<Gringo::Input::LitComparison>::operator()(Gringo::Input::LitComparison const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LitComparison), x.sign_, x.lhs_, x.rhs_);
}

auto value_hasher<Gringo::Input::LitSymbolic>::operator()(Gringo::Input::LitSymbolic const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::LitSymbolic), x.sign_, x.term_);
}

auto value_hasher<Gringo::Input::CondLit>::operator()(Gringo::Input::CondLit const &x) const -> size_t {
    return Gringo::Util::value_hash(typeid(Gringo::Input::CondLit), x.lit_, x.cond_);
}

} // namespace Gringo::Util
