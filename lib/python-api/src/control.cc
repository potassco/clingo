#include "control.hh"
#include "core.hh"
#include "util.hh"

namespace Clingo::Python {

Control::Control(Library &lib, std::vector<std::string> const &args) {
    auto c_args = transform(args, [](auto const &str) { return str.c_str(); });
    auto *ctl = static_cast<clingo_control_t *>(nullptr);
    handle_error(clingo_control_new(lib, c_args.data(), c_args.size(), &ctl));
    ctl_.reset(ctl);
}

void Control::join(Program &prg) { handle_error(clingo_control_join(ctl_.get(), prg)); }

void Control::parse_string(char const *str) { handle_error(clingo_control_parse_string(ctl_.get(), str)); }

void Control::ground(std::vector<std::pair<std::string, SymbolVec>> const &parts) {
    auto c_args = transform(parts, [](auto const &part) {
        return clingo_part_t{part.first.c_str(),
                             // NOLINTNEXTLINE
                             reinterpret_cast<clingo_symbol_t const *>(part.second.data()), part.second.size()};
    });

    handle_error(clingo_control_ground(ctl_.get(), c_args.data(), c_args.size()));
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
)"));
    py::class_<Control>(control, "Control", R"(A control object for grounding and solving.)")
        .def(py::init<Library &, std::vector<std::string> const &>(), py::arg("lib"), py::arg("options"), doc(R"(
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
        .def("ground", &Control::ground, py::arg("parts"), doc(R"(
Ground the given program parts.

Args:
    parts: A list of tuples of part names and their symbolic arguments.
)"))
        .def("main", &Control::main, doc(R"(
Ground and solver a logic program.

This function proceeds as clingo calling the main function from a script if
there is any.
)"))
        .def_property_readonly("buffer", &Control::buffer, R"(The content of the output bufer.)");
}

} // namespace Clingo::Python
