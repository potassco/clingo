#include <gringo/grounder/grounder.hh>

#include <gringo/input/rewrite/parse.hh>

#include <gringo/output/text.hh>

#include <CLI/CLI.hpp>

#include <iostream>

using namespace Gringo::Input;

enum class AppMode : uint8_t { parse, rewrite, ground };

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
auto run(int argc, char *argv[]) -> int {
    auto opts = RewriteOptions{};
    auto mode = AppMode::ground;
    std::vector<std::string> files;
    std::vector<std::string> const_defs;
    auto log_level = Gringo::LogLevel::info;
    auto params_str = std::optional<std::string>{};

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

    app.add_option("files", files, "files to parse");
    // later...
    // ->check(CLI::ExistingFile);
    app.add_option_no_stream("--const,-c", const_defs, "constant definition");
    app.add_option_no_stream("--params", params_str, "program parts to ground");
    app.add_option("--log-level", "{error,warn,info,debug,trace}")->check([&log_level](std::string const &value) {
        using P = std::pair<char const *, Gringo::LogLevel>;
        auto levels = std::array{P{"trace", Gringo::LogLevel::trace}, P{"debug", Gringo::LogLevel::debug},
                                 P{"info", Gringo::LogLevel::info}, P{"warn", Gringo::LogLevel::warn},
                                 P{"error", Gringo::LogLevel::error}};
        for (auto &[name, level] : levels) {
            if (value == name) {
                log_level = level;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_option("--projection-mode", "{off,anonymous,pure}")->check([&opts](std::string const &value) {
        using P = std::pair<char const *, Gringo::Input::ProjectionMode>;
        auto levels = std::array{P{"off", Gringo::Input::ProjectionMode::disabled},
                                 P{"anonymous", Gringo::Input::ProjectionMode::anonymous},
                                 P{"pure", Gringo::Input::ProjectionMode::pure}};
        for (auto &[name, mode] : levels) {
            if (value == name) {
                opts.project_mode = mode;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
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
    app.add_flag("--project-anonymous", opts.project_anonymous, "project anoymous variables in negated literals");
    try {
        app.parse(argc, argv);
    } catch (CLI::ParseError const &e) {
        return app.exit(e);
    }

    auto log = Gringo::Logger{};
    try {
        log.set_level(log_level);
        auto store = Gringo::make_symbol_store(true, false);
        auto out = Gringo::Output::make_text_output(std::cout);
        Gringo::Grounder grd{log, *store, opts, *out};
        [&]() {
            auto params = std::optional<std::vector<Gringo::Input::ProgramParamVec>>();
            if (params_str) {
                params = Gringo::Input::parse_parts(log, *store, *params_str);
            }
            for (auto const &str : const_defs) {
                auto def = parse_const(log, *store, str);
                grd.add_const(*def.first, *def.second);
            }
            grd.parse(files);
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
                std::ignore = grd.ground(Gringo::Input::ProgramParamVec{{store->string("base"), {}}});
            }
        }();
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return 1;
    }
    return 0;
}

auto main(int argc, char *argv[]) -> int {
    try {
        return run(argc, argv);
    } catch (std::exception const &e) {
        fprintf(stderr, "panic: %s\n", "unrecoverable error during startup");
        fflush(stderr);
        return 1;
    }
}
