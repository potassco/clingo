#pragma once

#include <memory>

#include <input/body_literal.hh>
#include <input/head_literal.hh>

namespace Gringo::Input {

class Statement;
class StatementVisitor;
using SStatement = Util::shared_ptr<Statement>;
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
  public:
    virtual ~Statement() = default;

    virtual void visit_variables(VarVisitFun const &fun, VariableContext ctx) const = 0;

    [[nodiscard]] auto unpool() const -> std::optional<SStatementVec>;
    [[nodiscard]] auto project(ProjectionMode mode, bool project_anonymous) const -> std::optional<SStatement>;

    //! Visit statements with the given visitor.
    virtual void accept(StatementVisitor const &visitor) const = 0;

  protected:
    [[nodiscard]] virtual auto do_unpool() const -> std::optional<SStatementVec> = 0;
    [[nodiscard]] virtual auto do_project(ProjectionMode mode) const -> std::optional<SStatement> = 0;
    [[nodiscard]] virtual auto do_project_anonymous() const -> std::optional<SStatement> = 0;
};

void rewrite(SStatement stm, RewriteOptions opts, SStatementVec &stms);

class Rule : public Statement {
  public:
    explicit Rule(SHeadLiteral head, SBodyLiteralVec body) : head_{std::move(head)}, body_{std::move(body)} {}

    //! Get the head of the rule.
    [[nodiscard]] auto head() const -> SHeadLiteral const & { return head_; }
    //! Get the body of the rule.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

    //! Get the theory operator.
    [[nodiscard]] auto theory_operator() const -> std::string const & { return op_; }
    //! Get the priority.
    [[nodiscard]] auto priority() const -> int { return prio_; }
    //! Get the type.
    [[nodiscard]] auto type() const -> TheoryOpType { return type_; }

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

    //! Get the name of the definition.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the operator definitions.
    [[nodiscard]] auto operator_definitions() const -> TheoryOpDefinitionVec const & { return op_defs_; }

  private:
    std::string name_;
    std::vector<TheoryOpDefinition> op_defs_;
};

using TheoryTermDefinitionVec = std::vector<TheoryTermDefinition>;

enum class TheoryAtomType { head, body, any, directive };

