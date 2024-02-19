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
class StmRule : public Gringo::Util::Record::Base<StmRule> {
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

    //! Compare two rules.
    friend auto operator==(StmRule const &a, StmRule const &b) -> bool {
        return std::tie(a.head_, a.body_) == std::tie(b.head_, b.body_);
    }
    //! Compare two rules.
    friend auto operator<=>(StmRule const &a, StmRule const &b) -> std::strong_ordering {
        return std::tie(a.head_, a.body_) <=> std::tie(b.head_, b.body_);
    }

  private:
    Location loc_;
    HdLit head_;
    BdLitArray body_;
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
class TheoryOpDefinition : public Gringo::Util::Record::Base<TheoryOpDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryOpDefinition::loc_, a_op = &TheoryOpDefinition::op_,
                          a_prio = &TheoryOpDefinition::prio_, a_type = &TheoryOpDefinition::type_};
    }

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

    //! Compare two theory operator definitions.
    friend auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
        return std::tie(a.op_, a.prio_, a.type_) == std::tie(b.op_, b.prio_, b.type_);
    }
    //! Compare two theory operator definitions.
    friend auto operator<=>(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> std::strong_ordering {
        return std::tie(a.op_, a.prio_, a.type_) <=> std::tie(b.op_, b.prio_, b.type_);
    }

  private:
    Location loc_;
    String op_;
    int prio_;
    TheoryOpType type_;
};

//! A vector of theory operator definitions.
using TheoryOpDefinitionArray = Util::immutable_array<TheoryOpDefinition>;

//! A theory term definition.
//!
//! For example: <tt>term { - : 0, unary }</tt>.
class TheoryTermDefinition : public Gringo::Util::Record::Base<TheoryTermDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermDefinition::loc_, a_name = &TheoryTermDefinition::name_,
                          a_op_defs = &TheoryTermDefinition::op_defs_};
    }

    //! Construct a theory term definition.
    explicit TheoryTermDefinition(Location loc, String name, TheoryOpDefinitionArray op_defs)
        : loc_{std::move(loc)}, name_{name}, op_defs_{std::move(op_defs)} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The associated operator definitions.
    [[nodiscard]] auto op_defs() const -> TheoryOpDefinitionArray const & { return op_defs_; }

    //! Compare two theory term definitions.
    friend auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
        return std::tie(a.name_, a.op_defs_) == std::tie(b.name_, b.op_defs_);
    }
    //! Compare two theory term definitions.
    friend auto operator<=>(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.op_defs_) <=> std::tie(b.name_, b.op_defs_);
    }

  private:
    Location loc_;
    String name_;
    TheoryOpDefinitionArray op_defs_;
};

//! A vector of theory term definitions.
using TheoryTermDefinitionArray = Util::immutable_array<TheoryTermDefinition>;

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
using TheoryRGuardDefinition = std::pair<StringArray, String>;

//! A theory atom definition.
//!
//! For example: <tt>name/2: term, any</tt>.
class TheoryAtomDefinition : public Gringo::Util::Record::Base<TheoryAtomDefinition> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryAtomDefinition::loc_,     a_name = &TheoryAtomDefinition::name_,
                          a_arity = &TheoryAtomDefinition::arity_, a_term = &TheoryAtomDefinition::term_,
                          a_rhs = &TheoryAtomDefinition::rhs_,     a_type = &TheoryAtomDefinition::type_};
    }

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

    //! Compare two theory atom definitions.
    friend auto operator==(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.term_, a.type_, a.rhs_) ==
               std::tie(b.name_, b.arity_, b.term_, b.type_, b.rhs_);
    }
    //! Compare two theory atom definitions.
    friend auto operator<=>(TheoryAtomDefinition const &a, TheoryAtomDefinition const &b) -> std::strong_ordering {
        return Util::make_strong_ordering(std::tie(a.name_, a.arity_, a.term_, a.type_, a.rhs_) <=>
                                          std::tie(b.name_, b.arity_, b.term_, b.type_, b.rhs_));
    }

  private:
    Location loc_;
    String name_;
    int arity_;
    String term_;
    std::optional<TheoryRGuardDefinition> rhs_;
    TheoryAtomType type_;
};

