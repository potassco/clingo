#include <clingo/backend.h>

#include "backend.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

namespace Clingo::Python {

namespace {

auto map(WeightLitSpan lits) {
    static thread_local auto ret = std::vector<clingo_weighted_literal_t>{};
    ret.clear();
    ret.reserve(lits.size());
    std::ranges::transform(lits, std::back_inserter(ret),
                           [](auto const &a) { return clingo_weighted_literal_t{a.first, a.second}; });
    return ret.data();
}

} // namespace

// Backend

auto Backend::atom(std::optional<Symbol> symbol) -> clingo_atom_t {
    clingo_atom_t atom = 0;
    clingo_backend_add_atom(backend_, symbol ? c_cast(&*symbol) : nullptr, &atom);
    return atom;
}

void Backend::rule(AtomSpan head, LitSpan body, bool choice) {
    handle_error(clingo_backend_rule(backend_, choice, head.data(), head.size(), body.data(), body.size()));
}

void Backend::weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice) {
    handle_error(clingo_backend_weight_rule(backend_, choice, head.data(), head.size(), lower, map(body), body.size()));
}

// BackendManager

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
Functions and classes to observe or add ground statements.

Examples
--------
The following example shows how to add a fact to a program:

```python
>>> from clingo.symbol import Function
>>> from clingo.control import Control
>>>
>>> ctl = Control()
>>>
>>> sym = Function("a")
>>> with ctl.backend as bck:
...     atm_a = bck.atom(sym)
...     bck.rule([atm_a])
...
>>> ctl.base[sym].is_fact
True
>>>
>>> print(ctl.solve(on_model=print))
a
SAT
```
>>> from clingo.core import Library
>>> from clingo.symbol import Function
>>> from clingo.control import Control
...
>>> lib = Library()
>>> ctl = Control(lib)
...
>>> sym = Function(lib, "a")
>>> with ctl.backend as bck:
...     atm_a = bck.atom(sym)
...     bck.rule([atm_a], [])
...
>>> print(ctl.base.is_fact(ctl.base[sym].literal))
True
>>> with ctl.solve(on_model=print) as hnd:
...     print(hnd.get())
a
SAT
)"_d);

    py::class_<Backend>(backend, "Backend", R"(
Backend object providing a low level interface to extend a logic program.

This class allows for adding ground statements in ASPIF format.

See Also:
    clingo.control.Control.backend
)"_d)
        .def("atom", &Backend::atom, py::arg("symbol") = std::nullopt, R"(
Return a fresh program atom or the atom associated with the given symbol.

If the given symbol does not exist in the atom base, it is added first. Such
atoms will be used in subequents calls to ground for instantiation.

Args:
    symbol: The symbol associated with the atom.

Returns: The program atom representing the atom.
)"_d)
        .def("rule", &Backend::rule, py::arg("head"), py::arg("body"), py::arg("choice") = false, R"(
TODO
)"_d)
        .def("weight_rule", &Backend::weight_rule, py::arg("head"), py::arg("lower_bound"), py::arg("body"),
             py::arg("choice") = false, R"(
TODO
)"_d);

    py::class_<BackendManager>(backend, "BackendManager", R"(
A context manager to initialize and finalize a backend.
)"_d)
        .def("__enter__", &BackendManager::enter, "Initialize backend the backend.")
        .def("__exit__", &BackendManager::exit, py::arg("type"), py::arg("value"), py::arg("traceback"),
             "Finalize the backend.");
}

} // namespace Clingo::Python
