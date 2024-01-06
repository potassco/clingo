#include <pybind11/pybind11.h>

#include <clingo.h>

namespace Clingo::Core {

namespace py = pybind11;

auto version() -> pybind11::tuple {
    int major;
    int minor;
    int patch;
    clingo_version(&major, &minor, &patch);
    return pybind11::make_tuple(major, minor, patch);
}

void register_module(pybind11::module &m) {
    auto core = m.def_submodule("core");
    core.def("version", &version, "Clingo's version as a tuple (major, minor, revision).");

    py::enum_<clingo_message_e>(core, "MessageType")
        .value("Trace", clingo_message_trace)
        .value("Debug", clingo_message_debug)
        .value("Info", clingo_message_info)
        .value("OperationUndefined", clingo_message_operation_undefined)
        .value("AtomUndefined", clingo_message_atom_undefined)
        .value("FileIncluded", clingo_message_file_included)
        .value("GlobalVariable", clingo_message_global_variable)
        .value("Warn", clingo_message_warn)
        .value("Error", clingo_message_error);
}

} // namespace Clingo::Core
