#include <clingo/control/solver.hh>

#include <clingo/model.h>

#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

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

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_model_type(clingo_model_t const *model, clingo_model_type_t *type) -> bool {
    CLINGO_TRY {
        require(not_null(model, type));
        *type = static_cast<clingo_model_type_t>(cpp_cast(model)->type());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_number(clingo_model_t const *model, uint64_t *number) -> bool {
    CLINGO_TRY {
        require(not_null(model, number));
        *number = cpp_cast(model)->number();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_symbols(clingo_model_t const *model, clingo_show_type_bitset_t show,
                                     clingo_symbol_callback_t callback, void *data) -> bool {
    CLINGO_TRY {
        require(not_null(model, callback) &&
                (show & ~static_cast<clingo_show_type_bitset_t>(clingo_show_type_all)) == 0);
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
        require(not_null(model, contained));
        *contained = cpp_cast(model)->contains(cpp_cast(atom));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_true(clingo_model_t const *model, clingo_literal_t literal, bool *result) -> bool {
    CLINGO_TRY {
        require(not_null(model, result));
        *result = cpp_cast(model)->is_true(literal);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_is_consequence(clingo_model_t const *model, clingo_literal_t literal,
                                            clingo_consequence_t *result) -> bool {
    CLINGO_TRY {
        require(not_null(model, result));
        *result = static_cast<clingo_consequence_t>(cpp_cast(model)->is_consequence(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_cost(clingo_model_t const *model, int64_t const **costs, size_t *size) -> bool {
    CLINGO_TRY {
        require(model != nullptr);
        auto res = cpp_cast(model)->costs();
        set_opt(size, res.size());
        set_opt(costs, res.data());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_priority(clingo_model_t const *model, clingo_weight_t const **priorities, size_t *size)
    -> bool {
    CLINGO_TRY {
        require(model != nullptr);
        auto res = cpp_cast(model)->priorities();
        set_opt(size, res.size());
        set_opt(priorities, res.data());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_optimality_proven(clingo_model_t const *model, bool *proven) -> bool {
    CLINGO_TRY {
        require(not_null(model, proven));
        *proven = cpp_cast(model)->optimality_proven();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_thread_id(clingo_model_t const *model, clingo_id_t *id) -> bool {
    CLINGO_TRY {
        require(not_null(model, id));
        *id = cpp_cast(model)->thread_id();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size) -> bool {
    CLINGO_TRY {
        require(model != nullptr && valid_span(symbols, size));
        cpp_cast(model)->extend({cpp_cast(symbols), size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_model_control(clingo_model_t *model, clingo_solve_control_t **control) -> bool {
    CLINGO_TRY {
        require(not_null(model, control));
        // NOLINTNEXTLINE
        *control = c_cast(&cpp_cast(model)->context());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_base(clingo_solve_control_t const *control, clingo_base_t const **base) -> bool {
    CLINGO_TRY {
        require(not_null(control, base));
        // NOLINTBEGIN
        *base = reinterpret_cast<clingo_base_t const *>(cpp_cast(control));
        // NOLINTEND
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_control_add_clause(clingo_solve_control_t *control, clingo_literal_t const *clause,
                                                size_t size) -> bool {
    CLINGO_TRY {
        require(control != nullptr && valid_span(clause, size));
        cpp_cast(control)->add_clause({clause, size});
    }
    CLINGO_CATCH;
}
