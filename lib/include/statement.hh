#pragma once

#include <memory>

#include <body_literal.hh>
#include <head_literal.hh>

class Statement;
using SStatement = shared_ptr<Statement>;
using SStatementVec = std::vector<SStatement>;

class PoolStatement;

struct destruct_pool {
    void operator()(PoolStatement *pool) const;
};
auto construct_pool(SStatementVec &pool) -> std::unique_ptr<PoolStatement, destruct_pool>;

class Statement {
  public:
    virtual ~Statement() = default;

    virtual void unpool(PoolStatement &pool) = 0;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream &;
    auto unpool() -> SStatementVec;

    size_t refs = 0;
};

class Rule : public Statement {
  public:
    explicit Rule(SHeadLiteral head, SBodyLiteralVec body) : head_{std::move(head)}, body_{std::move(body)} {}

    void print(std::ostream &out) const override;
    void unpool(PoolStatement &pool) override;

  private:
    SHeadLiteral head_;
    SBodyLiteralVec body_;
};

enum class TheoryOpType { unary, binary_left, binary_right };

class TheoryOpDefinition {
  public:
    explicit TheoryOpDefinition(std::string op, int prio, TheoryOpType type)
        : op_{std::move(op)}, prio_{prio}, type_{type} {}

    friend auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream &;

  private:
    std::string op_;
    int prio_;
    TheoryOpType type_;
};

using TheoryOpDefinitionVec = std::vector<TheoryOpDefinition>;

class TheoryTermDefinition {
  public:
    explicit TheoryTermDefinition(std::string name, TheoryOpDefinitionVec op_defs)
        : name_{std::move(name)}, op_defs_{std::move(op_defs)} {}

    friend auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream &;

  private:
    std::string name_;
    std::vector<TheoryOpDefinition> op_defs_;
};

using TheoryTermDefinitionVec = std::vector<TheoryTermDefinition>;

enum class TheoryAtomType { head, body, any, directive };

auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream &;

class TheoryAtomDefinition {
  public:
    using RHS = std::optional<std::pair<std::vector<std::string>, std::string>>;

    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, RHS rhs, TheoryAtomType type)
        : name_(std::move(name)), arity_(arity), term_(std::move(term)), rhs_(std::move(rhs)), type_(type) {}

    friend auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream &;

  private:
    std::string name_;
    int arity_;
    std::string term_;
    RHS rhs_;
    TheoryAtomType type_;
};
using TheoryAtomDefinitionVec = std::vector<TheoryAtomDefinition>;

class TheoryDefinition : public Statement {
  public:
    explicit TheoryDefinition(std::string name, TheoryTermDefinitionVec term_defs, TheoryAtomDefinitionVec atom_defs)
        : name_{std::move(name)}, term_defs_{std::move(term_defs)}, atom_defs_{std::move(atom_defs)} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    std::string name_;
    TheoryTermDefinitionVec term_defs_;
    TheoryAtomDefinitionVec atom_defs_;
};

enum class OptimizeType { minimize, maximize };

auto operator<<(std::ostream &out, OptimizeType type) -> std::ostream &;

class StatementOptimize : public Statement {
  public:
    using Tuple = std::tuple<STerm, std::optional<STerm>, STermVec>;
    using Element = std::pair<Tuple, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit StatementOptimize(OptimizeType type, ElementVec elems) : type_{type}, elems_{std::move(elems)} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    OptimizeType type_;
    ElementVec elems_;
};

class StatementWeakConstraint : public Statement {
  public:
    using Tuple = StatementOptimize::Tuple;

    explicit StatementWeakConstraint(SBodyLiteralVec body, Tuple tuple)
        : body_{std::move(body)}, tuple_{std::move(tuple)} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    SBodyLiteralVec body_;
    Tuple tuple_;
};

class StatementShow : public Statement {
  public:
    StatementShow(STerm term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    STerm term_;
    SBodyLiteralVec body_;
};

class StatementShowSig : public Statement {
  public:
    explicit StatementShowSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementProject : public Statement {
  public:
    explicit StatementProject(STerm term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    STerm term_;
    SBodyLiteralVec body_;
};

class StatementProjectSig : public Statement {
  public:
    explicit StatementProjectSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementDefined : public Statement {
  public:
    explicit StatementDefined(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementExternal : public Statement {
  public:
    explicit StatementExternal(STerm term, SBodyLiteralVec body, std::optional<STerm> type = std::nullopt)
        : term_(std::move(term)), body_(std::move(body)), type_{std::move(type)} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    STerm term_;
    SBodyLiteralVec body_;
    std::optional<STerm> type_;
};

class StatementEdge : public Statement {
  public:
    using Edge = std::pair<STerm, STerm>;
    using EdgeVec = std::vector<Edge>;

    explicit StatementEdge(EdgeVec edges, SBodyLiteralVec body = {})
        : edges_{std::move(edges)}, body_{std::move(body)} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    EdgeVec edges_;
    SBodyLiteralVec body_;
};

class StatementHeuristic : public Statement {
  public:
    explicit StatementHeuristic(bool has_sign, STerm atom, SBodyLiteralVec body, STerm type, STerm prio, STerm mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), prio_(std::move(prio)),
          mod_(std::move(mod)), has_sign_{has_sign} {}
    explicit StatementHeuristic(bool has_sign, STerm atom, SBodyLiteralVec body, STerm type, STerm mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), mod_(std::move(mod)),
          has_sign_{has_sign} {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    STerm atom_;
    SBodyLiteralVec body_;
    STerm type_;
    std::optional<STerm> prio_;
    STerm mod_;
    bool has_sign_;
};

enum class ScriptType {
    lua,
    python,
};

auto operator<<(std::ostream &out, ScriptType type) -> std::ostream &;

class StatementScript : public Statement {
  public:
    explicit StatementScript(ScriptType type, std::string content) : type_(type), content_(std::move(content)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    ScriptType type_;
    std::string content_;
};

enum class IncludeType {
    system,
    inbuild,
};

auto operator<<(std::ostream &out, IncludeType type) -> std::ostream &;

class StatementInclude : public Statement {
  public:
    explicit StatementInclude(IncludeType type, std::string path) : type_(type), path_(std::move(path)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    IncludeType type_;
    std::string path_;
};

class StatementProgram : public Statement {
  public:
    explicit StatementProgram(std::string name, std::vector<std::string> args)
        : name_(std::move(name)), args_(std::move(args)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    std::string name_;
    std::vector<std::string> args_;
};

enum class ConstType { default_, override_ };

auto operator<<(std::ostream &out, ConstType type) -> std::ostream &;

class StatementConst : public Statement {
  public:
    explicit StatementConst(ConstType type, std::string name, STerm value)
        : type_(type), name_(std::move(name)), value_(std::move(value)) {}

    void unpool(PoolStatement &pool) override;
    void print(std::ostream &out) const override;

  private:
    ConstType type_;
    std::string name_;
    STerm value_;
};
