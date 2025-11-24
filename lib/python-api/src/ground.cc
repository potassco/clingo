#include "ground.hh"
#include "core.hh"
#include "util.hh"

#include <clingo/ground.h>

#include <pybind11/native_enum.h>

#include <utility>

namespace PyClingo {

auto GroundHandle::get() -> clingo_ground_result_e {
    auto release = py::gil_scoped_release{};
    clingo_ground_result_t res = 0;
    handle_error(clingo_ground_handle_get(hnd_, &res));
    return static_cast<clingo_ground_result_e>(res);
}

void GroundHandle::cancel() {
    auto release = py::gil_scoped_release{};
    handle_error(clingo_ground_handle_cancel(hnd_));
}

auto GroundHandle::wait(std::optional<double> timeout) -> bool {
    auto release = py::gil_scoped_release{};
    bool result = false;
    handle_error(clingo_ground_handle_wait(hnd_, timeout ? *timeout : -1, &result));
    return result;
}

void GroundHandle::close() {
    if (hnd_ != nullptr) {
        auto release = py::gil_scoped_release{};
        clingo_ground_handle_close(std::exchange(hnd_, nullptr));
    }
}

void register_ground(pybind11::module &m) {
    auto ground = m.def_submodule("ground", R"(
Functions and classes related to grounding.

# Examples

The example shows how call external functions during grounding:

```python
>>> from clingo.core import Library
>>> from clingo.symbol import Number
>>> from clingo.control import Control
>>>
>>> class Context:
...     def __init__(self, lib):
...       self.lib = lib
...     def inc(self, x):
...         return Number(self.lib, x.number + 1)
...     def seq(self, x, y):
...         return [x, y]
...
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("""
... p(@inc(10)).
... q(@seq(1,2)).
... """)
>>> ctl.ground(context=Context(lib))
>>> with ctl.solve(on_model=print) as hnd:
...     print(hnd.get())
p(11) q(1) q(2)
SAT
```

The example below shows how to start grounding in the background and wait for
grounding to finish:

```python
>>> from clingo.control import Control
>>> from clingo.core import Library
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("{a}.")
>>> with ctl.start_ground() as hnd:
...     w = {True: "finished", False: "running"}
...     print(f"start_ground status: {w[hnd.wait(0)]}")
...     print(f"start_ground result: {hnd.get()!r}")
...     print(f"start_ground status: {w[hnd.wait(0)]}")
...
start_ground status: running
start_ground result: <GroundResult.Ok: 0>
start_ground status: finished
```
)"_d);

    py::native_enum<clingo_ground_result_e>(ground, "GroundResult", "enum.IntEnum",
                                            R"(Enumeration of ground result types.)")
        .value("Ok", clingo_ground_result_ok, R"(Grounding finished successfully.)")
        .value("Unsatisfiable", clingo_ground_result_unsatisfiable, R"(Inconsistency detected while grounding.)")
        .value("Interrupted", clingo_ground_result_interrupted, R"(Grounding was interrupted.)")
        .finalize();

    py::class_<GroundHandle>(ground, "GroundHandle", py::custom_type_setup(GroundHandle::setup), R"(
An object to interact with a running search.

It can be used to control solving, like, retrieving models or cancelling a
search.

A GroundHandle is a context manager and must be used with Python's with
statement.

Blocking functions in this object release the GIL. They are not thread-safe
though.

See also: `clingo.control.Control.ground`
)"_d)
        .def("get", &GroundHandle::get, R"(
Get the ground result.

This is always the last function to be called on a handle to ensure that the
search is properly terminated. It might be preceded by a call to cancel to stop
the search.
)"_d)
        .def("wait", &GroundHandle::wait, py::arg("timeout") = std::nullopt, R"(
Wait for the ground call to finish or the next result with an optional timeout.

If a timeout is provided, the function blocks for the given duration or until a
result is ready. A positive timeout blocks for that amount of time. A negative
timeout blocks until a result is available, and a zero timeout allows polling
for a result.

Args:
    timeout: The maximum time to block in seconds.

Returns:
    Whether the ground call has finished or the next result is ready.
)"_d)
        .def("cancel", &GroundHandle::cancel, R"(
Cancel the running search.

See also: `clingo.control.Control.interrupt`
)"_d)
        .def(
            "__enter__", [&](GroundHandle *hnd) -> GroundHandle * { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](GroundHandle *hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd->close(); },
            "Stop grounding and close the handle.");
}

} // namespace PyClingo
