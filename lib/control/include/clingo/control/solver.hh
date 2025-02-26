#pragma once

#include <clingo/control/grounder.hh>

#include <clingo/output/backend.hh>

#include <clasp/clasp_facade.h>
#include <clasp/cli/clasp_options.h>
#include <clasp/clingo.h>

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

//! A bit set of symbol selection flags.
enum class SymbolSelectFlags : uint8_t {
    none = 0,   //!< Select nothing.
    shown = 1,  //!< Select shown atoms and terms.
    atoms = 2,  //!< Select all atoms.
    terms = 4,  //!< Select all terms.
    theory = 8, //!< Select symbols added by theory.
    all = 15,   //!< Select everything.
};
//! Enable bit set operations.
CLINGO_ENABLE_BITSET_ENUM(SymbolSelectFlags);

//! Enumeration of available model flags.
enum class ModelType : uint8_t {
    model = 0,                //!< The model represents a stable model.
    brave_consequences = 1,   //!< The model represents a set of brave consequences.
    cautious_consequences = 2 //!< The model represents a set of cautious consequences.
};

//! Enumeration of availale consequence types.
enum class ConsequenceType : uint8_t {
    false_ = 0, //!< The literal is not a consequence.
    true_ = 1,  //!< The literal is a consequence.
    unknown = 2 //!< The literal might or might not be a consequence.
};

//! Interface providing the necessary data to inspect atom, term, and theory bases.
class BaseView {
  public:
    //! The default destructor.
    virtual ~BaseView() = default;
    //! Get a reference to the underlying atom/term bases.
    [[nodiscard]] auto bases() const -> Ground::Bases const & { return do_bases(); }
    //! Get a reference to the underlying facade.
    [[nodiscard]] auto clasp_program() const -> Clasp::Asp::LogicProgram const & { return do_clasp_program(); }
    //! Get a reference to the underlying facade.
    [[nodiscard]] auto clasp_theory() const -> Potassco::TheoryData const & { return clasp_program().theoryData(); }

  private:
    [[nodiscard]] virtual auto do_bases() const -> Ground::Bases const & = 0;
    [[nodiscard]] virtual auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & = 0;
};

//! Simple control class to add clauses while enumerating models.
class SolveControl : public BaseView {
  public:
    //! Add a clause over the given literal.
    void add_clause(Output::LitSpan lits) { do_add_clause(lits); }

  private:
    virtual void do_add_clause(Output::LitSpan lits) = 0;
};

//! The model class.
class Model {
  public:
    virtual ~Model() = default;

    //! Get the selected symbols in the model.
    //!
    //! @param type which symbols to select
    //! @param res a vector to store the symbols
    void symbols(SymbolSelectFlags type, SymbolVec &res) const { do_symbols(type, res); }
    //! Get the running number of the model.
    //!
    //! @return the number
    [[nodiscard]] auto number() const -> uint64_t { return do_number(); }
    //! Get the type of the model.
    //!
    //! @return the type
    [[nodiscard]] auto type() const -> ModelType { return do_type(); }
    //! Check if the model contains a (symbolic) atom.
    //!
    //! @param sym The symbol representing the atom.
    //! @return whether the atom is contained
    [[nodiscard]] auto contains(Symbol sym) const -> bool { return do_contains(sym); }
    //! Check if a program literal is true in a model.
    //!
    //! @param lit the program literal
    //! @return whether the literal is true
    [[nodiscard]] auto is_true(Output::lit_t lit) const -> bool { return do_is_true(lit); }
    //! Check whether the given literal is a consequence.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is a consequence
    [[nodiscard]] auto is_consequence(Output::lit_t lit) const -> ConsequenceType { return do_is_consequence(lit); }
    //! Get the costs associated with a model.
    //!
    //! @return the costs
    [[nodiscard]] auto costs() const -> std::span<Output::sum_t const> { return do_costs(); }
    //! get the priorites of the costs.
    //!
    //! @return the priorities
    [[nodiscard]] auto priorities() const -> std::span<Output::weight_t const> { return do_priorities(); }
    //! Check if the model coresponds to an optimal solution.
    //!
    //! @return whether the model is optimal
    [[nodiscard]] auto optimality_proven() const -> bool { return do_optimality_proven(); }
    //! Get the solver/thread id the model was found in.
    //!
    //! @return the thread id
    [[nodiscard]] auto thread_id() const -> Output::id_t { return do_thread_id(); }
    //! Get the context object to control the search.
    //!
    //! @return the context object
    [[nodiscard]] auto context() -> SolveControl & { return do_control(); }

