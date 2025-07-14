#include "control.hh"
#include "ast.hh"
#include "core.hh"
#include "stats.hh"
#include "util.hh"

#include <clingo/profile.h>
#include <clingo/solve.h>

#include <span>

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
    auto cargs = transform(args, [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
    clingo_control_t *ctl = nullptr;
    handle_error(clingo_control_new(lib, cargs.data(), cargs.size(), &ctl));
    reset(ctl, false);
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

void Control::write_aspif(std::string_view path, bool symbols, bool append, std::optional<bool> preamble,
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
    handle_error(clingo_control_write_aspif(get(), path.data(), path.size(), mode));
}

void Control::parse_files(std::span<std::string const> files) {
    auto cfiles = transform(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
    handle_error(clingo_control_parse_files(get(), cfiles.data(), cfiles.size()));
}

auto Control::ctx_([[maybe_unused]] clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *location,
                   char const *name, size_t name_size, clingo_symbol_t const *arguments, size_t arguments_size,
                   void *data, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> bool {
    auto &handle = *static_cast<py::handle *>(data);
    CLINGO_TRY {
        auto syms = [&] {
            auto acquire = py::gil_scoped_acquire{};
            py::list args;
            for (auto sym : std::span{arguments, arguments_size}) {
                args.append(Symbol{sym, true});
            }
            return handle.attr(std::string{name, name_size}.c_str())(*args).cast<std::variant<SymbolVec, Symbol>>();
        }();
        return std::visit(
            [&]<class T>(T const &res) {
                if constexpr (std::is_same_v<T, Symbol>) {
                    // NOLINTNEXTLINE
                    auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(&res);
                    return symbol_callback(c_syms, 1, symbol_callback_data);
                } else {
                    // NOLINTNEXTLINE
                    auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(res.data());
                    return symbol_callback(c_syms, res.size(), symbol_callback_data);
                }
            },
            syms);
    }
    CLINGO_CATCH;
}

void Control::ground(std::optional<PartSpan> parts, py::handle ctx) {
    auto release = py::gil_scoped_release{};
    if (!parts) {
        parts = this->parts();
    }
    if (!parts) {
        static constexpr auto part = clingo_part_t{"base", 4, nullptr, 0};
        parts.emplace(&part, 1);
    }
    handle_error(
        clingo_control_ground(get(), parts->data(), parts->size(), !ctx.is_none() ? &Control::ctx_ : nullptr, &ctx));
}

auto Control::base() -> Base {
    clingo_base_t const *base = nullptr;
    clingo_control_base(get(), &base);
    return {base};
}

void Control::observe(Observer &obs, bool preprocess) {
    obs.observe(get(), preprocess);
}

auto Control::backend() -> BackendManager {
    return BackendManager{get()};
}

auto Control::config() -> Config {
    clingo_config_t *config = nullptr;
    handle_error(clingo_control_config(get(), &config));
    clingo_id_t key = 0;
    handle_error(clingo_config_root(config, &key));
    return Config{config, key};
}

auto Control::stats() -> py::dict {
    clingo_stats_t const *stats = nullptr;
    handle_error(clingo_control_stats(get(), &stats));
    uint64_t key = 0;
    handle_error(clingo_stats_root(stats, &key));
    // NOLINTNEXTLINE
    return Stats{const_cast<clingo_stats_t *>(stats), key}.nestify();
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
        }};
    handle_error(clingo_control_profile(get(), &visitor, &data));
    return std::move(data.res);
}

auto Control::solve(MixedLitSpan const &assumptions, Annotation<std::optional<ModelCallback>> on_model,
                    Annotation<std::optional<UnsatCallback>> on_unsat,
                    Annotation<std::optional<StatsCallback>> on_stats,
                    Annotation<std::optional<FinishCallback>> on_finish, bool yield, bool async)
    -> Annotation<SolveHandle> {
    auto res = py::cast(std::make_unique<SolveHandle>());
    auto *hnd = res.cast<SolveHandle *>();
    auto store = [&]<class T>(Annotation<std::optional<T>> &src, py::handle &dst) -> void {
        if (!src.is_none()) {
            // keep the callback alive
            hnd->tie(src);
            // store a reference to the callback
            dst = src;
        }
    };
    store(on_model, hnd->mdl_);
    store(on_unsat, hnd->unsat_);
    store(on_stats, hnd->stats_);
    store(on_finish, hnd->finish_);
    auto mode = clingo_solve_mode_bitset_t{0};
    if (yield) {
        mode |= clingo_solve_mode_yield;
    }
    if (async) {
        mode |= clingo_solve_mode_async;
    }
    auto c_event_handler = clingo_solve_event_handler_t{
        hnd->mdl_ ? +[](clingo_model_t *model, void *data, bool *goon) {
            CLINGO_TRY {
                auto guard = py::gil_scoped_acquire{};
                auto *hnd = static_cast<SolveHandle *>(data);
                auto mdl = Model{model};
                auto res = (*hnd->mdl_)(mdl).cast<std::optional<bool>>();
                *goon = !res || *res;
            }
            CLINGO_CATCH;
        } : nullptr,
        hnd->unsat_ ? +[](int64_t const *values, size_t size, void *data) -> bool {
            CLINGO_TRY {
                auto guard = py::gil_scoped_acquire{};
                auto *hnd = static_cast<SolveHandle *>(data);
                assert(hnd != nullptr);
                hnd->unsat_(std::span{values, size});
            }
            CLINGO_CATCH;

        } : nullptr,
        hnd->stats_ ? +[](clingo_stats_t *stats, void *data) -> bool {
            CLINGO_TRY {
                auto guard = py::gil_scoped_acquire{};
                auto *hnd = static_cast<SolveHandle *>(data);
                uint64_t root = 0;
                handle_error(clingo_stats_root(stats, &root));
                uint64_t step = 0;
                std::string_view user_step = "user_step";
                std::string_view user_accu = "user_accu";
                handle_error(clingo_stats_map_add_subkey(stats, root, user_step.data(), user_step.size(),
                                                         clingo_stats_type_map, &step));
                uint64_t accu = 0;
                handle_error(clingo_stats_map_add_subkey(stats, root, user_accu.data(), user_accu.size(),
                                                         clingo_stats_type_map, &accu));
                (*hnd->stats_)(Stats{stats, step}, Stats{stats, accu});
            }
            CLINGO_CATCH;
        } : nullptr,
        hnd->finish_ ? +[](clingo_solve_result_bitset_t result, void *data) -> void {
            try {
                auto guard = py::gil_scoped_acquire{};
                auto *hnd = static_cast<SolveHandle *>(data);
                assert(hnd != nullptr);
                hnd->finish_(static_cast<SolveResult>(result));
            }
            catch (std::exception &e) {
                printf("panic: %s\n", e.what());
                std::abort();
            }
        } : nullptr,
        nullptr,
    };
    auto ass = convert(base(), assumptions, false);
    {
        auto guard = py::gil_scoped_release{};
        auto has_handler = hnd->mdl_ || hnd->unsat_ || hnd->stats_ || hnd->finish_;
        handle_error(clingo_control_solve(get(), mode, ass.data(), assumptions.size(),
                                          has_handler ? &c_event_handler : nullptr, hnd, &hnd->handle()));
    }
    return res;
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

void Control::register_propagator(Annotation<Propagator> const &propagator) {
    auto &prop = propagator.cast<Propagator &>();
    tie(propagator);
    PyClingo::register_propagator(get(), prop);
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

auto Control::cast(clingo_control_t *ctl, bool convert) -> PyControl {
    auto *ptr = from_registry(ctl);
    if (ptr != nullptr) {
        return py::cast(ptr);
    }
    if (convert) {
        return py::cast(std::unique_ptr<Control>(new Control{ctl}));
    }
    throw py::cast_error("invalid Control cast");
}

void register_control(pybind11::module &m) {
    auto control = m.def_submodule("control", R"(
Module containing the Control class responsible for grounding and solving.

# Examples

The first example shows the most straightforward way to ground and solve a
small test program:

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib)
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
...     hnd.get()
a
```

The second example shows how to call functions from within a program:

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
)"_d);

    make_mapping(py::class_<ConstMap>(control, "_ConstMap", R"(The map from constants defined by #const directives.)"));

    py::enum_<clingo_mode_e>(control, "ControlMode", "Available control modes.")
        .value("Parse", clingo_mode_parse, R"(Parse only.)")
        .value("Rewrite", clingo_mode_rewrite, R"(Parse and rewrite.)")
        .value("Ground", clingo_mode_ground, R"(Parse, rewrite, and ground.)")
        .value("Solve", clingo_mode_solve, R"(Parse, rewrite, ground, and solve.)");

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
        .def("write_aspif", &Control::write_aspif, py::arg("path"), py::arg("symbols") = false,
             py::arg("append") = false, py::arg("preamble") = std::nullopt, py::arg("preprocess") = true, R"(
Write the current logic programs to the given file.

If append is true, a file will be created if none exists yet. If preamble is
None, then the aspif preamble is written for newly created files and omitted
for existing files.

Args:
    path:
        The path to write the program to.
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
        .def("solve", &Control::solve, py::arg("assumptions") = MixedLitSpan{}, py::arg("on_model") = std::nullopt,
             py::arg("on_unsat") = std::nullopt, py::arg("on_stats") = std::nullopt,
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
    clingo.solve: Contains examples on using this function.
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

} // namespace PyClingo
