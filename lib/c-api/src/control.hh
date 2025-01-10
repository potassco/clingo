#pragma once

#include <clingo/control/solver.hh>

#include <clingo/core.h>

struct clingo_control {
    clingo_lib_t *lib;
    Clingo::Control::Solver *slv;
};
