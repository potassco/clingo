#pragma once

#include <iostream>
#include <vector>
#include <memory>

#include <term.hh>

enum class Sign {
    none,
    once,
    twice,
};

inline auto operator-(Sign a) {
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

inline auto operator+(Sign a, Sign b) {
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

inline auto operator+=(Sign &a, Sign b) -> auto & {
    a = a + b;
    return a;
}

inline auto operator<<(std::ostream &out, Sign op) -> std::ostream & {
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

struct Literal {
    virtual ~Literal() = default;
    virtual void print(std::ostream &out) const = 0;
    virtual void add_sign(Sign sign) = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream & {
        literal.print(out);
        return out;
    }
};

using ULiteral = std::unique_ptr<Literal>;
using ULiteralVec = std::vector<ULiteral>;

enum class Relation {
    less,
    less_equal,
    greater,
    greater_equal,
    equal,
    inequal,
};

inline auto operator<<(std::ostream &out, Relation op) -> std::ostream & {
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

using Guard = std::pair<Relation, UTerm>;
using GuardVec = std::vector<Guard>;

struct LiteralRelation : Literal {
    LiteralRelation(UTerm lhs, GuardVec rhs) : sign(Sign::none), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    LiteralRelation(Sign sign, UTerm lhs, GuardVec rhs) : sign(sign), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    void print(std::ostream &out) const override {
        out << sign << *lhs;
        for (auto &&guard : rhs) {
            out << guard.first << *guard.second;
        }
    }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    UTerm lhs;
    GuardVec rhs;
};

struct LiteralBoolean : Literal {
    LiteralBoolean(bool value) : sign(Sign::none), value(value) {}
    LiteralBoolean(Sign sign, bool value) : sign(sign), value(value) {}
    void print(std::ostream &out) const override { out << sign << (value ? "#true" : "#false"); }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    bool value;
};

struct LiteralSymbolic : Literal {
    LiteralSymbolic(UTerm term) : sign(Sign::none), term(std::move(term)) {}
    LiteralSymbolic(Sign sign, UTerm term) : sign(sign), term(std::move(term)) {}
    void print(std::ostream &out) const override { out << sign << *term; }
    void add_sign(Sign s) override { sign += s; }
    Sign sign;
    UTerm term;
};

