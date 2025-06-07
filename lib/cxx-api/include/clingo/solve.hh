#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>
#include <clingo/stats.hh>

#include <clingo/solve.h>

namespace Clingo {

//! @addtogroup cpp_solve Solving
//! Intercept models and interact with a running search.
//! @{

//! Class to capture the result of solve calls.
class SolveResult {
  public:
    //! Construct the solve result from its C representation.
    //!
    //! @param res the C representation of the solve result
    explicit SolveResult(clingo_solve_result_bitset_t res) : res_{res} {}

    //! Check if the result is satisfiable.
    //!
    //! @return whether the result is satisfiable
    [[nodiscard]] auto satisfiable() const -> bool { return (res_ & clingo_solve_result_satisfiable) != 0; }

    //! Check if the result is unsatisfiable.
    //!
    //! @return whether the result is unsatisfiable
    [[nodiscard]] auto unsatisfiable() const -> bool { return (res_ & clingo_solve_result_unsatisfiable) != 0; }

    //! Check if the result is unknown.
    //!
    //! @return whether the result is unknown
    [[nodiscard]] auto unknown() const -> bool {
        return (res_ & (clingo_solve_result_unsatisfiable | clingo_solve_result_satisfiable)) != 0;
    }

    //! Check if the search space was exhausted.
    //!
    //! @return whether the search space was exhausted
    [[nodiscard]] auto exhausted() const -> bool { return (res_ & clingo_solve_result_exhausted) != 0; }

    //! Check if the search was interrupted.
    //!
    //! @return whether the search was interrupted
    [[nodiscard]] auto interrupted() const -> bool { return (res_ & clingo_solve_result_interrupted) != 0; }

    //! Convert the solve result to a string representation.
    //!
    //! @return the string representation of the solve result
    [[nodiscard]] auto to_string() const -> std::string_view {
        if (satisfiable()) {
            return "SAT";
        }
        if (unsatisfiable()) {
            return "UNSAT";
        }
        return "UNKNOWN";
    }

  private:
    clingo_solve_result_bitset_t res_;
};

//! Class to add clauses to a running search.
class SolveControl {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! @param ctl the C representation of the solve control object
    explicit SolveControl(clingo_solve_control_t *ctl) : ctl_{ctl} {}

    //! Get base associated with the solve control.
    //!
    //! @return the base associated with the solve control
    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_solve_control_base>(ctl_)}; }

    //! Add clause to the running search.
    //!
    //! @param lits the literals of the clause to add
    void add_clause(ProgramLiteralSpan lits) const {
        Detail::handle_error(clingo_solve_control_add_clause(ctl_, lits.data(), lits.size()));
    }

    //! Add clause to the running search.
    //!
    //! This is equivalent to calling Clingo::SolveControl::add_clause() with
    //! the negated literals.
    //!
    //! @param lits the literals of the nogood to add
    void add_nogood(ProgramLiteralSpan lits) const {
        add_clause(Detail::transform(lits, [](auto const &lit) { return -lit; }));
    }

  private:
    clingo_solve_control_t *ctl_;
};

//! Enumeration of the model types.
enum class ModelType : clingo_model_type_t {
    stable_model = clingo_model_type_stable_model,             //!< The model represents a stable model.
    brave_consequences = clingo_model_type_brave_consequences, //!< The model represents a set of brave consequences.
    cautious_consequences =
        clingo_model_type_cautious_consequences //!< The model represents a set of cautious consequences.
};

//! Enumeration of bit flags to select symbols in models.
enum class ShowFlags : clingo_show_type_bitset_t {
    none = 0,                         //!< Show nothing.
    shown = clingo_show_type_shown,   //!< Select shown atoms and terms.
    atoms = clingo_show_type_atoms,   //!< Select all atoms.
    terms = clingo_show_type_terms,   //!< Select all terms.
    theory = clingo_show_type_theory, //!< Select symbols added by theory.
};
CLINGO_ENABLE_BITSET_ENUM(ShowFlags);

