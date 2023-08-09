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
    Position begin;
    Position end;
};

template <class T> auto location(T const &x) -> Location const & { return x.loc; }

template <class... T> auto location(std::variant<T...> const &x) -> Location const & {
    return std::visit([](auto const &y) -> Location const & { return location(y); }, x);
}

} // namespace Gringo::Input
