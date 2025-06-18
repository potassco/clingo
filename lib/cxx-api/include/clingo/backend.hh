#pragma once

#include <clingo/core.hh>
#include <clingo/symbol.hh>

#include <clingo/backend.h>

#include <cassert>

namespace Clingo {

//! @addtogroup cpp_backend
//! Extend the logic program with ground statements (in aspif-like format).
//! @{

//! Enumeration of available theory sequence types.
enum class TheorySequenceType : clingo_theory_sequence_type_t {
    tuple = clingo_theory_sequence_type_tuple, //!< Theory tuples "(t1,...,tn)".
    set = clingo_theory_sequence_type_set,     //!< Theory sets "{t1,...,tn}".
    list = clingo_theory_sequence_type_list    //!< Theory lists "[t1,...,tn]".
};

//! Theory backend to build theory atoms.
class TheoryBackend {
  public:
    //! Add a numeric theory term.
    //!
    //! @param number the value of the term
    //! @return the resulting term id
    [[nodiscard]] auto number(int number) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_number>(backend_.get(), number);
    }

    //! Add a string theory term.
    //!
    //! Includes constants as well as quoted strings.
    //!
    //! @param string the value of the term
    //! @return the resulting term id
    [[nodiscard]] auto string(std::string_view string) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_string>(backend_.get(), string.data(), string.size());
    }

    //! Convert the given symbol into a theory term.
    //!
    //! @param symbol the symbol to convert
    //! @return the resulting term id
    [[nodiscard]] auto symbol(Symbol const &symbol) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_symbol>(backend_.get(), c_cast(symbol));
    }

    //! Add a theory term sequence.
    //!
    //! @param type the type of the sequence
    //! @param elements the elements of the sequence
    //! @return the resulting term id
    [[nodiscard]] auto sequence(TheorySequenceType type, ProgramIdSpan elements) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_sequence>(
            backend_.get(), static_cast<clingo_theory_sequence_type_t>(type), elements.data(), elements.size());
    }

    //! Add a theory function.
    //!
    //! @param name the name of the function
    //! @param elements the arguments of the function
    //! @return the resulting term id
    [[nodiscard]] auto function(std::string_view name, ProgramIdSpan elements) const -> ProgramId {
        return Detail::call<clingo_backend_theory_term_function>(backend_.get(), name.data(), name.size(),
                                                                 elements.data(), elements.size());
    }

    //! Add a theory element.
    //!
    //! @param tuple the theory term tuple of the element
    //! @param condition the program literals forming the condition
    //! @return the resulting term id
    [[nodiscard]] auto element(ProgramIdSpan tuple, ProgramLiteralSpan condition) const -> ProgramId {
        return Detail::call<clingo_backend_theory_element>(backend_.get(), tuple.data(), tuple.size(), condition.data(),
                                                           condition.size());
    }

    //! Add a theory atom.
    //!
    //! If an equivalent theory atom already exists, the function returns its
    //! associated program atom. If no equivalent theory atom exists and the
    //! provided program atom is engaged, a new theory atom is created using
    //! this program atom, and the function returns it. If the provided program
    //! atom is not engaged, a new theory atom is created and assigned a fresh
    //! program atom, and the function returns the newly created program atom.
    //!
    //! @param atom an optional program atom to assign
    //! @param name the symbol representing the name of the atom
    //! @param elements the theory elements of the atom
    //! @param guard the optional guard of the atom
    //! @return the resulting term id
    [[nodiscard]] auto atom(std::optional<ProgramAtom> atom, Symbol const &name, ProgramIdSpan elements = {},
                            std::optional<std::pair<std::string_view, ProgramId>> const &guard = std::nullopt) const
        -> ProgramAtom {
        auto op = clingo_string_t{guard ? guard->first.data() : nullptr, guard ? guard->first.size() : 0};
        return Detail::call<clingo_backend_theory_atom>(backend_.get(), c_cast(name), elements.data(), elements.size(),
                                                        guard ? &op : nullptr, guard ? guard->second : 0,
                                                        atom ? &*atom : nullptr);
    }

  private:
    friend class ProgramBackend;

    struct Free {
        Free() {
            // NOTE: We assume that backends are only created during normal
            // operation - not during exception handling.
            assert(std::uncaught_exceptions() == 0);
        }
        auto operator()(clingo_backend_t *backend) const noexcept(false) -> void {
            if (std::uncaught_exceptions() == 0) {
                Detail::handle_error(clingo_backend_close(std::exchange(backend, nullptr)));
            }
        }
    };

    explicit TheoryBackend(clingo_backend_t *backend) : backend_{backend} {}

    ~TheoryBackend() = default;

    std::unique_ptr<clingo_backend_t, Free> backend_;
};

