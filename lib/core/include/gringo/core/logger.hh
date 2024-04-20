#pragma once

#include <bitset>
#include <cstdio>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace Gringo {

//! @addtogroup core_logger
//! @{

//! The default message limit.
static constexpr size_t default_message_limit = 20;

//! Error codes.
//!
//! This is used by the C-API.
enum class ErrorCode : uint8_t {
    success = 0,   //!< There was no error.
    runtime = 1,   //!< A runtime error.
    logic = 2,     //!< A logic error.
    bad_alloc = 3, //!< A failed allocation.
    unknown = 4    //!< An uncategorised error.
};

//! Exception thrown when an error code is set.
//!
//! This is used by the C-API.
class GringoError : public std::runtime_error {
  public:
    //! Construct the error.
    GringoError(char const *msg) : std::runtime_error(msg) {}
};

//! Codes of messages.
//!
//! Codes larger or equal to error indicate non-recoverable runtime errors.
enum class MessageCode : uint8_t {
    trace = 0,                    //! Trace messages.
    debug = 1,                    //! Debug messages.
    info = 2,                     //! Generic info messages.
    info_operation_undefined = 3, //! Info message for undefined operations.
    info_atom_undefined = 4,      //! Info message for undefined atoms.
    info_file_included = 5,       //! Info message for duplicate includes.
    info_global_variable = 6,     //! Info message for global variables.
    warn = 7,                     //! A warning.
    error = 8,                    //! An error.
};

//! Exception thrown when there is an error and the message limit has been reached.
class MessageLimitError : public std::runtime_error {
  public:
    //! Construct the error.
    MessageLimitError(char const *msg) : std::runtime_error(msg) {}
};

//! Log levels for course grain configuration of logging.
enum class LogLevel : uint8_t {
    trace = static_cast<uint8_t>(MessageCode::trace), //!< Trace as much as possible.
    debug = static_cast<uint8_t>(MessageCode::debug), //!< Output debug messages.
    info = static_cast<uint8_t>(MessageCode::info),   //!< Output info messages.
    warn = static_cast<uint8_t>(MessageCode::warn),   //!< Output warnings.
    error = static_cast<uint8_t>(MessageCode::error), //!< Output errors.
};

//! Simple logger to report message to stderr or via a callback.
class Logger {
  public:
    //! Callback to report messages.
    using Printer = std::function<void(MessageCode, char const *)>;
    //! Contruct a logger reporting messages to stderr.
    Logger(size_t limit = default_message_limit) : Logger{nullptr, limit} {}
    //! Contruct a logger reporting messages via the given callback.
    Logger(Printer p, size_t limit = default_message_limit)
        : p_(std::move(p)), limit_{limit}, cur_limit_(limit), color_{p_ == nullptr && isatty(fileno(stderr)) == 1} {}

    //! Check if a message with the given code should be reported.
    [[nodiscard]] auto check(MessageCode code) -> bool;
    //! Check if the logger is in the error state.
    [[nodiscard]] auto has_error() const -> bool;
    //! Enable or disable a message code.
    //!
    //! Note that errors cannot be disabled and are always reported.
    void enable(MessageCode code, bool enable);
    //! Check if the given message code is enabled.
    [[nodiscard]] auto enabled(MessageCode code) const -> bool;
    //! Unconditonally output a message with a given code.
    void print(MessageCode code, char const *msg);
    //! Unconditonally output a message with a given code.
    void print(MessageCode code, std::string const &str);
    //! Set the log level.
    void set_level(LogLevel level);
    //! Set the message limit.
    void set_limit(size_t limit);
    //! Get a string representation of the message category.
    [[nodiscard]] auto message_prefix(MessageCode code) const -> char const *;
    //! Reset the logger to the constructed state.
    //!
    //! This keeps all settings but resets the error flag and message limit.
    void reset();
    //! Explicitely enable or disable coloring.
    void enable_color(bool color) { color_ = color; };

  private:
    Printer p_;
    LogLevel level_ = LogLevel::info;
    size_t limit_;
    size_t cur_limit_;
    std::bitset<static_cast<int>(MessageCode::error) + 1> disabled_;
    bool error_ = false;
    bool color_;
};

