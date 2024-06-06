#pragma once

#include <gringo/core/symbol.hh>

#include <cstddef>
#include <optional>
#include <variant>

namespace Gringo::Input {

//! @addtogroup core_location
//! @{

//! A point in an input source.
struct Position {
    //! The name of a file/stream/string.
    String file;
    //! The line number.
    size_t line;
    //! The column number.
    size_t column;

    //! Compare two positions.
    friend auto operator==(Position const &a, Position const &b) -> bool = default;
    //! Compare two positions.
    friend auto operator<=>(Position const &a, Position const &b) = default;
};

//! The Location of an expression in an input source.
struct Location {
    //! Create a new location from the beginning and end of the given two locations.
    friend auto operator+(Location const &a, Location const &b) -> Location { return {a.begin, b.end}; }
    //! Create a new location from the beginning of the location and the position.
    friend auto operator+(Location const &a, Position b) -> Location { return {a.begin, b}; }
    //! Create a new location from the given one optionally adjusting its end position.
    friend auto operator+(Location const &a, std::optional<Position> b) -> Location {
        if (b.has_value()) {
            return {a.begin, b.value()};
        }
        return a;
    }
    //! Create a new location from the position of the end of the location.
    friend auto operator+(Position a, Location const &b) -> Location { return {a, b.end}; }
    //! Create a new location from the given one optionally adjusting its start position.
    friend auto operator+(std::optional<Position> a, Location const &b) -> Location {
        if (a) {
            return {*a, b.end};
        }
        return b;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, Location const &b) -> Location & {
        a.end = b.end;
        return a;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, Position b) -> Location & {
        a.end = b;
        return a;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, std::optional<Position> b) -> Location & {
        if (b) {
            a.end = *b;
        }
        return a;
    }

    //! Compare two positions.
    friend auto operator==(Location const &a, Location const &b) -> bool = default;
    //! Compare two positions.
    friend auto operator<=>(Location const &a, Location const &b) = default;

    //! The position where the expression starts.
    Position begin;
    //! The position where the expression ends.
    Position end;
};

//! Create a location from the given two positions.
inline auto operator+(Position a, Position b) -> Location { return {a, b}; }

//! Get the location of an expression.
template <class T>
    requires requires(T const &x) { x.loc(); }
auto location(T const &x) -> Location const & {
    return x.loc();
}

//! Get the location of an expression stored in a variant.
template <class... T>
    requires requires(T const &...x) { (location(x), ...); }
auto location(std::variant<T...> const &x) -> Location const & {
    return std::visit([](auto const &y) -> Location const & { return location(y); }, x);
}

//! @}

} // namespace Gringo::Input
