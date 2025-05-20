#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>

#include <clingo/observe.h>

namespace Clingo {

class Observer {
  public:
    Observer() = default;
    Observer(Observer &&other) = delete;
    virtual ~Observer() = default;

    void init_program(bool incremental) { do_init_program(incremental); }
    void begin_step() { do_begin_step(); }
    void end_step(Base base) { do_end_step(base); }
    void rule(ProgramAtomSpan head, ProgramLiteralSpan body, bool choice) { do_rule(head, body, choice); }
    void weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice) {
        do_weight_rule(head, lower, body, choice);
    }
    void minimize(WeightedLiteralSpan literals, Weight priority) { do_minimize(literals, priority); }
    void project(ProgramAtomSpan atoms) { do_project(atoms); }
    void external(ProgramAtom atom, ExternalType type) { do_external(atom, type); }
    void assume(ProgramLiteralSpan literals) { do_assume(literals); }
    void heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority, ProgramLiteralSpan condition) {
        do_heuristic(atom, type, bias, priority, condition);
    }
    void edge(int node_u, int node_v, ProgramLiteralSpan condition) { do_edge(node_u, node_v, condition); }

  private:
    friend class Control;

    void observe(clingo_control_t *ctl, bool preprocess) {
        static constexpr auto cobs = clingo_observer_t{
            [](bool incremental, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->init_program(incremental);
                }
                CLINGO_CATCH;
            },
            [](void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->begin_step();
                }
                CLINGO_CATCH;
            },
            [](clingo_base_t const *base, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->end_step(Base{base});
                }
                CLINGO_CATCH;
            },
            [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size,
               void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->rule(std::span{head, head_size}, std::span{body, body_size}, choice);
                }
                CLINGO_CATCH;
            },
            [](bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower,
               clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->weight_rule(std::span{head, head_size}, lower,
                                                               std::span{body, body_size}, choice);
                }
                CLINGO_CATCH;
            },
            [](clingo_weight_t priority, clingo_weighted_literal_t const *body, size_t body_size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->minimize(std::span{body, body_size}, priority);
                }
                CLINGO_CATCH;
            },
            [](clingo_atom_t const *atoms, size_t size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->project(std::span{atoms, size});
                }
                CLINGO_CATCH;
            },
            [](clingo_atom_t atom, clingo_external_type_t type, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->external(atom, static_cast<ExternalType>(type));
                }
                CLINGO_CATCH;
            },
            [](clingo_literal_t const *literals, size_t size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->assume(std::span{literals, size});
                }
                CLINGO_CATCH;
            },
            [](clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority,
               clingo_literal_t const *condition, size_t size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->heuristic(atom, static_cast<HeuristicType>(type), bias, priority,
                                                             std::span{condition, size});
                }
                CLINGO_CATCH;
            },
            [](int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data) -> bool {
                CLINGO_TRY {
                    static_cast<Observer *>(data)->edge(node_u, node_v, std::span{condition, size});
                }
                CLINGO_CATCH;
            },
        };
        Detail::handle_error(clingo_control_observe(ctl, &cobs, static_cast<void *>(this), preprocess));
    }

    virtual void do_init_program([[maybe_unused]] bool incremental) {}
    virtual void do_begin_step() {}
    virtual void do_end_step([[maybe_unused]] Base base) {}
    virtual void do_rule([[maybe_unused]] ProgramAtomSpan head, [[maybe_unused]] ProgramLiteralSpan body,
                         [[maybe_unused]] bool choice) {}
    virtual void do_weight_rule([[maybe_unused]] ProgramAtomSpan head, [[maybe_unused]] Weight lower,
                                [[maybe_unused]] WeightedLiteralSpan body, [[maybe_unused]] bool choice) {}
    virtual void do_minimize([[maybe_unused]] WeightedLiteralSpan literals, [[maybe_unused]] Weight priority) {}
    virtual void do_project([[maybe_unused]] ProgramAtomSpan atoms) {}
    virtual void do_external([[maybe_unused]] ProgramAtom atom, [[maybe_unused]] ExternalType type) {}
    virtual void do_assume([[maybe_unused]] ProgramLiteralSpan literals) {}
    virtual void do_heuristic([[maybe_unused]] ProgramAtom atom, [[maybe_unused]] HeuristicType type,
                              [[maybe_unused]] int bias, [[maybe_unused]] unsigned priority,
                              [[maybe_unused]] ProgramLiteralSpan condition) {}
    virtual void do_edge([[maybe_unused]] int node_u, [[maybe_unused]] int node_v,
                         [[maybe_unused]] ProgramLiteralSpan condition) {}
};

} // namespace Clingo
