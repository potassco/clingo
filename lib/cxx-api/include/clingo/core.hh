#pragma once

#include <clingo/core.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace Clingo {

namespace Detail {

#define CLINGO_ENABLE_BITSET_ENUM(E, ...)                                                                              \
    [[nodiscard]] CLINGO_ENUM_OP(~, (E a), __VA_ARGS__)->E {                                                           \
        return static_cast<E>(~static_cast<std::underlying_type_t<E>>(a));                                             \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(|, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(|=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(&, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(&=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(-, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(~b)); \
    }                                                                                                                  \
    CLINGO_ENUM_OP(-=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a - b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] CLINGO_ENUM_OP(^, (E a, E b), __VA_ARGS__)->E {                                                      \
        return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) ^ static_cast<std::underlying_type_t<E>>(b));  \
    }                                                                                                                  \
    CLINGO_ENUM_OP(^=, (E & a, E b), __VA_ARGS__)->E & {                                                               \
        return a = a ^ b;                                                                                              \
    }                                                                                                                  \
    [[nodiscard]] [[maybe_unused]] inline __VA_ARGS__ constexpr auto intersects(E a, E b) -> bool {                    \
        return static_cast<std::underlying_type_t<E>>(a & b) != 0;                                                     \
    }                                                                                                                  \
    static_assert(std::is_enum_v<E>)

#define CLINGO_ENUM_OP(op, arg, ...) [[maybe_unused]] inline __VA_ARGS__ constexpr auto operator op arg noexcept

#define CLINGO_TRY try
#define CLINGO_CATCH_PTR(x)                                                                                            \
    catch (...) {                                                                                                      \
        return Detail::handle_error(x);                                                                                \
    }                                                                                                                  \
    return clingo_result_success
#define CLINGO_CATCH                                                                                                   \
    catch (...) {                                                                                                      \
        return Detail::handle_error(Detail::get_exception_ptr());                                                      \
    }                                                                                                                  \
    return clingo_result_success

inline auto get_exception_ptr() -> std::exception_ptr & {
    thread_local std::exception_ptr ptr;
    return ptr;
}

inline auto handle_error(std::exception_ptr &ptr) -> clingo_result_t {
    try {
        throw;
    } catch (...) {
        ptr = std::current_exception();
    }
    return clingo_result_unknown;
}

inline void handle_error(clingo_result_t res) {
    switch (res) {
        case clingo_result_success: {
            return;
        }
        case clingo_result_runtime: {
            throw std::runtime_error("runtime error");
        }
        case clingo_result_logic: {
            throw std::runtime_error("logic error");
        }
        case clingo_result_range: {
            throw std::runtime_error("range error");
        }
        case clingo_result_bad_alloc: {
            throw std::runtime_error("bad alloc");
        }
        default: {
            throw std::runtime_error("unknown error");
        }
    }
}

inline auto user_data_slot() -> size_t {
    static auto slot = clingo_user_data_slot();
    return slot;
}

} // namespace Detail

//! Enumeration of message codes.
enum class MessageCode : clingo_message_t {
    trace = clingo_message_trace,                             //!< a trace message
    debug = clingo_message_debug,                             //!< a debug message
    info = clingo_message_info,                               //!< an info message
    operation_undefined = clingo_message_operation_undefined, //!< undefined operation in program
    atom_undefined = clingo_message_atom_undefined,           //!< undefined atom in program
    file_included = clingo_message_file_included,             //!< same file included multiple times
    global_variable = clingo_message_global_variable,         //!< global variable in tuple of aggregate element
    warn = clingo_message_warn,                               //!< a warning message
    error = clingo_message_error, //!< to report multiple errors; a corresponding runtime error is raised later
};

//! Enumeration of log levels.
enum class LogLevel {
    trace = clingo_log_level_trace, //!< the trace level (most verbose)
    debug = clingo_log_level_debug, //!< the debug level
    info = clingo_log_level_info,   //!< the info level
    wart = clingo_log_level_warn,   //!< the warning level
    error = clingo_log_level_error, //!< the error level (least verbose)
};

//! Flags to create library objects.
enum class LibraryFlags : clingo_lib_flags_t {
    none = 0,                                     //!< no flags set
    slotted = clingo_lib_flags_slotted,           //!< use custom allocator for storing symbols
    shared = clingo_lib_flags_shared,             //!< create symbols in a thread-safe manner
    fast_release = clingo_lib_flags_fast_release, //!< whether to enable fast release of libraries
};
CLINGO_ENABLE_BITSET_ENUM(LibraryFlags);

constexpr size_t default_message_limit = 25;

using Logger = std::function<void(MessageCode, char const *)>;

class Library {
  public:
    ~Library() { clingo_lib_release(rep_); }

    Library(Library const &other) noexcept : rep_{other.rep_} { clingo_lib_acquire(rep_); }
    auto operator=(Library const &other) noexcept -> Library & {
        clingo_lib_acquire(other.rep_);
        clingo_lib_release(rep_);
        rep_ = other.rep_;
        return *this;
    }

    Library(Library &&other) noexcept : rep_{std::exchange(other.rep_, nullptr)} {}
    auto operator=(Library &&other) noexcept -> Library & {
        if (rep_ != other.rep_) {
            clingo_lib_release(rep_);
            rep_ = std::exchange(other.rep_, nullptr);
        }
        return *this;
    }

    Library(LibraryFlags flags = LibraryFlags::none, Logger logger = nullptr, LogLevel level = LogLevel::info,
            size_t limit = default_message_limit) {
        auto log = std::make_unique<Logger>(logger ? std::move(logger) : nullptr);
        Detail::handle_error(clingo_lib_new(static_cast<clingo_lib_flags_t>(flags),
                                            static_cast<clingo_log_level_t>(level), log ? &logger_ : nullptr, log.get(),
                                            limit, &rep_));
        if (log) {
            Detail::handle_error(
                clingo_lib_set_user_data(rep_, Detail::user_data_slot(), log.release(), &free_logger_));
        }
    }
    explicit Library(clingo_lib_t *rep, bool acquire) : rep_{rep} {
        if (acquire) {
            clingo_lib_acquire(rep_);
        }
    }

    [[nodiscard]] friend auto c_cast(Library const &lib) -> clingo_lib_t * { return lib.rep_; }

  private:
    static void free_logger_(void *data) noexcept { std::unique_ptr<Logger>(static_cast<Logger *>(data)); }
    static void logger_(clingo_message_t code, char const *message, void *data) {
        (*static_cast<Logger *>(data))(static_cast<MessageCode>(code), message);
    }

    clingo_lib_t *rep_ = nullptr;
};

class StringBuilder {
  public:
    StringBuilder() { Detail::handle_error(clingo_string_builder_new(&rep_)); }
    ~StringBuilder() { clingo_string_builder_free(rep_); }

    StringBuilder(StringBuilder const &other) { Detail::handle_error(clingo_string_builder_copy(other.rep_, &rep_)); }
    auto operator=(StringBuilder const &other) -> StringBuilder & {
        if (other.rep_ != rep_) {
            clingo_string_builder_free(rep_);
            Detail::handle_error(clingo_string_builder_copy(other.rep_, &rep_));
        }
        return *this;
    }

    StringBuilder(StringBuilder &&other) noexcept : rep_{std::exchange(other.rep_, nullptr)} {}
    auto operator=(StringBuilder &&other) noexcept -> StringBuilder & {
        if (other.rep_ != rep_) {
            clingo_string_builder_free(rep_);
            rep_ = std::exchange(other.rep_, nullptr);
        }
        return *this;
    }

    [[nodiscard]] friend auto c_cast(StringBuilder &bld) -> clingo_string_builder_t * { return bld.rep_; }

    [[nodiscard]] auto str() const -> char const * {
        size_t size = 0;
        char const *res = nullptr;
        Detail::handle_error(clingo_string_builder_string(rep_, &res, &size));
        return res;
    }

    void clear() noexcept { clingo_string_builder_clear(rep_); }

  private:
    clingo_string_builder_t *rep_ = nullptr;
};

} // namespace Clingo
