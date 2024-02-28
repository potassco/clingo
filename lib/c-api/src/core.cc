#include "lib.hh"
#include "streams.hh"

#include <gringo/input/algo/print.hh>
#include <gringo/input/location.hh>

#include <cstring>

extern "C" void clingo_version(int *major, int *minor, int *revision) {
    *major = CLINGO_VERSION_MAJOR;
    *minor = CLINGO_VERSION_MINOR;
    *revision = CLINGO_VERSION_REVISION;
}

static_assert(static_cast<int>(Gringo::ErrorCode::success) == clingo_error_success);
static_assert(static_cast<int>(Gringo::ErrorCode::runtime) == clingo_error_runtime);
static_assert(static_cast<int>(Gringo::ErrorCode::logic) == clingo_error_logic);
static_assert(static_cast<int>(Gringo::ErrorCode::bad_alloc) == clingo_error_bad_alloc);
static_assert(static_cast<int>(Gringo::ErrorCode::unknown) == clingo_error_unknown);

extern "C" auto clingo_error_string(clingo_error_t code) -> char const * {
    switch (static_cast<clingo_error_e>(code)) {
        case clingo_error_success: {
            return "success";
        }
        case clingo_error_runtime: {
            return "runtime error";
        }
        case clingo_error_bad_alloc: {
            return "bad allocation";
        }
        case clingo_error_logic: {
            return "logic error";
        }
        case clingo_error_unknown: {
            return "unknown error";
        }
    }
    return nullptr;
}

static_assert(static_cast<int>(Gringo::MessageCode::trace) == clingo_message_trace);
static_assert(static_cast<int>(Gringo::MessageCode::debug) == clingo_message_debug);
static_assert(static_cast<int>(Gringo::MessageCode::info) == clingo_message_info);
static_assert(static_cast<int>(Gringo::MessageCode::info_operation_undefined) == clingo_message_operation_undefined);
static_assert(static_cast<int>(Gringo::MessageCode::info_atom_undefined) == clingo_message_atom_undefined);
static_assert(static_cast<int>(Gringo::MessageCode::info_file_included) == clingo_message_file_included);
static_assert(static_cast<int>(Gringo::MessageCode::info_global_variable) == clingo_message_global_variable);
static_assert(static_cast<int>(Gringo::MessageCode::warn) == clingo_message_warn);
static_assert(static_cast<int>(Gringo::MessageCode::error) == clingo_message_error);

extern "C" auto clingo_message_string(clingo_message_t code) -> char const * {
    switch (static_cast<clingo_message_e>(code)) {
        case clingo_message_trace: {
            return "trace";
        }
        case clingo_message_debug: {
            return "debug";
        }
        case clingo_message_info: {
            return "info";
        }
        case clingo_message_operation_undefined: {
            return "operation undefined";
        }
        case clingo_message_atom_undefined: {
            return "atom undefined";
        }
        case clingo_message_file_included: {
            return "file included";
        }
        case clingo_message_global_variable: {
            return "global variable";
        }
        case clingo_message_warn: {
            return "warning";
        }
        case clingo_message_error: {
            return "error";
        }
    }
    return "unknown message code";
}

extern "C" auto clingo_lib_new(clingo_lib_flags_t flags, clingo_logger_t logger, void *logger_data,
                               size_t message_limit) -> clingo_lib_t * {
    clingo_lib_t *lib = nullptr;
    try {
        Gringo::Logger::Printer prt = nullptr;
        if (logger != nullptr) {
            prt = [logger, logger_data](Gringo::MessageCode code, char const *msg) {
                return logger(static_cast<clingo_message_t>(code), msg, logger_data);
            };
        }
        lib = new clingo_lib{
            prt, Gringo::Logger{prt, message_limit},
            Gringo::make_symbol_store((flags & clingo_lib_flags_slotted) != 0, (flags & clingo_lib_flags_shared) != 0)};
    } catch (...) {
    }
    return lib;
}

extern "C" void clingo_lib_free(clingo_lib_t *lib) { delete lib; }

extern "C" void clingo_set_error(clingo_lib_t *lib, clingo_error_t code, char const *message) {
    if (lib != nullptr) {
        lib->last_code = code;
        try {
            lib->last_exception = std::make_exception_ptr(std::runtime_error(message));
        } catch (...) {
            lib->last_exception = nullptr;
        }
    }
}

extern "C" auto clingo_error_message(clingo_lib_t *lib) -> char const * {
    if (lib != nullptr && lib->last_exception) {
        try {
            std::rethrow_exception(lib->last_exception);
        } catch (std::bad_alloc const &) {
            return "bad_alloc";
        } catch (std::exception const &e) {
            try {
                lib->last_message = e.what();
                return lib->last_message.c_str();
            } catch (std::exception const &) {
                return "no message";
            }
        }
    }
    return "no message";
}

extern "C" auto clingo_error_code(clingo_lib_t *lib) -> clingo_error_t {
    if (lib != nullptr) {
        return lib->last_code;
    }
    return clingo_error_runtime;
}

extern "C" auto clingo_location_less_than(clingo_location_t const *a, clingo_location_t const *b) -> bool {
    if (a->begin_file != b->begin_file) {
        return std::strcmp(a->begin_file, b->begin_file) < 0;
    }
    if (a->begin_line != b->begin_line) {
        return a->begin_line < b->begin_line;
    }
    if (a->begin_column != b->begin_column) {
        return a->begin_column < b->begin_column;
    }
    if (a->end_file != b->end_file) {
        return std::strcmp(a->end_file, b->end_file) < 0;
    }
    if (a->end_line != b->end_line) {
        return a->end_line < b->end_line;
    }
    return a->end_column < b->end_column;
}

extern "C" auto clingo_location_equal(clingo_location_t const *a, clingo_location_t const *b) -> bool {
    return a->begin_file == b->begin_file && a->begin_line == b->begin_line && a->begin_column == b->begin_column &&
           a->end_file == b->end_file && a->end_line == b->end_line && a->end_column == b->end_column;
}

extern "C" auto clingo_location_hash(clingo_location_t const *loc) -> size_t {
    return Gringo::Util::hash_mix(Gringo::Util::value_hash_record<clingo_location_t>(
        reinterpret_cast<uintptr_t>(loc->begin_file), reinterpret_cast<uintptr_t>(loc->end_file), loc->begin_line,
        loc->end_line, loc->begin_column, loc->end_column));
}

extern "C" auto clingo_location_to_string_size(clingo_location_t location, size_t *size) -> bool {
    if (size == nullptr) {
        return false;
    }
    try {
        auto loc = Gringo::Input::Location{{Gringo::String::from_rep(reinterpret_cast<uint64_t>(location.begin_file)),
                                            location.begin_line, location.begin_column},
                                           {Gringo::String::from_rep(reinterpret_cast<uint64_t>(location.end_file)),
                                            location.end_line, location.end_column}};
        *size = print_size(loc);
        return true;
    } catch (...) {
        return false;
    }
}

extern "C" auto clingo_location_to_string(clingo_location_t location, char *string, size_t size) -> bool {
    if (string == nullptr) {
        return false;
    }
    try {
        auto loc = Gringo::Input::Location{{Gringo::String::from_rep(reinterpret_cast<uint64_t>(location.begin_file)),
                                            location.begin_line, location.begin_column},
                                           {Gringo::String::from_rep(reinterpret_cast<uint64_t>(location.end_file)),
                                            location.end_line, location.end_column}};
        print(string, size, loc);
        return true;
    } catch (...) {
        return false;
    }
}
