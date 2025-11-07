#include <clingo/util/algorithm.hh>

#include <clingo/control.h>
#include <clingo/script.h>

#include "control.hh"
#include "lib.hh"

namespace CppClingo::CAPI {
namespace {

class Handler : public CppClingo::Control::GroundEventHandler {
  public:
    Handler(clingo_lib_t *lib, CHandler<clingo_ground_event_handler_t> handler) noexcept
        : lib_{lib}, handler_{std::move(handler)} {}

  private:
    auto do_callable(std::string_view name, size_t args) -> bool override {
        bool result = false;
        if (handler_->callable != nullptr) {
            handle_error(handler_->callable(name.data(), name.size(), args, handler_.data(), &result));
        }
        return result;
    }

    void do_call(CppClingo::Location const &loc, std::string_view name, CppClingo::SymbolSpan args,
                 CppClingo::SymbolVec &out) override {
        if (handler_->call != nullptr) {
            handle_error(handler_->call(lib_, c_cast(&loc), name.data(), name.size(), c_cast(args.data()), args.size(),
                                        handler_.data(), &Handler::sym_cb_, &out));
        } else {
            throw std::logic_error("call function not implemented");
        }
    }

    void do_finish(GroundResult result) noexcept override {
        if (handler_->finish != nullptr) {
            handler_->finish(static_cast<clingo_ground_result_t>(result), handler_.data());
        }
    }

    static auto sym_cb_(clingo_symbol_t const *symbols, size_t symbols_size, void *data) -> bool {
        CLINGO_TRY {
            auto *out = static_cast<CppClingo::SymbolVec *>(data);
            auto const *it = cpp_cast(symbols);
            out->insert(out->end(), it, std::next(it, static_cast<std::ptrdiff_t>(symbols_size)));
        }
        CLINGO_CATCH;
    }

    clingo_lib_t *lib_ = nullptr;
    CHandler<clingo_ground_event_handler_t> handler_;
};

auto cpp_cast(clingo_ground_handle_t *hnd) -> CppClingo::Control::GroundHandle * {
    if (hnd == nullptr) {
        throw std::runtime_error("ground handle is null");
    }
    return reinterpret_cast<CppClingo::Control::GroundHandle *>(hnd); // NOLINT
}

auto c_cast(CppClingo::Control::GroundHandle *hnd) -> clingo_ground_handle_t * {
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
        *result = static_cast<clingo_ground_result_t>(cpp_cast(handle)->get());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ground_handle_wait(clingo_ground_handle_t *handle, double timeout, bool *result) -> bool {
    CLINGO_TRY {
        *result = cpp_cast(handle)->wait(timeout);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ground_handle_cancel(clingo_ground_handle_t *handle) -> bool {
    CLINGO_TRY {
        cpp_cast(handle)->cancel();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ground_handle_close(clingo_ground_handle_t *handle) {
    delete cpp_cast(handle);
}

extern "C" auto clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                      clingo_ground_event_handler_t const *handler, void *data) -> bool {
    CLINGO_TRY {
        auto ctx = std::optional<Handler>{};
        if (auto hnd = CHandler{handler, data}; hnd) {
            ctx.emplace(control->lib, std::move(hnd));
        }
        control->slv->ground(convert(control, parts, size), ctx ? &*ctx : nullptr);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_start_ground(clingo_control_t *control, clingo_part_t const *parts, size_t size,
                                            clingo_ground_event_handler_t const *handler, void *data,
                                            clingo_ground_handle_t **handle) -> bool {
    CLINGO_TRY {
        auto hnd = CHandler{handler, data};
        auto ctx = hnd ? std::make_unique<Handler>(control->lib, std::move(hnd)) : nullptr;
        *handle = c_cast(std::make_unique<CppClingo::Control::GroundHandle>(
                             control->slv->start_ground(convert(control, parts, size), std::move(ctx)))
                             .release());
    }
    CLINGO_CATCH;
}
