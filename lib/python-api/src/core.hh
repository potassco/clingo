#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include <clingo.h>

#define CLINGO_PY_TOTAL_ORDER                                                                                          \
    .def(py::self == py::self)                                                                                         \
        .def(py::self != py::self)                                                                                     \
        .def(py::self < py::self)                                                                                      \
        .def(py::self <= py::self)                                                                                     \
        .def(py::self > py::self)                                                                                      \
        .def(py::self >= py::self)

#define CLINGO_CPP_TOTAL_ORDER(T)                                                                                      \
    friend auto operator!=(T const &a, T const &b) -> bool { return !(a == b); }                                       \
    friend auto operator<=(T const &a, T const &b) -> bool { return !(b < a); }                                        \
    friend auto operator>(T const &a, T const &b) -> bool { return b < a; }                                            \
    friend auto operator>=(T const &a, T const &b) -> bool { return !(a < b); }

namespace Clingo {

namespace py = pybind11;

using LoggerCB = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

constexpr auto doc(char const *str) -> char const * { return str + 1; }

class Library {
  public:
    Library(bool shared, bool slotted, LoggerCB cb, size_t default_message_limit) {
        clingo_lib_flags_t flags = 0;
        if (shared) {
            flags |= clingo_lib_flags_shared;
        }
        if (slotted) {
            flags |= clingo_lib_flags_slotted;
        }
        cb_ = std::move(cb);
        lib_ =
            clingo_lib_new(flags, cb_ ? static_cast<clingo_logger_t>(&logger_) : nullptr, this, default_message_limit);
        if (lib_ == nullptr) {
            throw std::bad_alloc{};
        }
    }
    Library(Library const &) = delete;
    Library(Library &&) = delete;
    ~Library() { clingo_lib_free(lib_); }
    void close() {
        clingo_lib_free(lib_);
        lib_ = nullptr;
    }
    operator clingo_lib_t *() const {
        if (lib_ == nullptr) {
            throw std::runtime_error("library has already been closed");
        }
        return lib_;
    }

  private:
    static void logger_(clingo_message_t code, char const *message, void *self) {
        try {
            static_cast<Library *>(self)->cb_(static_cast<clingo_message_e>(code), message);
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

auto version() -> std::tuple<int, int, int> {
    int major;
    int minor;
    int patch;
    clingo_version(&major, &minor, &patch);
    return {major, minor, patch};
}

void register_module(pybind11::module &m) {
    auto core = m.def_submodule("core", doc(R"(
Core functionality used throughout the clingo package.
)"));
    core.def("version", &version, "Clingo's version as a tuple (major, minor, revision).");

    py::enum_<clingo_message_e>(core, "MessageType", "Message categories emitted by the logger.")
        .value("Trace", clingo_message_trace, R"(A trace message.)")
        .value("Debug", clingo_message_debug, R"(A debug message.)")
        .value("Info", clingo_message_info, R"(A generic info message.)")
        .value("OperationUndefined", clingo_message_operation_undefined,
               R"(An info message about an undefined operation.)")
        .value("AtomUndefined", clingo_message_atom_undefined, R"(An info message about an undefined atom.)")
        .value("FileIncluded", clingo_message_file_included, R"(An info message about an already included file.)")
        .value("GlobalVariable", clingo_message_global_variable,
               R"(An info message about a global variable in the tuple of an aggregate.)")
        .value("Warn", clingo_message_warn, R"(A warning message.)")
        .value("Error", clingo_message_error, R"(An error message.)");

    py::class_<Library>(core, "Library", doc(R"(
Library objects are used to store symbols. Any function/or class that needs to
create symbols takes this object as a parameter.

Destroying the library object frees all symbols.

This class implements the ContextManager interface.
)"))
        .def(py::init<bool, bool, LoggerCB, size_t>(), "Create a library object.", py::arg("shared") = true,
             py::arg("slotted") = true, py::arg("logger") = nullptr, py::arg("message_limit") = default_message_limit,
             doc(R"(
Create a library object.

Parameters
----------
slotted
    Use a slotted allocator to store symbols. Setting this to true might
    improve performance.
shared
    Indicates whether symbols should be created in a thread-safe manner.
    Setting this to false might improve performance in single-threaded
    application.
logger
    A logger to emit/intercept messages.
message_limit
    The maximum number of messages to emit.
)"))
        .def(
            "__enter__", [](Library &lib) -> Library & { return lib; }, doc(R"(
Return self.
)"))
        .def(
            "__exit__",
            [](Library &lib, py::object, py::object, py::object) -> bool {
                lib.close();
                return false;
            },
            doc(R"(
Close the library object.
)"));
}

} // namespace Clingo