//! Class to provide an immutable view of a model.
class ConstModel {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! For internal use.
    //!
    //! @param mdl the C representation of the model
    explicit ConstModel(clingo_model_t const *mdl) : mdl_{mdl} {}

    //! Cast the model to its C representation.
    //!
    //! @pram x the model to cast
    //! @return the C representation of the model
    friend auto c_cast(ConstModel const &x) -> clingo_model_t const * { return x.mdl_; }

    //! Get the symbols of the model.
    //!
    //! @param flags the flags to select which symbols to return
    //! @return the symbols of the model
    [[nodiscard]] auto symbols(ShowFlags flags = ShowFlags::shown) const -> SymbolVector {
        auto res = SymbolVector{};
        Detail::handle_error(clingo_model_symbols(mdl_, static_cast<clingo_show_type_bitset_t>(flags), &sym_cb_, &res));
        return res;
    }

    //! Check if the model contains a specific atom.
    //!
    //! @param atom the atom to check
    //! @return whether the model contains the atom
    [[nodiscard]] auto contains(Symbol const &atom) const -> bool {
        return Detail::call<clingo_model_contains>(mdl_, c_cast(atom));
    }

    //! Get the type of the model.
    //!
    //! @return the type of the model
    [[nodiscard]] auto type() const -> ModelType {
        return static_cast<ModelType>(Detail::call<clingo_model_type>(mdl_));
    }

    //! Get the running number of the model.
    //!
    //! @return the running number of the model
    [[nodiscard]] auto number() const -> uint64_t { return Detail::call<clingo_model_number>(mdl_); }

    //! Check whether the given literal is true in the model.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is true in the model
    [[nodiscard]] auto is_true(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_model_is_true>(mdl_, lit);
    }

    //! Check whether the given literal is consequence of the model.
    //!
    //! The function return std::nullopt if it is not known whether the literal
    //! is a consequence. Otherwise, it returns true if the literal is a
    //! consequence and false if it is not.
    //!
    //! @param lit the literal to check
    //! @return whether the literal is a consequence
    [[nodiscard]] auto is_consequence(ProgramLiteral lit) const -> std::optional<bool> {
        auto res = Detail::call<clingo_model_is_consequence>(mdl_, lit);
        if (res != clingo_consequence_unknown) {
            return res == clingo_consequence_true;
        }
        return std::nullopt;
    }

    //! Get the cost of the model.
    //!
    //! Each priority of a minimize constraint is associated with a cost.
    //!
    //! @return the cost of the model
    [[nodiscard]] auto cost() const -> SumSpan {
        int64_t const *costs = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_model_cost(mdl_, &costs, &size));
        return {costs, size};
    }

    //! Get the priorities of the costs of a model.
    //!
    //! @return the priorities of the costs
    [[nodiscard]] auto priorities() const -> WeightSpan {
        clingo_weight_t const *prios = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_model_priority(mdl_, &prios, &size));
        return {prios, size};
    }

    //! Check whether the model is proven to be optimal.
    //!
    //! Depending on the reasoning mode and the context the model is access,
    //! this function might return false even though the model is optimal.
    //!
    //! @return whether the model is proven to be optimal
    [[nodiscard]] auto optimality_proven() const -> bool { return Detail::call<clingo_model_optimality_proven>(mdl_); }

    //! Get the thread id of the solver that found the model.
    //!
    //! @return the thread id of the solver that found the model
    [[nodiscard]] auto thread_id() const -> ProgramId { return Detail::call<clingo_model_thread_id>(mdl_); }

    //! Convert the model to a string representation.
    //!
    //! @return the string representation of the model
    [[nodiscard]] auto to_string() const -> std::string {
        auto res = std::string{};
        auto comma = false;
        for (auto const &sym : symbols(ShowFlags::shown)) {
            res += comma ? ", " : "";
            res += sym.to_string();
            comma = true;
        }
        return res;
    }

  private:
    static auto sym_cb_(clingo_symbol_t const *symbols, size_t size, void *data) -> bool {
        auto *res = static_cast<SymbolVector *>(data);
        CLINGO_TRY {
            // NOLINTNEXTLINE
            res->insert(res->end(), cpp_cast(symbols), cpp_cast(symbols) + size);
        }
        CLINGO_CATCH;
    }

    clingo_model_t const *mdl_;
};

