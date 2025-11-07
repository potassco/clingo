#pragma once

#include <clingo/control/config.hh>
#include <clingo/control/grounder.hh>

#include <clingo/output/backend.hh>

#include <clasp/clasp_facade.h>
#include <clasp/cli/clasp_options.h>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

class Solver;

//! Script providing code execution, main, and callbacks.
//!
//! This interface should be implemented by custom scripts.
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

//! Stop condition for incremental mode.
enum class IStop : uint8_t {
    none,   //!< Do not consider solve result.
    sat,    //!< Stop when satisfiable.
    unsat,  //!< Stop when unsat.
    unknown //!< Stop when interrupted.
};

//! Enumeration of available application modes.
enum class AppMode : uint8_t {
    parse,   //!< Stop processing after parsing.
    rewrite, //!< Stop processing after rewriting.
    ground,  //!< Stop processing after grounding.
    solve    //!< Stop processing after solving.
};

//! Options for the solver.
struct SolverOptions {
    //! Operation mode of the solver.
    AppMode mode = AppMode::solve;
    //! The minimum number of incremental steps.
    size_t imin = 0;
    //! The maximum number of incremental steps.
    std::optional<size_t> imax = std::nullopt;
    //! The stop condition for the incremental mode.
    IStop istop = IStop::sat;
    //! Restrict to single shot-solving.
    bool single_shot = false;
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

//! Enumeration of available consequence types.
enum class ConsequenceType : uint8_t {
    false_ = 0, //!< The literal is not a consequence.
    true_ = 1,  //!< The literal is a consequence.
    unknown = 2 //!< The literal might or might not be a consequence.
};

//! Map from symbols to show term ids.
class TermBaseMap {
  public:
    //! The container storing the mapping (internal).
    using Map = Util::ordered_map<SharedSymbol, prg_id_t>;

    //! Add a new symbol to the map.
    //!
    //! The given function is used to generate an id if the symbol is not
    //! contained in the map yet.
    //!
    //! @param sym the symbol
    //! @param fun the id generator
    //! @return the existing or new id
    template <class F> auto add(Symbol sym, F &&fun) -> prg_id_t {
        auto [it, ins] = map_.emplace(sym, 0);
        if (ins) {
            it.value() = std::invoke(std::forward<F>(fun));
        }
        return it.value();
    }

    //! Add a symbol with the given id.
    //!
    //! Throws a runtime error if such an id already exists.
    //!
    //! @param sym the symbol
    //! @param id the id
    void add(Symbol sym, prg_id_t id) {
        if (!map_.emplace(sym, id).second) {
            throw std::runtime_error("collision of term ids");
        }
    }

    //! Get the id at the given index.
    //!
    //! @param i the index
    //! @return the term id
    [[nodiscard]] auto term_id(size_t i) const -> prg_id_t {
        if (i < size()) {
            return map_.nth(i).value();
        }
        throw std::range_error{"index out of range"};
    }

    //! Get the symbol at the given index.
    //!
    //! @param i the index
    //! @return the symbol
    [[nodiscard]] auto symbol(size_t i) const -> Symbol {
        if (i < size()) {
            return *map_.nth(i).key();
        }
        throw std::range_error{"index out of range"};
    }

    //! Get the index of the symbol.
    //!
    //! This resulting index can be used to obtain the symbol and its id. The
    //! index itself is not the id of the symbol.
    //!
    //! @param sym the symbol
    //! @return the index
    [[nodiscard]] auto index(Symbol sym) const -> size_t { return map_.find(sym) - map_.begin(); }

    //! Get the number of mapped symbols.
    //!
    //! @return the number of mapped symbols
    [[nodiscard]] auto size() const -> size_t { return map_.size(); }

    //! Get an iterator over the symbol id pairs in the map pointing to the
    //! beginning of the sequence.
    //!
    //! @return the iterator
    [[nodiscard]] auto begin() const -> Map::const_iterator { return map_.cbegin(); }

    //! Get an iterator over the symbol id pairs in the map pointing to the end
    //! of the sequence.
    //!
    //! @return the iterator
    [[nodiscard]] auto end() const -> Map::const_iterator { return map_.cend(); }

