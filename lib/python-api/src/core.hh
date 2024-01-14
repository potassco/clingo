#pragma once

#include <sstream>

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

#define CLINGO_CPP_TOTAL_ORDER(type, T)                                                                                \
    type auto operator!=(T const &a, T const &b) -> bool { return !(a == b); }                                         \
    type auto operator<=(T const &a, T const &b) -> bool { return !(b < a); }                                          \
    type auto operator>(T const &a, T const &b) -> bool { return b < a; }                                              \
    type auto operator>=(T const &a, T const &b) -> bool { return !(a < b); }

static auto operator==(clingo_location_t const &a, clingo_location_t const &b) -> bool {
    return clingo_location_equal(&a, &b);
}

static auto operator<(clingo_location_t const &a, clingo_location_t const &b) -> bool {
    return clingo_location_less_than(&a, &b);
}

CLINGO_CPP_TOTAL_ORDER(static, clingo_location_t)

namespace Clingo {

namespace py = pybind11;

using LoggerCB = std::function<void(clingo_message_e, char const *)>;

static constexpr size_t default_message_limit = 25;

constexpr auto doc(char const *str) -> char const * { return str + 1; }

class Library;

void handle_error(Library &lib, bool success);

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

struct Position {
    static auto construct(Library &lib, char const *file_name, size_t line, size_t column) -> Position {
        char const *str;
        handle_error(lib, clingo_add_string(lib, file_name, &str));
        return {str, line, column};
    }
    [[nodiscard]] auto str() const -> std::string {
        std::ostringstream oss;
        oss << file << ":" << line << ":" << column;
        return oss.str();
    }

    [[nodiscard]] auto repr() const -> std::string {
        std::ostringstream oss;
        oss << "Position(" << py::cast<std::string>(py::str{file}.attr("__repr__")()) << "," << line << "," << column
            << ")";
        return oss.str();
    }

    [[nodiscard]] auto hash() const -> size_t {
        clingo_location_t loc = {file, "", line, 0, column, 0};
        return clingo_location_hash(&loc);
    }
    friend auto operator==(Position const &a, Position const &b) -> bool {
        return a.file == b.file && a.line == b.line && a.column == b.column;
    }
    friend auto operator<(Position const &a, Position const &b) -> bool {
        if (a.file != b.file) {
            return std::strcmp(a.file, b.file) < 0;
        }
        if (a.line != b.line) {
            return a.line < b.line;
        }
        return a.column < b.column;
    }
    CLINGO_CPP_TOTAL_ORDER(friend, Position)
    char const *file;
    size_t line;
    size_t column;
};

inline auto location_hash(clingo_location_t const &a) -> size_t { return clingo_location_hash(&a); }

[[nodiscard]] auto location_str(clingo_location_t const &loc) -> std::string {
    size_t len = 0;
    if (!clingo_location_to_string_size(loc, &len)) {
        throw std::runtime_error("could convert to string");
    }
    std::string str;
    str.resize(len);
    if (!clingo_location_to_string(loc, str.data(), len)) {
        throw std::runtime_error("could convert to string");
    }
    if (!str.empty() && str.back() == '\0') {
        str.pop_back();
    }
    return str;
}

[[nodiscard]] auto location_repr(clingo_location_t const &loc) -> std::string {
    std::ostringstream oss;
    oss << "Location("
        << "Position(" << py::cast<std::string>(py::str{loc.begin_file}.attr("__repr__")()) << "," << loc.begin_line
        << "," << loc.begin_column << "),"
        << "Position(" << py::cast<std::string>(py::str{loc.end_file}.attr("__repr__")()) << "," << loc.end_line << ","
        << loc.end_column << "))";
    return oss.str();
}

inline auto construct_location(Position const &begin, Position const &end) -> clingo_location_t {
    return {begin.file, end.file, begin.line, end.line, begin.column, end.column};
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
    py::class_<Position>(core, "Position", R"(Position tracking object.)")
        .def(py::init(&Position::construct))
        .def_readonly("file", &Position::file)
        .def_readonly("line", &Position::line)
        .def_readonly("column", &Position::column)
        .def("__str__", &Position::str)
        .def("__repr__", &Position::repr)
        .def("__hash__", &Position::hash)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py::class_<clingo_location_t>(core, "Location", R"(Location tracking object.)")
        .def(py::init(&construct_location))
        .def_property_readonly("begin",
                               [](clingo_location_t const &loc) {
                                   return Position{loc.begin_file, loc.begin_line, loc.begin_column};
                               })
        .def_property_readonly("end",
                               [](clingo_location_t const &loc) {
                                   return Position{loc.end_file, loc.end_line, loc.end_column};
                               })
        .def("__str__", &location_str)
        .def("__repr__", &location_repr)
        .def("__hash__", &location_hash)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;
}

} // namespace Clingo
