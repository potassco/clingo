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
class Projection {
  public:
    //! Construct a projection indicator.
    explicit Projection(Location loc) : loc_{loc} {}
    //! The location of the projected position.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }

    //! Update the record.
    template <bool Opt = false, class... Args> auto update(Args &&...args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc}, Types{args...});
        return Projection{select<Opt>(a_loc, loc_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(Projection const &a, Projection const &b) -> bool;
    friend auto operator<=>(Projection const &a, Projection const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<Projection>;

    Location loc_;
};

//! Compare two projected positions.
//!
//! @related Projection
auto operator==(Projection const &a, Projection const &b) -> bool;

//! Compare two projected positions.
//!
//! @related Projection
auto operator<=>(Projection const &a, Projection const &b) -> std::strong_ordering;

//! A variant capturing either a term or a position that is to be projected.
using Argument = std::variant<Projection, Term>;
//! A tuple of terms or positions to project.
using ArgumentArray = Util::immutable_array<Argument>;

class ArgumentTuple {
  public:
    //! Construct an argument tuple.
    ArgumentTuple(ArgumentArray elems);

    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> ArgumentArray const &;

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_elems}, Types{args...});
        return ArgumentTuple{select<Opt>(a_elems, elems_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;
    friend auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<ArgumentTuple>;

    ArgumentArray elems_;
};

//! Compare two argument tuples.
//!
//! @related ArgumentTuple
auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool;

//! Compare two argument tuples.
//!
//! @related ArgumentTuple
auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering;

//! A vector of tuples used as function or predicate arguments.
using PoolArray = Util::immutable_array<ArgumentTuple>;

//! Term representing a variable.
//!
//! For example <tt>X</tt>.
class TermVariable {
  public:
    //! Construct a variable.
    explicit TermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{std::move(name)}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String { return name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_anonymous}, Types{args...});
        return TermVariable{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                            select<Opt>(a_anonymous, anonymous_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermVariable const &a, TermVariable const &b) -> bool;
    friend auto operator<=>(TermVariable const &a, TermVariable const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermVariable>;

    Location loc_;
    String name_;
    bool anonymous_;
};

//! Compare two variables.
//!
//! @related TermVariable
auto operator==(TermVariable const &a, TermVariable const &b) -> bool;

//! Compare two variables.
//!
//! @related TermVariable
auto operator<=>(TermVariable const &a, TermVariable const &b) -> std::strong_ordering;

//! Term representing a symbol.
//!
//! For example <tt>1</tt>.
class TermSymbol {
  public:
    //! Construct term with the given symbol.
    explicit TermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{std::move(value)} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The associated symbol.
    [[nodiscard]] auto value() const -> Symbol { return value_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_value}, Types{args...});
        return TermSymbol{select<Opt>(a_loc, loc_, args...), select<Opt>(a_value, value_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;
    friend auto operator<=>(TermSymbol const &a, TermSymbol const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermSymbol>;

    Location loc_;
    Symbol value_;
};

//! Compare two symbols.
//!
//! @related TermSymbol
auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool;

//! Compare two symbols.
//!
//! @related TermSymbol
auto operator<=>(TermSymbol const &a, TermSymbol const &b) -> std::strong_ordering;

//! A tuple element.
using TupleElement = std::variant<ArgumentTuple, Term>;
//! A vector of tuple elements.
using TupleElementArray = Util::immutable_array<TupleElement>;

//! Term representing a tuple.
//!
//! For example <tt>(a,b;c)</tt>.
class TermTuple {
  public:
    //! Construct a  tuple.
    explicit TermTuple(Location loc, TupleElementArray pool);

    //! The location of the tuple.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the tuple.
    [[nodiscard]] auto pool() const -> TupleElementArray const &;

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_pool}, Types{args...});
        return TermTuple{select<Opt>(a_loc, loc_, args...), select<Opt>(a_pool, pool_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermTuple const &a, TermTuple const &b) -> bool;
    friend auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermTuple>;

    Location loc_;
    TupleElementArray pool_;
};

//! Compare two tuple terms.
//!
//! @related TermTuple
auto operator==(TermTuple const &a, TermTuple const &b) -> bool;

//! Compare two tuple terms.
//!
//! @related TermTuple
auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering;

//! Term representing a symbolic or external function.
//!
//! For example <tt>f(a,b;c)</tt>.
class TermFunction {
  public:
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

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_name, a_pool, a_exteral}, Types{args...});
        return TermFunction{select<Opt>(a_loc, loc_, args...), select<Opt>(a_name, name_, args...),
                            select<Opt>(a_pool, pool_, args...), select<Opt>(a_exteral, external_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermFunction const &a, TermFunction const &b) -> bool;
    friend auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermFunction>;

    Location loc_;
    String name_;
    PoolArray pool_;
    bool external_;
};

//! Compare two function terms.
//!
//! @related TermFunction
auto operator==(TermFunction const &a, TermFunction const &b) -> bool;

//! Compare two function terms.
//!
//! @related TermFunction
auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering;

//! Term representing the absolute function.
//!
//! For example <tt>|-X|</tt>.
class TermAbs {
  public:
    //! Construct an absolute term.
    //!
    //! The term has a pool of arguments, which will be reduced to a single element after calling Term::unpool().
    explicit TermAbs(Location loc, TermArray pool);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The argument pool of the absolute term.
    [[nodiscard]] auto pool() const -> TermArray const &;

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_pool}, Types{args...});
        return TermAbs{select<Opt>(a_loc, loc_, args...), select<Opt>(a_pool, pool_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermAbs const &a, TermAbs const &b) -> bool;
    friend auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermAbs>;

    Location loc_;
    TermArray pool_;
};

//! Compare two absolute terms.
//!
//! @related TermAbs
auto operator==(TermAbs const &a, TermAbs const &b) -> bool;

//! Compare two absolute terms.
//!
//! @related TermAbs
auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering;

//! Enumeration of available unary operators.
enum class UnaryOperator : int {
    negate, //!< The unary minus sign (-).
    invert, //!< The unary negation sign (~).
};

//! Term representing an unary operation.
//!
//! For example <tt>-X</tt>.
class TermUnary {
  public:
    //! Construct a term for an unary operation.
    explicit TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The operation.
    [[nodiscard]] auto op() const -> UnaryOperator { return op_; }
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Util::immutable_value<Term> const &;

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_op, a_rhs}, Types{args...});
        return TermUnary{select<Opt>(a_loc, loc_, args...), select<Opt>(a_op, op_, args...),
                         select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermUnary const &a, TermUnary const &b) -> bool;
    friend auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermUnary>;

    Location loc_;
    UnaryOperator op_;
    Util::immutable_value<Term> rhs_;
};

