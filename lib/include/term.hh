#pragma once

#include <memory>
#include <sstream>
#include <variant>
#include <vector>

#include <util/print.hh>

enum class TermCheckType { atom, sig, identifier, signed_identifier, pos_number };

struct Term;
using UTerm = std::unique_ptr<Term>;

struct CheckTypeResult {
    bool has_sign = false;
    int pos_number = 0;
    std::string identifier;
};

struct Term {
    virtual ~Term() = default;
    [[nodiscard]] virtual auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool {
        return false;
    }
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.print(out);
        return out;
    }
};

using UTermVec = std::vector<UTerm>;
using UTermVecVec = std::vector<UTermVec>;

enum class Constant {
    supremum,
    infimum,
};

inline auto operator<<(std::ostream &out, Constant op) -> std::ostream & {
    switch (op) {
        case Constant::supremum: {
            out << "#sup";
            break;
        }
        case Constant::infimum: {
            out << "#inf";
            break;
        }
    }
    return out;
}

struct TermConstant : Term {
    explicit TermConstant(Constant value) : value(value) {}

    void print(std::ostream &out) const override { out << value; }

    Constant value;
};

struct TermInteger : Term {
    explicit TermInteger(int v) : value(v) {}

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override {
        if (type == TermCheckType::pos_number && value >= 0) {
            if (res != nullptr) {
                res->pos_number = value;
            }
            return true;
        }
        return false;
    }
    void print(std::ostream &out) const override { out << value; }

    int value;
};

struct TermTuple : Term {
    using Element = std::variant<UTermVec, UTerm>;
    using ElementVec = std::vector<Element>;
    explicit TermTuple(ElementVec args) : args(std::move(args)) {}

    void print(std::ostream &out) const override {
        if (args.size() == 1 && std::holds_alternative<UTerm>(args.front())) {
            std::get<UTerm>(args.front())->print(out);
        } else {
            out << "(";
            bool sem = false;
            for (const auto &tuple : args) {
                if (sem) {
                    out << ";";
                } else {
                    sem = true;
                }
                std::visit(
                    [&](auto &&arg) {
                        using T = std::decay_t<decltype(arg)>;
                        if constexpr (std::is_same_v<T, UTerm>) {
                            arg->print(out);
                        } else if constexpr (std::is_same_v<T, UTermVec>) {
                            bool comma = false;
                            for (const auto &term : arg) {
                                if (comma) {
                                    out << ",";
                                } else {
                                    comma = true;
                                }
                                term->print(out);
                            }
                            if (arg.size() == 1) {
                                out << ",";
                            }
                        }
                    },
                    tuple);
            }
            out << ")";
        }
    }

    ElementVec args;
};

struct TermString : Term {
    explicit TermString(std::string value) : value(std::move(value)) {}

    void print(std::ostream &out) const override { print_quoted(out, value); }

    std::string value;
};

struct TermVariable : Term {
    explicit TermVariable(std::string name) : name(std::move(name)) {}

    void print(std::ostream &out) const override { out << name; }

    std::string name;
};

struct TermAbs : Term {
    explicit TermAbs(UTermVec pool) : pool(std::move(pool)) {}

    void print(std::ostream &out) const override {
        out << "|";
        bool comma = false;
        for (const auto &term : pool) {
            if (comma) {
                out << ";";
            } else {
                comma = true;
            }
            term->print(out);
        }
        out << "|";
    }

    UTermVec pool;
};

struct TermFunction : Term {
    explicit TermFunction(std::string name, UTermVecVec args, bool external)
        : name(std::move(name)), args{std::move(args)}, external{external} {}

    void print(std::ostream &out) const override {
        if (external) {
            out << "@";
        }
        out << name;
        if (args.size() != 1 || !args.front().empty()) {
            out << "(";
            bool sem = false;
            for (const auto &tuple : args) {
                if (sem) {
                    out << ";";
                } else {
                    sem = true;
                }
                bool comma = false;
                for (const auto &term : tuple) {
                    if (comma) {
                        out << ",";
                    } else {
                        comma = true;
                    }
                    term->print(out);
                }
            }
            out << ")";
        }
    }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override {
        if (type == TermCheckType::atom) {
            return !external;
        }
        if ((type == TermCheckType::identifier || type == TermCheckType::signed_identifier) && !external &&
            args.size() == 1 && args.front().empty()) {
            if (res != nullptr) {
                res->identifier = name;
            }
            return true;
        }
        return false;
    }

    std::string name;
    UTermVecVec args;
    bool external;
};

enum class UnaryOperator {
    negate,
    invert,
};

inline auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream & {
    out << (op == UnaryOperator::negate ? "-" : "~");
    return out;
}

struct TermUnary : Term {
    explicit TermUnary(UnaryOperator op, UTerm e) : op(op), rhs(std::move(e)) {}

    void print(std::ostream &out) const override { out << "(" << op << *rhs << ")"; }

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override {
        if (type == TermCheckType::atom) {
            return op == UnaryOperator::negate && rhs->check_type(type);
        }
        if (type == TermCheckType::signed_identifier && op == UnaryOperator::negate &&
            rhs->check_type(TermCheckType::identifier, res)) {
            if (res != nullptr) {
                res->has_sign = true;
            }
            return true;
        }
        return false;
    }

    UnaryOperator op;
    UTerm rhs;
};

enum class BinaryOperator {
    dots,
    xor_,
    or_,
    and_,
    plus,
    minus,
    times,
    div,
    mod,
    pow,
};

inline auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream & {
    switch (op) {
        case BinaryOperator::dots: {
            out << "^";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
        case BinaryOperator::or_: {
            out << "?";
            break;
        }
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::mod: {
            out << "\\";
            break;
        }
        case BinaryOperator::pow: {
            out << "**";
            break;
        }
    }
    return out;
}

struct TermBinary : Term {
    explicit TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override {
        if (type == TermCheckType::sig) {
            return op == BinaryOperator::div && lhs->check_type(TermCheckType::signed_identifier, res) &&
                   rhs->check_type(TermCheckType::pos_number, res);
        }
        return false;
    }
    void print(std::ostream &out) const override { out << "(" << *lhs << op << *rhs << ")"; }

    BinaryOperator op;
    UTerm lhs;
    UTerm rhs;
};
