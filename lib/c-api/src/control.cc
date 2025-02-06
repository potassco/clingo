#include <clingo/util/algorithm.hh>

#include <clingo/control.h>
#include <clingo/script.h>

#include <clasp/cli/clasp_options.h>

#include <potassco/program_opts/program_options.h>
#include <potassco/program_opts/typed_value.h>

#include "ast.hh" // IWYU pragma: keep
#include "control.hh"
#include "lib.hh"

extern "C" auto clingo_control_new(clingo_lib_t *lib, char const *const *arguments, size_t arguments_size,
                                   clingo_control_t **control) -> clingo_result_t {
    CLINGO_TRY {
        using namespace Potassco::ProgramOptions;

        auto mode = Clingo::Control::AppMode::solve;
        auto grd_cfg = Clingo::Input::RewriteOptions{};
        auto slv_cfg = std::make_unique<Clasp::Cli::ClaspCliConfig>();
        auto clasp = std::make_unique<Clasp::ClaspFacade>();

        // parse options
        auto ctx = OptionContext{"<libclingo>"};
        auto group_basic = OptionGroup{"Basic Options"};
        group_basic.addOptions() //
            ("mode",
             storeTo(mode = Clingo::Control::AppMode::solve, values<Clingo::Control::AppMode>({
                                                                 {"parse", Clingo::Control::AppMode::parse},
                                                                 {"rewrite", Clingo::Control::AppMode::rewrite},
                                                                 {"ground", Clingo::Control::AppMode::ground},
                                                                 {"solve", Clingo::Control::AppMode::solve},
                                                             })),
             "Run in {parse|rewrite|ground|solve} mode");
        ctx.add(group_basic);
        slv_cfg->addOptions(ctx);
        auto pos_parser = [](const std::string &str, std::string &out) {
            int value = 0;
            auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
            if (ec == std::errc()) {
                out = "number";
                return true;
            }
            return false;
        };
        auto values = parseCommandArray(arguments, static_cast<int>(arguments_size), ctx, false, pos_parser);
        auto parsed = ParsedOptions{};
        parsed.assign(values);
        ctx.assignDefaults(parsed);
        slv_cfg->finalize(parsed, Clasp::ProblemType::asp, true);

        // setup control
        if (mode == Clingo::Control::AppMode::solve) {
            clasp->startAsp(*slv_cfg, true);
        }
        auto slv = std::make_unique<Clingo::Control::Solver>(*clasp, lib->log, *lib->store, lib->scripts, grd_cfg, mode,
                                                             nullptr);
        *control = new clingo_control{lib, nullptr, nullptr, nullptr};
        (*control)->cfg = slv_cfg.release();
        (*control)->clasp = clasp.release();
        (*control)->slv = slv.release();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_control_free(clingo_control_t *control) {
    delete control->slv;
    delete control->clasp;
    delete control->cfg;
    delete control;
}

extern "C" auto clingo_control_parse_files(clingo_control_t *control, char const **files, size_t files_size)
    -> clingo_result_t {
    CLINGO_TRY { control->slv->parse(std::vector<std::string_view>{files, files + files_size}); }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_parse_string(clingo_control_t *control, char const *program) -> clingo_result_t {
    CLINGO_TRY { control->slv->parse(program); }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_join(clingo_control_t *control, clingo_program_t const *program) -> clingo_result_t {
    CLINGO_TRY { control->slv->join(program->program); }
    CLINGO_CATCH;
}

namespace {

class Context : public Clingo::Ground::ScriptCallback {
  public:
    Context(clingo_lib_t *lib, clingo_ground_callback_t cb, void *data) : lib_{lib}, cb_{cb}, data_{data} {}

  private:
    auto do_callable([[maybe_unused]] std::string_view name, [[maybe_unused]] size_t args) -> bool override {
        return true;
    }

    void do_call(Clingo::Location const &loc, std::string_view name, Clingo::SymbolSpan args,
                 Clingo::SymbolVec &out) override {
        auto c_name = std::string{name};
        cb_(lib_, c_cast(&loc), c_name.c_str(), c_cast(args.data()), args.size(), data_, &Context::sym_cb_, &out);
    }

    static auto sym_cb_(clingo_symbol_t const *symbols, size_t symbols_size, void *data) -> clingo_result_t {
        CLINGO_TRY {
            auto *out = static_cast<Clingo::SymbolVec *>(data);
            auto const *it = cpp_cast(symbols);
            out->insert(out->end(), it, std::next(it, static_cast<ssize_t>(symbols_size)));
        }
        CLINGO_CATCH;
    }

    clingo_lib_t *lib_;
    clingo_ground_callback_t cb_;
    void *data_;
};

} // namespace

extern "C" auto clingo_control_ground(clingo_control_t *control, clingo_part_t const *parts, size_t parts_size,
                                      clingo_ground_callback_t ground_callback, void *ground_callback_data)
    -> clingo_result_t {
    CLINGO_TRY {
        auto ctx = ground_callback != nullptr
                       ? std::make_optional<Context>(control->lib, ground_callback, ground_callback_data)
                       : std::nullopt;
        auto make_part = [&](auto const &sym) { return Clingo::SharedSymbol{Clingo::Symbol::from_rep(sym)}; };
        auto make_parts = [&](auto const &part) {
            return Clingo::Input::ProgramParam{
                control->lib->store->string(part.name),
                Clingo::Util::transform(part.params, part.params + part.size, make_part)};
        };
        control->slv->ground(Clingo::Util::transform(parts, parts + parts_size, make_parts),
                             ctx ? &ctx.value() : nullptr);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_main(clingo_control_t *control) -> clingo_result_t {
    CLINGO_TRY { control->slv->main(std::nullopt); }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_buffer(clingo_control_t *control, char const **buffer) -> clingo_result_t {
    CLINGO_TRY { *buffer = control->slv->buf().c_str(); }
    CLINGO_CATCH;
}