//! Class to provide an mutable view of a model.
//!
//! The model can be extended by theory symbols or clauses added to the running
//! search.
class Model : public ConstModel {
  public:
    //! Constructor from the underlying C representation.
    //!
    //! @param mdl the C representation of the model
    explicit Model(clingo_model_t *mdl) : ConstModel{mdl} {}

    //! Cast the model to its C representation.
    //!
    //! @param x the model to cast
    //! @return the C representation of the model
    friend auto c_cast(Model const &x) -> clingo_model_t * { return x.mdl_(); }

    //! Get the associated solve control object.
    //!
    //! @return the solve control object
    [[nodiscard]] auto control() const -> SolveControl {
        return SolveControl{Detail::call<clingo_model_control>(mdl_())};
    }

    //! Extend the model with additional symbols.
    //!
    //! @param symbols the symbols to extend the model with
    void extend(SymbolSpan symbols) const {
        Detail::handle_error(clingo_model_extend(mdl_(), c_cast(symbols.data()), symbols.size()));
    }

  private:
    [[nodiscard]] auto mdl_() const -> clingo_model_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_model_t *>(c_cast(*static_cast<ConstModel const *>(this)));
    }
};

//! Interface to handle events during solving.
class SolveEventHandler {
  public:
    //! The default constructor.
    SolveEventHandler() = default;

    //! Disable copy and move operations.
    SolveEventHandler(SolveEventHandler &&other) = delete;

    //! The default destructor.
    virtual ~SolveEventHandler() = default;

    //! Callback to interact with the model found during solving.
    //!
    //! @param model the model found during solving
    //! @return whether to continue solving
    auto model(Model model) -> bool { return do_model(model); }

    //! Callback to inspect lower bounds found during solving.
    //!
    //! @param lower_bound the lower bound found during solving
    void unsat(SumSpan lower_bound) { do_unsat(lower_bound); }

    //! Callback to update solving statistics.
    //!
    //! @param step the statistics for the current step
    //! @param accu the accumulated statistics
    void stats(Stats step, Stats accu) { do_stats(step, accu); }

    //! Callback to handle the end of solving.
    //!
    //! The main purpose of this callback is synchronization when solving
    //! asynchronously.
    //!
    //! @param result the result of the solving process
    void finish(SolveResult result) noexcept { do_finish(result); }

  private:
    virtual auto do_model([[maybe_unused]] Model model) -> bool { return true; }
    virtual void do_unsat([[maybe_unused]] SumSpan lower_bound) {}
    virtual void do_stats([[maybe_unused]] Stats step, [[maybe_unused]] Stats accu) {}
    virtual void do_finish([[maybe_unused]] SolveResult result) noexcept {}
};

//! Class to control a running search.
class SolveHandle {
  public:
    //! Sentinel indicating that models have been exhausted.
    struct sentinel {};
    //! Iterator to iterate over models found during solving.
    class iterator {
      public:
        //! The iterator category.
        using iterator_category = std::input_iterator_tag;
        //! The difference type.
        using difference_type = std::ptrdiff_t;
        //! The value type, which are models.
        using value_type = ConstModel;
        //! The pointer type.
        using pointer = ConstModel *;
        //! The reference type.
        using reference = ConstModel &;

        //! The default constructor.
        //!
        //! Construct an iterator in invalid state. For interface completeness.
        iterator() = default;

        //! Get a reference to the current model.
        auto operator*() const -> reference {
            return mdl_.value(); // NOLINT
        }

        //! Member access operator to get a pointer to the current model.
        auto operator->() const -> pointer {
            return &mdl_.value(); // NOLINT
        }

