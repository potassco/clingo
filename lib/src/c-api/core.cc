#include "lib.hh"

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
    if (lib != nullptr) {
        try {
            std::rethrow_exception(lib->last_exception);
        } catch (std::bad_alloc const &) {
            return "bad_alloc";
        } catch (std::exception const &e) {
            try {
                lib->last_message = e.what();
                return lib->last_message.c_str();
            } catch (std::exception const &) {
                return nullptr;
            }
        }
    }
    return nullptr;
}

extern "C" auto clingo_error_code(clingo_lib_t *lib) -> clingo_error_t {
    if (lib != nullptr) {
        return lib->last_code;
    }
    return clingo_error_runtime;
}
