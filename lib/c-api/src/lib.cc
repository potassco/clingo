#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
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
        if (code != nullptr) {
            *code = code_;
        }
        if (message != nullptr) {
            message->data = message_.data();
            message->size = message_.size();
        }
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

    auto store() noexcept -> bool {
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
        } catch (std::exception const &e) {
            return set_(clingo_result_runtime, e.what());
        } catch (...) {
            return set_(clingo_result_runtime, "no message");
        }
    }

  private:
    Error() = default;

    auto set_(clingo_result_t code, char const *message) noexcept -> bool {
        code_ = code;
        try {
            message_.assign(message);
        } catch (std::exception &ptr) {
            message_.clear();
        }
        return false;
    }

    clingo_result_t code_ = clingo_result_success;
    std::string message_;
};

} // namespace

void handle_error_no_code(bool res) {
    if (!res) {
        raise_error();
    } else if (Error::instance().active()) {
        Error::instance().raise();
    }
}

auto fail_arguments() -> bool {
    return fail_with(clingo_result_invalid, "invalid arguments");
}

auto fail_with(clingo_result_t code, std::string_view msg) -> bool {
    clingo_set_error(code, msg.data(), msg.size());
    return false;
}

void raise_error() {
    Error::instance().raise();
}

auto store_error() -> bool {
    return Error::instance().store();
}

} // namespace CppClingo::CAPI

extern "C" auto clingo_set_error(clingo_result_t code, char const *message, size_t size) -> bool {
    Error::instance().set(code, message, size);
    return false;
}

extern "C" void clingo_get_error(clingo_result_t *code, clingo_string_t *message) {
    Error::instance().get(code, message);
}

extern "C" void clingo_clear_error(void) {
    Error::instance().clear();
}
