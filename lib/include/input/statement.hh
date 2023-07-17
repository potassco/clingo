#pragma once

#include <input/body_literal.hh>
#include <input/head_literal.hh>

namespace Gringo::Input {

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

struct Rule {
    explicit Rule(HeadLiteral head, BodyLiteralVec body) : head{std::move(head)}, body{std::move(body)} {}

    HeadLiteral head;
    BodyLiteralVec body;
};

enum class TheoryOpType { unary, binary_left, binary_right };

struct TheoryOpDefinition {
    explicit TheoryOpDefinition(std::string op, int prio, TheoryOpType type)
        : op{std::move(op)}, prio{prio}, type{type} {}

    std::string op;
    int prio;
    TheoryOpType type;
};

using TheoryOpDefinitionVec = std::vector<TheoryOpDefinition>;

struct TheoryTermDefinition {
    explicit TheoryTermDefinition(std::string name, TheoryOpDefinitionVec op_defs)
        : name{std::move(name)}, op_defs{std::move(op_defs)} {}

    std::string name;
    std::vector<TheoryOpDefinition> op_defs;
};

using TheoryTermDefinitionVec = std::vector<TheoryTermDefinition>;

enum class TheoryAtomType { head, body, any, directive };

struct TheoryAtomDefinition {
    using RHS = std::optional<std::pair<std::vector<std::string>, std::string>>;

    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, RHS rhs, TheoryAtomType type)
        : name(std::move(name)), arity(arity), term(std::move(term)), rhs(std::move(rhs)), type(type) {}

    std::string name;
    int arity;
    std::string term;
    RHS rhs;
    TheoryAtomType type;
};
using TheoryAtomDefinitionVec = std::vector<TheoryAtomDefinition>;

struct TheoryDefinition {
    explicit TheoryDefinition(std::string name, TheoryTermDefinitionVec term_defs, TheoryAtomDefinitionVec atom_defs)
        : name{std::move(name)}, term_defs{std::move(term_defs)}, atom_defs{std::move(atom_defs)} {}

    std::string name;
    TheoryTermDefinitionVec term_defs;
    TheoryAtomDefinitionVec atom_defs;
};

enum class OptimizeType { minimize, maximize };

struct StatementOptimize {
    struct Tuple {
        Term weight;
        std::optional<Term> priority;
        TermVec terms;
    };
    using Element = std::pair<Tuple, LiteralVec>;
    using ElementVec = std::vector<Element>;

    OptimizeType type;
    ElementVec elems;
};

struct StatementWeakConstraint {
    using Tuple = StatementOptimize::Tuple;

    explicit StatementWeakConstraint(BodyLiteralVec body, Tuple tuple)
        : body{std::move(body)}, tuple{std::move(tuple)} {}

    BodyLiteralVec body;
    Tuple tuple;
};

struct StatementShow {
    explicit StatementShow(Term term, BodyLiteralVec body) : term(std::move(term)), body(std::move(body)) {}

    Term term;
    BodyLiteralVec body;
};

struct StatementShowSig {
    explicit StatementShowSig(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    bool has_sign;
    std::string name;
    int arity;
};

struct StatementProject {
    explicit StatementProject(Term term, BodyLiteralVec body) : term(std::move(term)), body(std::move(body)) {}

    Term term;
    BodyLiteralVec body;
};

struct StatementProjectSig {
    explicit StatementProjectSig(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    bool has_sign;
    std::string name;
    int arity;
};

struct StatementDefined {
    explicit StatementDefined(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}

    bool has_sign;
    std::string name;
    int arity;
};

struct StatementExternal {
    explicit StatementExternal(Term term, BodyLiteralVec body, std::optional<Term> type = std::nullopt)
        : term(std::move(term)), body(std::move(body)), type{std::move(type)} {}

    Term term;
    BodyLiteralVec body;
    std::optional<Term> type;
};

struct StatementEdge {
    struct Edge {
        Term u;
        Term v;
    };
    using EdgeVec = std::vector<Edge>;

    explicit StatementEdge(EdgeVec edges, BodyLiteralVec body = {}) : edges{std::move(edges)}, body{std::move(body)} {}

    EdgeVec edges;
    BodyLiteralVec body;
};

struct StatementHeuristic {
    explicit StatementHeuristic(Term atom, BodyLiteralVec body, Term type, std::optional<Term> prio, Term mod)
        : atom{std::move(atom)}, body{std::move(body)}, type(std::move(type)), prio(std::move(prio)),
          mod(std::move(mod)) {}
    explicit StatementHeuristic(Term atom, BodyLiteralVec body, Term type, Term prio, Term mod)
        : StatementHeuristic{std::move(atom), std::move(body), std::move(type), std::make_optional(std::move(prio)),
                             std::move(mod)} {}
    explicit StatementHeuristic(Term atom, BodyLiteralVec body, Term type, Term mod)
        : StatementHeuristic{std::move(atom), std::move(body), std::move(type), std::nullopt, std::move(mod)} {}

    Term atom;
    BodyLiteralVec body;
    Term type;
    std::optional<Term> prio;
    Term mod;
};

enum class ScriptType {
    lua,
    python,
};

struct StatementScript {
    explicit StatementScript(ScriptType type, std::string content) : type(type), content(std::move(content)) {}

    ScriptType type;
    std::string content;
};

enum class IncludeType {
    system,
    inbuild,
};

struct StatementInclude {
    explicit StatementInclude(IncludeType type, std::string path) : type(type), path(std::move(path)) {}

    IncludeType type;
    std::string path;
};

struct StatementProgram {
    explicit StatementProgram(std::string name, std::vector<std::string> args)
        : name(std::move(name)), args(std::move(args)) {}

    std::string name;
    std::vector<std::string> args;
};

enum class ConstType { default_, override_ };

struct StatementConst {
    explicit StatementConst(ConstType type, std::string name, Term value)
        : type(type), name(std::move(name)), value(std::move(value)) {}

    ConstType type;
    std::string name;
    Term value;
};

using Statement =
    std::variant<Rule, TheoryDefinition, StatementOptimize, StatementWeakConstraint, StatementShow, StatementShowSig,
                 StatementProject, StatementProjectSig, StatementDefined, StatementExternal, StatementEdge,
                 StatementHeuristic, StatementScript, StatementInclude, StatementProgram, StatementConst>;
using StatementVec = std::vector<Statement>;

} // namespace Gringo::Input
