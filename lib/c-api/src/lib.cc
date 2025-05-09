#include "lib.hh"

namespace {

// TODO: remove
void default_error_logger(clingo_result_t code, char const *message, size_t size, [[maybe_unused]] void *data) {
    char const *type = "*** ERROR: (clingo)";
    switch (code) {
        case clingo_result_logic: {
            type = "logic error";
            break;
        }
        case clingo_result_bad_alloc: {
            type = "allocation error";
            break;
        }
        case clingo_result_invalid: {
            type = "invalid argument";
            break;
        }
        case clingo_result_range: {
            type = "range error";
            break;
        }
        default: {
            break;
        }
    }
    fprintf(stderr, "%s: %.*s\n", type, static_cast<int>(size), message);
}

// TODO: remove
struct ErrorLogger {
  public:
    static auto instance() -> ErrorLogger & {
        static auto logger = ErrorLogger();
        return logger;
    }

    static void log(clingo_result_t code, std::string_view msg) noexcept {
        auto &x = instance();
        if (x.logger_ != nullptr) {
            x.logger_(code, msg.data(), msg.size(), x.data_);
        }
    }

    static void set(clingo_error_logger_t logger, void *data) {
        auto &x = instance();
        x.logger_ = logger;
        x.data_ = data;
    }

  private:
    ErrorLogger() = default;

    clingo_error_logger_t logger_ = default_error_logger;
    void *data_ = nullptr;
};

} // namespace

// TODO: remove
extern "C" void clingo_error_logger(clingo_error_logger_t logger, void *data) {
    ErrorLogger::set(logger, data);
}

// TODO: remove
extern "C" void clingo_error_report(clingo_result_t code, char const *message, size_t size) {
    ErrorLogger::log(code, std::string_view{message, size});
}

auto Error::instance() noexcept -> Error & {
    thread_local auto error = Error{};
    return error;
}

auto Error::set_(clingo_result_t code, char const *message) noexcept -> clingo_result_t {
    code_ = code;
    try {
        message_.assign(message);
    } catch (std::exception &ptr) {
        message_.clear();
    }
    return code;
}

void Error::set(clingo_result_t code, char const *message, size_t size) noexcept {
    code_ = code;
    try {
        message_.assign(message, size);
    } catch (std::exception &ptr) {
        message_.clear();
    }
}

void Error::get(clingo_result_t *code, clingo_string_t *message) noexcept {
    *code = code_;
    message->data = message_.data();
    message->size = message_.size();
}

void Error::clear() noexcept {
    code_ = clingo_result_success;
    message_.clear();
}

void Error::raise() {
    switch (code_) {
        case clingo_result_bad_alloc: {
            code_ = clingo_result_success;
            message_.clear();
            throw std::bad_alloc{};
        }
        case clingo_result_logic: {
            code_ = clingo_result_success;
            throw std::logic_error{std::exchange(message_, "")};
        }
        case clingo_result_invalid: {
            code_ = clingo_result_success;
            throw std::invalid_argument{std::exchange(message_, "")};
        }
        default: {
            code_ = clingo_result_success;
            throw std::runtime_error{std::exchange(message_, "")};
        }
    }
}

auto Error::store() noexcept -> clingo_result_t {
    try {
        throw;
    } catch (std::bad_alloc const &e) {
        return set_(clingo_result_range, e.what());
    } catch (std::range_error const &e) {
        return set_(clingo_result_range, e.what());
    } catch (std::invalid_argument const &e) {
        return set_(clingo_result_invalid, e.what());
    } catch (std::logic_error const &e) {
        return set_(clingo_result_logic, e.what());
    } catch (ClingoError const &e) {
        return set_(e.code(), e.what());
    } catch (std::exception const &e) {
        return set_(clingo_result_runtime, e.what());
    } catch (...) {
        return set_(clingo_result_runtime, "no message");
    }
}

extern "C" void clingo_set_error(clingo_result_t code, char const *message, size_t size) {
    Error::instance().set(code, message, size);
}

extern "C" void clingo_get_error(clingo_result_t *code, clingo_string_t *message) {
    Error::instance().get(code, message);
}

auto store_error(clingo_result_t code, char const *msg, Error *error) -> clingo_result_t {
    auto str = std::string_view{msg};
    ErrorLogger::log(clingo_result_bad_alloc, msg);
    if (error != nullptr) {
        error->set(code, str.data(), str.size());
    } else {
        Error::instance().set(code, str.data(), str.size());
    }
    return code;
}

void raise_error(Error *error) {
    if (error != nullptr && error->active()) {
        Error::instance().clear();
        error->raise();
    } else {
        Error::instance().raise();
    }
}

auto store_error(Error *error) -> clingo_result_t {
    return error != nullptr ? error->store() : Error::instance().store();
}
