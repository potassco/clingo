#include <gringo/input/program.hh>

namespace Gringo {

//! A grounder for logic programs.
class Grounder {
  public:
    Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts)
        : log_{log}, store_{store}, prg_{std::move(opts)} {};
    //! Parse a program from the given string.
    void parse(std::string_view prg);
    //! Parse the given files.
    void parse(std::vector<std::string> const &files);
    //! Prepare a program for grounding.
    void prepare();
    //! Ground the program.
    void ground();

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

  private:
    Logger &log_;
    SymbolStore &store_;
    Input::UnprocessedProgram unprocessed_prg_;
    Input::Program prg_;
};

} // namespace Gringo
