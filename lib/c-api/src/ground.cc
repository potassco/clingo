#include <clingo/util/algorithm.hh>

#include <clingo/control.h>
#include <clingo/script.h>

#include <clasp/cli/clasp_options.h>

#include <potassco/program_opts/program_options.h>
#include <potassco/program_opts/typed_value.h>

#include "control.hh"
#include "lib.hh"

namespace CppClingo::CAPI {
namespace {

class Context : public CppClingo::Ground::ScriptCallback {
  public:
    Context(clingo_lib_t *lib, clingo_ground_callback_t cb, void *data) : lib_{lib}, cb_{cb}, data_{data} {}

  private:
    auto do_callable([[maybe_unused]] std::string_view name, [[maybe_unused]] size_t args) -> bool override {
        return true;
    }

    void do_call(CppClingo::Location const &loc, std::string_view name, CppClingo::SymbolSpan args,
                 CppClingo::SymbolVec &out) override {
        auto c_name = std::string{name};
        handle_error(cb_(lib_, c_cast(&loc), c_name.data(), c_name.size(), c_cast(args.data()), args.size(), data_,
                         &Context::sym_cb_, &out));
    }

    static auto sym_cb_(clingo_symbol_t const *symbols, size_t symbols_size, void *data) -> bool {
        CLINGO_TRY {
            auto *out = static_cast<CppClingo::SymbolVec *>(data);
            auto const *it = cpp_cast(symbols);
            out->insert(out->end(), it, std::next(it, static_cast<std::ptrdiff_t>(symbols_size)));
        }
        CLINGO_CATCH;
    }

    clingo_lib_t *lib_;
    clingo_ground_callback_t cb_;
    void *data_;
};

//! Struct ensuring that the context lives at least as long as the handle.
struct GroundHandle {
    std::unique_ptr<Context> context;
    Control::GroundHandle handle;
};

auto cpp_cast(clingo_ground_handle_t *hnd) -> GroundHandle * {
    if (hnd == nullptr) {
        throw std::runtime_error("ground handle is null");
    }
    return reinterpret_cast<GroundHandle *>(hnd); // NOLINT
}

auto c_cast(GroundHandle *hnd) -> clingo_ground_handle_t * {
    return reinterpret_cast<clingo_ground_handle_t *>(hnd); // NOLINT
}

} // namespace
} // namespace CppClingo::CAPI

using namespace CppClingo::CAPI;

static_assert(static_cast<clingo_ground_result_e>(CppClingo::GroundResult::ok) == clingo_ground_result_ok);
static_assert(static_cast<clingo_ground_result_e>(CppClingo::GroundResult::unsatisfiable) ==
              clingo_ground_result_unsatisfiable);
static_assert(static_cast<clingo_ground_result_e>(CppClingo::GroundResult::interrupted) ==
              clingo_ground_result_interrupted);

extern "C" auto clingo_ground_handle_get(clingo_ground_handle_t *handle, clingo_ground_result_t *result) -> bool {
    CLINGO_TRY {
        *result = static_cast<clingo_ground_result_t>(cpp_cast(handle)->handle.get());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ground_handle_wait(clingo_ground_handle_t *handle, double timeout, bool *result) -> bool {
    CLINGO_TRY {
        *result = cpp_cast(handle)->handle.wait(timeout);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ground_handle_cancel(clingo_ground_handle_t *handle) -> bool {
    CLINGO_TRY {
        cpp_cast(handle)->handle.cancel();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ground_handle_close(clingo_ground_handle_t *handle) {
    delete cpp_cast(handle);
}

extern "C" auto clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                      clingo_ground_callback_t ground_callback, void *data) -> bool {
    CLINGO_TRY {
        auto ctx = ground_callback != nullptr ? std::make_optional<Context>(control->lib, ground_callback, data)
                                              : std::nullopt;
        control->slv->ground(convert(control, parts, size), ctx ? &ctx.value() : nullptr);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_start_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                            clingo_ground_callback_t ground_callback, void *data,
                                            clingo_ground_handle_t **handle) -> bool {
    CLINGO_TRY {
        auto ctx =
            ground_callback != nullptr ? std::make_unique<Context>(control->lib, ground_callback, data) : nullptr;
        auto hnd = std::make_unique<GroundHandle>(nullptr,
                                                  control->slv->start_ground(convert(control, parts, size), ctx.get()));
        // tie handle and context lifetime
        hnd->context = std::move(ctx);
        *handle = c_cast(hnd.release());
    }
    CLINGO_CATCH;
}
