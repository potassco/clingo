#include <iostream>

#include <CLI/CLI.hpp>

#include <gringo/logger.hh>

#include <gringo/input/program.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

using namespace Gringo::Input;

auto run(std::string program, std::vector<std::string> args) -> bool {
    auto opts = RewriteOptions{};
    bool parse_only = false;
    auto log_level = Gringo::LogLevel::info;

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

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
        app.parse(args);
    } catch (CLI::ParseError const &e) {
        std::cout << e.what();
        return false;
    }

    auto log = Gringo::Logger{};
    log.enable_color(false);
    try {
        std::optional<UnprocessedProgram> uprg;
        if (!parse_only) {
            uprg.emplace();
        }
        auto store = Gringo::make_symbol_store(true, false);
        log.set_level(log_level);
        GRINGO_REPORT(log, debug) << "starting up";
        auto scanner = scan_string(log, *store, program);
        for (auto stm = scanner.scan(); stm; stm = scanner.scan()) {
            if (uprg) {
                add(*store, std::move(stm).value(), *uprg);
            } else {
                std::cout << *stm << "\n";
            }
        }
        if (uprg) {
            Program prg{opts};
            prg.join(log, *store, std::move(uprg).value());
            prg.visit_stms(*store, [](auto const &stm) { std::cout << stm << "\n"; });
        }
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return false;
    }
    return !log.has_error();
}

#ifdef CLINGO_BUILD_WEB
#include <emscripten/bind.h>
EMSCRIPTEN_BINDINGS(module) {
    using namespace emscripten;
    function("run", &run);
    register_vector<std::string>("StringVec");
}
#endif
