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

} // namespace Gringo::Input
