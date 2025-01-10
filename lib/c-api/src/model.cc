#include <clingo/control/solver.hh>

#include <clingo/model.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

auto cpp_cast(clingo_model_t const *model) -> Clingo::Control::Model const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::Model const *>(model);
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
