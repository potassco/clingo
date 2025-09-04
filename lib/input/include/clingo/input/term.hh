#pragma once

#include <clingo/input/attributes.hh>

#include <clingo/core/location.hh>
#include <clingo/core/symbol.hh>

#include <clingo/util/hash.hh>
#include <clingo/util/immutable_array.hh>
#include <clingo/util/immutable_value.hh>
#include <clingo/util/optional.hh>
#include <clingo/util/ordered_set.hh>

#include <utility>
#include <variant>

namespace CppClingo::Input {

//! @addtogroup input_term
//! @{

//! A set of variable names.
using VariableSet = StringSet;
//! A vector of variable names.
using VariableVec = StringVec;

using CppClingo::location;

class TermVariable;
class TermSymbol;
class TermTuple;
class TermFunction;
class TermAbs;
class TermUnary;
class TermBinary;

//! The signature of a predicate.
using Sig = std::tuple<String, size_t, bool>;
//! The signature of a predicate.
using SharedSig = std::tuple<SharedString, size_t, bool>;
//! A set of predicate signatures.
using SharedSigSet = Util::ordered_set<SharedSig>;

//! Variant holding the different term types.
using Term = std::variant<TermVariable, TermSymbol, TermTuple, TermFunction, TermAbs, TermUnary, TermBinary>;

//! A vector of terms.
using TermArray = Util::immutable_array<Term>;

//! Indicate a projected position.
class Projection : public Expression<Projection> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &Projection::loc_}; }

    //! Construct a projection indicator.
    explicit Projection(Location loc) : loc_{std::move(loc)} {}
    //! The location of the projected position.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

  private:
    Location loc_;
};

//! A variant capturing either a term or a position that is to be projected.
using Argument = std::variant<Projection, Term>;
//! A tuple of terms or positions to project.
using ArgumentArray = Util::immutable_array<Argument>;

//! An argument tuple for a function or directly a tuple.
class ArgumentTuple : public RecursiveExpression<ArgumentTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_elems = &ArgumentTuple::elems_}; }

    //! Construct an argument tuple.
    ArgumentTuple(ArgumentArray elems);

    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> ArgumentArray const & { return elems_; }

  private:
    ArgumentArray elems_;
};

//! A vector of tuples used as function or predicate arguments.
using PoolArray = Util::immutable_array<ArgumentTuple>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
class TermVariable : public Expression<TermVariable> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermVariable::loc_, a_name = &TermVariable::name,
                          a_anonymous = &TermVariable::anonymous_};
    }

    //! Construct a variable.
    explicit TermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{name}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

  private:
    Location loc_;
    SharedString name_;
    bool anonymous_;
};

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
class TermSymbol : public Expression<TermSymbol> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &TermSymbol::loc_, a_value = &TermSymbol::value}; }

    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{value} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The associated symbol.
    [[nodiscard]] auto value() const -> Symbol const & { return *value_; }

  private:
    Location loc_;
    SharedSymbol value_;
};

struct FormatSpec {
    enum class Conversion : uint8_t {
        str = 0,
        repr = 1,
    };

    enum class Align : uint8_t {
        none,
        left,
        right,
        number,
        center,
    };

    enum class Sign : uint8_t {
        none,
        plus,
        minus,
        space,
    };

    enum class Grouping : uint8_t {
        none,
        comma,
        underscore,
    };

    enum class Type : uint8_t {
        character,
        binary,
        octal,
        decimal,
        hex_lower,
        hex_upper,
        locale,
        string,
    };

    FormatSpec() = default;
    [[nodiscard]] static auto build(SymbolStore &store, std::string_view str) -> std::optional<FormatSpec>;

    std::vector<std::variant<SharedString, size_t>> accessors;
    uint32_t width = 0;
    std::optional<char> fill;
    Type type = Type::string;
    Grouping grouping = Grouping::none;
    Conversion conversion = Conversion::str;
    Align align = Align::none;
    Sign sign = Sign::none;
    bool alternate_form = false;
};