  private:
    Util::ordered_map<SharedSymbol, prg_id_t> map_;
};

//! Interface providing the necessary data to inspect atom, term, and theory bases.
class BaseView {
  public:
    //! The default destructor.
    virtual ~BaseView() = default;
    //! Get a reference to the underlying atom bases.
    [[nodiscard]] auto bases() const -> Ground::Bases const & { return do_bases(); }
    //! Get a reference to the underlying term bases.
    [[nodiscard]] auto term_base() const -> TermBaseMap const & { return do_term_base(); }
    //! Get a reference to the underlying facade.
    [[nodiscard]] auto clasp_program() const -> Clasp::Asp::LogicProgram const & { return do_clasp_program(); }
    //! Get a reference to the underlying facade.
    [[nodiscard]] auto clasp_theory() const -> Potassco::TheoryData const & { return clasp_program().theoryData(); }

  private:
    [[nodiscard]] virtual auto do_bases() const -> Ground::Bases const & = 0;
    [[nodiscard]] virtual auto do_term_base() const -> TermBaseMap const & = 0;
    [[nodiscard]] virtual auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & = 0;
};

//! Simple control class to add clauses while enumerating models.
class SolveControl : public BaseView {
  public:
    //! Add a clause over the given literal.
    void add_clause(PrgLitSpan lits) { do_add_clause(lits); }

  private:
    virtual void do_add_clause(PrgLitSpan lits) = 0;
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
    [[nodiscard]] auto is_true(prg_lit_t lit) const -> bool { return do_is_true(lit); }
    //! Check whether the given literal is a consequence.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is a consequence
    [[nodiscard]] auto is_consequence(prg_lit_t lit) const -> ConsequenceType { return do_is_consequence(lit); }
    //! Get the costs associated with a model.
    //!
    //! @return the costs
    [[nodiscard]] auto costs() const -> std::span<prg_sum_t const> { return do_costs(); }
    //! get the priorities of the costs.
    //!
    //! @return the priorities
    [[nodiscard]] auto priorities() const -> std::span<prg_weight_t const> { return do_priorities(); }
    //! Check if the model corresponds to an optimal solution.
    //!
    //! @return whether the model is optimal
    [[nodiscard]] auto optimality_proven() const -> bool { return do_optimality_proven(); }
    //! Get the solver/thread id the model was found in.
    //!
    //! @return the thread id
    [[nodiscard]] auto thread_id() const -> prg_id_t { return do_thread_id(); }
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
    [[nodiscard]] virtual auto do_is_true(prg_lit_t lit) const -> bool = 0;
    [[nodiscard]] virtual auto do_is_consequence(prg_lit_t lit) const -> ConsequenceType = 0;
    [[nodiscard]] virtual auto do_costs() const -> std::span<prg_sum_t const> = 0;
    [[nodiscard]] virtual auto do_priorities() const -> std::span<prg_weight_t const> = 0;
    [[nodiscard]] virtual auto do_optimality_proven() const -> bool = 0;
    [[nodiscard]] virtual auto do_thread_id() const -> prg_id_t = 0;
    [[nodiscard]] virtual auto do_control() -> SolveControl & = 0;
};
//! A unique pointer to a model.
using UModel = std::unique_ptr<Model>;

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
    auto core() -> PrgLitSpan { return do_core(); }
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
    virtual auto do_core() -> PrgLitSpan = 0;
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
    //! asynchronously to allow for thread synchronization.
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
    [[nodiscard]] auto add_atom(Symbol atom) -> prg_lit_t { return do_add_atom(atom); }
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
    virtual auto do_add_atom(Symbol atom) -> prg_lit_t = 0;
    virtual void do_close() = 0;
};
//! A unique pointer to a backend handle.
using UBackendHandle = std::unique_ptr<BackendHandle>;