    //! Extend the model with the given symbols.
    virtual void extend(SymbolSpan symbols) { do_extend(symbols); }

  private:
    virtual void do_symbols(SymbolSelectFlags type, SymbolVec &res) const = 0;
    [[nodiscard]] virtual auto do_number() const -> uint64_t = 0;
    [[nodiscard]] virtual auto do_type() const -> ModelType = 0;
    [[nodiscard]] virtual auto do_contains(Symbol sym) const -> bool = 0;
    virtual void do_extend(SymbolSpan symbols) = 0;
    [[nodiscard]] virtual auto do_is_true(Output::lit_t lit) const -> bool = 0;
    [[nodiscard]] virtual auto do_is_consequence(Output::lit_t lit) const -> ConsequenceType = 0;
    [[nodiscard]] virtual auto do_costs() const -> std::span<Output::sum_t const> = 0;
    [[nodiscard]] virtual auto do_priorities() const -> std::span<Output::weight_t const> = 0;
    [[nodiscard]] virtual auto do_optimality_proven() const -> bool = 0;
    [[nodiscard]] virtual auto do_thread_id() const -> Output::id_t = 0;
    [[nodiscard]] virtual auto do_control() -> SolveControl & = 0;
};

//! The solve result.
//!
//! This is a bitset. For example, a model can be both satisfiable and
//! interrupted.
enum class SolveResult : uint8_t {
    empty = 0,         //!< No flags set.
    satisfiable = 1,   //!< The search produced at least one model.
    unsatisfiable = 2, //!< The search finished and no model was produced.
    exhausted = 4,     //!< The search has been exhausted.
    interrupted = 8,   //!< The search has been interrupted.
};
//! Enable bit set operations.
CLINGO_ENABLE_BITSET_ENUM(SolveResult);

//! A handle to control a running search.
class SolveHandle {
  public:
    //! The default destructor.
    virtual ~SolveHandle() = default;

    //! Get the result of a search.
    //!
    //! This call blocks until search has completed.
    //!
    //! @return the solve result
    auto get() -> SolveResult { return do_get(); }
    //! Cancel the current search.
    //!
    //! This call blocks until search has stopped.
    void cancel() { do_cancel(); }
    //! Resume search after a model has been found to start search for the next
    //! one.
    //!
    //! To ease writing while loops, this function can also be called
    //! initially.
    void resume() { do_resume(); }
    //! Get the current model or a nullptr if there is none.
    //!
    //! The function blocks until a model is ready or the search space has been
    //! exhausted.
    //!
    //! The function returns null if the search space has been exhausted or the
    //! search was not started in yield mode.
    //!
    //! @return the model or null
    auto model() -> Model const * { return do_model(); }
    //! Get the last model after the search has finished.
    //!
    //! Returns null if the problem is unsatisfiable.
    //!
    //! @return the model or null
    auto last() -> Model const * { return do_last(); }
    //! Get a subset of the assumptions that made the problem unsatisfiable.
    //!
    //! The subset is called an unsatisfiable core and is not necessarily
    //! subset minimal.
    //!
    //! @return the core
    auto core() -> Output::LitSpan { return do_core(); }
    //! Wait for the given amount of time or until the next result is ready.
    //!
    //! The next result is either a model (if yielding was enabled) or a solve
    //! result.
    //!
    //! The function blocks for at most the given amount of time and returns
    //! whether the next result is ready. If the timeout is zero, it can be
    //! used for polling for a result. If the timeout is negative, it blocks
    //! until the next result is ready.
    auto wait(double timeout) -> bool { return do_wait(timeout); }

  private:
    virtual auto do_get() -> SolveResult = 0;
    virtual void do_cancel() = 0;
    virtual void do_resume() = 0;
    virtual auto do_model() -> Model const * = 0;
    virtual auto do_last() -> Model const * = 0;
    virtual auto do_core() -> Output::LitSpan = 0;
    virtual auto do_wait(double timeout) -> bool = 0;
};
//! A unique pointer for a solve handle.
using USolveHandle = std::unique_ptr<SolveHandle>;

//! The event handler interface.
class EventHandler {
  public:
    virtual ~EventHandler() = default;

