#pragma once

#include <iostream>

struct TheoryAtom {
    friend auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream & {
        out << "&p{...}";
        return out;
    }
};
