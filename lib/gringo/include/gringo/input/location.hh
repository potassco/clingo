#pragma once

#include <cstddef>
#include <optional>
#include <variant>

#include <gringo/symbol.hh>

namespace Gringo::Input {

//! @defgroup input_location Source Locations
//! Data structures and functions to track source locations.
//!
//! @ingroup input_language
//!
//! @{

//! A point in an input source.
struct Position {
    //! The name of a file/stream/string.
    String file;
    //! The line number.
    size_t line;
    //! The column number.
    size_t column;
};

//! The Location of an expression in an input source.
struct Location {
    //! Create a new location from the beginning and end of the given two locations.
    friend auto operator+(Location const &a, Location const &b) -> Location { return {a.begin, b.end}; }
    //! Create a new location from the beginning of the location and the position.
    friend auto operator+(Location const &a, Position b) -> Location { return {a.begin, std::move(b)}; }
    //! Create a new location from the given one optionally adjusting its end position.
    friend auto operator+(Location const &a, std::optional<Position> b) -> Location {
        if (b.has_value()) {
            return {a.begin, std::move(b).value()};
        }
        return a;
    }
    //! Create a new location from the position of the end of the location.
    friend auto operator+(Position a, Location const &b) -> Location { return {std::move(a), b.end}; }
    //! Create a new location from the given one optionally adjusting its start position.
    friend auto operator+(std::optional<Position> a, Location const &b) -> Location {
        if (a.has_value()) {
            return {std::move(a).value(), b.end};
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
        a.end = std::move(b);
        return a;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, std::optional<Position> b) -> Location & {
        if (b.has_value()) {
            a.end = std::move(b).value();
        }
        return a;
    }
    //! The position where the expression starts.
    Position begin;
    //! The position where the expression ends.
    Position end;
};

namespace Detail {
template <typename T, typename V = void> static constexpr bool has_loc = false;
template <typename T> static constexpr bool has_loc<T, std::void_t<decltype(std::declval<T>().loc())>> = true;
}; // namespace Detail

//! Create a location from the given two positions.
inline auto operator+(Position a, Position b) -> Location { return {std::move(a), std::move(b)}; }

//! Get the location of an expression.
template <class T> auto location(T const &x) -> std::enable_if_t<Detail::has_loc<T>, Location const &> {
    return x.loc();
}

//! Get the location of an expression stored in a variant.
template <class... T> auto location(std::variant<T...> const &x) -> Location const & {
    return std::visit([](auto const &y) -> Location const & { return location(y); }, x);
}

//! @}

} // namespace Gringo::Input
