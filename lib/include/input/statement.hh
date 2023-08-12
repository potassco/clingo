#pragma once

#include <input/body_literal.hh>
#include <input/head_literal.hh>

namespace Gringo::Input {

// TODO:
//   Add simple head/body literals

// TODO: rewrite arithmetic
// - p(1..X) -> Aux=1..X
// - p(X+Y)  -> Aux=X+Y
// - p(X-1)  -> fine!
// - X < Y < Z+B -> fine if conjunctive
// - not X < Y < Z+B -> Aux=Z+B if disjunctive
// - p(X), f(Y)=X, not q(Y)
//   - no need to add X=f(Y) because Y cannot be bound
//   - could be done by pruning equations

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

//! @defgroup input_statement Statements
//! @ingroup input_language
//!
//! Data structures and functions to represent statements.
//!
//! @{

//! A rule.
//!
//! For example: <tt>p(X) :- q(X)</tt>.
struct Rule {
    //! Construct a rule.
    explicit Rule(Location loc, HeadLiteral head, BodyLiteralVec body)
        : loc{std::move(loc)}, head{std::move(head)}, body{std::move(body)} {}

    //! The location of the rule.
    Location loc;
    //! The head.
    HeadLiteral head;
    //! The body.
    BodyLiteralVec body;
};

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
struct TheoryOpDefinition {
    //! Construct a theory operator definition.
    explicit TheoryOpDefinition(Location loc, std::string op, int prio, TheoryOpType type)
        : loc{std::move(loc)}, op{std::move(op)}, prio{prio}, type{type} {}

    //! The location of the definition.
    Location loc;
    //! The representation of the operator.
    std::string op;
    //! The priority of the operator.
    int prio;
    //! The type of the operator.
    TheoryOpType type;
};

//! A vector of theory operator definitions.
//!
//! @related TheoryOpDefinition
using TheoryOpDefinitionVec = std::vector<TheoryOpDefinition>;

//! A theory term definition.
//!
//! For example: <tt>term { - : 0, unary }</tt>.
struct TheoryTermDefinition {
    //! Construct a theory term definition.
    explicit TheoryTermDefinition(Location loc, std::string name, TheoryOpDefinitionVec op_defs)
        : loc{std::move(loc)}, name{std::move(name)}, op_defs{std::move(op_defs)} {}

    //! The location of the definition.
    Location loc;
    //! The name of the definition.
    std::string name;
    //! The associated operator definitions.
    std::vector<TheoryOpDefinition> op_defs;
};

//! A vector of theory term definitions.
//!
//! @related TheoryTermDefinition
using TheoryTermDefinitionVec = std::vector<TheoryTermDefinition>;

//! Enumeration of theory atom types.
//!
//! @see TheoryAtomDefinition
enum class TheoryAtomType {
    head,     //!< A theory atom that can appear only in the head.
    body,     //!< A theory atom that can appear only in the body.
    any,      //!< A theory atom that can appear only in the head and a body.
    directive //!< A theory atom that can appear only in the head with an empty body.
};

//! A theory atom definition.
//!
//! For example: <tt>name/2: term, any</tt>.
struct TheoryAtomDefinition {
    //! An optional definition for the right-hand-side of a theory atom.
    //!
    //! It consists of a list of possible operators and a name of a term definition.
    using RHS = std::optional<std::pair<std::vector<std::string>, std::string>>;

    //! Construct a theory atom definition.
    explicit TheoryAtomDefinition(Location loc, std::string name, int arity, std::string term, RHS rhs,
                                  TheoryAtomType type)
        : loc{std::move(loc)}, name(std::move(name)), arity(arity), term(std::move(term)), rhs(std::move(rhs)),
          type(type) {}

    //! The location of the definition.
    Location loc;
    //! The name of the atom.
    std::string name;
    //! The arity of the atom.
    int arity;
    //! The name of the term definition used in elements.
    std::string term;
    //! The definition for the right hand side of the atom.
    RHS rhs;
    //! The type of the atom.
    TheoryAtomType type;
};

//! A vector of theory atom definitions.
//!
//! @related TheoryAtomDefinition
using TheoryAtomDefinitionVec = std::vector<TheoryAtomDefinition>;

//! A theory definition.
//!
//! For example: <tt>\#theory name { term { - : 0, unary }; name/2: term, any }</tt>.
struct TheoryDefinition {
    //! Construct a theory definition.
    explicit TheoryDefinition(Location loc, std::string name, TheoryTermDefinitionVec term_defs,
                              TheoryAtomDefinitionVec atom_defs)
        : loc{std::move(loc)}, name{std::move(name)}, term_defs{std::move(term_defs)}, atom_defs{std::move(atom_defs)} {
    }