//! Program backend to add atoms and statements.
//!
//! The program backend must not be stored. Its destructor finalizes the added
//! statements.
class ProgramBackend : private TheoryBackend {
  public:
    //! Create a program atom from its C representation.
    //!
    //! For internal use.
    //!
    //! @param backend the C backend
    explicit ProgramBackend(clingo_backend_t *backend) : TheoryBackend{backend} {}

    //! Finalize the backend.
    //!
    //! After closing the backend, it must no longer be used. If this function
    //! is called, the destructor is guaranteed to not throw.
    void close() { Detail::handle_error(clingo_backend_close(backend_.release())); }

    //! Create a fresh program atom.
    //!
    //! If a symbol is given, the corresponding atom is returned or a fresh one
    //! created. If no symbol is given, a fresh program atom is returned.
    //!
    //! @param symbol the symbol of the program atom
    //! @return the program atom
    [[nodiscard]] auto atom(std::optional<Symbol> symbol) const -> ProgramAtom {
        return Detail::call<clingo_backend_add_atom>(backend_.get(), symbol ? c_cast(&symbol.value()) : nullptr);
    }

    //! Add a choice or disjunctive ground rule.
    //!
    //! @param head the head atoms
    //! @param body the body literals
    //! @param choice whether the head is a choice or disjunction
    void rule(ProgramAtomSpan head, ProgramLiteralSpan body = {}, bool choice = false) const {
        Detail::handle_error(
            clingo_backend_rule(backend_.get(), choice, head.data(), head.size(), body.data(), body.size()));
    }

    //! Add a ground weight rule to the program.
    //!
    //! @param head the head atoms
    //! @param lower the lower bound
    //! @param body the weighted body literals
    //! @param choice whether the head is a choice or disjunction
    void weight_rule(ProgramAtomSpan head, Weight lower, WeightedLiteralSpan body, bool choice = false) const {
        Detail::handle_error(clingo_backend_weight_rule(backend_.get(), choice, head.data(), head.size(), lower,
                                                        body.data(), body.size()));
    }

    //! Add a minimize constraint to the program.
    //!
    //! @param literals the weighted literals
    //! @param priority the priority of the constraint
    void minimize(WeightedLiteralSpan literals, Weight priority = 0) const {
        Detail::handle_error(clingo_backend_minimize(backend_.get(), priority, literals.data(), literals.size()));
    }

    //! Add a project directive to the program.
    //!
    //! @param atoms the atoms to project
    void project(ProgramAtomSpan atoms) const {
        Detail::handle_error(clingo_backend_project(backend_.get(), atoms.data(), atoms.size()));
    }

    //! Add an external directive to the program.
    //!
    //! @param atom the external atom
    //! @param type the type of the external atom
    void external(ProgramAtom atom, ExternalType type) const {
        Detail::handle_error(clingo_backend_external(backend_.get(), atom, static_cast<clingo_external_type_t>(type)));
    }

    //! Add assumptions to the program.
    //!
    //! @param literals the literals to assume
    void assume(ProgramLiteralSpan literals) const {
        Detail::handle_error(clingo_backend_assume(backend_.get(), literals.data(), literals.size()));
    }

    //! Add a heuristic directive to the program.
    //!
    //! @param atom the target atom
    //! @param type the type of the heuristic modification
    //! @param bias the heuristic bias
    //! @param priority the heuristic priority
    //! @param condition the condition under which to apply the heuristic modification
    void heuristic(ProgramAtom atom, HeuristicType type, int bias, unsigned priority = 0,
                   ProgramLiteralSpan condition = {}) const {
        Detail::handle_error(clingo_backend_heuristic(backend_.get(), atom, static_cast<clingo_heuristic_type_t>(type),
                                                      bias, priority, condition.data(), condition.size()));
    }

    //! Add an edge directive.
    //!
    //! @param node_u the start vertex of the edge
    //! @param node_v the end vertex of the edge
    //! @param condition the condition under which the edge is part of the graph
    void edge(int node_u, int node_v, ProgramLiteralSpan condition) const {
        Detail::handle_error(
            clingo_backend_acyc_edge(backend_.get(), node_u, node_v, condition.data(), condition.size()));
    }

    //! Get the associated theory backend.
    //!
    //! @return the backend
    [[nodiscard]] auto theory() const -> TheoryBackend const & { return *this; }
};

//! @}

} // namespace Clingo
