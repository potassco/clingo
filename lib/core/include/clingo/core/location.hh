#pragma once

#include <clingo/core/symbol.hh>

#include <cstddef>
#include <optional>
#include <variant>

namespace CppClingo {

//! @addtogroup core_location
//! @{

//! A point in an input source.
class Position {
  public:
    //! Construct a position.
    Position(String file, size_t line, size_t column) : file_{file}, line_{line}, column_{column} {}
    //! The name of a file/stream/string.
    [[nodiscard]] auto file() const -> String const & { return *file_; }
    //! The line number.
    [[nodiscard]] auto line() const -> size_t { return line_; }
    //! The column number.
    [[nodiscard]] auto column() const -> size_t { return column_; }

    //! Compare two positions.
    friend auto operator==(Position const &a, Position const &b) -> bool = default;
    //! Compare two positions.
    friend auto operator<=>(Position const &a, Position const &b) = default;

    //! Output the position to the given stream.
    template <class T> friend auto operator<<(T &out, Position const &pos) -> T & {
        out << pos.file() << ":" << pos.line() << ":" << pos.column();
        return out;
    }

  private:
    SharedString file_;
    size_t line_;
    size_t column_;
};

//! The Location of an expression in an input source.
class Location {
  public:
    //! Construct a location.
    Location(Position begin, Position end) : begin_{std::move(begin)}, end_{std::move(end)} {}

    //! The position where the expression starts.
    [[nodiscard]] auto begin() const -> Position const & { return begin_; }
    //! The position where the expression ends.
    [[nodiscard]] auto end() const -> Position const & { return end_; }

    //! Create a new location from the beginning and end of the given two locations.
    friend auto operator+(Location const &a, Location const &b) -> Location { return {a.begin_, b.end_}; }
    //! Create a new location from the beginning of the location and the position.
    friend auto operator+(Location const &a, Position b) -> Location { return {a.begin_, std::move(b)}; }
    //! Create a new location from the given one optionally adjusting its end position.
    friend auto operator+(Location const &a, std::optional<Position> b) -> Location {
        if (b.has_value()) {
            return {a.begin_, b.value()};
        }
        return a;
    }
    //! Create a new location from the position of the end of the location.
    friend auto operator+(Position a, Location const &b) -> Location { return {std::move(a), b.end_}; }
    //! Create a new location from the given one optionally adjusting its start position.
    friend auto operator+(std::optional<Position> a, Location const &b) -> Location {
        if (a) {
            return {*a, b.end_};
        }
        return b;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, Location const &b) -> Location & {
        a.end_ = b.end_;
        return a;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, Position b) -> Location & {
        a.end_ = std::move(b);
        return a;
    }
    //! See the corresponding + operator.
    friend auto operator+=(Location &a, std::optional<Position> b) -> Location & {
        if (b) {
            a.end_ = *std::move(b);
        }
        return a;
    }

    //! Compare two positions.
    friend auto operator==(Location const &a, Location const &b) -> bool = default;
    //! Compare two positions.
    friend auto operator<=>(Location const &a, Location const &b) = default;

    //! Output the location to the given stream.
    template <class T> friend auto operator<<(T &out, Location const &loc) -> T & {
        out << loc.begin() << "-";
        if (loc.end().file() != loc.begin().file()) {
            out << loc.end();
        } else if (loc.end().line() != loc.begin().line()) {
            out << loc.end().line() << ":" << loc.end().column();
        } else {
            out << loc.end().column();
        }
        return out;
    }

  private:
    Position begin_;
    Position end_;
};

//! Create a location from the given two positions.
inline auto operator+(Position a, Position b) -> Location {
    return {std::move(a), std::move(b)};
}

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

} // namespace CppClingo
