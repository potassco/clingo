#pragma once

#include <pybind11/pybind11.h>

#include <clingo.h>

namespace Clingo::Core {

namespace py = pybind11;

class Library {
  public:
    Library() {
        // TODO: make arguments available
        lib_ = clingo_lib_new(0, nullptr, nullptr, 25);
        if (lib_ == nullptr) {
            throw std::bad_alloc{};
        }
    }
    ~Library() { clingo_lib_free(lib_); }
    operator clingo_lib_t *() const { return lib_; }

  private:
    clingo_lib_t *lib_ = nullptr;
};

void handle_error(Library &lib, bool success) {
    if (!success) {
        auto const *msg = clingo_error_message(lib);
        switch (clingo_error_code(lib)) {
            case clingo_error_success:
            case clingo_error_unknown:
            case clingo_error_runtime: {
                throw std::logic_error(msg);
            }
            case clingo_error_logic: {
                throw std::logic_error(msg);
            }
            case clingo_error_bad_alloc: {
                throw std::bad_alloc();
            }
        }
    }
}

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

    py::class_<Library>(core, "Library").def(py::init());
}

} // namespace Clingo::Core
