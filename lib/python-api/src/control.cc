#include "control.hh"
#include "core.hh"
#include "util.hh"

#include <clingo/solve.h>

#include <span>
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

auto Model::symbols(bool shown, bool atoms, bool terms, bool theory, bool complement) -> SymbolVec {
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
    if (complement) {
        show |= clingo_show_type_complement;
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

void SolveHandle::close() {
    if (hnd_ != nullptr) {
        clingo_solve_handle_close(std::exchange(hnd_, nullptr));
    }
}

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

auto Control::solve(std::optional<ModelCallback> on_model) -> USolveHandle {
    auto res = std::make_unique<SolveHandle>(std::move(on_model));
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
    py::class_<SolveResult>(control, "SolveResult", R"(A solve result captures information about a solve call.)")
        .def("__str__", &SolveResult::str, R"(Get a string representation of the solve result.)")
        .def_property_readonly("satisfiable", &SolveResult::satisfiable, R"(True if there was at least one model.)")
        .def_property_readonly("unsatisfiable", &SolveResult::unsatisfiable, R"(True if there was no model.)")
        .def_property_readonly("exhausted", &SolveResult::exhausted, R"(True if all models have been enumerated.)")
        .def_property_readonly("interrupted", &SolveResult::interrupted, R"(True if the search was interrupted.)");

    py::class_<Model>(control, "Model", R"(A view on the solver's current solution.)")
        .def("symbols", &Model::symbols, py::arg("shown") = false, py::arg("atoms") = false, py::arg("terms") = false,
             py::arg("theory") = false, py::arg("complement") = false, doc(R"(
Get the symbols in the model.

Args:
    shown: Include shown atoms and terms.
    atoms: Include all true atoms including hidden ones.
    terms: Include shown terms.
    theory: Include terms added by external theories.
    complement:
		Has to be used in combination with shown/atoms/terms. Selects atoms
		that are false and terms with false conditions.
)"));

    py::class_<SolveHandle>(control, "SolveHandle", R"(An object to interact with a running search.)")
        .def("get", &SolveHandle::get, R"(Get the solve result.)")
        .def(
            "__enter__", [&](py::object hnd) -> py::object { return hnd; }, "Start the search.")
        .def(
            "__exit__",
            [&](SolveHandle &hnd, [[maybe_unused]] const std::optional<pybind11::type> &type,
                [[maybe_unused]] const std::optional<pybind11::object> &value,
                [[maybe_unused]] const std::optional<pybind11::object> &traceback) { hnd.close(); },
            "Stop the search closing the handle.");

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
