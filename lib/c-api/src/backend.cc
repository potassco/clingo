#include <clingo/backend.h>

#include <clingo/control/solver.hh>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

namespace {

auto cpp_cast(clingo_backend_t *backend) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BackendHandle *>(backend);
}

auto get_program(clingo_backend_t *backend) -> Clasp::Asp::LogicProgram & {
    return cpp_cast(backend)->program();
}

auto get_theory(clingo_backend_t *backend) -> Clingo::Output::TheoryData & {
    return cpp_cast(backend)->theory();
}

auto get_store(clingo_backend_t *backend) -> Clingo::SymbolStore & {
    return cpp_cast(backend)->store();
}

auto map(clingo_external_type_e type) -> Potassco::TruthValue {
    switch (type) {
        case clingo_external_type_false: {
            return Potassco::TruthValue::false_;
        }
        case clingo_external_type_true: {
            return Potassco::TruthValue::true_;
        }
        case clingo_external_type_free: {
            return Potassco::TruthValue::free;
        }
        case clingo_external_type_release: {
            return Potassco::TruthValue::release;
        }
    }
    throw std::runtime_error("invalid type");
}

auto map(clingo_heuristic_type_e type) -> Potassco::DomModifier {
    switch (type) {
        case clingo_heuristic_type_factor: {
            return Potassco::DomModifier::factor;
        }
        case clingo_heuristic_type_false: {
            return Potassco::DomModifier::false_;
        }
        case clingo_heuristic_type_true: {
            return Potassco::DomModifier::true_;
        }
        case clingo_heuristic_type_init: {
            return Potassco::DomModifier::init;
        }
        case clingo_heuristic_type_level: {
            return Potassco::DomModifier::level;
        }
        case clingo_heuristic_type_sign: {
            return Potassco::DomModifier::sign;
        }
    }
    throw std::runtime_error("invalid type");
}

auto map(clingo_theory_sequence_type_e type) -> Clingo::TheoryTermTupleType {
    switch (type) {
        case clingo_theory_sequence_type_tuple: {
            return Clingo::TheoryTermTupleType::tuple;
        }
        case clingo_theory_sequence_type_list: {
            return Clingo::TheoryTermTupleType::list;
        }
        case clingo_theory_sequence_type_set: {
            return Clingo::TheoryTermTupleType::set;
        }
    }
    throw std::runtime_error("invalid type");
}

} // namespace

