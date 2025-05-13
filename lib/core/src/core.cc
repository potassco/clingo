#include <clingo/core/core.hh>

namespace CppClingo {

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

namespace {

template <class T> void output_sign(T &out, Sign sign) {
    switch (sign) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
}

} // namespace

auto operator<<(std::ostream &out, Sign sign) -> std::ostream & {
    output_sign(out, sign);
    return out;
}

auto operator<<(Util::OutputBuffer &out, Sign sign) -> Util::OutputBuffer & {
    output_sign(out, sign);
    return out;
}

auto flip(Relation rel) -> Relation {
    switch (rel) {
        case Relation::equal:
        case Relation::not_equal: {
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
            return Relation::not_equal;
        }
        case Relation::not_equal: {
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

namespace {

template <class T> void output_relation(T &out, Relation rel) {
    switch (rel) {
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::greater: {
            out << ">";
            break;
        }
        case Relation::greater_equal: {
            out << ">=";
            break;
        }
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
            break;
        }
        case Relation::not_equal: {
            out << "!=";
            break;
        }
    }
}

} // namespace

auto operator<<(std::ostream &out, Relation rel) -> std::ostream & {
    output_relation(out, rel);
    return out;
}

auto operator<<(Util::OutputBuffer &out, Relation rel) -> Util::OutputBuffer & {
    output_relation(out, rel);
    return out;
}

namespace {

template <class T> void output_fun(T &out, AggregateFunction fun) {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
}

} // namespace

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    output_fun(out, fun);
    return out;
}

auto operator<<(Util::OutputBuffer &out, AggregateFunction fun) -> Util::OutputBuffer & {
    output_fun(out, fun);
    return out;
}

auto operator<<(Util::OutputBuffer &out, HeuristicType type) -> Util::OutputBuffer & {
    switch (type) {
        case HeuristicType::factor: {
            out << "factor";
            break;
        }
        case HeuristicType::false_: {
            out << "false";
            break;
        }
        case HeuristicType::init: {
            out << "init";
            break;
        }
        case HeuristicType::level: {
            out << "level";
            break;
        }
        case HeuristicType::sign: {
            out << "sign";
            break;
        }
        case HeuristicType::true_: {
            out << "true";
            break;
        }
    }
    return out;
}

auto operator<<(Util::OutputBuffer &out, ExternalType type) -> Util::OutputBuffer & {
    switch (type) {
        case ExternalType::true_: {
            out << "true";
            break;
        }
        case ExternalType::false_: {
            out << "false";
            break;
        }
        case ExternalType::free: {
            out << "free";
            break;
        }
        case ExternalType::release: {
            out << "release";
            break;
        }
    }
    return out;
}

} // namespace CppClingo
