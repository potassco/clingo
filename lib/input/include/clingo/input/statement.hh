#pragma once

#include <clingo/input/body_literal.hh>
#include <clingo/input/head_literal.hh>

#include <utility>

namespace CppClingo::Input {

//! @addtogroup input_statement
//! @{

//! A rule.
//!
//! For example: <tt>p(X) :- q(X)</tt>.
class StmRule : public Expression<StmRule> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmRule::loc_, a_head = &StmRule::head_, a_body = &StmRule::body_};
    }

    //! Construct a rule.
    explicit StmRule(Location loc, HdLit head, BdLitArray body)
        : loc_{std::move(loc)}, head_{std::move(head)}, body_{std::move(body)} {}

    //! The location of the rule.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The head.
    [[nodiscard]] auto head() const -> HdLit const & { return head_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

  private:
    Location loc_;
    HdLit head_;
    BdLitArray body_;
};

//! The type of a theory operator.
//!
//! @see TheoryOpDefinition
enum class TheoryOpType : uint8_t {
    unary,       //!< An unary theory operator.
    binary_left, //!< An binary left associative theory operator.
    binary_right //!< An binary right associative theory operator.
};

//! A theory operator definition.
//!
//! For example: <tt>- : 0, unary</tt>.
class TheoryOpDefinition : public Expression<TheoryOpDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryOpDefinition::loc_, a_op = &TheoryOpDefinition::op,
                          a_prio = &TheoryOpDefinition::prio_, a_type = &TheoryOpDefinition::type_};
    }

    //! Construct a theory operator definition.
    explicit TheoryOpDefinition(Location loc, String op, int prio, TheoryOpType type)
        : loc_{std::move(loc)}, op_{op}, prio_{prio}, type_{type} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The representation of the operator.
    [[nodiscard]] auto op() const -> String const & { return *op_; }
    //! The priority of the operator.
    [[nodiscard]] auto prio() const -> int { return prio_; }
    //! The type of the operator.
    [[nodiscard]] auto type() const -> TheoryOpType { return type_; }

  private:
    Location loc_;
    SharedString op_;
    int prio_;
    TheoryOpType type_;
};

//! A vector of theory operator definitions.
using TheoryOpDefinitionArray = Util::immutable_array<TheoryOpDefinition>;

//! A theory term definition.
//!
//! For example: <tt>term { - : 0, unary }</tt>.
class TheoryTermDefinition : public Expression<TheoryTermDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermDefinition::loc_, a_name = &TheoryTermDefinition::name,
                          a_op_defs = &TheoryTermDefinition::op_defs_};
    }

    //! Construct a theory term definition.
    explicit TheoryTermDefinition(Location loc, String name, TheoryOpDefinitionArray op_defs)
        : loc_{std::move(loc)}, name_{name}, op_defs_{std::move(op_defs)} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The associated operator definitions.
    [[nodiscard]] auto op_defs() const -> TheoryOpDefinitionArray const & { return op_defs_; }

  private:
    Location loc_;
    SharedString name_;
    TheoryOpDefinitionArray op_defs_;
};

//! A vector of theory term definitions.
using TheoryTermDefinitionArray = Util::immutable_array<TheoryTermDefinition>;

//! Enumeration of theory atom types.
//!
//! @see TheoryAtomDefinition
enum class TheoryAtomType : uint8_t {
    head,     //!< A theory atom that can appear only in the head.
    body,     //!< A theory atom that can appear only in the body.
    any,      //!< A theory atom that can appear only in the head and a body.
    directive //!< A theory atom that can appear only in the head with an empty body.
};

//! An optional definition for the right-hand-side of a theory atom.
//!
//! It consists of a list of possible operators and a name of a term definition.
class TheoryRGuardDefinition : public Expression<TheoryRGuardDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_ops = &TheoryRGuardDefinition::ops, a_term = &TheoryRGuardDefinition::term_};
    }
    //! Construct a right guard definition.
    explicit TheoryRGuardDefinition(StringSpan ops, String term) : ops_{ops.begin(), ops.end()}, term_{term} {}
    //! Construct a right guard definition.
    explicit TheoryRGuardDefinition(SharedStringArray ops, String term) : ops_{std::move(ops)}, term_{term} {}

    //! The list of operator names.
    [[nodiscard]] auto ops() const -> StringSpan { return as_string_span(ops_); }
    //! The name of the term definition.
    [[nodiscard]] auto term() const -> String const & { return *term_; }

  private:
    SharedStringArray ops_;
    SharedString term_;
};