    //! The location of the definition.
    Location loc;
    //! The name of the definition.
    std::string name;
    //! The theory term definitions.
    TheoryTermDefinitionVec term_defs;
    //! The theory atom definitions.
    TheoryAtomDefinitionVec atom_defs;
};

//! Enumeration of optimization statement types.
//!
//! @related StatementOptimize
enum class OptimizeType { minimize, maximize };

//! An optimization statement.
//!
//! For example: <tt>\#minimize { 1@0,X: p(X) }</tt>.
struct StatementOptimize {
    //! The tuple of a minimize element.
    struct Tuple {
        //! The weight.
        Term weight;
        //! The optional priority.
        std::optional<Term> priority;
        //! The remaining terms.
        TermVec terms;
    };
    //! An element.
    using Element = std::pair<Tuple, LiteralVec>;
    //! A vector of elements.
    using ElementVec = std::vector<Element>;

    //! Construct a weak constraint.
    explicit StatementOptimize(Location loc, OptimizeType type, ElementVec elems)
        : loc{std::move(loc)}, type{type}, elems{std::move(elems)} {}

    //! The location of the statement.
    Location loc;
    //! The type of the statement.
    OptimizeType type;
    //! The elements of the statement.
    ElementVec elems;
};

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
struct StatementWeakConstraint {
    //! A weak constraint uses the same tuple as an optimize statement.
    using Tuple = StatementOptimize::Tuple;

    //! Construct a weak constraint.
    explicit StatementWeakConstraint(Location loc, BodyLiteralVec body, Tuple tuple)
        : loc{std::move(loc)}, body{std::move(body)}, tuple{std::move(tuple)} {}

    //! The location of the statement.
    Location loc;
    //! The body of the constraint.
    BodyLiteralVec body;
    //! The tuple of the constraint.
    Tuple tuple;
};

//! A show statement.
//!
//! Example: <tt>\#show p(X): q(X)</tt>.
struct StatementShow {
    //! Construct a show statement.
    explicit StatementShow(Location loc, Term term, BodyLiteralVec body)
        : loc{std::move(loc)}, term(std::move(term)), body(std::move(body)) {}

    //! The location of the statement.
    Location loc;
    //! The term to show.
    Term term;
    //! The body.
    BodyLiteralVec body;
};

//! A show signature statement.
//!
//! Example: <tt>\#show p/2</tt>.
struct StatementShowSig {
    //! Construct a show signature statement.
    explicit StatementShowSig(Location loc, bool has_sign, std::string name, int arity)
        : loc{std::move(loc)}, has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    //! The location of the statement.
    Location loc;
    //! Whether the signature is negative.
    bool has_sign;
    //! The name.
    std::string name;
    //! The arity.
    int arity;
};

//! A project statement.
//!
//! Example: <tt>\#project p(X): q(X)</tt>.
struct StatementProject {
    //! Construct a project statement.
    explicit StatementProject(Location loc, Term term, BodyLiteralVec body)
        : loc{std::move(loc)}, term(std::move(term)), body(std::move(body)) {}

    //! The location of the statement.
    Location loc;
    //! The term representing the atom to project.
    Term term;
    //! The body.
    BodyLiteralVec body;
};

//! A project signature statement.
//!
//! Example: <tt>\#project p/2</tt>.
struct StatementProjectSig {
    //! Construct a project signature statement.
    explicit StatementProjectSig(Location loc, bool has_sign, std::string name, int arity)
        : loc{std::move(loc)}, has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    //! The location of the statement.
    Location loc;
    //! Whether the signature is negative.
    bool has_sign;
    //! The name.
    std::string name;
    //! The arity.
    int arity;
};

//! A defined statement.
//!
//! Example: <tt>\#defined p/2</tt>.
struct StatementDefined {
    //! Construct a defined statement.
    explicit StatementDefined(Location loc, bool has_sign, std::string name, int arity)
        : loc{std::move(loc)}, has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    //! The location of the statement.
    Location loc;
    //! Whether the signature is negative.
    bool has_sign;
    //! The name.
    std::string name;
    //! The arity.
    int arity;
};

//! An external statement.
//!
//! Example: <tt>\#external p(X): q(X)</tt>.
struct StatementExternal {
    //! Construct an external statement.
    explicit StatementExternal(Location loc, Term term, BodyLiteralVec body, std::optional<Term> type = std::nullopt)
        : loc{std::move(loc)}, term(std::move(term)), body(std::move(body)), type{std::move(type)} {}

