#pragma once

#include <gringo/core/symbol.hh>

#include <gringo/util/algorithm.hh>

#include <cstdint>
#include <ostream>

namespace Gringo {

//! @addtogroup core
//! @{

//! Enumeration of signs (default negation).
enum class Sign : uint8_t {
    none,  //!< No sign.
    once,  //!< One sign (not).
    twice, //!< Two signs (not not).
};

//! Negate the sign.
auto operator-(Sign a) -> Sign;
//! Combine two signs.
auto operator+(Sign a, Sign b) -> Sign;
//! Combine two signs.
auto operator+=(Sign &a, Sign b) -> Sign &;

//! Output the given sign.
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

//! Enumeration of supported relations.
enum class Relation : uint8_t {
    equal,         //!< The equal to symbol (=).
    not_equal,     //!< The not equal to symbol (!=).
    less,          //!< The less than symbol (<).
    less_equal,    //!< The less than or equal to symbol (<=).
    greater,       //!< The greater than symbol (>).
    greater_equal, //!< The greater than or equal to symbol (>=).
};

//! Return the equivalent relation when arguments are flipped.
[[nodiscard]] auto flip(Relation rel) -> Relation;
//! Return the complement of the given relation.
[[nodiscard]] auto complement(Relation rel) -> Relation;

//! Evaluate the comparison.
auto evaluate(auto const &lhs, Relation rel, auto const &rhs) -> bool {
    switch (rel) {
        case Relation::equal: {
            return lhs == rhs;
        }
        case Relation::not_equal: {
            return lhs != rhs;
        }
        case Relation::less: {
            return lhs < rhs;
        }
        case Relation::less_equal: {
            return lhs <= rhs;
        }
        case Relation::greater: {
            return lhs > rhs;
        }
        case Relation::greater_equal: {
            return lhs >= rhs;
        }
    }
    Util::unreachable();
}

//! Output the given relation.
auto operator<<(std::ostream &out, Relation rel) -> std::ostream &;

//! Truth values for expressions.
enum class TruthValue : uint8_t {
    top,     //!< Indicate a true expression.
    bot,     //!< Indicate a false expression.
    unknown, //!< Indicate an expression with an unknown truth value.
};

//! Enumeration of aggregate functions.
enum class AggregateFunction : uint8_t {
    count, //! The <tt>\#count</tt> function.
    sum,   //! The <tt>\#sum</tt> function.
    sump,  //! The <tt>\#sum+</tt> function.
    min,   //! The <tt>\#min</tt> function.
    max,   //! The <tt>\#max</tt> function.
};

//! Output the given aggregate function.
auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream &;

//! Get the neutral value for the given aggregate function.
inline auto neutral_val(AggregateFunction fun) -> Symbol {
    switch (fun) {
        case AggregateFunction::sum:
        case AggregateFunction::sump: {
            return SymbolStore::num_ref(0);
        }
        case AggregateFunction::min: {
            return SymbolStore::sup();
        }
        case AggregateFunction::max: {
            return SymbolStore::inf();
        }
        case AggregateFunction::count: {
            throw std::invalid_argument("count function has no neutral value");
        }
    }
    Util::unreachable();
}

//! Get the neutral value or number for the given aggregate function.
inline auto neutral_num(AggregateFunction fun) -> std::variant<Number, Symbol> {
    switch (fun) {
        case AggregateFunction::sum:
        case AggregateFunction::sump: {
            return Number(0);
        }
        case AggregateFunction::min: {
            return SymbolStore::sup();
        }
        case AggregateFunction::max: {
            return SymbolStore::inf();
        }
        case AggregateFunction::count: {
            throw std::invalid_argument("count function has no neutral value");
        }
    }
    Util::unreachable();
}

//! Check if the symbol is relevant for the given aggregate function.
inline auto relevant_val(AggregateFunction fun, Symbol sym) -> bool {
    switch (fun) {
        case AggregateFunction::min: {
            return sym.type() != SymbolType::sup;
        }
        case AggregateFunction::max: {
            return sym.type() != SymbolType::inf;
        }
        case AggregateFunction::sum: {
            return sym.type() == SymbolType::number && sym.num() != 0;
        }
        case AggregateFunction::sump: {
            return sym.type() == SymbolType::number && sym.num() > 0;
        }
        case AggregateFunction::count: {
            return true;
        }
    }
    Util::unreachable();
}

//! @}

} // namespace Gringo
