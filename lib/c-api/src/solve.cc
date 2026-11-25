#include "clingo/solve.h"
#include "clingo/stats.h"

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

auto c_cast(CppClingo::Control::Model const *model) -> clingo_model_t const * {
    return reinterpret_cast<clingo_model_t const *>(model); // NOLINT
}

auto c_cast(CppClingo::Control::Model *model) -> clingo_model_t * {
    return reinterpret_cast<clingo_model_t *>(model); // NOLINT
}

auto c_cast(Potassco::AbstractStatistics *config) -> clingo_stats_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_stats_t *>(config);
}

auto c_cast(CppClingo::Control::SolveHandle *hnd) -> clingo_solve_handle_t * {
    return reinterpret_cast<clingo_solve_handle_t *>(hnd); // NOLINT
}

auto cpp_cast(clingo_solve_handle_t *hnd) -> CppClingo::Control::SolveHandle * {
    return reinterpret_cast<CppClingo::Control::SolveHandle *>(hnd); // NOLINT
}

class SolveEventHandler : public CppClingo::Control::SolveEventHandler {
  public:
    explicit SolveEventHandler(CHandler<clingo_solve_event_handler_t> hnd) noexcept : hnd_{std::move(hnd)} {}

    auto do_on_model(CppClingo::Control::Model &m) -> bool override {
        bool goon = true;
        if (hnd_->model != nullptr) {
            handle_error(hnd_->model(c_cast(&m), hnd_.data(), &goon));
        }
        return goon;
    }

    void do_on_stats(Potassco::AbstractStatistics &stats) override {
        if (hnd_->stats != nullptr) {
            handle_error(hnd_->stats(c_cast(&stats), hnd_.data()));
        }
    }

    void do_on_unsat(Clasp::SumView bound) override {
        if (hnd_->unsat != nullptr) {
            handle_error(hnd_->unsat(bound.data(), bound.size(), hnd_.data()));
        }
    }

    void do_on_finish(CppClingo::Control::SolveResult res) override {
        if (hnd_->finish != nullptr) {
            hnd_->finish(static_cast<clingo_solve_result_bitset_t>(res), hnd_.data());
        }
    }

  private:
    CHandler<clingo_solve_event_handler_t> hnd_;
};

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_solve_handle_get(clingo_solve_handle_t *handle, clingo_solve_result_bitset_t *result) -> bool {
    CLINGO_TRY {
        require(not_null(handle, result));
        *result = static_cast<clingo_solve_result_bitset_t>(cpp_cast(handle)->get());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_wait(clingo_solve_handle_t *handle, double timeout, bool *result) -> bool {
    CLINGO_TRY {
        require(not_null(handle, result));
        *result = cpp_cast(handle)->wait(timeout);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_model(clingo_solve_handle_t *handle, clingo_model_t **model) -> bool {
    CLINGO_TRY {
        require(not_null(handle, model));
        *model = c_cast(cpp_cast(handle)->model());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_core(clingo_solve_handle_t *handle, clingo_literal_t const **literals, size_t *size)
    -> bool {
    CLINGO_TRY {
        require(handle != nullptr);
        auto lits = cpp_cast(handle)->core();
        set_opt(literals, lits.data());
        set_opt(size, lits.size());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_last(clingo_solve_handle_t *handle, clingo_model_t const **model) -> bool {
    CLINGO_TRY {
        require(not_null(handle, model));
        *model = c_cast(cpp_cast(handle)->last());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_resume(clingo_solve_handle_t *handle) -> bool {
    CLINGO_TRY {
        require(handle != nullptr);
        cpp_cast(handle)->resume();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_cancel(clingo_solve_handle_t *handle) -> bool {
    CLINGO_TRY {
        require(handle != nullptr);
        cpp_cast(handle)->cancel();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_interrupt(clingo_solve_handle_t *handle) -> bool {
    CLINGO_TRY {
        require(handle != nullptr);
        cpp_cast(handle)->interrupt();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_solve_handle_close(clingo_solve_handle_t *handle) {
    delete cpp_cast(handle);
}

extern "C" auto clingo_control_solve(clingo_control_t *control, clingo_solve_mode_bitset_t mode,
                                     clingo_literal_t const *assumptions, size_t assumptions_size,
                                     clingo_solve_event_handler_t const *handler, void *data,
                                     clingo_solve_handle_t **handle) -> bool {
    CLINGO_TRY {
        // NOTE: the handler owns the data also if the arguments are invalid
        auto hnd = CHandler{handler, data};
        constexpr auto modes =
            clingo_solve_mode_bitset_t{clingo_solve_mode_async | clingo_solve_mode_yield | clingo_solve_mode_lock};
        require(not_null(control, handle) && valid_span(assumptions, assumptions_size) && (mode & ~modes) == 0);
        *handle = nullptr;
        auto cpp_mode = CppClingo::Control::SolveMode::none;
        if ((mode & clingo_solve_mode_yield) != 0) {
            cpp_mode |= CppClingo::Control::SolveMode::yield;
        }
        if ((mode & clingo_solve_mode_async) != 0) {
            cpp_mode |= CppClingo::Control::SolveMode::async;
        }
        auto cpp_assumptions = std::span{assumptions, assumptions_size};
        auto cpp_eh = CppClingo::Control::USolveEventHandler{};
        if (hnd) {
            cpp_eh = std::make_unique<SolveEventHandler>(std::move(hnd));
        }
        control->slv->get_lock().enable((mode & clingo_solve_mode_lock) != 0);
        *handle = c_cast(control->slv->solve(std::move(cpp_eh), cpp_assumptions, cpp_mode).release());
    }
    CLINGO_CATCH;
}
