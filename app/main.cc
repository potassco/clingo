#include <iostream>

#include <CLI/CLI.hpp>

#include <logger.hh>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

using namespace Gringo::Input;

template <class Scanner> void process(Gringo::SymbolStore &store, Scanner &&scanner, UnprocessedProgram &prg) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        prg.add(store, std::move(stm).value());
    }
}

auto main() -> int {
    auto opts = RewriteOptions{};
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
    app.add_option("--rewrite-level", "{off,anonymous,unpool,project}")->check([&opts](std::string const &value) {
        using P = std::pair<char const *, Gringo::Input::RewriteLevel>;
        auto levels = std::array{P{"off", Gringo::Input::RewriteLevel::disabled},
                                 P{"anonymous", Gringo::Input::RewriteLevel::rewrite_anonymous},
                                 P{"unpool", Gringo::Input::RewriteLevel::unpool},
                                 P{"project", Gringo::Input::RewriteLevel::project},
                                 P{"simplify", Gringo::Input::RewriteLevel::simplify}};
        for (auto &[name, level] : levels) {
            if (value == name) {
                opts.level = level;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_option("--projection-mode", "{off,anonymous,pure}")->check([&opts](std::string const &value) {
        using P = std::pair<char const *, Gringo::Input::ProjectionMode>;
        auto levels = std::array{P{"off", Gringo::Input::ProjectionMode::disabled},
                                 P{"anonymous", Gringo::Input::ProjectionMode::anonymous},
                                 P{"unpool", Gringo::Input::ProjectionMode::pure}};
        for (auto &[name, mode] : levels) {
            if (value == name) {
                opts.project_mode = mode;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_flag("--project-anonymous", opts.project_anonymous, "project anoymous variables in negated literals");
    try {
        app.parse();
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    UnprocessedProgram uprg;
    auto log = Gringo::Logger{};
    auto store = Gringo::make_symbol_store(true, false);
    log.set_level(log_level);
    GRINGO_REPORT(log, debug) << "starting up";
    if (files.empty()) {
        process(*store, scan_stream(log, *store, std::cin), uprg);
    } else {
        for (auto const &file : files) {
            process(*store, scan_file(log, *store, file.c_str()), uprg);
        }
    }
    Program prg{opts};
    prg.join(log, *store, std::move(uprg));
}