//! The propagator interface.
class Propagator : public Potassco::AbstractPropagator, public Potassco::AbstractHeuristic {
  public:
    //! Can return false to not also register the propagator as a heuristic.
    [[nodiscard]] virtual auto hasHeuristic() const -> bool = 0;
};
//! A unique pointer to a propagator.
using UPropagator = std::unique_ptr<Propagator>;

//! This lock ensures that callbacks during solving are called in lock-step.
//!
//! This lock is required for scripting languages that do not support
//! multi-threading - like for example, the Lua language.
class CallbackLock {
  public:
    //! Acquire the lock.
    void lock() {
        if (mut_) {
            mut_->lock();
        }
    }
    //! Release the lock.
    void unlock() {
        if (mut_) {
            mut_->unlock();
        }
    }
    //! Enable or disable the lock.
    void enable(bool state) {
        if (!state) {
            mut_.reset();
        } else if (!mut_) {
            mut_.emplace();
            mut_->lock();
        }
    }

  private:
    std::optional<std::mutex> mut_;
};

//! RAII helper to unlock a mutex.
template <class M> class unlock_guard {
  public:
    //! Constructor unlocking the mutex.
    explicit unlock_guard(M &mut) : mut_{&mut} { mut_->unlock(); }
    //! Destructor re-locking the mutex.
    unlock_guard(const unlock_guard &) = delete;
    ~unlock_guard() { mut_->lock(); }
    auto operator=(const unlock_guard &) -> unlock_guard & = delete;

  private:
    M *mut_;
};

//! Helper to output symbols.
class SymbolTable {
  public:
    //! Initialize the table before output.
    void init(CppClingo::Control::BaseView &view, std::ostream &out);
    //! Output ids of shown terms in extended aspif format.
    void begin_step();
    //! Output atoms in extended aspif format.
    void end_step();
    //! Get the underlying output stream.
    auto out() -> std::ostream & { return *out_; }

  private:
    struct State {
        State() = default;
        size_t atom : 1 = 0;
        size_t term : 1 = 0;
        size_t index : (8 * sizeof(size_t)) - 2 = 0;
    };

    auto output(CppClingo::Symbol const &sym) -> State &;

    CppClingo::Control::BaseView *view_ = nullptr;
    std::ostream *out_ = nullptr;
    size_t ids_ = 0;
    std::vector<size_t> buf_;
    CppClingo::Util::unordered_map<CppClingo::SharedSymbol, State> done_;
};
//! A unique pointer to a symbol table.
using USymbolTable = std::unique_ptr<SymbolTable>;

//! Event emitted by a solver after the program is grounded.
// TODO: Simplify/Move on next output refactoring
class Grounded : public Clasp::Event {
  public:
    //! Construct a grounded event.
    explicit Grounded(Input::ProgramParamVec const &params)
        : Event(this, subsystem_load, verbosity_quiet), params(params) {}
    //! The program parts that have been grounded.
    std::span<Input::ProgramParamVec::value_type const> params;
};

class Grounder;

//! This callback interface provides grounding events.
//!
//! It extends the script callback interface with an callback that is called
//! when grounding has finished.
class GroundEventHandler : public Ground::ScriptCallback {
  public:
    //! Callback to inform that the grounding has finished.
    //!
    //! Note that this function is not called from the main thread when solving
    //! asynchronously to allow for thread synchronization.
    //!
    //! @param result the result of the ground call
    void finish(GroundResult result) noexcept { do_finish(result); }

  private:
    virtual void do_finish([[maybe_unused]] GroundResult result) noexcept {}
};

//! A unique pointer to a ground event handler.
using UGroundEventHandler = std::unique_ptr<GroundEventHandler>;

//! A handle for asynchronous grounding.
//!
//! The handle can be moved but not copied.
class GroundHandle {
  public:
    //! Start grounding with the given params and context.
    GroundHandle(Solver &solver, Input::ProgramParamVec params, UGroundEventHandler handler);
    //! Destroy the ground handle.
    //!
    //! If grounding has not stopped yet, this requests grounding to stop and
    //! joins the grounding thread.
    ~GroundHandle() noexcept;