    //! The location of the statement.
    Location loc;
    //! The term representing the atom to project.
    Term term;
    //! The body.
    BodyLiteralVec body;
    //! The type of the statement.
    std::optional<Term> type;
};

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
struct StatementEdge {
    //! An directed edge.
    struct Edge {
        //! The source vertex.
        Term u;
        //! The target vertex.
        Term v;
    };
    //! A vector of edges.
    using EdgeVec = std::vector<Edge>;

    //! Construct an edge statement.
    explicit StatementEdge(Location loc, EdgeVec edges, BodyLiteralVec body = {})
        : loc{std::move(loc)}, edges{std::move(edges)}, body{std::move(body)} {}

    //! The location of the statement.
    Location loc;
    //! The pool of edges.
    EdgeVec edges;
    //! The body.
    BodyLiteralVec body;
};

//! A heuristic statement.
//!
//! Example: <tt>\#heuristic p(X). [sign,false]</tt>.
struct StatementHeuristic {
    //! Construct a heuristic statement.
    explicit StatementHeuristic(Location loc, Term atom, BodyLiteralVec body, Term type, std::optional<Term> prio,
                                Term mod)
        : loc{std::move(loc)}, atom{std::move(atom)}, body{std::move(body)}, type(std::move(type)),
          prio(std::move(prio)), mod(std::move(mod)) {}
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
    Location loc;
    //! The atom to modify.
    Term atom;
    //! The body.
    BodyLiteralVec body;
    //! The type.
    Term type;
    //! The optional priority.
    std::optional<Term> prio;
    //! The modifier.
    Term mod;
};

//! Enumeration of script types.
//!
//! @see StatementScript
enum class ScriptType {
    lua,    //!< Lua code.
    python, //!< Python code.
};

//! A script statement.
//!
//! For example: <tt>\#script(python) some code \#end</tt>.
struct StatementScript {
    //! Construct a script statement.
    explicit StatementScript(Location loc, ScriptType type, std::string content)
        : loc{std::move(loc)}, type(type), content(std::move(content)) {}

    //! The location of the statement.
    Location loc;
    //! The code type.
    ScriptType type;
    //! The code.
    std::string content;
};

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
struct StatementInclude {
    //! Construct an include statement.
    explicit StatementInclude(Location loc, IncludeType type, std::string path)
        : loc{std::move(loc)}, type(type), path(std::move(path)) {}

    //! The location of the statement.
    Location loc;
    //! The include type.
    IncludeType type;
    //! The path.
    std::string path;
};

//! A program statement.
//!
//! For example: <tt>\#program check(t)"</tt>.
struct StatementProgram {
    //! Construct an program statement.
    explicit StatementProgram(Location loc, std::string name, std::vector<std::string> args)
        : loc{std::move(loc)}, name(std::move(name)), args(std::move(args)) {}

    //! The location of the statement.
    Location loc;
    //! The name of the program.
    std::string name;
    //! The arguments of the program.
    std::vector<std::string> args;
};

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
struct StatementConst {
    //! Construct a const statement.
    explicit StatementConst(Location loc, ConstType type, std::string name, Term value)
        : loc{std::move(loc)}, type(type), name(std::move(name)), value(std::move(value)) {}

    //! The location of the statement.
    Location loc;
    //! The type of the statement.
    ConstType type;
    //! The name of the constant.
    std::string name;
    //! The value of the constant
    //!
    //! @todo This should become a symbol at some point.
    Term value;
};

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
struct Comment {
    //! Construct a comment.
    explicit Comment(Location loc, CommentType type, std::string value)
        : loc{std::move(loc)}, type{type}, value{std::move(value)} {}

    //! The location of the statement.
    Location loc;
    //! The type of the comment.
    CommentType type;
    //! The content of the comment including comment markers.
    std::string value;
};

//! Variant of available statements.
using Statement =
    std::variant<Rule, TheoryDefinition, StatementOptimize, StatementWeakConstraint, StatementShow, StatementShowSig,
                 StatementProject, StatementProjectSig, StatementDefined, StatementExternal, StatementEdge,
                 StatementHeuristic, StatementScript, StatementInclude, StatementProgram, StatementConst, Comment>;
//! A vector of statements.
using StatementVec = std::vector<Statement>;

//! @}

} // namespace Gringo::Input
