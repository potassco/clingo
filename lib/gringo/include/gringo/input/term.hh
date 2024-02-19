#pragma once

#include <gringo/input/attributes.hh>
#include <gringo/input/location.hh>

#include <gringo/symbol.hh>

#include <gringo/util/hash.hh>
#include <gringo/util/immutable_array.hh>
#include <gringo/util/immutable_value.hh>
#include <gringo/util/optional.hh>

#include <variant>

namespace Gringo::Input {

//! @defgroup input Input
//! Data structures and functions to parse and rewrite the gringo language.
//!
//! @ingroup API

//! @defgroup input_language Language
//! Data structures and functions to capture the gringo language.
//!
//! @ingroup input

//! @defgroup input_term Terms
//! Data structures and functions to represent terms.
//!
//! @ingroup input_language
//!
//! @{

//! An array of strings.
using StringArray = Util::immutable_array<String>;

//! A set of variable names.
using VariableSet = StringSet;
//! A vector of variable names.
using VariableVec = StringVec;

class TermVariable;
class TermSymbol;
class TermTuple;
class TermFunction;
class TermAbs;
class TermUnary;
class TermBinary;

//! Variant holding the different term types.
using Term = std::variant<TermVariable, TermSymbol, TermTuple, TermFunction, TermAbs, TermUnary, TermBinary>;

//! A vector of terms.
using TermArray = Util::immutable_array<Term>;

//! Indicate a projected position.
class Projection : public Gringo::Util::Record::Base<Projection> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &Projection::loc_}; }

    //! Construct a projection indicator.
    explicit Projection(Location loc) : loc_{loc} {}
    //! The location of the projected position.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

    //! Compare two projected positions.
    friend auto operator==(Projection const &a, Projection const &b) -> bool {
        static_cast<void>(a);
        static_cast<void>(b);
        return true;
    }
    //! Compare two projected positions.
    friend auto operator<=>(Projection const &a, Projection const &b) -> std::strong_ordering {
        static_cast<void>(a);
        static_cast<void>(b);
        return 1 <=> 1;
    }

  private:
    Location loc_;
};

//! A variant capturing either a term or a position that is to be projected.
using Argument = std::variant<Projection, Term>;
//! A tuple of terms or positions to project.
using ArgumentArray = Util::immutable_array<Argument>;

class ArgumentTuple : public Gringo::Util::Record::Base<ArgumentTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_elems = &ArgumentTuple::elems_}; }

    //! Construct an argument tuple.
    ArgumentTuple(ArgumentArray elems);

    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> ArgumentArray const & { return elems_; }

    //! Compare two argument tuples.
    friend auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;
    //! Compare two argument tuples.
    friend auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering;

  private:
    ArgumentArray elems_;
};

//! A vector of tuples used as function or predicate arguments.
using PoolArray = Util::immutable_array<ArgumentTuple>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
class TermVariable : public Gringo::Util::Record::Base<TermVariable> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermVariable::loc_, a_name = &TermVariable::name_,
                          a_anonymous = &TermVariable::anonymous_};
    }

    //! Construct a variable.
    explicit TermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{std::move(name)}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

    //! Compare two variables.
    friend auto operator==(TermVariable const &a, TermVariable const &b) -> bool { return a.name_ == b.name_; }
    //! Compare two variables.
    friend auto operator<=>(TermVariable const &a, TermVariable const &b) -> std::strong_ordering {
        return a.name_ <=> b.name_;
    }

  private:
    Location loc_;
    String name_;
    bool anonymous_;
};

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
class TermSymbol : public Gringo::Util::Record::Base<TermSymbol> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &TermSymbol::loc_, a_value = &TermSymbol::value_}; }

    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The associated symbol.
    [[nodiscard]] auto value() const -> Symbol { return value_; }

    //! Compare two symbols.
    friend auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool { return a.value_ == b.value_; }
    //! Compare two symbols.
    friend auto operator<=>(TermSymbol const &a, TermSymbol const &b) -> std::strong_ordering {
        return a.value_ <=> b.value_;
    }

  private:
    Location loc_;
    Symbol value_;
};

