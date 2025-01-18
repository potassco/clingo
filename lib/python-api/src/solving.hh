#pragma once

#include "symbol.hh"

#include <clingo/solve.h>

#include <pybind11/pybind11.h>

namespace Clingo::Python {

class SolveResult {
  public:
    SolveResult(clingo_solve_result_bitset_t res) : res_{res} {}

    [[nodiscard]] auto satisfiable() const -> bool { return (res_ & clingo_solve_result_satisfiable) != 0; }
    [[nodiscard]] auto unsatisfiable() const -> bool { return (res_ & clingo_solve_result_unsatisfiable) != 0; }
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

class Model {
  public:
    Model(clingo_model_t const *mdl) : mdl_{mdl} {}

    auto symbols(bool shown, bool atoms, bool terms, bool theory, bool complement) -> SymbolVec;

  private:
    clingo_model_t const *mdl_;
};

using ModelCallback = std::function<bool(Model &)>;

class SolveHandle {
  public:
    SolveHandle(std::optional<ModelCallback> mdl) : mdl_{std::move(mdl)} {}
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
    auto core() -> std::vector<clingo_literal_t>;
    auto wait(double timeout) -> bool;
    void close();

    auto handle() -> clingo_solve_handle_t *& { return hnd_; }
    auto exception_ptr() -> std::exception_ptr { return std::exchange(ptr_, nullptr); }
    static auto c_event_handler(clingo_solve_event_type_t type, void *event, void *data, bool *goon) -> clingo_result_t;

  private:
    clingo_solve_handle_t *hnd_ = nullptr;
    std::optional<ModelCallback> mdl_;
    std::exception_ptr ptr_;
};
using SSolveHandle = std::shared_ptr<SolveHandle>;

void register_solving(pybind11::module &m);

} // namespace Clingo::Python