//! A vector of theory atom definitions.
using TheoryAtomDefinitionArray = Util::immutable_array<TheoryAtomDefinition>;

//! A theory definition.
//!
//! For example: <tt>\#theory name { term { - : 0, unary }; name/2: term, any }</tt>.
class StmTheory : public Gringo::Util::Record::Base<StmTheory> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmTheory::loc_, a_name = &StmTheory::name_, a_term_defs = &StmTheory::term_defs_,
                          a_atom_defs = &StmTheory::atom_defs};
    }

    //! Construct a theory definition.
    explicit StmTheory(Location loc, String name, TheoryTermDefinitionArray term_defs,
                       TheoryAtomDefinitionArray atom_defs)
        : loc_{std::move(loc)}, name_{name}, term_defs_{std::move(term_defs)}, atom_defs_{std::move(atom_defs)} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The theory term definitions.
    [[nodiscard]] auto term_defs() const -> TheoryTermDefinitionArray const & { return term_defs_; }
    //! The theory atom definitions.
    [[nodiscard]] auto atom_defs() const -> TheoryAtomDefinitionArray const & { return atom_defs_; }

    //! Compare two theory statements.
    friend auto operator==(StmTheory const &a, StmTheory const &b) -> bool {
        return std::tie(a.name_, a.term_defs_, a.atom_defs_) == std::tie(b.name_, b.term_defs_, b.atom_defs_);
    }
    //! Compare two theory statements.
    friend auto operator<=>(StmTheory const &a, StmTheory const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.term_defs_, a.atom_defs_) <=> std::tie(b.name_, b.term_defs_, b.atom_defs_);
    }

  private:
    Location loc_;
    String name_;
    TheoryTermDefinitionArray term_defs_;
    TheoryAtomDefinitionArray atom_defs_;
};

//! Enumeration of optimization statement types.
//!
//! @related StatementOptimize
enum class OptimizeType { minimize, maximize };

//! The tuple of a minimize element.
class OptimizeTuple : public Gringo::Util::Record::Base<OptimizeTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_weight = &OptimizeTuple::weight_, a_prio = &OptimizeTuple::prio_,
                          a_terms = &OptimizeTuple::terms_};
    }

    explicit OptimizeTuple(Term weight, std::optional<Term> priority, TermArray terms)
        : weight_{std::move(weight)}, prio_{std::move(priority)}, terms_{std::move(terms)} {}

    //! The weight.
    [[nodiscard]] auto weight() const -> Term const & { return weight_; }
    //! The optional priority.
    [[nodiscard]] auto prio() const -> std::optional<Term> const & { return prio_; }
    //! The remaining terms.
    [[nodiscard]] auto terms() const -> TermArray const & { return terms_; }

    //! Compare two optimize tuples.
    friend auto operator==(OptimizeTuple const &a, OptimizeTuple const &b) -> bool {
        return std::tie(a.weight_, a.terms_, a.prio_) == std::tie(b.weight_, b.terms_, b.prio_);
    }
    //! Compare two optimize tuples.
    friend auto operator<=>(OptimizeTuple const &a, OptimizeTuple const &b) -> std::strong_ordering {
        return Util::make_strong_ordering(std::tie(a.weight_, a.terms_, a.prio_) <=>
                                          std::tie(b.weight_, b.terms_, b.prio_));
    }

  private:
    Term weight_;
    std::optional<Term> prio_;
    TermArray terms_;
};

//! An element.
using OptimizeElement = std::pair<OptimizeTuple, LitArray>;
//! A vector of elements.
using OptimizeElementArray = Util::immutable_array<OptimizeElement>;

//! An optimization statement.
//!
//! For example: <tt>\#minimize { 1@0,X: p(X) }</tt>.
class StmOptimize : public Gringo::Util::Record::Base<StmOptimize> {
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

    //! Compare two optimization statements.
    friend auto operator==(StmOptimize const &a, StmOptimize const &b) -> bool {
        return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
    }
    //! Compare two optimization statements.
    friend auto operator<=>(StmOptimize const &a, StmOptimize const &b) -> std::strong_ordering {
        return std::tie(a.type_, a.elems_) <=> std::tie(b.type_, b.elems_);
    }

