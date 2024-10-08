#pragma once

#include <clingo/control/parse.hh>

#include <clingo/input/program.hh>

#include <clingo/core/output.hh>

namespace Clingo::Control {

//! @addtogroup grounder
//! @{

//! A grounder for logic programs.
//!
//! Takes care of parsing, grounding, and output.
class Grounder {
  public:
    struct Impl;
    //! Create a grounder object.
    Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out);
    //! Destroy grounder.
    ~Grounder() noexcept;
    //! Parse a program from the given string.
    void parse(std::string_view str, ScriptExec *code = nullptr);
    //! Parse the given files.
    void parse(std::vector<std::string> const &files, ScriptExec *code = nullptr);
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Ground the program.
    [[nodiscard]] auto ground(Input::ProgramParamVec const &params) -> bool;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

    //! Get the contained symbol store.
    [[nodiscard]] auto store() const -> SymbolStore &;
    //! Get the contained symbol store.
    [[nodiscard]] auto log() const -> Logger &;

  private:
    //! Prepare a program for grounding.
    void prepare_();

    std::unique_ptr<Impl> impl_;
};

//! @}

} // namespace Clingo::Control
