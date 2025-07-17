#pragma once

#include <clingo/base.hh>
#include <clingo/core.hh>

#include <clingo/observe.h>

namespace Clingo {

//! @addtogroup cpp_observe
//! Functions and data structures to inspect ground programs.
//! @{

//! Observer interface to inspect the current ground program.
class Observer {
  public:
    //! The default constructor.
    Observer() = default;

    //! Disable move and copy operations.
    Observer(Observer &&other) = delete;

    //! Disable move and copy operations.
    auto operator=(Observer &&other) -> Observer & = delete;

    //! The default destructor.
    virtual ~Observer() = default;

    //! Callback for the beginning of the program.
    //!
    //! @param incremental whether the program is incremental
    void init_program(bool incremental) { do_init_program(incremental); }

    //! Callback for the beginning of a step.
    void begin_step() { do_begin_step(); }

    //! Callback for the end of a step.
    void end_step(Base base) { do_end_step(base); }

    //! Callback for a rule.
    //!
    //! @param head the head of the rule
    //! @param body the body of the rule
    //! @param choice whether the head is a choice or a disjunction
    void rule(ProgramAtomSpan head, ProgramLiteralSpan body, bool choice) { do_rule(head, body, choice); }

    //! Callback for a weight rule.
    //!
    //! @param head the head of the weight rule
    //! @param lower the lower bound of the weight rule
    //! @param body the weighted body of the weight rule
    //! @param choice whether the head is a choice or a disjunction
    void weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice) {
        do_weight_rule(head, lower, body, choice);
    }

    //! Callback for a minimize constraint.
    //!
    //! @param literals the weighted literals to minimize
    //! @param priority the priority of the minimize constraint
    void minimize(WeightedLiteralSpan literals, Weight priority) { do_minimize(literals, priority); }

    //! Callback for a projection directive.
    //!
    //! @param atoms the atoms to project on
    void project(ProgramAtomSpan atoms) { do_project(atoms); }

    //! Callback for an external statement.
    //!
    //! @param atom the external atom
    //! @param type the type of the external statement
    void external(ProgramAtom atom, ExternalType type) { do_external(atom, type); }

    //! Callback for an assumption directive.
    //!
    //! @param literals the literals to assume
    void assume(ProgramLiteralSpan literals) { do_assume(literals); }

    //! @param atom the atom to apply the heuristic to
    //! @param type the type of the heuristic modification
    //! @param bias the bias of the heuristic modification
    //! @param priority the priority of the heuristic modification
    //! @param condition the condition when to apply the heuristic modification
    void heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority, ProgramLiteralSpan condition) {
        do_heuristic(atom, type, bias, priority, condition);
    }

    //! Callback for an edge statement.
    //!
    //! @param node_u the first node of the edge
    //! @param node_v the second node of the edge
    //! @param condition the condition under which the edge is active
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

//! @}

} // namespace Clingo
