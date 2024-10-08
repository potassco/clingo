#pragma once

#include <gringo/control/grounder.hh>

namespace Gringo::Control {

//! @addtogroup grounder
//! @{

enum class OutputMode : uint8_t { text };

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver {
  public:
    //! Create a grounder object.
    Solver(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputMode mode);
    //! Parse a program from the given string.
    void parse(std::string_view str);
    //! Parse the given files.
    void parse(std::vector<std::string> const &files);
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Prepare a program for grounding.
    void prepare();
    //! Ground the program.
    [[nodiscard]] auto ground(Input::ProgramParamVec const &params) -> bool;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

  private:
    Util::OutputBuffer buf_;
    std::unique_ptr<OutputStm> out_;
    Grounder grd_;
};

//! @}

} // namespace Gringo::Control
