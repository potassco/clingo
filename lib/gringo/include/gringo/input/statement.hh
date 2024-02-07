#pragma once

#include <gringo/input/body_literal.hh>
#include <gringo/input/head_literal.hh>

namespace Gringo::Input {

//! @defgroup input_statement Statements
//! Data structures and functions to represent statements.
//!
//! @ingroup input_language
//!
//! @{

//! A rule.
//!
//! For example: <tt>p(X) :- q(X)</tt>.
class Rule {
  public:
    //! Construct a rule.
    explicit Rule(Location loc, HeadLiteral head, BodyLiteralVec body)
        : loc_{std::move(loc)}, head_{std::move(head)}, body_{std::move(body)} {}

    //! The location of the rule.
    Location loc_;
    //! The head.
    HeadLiteral head_;
    //! The body.
    BodyLiteralVec body_;
};

//! Compare two rules.
auto operator==(Rule const &a, Rule const &b) -> bool;

//! Compare two rules.
auto operator<(Rule const &a, Rule const &b) -> bool;

//! The type of a theory operator.
//!
//! @see TheoryOpDefinition
enum class TheoryOpType {
    unary,       //!< An unary theory operator.
    binary_left, //!< An binary left associative theory operator.
    binary_right //!< An binary right associative theory operator.
};

//! A theory operator definition.
//!
//! For example: <tt>- : 0, unary</tt>.
class TheoryOpDefinition {
  public:
    //! Construct a theory operator definition.
    explicit TheoryOpDefinition(Location loc, String op, int prio, TheoryOpType type)
        : loc_{std::move(loc)}, op_{op}, prio_{prio}, type_{type} {}

