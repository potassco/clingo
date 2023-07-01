#pragma once

#include <term.hh>

enum class Sign {
    none,
    once,
    twice,
};

auto operator-(Sign a) -> Sign;
auto operator+(Sign a, Sign b) -> Sign;
auto operator+=(Sign &a, Sign b) -> Sign &;
auto operator<<(std::ostream &out, Sign op) -> std::ostream &;

enum class Relation {
    less,
    less_equal,
    greater,
    greater_equal,
    equal,
    inequal,
};

auto operator<<(std::ostream &out, Relation op) -> std::ostream &;

using Guard = std::pair<Relation, STerm>;
using GuardVec = std::vector<Guard>;

class Literal;
using SLiteral = shared_ptr<Literal>;
using SLiteralVec = std::vector<SLiteral>;
using PoolLiteral = PoolParent<SLiteral, PoolTerm>;

class Literal {
  public:
    virtual ~Literal() = default;
    virtual void print(std::ostream &out) const = 0;
    virtual void add_sign(Sign sign) = 0;
    virtual void unpool(PoolLiteral &pool) = 0;
    [[nodiscard]] virtual auto is_atom() const -> bool;
    [[nodiscard]] virtual auto is_test() const -> bool;
    auto unpool() -> SLiteralVec;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, Literal const &literal) -> std::ostream &;
    [[nodiscard]] virtual auto is_equal(Literal const &other) const -> bool = 0;
    friend auto operator==(Literal const &a, Literal const &b) { return a.is_equal(b); }
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    virtual void visit_variables(VarVisitFun const &fun) const = 0;
    [[nodiscard]] virtual auto project(Projection project) const -> std::optional<SLiteral> = 0;

    size_t refs = 0;
};

class LiteralRelation : public Literal {
  public:
    LiteralRelation(STerm lhs, GuardVec rhs) : sign_(Sign::none), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    LiteralRelation(Sign sign, STerm lhs, GuardVec rhs) : sign_(sign), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
    void print(std::ostream &out) const override;
    void add_sign(Sign s) override;
    void unpool(PoolLiteral &pool) override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;

  private:
    Sign sign_;
    STerm lhs_;
    GuardVec rhs_;
};

class LiteralBoolean : public Literal {
  public:
    LiteralBoolean(bool value) : sign_(Sign::none), value_(value) {}
    LiteralBoolean(Sign sign, bool value) : sign_(sign), value_(value) {}
    void print(std::ostream &out) const override;
    void add_sign(Sign s) override;
    void unpool(PoolLiteral &pool) override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;

  private:
    Sign sign_;
    bool value_;
};

class LiteralSymbolic : public Literal {
  public:
    LiteralSymbolic(STerm term) : sign_(Sign::none), term_(std::move(term)) {}
    LiteralSymbolic(Sign sign, STerm term) : sign_(sign), term_(std::move(term)) {}
    void print(std::ostream &out) const override;
    void add_sign(Sign s) override;
    void unpool(PoolLiteral &pool) override;
    [[nodiscard]] auto is_equal(Literal const &other) const -> bool override;
    [[nodiscard]] auto hash() const -> size_t override;
    void visit_variables(VarVisitFun const &fun) const override;
    [[nodiscard]] auto project(Projection project) const -> std::optional<SLiteral> override;
    [[nodiscard]] auto is_atom() const -> bool override;
    [[nodiscard]] auto is_test() const -> bool override;

  private:
    Sign sign_;
    STerm term_;
};

HASH(Literal)