        //! Increment the iterator to the next model.
        auto operator++() -> iterator & {
            hnd_->resume();
            mdl_ = hnd_->model();
            return *this;
        }

        //! Postfix increment the iterator.
        //!
        //! For interface completeness. Do not use.
        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        //! Compare iterators for equality.
        //!
        //! For interface completeness. Do not use.
        //!
        //! @param a the first iterator to compare
        //! @param b the second iterator to compare
        //! @return whether the iterators are equal
        friend auto operator==(const iterator &a, const iterator &b) -> bool { return a.hnd_ == b.hnd_; }

        //! Check whether all models have been exhausted.
        //!
        //! @param a the iterator to compare
        //! @param b the sentinel to compare
        //! @return whether all models have been exhausted
        friend auto operator==(iterator const &a, [[maybe_unused]] sentinel const &b) -> bool {
            return !a.mdl_.has_value();
        }

      private:
        friend class SolveHandle;

        explicit iterator(SolveHandle &hnd) : hnd_{&hnd} { operator++(); }

        SolveHandle *hnd_ = nullptr;
        mutable std::optional<value_type> mdl_;
    };
    //! The difference type.
    using difference_type = iterator::difference_type;
    //! The value type, which are models.
    using value_type = iterator::value_type;
    //! The reference type.
    using reference = iterator::reference;
    //! The pointer type.
    using pointer = iterator::pointer;

    //! Constructor from the underlying C representation.
    //!
    //! For internal use.
    //!
    //! @param hnd the C representation of the solve handle
    explicit SolveHandle(clingo_solve_handle_t *hnd) : hnd_{hnd} {}

    //! Cast the solve handle to its C representation.
    //! @param x the solve handle to cast
    //! @return the C representation of the solve handle
    friend auto c_cast(SolveHandle const &x) -> clingo_solve_handle_t * { return x.hnd_.get(); }

    //! Get the solve result.
    //!
    //! This is a blocking operation and should always be called at the end of
    //! a search.
    //!
    //! @return the solve result
    [[nodiscard]] auto get() const -> SolveResult {
        return SolveResult{Detail::call<clingo_solve_handle_get>(hnd_.get())};
    }

    //! Cancel the current search.
    //!
    //! This is a blocking operation.
    void cancel() { Detail::handle_error(clingo_solve_handle_cancel(hnd_.get())); }

    //! Close the solve handle.
    //!
    //! This is a blocking operation.
    //!
    //! Closing a solver handle guarantees that the destructor of the solve
    //! handle does not throw.
    void close() { Detail::handle_error(clingo_solve_handle_close(hnd_.release())); }

    //! Resume the current search.
    //!
    //! In a asynchronous context, this will start the search for the next
    //! model. Otherwise, it is a no-op.
    void resume() { Detail::handle_error(clingo_solve_handle_resume(hnd_.get())); }

    //! Get the current model.
    //!
    //! Return std::optional if no model is available.
    //! This can happen if models are exhausted or in an asynchronous context
    //! where the next model has not yet been found.
    //!
    //! @return the current model, if available
    [[nodiscard]] auto model() -> std::optional<ConstModel> {
        clingo_model_t const *mdl = Detail::call<clingo_solve_handle_model>(hnd_.get());
        return mdl != nullptr ? std::make_optional<ConstModel>(mdl) : std::nullopt;
    }

    //! Get the last model.
    //!
    //! This function can be called after all models have been enumerated. This
    //! is particularly useful when optimizing to inpsect the optimal model.
    //!
    //! @return the last model, if available
    [[nodiscard]] auto last() const -> std::optional<ConstModel> {
        clingo_model_t const *mdl = Detail::call<clingo_solve_handle_last>(hnd_.get());
        return mdl != nullptr ? std::make_optional<ConstModel>(mdl) : std::nullopt;
    }

