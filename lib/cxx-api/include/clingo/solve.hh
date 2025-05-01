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

    [[nodiscard]] auto base() const -> Base {
        clingo_base_t const *base = nullptr;
        Detail::handle_error(clingo_solve_control_base(ctl_, &base));
        return {base};
    }

    auto add_clause(LiteralSpan lits) const {
        Detail::handle_error(clingo_solve_control_add_clause(ctl_, lits.data(), lits.size()));
    }

    auto add_nogood(LiteralSpan lits) const {
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

class ConstModel {
  public:
    explicit ConstModel(clingo_model_t const *mdl) : mdl_{mdl} {}

    friend auto c_cast(ConstModel const &x) -> clingo_model_t const * { return x.mdl_; }

    [[nodiscard]] auto symbols(ShowFlags flags) const -> SymbolVector {
        auto res = SymbolVector{};
        Detail::handle_error(clingo_model_symbols(mdl_, static_cast<clingo_show_type_bitset_t>(flags), &sym_cb_, &res));
        return res;
    }

    [[nodiscard]] auto contains(Symbol const &atom) const -> bool {
        bool res = false;
        Detail::handle_error(clingo_model_contains(mdl_, c_cast(atom), &res));
        return res;
    }

    [[nodiscard]] auto type() const -> ModelType {
        clingo_model_type_t type = 0;
        Detail::handle_error(clingo_model_type(mdl_, &type));
        return static_cast<ModelType>(type);
    }

    [[nodiscard]] auto number() const -> uint64_t {
        uint64_t num = 0;
        Detail::handle_error(clingo_model_number(mdl_, &num));
        return num;
    }

    [[nodiscard]] auto is_true(Literal lit) const -> bool {
        auto res = false;
        Detail::handle_error(clingo_model_is_true(mdl_, lit, &res));
        return res;
    }

    [[nodiscard]] auto is_consequence(Literal lit) const -> std::optional<bool> {
        clingo_consequence_t res = 0;
        Detail::handle_error(clingo_model_is_consequence(mdl_, lit, &res));
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

    [[nodiscard]] auto optimality_proven() const -> bool {
        bool res = false;
        Detail::handle_error(clingo_model_optimality_proven(mdl_, &res));
        return res;
    }

    [[nodiscard]] auto thread_id() const -> Id {
        clingo_id_t id = 0;
        Detail::handle_error(clingo_model_thread_id(mdl_, &id));
        return id;
    }

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
    static auto sym_cb_(clingo_symbol_t const *symbols, size_t size, void *data) -> clingo_result_t {
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
        clingo_solve_control_t *ctl = nullptr;
        Detail::handle_error(clingo_model_control(mdl_(), &ctl));
        return SolveControl{ctl};
    }

    void extend(SymbolSpan symbols) const {
        Detail::handle_error(clingo_model_extend(mdl_(), c_cast(symbols.data()), symbols.size()));
    }

    friend auto c_cast(Model const &x) -> clingo_model_t const * { return x.mdl_(); }

  private:
    // NOTE: the const_cast is fine because the base class has been initialized
    // with a non-const pointer.
    [[nodiscard]] auto mdl_() const -> clingo_model_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_model_t *>(c_cast(*static_cast<ConstModel const *>(this)));
    }
};

/*
struct Stats;

using StatsCallback = std::function<void(Stats, Stats)>;
using ModelCallback = std::function<std::optional<bool>(Model &)>;

class SolveHandle {
  public:
    SolveHandle(ModelCallback mdl = nullptr, StatsCallback stats = nullptr)
        : mdl_{std::move(mdl)}, stats_{std::move(stats)} {}
    SolveHandle(SolveHandle const &other) = delete;
    SolveHandle(SolveHandle &&other) noexcept = delete;
    auto operator=(SolveHandle const &other) -> SolveHandle & = delete;
    auto operator=(SolveHandle &&other) noexcept -> SolveHandle & = delete;
    ~SolveHandle() { close(); }

    auto get() -> SolveResult;
    void cancel();
    void resume();
    auto model() -> std::optional<Model>;
    auto last() -> std::optional<Model>;
    auto core() -> std::span<clingo_literal_t const>;
    auto wait(std::optional<double> timeout) -> bool;
    auto next() -> Model;
    void close();

    auto handle() -> clingo_solve_handle_t *& { return hnd_; }
    static auto c_event_handler(clingo_solve_event_type_t type, void *event, void *data, bool *goon) -> clingo_result_t;
    auto exception() -> std::exception_ptr & { return ptr_; }

  private:
    std::exception_ptr ptr_;
    clingo_solve_handle_t *hnd_ = nullptr;
    std::optional<ModelCallback> mdl_;
    std::optional<StatsCallback> stats_;
};
using SSolveHandle = std::shared_ptr<SolveHandle>;
*/

} // namespace Clingo
