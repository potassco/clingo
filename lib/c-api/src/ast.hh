#pragma once

#include <clingo/input/program.hh>

#include <clingo.h>

struct clingo_program {
    clingo_program(clingo_lib_t *lib) : lib{lib} {}
    clingo_lib_t *lib;
    Clingo::Input::UnprocessedProgram program;
};
