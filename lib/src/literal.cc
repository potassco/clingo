#include <sstream>

#include <util/print.hh>

#include <literal.hh>

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

auto operator<<(std::ostream &out, Sign op) -> std::ostream & {
    switch (op) {
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
    return out;
}

[[nodiscard]] auto Literal::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream & {
    literal.print(out);
    return out;
}

////////// LiteralRelation //////////

auto operator<<(std::ostream &out, Relation op) -> std::ostream & {
    switch (op) {
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
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
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::inequal: {
            out << "!=";
            break;
        }
    }
    return out;
}

void LiteralRelation::print(std::ostream &out) const {
    out << sign_ << *lhs_;
    for (auto &&guard : rhs_) {
        out << guard.first << *guard.second;
    }
}

void LiteralRelation::add_sign(Sign s) { sign_ += s; }

////////// LiteralBoolean //////////

void LiteralBoolean::print(std::ostream &out) const { out << sign_ << (value_ ? "#true" : "#false"); }

void LiteralBoolean::add_sign(Sign s) { sign_ += s; }

////////// LiteralSymbolic //////////

void LiteralSymbolic::print(std::ostream &out) const { out << sign_ << *term_; }

void LiteralSymbolic::add_sign(Sign s) { sign_ += s; }