//! A theory atom definition.
//!
//! For example: <tt>name/2: term, any</tt>.
class TheoryAtomDefinition : public Expression<TheoryAtomDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryAtomDefinition::loc_,     a_name = &TheoryAtomDefinition::name,
                          a_arity = &TheoryAtomDefinition::arity_, a_term = &TheoryAtomDefinition::term,
                          a_rhs = &TheoryAtomDefinition::rhs_,     a_type = &TheoryAtomDefinition::type_};
    }

    //! Construct a theory atom definition.
    explicit TheoryAtomDefinition(Location loc, String name, int arity, String term,
                                  std::optional<TheoryRGuardDefinition> rhs, TheoryAtomType type)
        : loc_{std::move(loc)}, name_(name), arity_(arity), term_(term), rhs_(std::move(rhs)), type_(type) {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the atom.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arity of the atom.
    [[nodiscard]] auto arity() const -> int { return arity_; }
    //! The name of the term definition used in elements.
    [[nodiscard]] auto term() const -> String const & { return *term_; }
    //! The definition for the right hand side of the atom.
    [[nodiscard]] auto rhs() const -> std::optional<TheoryRGuardDefinition> const & { return rhs_; }
    //! The type of the atom.
    [[nodiscard]] auto type() const -> TheoryAtomType { return type_; }

  private:
    Location loc_;
    SharedString name_;
    int arity_;
    SharedString term_;
    std::optional<TheoryRGuardDefinition> rhs_;
    TheoryAtomType type_;
};

//! A vector of theory atom definitions.
using TheoryAtomDefinitionArray = Util::immutable_array<TheoryAtomDefinition>;

//! A theory definition.
//!
//! For example: <tt>\#theory name { term { - : 0, unary }; name/2: term, any }</tt>.
class StmTheory : public Expression<StmTheory> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmTheory::loc_, a_name = &StmTheory::name, a_term_defs = &StmTheory::term_defs_,
                          a_atom_defs = &StmTheory::atom_defs};
    }

    //! Construct a theory definition.
    explicit StmTheory(Location loc, String name, TheoryTermDefinitionArray term_defs,
                       TheoryAtomDefinitionArray atom_defs)
        : loc_{std::move(loc)}, name_{name}, term_defs_{std::move(term_defs)}, atom_defs_{std::move(atom_defs)} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The theory term definitions.
    [[nodiscard]] auto term_defs() const -> TheoryTermDefinitionArray const & { return term_defs_; }
    //! The theory atom definitions.
    [[nodiscard]] auto atom_defs() const -> TheoryAtomDefinitionArray const & { return atom_defs_; }

  private:
    Location loc_;
    SharedString name_;
    TheoryTermDefinitionArray term_defs_;
    TheoryAtomDefinitionArray atom_defs_;
};

//! Enumeration of optimization statement types.
//!
//! @related StatementOptimize
enum class OptimizeType : uint8_t { minimize, maximize };

//! The tuple of a minimize element.
class OptimizeTuple : public Expression<OptimizeTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_weight = &OptimizeTuple::weight_, a_prio = &OptimizeTuple::prio_,
                          a_terms = &OptimizeTuple::terms_};
    }

    //! Construct an optimize tuple.
    explicit OptimizeTuple(Term weight, std::optional<Term> priority, TermArray terms)
        : weight_{std::move(weight)}, prio_{std::move(priority)}, terms_{std::move(terms)} {}

    //! The weight.
    [[nodiscard]] auto weight() const -> Term const & { return weight_; }
    //! The optional priority.
    [[nodiscard]] auto prio() const -> std::optional<Term> const & { return prio_; }
    //! The remaining terms.
    [[nodiscard]] auto terms() const -> TermArray const & { return terms_; }

  private:
    Term weight_;
    std::optional<Term> prio_;
    TermArray terms_;
};

//! An optimize element.
class OptimizeElement : public Expression<OptimizeElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_tuple = &OptimizeElement::tuple_, a_cond = &OptimizeElement::cond_};
    }

    //! Construct an optimize element.
    explicit OptimizeElement(OptimizeTuple tuple, LitArray cond) : tuple_{std::move(tuple)}, cond_{std::move(cond)} {}

    //! The weight.
    [[nodiscard]] auto tuple() const -> OptimizeTuple const & { return tuple_; }
    //! The condition.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    OptimizeTuple tuple_;
    LitArray cond_;
};
//! A vector of optimize elements.
using OptimizeElementArray = Util::immutable_array<OptimizeElement>;

