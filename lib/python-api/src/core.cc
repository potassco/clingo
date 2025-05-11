#include "core.hh"
#include "util.hh"

#include <sstream>

namespace Clingo::Python {

void raise_error() {
    clingo_string_t str;
    clingo_result_t rc = clingo_result_success;
    clingo_get_error(&rc, &str);
    switch (rc) {
        case clingo_result_bad_alloc: {
            throw std::bad_alloc{};
        }
        case clingo_result_logic: {
            throw std::logic_error{std::string{str.data, str.size}};
        }
        case clingo_result_invalid: {
            throw std::invalid_argument{std::string{str.data, str.size}};
        }
        case clingo_result_range: {
            throw std::out_of_range{std::string{str.data, str.size}};
        }
        default: {
            throw std::runtime_error{std::string{str.data, str.size}};
        }
    }
}

void handle_error_no_code(bool res) {
    if (!res) {
        raise_error();
    }
    clingo_string_t str;
    clingo_result_t rc = clingo_result_success;
    clingo_get_error(&rc, &str);
    if (rc != clingo_result_success) {
        raise_error();
    }
}

auto user_data_slot() noexcept -> size_t {
    static size_t slot = clingo_user_data_slot();
    return slot;
}

auto store_error() -> bool {
    try {
        throw;
    } catch (py::error_already_set const &e) {
        auto msg = std::string_view{e.what()};
        if (e.matches(PyExc_ValueError)) {
            return clingo_set_error(clingo_result_invalid, msg.data(), msg.size());
        }
        if (e.matches(PyExc_IndexError)) {
            return clingo_set_error(clingo_result_range, msg.data(), msg.size());
        }
        if (e.matches(PyExc_MemoryError)) {
            return clingo_set_error(clingo_result_bad_alloc, msg.data(), msg.size());
        }
        return clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    } catch (std::out_of_range const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_range, msg.data(), msg.size());
    } catch (std::invalid_argument const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_invalid, msg.data(), msg.size());
    } catch (std::bad_alloc const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_bad_alloc, msg.data(), msg.size());
    } catch (std::logic_error const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_logic, msg.data(), msg.size());
    } catch (std::exception const &e) {
        auto msg = std::string_view{e.what()};
        return clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    } catch (...) {
        auto msg = std::string_view{"no message"};
        return clingo_set_error(clingo_result_runtime, msg.data(), msg.size());
    }
}

auto string_builder() -> clingo_string_builder_t * {
    struct free_builder {
        void operator()(clingo_string_builder_t const *bld) { clingo_string_builder_free(bld); }
    };
    thread_local static std::unique_ptr<clingo_string_builder_t, free_builder> builder;
    if (builder == nullptr) {
        clingo_string_builder_t *bld = nullptr;
        handle_error(clingo_string_builder_new(&bld));
        builder.reset(bld);
    } else {
        clingo_string_builder_clear(builder.get());
    }
    return builder.get();
}

Library::Library(bool shared, bool slotted, clingo_log_level_e level, Annotation<std::optional<Logger>> cb,
                 size_t default_message_limit) {
    clingo_lib_flags_t flags = 0;
    if (shared) {
        flags |= clingo_lib_flags_shared;
    }
    if (slotted) {
        flags |= clingo_lib_flags_slotted;
    }
    auto logger = cb.cast<std::optional<Logger>>();
    auto *ptr = logger ? cb.ptr() : nullptr;
    clingo_lib_t *lib = nullptr;
    handle_error(clingo_lib_new(flags, level, ptr != nullptr ? &c_logger : nullptr, ptr, default_message_limit, &lib));
    lib_.reset(lib, false);
    registry.emplace(lib_.get(), this);
}

Library::Library(clingo_lib_t *lib) : lib_{lib} {
    registry.emplace(lib_.get(), this);
}

Library::~Library() {
    registry.erase(lib_.get());
}

auto Library::cast(clingo_lib_t *lib, bool convert) -> PyLibrary {
    auto it = registry.find(lib);
    if (it != registry.end()) {
        return py::cast(it->second);
    }
    if (!convert) {
        return py::cast(std::unique_ptr<Library>(new Library{lib}));
    }
    throw py::cast_error("invalid Library cast");
}

std::unordered_map<clingo_lib_t *, Library *> Library::registry;

