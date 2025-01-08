#include "clingo.hh"
#include "control.hh"
#include "core.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/app.h>
#include <clingo/script.h>

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

    auto callable(char const *name) -> bool { return scope_.contains(name) && callable_(scope_[name]).cast<bool>(); }

    auto call(Library lib, char const *name, SymbolVec args) -> std::variant<SymbolVec, Symbol> {
        return scope_[name](&lib, *py::cast(args)).cast<std::variant<SymbolVec, Symbol>>();
    }

    auto main(Library lib, Control ctl) { scope_["main"](&lib, &ctl); }

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

    static auto cast(void *data) -> MainScript * { return static_cast<MainScript *>(data); }

    static auto c_execute(char const *code, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            if (!self->py_) {
                self->py_ = std::make_unique<Interpreter>();
            }
            self->py_->exec(code);
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_call(clingo_lib_t *lib, clingo_location_t const *loc, char const *name,
                       clingo_symbol_t const *arguments, size_t arguments_size,
                       clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *data)
        -> clingo_result_t {
        // Note that the location could in principle be used for better error reporting.
        static_cast<void>(loc);
        auto *self = cast(data);
        CLINGO_TRY {
            auto args = transform(arguments, std::next(arguments, static_cast<ssize_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            if (self->py_) {
                auto syms = self->py_->call(lib, name, args);
                return std::visit(
                    [&]<class T>(T const &res) {
                        if constexpr (std::is_same_v<T, Symbol>) {
                            // NOLINTNEXTLINE
                            auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(&res);
                            return symbol_callback(c_syms, 1, symbol_callback_data);
                        } else {
                            // NOLINTNEXTLINE
                            auto const *c_syms = reinterpret_cast<clingo_symbol_t const *>(res.data());
                            return symbol_callback(c_syms, res.size(), symbol_callback_data);
                        }
                    },
                    syms);
            }
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_callable(char const *name, size_t arguments, bool *result, void *data) -> clingo_result_t {
        // Note: that python cannot check the number of arguments
        static_cast<void>(arguments);
        auto *self = cast(data);
        CLINGO_TRY { *result = self->py_ && self->py_->callable(name); }
        CLINGO_CATCH(self->lib_);
    }

    static auto main(clingo_lib_t *lib, clingo_control_t *control, void *data) -> clingo_result_t {
        auto *self = cast(data);
        CLINGO_TRY {
            if (self->py_) {
                self->py_->main(lib, control);
            }
        }
        CLINGO_CATCH(self->lib_);
    }

    static auto c_name(void *data) -> char const * {
        static_cast<void>(data);
        return "python";
    }

    static auto c_version(void *data) -> char const * {
        static_cast<void>(data);
        return CLINGO_PYTHON_VERSION;
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