    //! Get the unsatisfiable core.
    //!
    //! This function can be called when the search was reported unsatisfiable.
    //! It contains a subset of the assumptions that made the problem
    //! unsatisfiable. The set is not necessarily minimal.
    //!
    //! @return the unsatisfiable core
    [[nodiscard]] auto core() const -> ProgramLiteralSpan {
        auto const *lits = static_cast<clingo_literal_t *>(nullptr);
        auto size = size_t{0};
        Detail::handle_error(clingo_solve_handle_core(hnd_.get(), &lits, &size));
        return {lits, size};
    }

    //! Wait for the next model or solve result.
    //!
    //! If yielding is active, waits for the next model.
    //! Otherwise, waits for the solve result.
    //!
    //! If no timeout is given, waits until the model or result is available.
    //! Otherwise, waits for the given timeout in seconds.
    //! A value of zero can be used for polling.
    //!
    //! @param timeout the timeout in seconds
    //! @return whether a model or result is available
    [[nodiscard]] auto wait(std::optional<double> timeout) -> bool {
        return Detail::call<clingo_solve_handle_wait>(hnd_.get(), timeout ? *timeout : -1);
    }

    //! Get an iterator to iterate over models found during solving.
    [[nodiscard]] auto begin() -> iterator { return iterator{*this}; }
    //! Get a sentinel to indicate that all models have been exhausted.
    [[nodiscard]] auto end() -> sentinel {
        static_cast<void>(this);
        return sentinel{};
    }

  private:
    friend class Control;

    struct Free {
        Free() {
            // NOTE: We assume that solve is only called during normal
            // operation - not during exception handling.
            assert(std::uncaught_exceptions() == 0);
        }
        void operator()(clingo_solve_handle_t *hnd) const noexcept(false) {
            try {
                // NOTE: currently the solve handle calls cancel and then
                // deletes clasp's underlying solve handle. I am not sure
                // whether this can actually throw or not.
                Detail::handle_error(clingo_solve_handle_close(hnd));
            } catch (...) {
                if (std::uncaught_exceptions() == 1) {
                    throw;
                }
            }
        }
    };

    std::unique_ptr<clingo_solve_handle_t, Free> hnd_;
};
static_assert(std::input_iterator<SolveHandle::iterator>);
static_assert(std::sentinel_for<SolveHandle::sentinel, SolveHandle::iterator>);

//! @}

namespace Detail {

static constexpr clingo_solve_event_handler_t c_solve_event_handler{
    [](clingo_model_t *model, void *data, bool *goon) -> bool {
        CLINGO_TRY {
            auto *hnd = static_cast<SolveEventHandler *>(data);
            assert(hnd != nullptr);
            auto mdl = Model{model};
            *goon = hnd->model(mdl);
            return true;
        }
        CLINGO_CATCH;
    },
    [](int64_t const *values, size_t size, void *data) -> bool {
        CLINGO_TRY {
            auto *hnd = static_cast<SolveEventHandler *>(data);
            assert(hnd != nullptr);
            hnd->unsat({values, size});
        }
        CLINGO_CATCH;
    },
    [](clingo_stats_t *stats, void *data) -> bool {
        CLINGO_TRY {
            auto *hnd = static_cast<SolveEventHandler *>(data);
            assert(hnd != nullptr);
            std::string_view user_step = "user_step";
            std::string_view user_accu = "user_accu";
            uint64_t root = Detail::call<clingo_stats_root>(stats);
            uint64_t step = Detail::call<clingo_stats_map_add_subkey>(stats, root, user_step.data(), user_step.size(),
                                                                      clingo_stats_type_map);
            uint64_t accu = Detail::call<clingo_stats_map_add_subkey>(stats, root, user_accu.data(), user_accu.size(),
                                                                      clingo_stats_type_map);
            hnd->stats(Stats{stats, step}, Stats{stats, accu});
        }
        CLINGO_CATCH;
    },
    [](clingo_solve_result_bitset_t result, void *data) -> void {
        auto *hnd = static_cast<SolveEventHandler *>(data);
        assert(hnd != nullptr);
        hnd->finish(static_cast<SolveResult>(result));
    },
    nullptr,
};

} // namespace Detail

} // namespace Clingo