//! A tuple element.
using TupleElement = std::variant<ArgumentTuple, Term>;
//! A vector of tuple elements.
using TupleElementArray = Util::immutable_array<TupleElement>;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
class TermTuple : public Gringo::Util::Record::Base<TermTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &TermTuple::loc_, a_pool = &TermTuple::pool_}; }

    //! Construct a  tuple.
    explicit TermTuple(Location loc, TupleElementArray pool);

    //! The location of the tuple.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the tuple.
    [[nodiscard]] auto pool() const -> TupleElementArray const & { return pool_; }

    //! Compare two tuple terms.
    friend auto operator==(TermTuple const &a, TermTuple const &b) -> bool;
    //! Compare two tuple terms.
    friend auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering;

  private:
    Location loc_;
    TupleElementArray pool_;
};

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
class TermFunction : public Gringo::Util::Record::Base<TermFunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermFunction::loc_, a_name = &TermFunction::name_, a_pool = &TermFunction::pool_,
                          a_exteral = &TermFunction::external_};
    }

    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(Location loc, String name, PoolArray pool, bool external);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the function.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! The argument pool of the function.
    [[nodiscard]] auto pool() const -> PoolArray const & { return pool_; }
    //! Whether this is an external function.
    [[nodiscard]] auto external() const -> bool { return external_; }

    //! Compare two function terms.
    friend auto operator==(TermFunction const &a, TermFunction const &b) -> bool;
    //! Compare two function terms.
    friend auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering;

  private:
    Location loc_;
    String name_;
    PoolArray pool_;
    bool external_;
};

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
class TermAbs : public Gringo::Util::Record::Base<TermAbs> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &TermAbs::loc_, a_pool = &TermAbs::pool_}; }

    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(Location loc, TermArray pool);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the absolute term.
    [[nodiscard]] auto pool() const -> TermArray const & { return pool_; }

    //! Compare two absolute terms.
    friend auto operator==(TermAbs const &a, TermAbs const &b) -> bool;
    //! Compare two absolute terms.
    friend auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering;

  private:
    Location loc_;
    TermArray pool_;
};

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
class TermUnary : public Gringo::Util::Record::Base<TermUnary> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermUnary::loc_, a_op = &TermUnary::op_, a_rhs = &TermUnary::rhs_};
    }

    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The operation.
    [[nodiscard]] auto op() const -> UnaryOperator { return op_; }
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Util::immutable_value<Term> const & { return rhs_; }

    //! Compare two unary terms.
    friend auto operator==(TermUnary const &a, TermUnary const &b) -> bool;
    //! Compare two unary terms.
    friend auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering;

  private:
    Location loc_;
    UnaryOperator op_;
    Util::immutable_value<Term> rhs_;
};

//! Enumeration of available binary operators.
enum class BinaryOperator : int {
    and_,  //!< The AND bit operation.
    div,   //!< The (integer) divide arithmetic operation.
    minus, //!< The minus arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    times, //!< The multiply arithmetic operation.
    or_,   //!< The OR bit operation.
    plus,  //!< The plus arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
    xor_,  //!< The XOR bit operation.
    dots,  //!< The interval operator.
};

//! Term representing a binary operation.
//!
//! For example <tt>X-Y</tt>.
class TermBinary : public Gringo::Util::Record::Base<TermBinary> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermBinary::loc_, a_lhs = &TermBinary::lhs_, a_op = &TermBinary::op_,
                          a_rhs = &TermBinary::rhs_};
    }

    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                        Util::immutable_value<Term> rhs);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The left-hand-side.
    [[nodiscard]] auto lhs() const -> Util::immutable_value<Term> const & { return lhs_; }
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Util::immutable_value<Term> const & { return rhs_; }
    //! The operation.
    [[nodiscard]] auto op() const -> BinaryOperator { return op_; }

    //! Compare two binary terms.
    friend auto operator==(TermBinary const &a, TermBinary const &b) -> bool;
    //! Compare two binary terms.
    friend auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering;

  private:
    Location loc_;
    Util::immutable_value<Term> lhs_;
    Util::immutable_value<Term> rhs_;
    BinaryOperator op_;
};

//! @}

// ArgumentTuple

inline ArgumentTuple::ArgumentTuple(ArgumentArray elems) : elems_{std::move(elems)} {}

inline auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
}

inline auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

// TermTuple

inline TermTuple::TermTuple(Location loc, TupleElementArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto operator==(TermTuple const &a, TermTuple const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
}

inline auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

// TermFunction

inline TermFunction::TermFunction(Location loc, String name, PoolArray pool, bool external)
    : loc_{std::move(loc)}, name_(std::move(name)), pool_{std::move(pool)}, external_{external} {}

inline auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
}

inline auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

// TermAbs

inline TermAbs::TermAbs(Location loc, TermArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto operator==(TermAbs const &a, TermAbs const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
}

inline auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

// TermUnary

inline TermUnary::TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{std::move(rhs)} {}

inline auto operator==(TermUnary const &a, TermUnary const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
};

inline auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

// TermBinary

inline TermBinary::TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                              Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}

inline auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return a.comparison_tuple() == b.comparison_tuple();
};

inline auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering {
    return a.comparison_tuple() <=> b.comparison_tuple();
}

} // namespace Gringo::Input
