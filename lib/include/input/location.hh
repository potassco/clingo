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

} // namespace Gringo::Input