extern "C" auto clingo_control_backend(clingo_control_t *control, clingo_backend_t **backend) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *backend = reinterpret_cast<clingo_backend_t *>(control->slv->backend().release());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_close(clingo_backend_t *backend) -> bool {
    CLINGO_TRY {
        if (backend != nullptr) {
            delete cpp_cast(backend);
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_size,
                                    clingo_literal_t const *body, size_t body_size) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (head == nullptr && head_size > 0) || (body == nullptr && body_size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addRule(choice ? Potassco::HeadType::choice : Potassco::HeadType::disjunctive,
                                     Potassco::AtomSpan{head, head_size}, Potassco::LitSpan{body, body_size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head,
                                           size_t head_size, clingo_weight_t lower_bound,
                                           clingo_weighted_literal_t const *body, size_t body_size) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (head == nullptr && head_size > 0) || (body == nullptr && body_size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addRule(choice ? Potassco::HeadType::choice : Potassco::HeadType::disjunctive,
                                     std::span{head, head_size}, lower_bound, map(body, body_size));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t priority,
                                        clingo_weighted_literal_t const *literals, size_t size) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (literals == nullptr && size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addMinimize(priority, map(literals, size));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t size) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (atoms == nullptr && size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addProject(std::span{atoms, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t type)
    -> bool {
    CLINGO_TRY {
        if (backend == nullptr) {
            return fail_arguments();
        }
        get_program(backend).addExternal(atom, map(static_cast<clingo_external_type_e>(type)));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals, size_t size)
    -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (literals == nullptr && size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addAssumption(std::span{literals, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type,
                                         int bias, unsigned priority, clingo_literal_t const *condition, size_t size)
    -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (condition == nullptr && size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addDomHeuristic(atom, map(static_cast<clingo_heuristic_type_e>(type)), bias, priority,
                                             std::span{condition, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v,
                                         clingo_literal_t const *condition, size_t size) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (condition == nullptr && size > 0)) {
            return fail_arguments();
        }
        get_program(backend).addAcycEdge(node_u, node_v, std::span{condition, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_add_atom(clingo_backend_t *backend, clingo_symbol_t const *symbol, clingo_atom_t *atom)
    -> bool {
    CLINGO_TRY {
        if (backend == nullptr || atom == nullptr) {
            return fail_arguments();
        }
        *atom = symbol != nullptr ? cpp_cast(backend)->add_atom(*cpp_cast(symbol)) : get_program(backend).newAtom();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_term_number(clingo_backend_t *backend, int number, clingo_id_t *term_id) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || term_id == nullptr) {
            return fail_arguments();
        }
        *term_id = get_theory(backend).num(number);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_term_string(clingo_backend_t *backend, char const *string, size_t size,
                                                  clingo_id_t *term_id) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (string == nullptr && size > 0) || term_id == nullptr) {
            return fail_arguments();
        }
        *term_id = get_theory(backend).str(*get_store(backend).string(std::string_view{string, size}));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_term_symbol(clingo_backend_t *backend, clingo_symbol_t symbol,
                                                  clingo_id_t *term_id) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || term_id == nullptr) {
            return fail_arguments();
        }
        *term_id = get_theory(backend).sym(*cpp_cast(&symbol));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_term_sequence(clingo_backend_t *backend, clingo_theory_sequence_type_t type,
                                                    clingo_id_t const *arguments, size_t size, clingo_id_t *term_id)
    -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (arguments == nullptr && size == 0) || term_id == nullptr) {
            return fail_arguments();
        }
        *term_id =
            get_theory(backend).tup(map(static_cast<clingo_theory_sequence_type_e>(type)), std::span{arguments, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_term_function(clingo_backend_t *backend, char const *name, size_t name_size,
                                                    clingo_id_t const *arguments, size_t arguments_size,
                                                    clingo_id_t *term_id) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (name == nullptr && name_size > 0) || arguments == nullptr || arguments_size == 0 ||
            term_id == nullptr) {
            return fail_arguments();
        }
        *term_id = get_theory(backend).fun(*get_store(backend).string(name), std::span{arguments, arguments_size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_element(clingo_backend_t *backend, clingo_id_t const *tuple, size_t tuple_size,
                                              clingo_literal_t const *condition, size_t condition_size,
                                              clingo_id_t *element_id) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (tuple == nullptr && tuple_size > 0) ||
            (condition == nullptr && condition_size > 0) || element_id == nullptr) {
            return fail_arguments();
        }
        *element_id = get_theory(backend).elem(std::span{tuple, tuple_size}, std::span{condition, condition_size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_backend_theory_atom(clingo_backend_t *backend, clingo_symbol_t name, clingo_id_t const *elements,
                                           size_t size, clingo_string_t *operator_name, clingo_id_t right_hand_side_id,
                                           clingo_atom_t const *atom_in, clingo_atom_t *atom_out) -> bool {
    CLINGO_TRY {
        if (backend == nullptr || (elements == nullptr && size > 0)) {
            return fail_arguments();
        }
        auto op = Clingo::SharedString{};
        auto guard = std::optional<std::pair<Clingo::String, clingo_id_t>>{};
        if (operator_name != nullptr) {
            op = get_store(backend).string(std::string_view{operator_name->data, operator_name->size});
            guard.emplace(*op, right_hand_side_id);
        }
        *atom_out =
            get_theory(backend).atom([&]() { return atom_in != nullptr ? *atom_in : get_program(backend).newAtom(); },
                                     *cpp_cast(&name), std::span{elements, size}, std::move(guard));
    }
    CLINGO_CATCH;
}
