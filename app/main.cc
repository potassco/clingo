#include <gringo/grounder/grounder.hh>

#include <CLI/CLI.hpp>

#include <iostream>

using namespace Gringo::Input;

auto main(int argc, char *argv[]) -> int {
    auto opts = RewriteOptions{};
    bool parse_only = false;
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
    app.add_flag("--project-anonymous", opts.project_anonymous, "project anoymous variables in negated literals");
    app.add_flag("--parse-only", parse_only, "project anoymous variables in negated literals");
    try {
        app.parse(argc, argv);
    } catch (CLI::ParseError const &e) {
        return app.exit(e);
    }

    auto log = Gringo::Logger{};
    try {
        log.set_level(log_level);
        auto store = Gringo::make_symbol_store(true, false);
        Gringo::Grounder grd{log, *store, std::move(opts)};
        grd.parse(files);
        [&]() {
            if (parse_only) {
                grd.output_unprocessed_program(std::cout);
                return;
            }
            grd.prepare();
            grd.output_program(std::cout);
        }();
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return 1;
    }
    return log.has_error() ? 1 : 0;
}
