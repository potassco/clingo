#pragma once

#include "base.hh"
#include "stats.hh"
#include "symbol.hh"

#include "clingo/solve.h"

#include <pybind11/pybind11.h>

namespace PyClingo {

class SolveResult {
  public:
    SolveResult(clingo_solve_result_bitset_t res) : res_{res} {}

    [[nodiscard]] auto satisfiable() const -> bool { return (res_ & clingo_solve_result_satisfiable) != 0; }
    [[nodiscard]] auto unsatisfiable() const -> bool { return (res_ & clingo_solve_result_unsatisfiable) != 0; }
    [[nodiscard]] auto unknown() const -> bool {
        return (res_ & (clingo_solve_result_unsatisfiable | clingo_solve_result_satisfiable)) != 0;
    }
    [[nodiscard]] auto exhausted() const -> bool { return (res_ & clingo_solve_result_exhausted) != 0; }
    [[nodiscard]] auto interrupted() const -> bool { return (res_ & clingo_solve_result_interrupted) != 0; }
    [[nodiscard]] auto str() const -> char const * {
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
    SolveControl(clingo_solve_control_t *ctl) : ctl_{ctl} {}

    auto base() -> Base;
    auto add_clause(MixedLitSpan const &lits);
    auto add_nogood(MixedLitSpan const &lits);

  private:
    clingo_solve_control_t *ctl_;
};

class Model {
  public:
    Model(clingo_model_t const *mdl) : mdl_{mdl} {}

    auto symbols(bool shown, bool atoms, bool terms, bool theory) -> SymbolVec;
    auto contains(Symbol atom) -> bool;
    auto control() -> SolveControl;
    auto type() -> clingo_model_type_e;
    auto number() -> uint64_t;
    auto is_true(clingo_literal_t lit) -> bool;
    auto is_consequence(clingo_literal_t lit) -> std::optional<bool>;
    auto cost() -> std::span<int64_t const>;
    auto priorities() -> std::span<clingo_weight_t const>;
    auto optimality_proven() -> bool;
    auto thread_id() -> clingo_id_t;
    auto extend(std::span<Symbol const> symbols);
    auto str() -> std::string;
    auto c_ptr() -> clingo_model_t const * { return mdl_; }

  private:
    clingo_model_t const *mdl_;
};

using StatsCallback = std::function<void(Stats, Stats)>;
using ModelCallback = std::function<std::optional<bool>(Model &)>;
using UnsatCallback = std::function<void(std::span<int64_t>)>;
using FinishCallback = std::function<void(SolveResult)>;

class SolveHandle : public reference_keeper<SolveHandle> {
  public:
    SolveHandle() = default;
    SolveHandle(SolveHandle const &other) = delete;
    SolveHandle(SolveHandle &&other) noexcept = delete;
    auto operator=(SolveHandle const &other) -> SolveHandle & = delete;
    auto operator=(SolveHandle &&other) noexcept -> SolveHandle & = delete;
    ~SolveHandle() noexcept(false) { close(); }

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

  private:
    friend class Control;

    clingo_solve_handle_t *hnd_ = nullptr;
    py::handle mdl_;
    py::handle unsat_;
    py::handle stats_;
    py::handle finish_;
};

void register_solve(pybind11::module &m);

} // namespace PyClingo
