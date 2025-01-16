#include <clingo/solve.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

auto c_cast(Clingo::Control::Model const *model) -> clingo_model_t const * {
    return reinterpret_cast<clingo_model_t const *>(model); // NOLINT
}

auto c_cast(Clingo::Control::SolveHandle *hnd) -> clingo_solve_handle_t * {
    return reinterpret_cast<clingo_solve_handle_t *>(hnd); // NOLINT
}

auto cpp_cast(clingo_solve_handle_t *hnd) -> Clingo::Control::SolveHandle * {
    return reinterpret_cast<Clingo::Control::SolveHandle *>(hnd); // NOLINT
}

extern "C" auto clingo_solve_handle_get(clingo_solve_handle_t *handle, clingo_solve_result_bitset_t *result)
    -> clingo_result_t {
    CLINGO_TRY { *result = static_cast<clingo_solve_result_bitset_t>(cpp_cast(handle)->get()); }
    CLINGO_CATCH;
}

extern "C" void clingo_solve_handle_wait(clingo_solve_handle_t *handle, double timeout, bool *result) {
    *result = cpp_cast(handle)->wait(timeout);
}

extern "C" auto clingo_solve_handle_model(clingo_solve_handle_t *handle, clingo_model_t const **model)
    -> clingo_result_t {
    CLINGO_TRY { *model = c_cast(cpp_cast(handle)->model()); }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_core(clingo_solve_handle_t *handle, clingo_literal_t const **literals, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        auto lits = cpp_cast(handle)->core();
        *literals = lits.data();
        *size = lits.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_last(clingo_solve_handle_t *handle, clingo_model_t const **model)
    -> clingo_result_t {
    CLINGO_TRY { *model = c_cast(cpp_cast(handle)->model()); }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_resume(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY { cpp_cast(handle)->resume(); }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_cancel(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY { cpp_cast(handle)->cancel(); }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_close(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY { delete cpp_cast(handle); }
    CLINGO_CATCH;
}

namespace {

class SolveEventHandler : public Clingo::Control::EventHandler {
  public:
    SolveEventHandler(clingo_solve_event_callback_t notify, void *data) : notify_{notify}, data_{data} {}

    auto do_on_model([[maybe_unused]] Clingo::Control::Model &m) -> bool override {
        bool goon = true;
        handle_error(notify_(clingo_solve_event_type_model, &m, data_, &goon));
        return goon;
    }

  private:
    clingo_solve_event_callback_t notify_;
    void *data_;
};

} // namespace

extern "C" auto clingo_control_solve(clingo_control_t *control, clingo_solve_mode_bitset_t mode,
                                     clingo_literal_t const *assumptions, size_t assumptions_size,
                                     clingo_solve_event_callback_t notify, void *data, clingo_solve_handle_t **handle)
    -> clingo_result_t {
    CLINGO_TRY {
        *handle = nullptr;
        if ((mode & clingo_solve_mode_yield) != 0) {
            throw std::runtime_error("implement me: yield");
        }
        if ((mode & clingo_solve_mode_async) != 0) {
            throw std::runtime_error("implement me: async");
        }
        if (assumptions_size > 0) {
            static_cast<void>(assumptions);
            throw std::runtime_error("implement me: assumptions");
        }
        if (notify != nullptr) {
            auto eh = std::make_unique<SolveEventHandler>(notify, data);
            *handle = c_cast(control->slv->solve(std::move(eh)).release());
        } else {
            *handle = c_cast(control->slv->solve(nullptr).release());
        }
    }
    CLINGO_CATCH;
}