    //! The location of the definition.
    Location loc_;
    //! The representation of the operator.
    String op_;
    //! The priority of the operator.
    int prio_;
    //! The type of the operator.
    TheoryOpType type_;
};

//! Compare two theory operator definitions.
auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool;

//! Compare two theory operator definitions.
auto operator<(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool;

//! A vector of theory operator definitions.
using TheoryOpDefinitionVec = Util::immutable_array<TheoryOpDefinition>;

//! A theory term definition.
//!
//! For example: <tt>term { - : 0, unary }</tt>.
class TheoryTermDefinition {
  public:
    //! Construct a theory term definition.
    explicit TheoryTermDefinition(Location loc, String name, TheoryOpDefinitionVec op_defs)
        : loc_{std::move(loc)}, name_{name}, op_defs_{std::move(op_defs)} {}

    //! The location of the definition.
    Location loc_;
    //! The name of the definition.
    String name_;
    //! The associated operator definitions.
    Util::immutable_array<TheoryOpDefinition> op_defs_;
};

//! A vector of theory term definitions.
using TheoryTermDefinitionVec = Util::immutable_array<TheoryTermDefinition>;

//! Compare two theory term definitions.
auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool;

//! Compare two theory term definitions.
auto operator<(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool;

//! Enumeration of theory atom types.
//!
//! @see TheoryAtomDefinition
enum class TheoryAtomType {
    head,     //!< A theory atom that can appear only in the head.
    body,     //!< A theory atom that can appear only in the body.
    any,      //!< A theory atom that can appear only in the head and a body.
    directive //!< A theory atom that can appear only in the head with an empty body.
};

//! An optional definition for the right-hand-side of a theory atom.
//!
//! It consists of a list of possible operators and a name of a term definition.
using TheoryRGuardDefinition = std::pair<Util::immutable_array<String>, String>;

//! A theory atom definition.
//!
//! For example: <tt>name/2: term, any</tt>.
class TheoryAtomDefinition {
  public:
    //! Construct a theory atom definition.
    explicit TheoryAtomDefinition(Location loc, String name, int arity, String term,
                                  std::optional<TheoryRGuardDefinition> rhs, TheoryAtomType type)
        : loc_{std::move(loc)}, name_(name), arity_(arity), term_(term), rhs_(std::move(rhs)), type_(type) {}

    //! The location of the definition.
    Location loc_;
    //! The name of the atom.
    String name_;
    //! The arity of the atom.
    int arity_;
    //! The name of the term definition used in elements.
    String term_;
    //! The definition for the right hand side of the atom.
    std::optional<TheoryRGuardDefinition> rhs_;
    //! The type of the atom.
    TheoryAtomType type_;
};

//! A vector of theory atom definitions.
using TheoryAtomDefinitionVec = Util::immutable_array<TheoryAtomDefinition>;

//! Compare two theory atom definitions.
auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool;

//! Compare two theory atom definitions.
auto operator<(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool;

//! A theory definition.
//!
//! For example: <tt>\#theory name { term { - : 0, unary }; name/2: term, any }</tt>.
class TheoryDefinition {
  public:
    //! Construct a theory definition.
    explicit TheoryDefinition(Location loc, String name, TheoryTermDefinitionVec term_defs,
                              TheoryAtomDefinitionVec atom_defs)
        : loc_{std::move(loc)}, name_{name}, term_defs_{std::move(term_defs)}, atom_defs_{std::move(atom_defs)} {}

    //! The location of the definition.
    Location loc_;
    //! The name of the definition.
    String name_;
    //! The theory term definitions.
    TheoryTermDefinitionVec term_defs_;
    //! The theory atom definitions.
    TheoryAtomDefinitionVec atom_defs_;
};

//! Compare two theory definitions.
auto operator==(TheoryDefinition const &a, TheoryDefinition const &b) -> bool;

//! Compare two theory definitions.
auto operator<(TheoryDefinition const &a, TheoryDefinition const &b) -> bool;

//! Enumeration of optimization statement types.
//!
//! @related StatementOptimize
enum class OptimizeType { minimize, maximize };

//! An optimization statement.
//!
//! For example: <tt>\#minimize { 1@0,X: p(X) }</tt>.
class StatementOptimize {
  public:
    //! The tuple of a minimize element.
    class Tuple {
      public:
        explicit Tuple(Term weight, std::optional<Term> priority, TermVec terms)
            : weight_{std::move(weight)}, priority_{std::move(priority)}, terms_{std::move(terms)} {}
        //! The weight.
        Term weight_;
        //! The optional priority.
        std::optional<Term> priority_;
        //! The remaining terms.
        TermVec terms_;
    };
    //! An element.
    using Element = std::pair<Tuple, LiteralVec>;
    //! A vector of elements.
    using ElementVec = Util::immutable_array<Element>;

    //! Construct a weak constraint.
    explicit StatementOptimize(Location loc, OptimizeType type, ElementVec elems)
        : loc_{std::move(loc)}, type_{type}, elems_{std::move(elems)} {}

    //! The location of the statement.
    Location loc_;
    //! The type of the statement.
    OptimizeType type_;
    //! The elements of the statement.
    ElementVec elems_;
};

//! Compare two tuples.
auto operator==(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool;

//! Compare two optimization tuples.
auto operator<(StatementOptimize::Tuple const &a, StatementOptimize::Tuple const &b) -> bool;

//! Compare two optimization statements.
auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool;

//! Compare two optimization statements.
auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool;

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
class StatementWeakConstraint {
  public:
    //! A weak constraint uses the same tuple as an optimize statement.
    using Tuple = StatementOptimize::Tuple;

    //! Construct a weak constraint.
    explicit StatementWeakConstraint(Location loc, BodyLiteralVec body, Tuple tuple)
        : loc_{std::move(loc)}, body_{std::move(body)}, tuple_{std::move(tuple)} {}

    //! The location of the statement.
    Location loc_;
    //! The body of the constraint.
    BodyLiteralVec body_;
    //! The tuple of the constraint.
    Tuple tuple_;
};

//! Compare two weak constraints.
auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool;

//! Compare two weak constraints.
auto operator<(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool;

//! A show statement.
//!
//! Example: <tt>\#show p(X): q(X)</tt>.
class StatementShow {
  public:
    //! Construct a show statement.
    explicit StatementShow(Location loc, Term term, BodyLiteralVec body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    Location loc_;
    //! The term to show.
    Term term_;
    //! The body.
    BodyLiteralVec body_;
};

//! Compare two show statements.
auto operator==(StatementShow const &a, StatementShow const &b) -> bool;

//! Compare two show statements.
auto operator<(StatementShow const &a, StatementShow const &b) -> bool;

//! A show signature statement.
//!
//! Example: <tt>\#show p/2</tt>.
class StatementShowSig {
  public:
    //! Construct a show signature statement.
    explicit StatementShowSig(Location loc, bool has_sign, String name, int arity)
        : loc_{std::move(loc)}, has_sign_{has_sign}, name_{name}, arity_{arity} {}

    //! The location of the statement.
    Location loc_;
    //! Whether the signature is negative.
    bool has_sign_;
    //! The name.
    String name_;
    //! The arity.
    int arity_;
};

//! Compare two show statements.
auto operator==(StatementShowSig const &a, StatementShowSig const &b) -> bool;

//! Compare two show statements.
auto operator<(StatementShowSig const &a, StatementShowSig const &b) -> bool;

//! A project statement.
//!
//! Example: <tt>\#project p(X): q(X)</tt>.
class StatementProject {
  public:
    //! Construct a project statement.
    explicit StatementProject(Location loc, Term term, BodyLiteralVec body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    Location loc_;
    //! The term representing the atom to project.
    Term term_;
    //! The body.
    BodyLiteralVec body_;
};

//! Compare two project statements.
auto operator==(StatementProject const &a, StatementProject const &b) -> bool;

//! Compare two project statements.
auto operator<(StatementProject const &a, StatementProject const &b) -> bool;

//! A project signature statement.
//!
//! Example: <tt>\#project p/2</tt>.
class StatementProjectSig {
  public:
    //! Construct a project signature statement.
    explicit StatementProjectSig(Location loc, bool has_sign, String name, int arity)
        : loc_{std::move(loc)}, has_sign_{has_sign}, name_{name}, arity_{arity} {}

    //! The location of the statement.
    Location loc_;
    //! Whether the signature is negative.
    bool has_sign_;
    //! The name.
    String name_;
    //! The arity.
    int arity_;
};

//! Compare two project statements.
auto operator==(StatementProjectSig const &a, StatementProjectSig const &b) -> bool;

//! Compare two project statements.
auto operator<(StatementProjectSig const &a, StatementProjectSig const &b) -> bool;

//! A defined statement.
//!
//! Example: <tt>\#defined p/2</tt>.
class StatementDefined {
  public:
    //! Construct a defined statement.
    explicit StatementDefined(Location loc, bool has_sign, String name, int arity)
        : loc_{std::move(loc)}, has_sign_{has_sign}, name_{name}, arity_{arity} {}

    //! The location of the statement.
    Location loc_;
    //! Whether the signature is negative.
    bool has_sign_;
    //! The name.
    String name_;
    //! The arity.
    int arity_;
};

//! Compare two defined statements.
auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool;

//! Compare two defined statements.
auto operator<(StatementDefined const &a, StatementDefined const &b) -> bool;

//! An external statement.
//!
//! Example: <tt>\#external p(X): q(X)</tt>.
class StatementExternal {
  public:
    //! Construct an external statement.
    explicit StatementExternal(Location loc, Term term, BodyLiteralVec body, std::optional<Term> type = std::nullopt)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)), type_{std::move(type)} {}

    //! The location of the statement.
    Location loc_;
    //! The term representing the atom to project.
    Term term_;
    //! The body.
    BodyLiteralVec body_;
    //! The type of the statement.
    std::optional<Term> type_;
};

//! Compare two external statements.
auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool;

//! Compare two external statements.
auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool;

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
class StatementEdge {
  public:
    //! An directed edge.
    class Edge {
      public:
        explicit Edge(Term u, Term v) : u_{std::move(u)}, v_{std::move(v)} {}
        //! The source vertex.
        Term u_;
        //! The target vertex.
        Term v_;
    };
    //! A vector of edges.
    using EdgeVec = Util::immutable_array<Edge>;

    //! Construct an edge statement.
    explicit StatementEdge(Location loc, EdgeVec edges, BodyLiteralVec body = {})
        : loc_{std::move(loc)}, edges_{std::move(edges)}, body_{std::move(body)} {}

    //! The location of the statement.
    Location loc_;
    //! The pool of edges.
    EdgeVec edges_;
    //! The body.
    BodyLiteralVec body_;
};

//! Compare two edges.
auto operator==(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool;

//! Compare two edges.
auto operator<(StatementEdge::Edge const &a, StatementEdge::Edge const &b) -> bool;

//! Compare two edge statements.
auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool;

//! Compare two edge statements.
auto operator<(StatementEdge const &a, StatementEdge const &b) -> bool;

//! A heuristic statement.
//!
//! Example: <tt>\#heuristic p(X). [sign,false]</tt>.
class StatementHeuristic {
  public:
    //! Construct a heuristic statement.
    explicit StatementHeuristic(Location loc, Term atom, BodyLiteralVec body, Term type, std::optional<Term> prio,
                                Term mod)
        : loc_{std::move(loc)}, atom_{std::move(atom)}, body_{std::move(body)}, type_(std::move(type)),
          prio_(std::move(prio)), mod_(std::move(mod)) {}
    //! Construct a heuristic statement.
    explicit StatementHeuristic(Location loc, Term atom, BodyLiteralVec body, Term type, Term prio, Term mod)
        : StatementHeuristic{
              std::move(loc), std::move(atom), std::move(body), std::move(type), std::make_optional(std::move(prio)),
              std::move(mod)} {}
    //! Construct a heuristic statement.
    explicit StatementHeuristic(Location loc, Term atom, BodyLiteralVec body, Term type, Term mod)
        : StatementHeuristic{std::move(loc),  std::move(atom), std::move(body),
                             std::move(type), std::nullopt,    std::move(mod)} {}

    //! The location of the statement.
    Location loc_;
    //! The atom to modify.
    Term atom_;
    //! The body.
    BodyLiteralVec body_;
    //! The type.
    Term type_;
    //! The optional priority.
    std::optional<Term> prio_;
    //! The modifier.
    Term mod_;
};

//! Compare two heuristic statements.
auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool;

//! Compare two heuristic statements.
auto operator<(StatementHeuristic const &a, StatementHeuristic const &b) -> bool;

//! A script statement.
//!
//! For example: <tt>\#script(python) some code \#end</tt>.
class StatementScript {
  public:
    //! Construct a script statement.
    explicit StatementScript(Location loc, String type, std::string content)
        : loc_{std::move(loc)}, type_(type), content_(std::move(content)) {}

    //! The location of the statement.
    Location loc_;
    //! The code type.
    String type_;
    //! The code.
    std::string content_;
};

//! Compare two script statements.
auto operator==(StatementScript const &a, StatementScript const &b) -> bool;

//! Compare two script statements.
auto operator<(StatementScript const &a, StatementScript const &b) -> bool;

//! Enumeration of include types.
//!
//! @related StatementInclude
enum class IncludeType {
    system,  //!< Include from path.
    inbuild, //!< Include inbuilt script.
};

//! An include statement.
//!
//! For example: <tt>\#include "encoding.lp"</tt>.
class StatementInclude {
  public:
    //! Construct an include statement.
    explicit StatementInclude(Location loc, IncludeType type, std::string path)
        : loc_{std::move(loc)}, type_(type), path_(std::move(path)) {}

    //! The location of the statement.
    Location loc_;
    //! The include type.
    IncludeType type_;
    //! The path.
    std::string path_;
};

//! Compare two include statements.
auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool;

//! Compare two include statements.
auto operator<(StatementInclude const &a, StatementInclude const &b) -> bool;

//! A program statement.
//!
//! For example: <tt>\#program check(t)"</tt>.
class StatementProgram {
  public:
    //! Construct an program statement.
    explicit StatementProgram(Location loc, String name, Util::immutable_array<String> args)
        : loc_{std::move(loc)}, name_(name), args_(std::move(args)) {}

    //! The location of the statement.
    Location loc_;
    //! The name of the program.
    String name_;
    //! The arguments of the program.
    Util::immutable_array<String> args_;
};

//! Compare two program statements.
auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool;

//! Compare two program statements.
auto operator<(StatementProgram const &a, StatementProgram const &b) -> bool;

//! Enumeration of constant statement types.
//!
//! @see StatementConst
enum class ConstType {
    default_, //! A statement providing default value.
    override_ //! A statement overriding a default value.
};

//! A const statement.
//!
//! For example: <tt>\#const n=42</tt>.
class StatementConst {
  public:
    //! Construct a const statement.
    explicit StatementConst(Location loc, ConstType type, String name, Term value)
        : loc_{std::move(loc)}, type_(type), name_(name), value_(std::move(value)) {}

    //! The location of the statement.
    Location loc_;
    //! The type of the statement.
    ConstType type_;
    //! The name of the constant.
    String name_;
    //! The value of the constant
    Term value_;
};

//! Compare two const statements.
auto operator==(StatementConst const &a, StatementConst const &b) -> bool;

//! Compare two const statements.
auto operator<(StatementConst const &a, StatementConst const &b) -> bool;

//! Enumeration of comment types.
//!
//! @related Comment
enum class CommentType {
    line,  //!< A newline terminated comment starting with <tt>%</tt>.
    block, //!< A block comment enclosed in <tt>%*</tt> and <tt>*%</tt>.
};

//! A commment.
//!
//! For example: <tt>%* comment *%</tt>
class Comment {
  public:
    //! Construct a comment.
    explicit Comment(Location loc, CommentType type, std::string value)
        : loc_{std::move(loc)}, type_{type}, value_{std::move(value)} {}

    //! The location of the statement.
    Location loc_;
    //! The type of the comment.
    CommentType type_;
    //! The content of the comment including comment markers.
    std::string value_;
};

//! Compare two comments.
auto operator==(Comment const &a, Comment const &b) -> bool;

//! Compare two comments.
auto operator<(Comment const &a, Comment const &b) -> bool;

//! Variant of available statements.
using Statement =
    std::variant<Rule, TheoryDefinition, StatementOptimize, StatementWeakConstraint, StatementShow, StatementShowSig,
                 StatementProject, StatementProjectSig, StatementDefined, StatementExternal, StatementEdge,
                 StatementHeuristic, StatementScript, StatementInclude, StatementProgram, StatementConst, Comment>;
//! A vector of statements.
using StatementVec = std::vector<Statement>;

//! @}

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TheoryOpDefinition);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermDefinition);
GRINGO_HASH_PROTO(Gringo::Input::TheoryAtomDefinition);
GRINGO_HASH_PROTO(Gringo::Input::TheoryDefinition);
GRINGO_HASH_PROTO(Gringo::Input::StatementOptimize::Tuple);
GRINGO_HASH_PROTO(Gringo::Input::StatementEdge::Edge);
GRINGO_HASH_PROTO(Gringo::Input::Rule);
GRINGO_HASH_PROTO(Gringo::Input::StatementOptimize);
GRINGO_HASH_PROTO(Gringo::Input::StatementWeakConstraint);
GRINGO_HASH_PROTO(Gringo::Input::StatementShow);
GRINGO_HASH_PROTO(Gringo::Input::StatementShowSig);
GRINGO_HASH_PROTO(Gringo::Input::StatementProject);
GRINGO_HASH_PROTO(Gringo::Input::StatementProjectSig);
GRINGO_HASH_PROTO(Gringo::Input::StatementDefined);
GRINGO_HASH_PROTO(Gringo::Input::StatementExternal);
GRINGO_HASH_PROTO(Gringo::Input::StatementEdge);
GRINGO_HASH_PROTO(Gringo::Input::StatementHeuristic);
GRINGO_HASH_PROTO(Gringo::Input::StatementScript);
GRINGO_HASH_PROTO(Gringo::Input::StatementInclude);
GRINGO_HASH_PROTO(Gringo::Input::StatementProgram);
GRINGO_HASH_PROTO(Gringo::Input::StatementConst);
GRINGO_HASH_PROTO(Gringo::Input::Comment);

#endif
