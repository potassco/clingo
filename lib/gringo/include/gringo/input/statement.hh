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
class StmRule {
  public:
    //! Construct a rule.
    explicit StmRule(Location loc, HdLit head, BdLitArray body)
        : loc_{std::move(loc)}, head_{std::move(head)}, body_{std::move(body)} {}

    //! The location of the rule.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The head.
    [[nodiscard]] auto head() const -> HdLit const & { return head_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_head, a_body}, Types{args...});
        return StmRule{select<Opt>(a_loc, loc_, args...), select<Opt>(a_head, head_, args...),
                       select<Opt>(a_body, body_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two rules.
    friend auto operator==(StmRule const &a, StmRule const &b) -> bool {
        return std::tie(a.head_, a.body_) == std::tie(b.head_, b.body_);
    }
    //! Compare two rules.
    friend auto operator<=>(StmRule const &a, StmRule const &b) -> std::strong_ordering {
        return std::tie(a.head_, a.body_) <=> std::tie(b.head_, b.body_);
    }

  private:
    friend struct Util::value_hasher<StmRule>;

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

    //! Compare two theory operator definitions.
    friend auto operator==(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> bool {
        return std::tie(a.op_, a.prio_, a.type_) == std::tie(b.op_, b.prio_, b.type_);
    }
    //! Compare two theory operator definitions.
    friend auto operator<=>(TheoryOpDefinition const &a, TheoryOpDefinition const &b) -> std::strong_ordering {
        return std::tie(a.op_, a.prio_, a.type_) <=> std::tie(b.op_, b.prio_, b.type_);
    }

  private:
    friend struct Util::value_hasher<TheoryOpDefinition>;

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
class TheoryTermDefinition {
  public:
    //! Construct a theory term definition.
    explicit TheoryTermDefinition(Location loc, String name, TheoryOpDefinitionArray op_defs)
        : loc_{std::move(loc)}, name_{name}, op_defs_{std::move(op_defs)} {}

    //! The location of the definition.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the definition.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The associated operator definitions.
    [[nodiscard]] auto op_defs() const -> TheoryOpDefinitionArray const & { return op_defs_; }

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

    //! Compare two theory term definitions.
    friend auto operator==(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> bool {
        return std::tie(a.name_, a.op_defs_) == std::tie(b.name_, b.op_defs_);
    }
    //! Compare two theory term definitions.
    friend auto operator<=>(TheoryTermDefinition const &a, TheoryTermDefinition const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.op_defs_) <=> std::tie(b.name_, b.op_defs_);
    }

  private:
    friend struct Util::value_hasher<TheoryTermDefinition>;

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
    friend struct Util::value_hasher<TheoryAtomDefinition>;

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
class StmTheory {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_term_defs, a_atom_defs}, Types{args...});
        return StmTheory{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                         select<Opt>(a_term_defs, term_defs_, args...), select<Opt>(a_atom_defs, atom_defs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(StmTheory const &a, StmTheory const &b) -> bool {
        return std::tie(a.name_, a.term_defs_, a.atom_defs_) == std::tie(b.name_, b.term_defs_, b.atom_defs_);
    }
    friend auto operator<=>(StmTheory const &a, StmTheory const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.term_defs_, a.atom_defs_) <=> std::tie(b.name_, b.term_defs_, b.atom_defs_);
    }
    friend struct Util::value_hasher<StmTheory>;

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
class OptimizeTuple {
  public:
    explicit OptimizeTuple(Term weight, std::optional<Term> priority, TermArray terms)
        : weight_{std::move(weight)}, prio_{std::move(priority)}, terms_{std::move(terms)} {}

    //! The weight.
    [[nodiscard]] auto weight() const -> Term const & { return weight_; }
    //! The optional priority.
    [[nodiscard]] auto prio() const -> std::optional<Term> const & { return prio_; }
    //! The remaining terms.
    [[nodiscard]] auto terms() const -> TermArray const & { return terms_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_weight, a_prio, a_terms}, Types{args...});
        return OptimizeTuple{select<Opt>(a_weight, weight_, args...), select<Opt>(a_prio, prio_, args...),
                             select<Opt>(a_terms, terms_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

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
    friend struct Util::value_hasher<OptimizeTuple>;

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
class StmOptimize {
  public:
    //! Construct a weak constraint.
    explicit StmOptimize(Location loc, OptimizeType type, OptimizeElementArray elems)
        : loc_{std::move(loc)}, type_{type}, elems_{std::move(elems)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the statement.
    [[nodiscard]] auto type() const -> OptimizeType { return type_; }
    //! The elements of the statement.
    [[nodiscard]] auto elems() const -> OptimizeElementArray const & { return elems_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_elems}, Types{args...});
        return StmOptimize{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                           select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two optimization statements.
    friend auto operator==(StmOptimize const &a, StmOptimize const &b) -> bool {
        return std::tie(a.type_, a.elems_) == std::tie(b.type_, b.elems_);
    }
    //! Compare two optimization statements.
    friend auto operator<=>(StmOptimize const &a, StmOptimize const &b) -> std::strong_ordering {
        return std::tie(a.type_, a.elems_) <=> std::tie(b.type_, b.elems_);
    }

  private:
    friend struct Util::value_hasher<StmOptimize>;

    Location loc_;
    OptimizeType type_;
    OptimizeElementArray elems_;
};

//! A weak constraint.
//!
//! For example: <tt>:~ p(X). [1@0,X]</tt>.
class StmWeakConstraint {
  public:
    //! Construct a weak constraint.
    explicit StmWeakConstraint(Location loc, BdLitArray body, OptimizeTuple tuple)
        : loc_{std::move(loc)}, body_{std::move(body)}, tuple_{std::move(tuple)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The body of the constraint.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }
    //! The tuple of the constraint.
    [[nodiscard]] auto tuple() const -> OptimizeTuple const & { return tuple_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_body, a_tuple}, Types{args...});
        return StmWeakConstraint{select<Opt>(a_loc, loc_, args...), select<Opt>(a_body, body_, args...),
                                 select<Opt>(a_tuple, tuple_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two weak constraints.
    friend auto operator==(StmWeakConstraint const &a, StmWeakConstraint const &b) -> bool {
        return std::tie(a.body_, a.tuple_) == std::tie(b.body_, b.tuple_);
    }
    //! Compare two weak constraints.
    friend auto operator<=>(StmWeakConstraint const &a, StmWeakConstraint const &b) -> std::strong_ordering {
        return std::tie(a.body_, a.tuple_) <=> std::tie(b.body_, b.tuple_);
    }

  private:
    friend struct Util::value_hasher<StmWeakConstraint>;

    Location loc_;
    BdLitArray body_;
    OptimizeTuple tuple_;
};

//! A show statement.
//!
//! Example: <tt>\#show p(X): q(X)</tt>.
class StmShow {
  public:
    //! Construct a show statement.
    explicit StmShow(Location loc, Term term, BdLitArray body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term to show.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_term, a_body}, Types{args...});
        return StmShow{select<Opt>(a_loc, loc_, args...), select<Opt>(a_term, term_, args...),
                       select<Opt>(a_body, body_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two show statements.
    friend auto operator==(StmShow const &a, StmShow const &b) -> bool {
        return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
    }
    //! Compare two show statements.
    friend auto operator<=>(StmShow const &a, StmShow const &b) -> std::strong_ordering {
        return std::tie(a.term_, a.body_) <=> std::tie(b.term_, b.body_);
    }

  private:
    friend struct Util::value_hasher<StmShow>;

    Location loc_;
    Term term_;
    BdLitArray body_;
};

//! A show signature statement.
//!
//! Example: <tt>\#show p/2</tt>.
class StmShowSig {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_name, a_arity}, Types{args...});
        return StmShowSig{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                          select<Opt>(a_name, name_, args...), select<Opt>(a_arity, arity_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two show signature statements.
    friend auto operator==(StmShowSig const &a, StmShowSig const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }

    //! Compare two show signature statements.
    friend auto operator<=>(StmShowSig const &a, StmShowSig const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    friend struct Util::value_hasher<StmShowSig>;

    Location loc_;
    String name_;
    int arity_;
    bool sign_;
};

//! A project statement.
//!
//! Example: <tt>\#project p(X): q(X)</tt>.
class StmProject {
  public:
    //! Construct a project statement.
    explicit StmProject(Location loc, Term term, BdLitArray body)
        : loc_{std::move(loc)}, term_(std::move(term)), body_(std::move(body)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The term representing the atom to project.
    [[nodiscard]] auto term() const -> Term const & { return term_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_term, a_body}, Types{args...});
        return StmProject{select<Opt>(a_loc, loc_, args...), select<Opt>(a_term, term_, args...),
                          select<Opt>(a_body, body_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two project statements.
    friend auto operator==(StmProject const &a, StmProject const &b) -> bool {
        return std::tie(a.term_, a.body_) == std::tie(b.term_, b.body_);
    }
    //! Compare two project statements.
    friend auto operator<=>(StmProject const &a, StmProject const &b) -> std::strong_ordering {
        return std::tie(a.term_, a.body_) <=> std::tie(b.term_, b.body_);
    }

  private:
    friend struct Util::value_hasher<StmProject>;

    Location loc_;
    Term term_;
    BdLitArray body_;
};

//! A project signature statement.
//!
//! Example: <tt>\#project p/2</tt>.
class StmProjectSig {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_name, a_arity}, Types{args...});
        return StmProjectSig{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                             select<Opt>(a_name, name_, args...), select<Opt>(a_arity, arity_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two project signature statements.
    friend auto operator==(StmProjectSig const &a, StmProjectSig const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }

    //! Compare two project signature statements.
    friend auto operator<=>(StmProjectSig const &a, StmProjectSig const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    friend struct Util::value_hasher<StmProjectSig>;

    Location loc_;
    bool sign_;
    String name_;
    int arity_;
};

//! A defined statement.
//!
//! Example: <tt>\#defined p/2</tt>.
class StmDefined {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_sign, a_name, a_arity}, Types{args...});
        return StmDefined{select<Opt>(a_loc, loc_, args...), select<Opt>(a_sign, sign_, args...),
                          select<Opt>(a_name, name_, args...), select<Opt>(a_arity, arity_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two defined statements.
    friend auto operator==(StmDefined const &a, StmDefined const &b) -> bool {
        return std::tie(a.name_, a.arity_, a.sign_) == std::tie(b.name_, b.arity_, b.sign_);
    }

    //! Compare two defined statements.
    friend auto operator<=>(StmDefined const &a, StmDefined const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.arity_, a.sign_) <=> std::tie(b.name_, b.arity_, b.sign_);
    }

  private:
    friend struct Util::value_hasher<StmDefined>;

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
class StmExternal {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_term, a_body, a_type}, Types{args...});
        return StmExternal{select<Opt>(a_loc, loc_, args...), select<Opt>(a_term, term_, args...),
                           select<Opt>(a_body, body_, args...), select<Opt>(a_type, type_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two external statements.
    friend auto operator==(StmExternal const &a, StmExternal const &b) -> bool {
        return std::tie(a.term_, a.body_, a.type_) == std::tie(b.term_, b.body_, b.type_);
    }
    //! Compare two external statements.
    friend auto operator<=>(StmExternal const &a, StmExternal const &b) -> std::strong_ordering {
        return Util::make_strong_ordering(std::tie(a.term_, a.body_, a.type_) <=> std::tie(b.term_, b.body_, b.type_));
    }

  private:
    friend struct Util::value_hasher<StmExternal>;

    Location loc_;
    Term term_;
    BdLitArray body_;
    std::optional<Term> type_;
};

//! An directed edge.
class Edge {
  public:
    explicit Edge(Term src, Term dst) : src_{std::move(src)}, dst_{std::move(dst)} {}

    //! The source vertex.
    [[nodiscard]] auto src() const -> Term const & { return src_; }
    //! The target vertex.
    [[nodiscard]] auto dst() const -> Term const & { return dst_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_src, a_dst}, Types{args...});
        return Edge{select<Opt>(a_src, src_, args...), select<Opt>(a_dst, dst_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two edges.
    friend auto operator==(Edge const &a, Edge const &b) -> bool {
        return std::tie(a.src_, a.dst_) == std::tie(b.src_, b.dst_);
    }
    //! Compare two edges.
    friend auto operator<=>(Edge const &a, Edge const &b) -> std::strong_ordering {
        return std::tie(a.src_, a.dst_) <=> std::tie(b.src_, b.dst_);
    }

  private:
    friend struct Util::value_hasher<Edge>;

    //! The source vertex.
    Term src_;
    //! The target vertex.
    Term dst_;
};
//! A vector of edges.
using EdgeArray = Util::immutable_array<Edge>;

//! An edge statement.
//!
//! Example: <tt>\#edge (X,Y): connected(X,Y).</tt>.
class StmEdge {
  public:
    //! Construct an edge statement.
    explicit StmEdge(Location loc, EdgeArray edges, BdLitArray body = {})
        : loc_{std::move(loc)}, edges_{std::move(edges)}, body_{std::move(body)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The pool of edges.
    [[nodiscard]] auto edges() const -> EdgeArray const & { return edges_; }
    //! The body.
    [[nodiscard]] auto body() const -> BdLitArray const & { return body_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_edges, a_body}, Types{args...});
        return StmEdge{select<Opt>(a_loc, loc_, args...), select<Opt>(a_edges, edges_, args...),
                       select<Opt>(a_body, body_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two edge statements.
    friend auto operator==(StmEdge const &a, StmEdge const &b) -> bool {
        return std::tie(a.edges_, a.body_) == std::tie(b.edges_, b.body_);
    }
    //! Compare two edge statements.
    friend auto operator<=>(StmEdge const &a, StmEdge const &b) -> std::strong_ordering {
        return std::tie(a.edges_, a.body_) <=> std::tie(b.edges_, b.body_);
    }

  private:
    friend struct Util::value_hasher<StmEdge>;

    Location loc_;
    EdgeArray edges_;
    BdLitArray body_;
};

//! A heuristic statement.
//!
//! Example: <tt>\#heuristic p(X). [sign,false]</tt>.
class StmHeuristic {
  public:
    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term type, std::optional<Term> prio, Term weight)
        : loc_{std::move(loc)}, atom_{std::move(atom)}, body_{std::move(body)}, weight_(std::move(type)),
          prio_(std::move(prio)), type_(std::move(weight)) {}
    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term type, Term prio, Term mod)
        : StmHeuristic{
              std::move(loc), std::move(atom), std::move(body), std::move(type), std::make_optional(std::move(prio)),
              std::move(mod)} {}
    //! Construct a heuristic statement.
    explicit StmHeuristic(Location loc, Term atom, BdLitArray body, Term type, Term mod)
        : StmHeuristic{std::move(loc),  std::move(atom), std::move(body),
                       std::move(type), std::nullopt,    std::move(mod)} {}

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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_atom, a_body, a_weight, a_prio, a_type}, Types{args...});
        return StmHeuristic{select<Opt>(a_loc, loc_, args...),   select<Opt>(a_atom, atom_, args...),
                            select<Opt>(a_body, body_, args...), select<Opt>(a_weight, weight_, args...),
                            select<Opt>(a_prio, prio_, args...), select<Opt>(a_type, type_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

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
    friend struct Util::value_hasher<StmHeuristic>;

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
class StmScript {
  public:
    //! Construct a script statement.
    explicit StmScript(Location loc, String type, std::string value)
        : loc_{std::move(loc)}, type_(type), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The code type.
    [[nodiscard]] auto type() const -> String { return type_; }
    //! The code.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_value}, Types{args...});
        return StmScript{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                         select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two script statements.
    friend auto operator==(StmScript const &a, StmScript const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }
    //! Compare two script statements.
    friend auto operator<=>(StmScript const &a, StmScript const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    friend struct Util::value_hasher<StmScript>;

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
class StmInclude {
  public:
    //! Construct an include statement.
    explicit StmInclude(Location loc, IncludeType type, std::string value)
        : loc_{std::move(loc)}, type_(type), value_(std::move(value)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The include type.
    [[nodiscard]] auto type() const -> IncludeType { return type_; }
    //! The path.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_value}, Types{args...});
        return StmInclude{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                          select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two include statements.
    friend auto operator==(StmInclude const &a, StmInclude const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }
    //! Compare two include statements.
    friend auto operator<=>(StmInclude const &a, StmInclude const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    friend struct Util::value_hasher<StmInclude>;

    Location loc_;
    IncludeType type_;
    std::string value_;
};

//! A program statement.
//!
//! For example: <tt>\#program check(t)"</tt>.
class StmProgram {
  public:
    //! Construct an program statement.
    explicit StmProgram(Location loc, String name, StringArray args)
        : loc_{std::move(loc)}, name_(name), args_(std::move(args)) {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the program.
    [[nodiscard]] auto name() const -> String const & { return name_; }
    //! The arguments of the program.
    [[nodiscard]] auto args() const -> StringArray const & { return args_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_args}, Types{args...});
        return StmProgram{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                          select<Opt>(a_args, args_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two program statements.
    friend auto operator==(StmProgram const &a, StmProgram const &b) -> bool {
        return std::tie(a.name_, a.args_) == std::tie(b.name_, b.args_);
    }
    //! Compare two program statements.
    friend auto operator<=>(StmProgram const &a, StmProgram const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.args_) <=> std::tie(b.name_, b.args_);
    }

  private:
    friend struct Util::value_hasher<StmProgram>;

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
class StmConst {
  public:
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

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_name, a_value}, Types{args...});
        return StmConst{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                        select<Opt>(a_name, name_, args...), select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two const statements.
    friend auto operator==(StmConst const &a, StmConst const &b) -> bool {
        return std::tie(a.name_, a.value_, a.type_) == std::tie(b.name_, b.value_, b.type_);
    }
    //! Compare two const statements.
    friend auto operator<=>(StmConst const &a, StmConst const &b) -> std::strong_ordering {
        return std::tie(a.name_, a.value_, a.type_) <=> std::tie(b.name_, b.value_, b.type_);
    }

  private:
    friend struct Util::value_hasher<StmConst>;

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
class StmComment {
  public:
    //! Construct a comment.
    explicit StmComment(Location loc, CommentType type, std::string value)
        : loc_{std::move(loc)}, type_{type}, value_{std::move(value)} {}

    //! The location of the statement.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the comment.
    [[nodiscard]] auto type() const -> CommentType { return type_; }
    //! The content of the comment including comment markers.
    [[nodiscard]] auto value() const -> std::string const & { return value_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_type, a_value}, Types{args...});
        return StmComment{select<Opt>(a_loc, loc_, args...), select<Opt>(a_type, type_, args...),
                          select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

    //! Compare two comments.
    friend auto operator==(StmComment const &a, StmComment const &b) -> bool {
        return std::tie(a.value_, a.type_) == std::tie(b.value_, b.type_);
    }

    //! Compare two comments.
    friend auto operator<=>(StmComment const &a, StmComment const &b) -> std::strong_ordering {
        return std::tie(a.value_, a.type_) <=> std::tie(b.value_, b.type_);
    }

  private:
    friend struct Util::value_hasher<StmComment>;

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

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::TheoryOpDefinition);
GRINGO_HASH_PROTO(Gringo::Input::TheoryTermDefinition);
GRINGO_HASH_PROTO(Gringo::Input::TheoryAtomDefinition);
GRINGO_HASH_PROTO(Gringo::Input::StmTheory);
GRINGO_HASH_PROTO(Gringo::Input::OptimizeTuple);
GRINGO_HASH_PROTO(Gringo::Input::Edge);
GRINGO_HASH_PROTO(Gringo::Input::StmRule);
GRINGO_HASH_PROTO(Gringo::Input::StmOptimize);
GRINGO_HASH_PROTO(Gringo::Input::StmWeakConstraint);
GRINGO_HASH_PROTO(Gringo::Input::StmShow);
GRINGO_HASH_PROTO(Gringo::Input::StmShowSig);
GRINGO_HASH_PROTO(Gringo::Input::StmProject);
GRINGO_HASH_PROTO(Gringo::Input::StmProjectSig);
GRINGO_HASH_PROTO(Gringo::Input::StmDefined);
GRINGO_HASH_PROTO(Gringo::Input::StmExternal);
GRINGO_HASH_PROTO(Gringo::Input::StmEdge);
GRINGO_HASH_PROTO(Gringo::Input::StmHeuristic);
GRINGO_HASH_PROTO(Gringo::Input::StmScript);
GRINGO_HASH_PROTO(Gringo::Input::StmInclude);
GRINGO_HASH_PROTO(Gringo::Input::StmProgram);
GRINGO_HASH_PROTO(Gringo::Input::StmConst);
GRINGO_HASH_PROTO(Gringo::Input::StmComment);

#endif
