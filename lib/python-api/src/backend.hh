#pragma once

#include <clingo/backend.h>
#include <clingo/observe.h>

#include <pybind11/pybind11.h>

#include "base.hh"
#include "symbol.hh"

namespace PyClingo {

class Observer {
  public:
    void init_program(bool incremental);
    void begin_step();
    void end_step(Base base);
    void rule(AtomSpan head, LitSpan body, bool choice);
    void weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice);
    void minimize(WeightLitSpan literals, clingo_weight_t priority);
    void project(AtomSpan atoms);
    void external(clingo_atom_t atom, clingo_external_type_e type);
    void assume(LitSpan literals);
    void heuristic(clingo_atom_t atom, clingo_heuristic_type_e type, int bias, unsigned priority, LitSpan condition);
    void edge(int node_u, int node_v, LitSpan condition);

    void observe(clingo_control_t *ctl, bool preprocess);

  private:
    template <class... Args> void no_op([[maybe_unused]] Args const &...args) {}
};

class Backend {
  public:
    Backend(clingo_backend_t *backend) : backend_{backend} {}
    auto atom(std::optional<Symbol> symbol) -> clingo_atom_t;
    void rule(AtomSpan head, LitSpan body, bool choice);
    void weight_rule(AtomSpan head, clingo_weight_t lower, WeightLitSpan body, bool choice);
    void minimize(WeightLitSpan literals, clingo_weight_t priority);
    void project(AtomSpan atoms);
    void external(clingo_atom_t atom, clingo_external_type_e type);
    void assume(LitSpan literals);
    void heuristic(clingo_atom_t atom, clingo_heuristic_type_e type, int bias, unsigned priority, LitSpan condition);
    void edge(int node_u, int node_v, LitSpan condition);

    auto theory_number(int number) -> clingo_id_t;
    auto theory_string(std::string_view string) -> clingo_id_t;
    auto theory_symbol(Symbol symbol) -> clingo_id_t;
    auto theory_sequence(clingo_theory_sequence_type_e type, IdSpan elements) -> clingo_id_t;
    auto theory_function(std::string_view name, IdSpan elements) -> clingo_id_t;
    auto theory_element(IdSpan tuple, LitSpan condition) -> clingo_id_t;
    auto theory_atom(std::optional<clingo_atom_t> atom, Symbol name, IdSpan elements,
                     std::optional<std::pair<std::string, clingo_id_t>> const &guard) -> clingo_atom_t;

  private:
    clingo_backend_t *backend_;
};

class BackendManager {
  public:
    BackendManager(clingo_control_t *ctl) : ctl_{ctl} {}

    auto enter() -> Backend;
    void exit(std::optional<pybind11::type> const &type, std::optional<pybind11::object> const &value,
              std::optional<pybind11::object> const &traceback);

  private:
    clingo_control_t *ctl_;
    clingo_backend_t *backend_ = nullptr;
};

void register_backend(pybind11::module &m);

} // namespace PyClingo
