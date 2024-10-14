#include "clingo.hh"
#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo.h>
#include <embed.h>

#include <pybind11/embed.h>

namespace Clingo::Python {

namespace py = pybind11;

namespace {

class Interpreter {
  public:
    Interpreter()
        : scope_{py::module_::import("__main__").attr("__dict__")},
          callable_{py::module_::import("builtins").attr("callable")},
          version_{py::module_::import("sys").attr("version").cast<std::string>()} {}

    void exec(char const *code) { py::exec(code, scope_); }

    auto callable(char const *name) -> bool { return callable_(scope_[name]).cast<bool>(); }

    auto call(char const *name, SymbolVec args) -> SymbolVec { return scope_[name](args).cast<SymbolVec>(); }

    auto version() -> char const * { return version_.c_str(); }

  private:
    py::scoped_interpreter py_;
    py::object scope_;
    py::object callable_;
    std::string version_;
};

class MainScript {
  public:
    MainScript(clingo_lib_t *lib) : lib_{lib} {}

    auto py() -> Interpreter & {
        if (!py_) {
            py_ = std::make_unique<Interpreter>();
        }
        return *py_;
    }

    static auto cast(void *data) -> MainScript * { return static_cast<MainScript *>(data); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY { self->py().exec(code); }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_call(char const *name, clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data,
                       void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto syms = self->py().call(name, args);
            // NOLINTNEXTLINE
            auto const *c_syms = reinterpret_cast<clingo_symbol_t *>(syms.data());
            handle_error(symbol_callback(c_syms, syms.size(), symbol_callback_data));
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        // Note: that python cannot check the number of arguments
        static_cast<void>(arguments);
        auto *self = cast(data);
        CLINGO_TRY { *result = self->py().callable(name); }
        CLINGO_CATCH(self->lib_);
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
        try {
            return cast(data)->py().version();
        } catch (...) {
            return "<error>";
        }
    }

    static void c_free(void *data) {
        // NOLINTNEXTLINE
        delete cast(data);
    }

  private:
    std::unique_ptr<Interpreter> py_;
    clingo_lib_t *lib_;
};

} // namespace

} // namespace Clingo::Python

extern "C" auto clingo_register_python(clingo_lib_t *lib) -> clingo_result_t {
    using Script = Clingo::Python::MainScript;
    auto c_script = clingo_script_t{Script::c_execute, Script::c_call,    Script::c_callable, Script::main,
                                    Script::c_name,    Script::c_version, Script::c_free};
    return clingo_script_register(lib, &c_script, std::make_unique<Script>(lib).release());
}

PYBIND11_EMBEDDED_MODULE(clingo, m) { Clingo::Python::register_clingo(m); }
