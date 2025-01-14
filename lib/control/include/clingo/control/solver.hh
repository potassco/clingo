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
    none = 0,       //!< Select nothing.
    shown = 1,      //!< Select shown atoms and terms.
    atoms = 2,      //!< Select all atoms.
    terms = 4,      //!< Select all terms.
    theory = 8,     //!< Select symbols added by theory.
    all = 15,       //!< Select everything.
    complement = 16 //!< Select false instead of true atoms (Atoms/Shown) or terms (Terms).
};
void is_bit_set_enum(SymbolSelectFlags type);

enum class ModelType : uint8_t {
    model = 0,                //!< The model represents a stable model.
    brave_consequences = 1,   //!< The model represents a set of brave consequences.
    cautious_consequences = 2 //!< The model represents a set of cautious consequences.
};

enum class ConsequenceType : uint8_t {
    false_ = 0, //!< The literal is not a consequence.
    true_ = 1,  //!< The literal is a consequence.
    unknown = 2 //!< The literal might or might not be a consequence.
};

class SolveControl {
  public:
    virtual ~SolveControl() = default;

    [[nodiscard]] auto add_clause(Output::LitSpan lits) -> bool { return do_add_clause(lits); }

  private:
    [[nodiscard]] virtual auto do_add_clause(Output::LitSpan lits) -> bool = 0;
};

//! The model class.
class Model {
  public:
    virtual ~Model() = default;

    void symbols(SymbolSelectFlags type, SymbolVec &res) const { do_symbols(type, res); }
    [[nodiscard]] auto number() const -> uint64_t { return do_number(); }
    [[nodiscard]] auto type() const -> ModelType { return do_type(); }
    [[nodiscard]] auto contains(Symbol sym) const -> bool { return do_contains(sym); }
    [[nodiscard]] auto is_true(Output::lit_t lit) const -> bool { return do_is_true(lit); }
    [[nodiscard]] auto is_consequence(Output::lit_t lit) const -> ConsequenceType { return do_is_consequence(lit); }
    [[nodiscard]] auto costs() const -> std::span<Output::sum_t const> { return do_costs(); }
    [[nodiscard]] auto priorities() const -> std::span<Output::weight_t const> { return do_priorities(); }
    [[nodiscard]] auto optimality_proven() const -> bool { return do_optimality_proven(); }
    [[nodiscard]] auto thread_id() const -> Output::id_t { return do_thread_id(); }
    [[nodiscard]] auto context() -> SolveControl & { return do_control(); }

  private:
    virtual void do_symbols(SymbolSelectFlags type, SymbolVec &res) const = 0;
    [[nodiscard]] virtual auto do_number() const -> uint64_t = 0;
    [[nodiscard]] virtual auto do_type() const -> ModelType = 0;
    [[nodiscard]] virtual auto do_contains(Symbol sym) const -> bool = 0;
    [[nodiscard]] virtual auto do_is_true(Output::lit_t lit) const -> bool = 0;
    [[nodiscard]] virtual auto do_is_consequence(Output::lit_t lit) const -> ConsequenceType = 0;
    [[nodiscard]] virtual auto do_costs() const -> std::span<Output::sum_t const> = 0;
    [[nodiscard]] virtual auto do_priorities() const -> std::span<Output::weight_t const> = 0;
    [[nodiscard]] virtual auto do_optimality_proven() const -> bool = 0;
    [[nodiscard]] virtual auto do_thread_id() const -> Output::id_t = 0;
    [[nodiscard]] virtual auto do_control() -> SolveControl & = 0;
};

enum class SolveResult : uint8_t {
    empty = 0,
    satisfiable = 1,
    unsatisfiable = 2,
    exhausted = 4,
    interrupted = 8,
};
void is_bit_set_enum(SolveResult);

class SolveHandle {
  public:
    virtual ~SolveHandle() = default;

    auto get() -> SolveResult { return do_get(); }
    void cancel() { do_cancel(); }
    void resume() { do_resume(); }
    auto model() -> Model const & { return do_model(); }
    auto last() -> Model const & { return do_last(); }
    void core(Output::LitVec &lits) { do_core(lits); }
    auto wait(double timeout) -> bool { return do_wait(timeout); }

  private:
    virtual auto do_get() -> SolveResult = 0;
    virtual void do_cancel() = 0;
    virtual void do_resume() = 0;
    virtual auto do_model() -> Model const & = 0;
    virtual auto do_last() -> Model const & = 0;
    virtual void do_core(Output::LitVec &lits) = 0;
    virtual auto do_wait(double timeout) -> bool = 0;
};
using USolveHandle = std::unique_ptr<SolveHandle>;

//! The event handler interface.
class EventHandler {
  public:
    virtual ~EventHandler() = default;

    auto on_model(Model &m) -> bool { return do_on_model(m); }

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
    auto solve(EventHandler *eh) -> USolveHandle;

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
