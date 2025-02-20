#include <clingo/backend.h>

#include <clingo/control/solver.hh>

#include "lib.hh"

namespace {

auto cpp_cast(clingo_backend_t *backend) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BackendHandle *>(backend);
}

auto get_backend(clingo_backend_t *backend) -> Clingo::Output::Backend & {
    return cpp_cast(backend)->backend();
}

} // namespace

extern "C" auto clingo_backend_close(clingo_backend_t *backend) -> clingo_result_t {
    CLINGO_TRY {
        if (backend == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(backend)->close();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_size,
                                    clingo_literal_t const *body, size_t body_size) -> clingo_result_t {
    CLINGO_TRY {
        if (backend == nullptr || (head == nullptr && head_size > 0) || (body == nullptr && body_size > 0)) {
            return clingo_result_invalid;
        }
        static thread_local auto lits = Clingo::Output::LitVec{};
        lits.clear();
        for (auto const &atom : std::span{head, head_size}) {
            lits.emplace_back(Clingo::Util::safe_cast<Clingo::Output::lit_t>(atom));
        }
        get_backend(backend).rule(lits, std::span{body, body_size}, choice);
    }
    CLINGO_CATCH;
}

// TODO: remove
// NOLINTBEGIN

extern "C" clingo_result_t clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head,
                                                      size_t head_size, clingo_weight_t lower_bound,
                                                      clingo_weighted_literal_t const *body, size_t body_size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t priority,
                                                   clingo_weighted_literal_t const *literals, size_t size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom,
                                                   clingo_external_type_t type) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals,
                                                 size_t size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom,
                                                    clingo_heuristic_type_t type, int bias, unsigned priority,
                                                    clingo_literal_t const *condition, size_t size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v,
                                                    clingo_literal_t const *condition, size_t size) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_add_atom(clingo_backend_t *backend, clingo_symbol_t *symbol,
                                                   clingo_atom_t *atom) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_term_number(clingo_backend_t *backend, int number,
                                                             clingo_id_t *term_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_term_string(clingo_backend_t *backend, char const *string,
                                                             clingo_id_t *term_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_term_sequence(clingo_backend_t *backend,
                                                               clingo_theory_sequence_type_t type,
                                                               clingo_id_t const *arguments, size_t size,
                                                               clingo_id_t *term_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_term_function(clingo_backend_t *backend, char const *name,
                                                               clingo_id_t const *arguments, size_t size,
                                                               clingo_id_t *term_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_term_symbol(clingo_backend_t *backend, clingo_symbol_t symbol,
                                                             clingo_id_t *term_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_element(clingo_backend_t *backend, clingo_id_t const *tuple,
                                                         size_t tuple_size, clingo_literal_t const *condition,
                                                         size_t condition_size, clingo_id_t *element_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_atom(clingo_backend_t *backend, clingo_atom_t atom,
                                                      clingo_id_t term_id, clingo_id_t const *elements, size_t size,
                                                      clingo_atom_t *atom_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

extern "C" clingo_result_t clingo_backend_theory_atom_with_guard(clingo_backend_t *backend, clingo_atom_t atom,
                                                                 clingo_id_t term_id, clingo_id_t const *elements,
                                                                 size_t size, char const *operator_name,
                                                                 clingo_id_t right_hand_side_id,
                                                                 clingo_atom_t *atom_id) {
    CLINGO_TRY {
    }
    CLINGO_CATCH;
}

// TODO: remove
// NOLINTEND
