#pragma once

#include <cstddef>
#include <string>

namespace Gringo::Input {

struct Position {
    std::string file;
    size_t line;
    size_t column;
};

struct Location {
    friend auto operator+=(Location &a, Location const &b) -> Location & {
        a.end = b.end;
        return a;
    }
    friend auto operator+=(Location &a, Position b) -> Location & {
        a.end = std::move(b);
        return a;
    }
    friend auto operator+=(Location &a, std::optional<Position> b) -> Location & {
        if (b.has_value()) {
            a.end = std::move(b).value();
        }
        return a;
    }
    friend auto operator+(Location const &a, Location const &b) -> Location { return {a.begin, b.end}; }
    friend auto operator+(Location const &a, Position b) -> Location { return {a.begin, std::move(b)}; }
    friend auto operator+(Location const &a, std::optional<Position> b) -> Location {
        if (b.has_value()) {
            return {a.begin, std::move(b).value()};
        }
        return a;
    }
    friend auto operator+(Position a, Location const &b) -> Location { return {std::move(a), b.end}; }
    friend auto operator+(std::optional<Position> a, Location const &b) -> Location {
        if (a.has_value()) {
            return {std::move(a).value(), b.end};
        }
        return b;
    }
    Position begin;
    Position end;
};

inline auto operator+(Position a, Position b) -> Location { return {std::move(a), std::move(b)}; }

template <class T> auto location(T &x) -> decltype((x.loc)) { return x.loc; }

template <class T> auto location(T &x) -> decltype((x.aggr.loc)) { return x.aggr.loc; }

template <class... T> auto location(std::variant<T...> &x) -> Location const & {
    return std::visit([](auto &y) -> Location & { return location(y); }, x);
}

template <class... T> auto location(std::variant<T...> const &x) -> Location const & {
    return std::visit([](auto const &y) -> Location const & { return location(y); }, x);
}

} // namespace Gringo::Input
