#include <clingo/solve.h>

#include "lib.hh"

extern "C" auto clingo_solve_handle_get(clingo_solve_handle_t *handle, clingo_solve_result_bitset_t *result)
    -> clingo_result_t {
    CLINGO_TRY {
        *result = clingo_solve_result_interrupted;
        static_cast<void>(handle);
        throw std::logic_error("implement me: get");
    }
    CLINGO_CATCH;
}

extern "C" void clingo_solve_handle_wait(clingo_solve_handle_t *handle, double timeout, bool *result) {
    static_cast<void>(handle);
    static_cast<void>(timeout);
    printf("implement me: wait");
    *result = false;
}

extern "C" auto clingo_solve_handle_model(clingo_solve_handle_t *handle, clingo_model_t const **model)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        *model = nullptr;
        throw std::logic_error("implement me: model");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_core(clingo_solve_handle_t *handle, clingo_literal_t const **core, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        *core = nullptr;
        *size = 0;
        throw std::logic_error("implement me: core");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_last(clingo_solve_handle_t *handle, clingo_model_t const **model)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        *model = nullptr;
        throw std::logic_error("implement me: last");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_resume(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        throw std::logic_error("implement me: resume");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_cancel(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        throw std::logic_error("implement me: cancel");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_solve_handle_close(clingo_solve_handle_t *handle) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(handle);
        throw std::logic_error("implement me: close");
    }
    CLINGO_CATCH;
}