//! Compare two unary terms.
//!
//! @related TermUnary
auto operator==(TermUnary const &a, TermUnary const &b) -> bool;

//! Compare two unary terms.
//!
//! @related TermUnary
auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering;

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
class TermBinary {
  public:
    //! Construct a term for an binary operation.
    explicit TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                        Util::immutable_value<Term> rhs);

    //! The location of the function.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The left-hand-side.
    [[nodiscard]] auto lhs() const -> Util::immutable_value<Term> const &;
    //! The right-hand-side.
    [[nodiscard]] auto rhs() const -> Util::immutable_value<Term> const &;
    //! The operation.
    [[nodiscard]] auto op() const -> BinaryOperator { return op_; }

    //! Record update.
    template <bool Opt = false, class... Args> auto update(Args... args) const {
        using namespace Gringo::Util::Record;
        check(Types{a_loc, a_lhs, a_op, a_rhs}, Types{args...});
        return TermBinary{select<Opt>(a_loc, loc_, args...), select<Opt>(a_lhs, lhs_, args...),
                          select<Opt>(a_op, op_, args...), select<Opt>(a_rhs, rhs_, args...)};
    }
    //! Rewrite the record.
    template <class... Args> auto rewrite(Args &&...args) const {
        return Gringo::Util::Record::rewrite(*this, std::forward<Args>(args)...);
    }

  private:
    friend auto operator==(TermBinary const &a, TermBinary const &b) -> bool;
    friend auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering;
    friend struct Util::value_hasher<TermBinary>;

    Location loc_;
    Util::immutable_value<Term> lhs_;
    Util::immutable_value<Term> rhs_;
    BinaryOperator op_;
};

//! Compare two binary terms.
//!
//! @related TermBinary
auto operator==(TermBinary const &a, TermBinary const &b) -> bool;

//! Compare two binary terms.
//!
//! @related TermBinary
auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering;

//! @}

// Note that constructors are defined here because at this point all types are
// complete.

inline ArgumentTuple::ArgumentTuple(ArgumentArray elems) : elems_{std::move(elems)} {}

inline auto ArgumentTuple::elems() const -> ArgumentArray const & { return elems_; }

