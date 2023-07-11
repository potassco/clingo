#pragma once

#include <memory>

#include <body_literal.hh>
#include <head_literal.hh>

class Statement;
using SStatement = shared_ptr<Statement>;
using SStatementVec = std::vector<SStatement>;

// TODO 1:
// - comparison literals should be normalized
//   - can expand disjunctively and conjunctively
//   - they can appear in bodies as well as nested elements
//   - intervals have to be removed from them
//     because unpooling duplicates terms
//   - unpooling non-binary relation literals requires removing non-singular pools
//     - for example `not (3 < 2 < 1+a)` should be false but gringo makes it true atm
// - aggregates have to be rewritten befor unpooling relation literals!
//   - set aggregates become head/body aggregates
//   - symbolic:
//     - {     p(X): C } -> #count { 0,p(X):         p(X), C }
//     - { not p(X): C } -> #count { 1,p(X):     not p(X), C }
//     - {        L: C } -> #count { #i+2,vars(L):        E, C }
// - shift/rewriting of literals
//   - to correctly handle pools, we can in general not shift in conditional literals
//     - it's probably easiest to just handle the general case
//   - we can shift into the rule body if literals do not have conditions
//   - example that can no longer be represented in standard format
//     - head :- p(G..L) : q(L); r(G).
//       - the current approach is to extend the syntax
//       - head :- #and{ p(Aux), Aux=G..L : q(L) }; r(G).
//     - head :- p(G+L) : q(L); r(G).
//       - head :- #and{ p(Aux), Aux=G+L : q(L) }; r(G).
//
// normalize_literal:
// - rewrite aggregates
//   - a single aggregate is rewritten
// - unpool_literals
//   - comparison literals can become conjunctions:
//     - a < b < c
//     - a < b && b < c
//   - comparison literals can become disjunctions:
//     - not a < b < c
//     - not a < b || not b < c
//   - unpool for literals can get context argument
//     - head, body, pool

// TODO 2:
// - terms should be normalized
// - all pools should have form: X = 1..Y
// - assignments should have form: unifiable = expression
// - assignments should be sets to avoid introducing duplicates
// - provisional name: normalize_terms (maybe cannot all be done in one function)

enum class RewriteLevel {
    disabled = 0,
    rewrite_anonymous = 1,
    unpool = 2,
    project = 3,
};

struct RewriteOptions {
    RewriteLevel level = RewriteLevel::project;
    ProjectionMode project_mode = ProjectionMode::pure;
    bool project_anonymous = false;
};

class Statement {
    friend shared_ptr<Statement>;

  public:
    virtual ~Statement() = default;

    virtual void print(std::ostream &out) const = 0;
    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;

    [[nodiscard]] auto to_string() const -> std::string;
    friend auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream &;
    [[nodiscard]] auto unpool() const -> std::optional<SStatementVec>;
    [[nodiscard]] auto project(ProjectionMode mode, bool project_anonymous) const -> std::optional<SStatement>;
    [[nodiscard]] virtual auto rewrite_anonymous() const -> std::optional<SStatement> = 0;

  protected:
    [[nodiscard]] virtual auto do_unpool() const -> std::optional<SStatementVec> = 0;
    [[nodiscard]] virtual auto do_project(ProjectionMode mode) const -> std::optional<SStatement> = 0;
    [[nodiscard]] virtual auto do_project_anonymous() const -> std::optional<SStatement> = 0;

  private:
    friend void inc_ref_count(Statement &stm) { ++stm.refs_; }
    friend void dec_ref_count(Statement &stm) { ++stm.refs_; }
    [[nodiscard]] friend auto get_ref_count(Statement const &stm) -> size_t { return stm.refs_; }

    size_t refs_ = 0;
};

void rewrite(SStatement stm, RewriteOptions opts, SStatementVec &stms);

class Rule : public Statement {
  public:
    explicit Rule(SHeadLiteral head, SBodyLiteralVec body) : head_{std::move(head)}, body_{std::move(body)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

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

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

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

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    OptimizeType type_;
    ElementVec elems_;
};

class StatementWeakConstraint : public Statement {
  public:
    using Tuple = StatementOptimize::Tuple;

    explicit StatementWeakConstraint(SBodyLiteralVec body, Tuple tuple)
        : body_{std::move(body)}, tuple_{std::move(tuple)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    SBodyLiteralVec body_;
    Tuple tuple_;
};

class StatementShow : public Statement {
  public:
    StatementShow(STerm term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    STerm term_;
    SBodyLiteralVec body_;
};

class StatementShowSig : public Statement {
  public:
    explicit StatementShowSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementProject : public Statement {
  public:
    explicit StatementProject(STerm term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    STerm term_;
    SBodyLiteralVec body_;
};

class StatementProjectSig : public Statement {
  public:
    explicit StatementProjectSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementDefined : public Statement {
  public:
    explicit StatementDefined(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    bool has_sign_;
    std::string name_;
    int arity_;
};

class StatementExternal : public Statement {
  public:
    explicit StatementExternal(STerm term, SBodyLiteralVec body, std::optional<STerm> type = std::nullopt)
        : term_(std::move(term)), body_(std::move(body)), type_{std::move(type)} {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

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

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    EdgeVec edges_;
    SBodyLiteralVec body_;
};

class StatementHeuristic : public Statement {
  public:
    explicit StatementHeuristic(STerm atom, SBodyLiteralVec body, STerm type, std::optional<STerm> prio, STerm mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), prio_(std::move(prio)),
          mod_(std::move(mod)) {}
    explicit StatementHeuristic(STerm atom, SBodyLiteralVec body, STerm type, STerm prio, STerm mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), prio_(std::move(prio)),
          mod_(std::move(mod)) {}
    explicit StatementHeuristic(STerm atom, SBodyLiteralVec body, STerm type, STerm mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), mod_(std::move(mod)) {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    STerm atom_;
    SBodyLiteralVec body_;
    STerm type_;
    std::optional<STerm> prio_;
    STerm mod_;
};

enum class ScriptType {
    lua,
    python,
};

auto operator<<(std::ostream &out, ScriptType type) -> std::ostream &;

class StatementScript : public Statement {
  public:
    explicit StatementScript(ScriptType type, std::string content) : type_(type), content_(std::move(content)) {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

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

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    IncludeType type_;
    std::string path_;
};

class StatementProgram : public Statement {
  public:
    explicit StatementProgram(std::string name, std::vector<std::string> args)
        : name_(std::move(name)), args_(std::move(args)) {}

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

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

    void print(std::ostream &out) const override;
    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;
    [[nodiscard]] auto rewrite_anonymous() const -> std::optional<SStatement> override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    ConstType type_;
    std::string name_;
    STerm value_;
};