class FormatField : public Expression<FormatField> {
  public:
    explicit FormatField(TermVariable variable, FormatSpec flags)
        : variable_{std::move(variable)}, flags_{std::move(flags)} {}

    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_lhs = &FormatField::variable_, a_rhs = &FormatField::flags_};
    }

    [[nodiscard]] auto lhs() const -> TermVariable const & { return variable_; }
    [[nodiscard]] auto rhs() const -> FormatSpec const & { return flags_; }

  private:
    TermVariable variable_;
    FormatSpec flags_;
};

using FormatFieldArray = Util::immutable_array<std::variant<SharedString, FormatField>>;

//! A format string term.
class TermFormatString : public Expression<TermFormatString> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermFormatString::loc_, a_elems = &TermFormatString::elems};
    }

    //! Construct a projection indicator.
    explicit TermFormatString(Location loc, FormatFieldArray elems) : loc_{std::move(loc)}, elems_{std::move(elems)} {}
    //! The location of the projected position.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The associated fields.
    [[nodiscard]] auto elems() const -> FormatFieldArray const & { return elems_; }

  private:
    Location loc_;
    FormatFieldArray elems_;
};

//! A tuple element.
using TupleElement = std::variant<ArgumentTuple, Term>;
//! A vector of tuple elements.
using TupleElementArray = Util::immutable_array<TupleElement>;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
class TermTuple : public RecursiveExpression<TermTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_loc = &TermTuple::loc_, a_pool = &TermTuple::pool_}; }

    //! Construct a  tuple.
    explicit TermTuple(Location loc, TupleElementArray pool);

    //! The location of the tuple.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the tuple.
    [[nodiscard]] auto pool() const -> TupleElementArray const & { return pool_; }

  private:
    Location loc_;
    TupleElementArray pool_;
};

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
class TermFunction : public RecursiveExpression<TermFunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TermFunction::loc_, a_name = &TermFunction::name, a_pool = &TermFunction::pool_,
                          a_external = &TermFunction::external_};
    }

    //! Construct a symbolic function.
    //!
    //! The function takes a pool of term tuples, which will be reduced to a single element after calling
    //! Term::unpool().
    explicit TermFunction(Location loc, String name, PoolArray pool, bool external);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the function.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The argument pool of the function.
    [[nodiscard]] auto pool() const -> PoolArray const & { return pool_; }
    //! Whether this is an external function.
    [[nodiscard]] auto external() const -> bool { return external_; }

  private:
    Location loc_;
    SharedString name_;
    PoolArray pool_;
    bool external_;
};

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
class TermAbs : public RecursiveExpression<TermAbs> {
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
enum class UnaryOperator : uint8_t {
    minus,  //!< The unary minus sign (-).
    negate, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
class TermUnary : public RecursiveExpression<TermUnary> {
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

  private:
    Location loc_;
    UnaryOperator op_;
    Util::immutable_value<Term> rhs_;
};

//! Enumeration of available binary operators.
enum class BinaryOperator : uint8_t {
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
class TermBinary : public RecursiveExpression<TermBinary> {
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

  private:
    Location loc_;
    Util::immutable_value<Term> lhs_;
    Util::immutable_value<Term> rhs_;
    BinaryOperator op_;
};

//! @}

// ArgumentTuple

inline ArgumentTuple::ArgumentTuple(ArgumentArray elems) : elems_{std::move(elems)} {
}

inline auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TermTuple

inline TermTuple::TermTuple(Location loc, TupleElementArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {
}

inline auto operator==(TermTuple const &a, TermTuple const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TermFunction

inline TermFunction::TermFunction(Location loc, String name, PoolArray pool, bool external)
    : loc_{std::move(loc)}, name_(name), pool_{std::move(pool)}, external_{external} {
}

inline auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TermAbs

inline TermAbs::TermAbs(Location loc, TermArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {
}

inline auto operator==(TermAbs const &a, TermAbs const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TermUnary

inline TermUnary::TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{std::move(rhs)} {
}

inline auto operator==(TermUnary const &a, TermUnary const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TermBinary

inline TermBinary::TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                              Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {
}

inline auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering {
    return a.compare(b);
}

} // namespace CppClingo::Input
