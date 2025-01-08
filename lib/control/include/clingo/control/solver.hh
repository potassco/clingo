#pragma once

#include <clingo/control/grounder.hh>

#include <clingo/output/backend.hh>

#include <clasp/clasp_facade.h>

namespace Clingo::Control {

//! @addtogroup control
//! @{

class Solver;

//! Script providing code execution, main, and callbacks.
//!
//! This interface should be implemend by custom scripts.
class Script : public Ground::ScriptCallback {
  public:
    //! Run the main function.
    void main(Solver &slv) { do_main(slv); }
    //! Execute the given code.
    void exec(std::string_view code) { do_exec(code); }

  private:
    virtual void do_exec(std::string_view code) = 0;
    virtual void do_main(Solver &slv) = 0;
};
//! A unique pointer to a script.
using UScript = std::unique_ptr<Script>;

//! Helper to run specific code and callbacks.
//!
//! Named scripts can be registered. Code, main, and callback execution and is
//! dispatched to registered scripts.
class Scripts : public Ground::ScriptCallback, public Ground::ScriptExec {
  public:
    //! Register the given script.
    void register_script(std::string_view name, UScript script);
    //! Run the main function.
    void main(Solver &slv);

  private:
    void do_exec(Location const &loc, Logger &log, std::string_view name, std::string_view code) override;
    auto do_callable(std::string_view name, size_t args) -> bool override;
    void do_call(Location const &loc, std::string_view name, SymbolSpan args, SymbolVec &out) override;

    std::vector<std::pair<std::string, UScript>> scripts_;
};

//! Enumeration of available application modes.
enum class AppMode : uint8_t {
    parse,   //!< Stop processing after parsing.
    rewrite, //!< Stop processing after rewriting.
    ground,  //!< Stop processing after grounding.
    solve    //!< Stop processing after solving.
};

enum class SymbolSelectFlags : uint8_t {
    None = 0,       //!< Select nothing.
    Shown = 1,      //!< Select shown atoms and terms.
    Atoms = 2,      //!< Select all atoms.
    Terms = 4,      //!< Select all terms.
    Theory = 8,     //!< Select symbols added by theory.
    All = 15,       //!< Select everything.
    Complement = 16 //!< Select false instead of true atoms (Atoms/Shown) or terms (Terms).
};
void is_bit_set_enum(SymbolSelectFlags type);

//! The model class.
class Model {
  public:
    class Impl;
    explicit Model(Impl &impl) : impl_{&impl} {}

    void symbols(SymbolSelectFlags type, SymbolVec &res);

  private:
    Impl *impl_;
};

//! The event handler interface.
class EventHandler {
  public:
    auto on_model(Model &m) -> bool { return do_on_model(m); }
    virtual ~EventHandler() noexcept = default;

  private:
    virtual auto do_on_model([[maybe_unused]] Model &m) -> bool { return true; }
};

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver {
  public:
    //! Create a grounder object.
    Solver(Logger &log, SymbolStore &store, Scripts &scripts, Input::RewriteOptions opts, AppMode mode,
           FILE *out = stdout);

    //! Parse, ground, and solve a program.
    void main(std::span<std::string_view const> const &files,
              std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params);
    //! Ground and solve a program.
    void main(std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &params);

    //! Parse a program from the given string.
    void join(Input::UnprocessedProgram const &prg);
    //! Parse a program from the given string.
    void parse(std::string_view str);
    //! Parse the given files.
    void parse(std::span<std::string_view const> const &files);
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Ground the program.
    void ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *ctx);
    //! Solve the program.
    //!
    //! @todo Incomplete and just to get started.
    void solve(EventHandler *eh);

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
    //! States for step transitions.
    //!
    //! The updated state is special and is only entered intially. The prepared
    //! state is entered after some API functions. For example, when
    //! information about literals is requested that needs help of the solver.
    //! The grounded and solved states are entederd after grounding and
    //! solving.
    enum class State : uint8_t {
        updated,  //< intial step
        grounded, //< step has been grounded
        prepared, //< step is prepared for solving
        solved,   //< step has been solved
    };

    //! Create output according to mode.
    //!
    //! This function additionally initizalizes required members, for example,
    //! the backend for clasp output.
    //!
    //! @param mode the configured output mode
    //! @return the resulting output
    auto make_output_(SymbolStore &store, AppMode mode) -> UOutputStm;

    Clasp::ClaspConfig cfg_;
    Clasp::ClaspFacade clasp_;
    Util::OutputBuffer buf_;
    Output::UBackend backend_;
    UOutputStm out_;
    Grounder grd_;
    Scripts *scripts_;
    State state_ = State::updated;
    AppMode mode_;
};

//! @}

} // namespace Clingo::Control
