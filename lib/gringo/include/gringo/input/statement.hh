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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The head.
    [[nodiscard]] auto head() const -> HeadLiteral const & { return head_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_head, a_body}, Types{args...});
        return Rule{select<Opt>(a_loc, loc_, args...), select<Opt>(a_head, head_, args...),
                    select<Opt>(a_body, body_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(Rule const &a, Rule const &b) -> bool;
    friend auto operator<(Rule const &a, Rule const &b) -> bool;
    friend struct Util::value_hasher<Rule>;

    Location loc_;
    HeadLiteral head_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The representation of the operator.
    [[nodiscard]] auto op() const -> String { return op_; }
    //! The priority of the operator.
    [[nodiscard]] auto prio() const -> int { return prio_; }
    //! The type of the operator.
    [[nodiscard]] auto type() const -> TheoryOpType { return type_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_op, a_prio, a_type}, Types{args...});
        return TheoryOpDefinition{select<Opt>(a_loc, loc_, args...), select<Opt>(a_op, op_, args...),
                                  select<Opt>(a_prio, prio_, args...), select<Opt>(a_type, type_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool;
    friend auto operator<(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool;
    friend struct Util::value_hasher<TheoryOpDefinition>;

    Location loc_;
    String op_;
    int prio_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The associated operator definitions.
    [[nodiscard]] auto op_defs() const -> Util::immutable_array<TheoryOpDefinition> const & { return op_defs_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_op_defs}, Types{args...});
        return TheoryTermDefinition{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                                    select<Opt>(a_op_defs, op_defs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool;
    friend auto operator<(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool;
    friend struct Util::value_hasher<TheoryTermDefinition>;

    Location loc_;
    String name_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the atom.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity of the atom.
    [[nodiscard]] auto arity() const -> int { return arity_; }
    //! The name of the term definition used in elements.
    [[nodiscard]] auto term() const -> String { return term_; }
    //! The definition for the right hand side of the atom.
    [[nodiscard]] auto rhs() const -> std::optional<TheoryRGuardDefinition> const & { return rhs_; }
    //! The type of the atom.
    [[nodiscard]] auto type() const -> TheoryAtomType { return type_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_arity, a_term, a_rhs, a_type}, Types{args...});
        return TheoryAtomDefinition{select<Opt>(a_loc, loc_, args...),     select<Opt>(a_name, name_, args...),
                                    select<Opt>(a_arity, arity_, args...), select<Opt>(a_term, term_, args...),
                                    select<Opt>(a_rhs, rhs_, args...),     select<Opt>(a_type, type_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool;
    friend auto operator<(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool;
    friend struct Util::value_hasher<TheoryAtomDefinition>;

    Location loc_;
    String name_;
    int arity_;
    String term_;
    std::optional<TheoryRGuardDefinition> rhs_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The theory term definitions.
    [[nodiscard]] auto term_defs() const -> TheoryTermDefinitionVec const & { return term_defs_; }
    //! The theory atom definitions.
    [[nodiscard]] auto atom_defs() const -> TheoryAtomDefinitionVec const & { return atom_defs_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_term_defs, a_atom_defs}, Types{args...});
        return TheoryDefinition{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                                select<Opt>(a_term_defs, term_defs_, args...),
                                select<Opt>(a_atom_defs, atom_defs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TheoryDefinition const &a, TheoryDefinition const &b) -> bool;
    friend auto operator<(TheoryDefinition const &a, TheoryDefinition const &b) -> bool;
    friend struct Util::value_hasher<TheoryDefinition>;

    Location loc_;
    String name_;
    TheoryTermDefinitionVec term_defs_;
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

//! The tuple of a minimize element.
class OptimizeTuple {
  public:
    explicit OptimizeTuple(Term weight, std::optional<Term> priority, TermVec terms)
        : weight_{std::move(weight)}, priority_{std::move(priority)}, terms_{std::move(terms)} {}

    //! The weight.
    [[nodiscard]] auto weight() const -> Term const & { return weight_; }
    //! The optional priority.
    [[nodiscard]] auto priority() const -> std::optional<Term> const & { return priority_; }
    //! The remaining terms.
    [[nodiscard]] auto terms() const -> TermVec const & { return terms_; }

  private:
    friend auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool;
    friend auto operator<(OptimizeTuple const &a, OptimizeTuple const &b) -> bool;
    friend struct Util::value_hasher<OptimizeTuple>;

    Term weight_;
    std::optional<Term> priority_;
    TermVec terms_;
};

//! An element.
using OptimizeElement = std::pair<OptimizeTuple, LiteralVec>;
//! A vector of elements.
using OptimizeElementVec = Util::immutable_array<OptimizeElement>;

//! An optimization statement.
//!
//! For example: <tt>\#minimize { 1@0,X: p(X) }</tt>.
class StatementOptimize {
  public:
    //! Construct a weak constraint.
    explicit StatementOptimize(Location loc, OptimizeType type, OptimizeElementVec elems)
        : loc_{std::move(loc)}, type_{type}, elems_{std::move(elems)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> OptimizeType { return type_; }
    //! The elements of the statement.
    [[nodiscard]] auto elems() const -> OptimizeElementVec const & { return elems_; }

  private:
    friend auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool;
    friend auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool;
    friend struct Util::value_hasher<StatementOptimize>;

    Location loc_;
    OptimizeType type_;
    OptimizeElementVec elems_;
};

//! Compare two tuples.
auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool;

//! Compare two optimization tuples.
auto operator<(OptimizeTuple const &a, OptimizeTuple const &b) -> bool;

//! Compare two optimization statements.
auto operator==(StatementOptimize const &a, StatementOptimize const &b) -> bool;

//! Compare two optimization statements.
auto operator<(StatementOptimize const &a, StatementOptimize const &b) -> bool;

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
class StatementWeakConstraint {
  public:
    //! Construct a weak constraint.
    explicit StatementWeakConstraint(Location loc, BodyLiteralVec body, OptimizeTuple tuple)
        : loc_{std::move(loc)}, body_{std::move(body)}, tuple_{std::move(tuple)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The body of the constraint.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }
    //! The tuple of the constraint.
    [[nodiscard]] auto tuple() const -> OptimizeTuple const & { return tuple_; }

  private:
    friend auto operator==(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool;
    friend auto operator<(StatementWeakConstraint const &a, StatementWeakConstraint const &b) -> bool;
    friend struct Util::value_hasher<StatementWeakConstraint>;

    Location loc_;
    BodyLiteralVec body_;
    OptimizeTuple tuple_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term to show.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }

  private:
    friend auto operator==(StatementShow const &a, StatementShow const &b) -> bool;
    friend auto operator<(StatementShow const &a, StatementShow const &b) -> bool;
    friend struct Util::value_hasher<StatementShow>;

    Location loc_;
    Term term_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

  private:
    friend auto operator==(StatementShowSig const &a, StatementShowSig const &b) -> bool;
    friend auto operator<(StatementShowSig const &a, StatementShowSig const &b) -> bool;
    friend struct Util::value_hasher<StatementShowSig>;

    Location loc_;
    bool has_sign_;
    String name_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }

  private:
    friend auto operator==(StatementProject const &a, StatementProject const &b) -> bool;
    friend auto operator<(StatementProject const &a, StatementProject const &b) -> bool;
    friend struct Util::value_hasher<StatementProject>;

    Location loc_;
    Term term_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

  private:
    friend auto operator==(StatementProjectSig const &a, StatementProjectSig const &b) -> bool;
    friend auto operator<(StatementProjectSig const &a, StatementProjectSig const &b) -> bool;
    friend struct Util::value_hasher<StatementProjectSig>;

    Location loc_;
    bool has_sign_;
    String name_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto has_sign() const -> bool { return has_sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

  private:
    friend auto operator==(StatementDefined const &a, StatementDefined const &b) -> bool;
    friend auto operator<(StatementDefined const &a, StatementDefined const &b) -> bool;
    friend struct Util::value_hasher<StatementDefined>;

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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> std::optional<Term> const & { return type_; }

  private:
    friend auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool;
    friend auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool;
    friend struct Util::value_hasher<StatementExternal>;

    Location loc_;
    Term term_;
    BodyLiteralVec body_;
    std::optional<Term> type_;
};

//! Compare two external statements.
auto operator==(StatementExternal const &a, StatementExternal const &b) -> bool;

//! Compare two external statements.
auto operator<(StatementExternal const &a, StatementExternal const &b) -> bool;

//! An directed edge.
class Edge {
  public:
    explicit Edge(Term u, Term v) : u_{std::move(u)}, v_{std::move(v)} {}

    //! The source vertex.
    [[nodiscard]] auto u() const -> Term const & { return u_; }
    //! The target vertex.
    [[nodiscard]] auto v() const -> Term const & { return v_; }

  private:
    friend auto operator==(Edge const &a, Edge const &b) -> bool;
    friend auto operator<(Edge const &a, Edge const &b) -> bool;
    friend struct Util::value_hasher<Edge>;

    //! The source vertex.
    Term u_;
    //! The target vertex.
    Term v_;
};
//! A vector of edges.
using EdgeVec = Util::immutable_array<Edge>;

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
class StatementEdge {
  public:
    //! Construct an edge statement.
    explicit StatementEdge(Location loc, EdgeVec edges, BodyLiteralVec body = {})
        : loc_{std::move(loc)}, edges_{std::move(edges)}, body_{std::move(body)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The pool of edges.
    [[nodiscard]] auto edges() const -> EdgeVec const & { return edges_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }

  private:
    friend auto operator==(StatementEdge const &a, StatementEdge const &b) -> bool;
    friend auto operator<(StatementEdge const &a, StatementEdge const &b) -> bool;
    friend struct Util::value_hasher<StatementEdge>;

    Location loc_;
    EdgeVec edges_;
    BodyLiteralVec body_;
};

//! Compare two edges.
auto operator==(Edge const &a, Edge const &b) -> bool;

//! Compare two edges.
auto operator<(Edge const &a, Edge const &b) -> bool;

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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The atom to modify.
    [[nodiscard]] auto atom() const -> Term const & { return atom_; }
    //! The body.
    [[nodiscard]] auto body() const -> BodyLiteralVec const & { return body_; }
    //! The type.
    [[nodiscard]] auto type() const -> Term const & { return type_; }
    //! The optional priority.
    [[nodiscard]] auto prio() const -> std::optional<Term> const & { return prio_; }
    //! The modifier.
    [[nodiscard]] auto mod() const -> Term const & { return mod_; }

  private:
    friend auto operator==(StatementHeuristic const &a, StatementHeuristic const &b) -> bool;
    friend auto operator<(StatementHeuristic const &a, StatementHeuristic const &b) -> bool;
    friend struct Util::value_hasher<StatementHeuristic>;

    Location loc_;
    Term atom_;
    BodyLiteralVec body_;
    Term type_;
    std::optional<Term> prio_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The code type.
    [[nodiscard]] auto type() const -> String { return type_; }
    //! The code.
    [[nodiscard]] auto content() const -> std::string const & { return content_; }

  private:
    friend auto operator==(StatementScript const &a, StatementScript const &b) -> bool;
    friend auto operator<(StatementScript const &a, StatementScript const &b) -> bool;
    friend struct Util::value_hasher<StatementScript>;

    Location loc_;
    String type_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The include type.
    [[nodiscard]] auto type() const -> IncludeType { return type_; }
    //! The path.
    [[nodiscard]] auto path() const -> std::string const & { return path_; }

  private:
    friend auto operator==(StatementInclude const &a, StatementInclude const &b) -> bool;
    friend auto operator<(StatementInclude const &a, StatementInclude const &b) -> bool;
    friend struct Util::value_hasher<StatementInclude>;

    Location loc_;
    IncludeType type_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the program.
    [[nodiscard]] auto name() const -> String const & { return name_; }
    //! The arguments of the program.
    [[nodiscard]] auto args() const -> Util::immutable_array<String> const & { return args_; }

  private:
    friend auto operator==(StatementProgram const &a, StatementProgram const &b) -> bool;
    friend auto operator<(StatementProgram const &a, StatementProgram const &b) -> bool;
    friend struct Util::value_hasher<StatementProgram>;

    Location loc_;
    String name_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> ConstType const & { return type_; }
    //! The name of the constant.
    [[nodiscard]] auto name() const -> String const & { return name_; }
    //! The value of the constant
    [[nodiscard]] auto value() const -> Term const & { return value_; }

  private:
    friend auto operator==(StatementConst const &a, StatementConst const &b) -> bool;
    friend auto operator<(StatementConst const &a, StatementConst const &b) -> bool;
    friend struct Util::value_hasher<StatementConst>;

    Location loc_;
    ConstType type_;
    String name_;
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
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the comment.
    [[nodiscard]] auto type() const -> CommentType { return type_; }
    //! The content of the comment including comment markers.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

  private:
    friend auto operator==(Comment const &a, Comment const &b) -> bool;
    friend auto operator<(Comment const &a, Comment const &b) -> bool;
    friend struct Util::value_hasher<Comment>;

    Location loc_;
    CommentType type_;
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
GRINGO_HASH_PROTO(Gringo::Input::OptimizeTuple);
GRINGO_HASH_PROTO(Gringo::Input::Edge);
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
