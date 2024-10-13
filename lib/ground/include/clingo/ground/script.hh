#pragma once

#include <clingo/core/location.hh>
#include <clingo/core/logger.hh>

namespace Clingo::Ground {

class ScriptExec {
  public:
    virtual ~ScriptExec() = default;
    void exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) {
        do_exec(loc, log, name, code);
    }

  private:
    virtual void do_exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) = 0;
};

//! Callbacks that can be called during parsing/grounding.
//!
//! This interface is used by external functions.
class ScriptCallback {
  public:
    virtual ~ScriptCallback() = default;
    auto callable(std::string_view name, size_t args) -> bool { return do_callable(name, args); }
    void call(std::string_view name, SymbolSpan args, SymbolVec &out) { do_call(name, args, out); }

  private:
    virtual auto do_callable(std::string_view name, size_t args) -> bool = 0;
    virtual void do_call(std::string_view name, SymbolSpan args, SymbolVec &out) = 0;
};

} // namespace Clingo::Ground
