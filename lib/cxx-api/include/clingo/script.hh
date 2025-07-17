#pragma once

#include <clingo/control.hh>
#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/script.h>

namespace Clingo {

//! @addtogroup cpp_script
//! Support for external functions solving customizations.
//!
//! This module provides an interface to implement custom scripts that can be
//! used to provide external functions callable during grounding and to
//! customize the main application flow.
//! @{

//! Interface for custom scripts.
class Script {
  public:
    //! The default constructor.
    Script() = default;
    //! Disable copy and move operations.
    Script(Script &&other) = delete;
    //! Disable copy and move operations.
    auto operator=(Script &&other) -> Script & = delete;
    //! The default destructor.
    virtual ~Script() = default;

    //! Callback to execute the given code.
    //!
    //! @param code the code to execute
    void execute(std::string_view code) { do_execute(code); }

    //! Callback to call the function with the given name and arguments.
    //!
    //! @param lib the library object for storing symbols
    //! @param name the name of the function to call
    //! @param arguments the arguments to the function
    //! @return the symbols returned by the function
    auto call(Library &lib, std::string_view name, SymbolSpan arguments) -> SymbolVector {
        return do_call(lib, name, arguments);
    }

    //! Callback to check if the given signature is callable.
    //!
    //! @param name the name of the function to check
    //! @param arguments the number of arguments of the function
    //! @return whether the function is callable
    auto callable(std::string_view name, size_t arguments) -> bool { return do_callable(name, arguments); }

    //! Callback to customize the main function.
    //!
    //! @param lib the library object for storing symbols
    //! @param ctl the control object
    void main(Library &lib, const Control &ctl) { do_main(lib, ctl); }

    //! Get the name of the script.
    //!
    //! @return the name of the script
    auto name() -> std::string_view { return do_name(); }

    //! Get the version of the script.
    //!
    //! @return the version of the script
    auto version() -> std::string_view { return do_version(); }

  private:
    virtual void do_execute(std::string_view code) = 0;
    virtual auto do_call(Library &lib, std::string_view name, SymbolSpan arguments) -> SymbolVector = 0;
    virtual auto do_callable(std::string_view name, size_t arguments) -> bool = 0;
    virtual void do_main(Library &lib, const Control &ctl) = 0;
    virtual auto do_name() -> std::string_view = 0;
    virtual auto do_version() -> std::string_view = 0;
};

//! @}

namespace Detail {

static constexpr clingo_script_t c_script = {
    [](char const *code, size_t size, void *data) -> bool {
        CLINGO_TRY {
            static_cast<Script *>(data)->execute(std::string_view{code, size});
        }
        CLINGO_CATCH;
    },
    [](clingo_lib_t *lib, [[maybe_unused]] clingo_location_t const *loc, char const *name, size_t name_size,
       clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t symbol_callback,
       void *symbol_callback_data, void *data) -> bool {
        CLINGO_TRY {
            auto &self = *static_cast<Script *>(data);
            auto args = transform(arguments, std::next(arguments, static_cast<std::ptrdiff_t>(arguments_size)),
                                  [](auto sym) { return Symbol{sym, true}; });
            auto cpp_lib = Library{lib, true};
            auto syms = self.call(cpp_lib, {name, name_size}, args);
            auto const *c_syms = c_cast(syms.data());
            return symbol_callback(c_syms, syms.size(), symbol_callback_data);
        }
        CLINGO_CATCH;
    },
    [](char const *name, size_t size, size_t arguments, bool *result, void *data) -> bool {
        CLINGO_TRY {
            auto &self = *static_cast<Script *>(data);
            *result = self.callable({name, size}, arguments);
        }
        CLINGO_CATCH;
    },
    [](clingo_lib_t *lib, clingo_control_t *control, void *data) -> bool {
        CLINGO_TRY {
            auto &self = *static_cast<Script *>(data);
            auto cpp_lib = Library{lib, true};
            auto cpp_ctl = Control{control, true};
            self.main(cpp_lib, cpp_ctl);
        }
        CLINGO_CATCH;
    },
    [](void *data, clingo_string_t *name) {
        auto &self = *static_cast<Script *>(data);
        auto str = self.name();
        name->data = str.data();
        name->size = str.size();
    },
    [](void *data, clingo_string_t *version) {
        auto &self = *static_cast<Script *>(data);
        auto str = self.name();
        version->data = str.data();
        version->size = str.size();
    },
    [](void *data) { std::ignore = std::unique_ptr<Script>(static_cast<Script *>(data)); },
};

} // namespace Detail

template <std::derived_from<Script> T> auto register_script(Library const &lib, std::unique_ptr<T> script) -> T & {
    auto &res = *script;
    Detail::handle_error(clingo_script_register(c_cast(lib), &Detail::c_script, script.release()));
    return res;
}

} // namespace Clingo
