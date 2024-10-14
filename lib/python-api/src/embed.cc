#include "clingo.hh"
#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo.h>
#include <embed.h>

#include <pybind11/embed.h>

namespace Clingo::Python {

namespace py = pybind11;

class MainScript {
  public:
    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *lib = static_cast<clingo_lib_t *>(data);
        CLINGO_TRY {
            // TODO: execute
            static_cast<void>(code);
        }
        CLINGO_CATCH(lib);
    }

    static auto c_call(char const *name, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data,
                       void *data) -> clingo_result_t {
        auto *lib = static_cast<clingo_lib_t *>(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto syms = SymbolVec{};
            // TODO: call
            static_cast<void>(name);
            // NOLINTNEXTLINE
            auto const *c_syms = reinterpret_cast<clingo_symbol_t *>(syms.data());
            handle_error(symbol_callback(c_syms, syms.size(), symbol_callback_data));
        }
        CLINGO_CATCH(lib);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        auto *lib = static_cast<clingo_lib_t *>(data);
        CLINGO_TRY {
            // TODO: callable
            static_cast<void>(name);
            static_cast<void>(arguments);
            *result = false;
        }
        CLINGO_CATCH(lib);
    }

    static auto main(clingo_control_t *control, void *data) -> clingo_result_t {
        static_cast<void>(control);
        static_cast<void>(data);
        return clingo_result_logic;
    }

    static auto c_name(void *data) -> char const * {
        static_cast<void>(data);
        return "python";
    }

    static auto c_version(void *data) -> char const * {
        // TODO: get correct python version
        static_cast<void>(data);
        return "3.10";
    }

    static void c_free(void *data) {
        // NOLINTNEXTLINE
        delete static_cast<py::object *>(data);
    }
};

} // namespace Clingo::Python

extern "C" auto register_python(clingo_lib_t *lib) -> clingo_result_t {
    using Script = Clingo::Python::MainScript;
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    return clingo_script_register(lib, &c_script, lib);
}

PYBIND11_EMBEDDED_MODULE(clingo, m) { Clingo::Python::register_clingo(m); }