inline TermTuple::TermTuple(Location loc, TupleElementArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto TermTuple::pool() const -> TupleElementArray const & { return pool_; }

inline TermFunction::TermFunction(Location loc, String name, PoolArray pool, bool external)
    : loc_{std::move(loc)}, name_(std::move(name)), pool_{std::move(pool)}, external_{external} {}

inline TermAbs::TermAbs(Location loc, TermArray pool) : loc_{std::move(loc)}, pool_{std::move(pool)} {}

inline auto TermAbs::pool() const -> TermArray const & { return pool_; }

// TermUnary

inline TermUnary::TermUnary(Location loc, UnaryOperator op, Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, op_{op}, rhs_{std::move(rhs)} {}

inline auto TermUnary::rhs() const -> Util::immutable_value<Term> const & { return rhs_; }

inline auto operator==(TermUnary const &a, TermUnary const &b) -> bool {
    return std::tie(a.op_, a.rhs_) == std::tie(b.op_, b.rhs_);
};

inline auto operator<=>(TermUnary const &a, TermUnary const &b) -> std::strong_ordering {
    return std::tie(a.op_, a.rhs_) <=> std::tie(b.op_, b.rhs_);
}

// TermBinary

inline TermBinary::TermBinary(Location loc, Util::immutable_value<Term> lhs, BinaryOperator op,
                              Util::immutable_value<Term> rhs)
    : loc_{std::move(loc)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}

inline auto TermBinary::lhs() const -> Util::immutable_value<Term> const & { return lhs_; }

inline auto TermBinary::rhs() const -> Util::immutable_value<Term> const & { return rhs_; }

inline auto operator==(TermBinary const &a, TermBinary const &b) -> bool {
    return std::tie(*a.lhs_, a.op_, a.rhs_) == std::tie(*b.lhs_, b.op_, b.rhs_);
};

inline auto operator<=>(TermBinary const &a, TermBinary const &b) -> std::strong_ordering {
    return std::tie(*a.lhs_, a.op_, a.rhs_) <=> std::tie(*b.lhs_, b.op_, b.rhs_);
}

// TODO...

inline auto operator==(Projection const &a, Projection const &b) -> bool {
    static_cast<void>(a);
    static_cast<void>(b);
    return true;
};

inline auto operator<=>(Projection const &a, Projection const &b) -> std::strong_ordering {
    static_cast<void>(a);
    static_cast<void>(b);
    return 1 <=> 1;
}

inline auto operator==(ArgumentTuple const &a, ArgumentTuple const &b) -> bool { return a.elems_ == b.elems_; }

inline auto operator<=>(ArgumentTuple const &a, ArgumentTuple const &b) -> std::strong_ordering {
    return a.elems_ <=> b.elems_;
}

inline auto operator==(TermVariable const &a, TermVariable const &b) -> bool { return a.name_ == b.name_; }

inline auto operator<=>(TermVariable const &a, TermVariable const &b) -> std::strong_ordering {
    return a.name_ <=> b.name_;
}

inline auto operator==(TermSymbol const &a, TermSymbol const &b) -> bool { return a.value_ == b.value_; }

inline auto operator<=>(TermSymbol const &a, TermSymbol const &b) -> std::strong_ordering {
    return a.value_ <=> b.value_;
}

inline auto operator==(TermTuple const &a, TermTuple const &b) -> bool { return a.pool_ == b.pool_; }

inline auto operator<=>(TermTuple const &a, TermTuple const &b) -> std::strong_ordering { return a.pool_ <=> b.pool_; }

inline auto operator==(TermFunction const &a, TermFunction const &b) -> bool {
    return std::tie(a.name_, a.pool_, a.external_) == std::tie(b.name_, b.pool_, b.external_);
}

inline auto operator<=>(TermFunction const &a, TermFunction const &b) -> std::strong_ordering {
    return std::tie(a.name_, a.pool_, a.external_) <=> std::tie(b.name_, b.pool_, b.external_);
}

inline auto operator==(TermAbs const &a, TermAbs const &b) -> bool { return a.pool_ == b.pool_; }

inline auto operator<=>(TermAbs const &a, TermAbs const &b) -> std::strong_ordering { return a.pool_ <=> b.pool_; }

} // namespace Gringo::Input

#ifndef GRINGO_DOXYGEN_SKIP

GRINGO_HASH_PROTO(Gringo::Input::ArgumentTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermVariable);
GRINGO_HASH_PROTO(Gringo::Input::TermSymbol);
GRINGO_HASH_PROTO(Gringo::Input::TermFunction);
GRINGO_HASH_PROTO(Gringo::Input::TermTuple);
GRINGO_HASH_PROTO(Gringo::Input::TermAbs);
GRINGO_HASH_PROTO(Gringo::Input::TermUnary);
GRINGO_HASH_PROTO(Gringo::Input::TermBinary);
GRINGO_HASH_PROTO(Gringo::Input::Projection);

#endif