//! An optimization statement.
//!
//! For example: <tt>\#minimize { 1@0,X: p(X) }</tt>.
class StmOptimize : public Expression<StmOptimize> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmOptimize::loc_, a_type = &StmOptimize::type_, a_elems = &StmOptimize::elems_};
    }

    //! Construct a weak constraint.
    explicit StmOptimize(Location loc, OptimizeType type, OptimizeElementArray elems)
        : loc_{std::move(loc)}, type_{type}, elems_{std::move(elems)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> OptimizeType { return type_; }
    //! The elements of the statement.
    [[nodiscard]] auto elems() const -> OptimizeElementArray const & { return elems_; }

  private:
    Location loc_;
    OptimizeType type_;
    OptimizeElementArray elems_;
};

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
class StmWeakConstraint : public Expression<StmWeakConstraint> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmWeakConstraint::loc_, a_body = &StmWeakConstraint::body_,
                          a_tuple = &StmWeakConstraint::tuple_};
    }

    //! Construct a weak constraint.
    explicit StmWeakConstraint(Location loc, BdLitArray body, OptimizeTuple tuple)
        : loc_{std::move(loc)}, body_{std::move(body)}, tuple_{std::move(tuple)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The body of the constraint.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }
    //! The tuple of the constraint.
    [[nodiscard]] auto tuple() const -> OptimizeTuple const & { return tuple_; }

  private:
    Location loc_;
    BdLitArray body_;
    OptimizeTuple tuple_;
};

//! A show statement.
//!
//! Example: <tt>\#show p(X): q(X)</tt>.
class StmShow : public Expression<StmShow> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmShow::loc_, a_term = &StmShow::term_, a_body = &StmShow::body_};
    }

    //! Construct a show statement.
    explicit StmShow(Location loc, Term term, BdLitArray body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term to show.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

  private:
    Location loc_;
    Term term_;
    BdLitArray body_;
};

//! A show signature statement.
//!
//! Example: <tt>\#show p/2</tt>.
class StmShowSig : public Expression<StmShowSig> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmShowSig::loc_, a_name = &StmShowSig::name, a_sign = &StmShowSig::sign_,
                          a_arity = &StmShowSig::arity_, a_value = &StmShowSig::value_};
    }

    //! Construct a show signature statement.
    explicit StmShowSig(Location loc, bool sign, String name, int arity, bool value)
        : loc_{std::move(loc)}, name_{name}, arity_{arity}, sign_{sign}, value_{value} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto sign() const -> bool { return sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }
    //! The value.
    [[nodiscard]] auto value() const -> bool { return value_; }

  private:
    Location loc_;
    SharedString name_;
    int arity_;
    bool sign_;
    bool value_;
};

//! A show signature statement.
//!
//! Example: <tt>\#show</tt>.
class StmShowNothing : public Expression<StmShowNothing> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &StmShowNothing::loc_}; }

    //! Construct the show statement.
    explicit StmShowNothing(Location loc) : loc_{std::move(loc)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;
};

//! A project statement.
//!
//! Example: <tt>\#project p(X): q(X)</tt>.
class StmProject : public Expression<StmProject> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProject::loc_, a_atom = &StmProject::atom_, a_body = &StmProject::body_};
    }

    //! Construct a project statement.
    explicit StmProject(Location loc, Term atom, BdLitArray body)
        : loc_{std::move(loc)}, atom_(std::move(atom)), body_(std::move(body)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto atom() const -> Term const & { return atom_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

  private:
    Location loc_;
    Term atom_;
    BdLitArray body_;
};

//! A project signature statement.
//!
//! Example: <tt>\#project p/2</tt>.
class StmProjectSig : public Expression<StmProjectSig> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProjectSig::loc_, a_name = &StmProjectSig::name, a_sign = &StmProjectSig::sign_,
                          a_arity = &StmProjectSig::arity_};
    }

    //! Construct a project signature statement.
    explicit StmProjectSig(Location loc, bool sign, String name, int arity)
        : loc_{std::move(loc)}, sign_{sign}, name_{name}, arity_{arity} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto sign() const -> bool { return sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

  private:
    Location loc_;
    bool sign_;
    SharedString name_;
    int arity_;
};

//! A defined statement.
//!
//! Example: <tt>\#defined p/2</tt>.
class StmDefined : public Expression<StmDefined> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmDefined::loc_, a_name = &StmDefined::name, a_sign = &StmDefined::sign_,
                          a_arity = &StmDefined::arity_};
    }

    //! Construct a defined statement.
    explicit StmDefined(Location loc, bool sign, String name, int arity)
        : loc_{std::move(loc)}, sign_{sign}, name_{name}, arity_{arity} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto sign() const -> bool { return sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

  private:
    Location loc_;
    //! Whether the signature is negative.
    bool sign_;
    //! The name.
    SharedString name_;
    //! The arity.
    int arity_;
};

