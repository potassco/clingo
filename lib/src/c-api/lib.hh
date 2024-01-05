#include <clingo.h>

#include <exception>
#include <stdexcept>
#include <string>

#include <gringo/logger.hh>
#include <gringo/symbol.hh>

struct clingo_lib {
    Gringo::Logger::Printer prt;
    Gringo::Logger log;
    std::unique_ptr<Gringo::SymbolStore> store;
    std::exception_ptr last_exception = nullptr;
    std::string last_message = nullptr;
    clingo_error_t last_code = clingo_error_success;
};

namespace {

[[maybe_unused]] void handle_error(clingo_lib_t *lib) {
    try {
        throw;
    } catch (std::bad_alloc const &) {
        lib->last_exception = std::current_exception();
        lib->last_code = clingo_error_bad_alloc;
    } catch (std::logic_error const &) {
        lib->last_exception = std::current_exception();
        lib->last_code = clingo_error_logic;
    } catch (...) {
        lib->last_exception = std::current_exception();
        lib->last_code = clingo_error_runtime;
    }
}

} // namespace

#define CLINGO_TRY try
#define CLINGO_CATCH(lib)                                                                                              \
    catch (...) {                                                                                                      \
        if (lib != nullptr) {                                                                                          \
            handle_error(lib);                                                                                         \
        }                                                                                                              \
        return false;                                                                                                  \
    }                                                                                                                  \
    return true
