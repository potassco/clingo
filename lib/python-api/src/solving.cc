#include "solving.hh"
#include "core.hh"
#include "util.hh"

#include <clingo/solve.h>

#include <utility>

namespace Clingo::Python {

namespace {

auto symbol_callback(clingo_symbol_t const *symbols, size_t size, void *data) -> clingo_result_t {
    CLINGO_TRY {
        auto *res = static_cast<SymbolVec *>(data);
        // NOLINTNEXTLINE
        res->assign(cpp_cast(symbols), cpp_cast(symbols) + size);
    }
    CLINGO_CATCH(nullptr);
}

} // namespace

auto Model::symbols(bool shown, bool atoms, bool terms, bool theory) -> SymbolVec {
    auto res = SymbolVec{};
    clingo_show_type_bitset_t show = 0;
    if (shown) {
        show |= clingo_show_type_shown;
    }
    if (atoms) {
        show |= clingo_show_type_atoms;
    }
    if (terms) {
        show |= clingo_show_type_terms;
    }
    if (theory) {
        show |= clingo_show_type_theory;
    }
    handle_error(clingo_model_symbols(mdl_, show, symbol_callback, &res));
    return res;
}

auto SolveHandle::get() -> SolveResult {
    clingo_solve_result_bitset_t res = 0;
    handle_error(clingo_solve_handle_get(hnd_, &res));
    if (ptr_) {
        std::rethrow_exception(std::exchange(ptr_, nullptr));
    }
    return {res};
}

void SolveHandle::cancel() { handle_error(clingo_solve_handle_cancel(hnd_)); }

void SolveHandle::resume() { handle_error(clingo_solve_handle_resume(hnd_)); }

auto SolveHandle::model() -> std::optional<Model> {
    auto const *mdl = static_cast<clingo_model_t const *>(nullptr);
    handle_error(clingo_solve_handle_model(hnd_, &mdl));
    return mdl != nullptr ? std::make_optional<Model>(mdl) : std::nullopt;
}

auto SolveHandle::last() -> std::optional<Model> {
    auto const *mdl = static_cast<clingo_model_t const *>(nullptr);
    handle_error(clingo_solve_handle_last(hnd_, &mdl));
    return mdl != nullptr ? std::make_optional<Model>(mdl) : std::nullopt;
}

auto SolveHandle::core() -> std::vector<clingo_literal_t> {
    auto const *lits = static_cast<clingo_literal_t *>(nullptr);
    auto size = size_t{0};
    handle_error(clingo_solve_handle_core(hnd_, &lits, &size));
    // NOLINTNEXTLINE
    return {lits, lits + size};
}

auto SolveHandle::wait(double timeout) -> bool {
    bool result = false;
    clingo_solve_handle_wait(hnd_, timeout, &result);
    return result;
}

void SolveHandle::close() {
    if (hnd_ != nullptr) {
        clingo_solve_handle_close(std::exchange(hnd_, nullptr));
    }
}

void register_solving(pybind11::module &m) {
    auto solving = m.def_submodule("solving", doc(R"(
Functions and classes related to solving.

Examples
--------

The following example shows how to intercept models with a callback:

    >>> from clingo import Control
    >>>
    >>> ctl = Control(["0"])
    >>> ctl.add("base", [], "1 { a; b } 1.")
    >>> ctl.ground([("base", [])])
    >>> print(ctl.solve(on_model=print))
    Answer: a
    Answer: b
    SAT

The following example shows how to yield models:

    >>> from clingo import Control
    >>>
    >>> ctl = Control(["0"])
    >>> ctl.add("base", [], "1 { a; b } 1.")
    >>> ctl.ground([("base", [])])
    >>> with ctl.solve(yield_=True) as hnd:
    ...     for m in hnd:
    ...         print(m)
    ...     print(hnd.get())
    ...
    Answer: a
    Answer: b
    SAT

The following example shows how to solve asynchronously:

    >>> from clingo import Control
    >>>
    >>> ctl = Control(["0"])
    >>> ctl.add("base", [], "1 { a; b } 1.")
    >>> ctl.ground([("base", [])])
    >>> with ctl.solve(on_model=print, async_=True) as hnd:
    ...     # some computation here
    ...     hnd.wait()
    ...     print(hnd.get())
    ...
    Answer: a
    Answer: b
    SAT

This example shows how to solve both iteratively and asynchronously:

    >>> from clingo import Control
    >>>
    >>> ctl = Control(["0"])
    >>> ctl.add("base", [], "1 { a; b } 1.")
    >>> ctl.ground([("base", [])])
    >>> with ctl.solve(yield_=True, async_=True) as hnd:
    ...     while True:
    ...         hnd.resume()
    ...         # some computation here
    ...         _ = hnd.wait()
    ...         m = hnd.model()
    ...         if m is None:
    ...             print(hnd.get())
    ...             break
    ...         print(m)
    b
    a
    a b
    None
)"));
    py::class_<SolveResult>(solving, "SolveResult", R"(A solve result captures information about a solve call.)")
        .def("__str__", &SolveResult::str, R"(Get a string representation of the solve result.)")
        .def_property_readonly("satisfiable", &SolveResult::satisfiable, R"(True if there was at least one model.)")
        .def_property_readonly("unsatisfiable", &SolveResult::unsatisfiable, R"(True if there was no model.)")
        .def_property_readonly("exhausted", &SolveResult::exhausted, R"(True if all models have been enumerated.)")
        .def_property_readonly("interrupted", &SolveResult::interrupted, R"(True if the search was interrupted.)");

    py::class_<Model>(solving, "Model", R"(A view on the solver's current solution.)")
        .def("symbols", &Model::symbols, py::arg("shown") = false, py::arg("atoms") = false, py::arg("terms") = false,
             py::arg("theory") = false, doc(R"(
Get the symbols in the model.

Args:
  shown: Include shown atoms and terms.
  atoms: Include all true atoms including hidden ones.
  terms: Include shown terms.
  theory: Include terms added by external theories.
)"));

    py::class_<SolveHandle>(solving, "SolveHandle", doc(R"(
An object to interact with a running search.

It can be used to control solving, like, retrieving models or cancelling a
search.

A SolveHandle is a context manager and must be used with Python's with
statement.

Blocking functions in this object release the GIL. They are not thread-safe
though.

See also: `clingo.control.Control.solve`
)"))
        .def("get", &SolveHandle::get, doc(R"(
Get the solve result.

This is always the last function that should be called on a handle to ensure
that the search is properly terminated. It might be preceded by a call to
cancel to stop a running search.
)"))
        .def("core", &SolveHandle::core, doc(R"(
Get the subset of assumptions that made the problem unsatisfiable.)"))
        .def("model", &SolveHandle::model, doc(R"(
Get the current model if there is any.
)"))
        .def("last", &SolveHandle::last, doc(R"(
Get the last computed model if there is any.

If the search is not completed yet or the problem is unsatisfiable, the
function returns `None`.
)"))
        .def("resume", &SolveHandle::resume, doc(R"(
Discards the last model and starts searching for the next one.

If the search has been started asynchronously, this function starts the search
in the background.
)"))
        .def("wait", &SolveHandle::wait, doc(R"(
Wait for solve call to finish or the next result with an optional timeout.

If a timeout is given, the behavior of the function changes depending on the
sign of the timeout. If a postive timeout is given, the function blocks for the
given amount time or until a result is ready. If the timeout is negative, the
function will block until a result is ready, which also corresponds to the
behavior of the function if no timeout is given. A timeout of zero can be used
to poll if a result is ready.

Args:
  timeout:
    If a timeout is given, the function blocks for at most timeout seconds.

Returns:
  Indicates whether the solve call has finished or the next result is ready.
)"))
        .def("cancel", &SolveHandle::cancel, doc(R"(
Cancel the running search.

See also: `clingo.control.Control.interrupt`
)"))
        .def(
            "__enter__", [&](SSolveHandle hnd) -> SSolveHandle { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](SSolveHandle const &hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd->close(); },
            "Stop the search closing the handle.");
}

} // namespace Clingo::Python
