#include <iostream>

#include <CLI/CLI.hpp>

#include <logger.hh>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

using namespace Gringo::Input;

void process(Gringo::SymbolStore &store, RewriteOptions opts, auto &&scanner, auto &&output) {
    for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
        StatementVec stms;
        rewrite(store, std::move(stm).value(), opts, stms);
        for (auto const &stm : stms) {
            output << stm << "\n";
        }
    }
}

auto main() -> int {
    auto opts = RewriteOptions{};
    auto timestamp = false;
    std::vector<std::string> files;
    auto loglevel = spdlog::level::info;

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

    app.add_option("files", files, "files to parse");
    app.add_flag("--timestamp", timestamp, "timestamp log messages");
    // later...
    // ->check(CLI::ExistingFile);
    app.add_option("--loglevel", "{off,critical,error,warn,info,debug,trace}")
        ->check([&loglevel](std::string const &value) {
            std::array levels = std::to_array<std::pair<char const *, spdlog::level::level_enum>>(
                {{"trace", spdlog::level::trace},
                 {"debug", spdlog::level::debug},
                 {"info", spdlog::level::info},
                 {"warn", spdlog::level::warn},
                 {"error", spdlog::level::err},
                 {"critical", spdlog::level::critical},
                 {"off", spdlog::level::off}});
            for (auto &[name, level] : levels) {
                if (value == name) {
                    loglevel = level;
                    return std::string{};
                }
            }
            return std::string{"unexpected value"};
        });
    app.add_option("--rewrite-level", "{off,anonymous,unpool,project}")->check([&opts](std::string const &value) {
        std::array levels = std::to_array<std::pair<char const *, Gringo::Input::RewriteLevel>>(
            {{"off", Gringo::Input::RewriteLevel::disabled},
             {"anonymous", Gringo::Input::RewriteLevel::rewrite_anonymous},
             {"unpool", Gringo::Input::RewriteLevel::unpool},
             {"project", Gringo::Input::RewriteLevel::project}});
        for (auto &[name, level] : levels) {
            if (value == name) {
                opts.level = level;
                return std::string{};
            }
        }
        return std::string{"unexpected value"};
    });
    app.add_option("--projection-mode", "{off,anonymous,pure}")->check([&opts](std::string const &value) {
        std::array levels = std::to_array<std::pair<char const *, Gringo::Input::ProjectionMode>>(
            {{"off", Gringo::Input::ProjectionMode::disabled},
             {"anonymous", Gringo::Input::ProjectionMode::anonymous},
             {"unpool", Gringo::Input::ProjectionMode::pure}});
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

    Gringo::with_logger wl{false};
    auto store = Gringo::make_symbol_store(false, false);
    auto &log = Gringo::logger();
    log.set_level(loglevel);
    if (timestamp) {
        log.set_pattern("[%Y-%m-%d %T.%e] %^%l%$: %v");
    } else {
        log.set_pattern("%^%l%$: %v");
    }
    log.debug("starting up");
    if (files.empty()) {
        process(*store, opts, scan_stream(*store, std::cin), std::cout);
    } else {
        for (auto const &file : files) {
            process(*store, opts, scan_file(*store, file.c_str()), std::cout);
        }
    }
}
