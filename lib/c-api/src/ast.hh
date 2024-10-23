#pragma once

#include <clingo/input/program.hh>

#include <clingo.h>

struct clingo_program : public Clingo::SymbolOwner {
    clingo_program(clingo_lib_t *lib);
    ~clingo_program() noexcept override;

    void mark(Clingo::SymbolCollector &gc) const override;
    clingo_lib_t *lib;
    Clingo::Input::UnprocessedProgram program;
};
