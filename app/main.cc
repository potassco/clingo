#include <gringo/grounder/grounder.hh>

#include <CLI/CLI.hpp>

#include <iostream>

using namespace Gringo::Input;

enum class AppMode { parse, rewrite, ground };

// NOLINTNEXTLINE(modernize-avoid-c-arrays)
auto run(int argc, char *argv[]) -> int {
    auto opts = RewriteOptions{};
    auto mode = AppMode::ground;
    std::vector<std::string> files;
    auto log_level = Gringo::LogLevel::info;

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

    app.add_option("files", files, "files to parse");
    // later...
    // ->check(CLI::ExistingFile);
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
        Gringo::Grounder grd{log, *store, opts};
        [&]() {
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
            grd.ground(Gringo::Input::ProgramParamVec{{store->string("base"), {}}});
        }();
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return 1;
    }
    return log.has_error() ? 1 : 0;
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