    //! Callback to intercept models.
    //!
    //! The function can return false to stop search.
    //!
    //! @param mdl the model
    //! @return whether to continue or stop search
    auto on_model(Model &mdl) -> bool { return do_on_model(mdl); }
    //! Callback to update statistics.
    //!
    //! Applications can add user defined statistics here. Statistics should be
    //! added under keys "user_step" and "user_accu" to appear in the text
    //! output.
    //!
    //! @param stats a handle to the statistics
    void on_stats(Potassco::AbstractStatistics &stats) { do_on_stats(stats); }
    //! Callback to intercept lower bounds.
    //!
    //! Whenever a (sub)problem is unsatisfiable during optimization, the
    //! current bound is reported as a lower bound.
    //!
    //! @param bound the lower bound
    void on_unsat(Clasp::SumView bound) { do_on_unsat(bound); }
    //! The unsatisfiable core of the current problem.
    //!
    //! @param core the core
    //! @see SolveHandle::core()
    void on_core(Potassco::LitSpan core) { do_on_core(core); }
    //! Callback to inform that the search has finished.
    //!
    //! Note that this function is not called from the main thread when solving
    //! asynchronously to allow for thread synchonization.
    //!
    //! @param result the solve result
    void on_finish(SolveResult result) { do_on_finish(result); }

  private:
    virtual auto do_on_model([[maybe_unused]] Model &mdl) -> bool { return true; }
    virtual void do_on_stats([[maybe_unused]] Potassco::AbstractStatistics &stats) {}
    virtual void do_on_unsat([[maybe_unused]] Clasp::SumView bound) {}
    virtual void do_on_core([[maybe_unused]] Potassco::LitSpan core) {}
    virtual void do_on_finish([[maybe_unused]] SolveResult result) {}
};
//! A unique pointer for an event handler.
using UEventHandler = std::unique_ptr<EventHandler>;

//! The available solve modes.
//!
//! This is a bitset.
enum class SolveMode : uint8_t {
    none = 0,  //!< Default synchronous callback-based solving.
    async = 1, //!< Solve asynchronously in background threads.
    yield = 2, //!< Yield models while solving via `SolveHandle::model()`.
};
//! Enable bit set operations.
CLINGO_ENABLE_BITSET_ENUM(SolveMode);

//! This lock ensures that callbacks during solving are called in lock-step.
//!
//! To avoid locking, the lock only engages if there are propagators that
//! require locking and search proceeds with more than one thread.
//!
//! This lock is required for scripting languages that do not support
//! mulit-threading - like for example, the Lua language.
//! @todo: Check if the current implementation covers all use cases.
class PropagatorLock : public Clasp::ClingoPropagatorLock {
  public:
    //! Acquire the lock.
    void lock() override {
        if (mut_) {
            mut_->lock();
        }
    }
    //! Release the lock.
    void unlock() override {
        if (mut_) {
            mut_->unlock();
        }
    }
    //! Add a propagator with the required locking.
    //!
    //! @param seq whether the propagator requires lockning
    //! @return a reference to self
    auto add(bool seq) -> PropagatorLock * {
        if (seq) {
            ++seq_;
            return this;
        }
        return nullptr;
    }
    //! Initialize the internal lock for the given number of threads.
    //!
    //! @param threads the number of threads
    void init(size_t threads) {
        if (threads < 2 || seq_ == 0) {
            mut_.reset();
        } else if (!mut_) {
            mut_.emplace();
        }
    }

  private:
    std::optional<std::mutex> mut_;
    size_t seq_ = 0;
};

//! Object to provide access to the backend.
//!
//! This backend is intended to be used in the C API to extend the ground
//! program representation.
//!
//! Logically, adding a block of rules via the backend can be seen like a call
//! to ground. In fact, ground is called with an empty part list when the
//! handle is destroyed to run the required finalization logic (grounding
//! currently does not involve dedicated initialization).
//!
//! Note that symbols added via the backend are not added to the domains of
//! the grounder. This has to be done using add_atom().
class BackendHandle {
  public:
    //! Get the logic program.
    //!
    //! @return the program
    [[nodiscard]] auto program() -> Clasp::Asp::LogicProgram & { return do_program(); }
    //! Get the theory.
    //!
    //! @return the theory
    [[nodiscard]] auto theory() -> Output::TheoryData & { return do_theory(); }
    //! The symbol store.
    //!
    //! @return the symbol store
    [[nodiscard]] auto store() -> SymbolStore & { return do_store(); }
    //! Add a literal for the given symbol.
    //!
    //! Returns literals of existing symbolic atoms are introduces a new one.
    //!
    //! @param atom the symbol
    //! @return the literal of the symbolic atom
    [[nodiscard]] auto add_atom(Symbol atom) -> Output::lit_t { return do_add_atom(atom); }
    //! Close the handle.
    //!
    //! This functions must be called before continuing to use the associated
    //! solver.
    void close() { do_close(); }
    //! Destroy the handle.
    virtual ~BackendHandle() = default;

