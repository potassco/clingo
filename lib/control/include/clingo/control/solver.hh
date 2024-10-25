#pragma once

#include <clingo/control/grounder.hh>

namespace Clingo::Control {

//! @addtogroup control
//! @{

enum class OutputMode : uint8_t { text };

class Solver;

//! Script providing code execution, main, and callbacks.
//!
//! This interface should be implemend by custom scripts.
class Script : public Ground::ScriptCallback {
  public:
    void main(Solver &slv) { do_main(slv); }
    void exec(std::string_view code) { do_exec(code); }

  private:
    virtual void do_exec(std::string_view code) = 0;
    virtual void do_main(Solver &slv) = 0;
};
using UScript = std::unique_ptr<Script>;

//! Helper to run specific code and callbacks.
//!
//! Named scripts can be registered. Code, main, and callback execution and is
//! dispatched to registered scripts.
class Scripts : public Ground::ScriptCallback, public Ground::ScriptExec {
  public:
    void register_script(std::string_view name, UScript script);
    void main(Solver &slv);

  private:
    void do_exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) override;
    auto do_callable(std::string_view name, size_t args) -> bool override;
    void do_call(std::string_view name, SymbolSpan args, SymbolVec &out) override;

    std::vector<std::pair<std::string, UScript>> scripts_;
};

enum class AppMode : uint8_t { parse, rewrite, ground };

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver {
  public:
    //! Create a grounder object.
    Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, OutputMode mode,
           FILE *out = stdout);

    //! Parse and ground a program.
    void main(AppMode mode, std::span<std::string_view const> const &files,
              std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params);
    //! Ground a program.
    void main(AppMode mode, std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params);

    //! Parse a program from the given string.
    void join(Input::UnprocessedProgram const &prg);
    //! Parse a program from the given string.
    void parse(std::string_view str);
    //! Parse the given files.
    void parse(std::span<std::string_view const> const &files);
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Ground the program.
    [[nodiscard]] auto ground(Input::ProgramParamVec const &params) -> bool;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

    //! Get the output buffer.
    //!
    //! If the control object has been constructed without a null output FILE,
    //! this buffer contains the output of the textoutput.
    [[nodiscard]] auto buf() -> Util::OutputBuffer & { return buf_; };

  private:
    Util::OutputBuffer buf_;
    std::unique_ptr<OutputStm> out_;
    Grounder grd_;
    Scripts *scripts_;
};

//! @}

} // namespace Clingo::Control
