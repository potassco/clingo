#pragma once

#include <pybind11/functional.h>
#include <pybind11/pybind11.h>

#include <clingo.h>

namespace Clingo::Core {

namespace py = pybind11;

using LoggerCB = std::function<void(clingo_message_e, char const *)>;

class Library {
  public:
    Library(bool shared, bool slotted, LoggerCB cb, size_t message_limit) {
        clingo_lib_flags_t flags = 0;
        if (shared) {
            flags |= clingo_lib_flags_shared;
        }
        if (slotted) {
            flags |= clingo_lib_flags_slotted;
        }
        cb_ = std::move(cb);
        lib_ = clingo_lib_new(flags, cb_ ? static_cast<clingo_logger_t>(&logger_) : nullptr, &cb_, message_limit);
        if (lib_ == nullptr) {
            throw std::bad_alloc{};
        }
    }
    ~Library() { clingo_lib_free(lib_); }
    operator clingo_lib_t *() const { return lib_; }

  private:
    static void logger_(clingo_message_t code, char const *message, void *cb) {
        try {
            std::invoke(*static_cast<LoggerCB *>(cb), static_cast<clingo_message_e>(code), message);
        } catch (std::exception const &e) {
            printf("panic: exception with message %s thrown in logger\n", e.what());
            std::terminate();
        }
    }

    clingo_lib_t *lib_ = nullptr;
    LoggerCB cb_;
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

    py::class_<Library>(core, "Library")
        .def(py::init<bool, bool, LoggerCB, size_t>(), py::arg("shared") = true, py::arg("slotted") = true,
             py::arg("logger") = nullptr, py::arg("message_limit") = 25);
}

} // namespace Clingo::Core
