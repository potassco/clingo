#include <clingo/script.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

namespace {

class CScript : public CppClingo::Control::Script {
  public:
    CScript(clingo_lib *lib, clingo_script_t script, void *data) : lib_{lib}, script_{script}, data_{data} {}
    ~CScript() noexcept override {
        if (script_.free != nullptr) {
            script_.free(data_);
        }
    }

  private:
    void do_exec(std::string_view code) override { script_.execute(code.data(), code.size(), data_); }

    void do_main(CppClingo::Control::Solver &slv) override {
        class main_guard {
          public:
            main_guard(CppClingo::Control::Solver &slv) : slv_{&slv} { slv_->block_main(true); }
            ~main_guard() { slv_->block_main(false); }

          private:
            CppClingo::Control::Solver *slv_;
        } guard{slv};
        auto *ctl = static_cast<clingo_control_t *>(slv.user_data());
        handle_error(script_.main(lib_, ctl, data_));
    }

    auto do_callable(std::string_view name, size_t args) -> bool override {
        bool res = true;
        handle_error(script_.callable(name.data(), name.size(), args, &res, data_));
        return res;
    }

    using CBData = std::pair<CScript *, CppClingo::SymbolVec &>;

    static auto cb(clingo_symbol_t const *symbols, size_t symbols_size, void *data) -> bool {
        auto &[self, out] = *static_cast<CBData *>(data);
        CLINGO_TRY {
            append_n(symbols, symbols_size, out, [](auto sym) { return CppClingo::Symbol::from_rep(sym); });
        }
        CLINGO_CATCH;
    }

    void do_call(CppClingo::Location const &loc, std::string_view name, CppClingo::SymbolSpan args,
                 CppClingo::SymbolVec &out) override {
        auto data = CBData{this, out};
        handle_error(script_.call(lib_, c_cast(&loc), name.data(), name.size(), c_cast(args.data()), args.size(), &cb,
                                  &data, data_));
    }

    clingo_lib_t *lib_;
    clingo_script_t script_;
    void *data_;
};

} // namespace

extern "C" auto clingo_script_register(clingo_lib_t *lib, clingo_script_t const *script, void *data) -> bool {
    CLINGO_TRY {
        clingo_string_t name;
        script->name(data, &name);
        lib->scripts.register_script({name.data, name.size}, std::make_unique<CScript>(lib, *script, data));
    }
    CLINGO_CATCH;
}