    //! Prevent copying.
    GroundHandle(GroundHandle const &other) = delete;
    //! Enable moving.
    GroundHandle(GroundHandle &&other) noexcept;
    //! Enable moving.
    auto operator=(GroundHandle &&other) noexcept -> GroundHandle &;
    //! Prevent copying.
    auto operator=(GroundHandle const &other) -> GroundHandle & = delete;

    //! Wait for grounding to finish.
    //!
    //! If the timeout is greater than zero, the method blocks for at most
    //! timeout seconds. If the timeout is zero, the method does not block. If
    //! the timeout is negative, the method blocks until grounding has
    //! finished.
    //!
    //! The function returns true if grounding has finished, false otherwise.
    //!
    //! @param timeout The maximum time to wait in seconds.
    [[nodiscard]] auto wait(double timeout) -> bool;
    //! Get the result of grounding.
    //!
    //! Blocks until grounding has finished. Raises exceptions thrown during
    //! grounding.
    [[nodiscard]] auto get() -> GroundResult;
    //! Stop grounding.
    //!
    //! Blocks until grounding has finished.
    void cancel();

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

//! A grounder and solver for logic programs.
//!
//! Takes care of parsing, grounding, and solving.
class Solver : public BaseView {
  public:
    //! Create a solver object.
    Solver(Clasp::ClaspFacade &clasp, Clasp::Cli::ClaspCliConfig &config, Logger &log, SymbolStore &store,
           Scripts &scripts, Input::RewriteOptions ropts, SolverOptions sopts, FILE *out = stdout);

    //! Parse, ground, and solve a program.
    void main(std::span<std::string_view const> const &files);
    //! Ground and solve a program.
    void main();

    //! Join with the given program.
    void join(Input::UnprocessedProgram const &prg);
    //! Parse a program from the given string.
    void parse(std::string_view str);
    //! Parse the given files.
    void parse(std::span<std::string_view const> const &files);
    //! Parse with optional backends.
    void parse_with(std::function<void(ProgramBackend *, TheoryBackend *)> cb);

    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Get the const map.
    [[nodiscard]] auto const_map() -> Input::ConstMap const &;
    //! Ground the program asynchronously.
    [[nodiscard]] auto start_ground(ProgramParamVec params, UGroundEventHandler handler) -> GroundHandle;
    //! Ground the program.
    auto ground(ProgramParamVec const &params, Ground::ScriptCallback *ctx, Util::StopFlag *stop = nullptr)
        -> GroundResult;
    //! Solve the program.
    //!
    //! @param handler optional event handler
    //! @param assumptions assumptions for solving
    //! @param mode mode for solving
    //! @return solve handle to control the search
    auto solve(UEventHandler handler = {}, PrgLitSpan assumptions = {}, SolveMode mode = SolveMode::none)
        -> USolveHandle;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);

    //! Output the current program.
    void output_program(std::ostream &out);

    //! Map the given clasp model to the clingo one.
    auto map_model(Clasp::Model const &mdl) -> Model &;

    //! Get the output buffer.
    //!
    //! If the control object has been constructed without a null output FILE,
    //! this buffer contains the output of the textoutput.
    [[nodiscard]] auto buf() -> Util::OutputBuffer & { return buf_; };

    //! Get a handle that provides access to the backend to add atoms and rules.
    //!
    //! While the handle is alive, the solver object should not be accessed.
    //! It can be seen like an active ground call.
    [[nodiscard]] auto backend() -> UBackendHandle;

    //! Get a pointer to the underlying clasp facade.
    [[nodiscard]] auto clasp_facade() -> Clasp::ClaspFacade & { return *clasp_; }

    //! Get a pointer to the underlying clasp facade.
    [[nodiscard]] auto clasp_facade() const -> Clasp::ClaspFacade const & { return *clasp_; }

    //! Only non-null in solving mode.
    [[nodiscard]] auto config() -> ClingoConfig & { return config_; }

    //! Get the statistics.
    [[nodiscard]] auto clasp_stats() -> Potassco::AbstractStatistics const & {
        auto const *stats = clasp_->getStats();
        return stats != nullptr ? *stats : throw std::runtime_error("not in solving mode");
    }

