#include <clingo/control/solver.hh>

#include "lib.hh"

// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-pointer-arithmetic)

template <class It, class Pred> auto make_vec(It begin, It end, Pred pred) {
    auto p = std::vector<std::invoke_result_t<Pred, typename std::iterator_traits<It>::value_type>>{};
    p.reserve(std::distance(begin, end));
    std::transform(begin, end, std::back_inserter(p), pred);
    return p;
}

template <typename In, typename C, typename Pred> auto append_n(In begin, size_t n, C out, Pred pred) {
    out.reserve(out.size() + n);
    for (auto end = begin + n; begin != end; ++begin) {
        out.emplace_back(pred(*begin));
    }
}

auto to_c_sym(Clingo::Symbol const *sym) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return reinterpret_cast<clingo_symbol_t const *>(sym);
}

struct clingo_control {
    clingo_lib_t *lib;
    Clingo::Control::Solver *slv;
};

extern "C" auto clingo_control_new(clingo_lib_t *lib, char const *const *arguments, size_t arguments_size,
                                   clingo_control_t **control) -> clingo_result_t {
    CLINGO_TRY {
        // for now could use the main stuff
        static_cast<void>(arguments);
        static_cast<void>(arguments_size);
        auto opts = Clingo::Input::RewriteOptions{};
        auto slv = std::make_unique<Clingo::Control::Solver>(lib->log, *lib->store, lib->scripts, opts,
                                                             Clingo::Control::OutputMode::text);
        *control = new clingo_control{lib, nullptr};
        (*control)->slv = slv.release();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_control_free(clingo_control_t *control) {
    delete control->slv;
    delete control;
}

extern "C" auto clingo_control_parse_files(clingo_control_t *control, char const **files,
                                           size_t files_size) -> clingo_result_t {
    CLINGO_TRY { control->slv->parse(std::vector<std::string_view>{files, files + files_size}); }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_parse_string(clingo_control_t *control, char const *program) -> clingo_result_t {
    CLINGO_TRY { control->slv->parse(program); }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts,
                                      size_t parts_size) -> clingo_result_t {
    CLINGO_TRY {
        auto make_part = [&](auto const &sym) { return Clingo::SharedSymbol{Clingo::Symbol::from_rep(sym)}; };
        auto make_parts = [&](auto const &part) {
            return Clingo::Input::ProgramParam{control->lib->store->string(part.name),
                                               make_vec(part.params, part.params + part.size, make_part)};
        };
        std::ignore = control->slv->ground(make_vec(parts, parts + parts_size, make_parts));
    }
    CLINGO_CATCH;
}

namespace {

class CScript : public Clingo::Control::Script {
  public:
    CScript(clingo_lib *lib, clingo_script_t script, void *data) : lib_{lib}, script_{script}, data_{data} {}

  private:
    void do_exec(std::string_view code) override { script_.execute(nullptr, std::string(code).c_str(), data_); }

    void do_main(Clingo::Control::Solver &slv) override {
        clingo_control_t ctl{lib_, &slv};
        handle_error(script_.main(&ctl, data_));
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

    void do_call(std::string_view name, Clingo::SymbolSpan args, Clingo::SymbolVec &out) override {
        auto data = CBData{this, out};
        handle_error(
            script_.call(nullptr, std::string(name).c_str(), to_c_sym(args.data()), args.size(), &cb, &data, data_));
    }

    clingo_lib_t *lib_;
    clingo_script_t script_;
    void *data_;
};

} // namespace

CLINGO_VISIBILITY_DEFAULT auto clingo_script_register(clingo_lib_t *lib, clingo_script_t const *script,
                                                      void *data) -> clingo_result_t {
    CLINGO_TRY {
        auto const *name = script->name(data);
        lib->scripts.register_script(name, std::make_unique<CScript>(lib, *script, data));
    }
    CLINGO_CATCH;
}

// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-bounds-pointer-arithmetic)
