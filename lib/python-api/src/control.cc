#include "control.hh"
#include "ast.hh"
#include "backend.hh"
#include "config.hh"
#include "core.hh"
#include "ground.hh"
#include "iterator.hh"
#include "propagate.hh"
#include "solve.hh"
#include "stats.hh"
#include "util.hh"

#include <clingo/profile.h>
#include <clingo/solve.h>

#include <pybind11/native_enum.h>

#include <span>
#include <utility>

namespace PyClingo {

auto ConstMap::contains(key_type name) const -> bool {
    bool found = false;
    handle_error(clingo_const_map_find(map_, name.data(), name.size(), nullptr, &found));
    return found;
}

auto ConstMap::get(key_type name, std::optional<Symbol> def) const -> std::optional<mapped_type> {
    clingo_symbol_t sym = 0;
    bool found = false;
    handle_error(clingo_const_map_find(map_, name.data(), name.size(), &sym, &found));
    if (found) {
        return Symbol{sym, true};
    }
    return def;
}

auto ConstMap::at(size_t index) const -> value_type {
    clingo_string_t name;
    clingo_symbol_t sym = 0;
    handle_error(clingo_const_map_at(map_, index, &name, &sym));
    return {{name.data, name.size}, Symbol{sym, true}};
}

auto ConstMap::size() const -> size_t {
    size_t size = 0;
    handle_error(clingo_const_map_size(map_, &size));
    return size;
}

Control::Control(Library &lib, std::span<std::string const> args) {
    keep_owner(py::cast(lib, py::return_value_policy::reference));
    auto cargs = transform(args, [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
    clingo_control_t *ctl = nullptr;
    handle_error(clingo_control_new(lib, cargs.data(), cargs.size(), &ctl));
    reset(ctl, false);
    register_();
}

Control::Control(py::object lib, clingo_control_t *ctl, Parent::borrow_t tag) : Parent{ctl, tag} {
    keep_owner(std::move(lib));
    register_();
}

Control::Control(py::object lib, clingo_control_t *ctl) : Parent{ctl} {
    keep_owner(std::move(lib));
    register_();
}

Control::~Control() {
    unregister_();
}

auto Control::acquire(py::object lib, clingo_control_t *ctl) -> py::object {
    return py::cast(std::unique_ptr<Control>(new Control{std::move(lib), ctl}));
}

void Control::invalidate() noexcept {
    unregister_();
    reset();
}

auto Control::borrow(py::object lib, clingo_control_t *ctl) -> py::object {
    return py::cast(std::unique_ptr<Control>(new Control{std::move(lib), ctl, Parent::borrow_t{}}));
}

auto Control::mode() -> clingo_mode_e {
    clingo_mode_t mode = 0;
    handle_error(clingo_control_mode(get(), &mode));
    return static_cast<clingo_mode_e>(mode);
}

void Control::join(AST::Program &prg) {
    handle_error(clingo_control_join(get(), prg));
}

void Control::parse_string(std::string_view str) {
    handle_error(clingo_control_parse_string(get(), str.data(), str.size()));
}

void Control::write_aspif(std::optional<std::string_view> path, bool symbols, bool append, std::optional<bool> preamble,
                          bool preprocess) {
    clingo_write_aspif_mode_t mode = 0;
    if (symbols) {
        mode |= clingo_write_aspif_mode_symbols;
    }
    if (append) {
        mode |= clingo_write_aspif_mode_append;
    }
    if (!preamble) {
        mode |= clingo_write_aspif_mode_preamble_auto;
    } else if (*preamble) {
        mode |= clingo_write_aspif_mode_preamble;
    }
    if (preprocess) {
        mode |= clingo_write_aspif_mode_preprocess;
    }
    if (path && path->empty()) {
        throw std::invalid_argument{"path cannot be empty"};
    }
    auto path_view = path ? *path : std::string_view{};
    handle_error(clingo_control_write_aspif(get(), path_view.data(), path_view.size(), mode));
}

void Control::parse_files(std::span<std::string const> files) {
    auto cfiles = transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
    handle_error(clingo_control_parse_files(get(), cfiles.data(), cfiles.size()));
}

void Control::ground(std::optional<PartSpan> parts, py::handle ctx) {
    if (!parts) {
        parts = this->parts();
    }
    if (!parts) {
        static constexpr auto part = clingo_part_t{"base", 4, nullptr, 0};
        parts.emplace(&part, 1);
    }
    auto type = steal(py_ground_handle_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    if (!py_ground(type.get(), get(), parts->data(), parts->size(), ctx.ptr())) {
        throw py::error_already_set();
    }
}

auto Control::start_ground(std::optional<PartSpan> parts, py::handle ctx,
                           TypeHint<"collections.abc.Callable[[clingo.ground.GroundResult], None] | None"> on_finish)
    -> TypeHint<"clingo.ground.GroundHandle"> {
    if (!parts) {
        parts = this->parts();
    }
    if (!parts) {
        static constexpr auto part = clingo_part_t{"base", 4, nullptr, 0};
        parts.emplace(&part, 1);
    }
    auto type = steal(py_ground_handle_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto owner = py::cast(this);
    auto *res =
        py_ground_start(type.get(), get(), owner.ptr(), parts->data(), parts->size(), ctx.ptr(), on_finish.ptr());
    if (res == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<py::object>(res);
}

auto Control::base() -> TypeHint<"clingo.base.Base"> {
    clingo_base_t const *base = nullptr;
    if (!clingo_control_base(get(), &base)) {
        raise_error();
    }
    auto type = steal(py_base_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto pybase = steal(py_base_new(py::cast(this).ptr(), pointer_slot(ptr_ref()), type.get(), base));
    if (pybase.get() == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<TypeHint<"clingo.base.Base">>(pybase.release());
}

void Control::observe(TypeHint<"clingo.backend.Observer"> &obs, bool preprocess) {
    auto module = py::module::import("clingo.backend");
    if (!py_observe(module.ptr(), get(), obs.ptr(), preprocess)) {
        throw py::error_already_set();
    }
}

auto Control::backend() -> TypeHint<"clingo.backend.BackendManager"> {
    auto type = steal(py_backend_manager_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto res = steal(py_backend_manager_new(type.get(), get(), py::cast(this).ptr()));
    if (res.get() == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<TypeHint<"clingo.backend.BackendManager">>(res.release());
}

auto Control::config() -> TypeHint<"clingo.config.Config"> {
    clingo_config_t *config = nullptr;
    handle_error(clingo_control_config(get(), &config));
    clingo_id_t key = 0;
    handle_error(clingo_config_root(config, &key));
    auto type = steal(py_config_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto cfg =
        steal(py_config_new(py::cast(this).ptr(), pointer_slot(ptr_ref()), keeper_ref(), type.get(), config, key));
    if (cfg.get() == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<TypeHint<"clingo.config.Config">>(cfg.release());
}

auto Control::stats() -> StatsView {
    clingo_stats_t const *stats = nullptr;
    handle_error(clingo_control_stats(get(), &stats));
    uint64_t key = 0;
    handle_error(clingo_stats_root(stats, &key));
    auto type = steal(py_stats_view_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }

    auto res = steal(py_stats_new(py::cast(this).ptr(), pointer_slot(ptr_ref()), type.get(),
                                  const_cast<clingo_stats_t *>(stats), key));
    if (res.get() == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<StatsView>(res.release());
}

auto Control::profile() -> py::list {
    struct Data {
        Data() = default;
        auto add(size_t depth, char const *type) -> py::dict {
            auto node = py::dict();
            node["type"] = type;
            if (depth < stack.size()) {
                stack.resize(depth);
            }
            if (depth == 0) {
                res.append(node);
            } else {
                stack.back()["children"].attr("append")(node);
            }
            stack.emplace_back(node);
            return node;
        }
        py::list res;
        std::vector<py::dict> stack;
    } data;
    auto visitor = clingo_profile_visitor_t{
        [](size_t depth, char const *key, size_t key_size, bool nested, void *data) -> bool {
            CLINGO_TRY {
                auto node = static_cast<Data *>(data)->add(depth, "internal");
                node["key"] = py::str(key, key_size);
                node["nested"] = nested;
                node["children"] = py::list();
            }
            CLINGO_CATCH;
        },
        [](size_t depth, clingo_profile_data_t *values, clingo_profile_type_t type, void *data) -> bool {
            CLINGO_TRY {
                auto node = static_cast<Data *>(data)->add(depth, "leaf");
                node["profile_type"] = type == clingo_profile_type_step ? "step" : "accu";
                node["matches"] = values->matches;
                node["instances"] = values->instances;
                node["time_instantiate"] = values->time_instantiate;
                node["time_propagate"] = values->time_propagate;
            }
            CLINGO_CATCH;
        },
    };
    handle_error(clingo_control_profile(get(), &visitor, &data));
    return std::move(data.res);
}

auto Control::solve(MixedLitSpan const &assumptions, ModelCallback on_model, UnsatCallback on_unsat,
                    StatsCallback on_stats, FinishCallback on_finish) -> SolveResult {
    auto type = steal(py_solve_handle_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto ass = convert(base(), assumptions, false, false);
    auto owner = py::cast(this);
    auto *res = py_solve(type.get(), get(), owner.ptr(), std::span<clingo_literal_t const>{ass.data(), ass.size()},
                         on_model.ptr(), on_unsat.ptr(), on_stats.ptr(), on_finish.ptr());
    if (res == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<TypeHint<"clingo.solve.SolveResult">>(res);
}

auto Control::start_solve(MixedLitSpan const &assumptions, ModelCallback on_model, UnsatCallback on_unsat,
                          StatsCallback on_stats, FinishCallback on_finish, bool yield, bool async)
    -> Annotation<SolveHandle> {
    auto type = steal(py_solve_handle_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    auto ass = convert(base(), assumptions, false, false);
    auto owner = py::cast(this);
    auto *res =
        py_solve_start(type.get(), get(), owner.ptr(), std::span<clingo_literal_t const>{ass.data(), ass.size()},
                       on_model.ptr(), on_unsat.ptr(), on_stats.ptr(), on_finish.ptr(), yield, async);
    if (res == nullptr) {
        throw py::error_already_set();
    }
    return py::reinterpret_steal<TypeHint<"clingo.solve.SolveHandle">>(res);
}

void Control::main() {
    auto release = py::gil_scoped_release{};
    handle_error(clingo_control_main(get()));
}

void Control::interrupt() {
    clingo_control_interrupt(get());
}

void Control::discard(bool minimize, bool project) {
    clingo_discard_type_t type = 0;
    if (minimize) {
        type |= clingo_discard_type_e::minimize;
    }
    if (project) {
        type |= clingo_discard_type_e::project;
    }
    handle_error(clingo_control_discard(get(), type));
}

auto Control::buffer() -> std::string_view {
    clingo_string_t res;
    handle_error(clingo_control_buffer(get(), &res));
    return {res.data, res.size};
}

auto Control::const_map() -> HintConstMap {
    clingo_const_map_t const *map = nullptr;
    handle_error(clingo_control_const_map(get(), &map));
    return py::cast(ConstMap{map});
}

auto Control::parts() -> std::optional<PartSpan> {
    clingo_part_t const *parts = nullptr;
    size_t size = 0;
    bool has_value = false;
    handle_error(clingo_control_get_parts(get(), &parts, &size, &has_value));
    if (!has_value) {
        return std::nullopt;
    }
    return PartSpan{parts, size};
}

void Control::set_parts(std::optional<PartSpan> parts) {
    if (parts) {
        handle_error(clingo_control_set_parts(get(), parts->data(), parts->size(), true));
    } else {
        handle_error(clingo_control_set_parts(get(), nullptr, 0, true));
    }
}

void Control::register_propagator(TypeHint<"clingo.propagate.Propagator"> propagator) {
    auto type = steal(py_propagator_type());
    if (type.get() == nullptr) {
        throw py::error_already_set();
    }
    if (!py_register_propagator(type.get(), get(), keeper_ref(), owner_ref(), propagator.ptr())) {
        throw py::error_already_set();
    }
}

void Control::release(clingo_control_t *ctl) noexcept {
    if (ctl != nullptr) {
        clingo_control_release(ctl);
        ctl = nullptr;
    }
}

void Control::acquire(clingo_control_t *ctl, bool inc) {
    if (ctl != nullptr && inc) {
        clingo_control_acquire(ctl);
    }
}

void register_control(pybind11::module &m) {
    auto control = m.def_submodule("control", R"(
Module containing the Control class responsible for grounding and solving.

# Example

The example shows the most straightforward way to ground and solve a
small test program:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> print(ctl.solve(on_model=print))
a
SAT
```
)"_d);

    make_mapping(py::class_<ConstMap>(control, "_ConstMap", R"(The map from constants defined by #const directives.)"));

    py::native_enum<clingo_mode_e>(control, "ControlMode", "enum.IntEnum", "Available control modes.")
        .value("Parse", clingo_mode_parse, R"(Parse only.)")
        .value("Rewrite", clingo_mode_rewrite, R"(Parse and rewrite.)")
        .value("Ground", clingo_mode_ground, R"(Parse, rewrite, and ground.)")
        .value("Solve", clingo_mode_solve, R"(Parse, rewrite, ground, and solve.)")
        .finalize();

    py::class_<Control>(control, "Control", py::custom_type_setup(&Control::setup),
                        R"(A control object for grounding and solving.)")
        .def(py::init<Library &, std::span<std::string> const &>(), py::arg("lib"),
             py::arg("options") = std::span<std::string>{}, R"(
Construct a control object.

Args:
  lib: The library storing symbols and scripts.
  options: The command line options to initialize the control object.
)"_d)
        .def("register_propagator", &Control::register_propagator, py::arg("propagator"), R"(
Register the given propagator for theory propagation.

See the `clingo.propagate` module for an example.

Args:
    propagator:
        The propagator.
)"_d)
        .def("join", &Control::join, py::arg("program"), R"(
Join with the given non-ground logic program.

Args:
    program:
        A non-ground logic program.
)"_d)
        .def("parse_string", &Control::parse_string, py::arg("program"), R"(
Parses a logic program given as a string.

Args:
    program:
        The logic program as a string.
)"_d)
        .def("parse_files", &Control::parse_files, py::arg("files"), R"(
Parses the logic programs in the given files

Args:
    files:
        A list of file paths to parse.
)"_d)
        .def("write_aspif", &Control::write_aspif, py::arg("path") = std::nullopt, py::arg("symbols") = false,
             py::arg("append") = false, py::arg("preamble") = std::nullopt, py::arg("preprocess") = true, R"(
Write the current logic programs to the given file.

If append is true, a file will be created if none exists yet. If preamble is
None, then the aspif preamble is written for newly created files and omitted
for existing files.

If `path` is None, clingo's internal output buffer is used instead of writing
to a file. The buffer content is available via `clingo.control.Control.buffer`.
If `preamble` is None, the preamble is written only if the buffer is currently
empty. In application mode, the buffer is printed to stdout and cleared after
each major operation like grounding, so the preamble flag should be set
explicitly in this
case.

Args:
    path:
        The optional path to write the program to.
    append:
        Whether to append to an existing file.
    preamble:
        Whether to write the aspif preamble.
    preprocess:
        Whether to preprocess the program before writing.
)"_d)
        .def("ground", &Control::ground, py::arg("parts") = std::nullopt, py::arg("context") = py::none(), R"(
Ground the given program parts.

Non-ground logic programs must be added before calling this function.  Programs
can define named sections using `#program.` directives.  These sections can be
selectively grounded by specifying their name and binding parameters to
symbols.

Args:
    parts:
        A sequence of tuples, each containing a section name and a sequence of
        symbols. The name identifies the program section to ground, and the
        symbols bind its parameters.  If `None`, the implicit base section
        without arguments is grounded.
    context:
        An optional object providing functions that can be called during
        grounding.
)"_d)
        .def("start_ground", &Control::start_ground, py::arg("parts") = std::nullopt, py::arg("context") = py::none(),
             py::arg("on_finish") = py::none(), R"(
Ground the given program parts.

Starts grounding in the background and returns a `clingo.ground.GroundHandle`
to the running grounding. See `clingo.control.Control.ground` for details on
grounding program parts.

Args:
    parts:
        A sequence of parts to ground.
    context:
        An optional object providing functions that can be called during
        grounding.
    on_finish:
        An optional callback called once grounding has finished.
)"_d)
        .def("solve", &Control::solve, py::arg("assumptions") = MixedLitSpan{}, py::arg("on_model") = std::nullopt,
             py::arg("on_unsat") = std::nullopt, py::arg("on_stats") = std::nullopt,
             py::arg("on_finish") = std::nullopt, R"(
Solve the current ground program.

This function is semantically equivalent to the following `start_solve` call:

```python
with self.start_solve(assumptions, on_model, on_unsat, on_stats, on_finish) as hnd:
    return hnd.get()
```

Args:
    assumptions:
        A list of assumptions.
    on_model:
        Optional callback to intercept models.
    on_unsat:
        Optional callback to intercept lower bounds during optimization.
    on_stats:
        Optional callback extend statistics.
    on_finish:
        Optional callback called once search has finished.
Returns:
    A `clingo.solve.SolveResult` representing the result of the search.
)"_d)
        .def("start_solve", &Control::start_solve, py::arg("assumptions") = MixedLitSpan{},
             py::arg("on_model") = std::nullopt, py::arg("on_unsat") = std::nullopt, py::arg("on_stats") = std::nullopt,
             py::arg("on_finish") = std::nullopt, py::arg("yield_") = false, py::arg("async_") = false, R"(
Solve the current ground program.

This function runs the solver on the current ground program, optionally  using
assumptions, callbacks, or asynchronous execution. It returns a  `SolveHandle`,
allowing interaction with the solving process.

If asynchronous solving (`async_`) is enabled, the function returns
immediately, and solving runs in the background. Otherwise, the function
blocks until solving is complete.

Args:
    assumptions:
        A list of assumptions that constrain this search. Each assumption is
        either a `tuple[clingo.symbol.Symbol, bool]` indicating an atom's truth
        value or a program literal (see `clingo.base.Atom.literal`). For
        example, using `[(clingo.symbol.Function(lib, "a"), True)]` only admits
        answer sets that contain atom `a`.
    on_model:
        Optional callback that receives a `clingo.solve.Model` object when
        a model is found. Returning `False` from the callback stops solving.
    on_unsat:
        Optional callback to intercept lower bounds during optimization.
    on_stats:
        Optional callback that receives statistics updates after each step.
        Two `clingo.stats.Stats` objects are passed: step-specific and
        accumulated stats.
    on_finish:
        Optional callback called once search has finished. A
        `clingo.solve.SolveResult` is passed to the callback.
    yield_:
        If `True`, the returned `clingo.solve.SolveHandle` is iterable,
        yielding  `clingo.solve.Model` objects during solving.
    async_:
        If `True`, solving runs asynchronously in a separate thread.
        Note: Callbacks (`on_model`, `on_stats`, etc.) will also be called
        from a separate thread.
Returns:
    A `clingo.solve.SolveHandle` to control the search.

Notes:
    Asynchronous solving requires compiling clingo with thread support.
    Blocking methods on `SolveHandle` release the GIL but are not thread-safe.

See Also:
    `clingo.solve`
)"_d)
        .def("main", &Control::main, R"(
Ground and solve a logic program based on the current control mode.

This function serves as a high-level entry point for the default
ground-and-solve process. It considers the current `ControlMode` and can
dispatch execution to a script's `main` function if defined.

If solving, the function proceeds as follows:

1. Each set of program parts in `parts` is grounded sequentially.
2. After grounding each set, solving is performed immediately.

Before calling `main()`, the control object can be prepared by parsing
programs, registering propagators, or performing other setup steps.
)"_d)
        .def("interrupt", &Control::interrupt, R"(
Interrupt the active solve call.

This function is thread-safe. Prefer using `clingo.solve.SolveHandle.cancel` if
possible.
)"_d)
        .def("observe", &Control::observe, py::arg("observer"), py::arg("preprocess") = true, R"(
Inspect the ground program of the current step.

Args:
    observer: The program observer to inspect the program.
    preprocess:
        Whether the program should be preprocessed first (default: True).
)"_d)
        .def("discard", &Control::discard, py::arg("minimize") = false, py::arg("project") = false, R"(
Discard statements of the selected types.

Args:
    minimize: Discard all minimize and weak constraints (default: False).
    project: Discard all previously added project  statements (default: False).
)"_d)
        .def_property_readonly("buffer", &Control::buffer, R"(The content of the output buffer.)")
        .def_property_readonly("base", &Control::base, R"(Get the atom/term bases of the program.)")
        .def_property_readonly("backend", &Control::backend, R"(Get a backend manager to extend the ground program.)")
        .def_property_readonly("config", &Control::config, R"(Get the solver config.)")
        .def_property_readonly("stats", &Control::stats, R"(Get the solver stats.)")
        .def_property_readonly("mode", &Control::mode, R"(Get the application mode.)")
        .def_property_readonly("profile", &Control::profile, R"(
Get the profiling information as a list of profile nodes.

Each node is a dictionary with keys such as "type", "key", "depth", "nested",
"children", etc. Returns a list of top-level profile nodes representing the
profiling tree.

The result is directly convertible to JSON using Python's `json` module.
)"_d)
        .def_property_readonly("const_map", &Control::const_map,
                               R"(Get the map of constants defined by `#const` directives.)")
        .def_property("parts", &Control::parts, &Control::set_parts, R"(Get/set the program parts to ground.)");
}

auto py_control_type() -> PyObject * {
    try {
        return py::type::of<Control>().release().ptr();
    } catch (py::error_already_set &e) {
        e.restore();
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }
    return nullptr;
}

namespace {

//! The tag under which owned control objects are registered with their clingo control objects.
//!
//! NOTE: should be moved into the module state once the module is ported.
char const control_owner_tag = 0;

//! Invalidate a non-owned control object and release the reference held by its clingo control object.
void control_finalizer(void *data) {
    // NOTE: objects cannot be released anymore once the interpreter is gone
    if (Py_IsInitialized() == 0) {
        return;
    }
    auto gil = GILGuard{};
    auto *obj = static_cast<PyObject *>(data);
    try {
        py::cast<Control &>(py::handle{obj}).invalidate();
    } catch (py::error_already_set &e) {
        e.discard_as_unraisable("control finalizer");
    } catch (std::exception const &) {
        // NOTE: only control objects created by py_control_begin are passed here
        assert(false);
    }
    Py_DECREF(obj);
}

} // namespace

void Control::register_() {
    handle_error(clingo_control_set_owner(get(), &control_owner_tag, this));
}

void Control::unregister_() noexcept {
    if (auto *ctl = get_unchecked(); ctl != nullptr && owner(ctl) == this) {
        // NOTE: removing an owner does not allocate and cannot fail
        std::ignore = clingo_control_set_owner(ctl, &control_owner_tag, nullptr);
    }
}

auto Control::owner(clingo_control_t *ctl) -> Control * {
    return static_cast<Control *>(clingo_control_get_owner(ctl, &control_owner_tag));
}

void Control::setup(PyHeapTypeObject *heap_type) {
    reference_keeper<Control>::setup(heap_type);
    auto *type = &heap_type->ht_type;
    type->tp_clear = [](PyObject *self_base) -> int {
        auto *self = py::cast<Control *>(py::handle(self_base));
        if (!constructed_.contains(self)) {
            return 0;
        }
        // NOTE: releasing the clingo control object first runs its finalizers
        // and ensures that registered objects outlive it
        if (self->owns()) {
            self->invalidate();
        }
        Py_CLEAR(self->list_.ptr());
        Py_CLEAR(self->owner_.ptr());
        return 0;
    };
}

auto py_control_begin(PyObject *type, PyObject *library, clingo_control_t *ctl, ControlRef ref_type) -> PyObject * {
    try {
        auto lib = py::reinterpret_borrow<py::object>(library);
        auto *owner = Control::owner(ctl);
        auto obj = py::object{};
        if (owner != nullptr) {
            // an existing owner is reused and stays valid after the end call
            if (ref_type == ControlRef::claim) {
                throw std::runtime_error{"cannot claim a control object that already has an owner"};
            }
            obj = py::cast(owner, py::return_value_policy::reference);
        } else if (ref_type == ControlRef::claim) {
            // the claimed control is owned until the end call
            obj = Control::acquire(lib, ctl);
        } else {
            // a non-owned control object registered as owner
            obj = Control::borrow(lib, ctl);
        }
        if (PyObject_TypeCheck(obj.ptr(), reinterpret_cast<PyTypeObject *>(type)) == 0) {
            PyErr_SetString(PyExc_TypeError, "type is not clingo.control.Control");
            return nullptr;
        }
        if (owner == nullptr && ref_type == ControlRef::share) {
            // the clingo control object keeps the non-owned object alive; if
            // this fails, destroying the object removes its registration
            handle_error(clingo_control_add_finalizer(ctl, control_finalizer, obj.ptr()));
            Py_INCREF(obj.ptr());
        }
        return obj.release().ptr();
    } catch (py::error_already_set &e) {
        e.restore();
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    }
    return nullptr;
}

void py_control_end(PyObject *obj, ControlRef ref_type) noexcept {
    auto ref = steal(obj);
    if (ref_type == ControlRef::claim) {
        // the lifetime of the claimed control object ends here
        auto *ctl = py_cast<Control>(obj);
        assert(ctl != nullptr);
        ctl->invalidate();
    }
}

} // namespace PyClingo
