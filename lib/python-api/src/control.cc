#include "control.hh"
#include "core.hh"
#include "util.hh"

#include <clingo/solve.h>

#include <span>
#include <utility>

namespace Clingo::Python {

Control::Control(Library &lib, std::vector<std::string> const &args) {
    auto c_args = transform(args, [](auto const &str) { return str.c_str(); });
    auto *ctl = static_cast<clingo_control_t *>(nullptr);
    handle_error(clingo_control_new(lib, c_args.data(), c_args.size(), &ctl));
    ctl_.reset(ctl);
}

void Control::join(Program &prg) { handle_error(clingo_control_join(ctl_.get(), prg)); }

void Control::parse_string(char const *str) { handle_error(clingo_control_parse_string(ctl_.get(), str)); }

auto Control::ctx_(clingo_lib_t *lib, clingo_location_t const *location, char const *name,
                   clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                   clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> clingo_result_t {
    // TODO: handle location!!!
    static_cast<void>(location);
    CLINGO_TRY {
        auto *handle = static_cast<py::handle *>(data);
        py::list args;
        for (auto sym : std::span{arguments, arguments_size}) {
            args.append(Symbol{sym, true});
        }
        auto syms = handle->attr(name)(*args).cast<std::variant<SymbolVec, Symbol>>();
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
    CLINGO_CATCH(lib);
}

void Control::ground(std::optional<std::vector<std::pair<std::string, SymbolVec>>> const &parts, py::handle ctx) {
    static auto const base = std::vector{std::pair{std::string{"base"}, SymbolVec{}}};
    auto c_args = transform(parts ? *parts : base, [](auto const &part) {
        return clingo_part_t{part.first.c_str(),
                             // NOLINTNEXTLINE
                             reinterpret_cast<clingo_symbol_t const *>(part.second.data()), part.second.size()};
    });
    handle_error(clingo_control_ground(ctl_.get(), c_args.data(), c_args.size(),
                                       !ctx.is_none() ? &Control::ctx_ : nullptr, &ctx));
}

auto SolveHandle::c_event_handler(clingo_solve_event_type_t type, void *event, void *data, bool *goon)
    -> clingo_result_t {
    auto *eh = static_cast<SolveHandle *>(data);
    CLINGO_TRY {
        if (eh->mdl_ && type == clingo_solve_event_type_model) {
            auto mdl = Model{static_cast<clingo_model_t *>(event)};
            auto ret = (*eh->mdl_)(mdl);
            *goon = ret ? *ret : true;
        }
    }
    CLINGO_CATCH(eh->ptr_);
}

auto Control::base() -> Base {
    auto base = clingo_base_t{};
    clingo_control_base(ctl_.get(), &base);
    return {base};
}

auto Control::solve(AssumptionVec const &assumptions, std::optional<ModelCallback> on_model, bool yield, bool async)
    -> SSolveHandle {
    auto res = std::make_shared<SolveHandle>(std::move(on_model));
    auto mode = clingo_solve_mode_bitset_t{0};
    if (yield) {
        mode |= clingo_solve_mode_yield;
    }
    if (async) {
        mode |= clingo_solve_mode_async;
    }
    auto b = base();
    LitVec ass;
    ass.reserve(assumptions.size());
    std::ranges::transform(assumptions, std::back_inserter(ass), [&](auto const &x) {
        return std::visit(
            [&]<typename T>(T const &x) {
                if constexpr (std::is_same_v<T, Lit_t>) {
                    ass.emplace_back(x);
                } else {
                    auto const &[sym, sign] = x;
                    ass.emplace_back(b.lookup(sym.signature().value()).lookup(sym).literal());
                }
                return 0;
            },
            x);
    });
    handle_error(clingo_control_solve(ctl_.get(), mode, ass.data(), assumptions.size(), &SolveHandle::c_event_handler,
                                      res.get(), &res->handle()),
                 res->exception_ptr());
    return res;
}

void Control::main() { handle_error(clingo_control_main(ctl_.get())); }

auto Control::buffer() -> char const * {
    char const *ret = nullptr;
    handle_error(clingo_control_buffer(ctl_.get(), &ret));
    return ret;
}

void register_control(pybind11::module &m) {
    auto control = m.def_submodule("control", R"(
Module containing the Control class responsible for grounding and solving.

```python
>>> from clingo.core import Library
>>> from clingo.control import Control
>>>
>>> lib = Library()
>>> ctl = Control(lib, [])
>>> ctl.parse_string("1 { a; b }.")
>>> ctl.ground()
>>> with ctl.solve(on_model=print) as hnd:
...     hnd.get()
a
```
)"_d);
    py::class_<Control>(control, "Control", R"(A control object for grounding and solving.)")
        .def(py::init<Library &, std::vector<std::string> const &>(), py::arg("lib"),
             py::arg("options") = std::vector<std::string>{}, R"(
Construct a control object.

Args:
  lib: The library storing symbols and scripts.
  options: The command line options to initialize the control object.
)"_d)
        .def("join", &Control::join, py::arg("program"), R"(
Join with the given non-ground logic program.

Args:
  program: A non-ground logic program.
)"_d)
        .def("parse_string", &Control::parse_string, py::arg("program"), R"(
Parses a logic program given as a string.

Args:
  program: The logic program as string.
)"_d)
        .def("ground", &Control::ground, py::arg("parts") = std::nullopt, py::arg("context") = py::none(), R"(
Ground the given program parts.

Args:
  parts: A list of tuples of part names and their symbolic arguments.
  context: An optional object with functions to call during grounding.
)"_d)
        .def("solve", &Control::solve, py::arg("assumptions") = Control::AssumptionVec{},
             py::arg("on_model") = std::nullopt, py::arg("yield_") = false, py::arg("async_") = false, R"(
Solve the current ground program.

Args:
    assumptions:
        List of `tuple[clingo.symbol.Symbol, bool]` or program literals (see
        `clingo.base.Atom.literal`) that serve as assumptions for the solve
        call, e.g., solving under assumptions
        `[(clingo.symbol.Function(lib, "a"), True)]` only admits answer sets
        that contain atom `a`.
    on_model:
        Optional callback for intercepting models. A `clingo.solving.Model`
        object is passed to the callback. The search can be interruped from the
        model callback by returning `False`.
    on_unsat:
        Optional callback to intercept lower bounds during optimization.
    on_statistics:
        Optional callback to update statistics.
        The step and accumulated statistics are passed as arguments.
    on_finish:
        Optional callback called once search has finished. A
        `clingo.solving.SolveResult` is passed to the callback.
    on_core:
        Optional callback called with the assumptions that made a problem
        unsatisfiable.
    yield_:
        The resulting `clingo.solving.SolveHandle` is iterable yielding
        `clingo.solving.Model` objects.
    async_:
        The solve call and the method `clingo.solving.SolveHandle.resume`
        of the returned handle are non-blocking.

Returns:
    A solve handle to control the search.

Note:
    If clingo is not in solving mode, this function just returns a
    `clingo.solving.SolveResult` where `clingo.solving.SolveResult.unknown` is
    true.

    If this function is used in embedded Python code, you might want to start
    clingo using the `--outf=3` option to disable all output from clingo.

    Asynchronous solving is only available if thread support was enabled.
    Furthermore, the `on_model` and `on_finish` callbacks are called from
    another thread. To ensure that the methods can be called, make sure to not
    use any functions that block Python's GIL indefinitely.

    This function as well as blocking functions on the
    `clingo.solving.SolveHandle` release the GIL but are not thread-safe.

See Also:
    clingo.solving: For more examples how to use this method.
)"_d)
        .def("main", &Control::main, R"(
Ground and solver a logic program.

This function proceeds as clingo calling the main function from a script if
there is any.
)"_d)
        .def_property_readonly("buffer", &Control::buffer, R"(The content of the output bufer.)")
        .def_property_readonly("base", &Control::base, R"(Get the atom/term bases of the program.)");
}

} // namespace Clingo::Python
