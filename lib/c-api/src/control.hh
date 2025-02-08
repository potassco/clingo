#pragma once

#include <clingo/control/solver.hh>

#include <clingo/control.h>

struct clingo_control {
    clingo_lib_t *lib;
    Clingo::Control::Solver *slv;
    Clasp::ClaspConfig *cfg;
    Clasp::ClaspFacade *clasp;
};