    //! Register the given propagator with the control object.
    //!
    //! @param propagator the propagator
    void register_propagator(UPropagator propagator);

    //! Get the solvers callback lock.
    auto get_lock() -> CallbackLock & { return lock_; }

    //! Block execution of the main function in scripts.
    void block_main(bool block) { block_main_ = block; }

    //! Get the application mode.
    [[nodiscard]] auto get_mode() const -> AppMode { return opts_.mode; }

    //! Get user data for C integration.
    auto user_data() -> void *& { return data_; }

    //! Interrupt the running (or next search).
    void interrupt() noexcept;

    //! Get the program parts to ground.
    [[nodiscard]] auto get_parts() -> std::optional<Input::StmParts> const & { return grd_.get_parts(); }

    //! Set the program parts to ground.
    void set_parts(std::optional<Input::StmParts> parts) { grd_.set_parts(std::move(parts)); }
    //! Set the program parts to ground.
    void set_parts(Input::ProgramParamVec parts) {
        auto pos = Position{*grd_.store().string("<cmd>"), 1, 1};
        auto loc = Location{pos, pos};
        grd_.set_parts(std::make_optional<Input::StmParts>(loc, Input::Precedence::override_, std::move(parts)));
    }

    //! Show the given signature.
    void show(Input::SharedSig const &sig) { grd_.show(sig); }

    //! Get the symbol table.
    auto sym_tab() -> SymbolTable & {
        if (!sym_tab_) {
            sym_tab_ = std::make_unique<SymbolTable>();
        }
        return *sym_tab_;
    }

    //! Print per step summaries.
    //!
    //! Currently, outputs profiling data if enabled.
    auto print_summary(bool final) { grd_.print_summary(final); }

    //! Accept a visitor for the profile nodes.
    void accept(Ground::ProfileNode::Visitor const &visit) const { grd_.accept(visit); }

  private:
    class ProgramBackendAdapter;

    //! States for step transitions.
    //!
    //! The initial state is special and is only entered initially. The prepared
    //! state is entered after some API functions. For example, when
    //! information about literals is requested that needs help of the solver.
    //! The grounded and solved states are entered after grounding and
    //! solving, respectively.
    enum class State : uint8_t {
        initial,  //< initial step
        grounded, //< step has been grounded
        prepared, //< step is prepared for solving
        solved,   //< step has been solved
    };

    //! Create output according to mode.
    //!
    //! This function additionally initializes required members, for example,
    //! the backend for the clasp output.
    //!
    //! @param mode the configured output mode
    //! @return the resulting output
    auto make_output_(SymbolStore &store, AppMode mode) -> UOutputStm;

    //! Prepare the solver for grounding.
    void prepare_();

    //! Simplify the grounder's domain with the solvers assignment.
    void simplify_();

    //! Enable program updates for incremental solving.
    void enable_updates_();

    [[nodiscard]] auto do_bases() const -> Ground::Bases const & override { return grd_.base(); }

    [[nodiscard]] auto do_term_base() const -> TermBaseMap const & override { return terms_; }

    [[nodiscard]] auto do_clasp_program() const -> Clasp::Asp::LogicProgram const & override {
        return clasp_->asp() != nullptr ? *clasp_->asp() : throw std::runtime_error("not in solving mode");
    }

    void incmode_();

    CallbackLock lock_;
    std::vector<UPropagator> propagators_;
    TermBaseMap terms_;
    Clasp::ClaspFacade *clasp_;
    ClingoConfig config_;
    Util::OutputBuffer buf_;
    UProgramBackend backend_;
    std::unique_ptr<Output::TheoryData> theory_;
    UOutputStm out_;
    UModel mdl_;
    USymbolTable sym_tab_;
    Grounder grd_;
    Scripts *scripts_;
    State state_ = State::initial;
    SolverOptions opts_;
    BuiltinIncludes includes_ = BuiltinIncludes::empty;
    void *data_ = nullptr;
    bool block_main_ = false;
};

//! @}

} // namespace CppClingo::Control
