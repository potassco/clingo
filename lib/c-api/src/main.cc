#include "lib.hh"

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <CLI/CLI.hpp>

namespace {

using namespace Clingo::Input;

auto run(clingo_lib_t *lib, std::vector<std::string> &&args) -> clingo_result_t {
    try {
        auto opts = RewriteOptions{};
        auto mode = Clingo::Control::AppMode::solve;
        std::vector<std::string> files;
        std::vector<std::string> const_defs;
        auto log_level = Clingo::LogLevel::info;
        auto params_str = std::optional<std::string>{};

        CLI::App app{"ASP preprocessor that wants to become a grounder"};

        app.add_option("files", files, "files to parse");
        // later...
        // ->check(CLI::ExistingFile);
        app.add_option("--const,-c", const_defs, "constant definition")->take_all();
        app.add_option("--params", params_str, "program parts to ground");
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
        app.add_option("--mode", "{parse,rewrite,ground,solve}")->check([&mode](std::string const &value) {
            using P = std::pair<char const *, Clingo::Control::AppMode>;
            auto modes =
                std::array{P{"parse", Clingo::Control::AppMode::parse}, P{"rewrite", Clingo::Control::AppMode::rewrite},
                           P{"ground", Clingo::Control::AppMode::ground}, P{"solve", Clingo::Control::AppMode::solve}};
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
            app.parse(std::move(args));
        } catch (CLI::ParseError const &e) {
            return app.exit(e) != 0 ? clingo_result_runtime : clingo_result_success;
        }

        auto output_mode = Clingo::Control::OutputMode::text;
        if (mode == Clingo::Control::AppMode::solve) {
            output_mode = Clingo::Control::OutputMode::clasp;
        }

        // TODO: maybe this should not happen here
        lib->log.set_level(log_level);
        auto slv = Clingo::Control::Solver{lib->log, *lib->store, lib->scripts, opts, output_mode};
        auto prs = Clingo::Input::Parser{lib->log, *lib->store};

        [&]() {
            auto params = std::optional<std::vector<Clingo::Input::ProgramParamVec>>();
            if (params_str) {
                prs.init(*params_str, *lib->store->string("<string>"));
                params = prs.parse_program_parts();
                if (!params) {
                    throw Clingo::parse_error();
                }
            }
            for (auto const &str : const_defs) {
                prs.init(str, *lib->store->string("<string>"));
                auto def = prs.parse_const_def();
                if (!def) {
                    throw Clingo::parse_error();
                }
                slv.add_const(*def->first, *def->second);
            }
            slv.main(mode, std::vector<std::string_view>{files.begin(), files.end()}, params);
        }();
    } catch (std::exception const &e) {
        GRINGO_REPORT(lib->log, error) << e.what();
        return handle_error();
    }
    return clingo_result_success;
}

} // namespace

extern "C" auto clingo_main(clingo_lib_t *lib, char const *const *arguments, size_t size) -> clingo_result_t {
    try {
        return run(lib, Clingo::Util::transform_n(arguments, size, [](auto const *str) { return std::string(str); }));
    } catch (std::exception const &e) {
        fprintf(stderr, "panic: %s\n", e.what());
        fflush(stderr);
        return clingo_result_runtime;
    }
}