clingo_logger_t Library::c_logger = {
    [](clingo_message_t code, char const *message, size_t size, void *log) {
        try {
            auto gil = py::gil_scoped_acquire{};
            auto hnd = py::reinterpret_borrow<py::object>(static_cast<PyObject *>(log));
            hnd.cast<Logger>()(static_cast<clingo_message_e>(code), {message, size});
        } catch (std::exception const &e) {
            printf("panic: exception with message %s thrown in logger\n", e.what());
            std::terminate();
        }
    },
    nullptr,
};

void Library::release(clingo_lib_t *lib) noexcept {
    if (lib != nullptr) {
        clingo_lib_release(lib);
        lib = nullptr;
    }
}

void Library::acquire(clingo_lib_t *lib, bool inc) {
    if (lib == nullptr) {
        return;
    }
    if (inc) {
        clingo_lib_acquire(lib);
    }
}

void Library::close() noexcept {
    lib_.reset();
}

Library::operator clingo_lib_t *() const {
    return lib_.get();
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
    handle_error(clingo_string_builder_new(&bld_));
}

StringBuilder::StringBuilder(StringBuilder const &other) {
    handle_error(clingo_string_builder_copy(other, &bld_));
}

auto StringBuilder::operator=(StringBuilder const &other) -> StringBuilder & {
    if (this != &other) {
        clingo_string_builder_free(bld_);
        handle_error(clingo_string_builder_copy(other, &bld_));
    }
    return *this;
}

StringBuilder::~StringBuilder() noexcept {
    clingo_string_builder_free(bld_);
}

auto StringBuilder::str() const -> std::string_view {
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld_, &str));
    return std::string_view{str.data, str.size};
}

// definition of position

Position::Position(clingo_position_t const *pos) {
    handle_error(clingo_position_copy(pos, &pos_));
}

Position::Position(Library &lib, std::string_view file, size_t line, size_t column) {
    handle_error(clingo_position_new(lib, file.data(), file.size(), line, column, &pos_));
}

Position::Position(Position const &other) {
    handle_error(clingo_position_copy(other, &pos_));
}

Position::Position(Position &&other) noexcept : pos_{std::exchange(other.pos_, nullptr)} {
}

auto Position::operator=(Position const &other) -> Position & {
    if (this != &other) {
        clingo_position_free(pos_);
        handle_error(clingo_position_copy(other, &pos_));
    }
    return *this;
}

auto Position::operator=(Position &&other) noexcept -> Position & {
    std::swap(pos_, other.pos_);
    return *this;
}

Position::~Position() noexcept {
    clingo_position_free(pos_);
}

auto Position::file() const -> std::string_view {
    clingo_string_t val;
    clingo_position_file(pos_, &val);
    return {val.data, val.size};
}

auto Position::line() const -> size_t {
    return clingo_position_line(pos_);
}

auto Position::column() const -> size_t {
    return clingo_position_column(pos_);
}