  private:
    Location loc_;
    OptimizeType type_;
    OptimizeElementArray elems_;
};

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
class StmWeakConstraint : public Gringo::Util::Record::Base<StmWeakConstraint> {
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

    //! Compare two weak constraints.
    friend auto operator==(StmWeakConstraint const &a, StmWeakConstraint const &b) -> bool {
        return std::tie(a.body_, a.tuple_) == std::tie(b.body_, b.tuple_);
    }
    //! Compare two weak constraints.
    friend auto operator<=>(StmWeakConstraint const &a, StmWeakConstraint const &b) -> std::strong_ordering {
        return std::tie(a.body_, a.tuple_) <=> std::tie(b.body_, b.tuple_);
    }

  private:
    Location loc_;
    BdLitArray body_;
    OptimizeTuple tuple_;
};

//! A show statement.
//!
//! Example: <tt>\#show p(X): q(X)</tt>.
class StmShow : public Gringo::Util::Record::Base<StmShow> {
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

    //! Compare two show statements.
    friend auto operator==(StmShow const &a, StmShow const &b) -> bool {
        return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
    }
    //! Compare two show statements.
    friend auto operator<=>(StmShow const &a, StmShow const &b) -> std::strong_ordering {
        return std::tie(a.term_, a.body_) <=> std::tie(b.term_, b.body_);
    }

  private:
    Location loc_;
    Term term_;
    BdLitArray body_;
};

//! A show signature statement.
//!
//! Example: <tt>\#show p/2</tt>.
class StmShowSig : public Gringo::Util::Record::Base<StmShowSig> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmShowSig::loc_, a_name = &StmShowSig::name_, a_sign = &StmShowSig::sign_,
                          a_arity = &StmShowSig::arity_};
    }

    //! Construct a show signature statement.
    explicit StmShowSig(Location loc, bool sign, String name, int arity)
        : loc_{std::move(loc)}, name_{name}, arity_{arity}, sign_{sign} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! Whether the signature is negative.
    [[nodiscard]] auto sign() const -> bool { return sign_; }
    //! The name.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    //! Compare two show signature statements.
    friend auto operator==(StmShowSig const &a, StmShowSig const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }
    //! Compare two show signature statements.
    friend auto operator<=>(StmShowSig const &a, StmShowSig const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    Location loc_;
    String name_;
    int arity_;
    bool sign_;
};

//! A project statement.
//!
//! Example: <tt>\#project p(X): q(X)</tt>.
class StmProject : public Gringo::Util::Record::Base<StmProject> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProject::loc_, a_term = &StmProject::term_, a_body = &StmProject::body_};
    }

    //! Construct a project statement.
    explicit StmProject(Location loc, Term term, BdLitArray body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

    //! Compare two project statements.
    friend auto operator==(StmProject const &a, StmProject const &b) -> bool {
        return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
    }
    //! Compare two project statements.
    friend auto operator<=>(StmProject const &a, StmProject const &b) -> std::strong_ordering {
        return std::tie(a.term_, a.body_) <=> std::tie(b.term_, b.body_);
    }

  private:
    Location loc_;
    Term term_;
    BdLitArray body_;
};

//! A project signature statement.
//!
//! Example: <tt>\#project p/2</tt>.
class StmProjectSig : public Gringo::Util::Record::Base<StmProjectSig> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProjectSig::loc_, a_name = &StmProjectSig::name_, a_sign = &StmProjectSig::sign_,
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
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    //! Compare two project signature statements.
    friend auto operator==(StmProjectSig const &a, StmProjectSig const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }
    //! Compare two project signature statements.
    friend auto operator<=>(StmProjectSig const &a, StmProjectSig const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    Location loc_;
    bool sign_;
    String name_;
    int arity_;
};

//! A defined statement.
//!
//! Example: <tt>\#defined p/2</tt>.
class StmDefined : public Gringo::Util::Record::Base<StmDefined> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmDefined::loc_, a_name = &StmDefined::name_, a_sign = &StmDefined::sign_,
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
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The arity.
    [[nodiscard]] auto arity() const -> int { return arity_; }

    //! Compare two defined statements.
    friend auto operator==(StmDefined const &a, StmDefined const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }
    //! Compare two defined statements.
    friend auto operator<=>(StmDefined const &a, StmDefined const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    Location loc_;
    //! Whether the signature is negative.
    bool sign_;
    //! The name.
    String name_;
    //! The arity.
    int arity_;
};