//! An external statement.
//!
//! Example: <tt>\#external p(X): q(X)</tt>.
class StmExternal : public Expression<StmExternal> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmExternal::loc_, a_atom = &StmExternal::atom_, a_body = &StmExternal::body_,
                          a_type = &StmExternal::type_};
    }

    //! Construct an external statement.
    explicit StmExternal(Location loc, Term atom, BdLitArray body, std::optional<Term> type = std::nullopt)
        : loc_{std::move(loc)}, atom_(std::move(atom)), body_(std::move(body)), type_{std::move(type)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto atom() const -> Term const & { return atom_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> std::optional<Term> const & { return type_; }

  private:
    Location loc_;
    Term atom_;
    BdLitArray body_;
    std::optional<Term> type_;
};

//! An directed edge.
class Edge : public Expression<Edge> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_src = &Edge::src_, a_dst = &Edge::dst_}; }

    //! Construct an edge.
    explicit Edge(Term src, Term dst) : src_{std::move(src)}, dst_{std::move(dst)} {}

    //! The source vertex.
    [[nodiscard]] auto src() const -> Term const & { return src_; }
    //! The target vertex.
    [[nodiscard]] auto dst() const -> Term const & { return dst_; }

  private:
    Term src_;
    Term dst_;
};
//! A vector of edges.
using EdgeArray = Util::immutable_array<Edge>;

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
class StmEdge : public Expression<StmEdge> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmEdge::loc_, a_edges = &StmEdge::edges_, a_body = &StmEdge::body_};
    }

    //! Construct an edge statement.
    explicit StmEdge(Location loc, EdgeArray edges, BdLitArray body = {})
        : loc_{std::move(loc)}, edges_{std::move(edges)}, body_{std::move(body)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The pool of edges.
    [[nodiscard]] auto edges() const -> EdgeArray const & { return edges_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

  private:
    Location loc_;
    EdgeArray edges_;
    BdLitArray body_;
};

//! A heuristic statement.
//!
//! Example: <tt>\#heuristic p(X). [sign,false]</tt>.
class StmHeuristic : public Expression<StmHeuristic> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmHeuristic::loc_,   a_atom = &StmHeuristic::atom_,
                          a_body = &StmHeuristic::body_, a_weight = &StmHeuristic::weight_,
                          a_prio = &StmHeuristic::prio_, a_type = &StmHeuristic::type_};
    }

    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term weight, std::optional<Term> prio, Term type)
        : loc_{std::move(loc)}, atom_{std::move(atom)}, body_{std::move(body)}, weight_(std::move(weight)),
          prio_(std::move(prio)), type_(std::move(type)) {}
    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term weight, Term prio, Term type)
        : StmHeuristic{
              std::move(loc), std::move(atom), std::move(body), std::move(weight), std::make_optional(std::move(prio)),
              std::move(type)} {}
    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term weight, Term type)
        : StmHeuristic{std::move(loc),    std::move(atom), std::move(body),
                       std::move(weight), std::nullopt,    std::move(type)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The atom to modify.
    [[nodiscard]] auto atom() const -> Term const & { return atom_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }
    //! The weight.
    [[nodiscard]] auto weight() const -> Term const & { return weight_; }
    //! The optional priority.
    [[nodiscard]] auto prio() const -> std::optional<Term> const & { return prio_; }
    //! The type of the heuristic modification.
    [[nodiscard]] auto type() const -> Term const & { return type_; }

  private:
    Location loc_;
    Term atom_;
    BdLitArray body_;
    Term weight_;
    std::optional<Term> prio_;
    Term type_;
};

//! A script statement.
//!
//! For example: <tt>\#script(python) some code \#end</tt>.
class StmScript : public Expression<StmScript> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmScript::loc_, a_type = &StmScript::type, a_value = &StmScript::value};
    }

    //! Construct a script statement.
    explicit StmScript(Location loc, String type, String value) : loc_{std::move(loc)}, type_(type), value_(value) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The code type.
    [[nodiscard]] auto type() const -> String const & { return *type_; }
    //! The code.
    [[nodiscard]] auto value() const -> String const & { return *value_; }

  private:
    Location loc_;
    SharedString type_;
    SharedString value_;
};

//! Enumeration of include types.
//!
//! @related StatementInclude
enum class IncludeType : uint8_t {
    system,  //!< Include from path.
    inbuild, //!< Include inbuilt script.
};

