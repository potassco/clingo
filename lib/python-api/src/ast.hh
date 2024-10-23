#pragma once

#include "core.hh"
#include "util.hh"

#include <pybind11/pybind11.h>

namespace Clingo::Python {

namespace py = pybind11;

class Program {
  public:
    Program(Library &lib);
    operator clingo_program_t *() { return prg_.get(); }

  private:
    static void free(clingo_program_t *prg) noexcept;

    owner_ptr<clingo_program_t, free> prg_;
};

void register_ast(pybind11::module &m);

} // namespace Clingo::Python