//! An external statement.
//!
//! Example: <tt>\#external p(X): q(X)</tt>.
class StmExternal : public Gringo::Util::Record::Base<StmExternal> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmExternal::loc_, a_term = &StmExternal::term_, a_body = &StmExternal::body_,
                          a_type = &StmExternal::type_};
    }

    //! Construct an external statement.
    explicit StmExternal(Location loc, Term term, BdLitArray body, std::optional<Term> type = std::nullopt)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)), type_{std::move(type)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> std::optional<Term> const & { return type_; }

    //! Compare two external statements.
    friend auto operator==(StmExternal const &a, StmExternal const &b) -> bool {
        return std::tie(a.term_, a.body_, a.type_) == std::tie(b.term_, b.body_, b.type_);
    }
    //! Compare two external statements.
    friend auto operator<=>(StmExternal const &a, StmExternal const &b) -> std::strong_ordering {
        return Util::make_strong_ordering(std::tie(a.term_, a.body_, a.type_) <=> std::tie(b.term_, b.body_, b.type_));
    }

  private:
    Location loc_;
    Term term_;
    BdLitArray body_;
    std::optional<Term> type_;
};

//! An directed edge.
class Edge : public Gringo::Util::Record::Base<Edge> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_src = &Edge::src_, a_dst = &Edge::dst_}; }

    explicit Edge(Term src, Term dst) : src_{std::move(src)}, dst_{std::move(dst)} {}

    //! The source vertex.
    [[nodiscard]] auto src() const -> Term const & { return src_; }
    //! The target vertex.
    [[nodiscard]] auto dst() const -> Term const & { return dst_; }

    //! Compare two edges.
    friend auto operator==(Edge const &a, Edge const &b) -> bool = default;
    //! Compare two edges.
    friend auto operator<=>(Edge const &a, Edge const &b) -> std::strong_ordering = default;

  private:
    Term src_;
    Term dst_;
};
//! A vector of edges.
using EdgeArray = Util::immutable_array<Edge>;

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
class StmEdge : public Gringo::Util::Record::Base<StmEdge> {
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

    //! Compare two edge statements.
    friend auto operator==(StmEdge const &a, StmEdge const &b) -> bool {
        return std::tie(a.edges_, a.body_) == std::tie(b.edges_, b.body_);
    }
    //! Compare two edge statements.
    friend auto operator<=>(StmEdge const &a, StmEdge const &b) -> std::strong_ordering {
        return std::tie(a.edges_, a.body_) <=> std::tie(b.edges_, b.body_);
    }

  private:
    Location loc_;
    EdgeArray edges_;
    BdLitArray body_;
};

//! A heuristic statement.
//!
//! Example: <tt>\#heuristic p(X). [sign,false]</tt>.
class StmHeuristic : public Gringo::Util::Record::Base<StmHeuristic> {
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

    //! Compare two heuristic statements.
    friend auto operator==(StmHeuristic const &a, StmHeuristic const &b) -> bool {
        return std::tie(a.atom_, a.body_, a.type_, a.prio_, a.weight_) ==
               std::tie(b.atom_, b.body_, b.type_, b.prio_, b.weight_);
    }
    //! Compare two heuristic statements.
    friend auto operator<=>(StmHeuristic const &a, StmHeuristic const &b) -> std::strong_ordering {
        return Util::make_strong_ordering(std::tie(a.atom_, a.body_, a.type_, a.prio_, a.weight_) <=>
                                          std::tie(b.atom_, b.body_, b.type_, b.prio_, b.weight_));
    }

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
class StmScript : public Gringo::Util::Record::Base<StmScript> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmScript::loc_, a_type = &StmScript::type_, a_value = &StmScript::value_};
    }

    //! Construct a script statement.
    explicit StmScript(Location loc, String type, std::string value)
        : loc_{std::move(loc)}, type_(type), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The code type.
    [[nodiscard]] auto type() const -> String { return type_; }
    //! The code.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Compare two script statements.
    friend auto operator==(StmScript const &a, StmScript const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }
    //! Compare two script statements.
    friend auto operator<=>(StmScript const &a, StmScript const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    Location loc_;
    String type_;
    std::string value_;
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
class StmInclude : public Gringo::Util::Record::Base<StmInclude> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmInclude::loc_, a_type = &StmInclude::type_, a_value = &StmInclude::value_};
    }

    //! Construct an include statement.
    explicit StmInclude(Location loc, IncludeType type, std::string value)
        : loc_{std::move(loc)}, type_(type), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The include type.
    [[nodiscard]] auto type() const -> IncludeType { return type_; }
    //! The path.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Compare two include statements.
    friend auto operator==(StmInclude const &a, StmInclude const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }
    //! Compare two include statements.
    friend auto operator<=>(StmInclude const &a, StmInclude const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    Location loc_;
    IncludeType type_;
    std::string value_;
};

