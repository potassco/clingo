#include <clingo/control/grounder.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <CLI/CLI.hpp>

#include <iostream>

using namespace Clingo::Input;

enum class AppMode : uint8_t { parse, rewrite, ground };

auto run(std::string const &program, std::vector<std::string> args) -> bool {
    auto opts = RewriteOptions{};
    auto mode = AppMode::ground;
    auto log_level = Clingo::LogLevel::info;
    auto params_str = std::optional<std::string>{};
    std::vector<std::string> const_defs;

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

    app.add_option_no_stream("--const,-c", const_defs, "constant definition");
    app.add_option_no_stream("--params", params_str, "program parts to ground");
    app.add_option("--log-level", "{error,warn,info,debug,trace}")->check([&log_level](std::string const &value) {
        using P = std::pair<char const *, Clingo::LogLevel>;
        auto levels = std::array{P{"trace", Clingo::LogLevel::trace}, P{"debug", Clingo::LogLevel::debug},
                                 P{"info", Clingo::LogLevel::info}, P{"warn", Clingo::LogLevel::warn},
                                 P{"error", Clingo::LogLevel::error}};
        for (auto &[name, level] : levels) {
            if (value == name) {
                log_level = level;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_option("--projection-mode", "{off,anonymous,pure}")->check([&opts](std::string const &value) {
        using P = std::pair<char const *, Clingo::Input::ProjectionMode>;
        auto levels = std::array{P{"off", Clingo::Input::ProjectionMode::disabled},
                                 P{"anonymous", Clingo::Input::ProjectionMode::anonymous},
                                 P{"pure", Clingo::Input::ProjectionMode::pure}};
        for (auto &[name, mode] : levels) {
            if (value == name) {
                opts.project_mode = mode;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_flag("--project-anonymous", opts.project_anonymous, "project anoymous variables in negated literals");
    app.add_option("--mode", "{parse,rewrite,ground}")->check([&mode](std::string const &value) {
        using P = std::pair<char const *, AppMode>;
        auto modes =
            std::array{P{"parse", AppMode::parse}, P{"rewrite", AppMode::rewrite}, P{"ground", AppMode::ground}};
        for (auto &[name, m] : modes) {
            if (name == value) {
                mode = m;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    try {
        app.parse(args);
    } catch (CLI::ParseError const &e) {
        std::cout << e.what();
        return false;
    }

    auto log = Clingo::Logger{};
    try {
        log.enable_color(false);
        log.set_level(log_level);
        auto store = Clingo::make_symbol_store(true, false);
        auto buf = Clingo::Util::OutputBuffer{stdout};
        auto out = Clingo::Output::make_text_output(buf);
        Clingo::Control::Grounder grd{log, *store, opts, *out};
        auto prs = Clingo::Input::Parser{log, *store};

        auto params = std::optional<std::vector<Clingo::Input::ProgramParamVec>>();
        [&]() {
            auto params = std::optional<std::vector<Clingo::Input::ProgramParamVec>>();
            if (params_str) {
                prs.init(*params_str, *store->string("<string>"));
                params = prs.parse_program_parts();
                if (!params) {
                    throw Clingo::parse_error();
                }
            }
            for (auto const &str : const_defs) {
                prs.init(str, *store->string("<string>"));
                auto def = prs.parse_const_def();
                if (!def) {
                    throw Clingo::parse_error();
                }
                grd.add_const(*def->first, *def->second);
            }
            grd.parse(program);
            if (mode == AppMode::parse) {
                grd.output_unprocessed_program(std::cout);
                return;
            }
            grd.prepare();
            if (mode == AppMode::rewrite) {
                grd.output_program(std::cout);
                return;
            }
            if (params) {
                for (auto const &param : *params) {
                    if (!grd.ground(param)) {
                        break;
                    }
                }
            } else {
                std::ignore = grd.ground(Clingo::Input::ProgramParamVec{{store->string("base"), {}}});
            }
        }();
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Clingo::MessageCode::error), e.what());
        fflush(stderr);
        return false;
    }
    return true;
}

#ifdef CLINGO_BUILD_WEB
#include <emscripten/bind.h>
EMSCRIPTEN_BINDINGS(module) {
    using namespace emscripten;
    function("run", &run);
    register_vector<std::string>("StringVec");
}
#endif
