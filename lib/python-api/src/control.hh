#pragma once

#include "ast.hh"
#include "symbol.hh"

#include <clingo/control.h>
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
    Model(clingo_model_t *mdl) : mdl_{mdl} {}

    auto symbols(bool shown, bool atoms, bool terms, bool theory, bool complement) -> SymbolVec;

  private:
    clingo_model_t *mdl_;
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

    auto handle() -> clingo_solve_handle_t *& { return hnd_; }
    auto get() -> SolveResult;
    auto exception_ptr() -> std::exception_ptr { return std::exchange(ptr_, nullptr); }
    void close();

    static auto c_event_handler(clingo_solve_event_type_t type, void *event, void *data, bool *goon) -> clingo_result_t;

  private:
    clingo_solve_handle_t *hnd_ = nullptr;
    std::optional<ModelCallback> mdl_;
    std::exception_ptr ptr_;
};
using USolveHandle = std::unique_ptr<SolveHandle>;

class Control {
  public:
    Control(Library &lib, std::vector<std::string> const &args);
    Control(clingo_control_t *ctl) : ctl_{ctl} {}

    void parse_string(char const *str);
    void join(Program &prg);
    void ground(std::optional<std::vector<std::pair<std::string, SymbolVec>>> const &parts, py::handle ctx);
    auto solve(std::optional<ModelCallback> on_model = std::nullopt) -> USolveHandle;
    void main();
    auto buffer() -> char const *;

  private:
    static auto ctx_(clingo_lib_t *lib, clingo_location_t const *location, char const *name,
                     clingo_symbol_t const *arguments, size_t arguments_size, void *data,
                     clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) -> clingo_result_t;
    static void free_(clingo_control_t *ctl) noexcept { clingo_control_free(ctl); }
    owner_ptr<clingo_control_t, free_> ctl_;
};

void register_control(pybind11::module &m);

} // namespace Clingo::Python
