#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>
#include <clingo/stats.hh>

#include <clingo/solve.h>

namespace Clingo {

class SolveResult {
  public:
    explicit SolveResult(clingo_solve_result_bitset_t res) : res_{res} {}

    [[nodiscard]] auto satisfiable() const -> bool { return (res_ & clingo_solve_result_satisfiable) != 0; }
    [[nodiscard]] auto unsatisfiable() const -> bool { return (res_ & clingo_solve_result_unsatisfiable) != 0; }
    [[nodiscard]] auto unknown() const -> bool {
        return (res_ & (clingo_solve_result_unsatisfiable | clingo_solve_result_satisfiable)) != 0;
    }
    [[nodiscard]] auto exhausted() const -> bool { return (res_ & clingo_solve_result_exhausted) != 0; }
    [[nodiscard]] auto interrupted() const -> bool { return (res_ & clingo_solve_result_interrupted) != 0; }
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

class SolveControl {
  public:
    explicit SolveControl(clingo_solve_control_t *ctl) : ctl_{ctl} {}

    [[nodiscard]] auto base() const -> Base { return Base{Detail::call<clingo_solve_control_base>(ctl_)}; }

    auto add_clause(ProgramLiteralSpan lits) const {
        Detail::handle_error(clingo_solve_control_add_clause(ctl_, lits.data(), lits.size()));
    }

    auto add_nogood(ProgramLiteralSpan lits) const {
        add_clause(Detail::transform(lits, [](auto const &lit) { return -lit; }));
    }

  private:
    clingo_solve_control_t *ctl_;
};

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

class ConstModel {
  public:
    explicit ConstModel(clingo_model_t const *mdl) : mdl_{mdl} {}

    friend auto c_cast(ConstModel const &x) -> clingo_model_t const * { return x.mdl_; }

    [[nodiscard]] auto symbols(ShowFlags flags = ShowFlags::shown) const -> SymbolVector {
        auto res = SymbolVector{};
        Detail::handle_error(clingo_model_symbols(mdl_, static_cast<clingo_show_type_bitset_t>(flags), &sym_cb_, &res));
        return res;
    }

    [[nodiscard]] auto contains(Symbol const &atom) const -> bool {
        return Detail::call<clingo_model_contains>(mdl_, c_cast(atom));
    }

    [[nodiscard]] auto type() const -> ModelType {
        return static_cast<ModelType>(Detail::call<clingo_model_type>(mdl_));
    }

    [[nodiscard]] auto number() const -> uint64_t { return Detail::call<clingo_model_number>(mdl_); }

    [[nodiscard]] auto is_true(ProgramLiteral lit) const -> bool {
        return Detail::call<clingo_model_is_true>(mdl_, lit);
    }

    [[nodiscard]] auto is_consequence(ProgramLiteral lit) const -> std::optional<bool> {
        auto res = Detail::call<clingo_model_is_consequence>(mdl_, lit);
        if (res != clingo_consequence_unknown) {
            return res == clingo_consequence_true;
        }
        return std::nullopt;
    }

    [[nodiscard]] auto cost() const -> SumSpan {
        int64_t const *costs = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_model_cost(mdl_, &costs, &size));
        return {costs, size};
    }

    [[nodiscard]] auto priorities() const -> WeightSpan {
        clingo_weight_t const *prios = nullptr;
        size_t size = 0;
        Detail::handle_error(clingo_model_priority(mdl_, &prios, &size));
        return {prios, size};
    }

    [[nodiscard]] auto optimality_proven() const -> bool { return Detail::call<clingo_model_optimality_proven>(mdl_); }

    [[nodiscard]] auto thread_id() const -> ProgramId { return Detail::call<clingo_model_thread_id>(mdl_); }

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

class Model : public ConstModel {
  public:
    explicit Model(clingo_model_t *mdl) : ConstModel{mdl} {}

    [[nodiscard]] auto control() const -> SolveControl {
        return SolveControl{Detail::call<clingo_model_control>(mdl_())};
    }

