#pragma once

#include <memory>
#include <optional>
#include <sstream>
#include <variant>
#include <vector>

#include <util/print.hh>

enum class TermCheckType { atom, sig, identifier, signed_identifier, pos_number };

class Term;
using UTerm = std::unique_ptr<Term>;

struct CheckTypeResult {
    bool has_sign = false;
    int pos_number = 0;
    std::string identifier;
};

class AST;
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

enum class ASTAttr {
    Value,
    Name,
    Pool,
    Arguments,
    Left,
    Right,
    Operator,
};

auto operator<<(std::ostream &out, ASTType type) -> std::ostream &;
auto operator<<(std::ostream &out, ASTAttr attr) -> std::ostream &;

class AST {
  public:
    virtual ~AST() = default;

    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto type() const -> ASTType = 0;
    [[nodiscard]] virtual auto get_int(ASTAttr attr) -> int &;
    [[nodiscard]] virtual auto get_ast(ASTAttr attr) -> SAST &;

    friend auto operator<<(std::ostream &out, AST const &ast) -> std::ostream &;
    [[nodiscard]] auto to_string() const -> std::string;
    template <class T> auto get(ASTAttr attr) -> T & {
        if constexpr (std::is_same_v<T, int>) {
            return get_int(attr);
        } else {
            static_assert(sizeof(T *) == 0, "unsupported type in AST::get");
        }
    };
};

class Term : public AST {
  public:
    [[nodiscard]] virtual auto check_type(TermCheckType type, CheckTypeResult *res = nullptr) const -> bool;
};

using UTermVec = std::vector<UTerm>;
using UTermVecVec = std::vector<UTermVec>;

enum class Constant : int {
    supremum,
    infimum,
};

auto operator<<(std::ostream &out, Constant op) -> std::ostream &;

class TermConstant : public Term {
  public:
    explicit TermConstant(Constant value) : value_{value} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;
    [[nodiscard]] auto get_int(ASTAttr attr) -> int & override;

  private:
    Constant value_;
};

class TermInteger : public Term {
  public:
    explicit TermInteger(int v) : value_{v} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;
    [[nodiscard]] auto get_int(ASTAttr attr) -> int & override;

    // Term interface
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

  private:
    int value_;
};

class TermTuple : public Term {
  public:
    using Element = std::variant<UTermVec, UTerm>;
    using ElementVec = std::vector<Element>;
    explicit TermTuple(ElementVec args) : args_{std::move(args)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

  private:
    ElementVec args_;
};

class TermString : public Term {
  public:
    explicit TermString(std::string value) : value_{std::move(value)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

  private:
    std::string value_;
};

class TermVariable : public Term {
  public:
    explicit TermVariable(std::string name) : name_{std::move(name)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

  private:
    std::string name_;
};

class TermAbs : public Term {
  public:
    explicit TermAbs(UTermVec pool) : pool_{std::move(pool)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

  private:
    UTermVec pool_;
};

class TermFunction : public Term {
  public:
    explicit TermFunction(std::string name, UTermVecVec args, bool external)
        : name_(std::move(name)), args_{std::move(args)}, external_{external} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    // Term interface
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

  private:
    std::string name_;
    UTermVecVec args_;
    bool external_;
};

enum class UnaryOperator {
    negate,
    invert,
};

auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

class TermUnary : public Term {
  public:
    explicit TermUnary(UnaryOperator op, UTerm e) : op_{op}, rhs_{std::move(e)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    // Term interface
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

  private:
    UnaryOperator op_;
    UTerm rhs_;
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

class TermBinary : public Term {
  public:
    explicit TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs)
        : op_{op}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)} {}

    // AST interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto type() const -> ASTType override;

    // Term interface
    [[nodiscard]] auto check_type(TermCheckType type, CheckTypeResult *res) const -> bool override;

    BinaryOperator op_;
    UTerm lhs_;
    UTerm rhs_;
};
