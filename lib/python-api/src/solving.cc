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

class ModelIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Model;
    using pointer = Model *;
    using reference = Model &;

    ModelIterator() = default;
    ModelIterator(SSolveHandle hnd) : hnd_(std::move(hnd)) { operator++(); }

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    auto operator*() -> reference { return mdl_.value(); }
    auto operator->() -> pointer { return &mdl_.value(); }
    // NOLINTEND(bugprone-unchecked-optional-access)

    auto operator++() -> ModelIterator & {
        if (hnd_) {
            hnd_->resume();
            mdl_ = hnd_->model();
            if (!mdl_) {
                hnd_.reset();
            }
        }
        return *this;
    }
    auto operator++(int) -> ModelIterator {
        ModelIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    [[maybe_unused]] friend auto operator==(ModelIterator const &a, ModelIterator const &b) -> bool {
        return a.hnd_ == b.hnd_;
    }
    [[maybe_unused]] friend auto operator!=(ModelIterator const &a, ModelIterator const &b) -> bool {
        return a.hnd_ != b.hnd_;
    }

  private:
    SSolveHandle hnd_;
    std::optional<Model> mdl_;
};

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

auto Model::str() -> std::string {
    auto res = std::string{};
    auto comma = false;
    for (auto const &sym : symbols(true, false, false, false)) {
        res += comma ? ", " : "";
        res += sym.str();
        comma = true;
    }
    return res;
};

auto SolveHandle::get() -> SolveResult {
    clingo_solve_result_bitset_t res = 0;
    handle_error(clingo_solve_handle_get(hnd_, &res), ptr_);
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

auto SolveHandle::wait(std::optional<double> timeout) -> bool {
    bool result = false;
    clingo_solve_handle_wait(hnd_, timeout ? *timeout : -1, &result);
    return result;
}

void SolveHandle::close() {
    if (hnd_ != nullptr) {
        clingo_solve_handle_close(std::exchange(hnd_, nullptr));
    }
}

void register_solving(pybind11::module &m) {
    auto solving = m.def_submodule("solving", R"(
Functions and classes related to solving.

Examples
--------

The following example shows how to intercept models with a callback:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(on_model=print) as hnd:
    ...     print(hnd.get())
    ...
    a
    b
    SAT

The following example shows how to yield models:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(yield_=True) as hnd:
    ...     for m in hnd:
    ...         print(m)
    ...     print(hnd.get())
    ...
    a
    b
    SAT

The following example shows how to solve asynchronously:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(on_model=print, async_=True) as hnd:
    ...     # some computation here
    ...     print(hnd.get())
    ...
    a
    b
    SAT

This example shows how to solve both iteratively and asynchronously:

    >>> from clingo.core import Library
    >>> from clingo.control import Control
    >>>
    >>> lib = Library()
    >>> ctl = Control(lib, ["0"])
    >>> ctl.parse_string("1 { a; b } 1.")
    >>> ctl.ground()
    >>> with ctl.solve(yield_=True, async_=True) as hnd:
    ...     while True:
    ...         hnd.resume()
    ...         # some computation here
    ...         m = hnd.model()
    ...         if m is None:
    ...             print(hnd.get())
    ...             break
    ...         print(m)
    ...
    b
    a
    SAT
)"_d);
    py::class_<SolveResult>(solving, "SolveResult", R"(A solve result captures information about a solve call.)")
        .def("__str__", &SolveResult::str, R"(Get a string representation of the solve result.)")
        .def_property_readonly("satisfiable", &SolveResult::satisfiable, R"(Whether there was at least one model.)")
        .def_property_readonly("unsatisfiable", &SolveResult::unsatisfiable, R"(Whether there was no model.)")
        .def_property_readonly("unknown", &SolveResult::unknown, R"(Whether the satisfiablity could be determined.)")
        .def_property_readonly("exhausted", &SolveResult::exhausted, R"(Whether all models have been enumerated.)")
        .def_property_readonly("interrupted", &SolveResult::interrupted, R"(Whether the search was interrupted.)");

    py::class_<Model>(solving, "Model", R"(A view on the solver's current solution.)")
        .def("symbols", &Model::symbols, py::arg("shown") = false, py::arg("atoms") = false, py::arg("terms") = false,
             py::arg("theory") = false, R"(
Get the symbols in the model.

Args:
  shown: Include shown atoms and terms.
  atoms: Include all true atoms including hidden ones.
  terms: Include shown terms.
  theory: Include terms added by external theories.
)"_d)
        .def("__str__", &Model::str, "Get a string representation of the model.");

    py::class_<SolveHandle, SSolveHandle>(solving, "SolveHandle", R"(
An object to interact with a running search.

It can be used to control solving, like, retrieving models or cancelling a
search.

A SolveHandle is a context manager and must be used with Python's with
statement.

Blocking functions in this object release the GIL. They are not thread-safe
though.

See also: `clingo.control.Control.solve`
)"_d)
        .def("get", &SolveHandle::get, R"(
Get the solve result.

This is always the last function that should be called on a handle to ensure
that the search is properly terminated. It might be preceded by a call to
cancel to stop a running search.
)"_d)
        .def("core", &SolveHandle::core, R"(Get the subset of assumptions that made the problem unsatisfiable.)")
        .def("model", &SolveHandle::model, R"(Get the current model if there is any.)")
        .def("last", &SolveHandle::last, R"(
Get the last computed model if there is any.

If the search is not completed yet or the problem is unsatisfiable, the
function returns `None`.
)"_d)
        .def("resume", &SolveHandle::resume, R"(
Discards the last model and starts searching for the next one.

If the search has been started asynchronously, this function starts the search
in the background.
)"_d)
        .def("wait", &SolveHandle::wait, py::arg("timeout") = std::nullopt, R"(
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
)"_d)
        .def("cancel", &SolveHandle::cancel, R"(
Cancel the running search.

See also: `clingo.control.Control.interrupt`
)"_d)
        .def(
            "__enter__", [&](SSolveHandle hnd) -> SSolveHandle { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](SSolveHandle const &hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd->close(); },
            "Stop the search closing the handle.")
        .def(
            "__iter__",
            [&](SSolveHandle const &hnd) { return pybind11::make_iterator(ModelIterator{hnd}, ModelIterator{}); },
            "Get an iterator over the models.");
}

} // namespace Clingo::Python
