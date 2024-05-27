#include <gringo/input/program.hh>

namespace Gringo {

//! A grounder for logic programs.
class Grounder {
  public:
    struct Impl;
    Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts);
    ~Grounder() noexcept;
    //! Parse a program from the given string.
    void parse(std::string_view prg);
    //! Parse the given files.
    void parse(std::vector<std::string> const &files);
    //! Prepare a program for grounding.
    void prepare();
    //! Ground the program.
    [[nodiscard]] auto ground(Input::ProgramParamVec const &params) -> bool;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);

  private:
    std::unique_ptr<Impl> impl_;
};

} // namespace Gringo