//! A program statement.
//!
//! For example: <tt>\#program check(t)"</tt>.
class StmProgram : public Gringo::Util::Record::Base<StmProgram> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmProgram::loc_, a_name = &StmProgram::name_, a_args = &StmProgram::args_};
    }

    //! Construct an program statement.
    explicit StmProgram(Location loc, String name, StringArray args)
        : loc_{std::move(loc)}, name_(name), args_(std::move(args)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the program.
    [[nodiscard]] auto name() const -> String const & { return name_; }
    //! The arguments of the program.
    [[nodiscard]] auto args() const -> StringArray const & { return args_; }

    //! Compare two program statements.
    friend auto operator==(StmProgram const &a, StmProgram const &b) -> bool {
        return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
    }
    //! Compare two program statements.
    friend auto operator<=>(StmProgram const &a, StmProgram const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.args_) <=> std::tie(b.name_, b.args_);
    }

  private:
    Location loc_;
    String name_;
    StringArray args_;
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
class StmConst : public Gringo::Util::Record::Base<StmConst> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmConst::loc_, a_type = &StmConst::type_, a_name = &StmConst::name_,
                          a_value = &StmConst::value_};
    }

    //! Construct a const statement.
    explicit StmConst(Location loc, ConstType type, String name, Term value)
        : loc_{std::move(loc)}, type_(type), name_(name), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> ConstType const & { return type_; }
    //! The name of the constant.
    [[nodiscard]] auto name() const -> String const & { return name_; }
    //! The value of the constant
    [[nodiscard]] auto value() const -> Term const & { return value_; }

    //! Compare two const statements.
    friend auto operator==(StmConst const &a, StmConst const &b) -> bool {
        return std::tie(a.name_, a.value_, a.type_) == std::tie(b.name_, b.value_, b.type_);
    }
    //! Compare two const statements.
    friend auto operator<=>(StmConst const &a, StmConst const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.value_, a.type_) <=> std::tie(b.name_, b.value_, b.type_);
    }

  private:
    Location loc_;
    ConstType type_;
    String name_;
    Term value_;
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
class StmComment : public Gringo::Util::Record::Base<StmComment> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &StmComment::loc_, a_type = &StmComment::type_, a_value = &StmComment::value_};
    }

    //! Construct a comment.
    explicit StmComment(Location loc, CommentType type, std::string value)
        : loc_{std::move(loc)}, type_{type}, value_{std::move(value)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the comment.
    [[nodiscard]] auto type() const -> CommentType { return type_; }
    //! The content of the comment including comment markers.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Compare two comments.
    friend auto operator==(StmComment const &a, StmComment const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }
    //! Compare two comments.
    friend auto operator<=>(StmComment const &a, StmComment const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    Location loc_;
    CommentType type_;
    std::string value_;
};

//! Variant of available statements.
using Stm = std::variant<StmRule, StmTheory, StmOptimize, StmWeakConstraint, StmShow, StmShowSig, StmProject,
                         StmProjectSig, StmDefined, StmExternal, StmEdge, StmHeuristic, StmScript, StmInclude,
                         StmProgram, StmConst, StmComment>;
//! A vector of statements.
using StmVec = std::vector<Stm>;

//! @}

} // namespace Gringo::Input