    void extend(SymbolSpan symbols) const {
        Detail::handle_error(clingo_model_extend(mdl_(), c_cast(symbols.data()), symbols.size()));
    }

    friend auto c_cast(Model const &x) -> clingo_model_t const * { return x.mdl_(); }

  private:
    [[nodiscard]] auto mdl_() const -> clingo_model_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_model_t *>(c_cast(*static_cast<ConstModel const *>(this)));
    }
};

class SolveEventHandler {
  public:
    virtual ~SolveEventHandler() = default;
    auto model(Model &model) -> bool { return do_model(model); }
    void unsat(SumSpan lower_bound) { do_unsat(lower_bound); }
    void stats(Stats step, Stats accu) { do_stats(step, accu); }
    void finish(SolveResult result) noexcept { do_finish(result); }

  private:
    virtual auto do_model([[maybe_unused]] Model &model) -> bool { return true; }
    virtual void do_unsat([[maybe_unused]] SumSpan lower_bound) {}
    virtual void do_stats([[maybe_unused]] Stats step, [[maybe_unused]] Stats accu) {}
    virtual void do_finish([[maybe_unused]] SolveResult result) noexcept {}
};

class SolveHandle {
  public:
    struct sentinel {};
    class iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = ConstModel;
        using pointer = ConstModel *;
        using reference = ConstModel &;

        iterator() = default;

        explicit iterator(SolveHandle &hnd) : hnd_{&hnd} { operator++(); }

        auto operator*() const -> reference {
            return mdl_.value(); // NOLINT
        }

        auto operator->() const -> pointer {
            return &mdl_.value(); // NOLINT
        }

        auto operator++() -> iterator & {
            hnd_->resume();
            mdl_ = hnd_->model();
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend auto operator==(const iterator &a, const iterator &b) -> bool { return a.hnd_ == b.hnd_; }

        friend auto operator==(iterator const &a, [[maybe_unused]] sentinel const &b) -> bool {
            return !a.mdl_.has_value();
        }

      private:
        SolveHandle *hnd_ = nullptr;
        mutable std::optional<value_type> mdl_;
    };
    using difference_type = iterator::difference_type;
    using value_type = iterator::value_type;
    using reference = iterator::reference;
    using pointer = iterator::pointer;

    explicit SolveHandle(clingo_solve_handle_t *hnd) : hnd_{hnd} {}

    friend auto c_cast(SolveHandle const &x) -> clingo_solve_handle_t * { return x.hnd_.get(); }

    [[nodiscard]] auto get() const -> SolveResult {
        return SolveResult{Detail::call<clingo_solve_handle_get>(hnd_.get())};
    }

    void cancel() { Detail::handle_error(clingo_solve_handle_cancel(hnd_.get())); }

    void close() { Detail::handle_error(clingo_solve_handle_close(hnd_.release())); }

    void resume() { Detail::handle_error(clingo_solve_handle_resume(hnd_.get())); }

    [[nodiscard]] auto model() -> std::optional<ConstModel> {
        clingo_model_t const *mdl = Detail::call<clingo_solve_handle_model>(hnd_.get());
        return mdl != nullptr ? std::make_optional<ConstModel>(mdl) : std::nullopt;
    }

    [[nodiscard]] auto last() const -> std::optional<ConstModel> {
        clingo_model_t const *mdl = Detail::call<clingo_solve_handle_last>(hnd_.get());
        return mdl != nullptr ? std::make_optional<ConstModel>(mdl) : std::nullopt;
    }

    [[nodiscard]] auto core() const -> ProgramLiteralSpan {
        auto const *lits = static_cast<clingo_literal_t *>(nullptr);
        auto size = size_t{0};
        Detail::handle_error(clingo_solve_handle_core(hnd_.get(), &lits, &size));
        return {lits, size};
    }

    [[nodiscard]] auto wait(std::optional<double> timeout) -> bool {
        return Detail::call<clingo_solve_handle_wait>(hnd_.get(), timeout ? *timeout : -1);
    }

    [[nodiscard]] auto begin() -> iterator { return iterator{*this}; }
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
