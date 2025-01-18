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
            *goon = (*eh->mdl_)(mdl);
        }
    }
    CLINGO_CATCH(eh->ptr_);
}

auto Control::solve(std::optional<ModelCallback> on_model) -> SSolveHandle {
    auto res = std::make_shared<SolveHandle>(std::move(on_model));
    handle_error(
        clingo_control_solve(ctl_.get(), 0, nullptr, 0, &SolveHandle::c_event_handler, res.get(), &res->handle()),
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
    register_solving(m);
    auto control = m.def_submodule("control", doc(R"(
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
)"));
    py::class_<Control>(control, "Control", R"(A control object for grounding and solving.)")
        .def(py::init<Library &, std::vector<std::string> const &>(), py::arg("lib"),
             py::arg("options") = std::vector<std::string>{}, doc(R"(
Construct a control object.

Args:
  lib: The library storing symbols and scripts.
  options: The command line options to initialize the control object.
)"))
        .def("join", &Control::join, py::arg("program"), doc(R"(
Join with the given non-ground logic program.

Args:
  program: A non-ground logic program.
)"))
        .def("parse_string", &Control::parse_string, py::arg("program"), doc(R"(
Parses a logic program given as a string.

Args:
  program: The logic program as string.
)"))
        .def("ground", &Control::ground, py::arg("parts") = std::nullopt, py::arg("context") = py::none(), doc(R"(
Ground the given program parts.

Args:
  parts: A list of tuples of part names and their symbolic arguments.
  context: An optional object with functions to call during grounding.
)"))
        .def("solve", &Control::solve, py::arg("on_model") = std::nullopt, doc(R"(
Solve the current ground program.
)"))
        .def("main", &Control::main, doc(R"(
Ground and solver a logic program.

This function proceeds as clingo calling the main function from a script if
there is any.
)"))
        .def_property_readonly("buffer", &Control::buffer, R"(The content of the output bufer.)");
}

} // namespace Clingo::Python
