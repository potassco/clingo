#include "lib.hh"

namespace {

class Error {
  public:
    static auto instance() noexcept -> Error & {
        thread_local auto error = Error{};
        return error;
    }

    void set(clingo_result_t code, char const *message, size_t size) noexcept {
        code_ = code;
        try {
            message_.assign(message, size);
        } catch (std::exception &ptr) {
            message_.clear();
        }
    }

    void get(clingo_result_t *code, clingo_string_t *message) noexcept {
        *code = code_;
        message->data = message_.data();
        message->size = message_.size();
    }

    void clear() noexcept {
        code_ = clingo_result_success;
        message_.clear();
    }

    [[nodiscard]] auto active() const noexcept -> bool { return code_ != clingo_result_success; }

    void raise() {
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

    auto store() noexcept -> clingo_result_t {
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

  private:
    Error() = default;

    auto set_(clingo_result_t code, char const *message) noexcept -> clingo_result_t {
        code_ = code;
        try {
            message_.assign(message);
        } catch (std::exception &ptr) {
            message_.clear();
        }
        return code;
    }

    clingo_result_t code_ = clingo_result_success;
    std::string message_;
};

} // namespace

extern "C" void clingo_set_error(clingo_result_t code, char const *message, size_t size) {
    Error::instance().set(code, message, size);
}

extern "C" void clingo_get_error(clingo_result_t *code, clingo_string_t *message) {
    Error::instance().get(code, message);
}

void handle_error_no_code(clingo_result_t code) {
    if (code != clingo_result_success) {
        raise_error();
    } else if (Error::instance().active()) {
        Error::instance().raise();
    }
}

auto fail_arguments() -> clingo_result_t {
    return fail_with(clingo_result_invalid, "invalid arguments");
}

auto fail_with(clingo_result_t code, std::string_view msg) -> clingo_result_t {
    clingo_set_error(code, msg.data(), msg.size());
    return code;
}

void raise_error() {
    Error::instance().raise();
}

auto store_error() -> clingo_result_t {
    return Error::instance().store();
}
