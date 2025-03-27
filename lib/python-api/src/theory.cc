#include "control.hh"

#include <clingo/theory.h>

namespace Clingo::Python {

class Theory {
  public:
    Theory(py::capsule ptr) : ptr_{std::move(ptr)}, theory_{static_cast<clingo_theory_t *>(ptr_.get_pointer())} {
        if (std::strcmp(ptr_.name(), "clingo_theory_t") != 0) {
            throw std::invalid_argument("clingo_theory_t pointer expected");
        }
    }

    auto version() {
        int major = 0;
        int minor = 0;
        int revision = 0;
        handle_error(theory_->info(theory_->self, nullptr, &major, &minor, &revision));
        return std::make_tuple(major, minor, revision);
    }

    auto name() -> auto const * {
        char const *name = nullptr;
        handle_error(theory_->info(theory_->self, &name, nullptr, nullptr, nullptr));
        return name;
    }

    void register_theory(Control &ctl) { handle_error(theory_->register_theory(theory_->self, ctl.c_ptr())); }

  private:
    py::capsule ptr_;
    clingo_theory_t *theory_;
};

void register_theory(pybind11::module &m) {
    using namespace Clingo::Python;

    auto theory = m.def_submodule("theory", R"(
This module allows for using theories implemented in C from Python.
)"_d);

    py::class_<Theory>(theory, "Theory", R"(
Object to call functions from a C-library implementing a custom theory.
)"_d)
        .def(py::init<py::capsule>(), py::arg("theory_pointer"), R"(
Construct a theory object form the given pointer.

Args:
    theory_pointer: A capsule object holding a clingo_theory_t pointer.
)"_d)
        .def("register", &Theory::register_theory, py::arg("control"), R"(
Register the theory with the given control object.

This function should be called once on the control object before grounding and
solving starts.

Args:
    control: The control object to register the theory with.
)"_d)
        .def_property_readonly("version", &Theory::version, "Get the version of the theory (major, minor, revision).")
        .def_property_readonly("name", &Theory::name, "Get the name of the theory.");
}

} // namespace Clingo::Python