class TheoryAtomDefinition {
  public:
    using RHS = std::optional<std::pair<std::vector<std::string>, std::string>>;

    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, RHS rhs, TheoryAtomType type)
        : name_(std::move(name)), arity_(arity), term_(std::move(term)), rhs_(std::move(rhs)), type_(type) {}

    //! Get the name of the definition.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the arity of the theory atom.
    [[nodiscard]] auto arity() const -> int { return arity_; }
    //! Get the name of the term definition for elements.
    [[nodiscard]] auto term() const -> std::string const & { return term_; }
    //! Get the definition of the right-hand-side.
    [[nodiscard]] auto rhs() const -> RHS const & { return rhs_; }
    //! Get the type of the definition.
    [[nodiscard]] auto type() const -> TheoryAtomType { return type_; }

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

    //! Get the name of the definition.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the term definitions.
    [[nodiscard]] auto term_defs() const -> TheoryTermDefinitionVec const & { return term_defs_; }
    //! Get the atom definitions.
    [[nodiscard]] auto atom_defs() const -> TheoryAtomDefinitionVec const & { return atom_defs_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

class StatementOptimize : public Statement {
  public:
    using Tuple = std::tuple<Term, std::optional<Term>, TermVec>;
    using Element = std::pair<Tuple, SLiteralVec>;
    using ElementVec = std::vector<Element>;

    explicit StatementOptimize(OptimizeType type, ElementVec elems) : type_{type}, elems_{std::move(elems)} {}

    //! Get the type.
    [[nodiscard]] auto type() const -> OptimizeType { return type_; }
    //! Get the elements.
    [[nodiscard]] auto elements() const -> ElementVec const & { return elems_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }
    //! Get the elements.
    [[nodiscard]] auto tuple() const -> Tuple const & { return tuple_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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
    StatementShow(Term term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    //! Get the term.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    Term term_;
    SBodyLiteralVec body_;
};

class StatementShowSig : public Statement {
  public:
    explicit StatementShowSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    //! Check if the signature has a sign.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! Get the name.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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
    explicit StatementProject(Term term, SBodyLiteralVec body) : term_(std::move(term)), body_(std::move(body)) {}

    //! Get the term.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    Term term_;
    SBodyLiteralVec body_;
};

class StatementProjectSig : public Statement {
  public:
    explicit StatementProjectSig(bool has_sign, std::string name, int arity)
        : has_sign_{has_sign}, name_{std::move(name)}, arity_{arity} {}

    //! Check if the signature has a sign.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! Get the name.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

    //! Check if the signature has a sign.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! Get the name.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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
    explicit StatementExternal(Term term, SBodyLiteralVec body, std::optional<Term> type = std::nullopt)
        : term_(std::move(term)), body_(std::move(body)), type_{std::move(type)} {}

    //! Get the term.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }
    //! Get the type.
    [[nodiscard]] auto type() const -> std::optional<Term> const & { return type_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    Term term_;
    SBodyLiteralVec body_;
    std::optional<Term> type_;
};

class StatementEdge : public Statement {
  public:
    using Edge = std::pair<Term, Term>;
    using EdgeVec = std::vector<Edge>;

    explicit StatementEdge(EdgeVec edges, SBodyLiteralVec body = {})
        : edges_{std::move(edges)}, body_{std::move(body)} {}

    //! Get the edges.
    [[nodiscard]] auto edges() const -> EdgeVec const & { return edges_; }
    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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
    explicit StatementHeuristic(Term atom, SBodyLiteralVec body, Term type, std::optional<Term> prio, Term mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), prio_(std::move(prio)),
          mod_(std::move(mod)) {}
    explicit StatementHeuristic(Term atom, SBodyLiteralVec body, Term type, Term prio, Term mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), prio_(std::move(prio)),
          mod_(std::move(mod)) {}
    explicit StatementHeuristic(Term atom, SBodyLiteralVec body, Term type, Term mod)
        : atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)), mod_(std::move(mod)) {}

    //! Get the atom.
    [[nodiscard]] auto atom() const -> Term const & { return atom_; }
    //! Get the body.
    [[nodiscard]] auto body() const -> SBodyLiteralVec const & { return body_; }
    //! Get the type.
    [[nodiscard]] auto type() const -> Term const & { return type_; }
    //! Get the priority.
    [[nodiscard]] auto priority() const -> std::optional<Term> const & { return prio_; }
    //! Get the modifier.
    [[nodiscard]] auto modifier() const -> Term const & { return mod_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    Term atom_;
    SBodyLiteralVec body_;
    Term type_;
    std::optional<Term> prio_;
    Term mod_;
};

enum class ScriptType {
    lua,
    python,
};

class StatementScript : public Statement {
  public:
    explicit StatementScript(ScriptType type, std::string content) : type_(type), content_(std::move(content)) {}

    //! Get the type.
    [[nodiscard]] auto type() const -> ScriptType { return type_; }
    //! Get the content.
    [[nodiscard]] auto content() const -> std::string const & { return content_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

class StatementInclude : public Statement {
  public:
    explicit StatementInclude(IncludeType type, std::string path) : type_(type), path_(std::move(path)) {}

    //! Get the type.
    [[nodiscard]] auto type() const -> IncludeType { return type_; }
    //! Get the path.
    [[nodiscard]] auto path() const -> std::string const & { return path_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

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

    //! Get the name.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the path.
    [[nodiscard]] auto arguments() const -> std::vector<std::string> const & { return args_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    std::string name_;
    std::vector<std::string> args_;
};

enum class ConstType { default_, override_ };

class StatementConst : public Statement {
  public:
    explicit StatementConst(ConstType type, std::string name, Term value)
        : type_(type), name_(std::move(name)), value_(std::move(value)) {}

    //! Get the type.
    [[nodiscard]] auto type() const -> ConstType { return type_; }
    //! Get the name.
    [[nodiscard]] auto name() const -> std::string const & { return name_; }
    //! Get the value.
    [[nodiscard]] auto value() const -> Term const & { return value_; }

    void visit_variables(VarVisitFun const &fun, VariableContext ctx) const override;

    void accept(StatementVisitor const &visitor) const override;

  protected:
    [[nodiscard]] auto do_unpool() const -> std::optional<SStatementVec> override;
    [[nodiscard]] auto do_project(ProjectionMode mode) const -> std::optional<SStatement> override;
    [[nodiscard]] auto do_project_anonymous() const -> std::optional<SStatement> override;

  private:
    ConstType type_;
    std::string name_;
    Term value_;
};

//! A visitor for available term types.
class StatementVisitor {
  public:
    //! Virtual destructor.
    virtual ~StatementVisitor() = default;

    //! Visit a rule.
    virtual void visit(Rule const &stm) const = 0;
    //! Visit a theory definition.
    virtual void visit(TheoryDefinition const &stm) const = 0;
    //! Visit an optimize statement.
    virtual void visit(StatementOptimize const &stm) const = 0;
    //! Visit a weak constraint.
    virtual void visit(StatementWeakConstraint const &stm) const = 0;
    //! Visit a show statement.
    virtual void visit(StatementShow const &stm) const = 0;
    //! Visit a show signature statement.
    virtual void visit(StatementShowSig const &stm) const = 0;
    //! Visit a project statement.
    virtual void visit(StatementProject const &stm) const = 0;
    //! Visit a project signature statement.
    virtual void visit(StatementProjectSig const &stm) const = 0;
    //! Visit a defined statement.
    virtual void visit(StatementDefined const &stm) const = 0;
    //! Visit an external statement.
    virtual void visit(StatementExternal const &stm) const = 0;
    //! Visit an edge statement.
    virtual void visit(StatementEdge const &stm) const = 0;
    //! Visit an heuristic statement.
    virtual void visit(StatementHeuristic const &stm) const = 0;
    //! Visit a script statement.
    virtual void visit(StatementScript const &stm) const = 0;
    //! Visit an include statement.
    virtual void visit(StatementInclude const &stm) const = 0;
    //! Visit a program statement.
    virtual void visit(StatementProgram const &stm) const = 0;
    //! Visit a const statement.
    virtual void visit(StatementConst const &stm) const = 0;
};

} // namespace Gringo::Input
