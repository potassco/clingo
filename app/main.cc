#include <gringo/input/program.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

#include <gringo/util/ordered_set.hh>

#include <CLI/CLI.hpp>

#include <filesystem>
#include <iostream>

using namespace Gringo::Input;

// Note: candidate for library because cumbersome to implement
void process_files(Gringo::Logger &log, Gringo::SymbolStore &store, std::vector<std::string> &files,
                   std::optional<UnprocessedProgram> &prg) {
    namespace fs = std::filesystem;

    auto root = fs::current_path();
    std::deque<std::pair<fs::path, Gringo::Input::StmInclude>> includes;

    // parse a program
    auto process = [&]<class Scanner>(fs::path const &dir, Scanner &&scanner) {
        for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
            if (auto *include = std::get_if<Gringo::Input::StmInclude>(&*stm); include != nullptr) {
                includes.emplace_back(dir, *include);
            } else if (prg) {
                add(store, std::move(stm).value(), *prg);
            } else {
                std::cout << *stm << "\n";
            }
        }
    };

    // parse a program from a file
    auto process_path = [&, seen = Gringo::Util::unordered_set<fs::path>{}](auto &&path, bool required) mutable {
        if (fs::exists(path)) {
            path = fs::canonical(path);
            auto rel = path.lexically_relative(root);
            if (!fs::is_directory(path)) {
                if (seen.emplace(path).second) {
                    process(path.parent_path(), scan_file(log, store, rel.c_str()));
                } else {
                    GRINGO_REPORT(log, info_file_included) << "file already included: " << rel;
                }
            } else {
                GRINGO_REPORT(log, error) << "cannot include directory: " << rel;
            }
            return true;
        }
        if (required) {
            GRINGO_REPORT(log, error) << "file nout found: " << path;
        }
        return false;
    };

    // parse a program from stdin
    auto process_stdin = [&, processed_stdin = false]() mutable {
        if (!processed_stdin) {
            processed_stdin = true;
            process(root, scan_stream(log, store, std::cin));
        } else {
            GRINGO_REPORT(log, info_file_included) << "file already included: -";
        }
    };

    if (files.empty()) {
        process_stdin();
    }
    for (auto const &file : files) {
        if (file == "-") {
            process_stdin();
        } else {
            process_path(fs::path(file), true);
        }
        for (; !includes.empty(); includes.pop_front()) {
            auto const &[parent, include] = includes.front();
            if (include.type() == Gringo::Input::IncludeType::system) {
                auto path = fs::path(include.value());
                if (path.is_relative() && parent != root) {
                    if (process_path(parent / path, false)) {
                        continue;
                    }
                }
                process_path(path, true);
            }
        }
    }
}

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
        std::optional<UnprocessedProgram> uprg;
        if (!parse_only) {
            uprg.emplace();
        }
        auto store = Gringo::make_symbol_store(true, false);
        log.set_level(log_level);
        GRINGO_REPORT(log, debug) << "starting up";
        process_files(log, *store, files, uprg);
        if (uprg) {
            Program prg{opts};
            prg.join(log, *store, std::move(uprg).value());
            prg.visit_stms(*store, [](auto const &stm) { std::cout << stm << "\n"; });
        }
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", log.message_prefix(Gringo::MessageCode::error), e.what());
        fflush(stderr);
        return 1;
    }
    return log.has_error() ? 1 : 0;
}
