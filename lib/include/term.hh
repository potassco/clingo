#pragma once

#include <memory>
#include <optional>
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

struct AST;
using SAST = std::shared_ptr<AST>;
using SASTVec = std::vector<SAST>;

enum class ASTType {
    TermConstant,
    TermInteger,
    TermTuple,
    TermString,
    TermVariable,
    TermAbs,
    TermFunction,
    TermUnary,
    TermBinary,
};
enum class ASTAttr {};

auto operator<<(std::ostream &out, ASTType type) -> std::ostream &;
auto operator<<(std::ostream &out, ASTAttr attr) -> std::ostream &;

struct AST {
    virtual ~AST() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, AST const &ast) -> std::ostream &;
    [[nodiscard]] virtual auto type() const -> ASTType = 0;
    [[nodiscard]] virtual auto get_int(ASTAttr attr) -> int &;
    [[nodiscard]] virtual auto get_ast(ASTAttr attr) -> SAST &;
    template <class T> auto get(ASTAttr attr) -> T & {
        if constexpr (std::is_same_v<T, int>) {
            return get_int(attr);
        } else {
            static_assert(sizeof(T *) == 0, "unsupported type in AST::get");
        }
    };
};

struct Term : AST {
    [[nodiscard]] virtual auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool;
};

using UTermVec = std::vector<UTerm>;
using UTermVecVec = std::vector<UTermVec>;

enum class Constant {
    supremum,
    infimum,
};

auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

struct TermConstant : Term {
    explicit TermConstant(Constant value) : value(value) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    Constant value;
};

struct TermInteger : Term {
    explicit TermInteger(int v) : value(v) {}

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    int value;
};

struct TermTuple : Term {
    using Element = std::variant<UTermVec, UTerm>;
    using ElementVec = std::vector<Element>;
    explicit TermTuple(ElementVec args) : args(std::move(args)) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    ElementVec args;
};

struct TermString : Term {
    explicit TermString(std::string value) : value(std::move(value)) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    std::string value;
};

struct TermVariable : Term {
    explicit TermVariable(std::string name) : name(std::move(name)) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    std::string name;
};

struct TermAbs : Term {
    explicit TermAbs(UTermVec pool) : pool(std::move(pool)) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    UTermVec pool;
};

struct TermFunction : Term {
    explicit TermFunction(std::string name, UTermVecVec args, bool external)
        : name(std::move(name)), args{std::move(args)}, external{external} {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

    std::string name;
    UTermVecVec args;
    bool external;
};

enum class UnaryOperator {
    negate,
    invert,
};

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

struct TermUnary : Term {
    explicit TermUnary(UnaryOperator op, UTerm e) : op(op), rhs(std::move(e)) {}

    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

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

auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;

struct TermBinary : Term {
    explicit TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    BinaryOperator op;
    UTerm lhs;
    UTerm rhs;
};
