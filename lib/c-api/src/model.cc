#include <clingo/control/solver.hh>

#include <clingo/model.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

auto cpp_cast(clingo_model_t const *model) -> Clingo::Control::Model const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::Model const *>(model);
}

extern "C" auto clingo_model_type(clingo_model_t const *model, clingo_model_type_t *type) -> clingo_result_t {
    CLINGO_TRY { *type = static_cast<clingo_model_type_t>(cpp_cast(model)->type()); }
    CLINGO_CATCH;
}
extern "C" auto clingo_model_number(clingo_model_t const *model, uint64_t *number) -> clingo_result_t {
    CLINGO_TRY { *number = cpp_cast(model)->number(); }
    CLINGO_CATCH;
}
extern "C" auto clingo_model_symbols(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                     clingo_symbol_callback_t callback, void *data) -> clingo_result_t {
    CLINGO_TRY {
        auto const *mdl = cpp_cast(model);
        auto flags = static_cast<Clingo::Control::SymbolSelectFlags>(show);
        Clingo::SymbolVec res;
        mdl->symbols(flags, res);
        handle_error(callback(c_cast(res.data()), res.size(), data));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_contains(clingo_model_t const *model, clingo_symbol_t atom, bool *contained)
    -> clingo_result_t {
    CLINGO_TRY { *contained = cpp_cast(model)->contains(cpp_cast(atom)); }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_true(clingo_model_t const *model, clingo_literal_t literal, bool *result)
    -> clingo_result_t {
    CLINGO_TRY { *result = cpp_cast(model)->is_true(literal); }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_consequence(clingo_model_t const *model, clingo_literal_t literal,
                                            clingo_consequence_t *result) -> clingo_result_t {
    CLINGO_TRY { *result = static_cast<clingo_consequence_t>(cpp_cast(model)->is_consequence(literal)); }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_cost_size(clingo_model_t const *model, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        *size = 0;
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_cost(clingo_model_t const *model, int64_t *costs, size_t size) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        if (size > 0) {
            *costs = 0;
        }
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_priority(clingo_model_t const *model, clingo_weight_t *priorities, size_t size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        if (size > 0) {
            *priorities = 0;
        }
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_optimality_proven(clingo_model_t const *model, bool *proven) -> clingo_result_t {
    CLINGO_TRY {
        *proven = true;
        static_cast<void>(model);
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_thread_id(clingo_model_t const *model, clingo_id_t *id) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        *id = 0;
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        static_cast<void>(symbols);
        static_cast<void>(size);
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_context(clingo_model_t const *model, clingo_solve_control_t **control) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(model);
        *control = nullptr;
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_symbolic_atoms(clingo_solve_control_t const *control,
                                                    clingo_symbolic_atoms_t const **atoms) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(control);
        *atoms = nullptr;
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_add_clause(clingo_solve_control_t *control, clingo_literal_t const *clause,
                                                size_t size) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(control);
        static_cast<void>(clause);
        static_cast<void>(size);
        throw std::runtime_error("implement me!!!");
    }
    CLINGO_CATCH;
}
