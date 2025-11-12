#include "solve.hh"
#include "core.hh"
#include "iterable.hh" // IWYU pragma: keep
#include "util.hh"

#include <clingo/solve.h>

#include <utility>

namespace PyClingo {

namespace {

auto symbol_callback(clingo_symbol_t const *symbols, size_t size, void *data) -> bool {
    auto *res = static_cast<SymbolVec *>(data);
    CLINGO_TRY {
        // NOLINTNEXTLINE
        res->assign(cpp_cast(symbols), cpp_cast(symbols) + size);
    }
    CLINGO_CATCH;
}

class ModelIterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Model;
    using pointer = Model *;
    using reference = Model &;

    ModelIterator() = default;
    ModelIterator(SolveHandle *hnd) : hnd_(hnd) { operator++(); }

    // NOLINTBEGIN(bugprone-unchecked-optional-access)
    auto operator*() -> reference { return mdl_.value(); }
    auto operator->() -> pointer { return &mdl_.value(); }
    // NOLINTEND(bugprone-unchecked-optional-access)

    auto operator++() -> ModelIterator & {
        if (hnd_ != nullptr) {
            hnd_->resume();
            mdl_ = hnd_->model();
            if (!mdl_) {
                hnd_ = nullptr;
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
    SolveHandle *hnd_ = nullptr;
    std::optional<Model> mdl_;
};

} // namespace

auto SolveControl::base() -> Base {
    clingo_base_t const *base = nullptr;
    handle_error(clingo_solve_control_base(ctl_, &base));
    return {base};
}

auto SolveControl::add_clause(MixedLitSpan const &lits) {
    auto x = convert(base(), lits, false);
    handle_error(clingo_solve_control_add_clause(ctl_, x.data(), x.size()));
}

auto SolveControl::add_nogood(MixedLitSpan const &lits) {
    auto x = convert(base(), lits, true);
    handle_error(clingo_solve_control_add_clause(ctl_, x.data(), x.size()));
}

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

auto Model::contains(Symbol atom) -> bool {
    bool res = false;
    handle_error(clingo_model_contains(mdl_, *c_cast(&atom), &res));
    return res;
}

auto Model::control() -> SolveControl {
    clingo_solve_control_t *ctl = nullptr;
    auto *mdl = const_cast<clingo_model_t *>(mdl_); // NOLINT
    handle_error(clingo_model_control(mdl, &ctl));
    return ctl;
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

auto Model::type() -> clingo_model_type_e {
    clingo_model_type_t type = 0;
    handle_error(clingo_model_type(mdl_, &type));
    return static_cast<clingo_model_type_e>(type);
}

auto Model::number() -> uint64_t {
    uint64_t num = 0;
    handle_error(clingo_model_number(mdl_, &num));
    return num;
}

auto Model::is_true(clingo_literal_t lit) -> bool {
    auto res = false;
    handle_error(clingo_model_is_true(mdl_, lit, &res));
    return res;
}

auto Model::is_consequence(clingo_literal_t lit) -> std::optional<bool> {
    clingo_consequence_t res = 0;
    handle_error(clingo_model_is_consequence(mdl_, lit, &res));
    if (res != clingo_consequence_unknown) {
        return res == clingo_consequence_true;
    }
    return std::nullopt;
}

auto Model::cost() -> std::span<int64_t const> {
    int64_t const *costs = nullptr;
    size_t size = 0;
    handle_error(clingo_model_cost(mdl_, &costs, &size));
    return {costs, size};
}

auto Model::priorities() -> std::span<clingo_weight_t const> {
    clingo_weight_t const *prios = nullptr;
    size_t size = 0;
    handle_error(clingo_model_priority(mdl_, &prios, &size));
    return {prios, size};
}

auto Model::optimality_proven() -> bool {
    bool res = false;
    handle_error(clingo_model_optimality_proven(mdl_, &res));
    return res;
}

auto Model::thread_id() -> clingo_id_t {
    clingo_id_t id = 0;
    handle_error(clingo_model_thread_id(mdl_, &id));
    return id;
}

auto Model::extend(std::span<Symbol const> symbols) {
    auto *mdl = const_cast<clingo_model_t *>(mdl_); // NOLINT
    handle_error(clingo_model_extend(mdl, c_cast(symbols.data()), symbols.size()));
}

auto SolveHandle::get() -> SolveResult {
    auto release = py::gil_scoped_release{};
    clingo_solve_result_bitset_t res = 0;
    handle_error(clingo_solve_handle_get(hnd_, &res));
    return {res};
}

void SolveHandle::cancel() {
    auto release = py::gil_scoped_release{};
    handle_error(clingo_solve_handle_cancel(hnd_));
}

void SolveHandle::resume() {
    handle_error(clingo_solve_handle_resume(hnd_));
}

auto SolveHandle::model() -> std::optional<Model> {
    auto release = py::gil_scoped_release{};
    auto const *mdl = static_cast<clingo_model_t const *>(nullptr);
    handle_error(clingo_solve_handle_model(hnd_, &mdl));
    return mdl != nullptr ? std::make_optional<Model>(mdl) : std::nullopt;
}

auto SolveHandle::last() -> std::optional<Model> {
    auto const *mdl = static_cast<clingo_model_t const *>(nullptr);
    handle_error(clingo_solve_handle_last(hnd_, &mdl));
    return mdl != nullptr ? std::make_optional<Model>(mdl) : std::nullopt;
}

auto SolveHandle::core() -> std::span<clingo_literal_t const> {
    auto release = py::gil_scoped_release{};
    auto const *lits = static_cast<clingo_literal_t *>(nullptr);
    auto size = size_t{0};
    handle_error(clingo_solve_handle_core(hnd_, &lits, &size));
    return {lits, size};
}

auto SolveHandle::wait(std::optional<double> timeout) -> bool {
    auto release = py::gil_scoped_release{};
    bool result = false;
    handle_error(clingo_solve_handle_wait(hnd_, timeout ? *timeout : -1, &result));
    return result;
}

void SolveHandle::close() {
    if (hnd_ != nullptr) {
        auto release = py::gil_scoped_release{};
        clingo_solve_handle_close(std::exchange(hnd_, nullptr));
    }
}

void register_solve(pybind11::module &m) {
    auto solve = m.def_submodule("solve", R"(
Functions and classes related to solving.

Examples
--------

The examples below show various ways to intercept models. The asynchronous
variants leave room for additional computation before calling blocking functions
`like SolveHandle.get` or `SolveHandle.model`.

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
    ...     for mdl in hnd:
    ...         print(mdl)
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
    ...     while mdl := hnd.model():
    ...         print(mdl)
    ...         hnd.resume()
    ...     print(hnd.get())
    ...
    b
    a
    SAT
)"_d);

    py::class_<SolveControl>(solve, "SolveControl", R"(A control object to add clauses while solving.)")
        .def("add_clause", &SolveControl::add_clause, py::arg("clause"), R"(
Add a clause that applies to the current solving step during the search.

Args:
  clause: The literals of the clause.
)"_d)
        .def("add_nogood", &SolveControl::add_nogood, py::arg("nogood"), R"(
Add a nogood that applies to the current solving step during the search.

Args:
  nogood: The literals of the nogood.
)"_d)
        .def_property_readonly("base", &SolveControl::base, R"(Get the atom/term bases of the program.)");

    py::enum_<clingo_model_type_e>(solve, "ModelType", R"(Enumeration of model types.)")
        .value("StableModel", clingo_model_type_stable_model, R"(The model captures a stable model.)")
        .value("CautiousConsequences", clingo_model_type_cautious_consequences,
               R"(The model stores the set of cautious consequences.)")
        .value("BraveConsequences", clingo_model_type_brave_consequences,
               R"(The model stores the set of brave consequences.)");

    py::class_<Model>(solve, "Model", R"(A view on the solver's current solution.)")
        .def("symbols", &Model::symbols, py::arg("shown") = false, py::arg("atoms") = false, py::arg("terms") = false,
             py::arg("theory") = false, R"(
Get the symbols in the model.

Args:
    shown: Include shown atoms and terms.
    atoms: Include all true atoms, including hidden ones.
    terms: Include shown terms.
    theory: Include terms added by external theories.

Returns:
    A sequence of symbols present in the model.
)"_d)
        .def("contains", &Model::contains, py::arg("atom"), R"(
Check if the model contains the given atom.

Args:
    atom: The atom to look up.
Returns:
    Whether the atom is contained.
)"_d)
        .def("is_true", &Model::is_true, py::arg("literal"), R"(
Check if the given program literal is true.

Args:
    literal: The given program literal.

Returns:
    Whether the given program literal is true.
)"_d)
        .def("is_consequence", &Model::is_consequence, py::arg("literal"), R"(
Check if the given program literal is a consequence.

The function returns `True`, `False`, or `None` if the literal is a
consequence, not a consequence, or it is not yet known whether it is a
consequence, respectively.

While enumerating cautious or brave consequences, there is partial information
about which literals are consequences. The current state of a literal can be
requested using this function. If this function is used during normal model
enumeration, it simply returns whether the literal is true or false in the
current model.

Args:
    literal: The given program literal.

Returns:
    Whether the given program literal is a consequence.
)"_d)
        .def("extend", &Model::extend, py::arg("symbols"), R"(
Extend a model with the given symbols.

This only has an effect if there is an underlying clingo application, which
will print the added symbols.

Args:
	symbols: The symbols to add to the model.
)")
        .def("__str__", &Model::str, "Get a string representation of the model.")
        .def_property_readonly("control", &Model::control, "Get the associated solve control object.")
        .def_property_readonly("type", &Model::type, "Get the type of a model.")
        .def_property_readonly("number", &Model::number, "Get the running number of a model.")
        .def_property_readonly("cost", &Model::cost, "Return a sequence of integer cost values of the model.")
        .def_property_readonly("priorities", &Model::priorities, "Get the associated priorities of the cost values.")
        .def_property_readonly("optimality_proven", &Model::optimality_proven,
                               "Whether the optimality of the model has been proven.")
        .def_property_readonly("thread_id", &Model::thread_id, "Get the thread/solver id the model was found in.");

    py::class_<SolveResult>(solve, "SolveResult", R"(A solve result captures information about a solve call.)")
        .def("__str__", &SolveResult::str, R"(Get a string representation of the solve result.)")
        .def_property_readonly("satisfiable", &SolveResult::satisfiable, R"(Whether at least one model was found.)")
        .def_property_readonly("unsatisfiable", &SolveResult::unsatisfiable, R"(Whether there was no model.)")
        .def_property_readonly("unknown", &SolveResult::unknown, R"(Whether the satisfiablity could be determined.)")
        .def_property_readonly("exhausted", &SolveResult::exhausted, R"(Whether all models have been enumerated.)")
        .def_property_readonly("interrupted", &SolveResult::interrupted, R"(Whether the search was interrupted.)");

    py::class_<SolveHandle>(solve, "SolveHandle", R"(
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

This is always the last function to be called on a handle to ensure that the
search is properly terminated. It might be preceded by a call to cancel to stop
the search.
)"_d)
        .def("core", &SolveHandle::core, R"(Get the subset of assumptions that made the problem unsatisfiable.)")
        .def("model", &SolveHandle::model, R"(Get the current model if there is any.)")
        .def("last", &SolveHandle::last, R"(
Get the last computed model, if any.

If the search is not completed yet or the problem is unsatisfiable, the
function returns `None`.
)"_d)
        .def("resume", &SolveHandle::resume, R"(
Discards the last model and starts searching for the next one.

If the search has been started asynchronously, this function starts the search
in the background.
)"_d)
        .def("wait", &SolveHandle::wait, py::arg("timeout") = std::nullopt, R"(
Wait for the solve call to finish or the next result with an optional timeout.

If a timeout is provided, the function blocks for the given duration or until a
result is ready. A positive timeout blocks for that amount of time. A negative
timeout blocks until a result is available, and a zero timeout allows polling
for a result.

Args:
    timeout: The maximum time to block in seconds.

Returns:
    Whether the solve call has finished or the next result is ready.
)"_d)
        .def("cancel", &SolveHandle::cancel, R"(
Cancel the running search.

See also: `clingo.control.Control.interrupt`
)"_d)
        .def(
            "__enter__", [&](SolveHandle *hnd) -> SolveHandle * { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](SolveHandle *hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd->close(); },
            "Stop the search closing the handle.")
        .def(
            "__iter__", [&](SolveHandle *hnd) { return pybind11::make_iterator(ModelIterator{hnd}, ModelIterator{}); },
            "Get an iterator over the models.");
}

} // namespace PyClingo