//! An include statement.
//!
//! For example: <tt>\#include "encoding.lp"</tt>.
class StmInclude : public Expression<StmInclude> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmInclude::loc_, a_type = &StmInclude::type_, a_value = &StmInclude::value};
    }

    //! Construct an include statement.
    explicit StmInclude(Location loc, IncludeType type, String value)
        : loc_{std::move(loc)}, type_(type), value_(value) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The include type.
    [[nodiscard]] auto type() const -> IncludeType { return type_; }
    //! The path.
    [[nodiscard]] auto value() const -> String const & { return *value_; }

  private:
    Location loc_;
    IncludeType type_;
    SharedString value_;
};

//! A program statement.
//!
//! For example: <tt>\#program check(t)"</tt>.
class StmProgram : public Expression<StmProgram> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProgram::loc_, a_name = &StmProgram::name, a_args = &StmProgram::args};
    }

    //! Construct an program statement.
    explicit StmProgram(Location loc, String name, StringSpan args)
        : loc_{std::move(loc)}, name_(name), args_{args.begin(), args.end()} {}
    //! Construct an program statement.
    explicit StmProgram(Location loc, String name, SharedStringArray args)
        : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the program.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arguments of the program.
    [[nodiscard]] auto args() const -> StringSpan { return as_string_span(args_); }

  private:
    Location loc_;
    SharedString name_;
    SharedStringArray args_;
};

//! Enumeration of constant statement types.
//!
//! @see StatementConst
enum class Precedence : uint8_t {
    default_, //! A statement providing default value.
    override_ //! A statement overriding a default value.
};

//! A const statement.
//!
//! For example: <tt>\#const n=42</tt>.
class StmConst : public Expression<StmConst> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmConst::loc_, a_type = &StmConst::type_, a_name = &StmConst::name,
                          a_value = &StmConst::value_};
    }

    //! Construct a const statement.
    explicit StmConst(Location loc, Precedence type, String name, Term value)
        : loc_{std::move(loc)}, type_(type), name_(name), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> Precedence const & { return type_; }
    //! The name of the constant.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The value of the constant
    [[nodiscard]] auto value() const -> Term const & { return value_; }

  private:
    Location loc_;
    Precedence type_;
    SharedString name_;
    Term value_;
};

//! Concrete symbols for a program statement.
//!
//! @see StmProgram
using ProgramParam = std::pair<SharedString, std::vector<SharedSymbol>>;
//! A list of program params.
using ProgramParamVec = std::vector<ProgramParam>;

//! A parts statement.
//!
//! For example: <tt>\#parts base;check.</tt>.
class StmParts : public Expression<StmParts> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmParts::loc_, a_type = &StmParts::type_, a_elems = &StmParts::elems_};
    }

    //! Construct a const statement.
    explicit StmParts(Location loc, Precedence type, ProgramParamVec elems)
        : loc_{std::move(loc)}, type_(type), elems_(std::move(elems)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> Precedence const & { return type_; }
    //! The program parts to ground and solve.
    [[nodiscard]] auto elems() const -> ProgramParamVec const & { return elems_; }

  private:
    Location loc_;
    Precedence type_;
    ProgramParamVec elems_;
};

//! Enumeration of comment types.
//!
//! @related Comment
enum class CommentType : uint8_t {
    line,  //!< A newline terminated comment starting with <tt>%</tt>.
    block, //!< A block comment enclosed in <tt>%*</tt> and <tt>*%</tt>.
};

//! A comment.
//!
//! For example: <tt>%* comment *%</tt>
class StmComment : public Expression<StmComment> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmComment::loc_, a_type = &StmComment::type_, a_value = &StmComment::value};
    }

    //! Construct a comment.
    explicit StmComment(Location loc, CommentType type, String value)
        : loc_{std::move(loc)}, type_{type}, value_{value} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the comment.
    [[nodiscard]] auto type() const -> CommentType { return type_; }
    //! The content of the comment including comment markers.
    [[nodiscard]] auto value() const -> String const & { return *value_; }

  private:
    Location loc_;
    CommentType type_;
    SharedString value_;
};

//! Variant of available statements.
using Stm = std::variant<StmRule, StmTheory, StmOptimize, StmWeakConstraint, StmShow, StmShowNothing, StmShowSig,
                         StmProject, StmProjectSig, StmDefined, StmExternal, StmEdge, StmHeuristic, StmScript,
                         StmInclude, StmProgram, StmConst, StmParts, StmComment>;
//! A vector of statements.
using StmVec = std::vector<Stm>;

//! @}

} // namespace CppClingo::Input
