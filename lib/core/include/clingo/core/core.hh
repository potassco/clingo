#pragma once

#include <clingo/core/symbol.hh>

#include <clingo/util/algorithm.hh>

#include <cstdint>
#include <ostream>

namespace CppClingo {

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
//! Output the given sign.
auto operator<<(Util::OutputBuffer &out, Sign sign) -> Util::OutputBuffer &;

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
//! Output the given relation.
auto operator<<(Util::OutputBuffer &out, Relation rel) -> Util::OutputBuffer &;

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
//! Output the given aggregate function.
auto operator<<(Util::OutputBuffer &out, AggregateFunction fun) -> Util::OutputBuffer &;

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

//! Enumeration of theory term tuple types.
//!
//! @related TheoryTermTuple
enum class TheoryTermTupleType : uint8_t {
    tuple, //!< A tuple of terms enclosed in parentheses.
    set,   //!< A set of terms enclosed in braces.
    list   //!< A list of terms enclosed in brackets.
};

//! Available heuristic types.
enum class HeuristicType : uint8_t { level = 0, sign = 1, factor = 2, init = 3, true_ = 4, false_ = 5 };
//! Output the given heuristic type.
auto operator<<(Util::OutputBuffer &out, HeuristicType type) -> Util::OutputBuffer &;

//! Available external types.
enum class ExternalType : uint8_t { free = 0, true_ = 1, false_ = 2, release = 3 };
//! Output the given external type.
auto operator<<(Util::OutputBuffer &out, ExternalType type) -> Util::OutputBuffer &;

//! A signature of a theory atom.
using TheorySig = std::tuple<String, size_t>;
//! A vector of theory atom signatures.
using TheorySigVec = std::vector<TheorySig>;

//! Available grounding results.
enum class GroundResult : uint8_t {
    ok,            //!< The grounding was successful.
    unsatisfiable, //!< The grounding was successful but an inconsistency was derived.
    interrupted,   //!< The grounding was interrupted.
};

//! @}

} // namespace CppClingo