  private:
    virtual auto do_program() -> Clasp::Asp::LogicProgram & = 0;
    virtual auto do_theory() -> Output::TheoryData & = 0;
    virtual auto do_store() -> SymbolStore & = 0;
    virtual auto do_add_atom(Symbol atom) -> Output::lit_t = 0;
    virtual void do_close() = 0;
};
//! A unique pointer to a backend handle.
using UBackendHandle = std::unique_ptr<BackendHandle>;

//! The propagator interface.
class Propagator : public Potassco::AbstractPropagator, public Potassco::AbstractHeuristic {
  public:
    //! Called before solving to initialize the propagator.
    virtual void init(Clingo::Control::Solver &slv, Clasp::ClingoPropagatorInit &init) = 0;
    //! Can return false to not also register the propagator as a heuristic.
    [[nodiscard]] virtual auto hasHeuristic() const -> bool = 0;
};
//! A unique pointer to a propagator.
using UPropagator = std::unique_ptr<Propagator>;

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver : public BaseView {
  public:
    //! Create a solver object.
    Solver(Clasp::ClaspFacade &clasp, Clasp::Cli::ClaspCliConfig &config, Logger &log, SymbolStore &store,
           Scripts &scripts, Input::RewriteOptions opts, AppMode mode, FILE *out = stdout);

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
    //! @param handler optional event handler
    //! @param assumptions assumptions for solving
    //! @param mode mode for solving
    //! @return solve handle to control the search
    auto solve(UEventHandler handler = {}, Output::LitSpan assumptions = {}, SolveMode mode = SolveMode::none)
        -> USolveHandle;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);

    //! Output the current program.
    void output_program(std::ostream &out);

    //! Get the output buffer.
    //!
    //! If the control object has been constructed without a null output FILE,
    //! this buffer contains the output of the textoutput.
    [[nodiscard]] auto buf() -> Util::OutputBuffer & { return buf_; };

    //! Get a handle that provides access to the backend to add atoms and rules.
    //!
    //! While the handle is alive, the solver object should not be accessed.
    //! It can be seen like an active ground ground call.
    [[nodiscard]] auto backend() -> UBackendHandle;

    //! Get a pointer to the underlying clasp facade.
    [[nodiscard]] auto clasp_facade() -> Clasp::ClaspFacade & { return *clasp_; }

    //! Get a pointer to the underlying clasp facade.
    [[nodiscard]] auto clasp_facade() const -> Clasp::ClaspFacade const & { return *clasp_; }

    //! Only non-null in solving mode.
    [[nodiscard]] auto clasp_config() -> Clasp::Cli::ClaspCliConfig & {
        return clasp_config_ != nullptr ? *clasp_config_ : throw std::runtime_error("not in solving mode");
    }

    //! Get the statsistics.
    [[nodiscard]] auto clasp_stats() -> Potassco::AbstractStatistics const & {
        auto const *stats = clasp_->getStats();
        return stats != nullptr ? *stats : throw std::runtime_error("not in solving mode");
    }

    //! Register the given propagator with the control object.
    //!
    //! @param propagator the propagator
    void register_propagator(UPropagator propagator);

  private:
    //! States for step transitions.
    //!
    //! The initial state is special and is only entered intially. The prepared
    //! state is entered after some API functions. For example, when
    //! information about literals is requested that needs help of the solver.
    //! The grounded and solved states are entederd after grounding and
    //! solving, respectively.
    enum class State : uint8_t {
        initial,  //< intial step
        grounded, //< step has been grounded
        prepared, //< step is prepared for solving
        solved,   //< step has been solved
    };

    //! Create output according to mode.
    //!
    //! This function additionally initizalizes required members, for example,
    //! the backend for the clasp output.
    //!
    //! @param mode the configured output mode
    //! @return the resulting output
    auto make_output_(SymbolStore &store, AppMode mode) -> UOutputStm;

    //! Prepare the solver for grounding.
    void prepare_();

    [[nodiscard]] auto do_bases() const -> Ground::Bases const & override { return grd_.base(); }

    [[nodiscard]] auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & override {
        return clasp_->asp() != nullptr ? *clasp_->asp() : throw std::runtime_error("not in solving mode");
    }

    PropagatorLock lock_;
    Clasp::ClaspFacade *clasp_;
    Clasp::Cli::ClaspCliConfig *clasp_config_;
    Util::OutputBuffer buf_;
    Output::UProgramBackend backend_;
    std::unique_ptr<Output::TheoryData> theory_;
    UOutputStm out_;
    Grounder grd_;
    Scripts *scripts_;
    State state_ = State::initial;
    AppMode mode_;
};

//! @}

} // namespace Clingo::Control
