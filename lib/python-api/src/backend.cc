#include <clingo/backend.h>

#include "backend.hh"
#include "util.hh"

namespace Clingo::Python {

auto Backend::atom(std::optional<Symbol> symbol) -> clingo_atom_t {
    clingo_atom_t atom = 0;
    clingo_backend_add_atom(backend_, symbol ? c_cast(&*symbol) : nullptr, &atom);
    return atom;
}

auto BackendManager::enter() -> Backend {
    handle_error(clingo_control_backend(ctl_, &backend_));
    return Backend{backend_};
}

void BackendManager::exit([[maybe_unused]] std::optional<pybind11::type> const &type,
                          [[maybe_unused]] std::optional<pybind11::object> const &value,
                          [[maybe_unused]] std::optional<pybind11::object> const &traceback) {
    if (backend_ != nullptr) {
        handle_error(clingo_backend_close(backend_));
        backend_ = nullptr;
    }
}

void register_backend(pybind11::module &m) {
    using namespace Clingo::Python;

    auto backend = m.def_submodule("backend", R"(
)"_d);

    py::class_<Backend>(backend, "Backend", R"(TODO)").def("atom", &Backend::atom, py::arg("symbol") = std::nullopt, R"(
TODO)"_d);

    py::class_<BackendManager>(backend, "BackendManager", R"(TODO)")
        .def("__enter__", &BackendManager::enter, "Initialize backend the backend.")
        .def("__exit__", &BackendManager::exit, py::arg("type"), py::arg("value"), py::arg("traceback"),
             "Finalize the backend.");
}

} // namespace Clingo::Python
