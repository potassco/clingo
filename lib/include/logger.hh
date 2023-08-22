#pragma once

#include <bitset>
#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>

// TODO: windows
#include <unistd.h>

namespace Gringo {

static constexpr size_t default_message_limit = 20;

//! Error codes.
//!
//! This is used by the C-API.
enum class ErrorCode : int { success = 0, runtime = 1, logic = 2, bad_alloc = 3, unknown = 4 };

//! Exception thrown when an error code is set.
//!
//! This is used by the C-API.
class GringoError : public std::runtime_error {
  public:
    GringoError(char const *msg) : std::runtime_error(msg) {}
};

//! Codes of messages.
//!
//! Codes larger or equal to error indicate non-recoverable runtime errors.
enum class MessageCode : int {
    trace = 0,
    debug = 1,
    info = 3,
    info_operation_undefined = 4,
    info_atom_undefined = 5,
    info_file_included = 6,
    info_global_variable = 7,
    warn = 8,
    error = 9,
};

//! Exception thrown when there is an error and the message limit has been reached.
class MessageLimitError : public std::runtime_error {
  public:
    MessageLimitError(char const *msg) : std::runtime_error(msg) {}
};

//! Log levels for course grain configuration of logging.
enum class LogLevel : int {
    trace = static_cast<int>(MessageCode::trace),
    debug = static_cast<int>(MessageCode::debug),
    info = static_cast<int>(MessageCode::info),
    warn = static_cast<int>(MessageCode::warn),
    error = static_cast<int>(MessageCode::error),
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
        : p_(std::move(p)), limit_(limit), color_{isatty(fileno(stderr)) == 1} {}

    //! Check if a message with the given code should be reported.
    [[nodiscard]] auto check(MessageCode code) -> bool;
    //! Check if the logger is in the error state.
    [[nodiscard]] auto has_error() const -> bool;
    //! Enable or disable a message code.
    //!
    //! Note that errors cannot be disabled and are always reported.
    void enable(MessageCode code, bool enable);
    //! Unconditonally output a message with a given code.
    void print(MessageCode code, char const *msg);
    //! Set the log level.
    void set_level(LogLevel level);
    //! Set the message limit.
    void set_limit(size_t limit);

  private:
    Printer p_;
    LogLevel level_ = LogLevel::info;
    size_t limit_;
    std::bitset<static_cast<int>(MessageCode::error) + 1> disabled_;
    bool error_ = false;
    bool color_;
};

class Report {
  public:
    Report(Logger &p, MessageCode code) : p_(p), code_(code) {}
    ~Report() { p_.print(code_, out_.str().c_str()); }
    [[nodiscard]] auto out() -> std::ostringstream & { return out_; }

  private:
    std::ostringstream out_;
    Logger &p_;
    MessageCode code_;
};

inline auto Logger::check(MessageCode code) -> bool {
    // unconditionally report errors
    if (code >= MessageCode::error) {
        error_ = true;
        if (limit_ == 0) {
            throw MessageLimitError("too many messages.");
        }
        if (limit_ != std::numeric_limits<size_t>::max()) {
            --limit_;
        }
        return true;
    }
    // ignore the message
    if (code < static_cast<MessageCode>(level_) || disabled_[static_cast<int>(code)]) {
        return false;
    }
    // output the message
    if (limit_ > 0) {
        if (limit_ != std::numeric_limits<size_t>::max()) {
            --limit_;
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

inline auto Logger::has_error() const -> bool { return error_; }

inline void Logger::enable(MessageCode code, bool enabled) { disabled_[static_cast<int>(code)] = !enabled; }

inline void Logger::set_level(LogLevel level) { level_ = level; }

inline void Logger::set_limit(size_t limit) { limit_ = limit; }

inline void Logger::print(MessageCode code, char const *msg) {
    if (p_ != nullptr) {
        p_(code, msg);
    } else {
        char const *prefix = color_ ? "\033[31m"
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
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        fprintf(stderr, "%s: %s\n", prefix, msg);
        fflush(stderr);
    }
}

} // namespace Gringo

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GRINGO_REPORT(p, id)                                                                                           \
    if (!(p).check(::Gringo::MessageCode::id)) {                                                                       \
    } else                                                                                                             \
        Gringo::Report(p, ::Gringo::MessageCode::id).out()
