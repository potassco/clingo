#include <clingo.h>

#include <clingo/control/solver.hh>
#include <clingo/core/logger.hh>
#include <clingo/core/symbol.hh>

#include <exception>
#include <stdexcept>
#include <string>

struct clingo_lib {
    clingo_lib(Clingo::Logger::Printer prt, Clingo::Logger log, std::unique_ptr<Clingo::SymbolStore> store)
        : prt{std::move(prt)}, log{std::move(log)}, store{std::move(store)} {}
    Clingo::Logger::Printer prt;
    Clingo::Logger log;
    Clingo::Control::Scripts scripts;
    std::unique_ptr<Clingo::SymbolStore> store;
    std::exception_ptr last_exception = nullptr;
    std::string last_message;
    clingo_lib_t *next_ = nullptr;
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

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CLINGO_TRY try
#define CLINGO_CATCH(lib)                                                                                              \
    catch (...) {                                                                                                      \
        if ((lib) != nullptr) {                                                                                        \
            handle_error(lib);                                                                                         \
        }                                                                                                              \
        return false;                                                                                                  \
    }                                                                                                                  \
    return true

// NOLINTEND(cppcoreguidelines-macro-usage)
