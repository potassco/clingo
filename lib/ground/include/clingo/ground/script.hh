#pragma once

#include <clingo/core/location.hh>
#include <clingo/core/logger.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_script
//! @{

//! Interface to execute code in source files.
class ScriptExec {
  public:
    //! Virtual destructor.
    virtual ~ScriptExec() = default;
    //! Execute the given code for the given script.
    void exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) {
        do_exec(loc, log, name, code);
    }

  private:
    virtual void do_exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) = 0;
};

//! Interface to call functions during parsing/grounding.
//!
//! This interface is used by external functions.
class ScriptCallback {
  public:
    //! Virtual destructor.
    virtual ~ScriptCallback() = default;
    //! Check if the function with the given name and number of arguments is callable.
    auto callable(std::string_view name, size_t args) -> bool { return do_callable(name, args); }
    //! Call the function with the given name and arguments.
    void call(Location const &loc, std::string_view name, SymbolSpan args, SymbolVec &out) {
        do_call(loc, name, args, out);
    }

  private:
    virtual auto do_callable(std::string_view name, size_t args) -> bool = 0;
    virtual void do_call(Location const &loc, std::string_view name, SymbolSpan args, SymbolVec &out) = 0;
};

//! @}

} // namespace CppClingo::Ground
