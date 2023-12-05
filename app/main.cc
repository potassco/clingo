#include <iostream>

#include <CLI/CLI.hpp>

#include <logger.hh>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

using namespace Gringo::Input;

template <class Scanner, class Output>
void process(Gringo::Logger &log, Gringo::SymbolStore &store, RewriteOptions opts, Scanner &&scanner, Output &&output) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        StatementVec stms;
        rewrite(log, store, std::move(stm).value(), opts, stms);
        for (auto const &stm : stms) {
            output << stm << "\n";
        }
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
                                 P{"project", Gringo::Input::RewriteLevel::project}};
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

    auto log = Gringo::Logger{};
    auto store = Gringo::make_symbol_store(true, false);
    log.set_level(log_level);
    GRINGO_REPORT(log, debug) << "starting up";
    if (files.empty()) {
        process(log, *store, opts, scan_stream(log, *store, std::cin), std::cout);
    } else {
        for (auto const &file : files) {
            process(log, *store, opts, scan_file(log, *store, file.c_str()), std::cout);
        }
    }
}
