#pragma once

#include <clingo/input/program.hh>

#include <clingo/ast.h>

struct clingo_program : public CppClingo::SymbolOwner {
    clingo_program(clingo_lib_t *lib);
    ~clingo_program() noexcept override;

    void mark(CppClingo::SymbolCollector &gc) const override;
    clingo_lib_t *lib;
    CppClingo::Input::UnprocessedProgram program;
};
