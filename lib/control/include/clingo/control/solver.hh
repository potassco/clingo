#pragma once

#include <clingo/control/grounder.hh>

namespace Clingo::Control {

//! @addtogroup grounder
//! @{

enum class OutputMode : uint8_t { text };

class Solver;

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

//! Script with a main function and callbacks.
class ScriptMain : public ScriptCallback {
  public:
    void main(Solver &slv) { do_main(slv); }

  private:
    virtual void do_main(Solver &slv) = 0;
};

//! Script providing code execution, main, and callbacks.
//!
//! This interface should be implemend by custom scripts.
class Script : public ScriptMain {
  public:
    void exec(std::string_view code) { do_exec(code); }

  private:
    virtual void do_exec(std::string_view code) = 0;
};
using UScript = std::unique_ptr<Script>;

//! Helper to run specific code and callbacks.
//!
//! Named scripts can be registered. Code, main, and callback execution and is
//! dispatched to registered scripts.
class Scripts : public ScriptMain, public ScriptExec {
  public:
    void register_script(std::string_view name, UScript script);

  private:
    void do_exec(Logger &log, std::string_view name, std::string_view code) override;
    void do_main(Solver &slv) override;
    auto do_callable(std::string_view name, size_t args) -> bool override;
    void do_call(std::string_view name, SymbolSpan args, SymbolVec &out) override;

    std::vector<std::pair<std::string, UScript>> scripts_;
};

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver {
  public:
    //! Create a grounder object.
    Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, OutputMode mode);

    //! Parse and ground a program.
    void main(std::span<std::string_view const> const &files);

    //! Parse a program from the given string.
    void parse(std::string_view str);
    //! Parse the given files.
    void parse(std::span<std::string_view const> const &files);
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Ground the program.
    [[nodiscard]] auto ground(Input::ProgramParamVec const &params) -> bool;

    //! Register script.
    void register_script(std::string_view name, UScript script);

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

  private:
    Util::OutputBuffer buf_;
    std::unique_ptr<OutputStm> out_;
    Grounder grd_;
    Scripts *scripts_;
};

//! @}

} // namespace Clingo::Control
