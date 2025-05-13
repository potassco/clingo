#include <clingo/control/solver.hh>

#include <clingo/model.h>

#include "lib.hh"

auto cpp_cast(clingo_model_t *model) -> CppClingo::Control::Model * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Control::Model *>(model);
}

auto cpp_cast(clingo_model_t const *model) -> CppClingo::Control::Model const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Control::Model const *>(model);
}

auto cpp_cast(clingo_solve_control_t *control) -> CppClingo::Control::SolveControl * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Control::SolveControl *>(control);
}

auto cpp_cast(clingo_solve_control_t const *control) -> CppClingo::Control::SolveControl const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Control::SolveControl const *>(control);
}

auto c_cast(CppClingo::Control::SolveControl *control) -> clingo_solve_control_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_solve_control_t *>(control);
}

extern "C" auto clingo_model_type(clingo_model_t const *model, clingo_model_type_t *type) -> bool {
    CLINGO_TRY {
        *type = static_cast<clingo_model_type_t>(cpp_cast(model)->type());
    }
    CLINGO_CATCH;
}
extern "C" auto clingo_model_number(clingo_model_t const *model, uint64_t *number) -> bool {
    CLINGO_TRY {
        *number = cpp_cast(model)->number();
    }
    CLINGO_CATCH;
}
extern "C" auto clingo_model_symbols(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                     clingo_symbol_callback_t callback, void *data) -> bool {
    CLINGO_TRY {
        auto const *mdl = cpp_cast(model);
        auto flags = static_cast<CppClingo::Control::SymbolSelectFlags>(show);
        CppClingo::SymbolVec res;
        mdl->symbols(flags, res);
        handle_error(callback(c_cast(res.data()), res.size(), data));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_contains(clingo_model_t const *model, clingo_symbol_t atom, bool *contained) -> bool {
    CLINGO_TRY {
        *contained = cpp_cast(model)->contains(cpp_cast(atom));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_true(clingo_model_t const *model, clingo_literal_t literal, bool *result) -> bool {
    CLINGO_TRY {
        *result = cpp_cast(model)->is_true(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_consequence(clingo_model_t const *model, clingo_literal_t literal,
                                            clingo_consequence_t *result) -> bool {
    CLINGO_TRY {
        *result = static_cast<clingo_consequence_t>(cpp_cast(model)->is_consequence(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_cost(clingo_model_t const *model, int64_t const **costs, size_t *size) -> bool {
    CLINGO_TRY {
        auto res = cpp_cast(model)->costs();
        *size = res.size();
        *costs = res.data();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_priority(clingo_model_t const *model, clingo_weight_t const **priorities, size_t *size)
    -> bool {
    CLINGO_TRY {
        auto res = cpp_cast(model)->priorities();
        *size = res.size();
        *priorities = res.data();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_optimality_proven(clingo_model_t const *model, bool *proven) -> bool {
    CLINGO_TRY {
        *proven = cpp_cast(model)->optimality_proven();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_thread_id(clingo_model_t const *model, clingo_id_t *id) -> bool {
    CLINGO_TRY {
        *id = cpp_cast(model)->thread_id();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size) -> bool {
    CLINGO_TRY {
        cpp_cast(model)->extend({cpp_cast(symbols), size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_control(clingo_model_t *model, clingo_solve_control_t **control) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *control = c_cast(&cpp_cast(model)->context());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_base(clingo_solve_control_t const *control, clingo_base_t const **base) -> bool {
    CLINGO_TRY {
        if (control == nullptr || base == nullptr) {
            return fail_arguments();
        }
        // NOLINTBEGIN
        *base = reinterpret_cast<clingo_base_t const *>(cpp_cast(control));
        // NOLINTEND
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_add_clause(clingo_solve_control_t *control, clingo_literal_t const *clause,
                                                size_t size) -> bool {
    CLINGO_TRY {
        cpp_cast(control)->add_clause({clause, size});
    }
    CLINGO_CATCH;
}