//! Helper class to ease logging.
class Report {
  public:
    //! Construct reporter.
    Report(Logger &p, MessageCode code) : log_(p), code_(code) { out_ << log_.message_prefix(code) << ": "; }
    //! Construct reporter with additional location information.
    template <class Loc> Report(Logger &p, MessageCode code, Loc &loc) : log_(p), code_(code) {
        out_ << loc << ": " << log_.message_prefix(code) << ": ";
    }
    //! Destroy the reporter and output message.
    ~Report() noexcept(false) { log_.print(code_, out_.str().c_str()); }
    //! Get message sink.
    [[nodiscard]] auto out() -> std::ostringstream & { return out_; }

  private:
    std::ostringstream out_;
    Logger &log_;
    MessageCode code_;
};

inline auto Logger::check(MessageCode code) -> bool {
    // unconditionally report errors
    if (code >= MessageCode::error) {
        error_ = true;
        if (cur_limit_ == 0) {
            throw MessageLimitError("too many messages.");
        }
        if (cur_limit_ != std::numeric_limits<size_t>::max()) {
            --cur_limit_;
        }
        return true;
    }
    // ignore the message
    if (code < static_cast<MessageCode>(level_) || disabled_[static_cast<int>(code)]) {
        return false;
    }
    // report trace and debug messages without reducing the limit
    if (code < MessageCode::info) {
        return true;
    }
    // report the message
    if (cur_limit_ > 0) {
        if (cur_limit_ != std::numeric_limits<size_t>::max()) {
            --cur_limit_;
        }
        return true;
    }
    // raise error if limit has been reached
    if (error_) {
        throw MessageLimitError("too many messages.");
    }
    // ignore the message due to limit
    return false;
}

inline auto Logger::enabled(MessageCode code) const -> bool {
    if (code >= MessageCode::error) {
        return true;
    }
    if (code < static_cast<MessageCode>(level_) || disabled_[static_cast<int>(code)]) {
        return false;
    }
    return code < MessageCode::info || cur_limit_ > 0;
}

inline auto Logger::has_error() const -> bool { return error_; }

inline void Logger::enable(MessageCode code, bool enabled) { disabled_[static_cast<int>(code)] = !enabled; }

inline void Logger::set_level(LogLevel level) { level_ = level; }

inline void Logger::set_limit(size_t limit) { cur_limit_ = limit; }

inline void Logger::print(MessageCode code, char const *msg) {
    if (p_ != nullptr) {
        p_(code, msg);
    } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        fprintf(stderr, "%s\n", msg);
        fflush(stderr);
    }
}

inline void Logger::reset() {
    cur_limit_ = limit_;
    error_ = false;
}

inline void Logger::print(MessageCode code, std::string const &str) { print(code, str.c_str()); }

inline auto Logger::message_prefix(MessageCode code) const -> char const * {
    auto const *prefix = color_ ? "\033[31m"
                                  "error"
                                  "\033[0m"
                                : "error";
    if (code < MessageCode::debug) {
        prefix = color_ ? "\033[32m"
                          "trace"
                          "\033[0m"
                        : "trace";
    } else if (code < MessageCode::info) {
        prefix = color_ ? "\033[34m"
                          "debug"
                          "\033[0m"
                        : "debug";
    } else if (code < MessageCode::warn) {
        prefix = color_ ? "\033[35m"
                          "info"
                          "\033[0m"
                        : "info";
    } else if (code < MessageCode::error) {
        prefix = color_ ? "\033[33m"
                          "warning"
                          "\033[0m"
                        : "warning";
    }
    return prefix;
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

//! Report messages of the given type.
#define GRINGO_REPORT(p, id)                                                                                           \
    if ((p).check(::Gringo::MessageCode::id))                                                                          \
    Gringo::Report(p, ::Gringo::MessageCode::id).out()

//! Report messages of the given type and location.
#define GRINGO_REPORT_LOC(p, id, loc)                                                                                  \
    if ((p).check(::Gringo::MessageCode::id))                                                                          \
    Gringo::Report(p, ::Gringo::MessageCode::id, loc).out()

//! Report message of the given type given as string.
#define GRINGO_REPORT_STR(p, id, msg)                                                                                  \
    if ((p).check(::Gringo::MessageCode::id)) {                                                                        \
        (p).print(::Gringo::MessageCode::id, msg);                                                                     \
    }

// NOLINTEND(cppcoreguidelines-macro-usage)

//! @}

} // namespace Gringo
