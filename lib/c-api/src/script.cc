#include <clingo/script.h>

#include "control.hh"
#include "lib.hh"

namespace {

class CScript : public Clingo::Control::Script {
  public:
    CScript(clingo_lib *lib, clingo_script_t script, void *data) : lib_{lib}, script_{script}, data_{data} {}
    ~CScript() noexcept override { script_.free(data_); }

  private:
    void do_exec(std::string_view code) override { script_.execute(std::string(code).c_str(), data_); }

    void do_main(Clingo::Control::Solver &slv) override {
        clingo_control_t ctl{lib_, nullptr, nullptr, &slv};
        handle_error(script_.main(lib_, &ctl, data_));
    }

    auto do_callable(std::string_view name, size_t args) -> bool override {
        bool res = true;
        handle_error(script_.callable(std::string(name).c_str(), args, &res, data_));
        return res;
    }

    using CBData = std::pair<CScript *, Clingo::SymbolVec &>;

    static auto cb(clingo_symbol_t const *symbols, size_t symbols_size, void *data) -> clingo_result_t {
        auto &[self, out] = *static_cast<CBData *>(data);
        CLINGO_TRY {
            append_n(symbols, symbols_size, out, [](auto sym) { return Clingo::Symbol::from_rep(sym); });
        }
        CLINGO_CATCH;
    }

    void do_call(Clingo::Location const &loc, std::string_view name, Clingo::SymbolSpan args,
                 Clingo::SymbolVec &out) override {
        auto data = CBData{this, out};
        handle_error(script_.call(lib_, c_cast(&loc), std::string(name).c_str(), c_cast(args.data()), args.size(), &cb,
                                  &data, data_));
    }

    clingo_lib_t *lib_;
    clingo_script_t script_;
    void *data_;
};

} // namespace

extern "C" auto clingo_script_register(clingo_lib_t *lib, clingo_script_t const *script, void *data)
    -> clingo_result_t {
    CLINGO_TRY {
        auto const *name = script->name(data);
        lib->scripts.register_script(name, std::make_unique<CScript>(lib, *script, data));
    }
    CLINGO_CATCH;
}
