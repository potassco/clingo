#include "core.hh"
#include "util.hh"

#include <sstream>

namespace Clingo::Core {

namespace py = pybind11;

Library::Library(bool shared, bool slotted, LoggerCB cb, size_t default_message_limit) {
    clingo_lib_flags_t flags = 0;
    if (shared) {
        flags |= clingo_lib_flags_shared;
    }
    if (slotted) {
        flags |= clingo_lib_flags_slotted;
    }
    cb_ = std::move(cb);
    lib_ = clingo_lib_new(flags, cb_ ? static_cast<clingo_logger_t>(&logger_) : nullptr, this, default_message_limit);
    if (lib_ == nullptr) {
        throw std::bad_alloc{};
    }
}

Library::~Library() noexcept { clingo_lib_free(lib_, false); }

void Library::close() noexcept {
    clingo_lib_free(lib_, false);
    lib_ = nullptr;
}

Library::operator clingo_lib_t *() const {
    if (lib_ == nullptr) {
        throw std::runtime_error("library has already been closed");
    }
    return lib_;
}

void Library::logger_(clingo_message_t code, char const *message, void *self) {
    try {
        static_cast<Library *>(self)->cb_(static_cast<clingo_message_e>(code), message);
    } catch (std::exception const &e) {
        printf("panic: exception with message %s thrown in logger\n", e.what());
        std::terminate();
    }
}

auto version() -> std::tuple<int, int, int> {
    int major = 0;
    int minor = 0;
    int patch = 0;
    clingo_version(&major, &minor, &patch);
    return {major, minor, patch};
}

// definition of StringBuilder

StringBuilder::StringBuilder() {
    if (!clingo_string_builder_new(&bld_)) {
        throw std::bad_alloc();
    }
}

StringBuilder::StringBuilder(StringBuilder const &other) {
    if (!clingo_string_builder_copy(other, &bld_)) {
        throw std::bad_alloc();
    }
}

auto StringBuilder::operator=(StringBuilder const &other) -> StringBuilder & {
    if (this != &other) {
        clingo_string_builder_free(bld_);
        if (!clingo_string_builder_copy(other, &bld_)) {
            throw std::bad_alloc();
        }
    }
    return *this;
}

StringBuilder::~StringBuilder() noexcept { clingo_string_builder_free(bld_); }

auto StringBuilder::str() const -> std::string {
    char const *str = nullptr;
    size_t size = 0;
    if (!clingo_string_builder_string(bld_, &str, &size)) {
        throw std::bad_alloc();
    }
    return std::string{str, size};
}

// definition of position

Position::Position(clingo_position_t const *pos) {
    if (!clingo_position_copy(pos, &pos_)) {
        throw std::bad_alloc();
    }
}

Position::Position(Library &lib, char const *file, size_t line, size_t column) {
    handle_error(lib, clingo_position_new(lib, file, line, column, &pos_));
}

Position::Position(Position const &other) {
    if (!clingo_position_copy(other, &pos_)) {
        throw std::bad_alloc();
    }
}

Position::Position(Position &&other) noexcept : pos_{std::exchange(other.pos_, nullptr)} {}

auto Position::operator=(Position const &other) -> Position & {
    if (this != &other) {
        clingo_position_free(pos_);
        if (!clingo_position_copy(other, &pos_)) {
            throw std::bad_alloc();
        }
    }
    return *this;
}

auto Position::operator=(Position &&other) noexcept -> Position & {
    std::swap(pos_, other.pos_);
    return *this;
}

Position::~Position() noexcept { clingo_position_free(pos_); }

auto Position::file() const -> char const * { return clingo_position_file(pos_); }

auto Position::line() const -> size_t { return clingo_position_line(pos_); }

auto Position::column() const -> size_t { return clingo_position_column(pos_); }

auto Position::str() const -> std::string {
    auto bld = StringBuilder{};
    if (!clingo_position_to_string(pos_, bld)) {
        throw std::bad_alloc();
    }
    return bld.str();
}

auto Position::repr() const -> std::string {
    std::ostringstream oss;
    oss << "Position(" << py::cast<std::string>(py::str{file()}.attr("__repr__")()) << "," << line() << "," << column()
        << ")";
    return oss.str();
}

auto Position::hash() const -> size_t { return clingo_position_hash(pos_); }

auto operator==(Position const &a, Position const &b) -> bool { return clingo_position_equal(a, b); }

auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering {
    return clingo_position_compare(a, b) <=> 0;
}

// definition of location

Location::Location(clingo_location_t const *loc) {
    if (!clingo_location_copy(loc, &loc_)) {
        throw std::bad_alloc();
    }
}

Location::Location(Position const &begin, Position const &end) {
    if (!clingo_location_new(begin, end, &loc_)) {
        throw std::bad_alloc();
    }
}

Location::Location(Location const &other) {
    if (!clingo_location_copy(other, &loc_)) {
        throw std::bad_alloc();
    }
}

Location::Location(Location &&other) noexcept : loc_{std::exchange(other.loc_, nullptr)} {}

auto Location::operator=(Location const &other) -> Location & {
    if (this != &other) {
        clingo_location_free(loc_);
        if (!clingo_location_copy(other, &loc_)) {
            throw std::bad_alloc();
        }
    }
    return *this;
}

auto Location::operator=(Location &&other) noexcept -> Location & {
    std::swap(loc_, other.loc_);
    return *this;
}

Location::~Location() noexcept { clingo_location_free(loc_); }

auto Location::begin() const -> Position { return Position{clingo_location_begin(loc_)}; }

auto Location::end() const -> Position { return Position{clingo_location_end(loc_)}; }

auto Location::str() const -> std::string {
    auto bld = StringBuilder{};
    if (!clingo_location_to_string(loc_, bld)) {
        throw std::bad_alloc();
    }
    return bld.str();
}

auto Location::repr() const -> std::string {
    std::ostringstream oss;
    oss << "Location(" << begin().repr() << "," << end().repr() << ")";
    return oss.str();
}

auto Location::hash() const -> size_t { return clingo_location_hash(loc_); }

auto operator==(Location const &a, Location const &b) -> bool { return clingo_location_equal(a, b); }

auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering {
    return clingo_location_compare(a, b) <=> 0;
}

// register module

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
    applications.
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
            [](Library &lib, py::object const &, py::object const &, py::object const &) -> bool {
                lib.close();
                return false;
            },
            doc(R"(
Close the library object.
)"));
    py::class_<Position>(core, "Position", R"(Position tracking object.)")
        .def(py::init<Library &, char const *, size_t, size_t>(), py::arg("lib"), py::arg("file"), py::arg("line"),
             py::arg("column"))
        .def_property_readonly("file", &Position::file)
        .def_property_readonly("line", &Position::line)
        .def_property_readonly("column", &Position::column)
        .def("__str__", &Position::str)
        .def("__repr__", &Position::repr)
        .def("__hash__", &Position::hash)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;

    py::class_<Location>(core, "Location", R"(Location tracking object.)")
        .def(py::init<Position const &, Position const &>(), py::arg("begin"), py::arg("end"))
        .def_property_readonly("begin", &Location::begin)
        .def_property_readonly("end", &Location::end)
        .def("__str__", &Location::str)
        .def("__repr__", &Location::repr)
        .def("__hash__", &Location::hash)
        // generate comparison operators
        CLINGO_PY_TOTAL_ORDER;
}

} // namespace Clingo::Core