auto Position::str() const -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_position_to_string(pos_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

auto Position::repr() const -> std::string {
    std::ostringstream oss;
    oss << "Position(" << py::cast<std::string>(py::str{file()}.attr("__repr__")()) << "," << line() << "," << column()
        << ")";
    return oss.str();
}

auto Position::hash() const -> size_t {
    return clingo_position_hash(pos_);
}

auto operator==(Position const &a, Position const &b) -> bool {
    return clingo_position_equal(a, b);
}

auto operator<=>(Position const &a, Position const &b) -> std::strong_ordering {
    return clingo_position_compare(a, b) <=> 0;
}

// definition of location

Location::Location(clingo_location_t const *loc) {
    handle_error(clingo_location_copy(loc, &loc_));
}

Location::Location(Position const &begin, Position const &end) {
    handle_error(clingo_location_new(begin, end, &loc_));
}

Location::Location(Location const &other) {
    handle_error(clingo_location_copy(other, &loc_));
}

Location::Location(Location &&other) noexcept : loc_{std::exchange(other.loc_, nullptr)} {
}

auto Location::operator=(Location const &other) -> Location & {
    if (this != &other) {
        clingo_location_free(loc_);
        handle_error(clingo_location_copy(other, &loc_));
    }
    return *this;
}

auto Location::operator=(Location &&other) noexcept -> Location & {
    std::swap(loc_, other.loc_);
    return *this;
}

Location::~Location() noexcept {
    clingo_location_free(loc_);
}

auto Location::begin() const -> Position {
    return Position{clingo_location_begin(loc_)};
}

auto Location::end() const -> Position {
    return Position{clingo_location_end(loc_)};
}

auto Location::str() const -> std::string_view {
    auto *bld = string_builder();
    handle_error(clingo_location_to_string(loc_, bld));
    clingo_string_t str;
    handle_error(clingo_string_builder_string(bld, &str));
    return {str.data, str.size};
}

auto Location::repr() const -> std::string {
    std::ostringstream oss;
    oss << "Location(" << begin().repr() << "," << end().repr() << ")";
    return oss.str();
}

auto Location::hash() const -> size_t {
    return clingo_location_hash(loc_);
}

auto operator==(Location const &a, Location const &b) -> bool {
    return clingo_location_equal(a, b);
}

auto operator<=>(Location const &a, Location const &b) -> std::strong_ordering {
    return clingo_location_compare(a, b) <=> 0;
}

// register module

void register_core(pybind11::module &m) {
    auto core = m.def_submodule("core", R"(
Core functionality used throughout the clingo package.

Examples
--------

```python
>>> from clingo.core import version
>>> version()
(6, 0, 0)
```
)"_d);
    core.def("version", &version, R"(
Get Clingo's version.

Returns:
    A tuple (major, minor, revision) representing the Clingo version.
)"_d);

    py::enum_<clingo_log_level_e>(core, "LogLevel", "The available log levels.")
        .value("Trace", clingo_log_level_trace, R"(Report trace messages (includes debug level).)")
        .value("Debug", clingo_log_level_debug, R"(Report debug messages (includes info level).)")
        .value("Info", clingo_log_level_info, R"(Report info messages (includes warning level).)")
        .value("Warn", clingo_log_level_warn, R"(Report warning messages (includes error level).)")
        .value("Error", clingo_log_level_error, R"(Report error messages.)");

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

    py::class_<Library>(core, "Library",
                        R"(
A library object that manages Clingo's core resources.

This object is responsible for storing the logger, symbols, strings, and scripts.
Functions and classes that need to create symbols require an instance of this class.

This class implements the `ContextManager` interface.
)"_d)
        .def(py::init<bool, bool, clingo_log_level_e, Annotation<std::optional<Logger>>, size_t>(),
             "Create a library object.", py::arg("shared") = true, py::arg("slotted") = true,
             py::arg("log_level") = clingo_log_level_info, py::arg("logger") = std::nullopt,
             py::arg("message_limit") = default_message_limit, py::keep_alive<1, 5>(), // NOLINT
             R"(
Create a library object.

Args:
    slotted: Use a slotted allocator to store symbols. Setting this to true
        might improve performance.
    shared: Indicates whether symbols should be created in a thread-safe
        manner. Setting this to false might improve performance in
        single-threaded applications.
    log_level: The log level.
    logger: A logger to emit/intercept messages.
    message_limit: The maximum number of messages to emit.
)"_d)
        .def(
            "__enter__", [](Library &lib) -> Library & { return lib; }, R"(
Return self.
)"_d)
        .def(
            "__exit__",
            [](Library &lib, py::object const &, py::object const &, py::object const &) -> bool {
                lib.close();
                return false;
            },
            R"(
Close the library object.
)"_d)
        .def("_tie", &Library::tie, py::arg("object"), py::keep_alive<1, 2>{},
             R"(Tie the lifetime of the given object to that of the library.)");
    make_comparable(py::class_<Position>(core, "Position", R"(
Represents a position in a source file.

A `Position` object tracks the location of a symbol or construct
within a source file, including its file name, line number, and column.
)"_d))
        .def(py::init<Library &, char const *, size_t, size_t>(), py::arg("lib"), py::arg("file"), py::arg("line"),
             py::arg("column"), R"(
Create a position object.

Args:
    lib: The library object managing symbols.
    file: The file name where the position is located.
    line: The line number in the file.
    column: The column number in the line.
)"_d)
        .def_property_readonly("file", &Position::file, "The file name.")
        .def_property_readonly("line", &Position::line, "The line number.")
        .def_property_readonly("column", &Position::column, "The column number.")
        .def("__str__", &Position::str)
        .def("__repr__", &Position::repr);

    make_comparable(py::class_<Location>(core, "Location", R"(
Represents a range of positions in a source file.

The `Location` object tracks the start and end positions of a region in the
file. It is used for error reporting and debugging, providing information about
the source of the program elements.
)"_d))
        .def(py::init<Position const &, Position const &>(), py::arg("begin"), py::arg("end"), R"(
Create a location object.

Args:
    begin: The beginning of the location.
    end: The end of the location.
)"_d)
        .def_property_readonly("begin", &Location::begin, "The beginning of the location.")
        .def_property_readonly("end", &Location::end, "The end of the location.")
        .def("__str__", &Location::str)
        .def("__repr__", &Location::repr);
}

} // namespace Clingo::Python
