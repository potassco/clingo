#include "lib.hh"

namespace {

void default_error_logger(clingo_result_t code, char const *message, [[maybe_unused]] void *data) {
    char const *type = "error";
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
    fprintf(stderr, "%s: %s\n", type, message);
}

struct ErrorLogger {
  public:
    static auto instance() -> ErrorLogger & {
        static auto logger = ErrorLogger();
        return logger;
    }

    static void log(clingo_result_t code, char const *message) noexcept {
        auto &x = instance();
        if (x.logger_ != nullptr) {
            x.logger_(code, message, x.data_);
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

auto handle_error() -> clingo_result_t {
    try {
        throw;
    } catch (std::bad_alloc const &e) {
        ErrorLogger::log(clingo_result_bad_alloc, e.what());
        return clingo_result_bad_alloc;
    } catch (std::range_error const &e) {
        ErrorLogger::log(clingo_result_range, e.what());
        return clingo_result_range;
    } catch (std::invalid_argument const &e) {
        ErrorLogger::log(clingo_result_invalid, e.what());
        return clingo_result_invalid;
    } catch (std::logic_error const &e) {
        ErrorLogger::log(clingo_result_logic, e.what());
        return clingo_result_logic;
    } catch (ClingoError const &e) {
        return e.code();
    } catch (std::runtime_error const &e) {
        ErrorLogger::log(clingo_result_runtime, e.what());
        return clingo_result_runtime;
    } catch (...) {
        ErrorLogger::log(clingo_result_runtime, "no message");
        return clingo_result_runtime;
    }
}
