#include "lib.hh"

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <CLI/CLI.hpp>

#include <iostream>

namespace {

using namespace Clingo::Input;

enum class AppMode : uint8_t { parse, rewrite, ground };

auto run(clingo_lib_t *lib, std::vector<std::string> &&args) -> clingo_result_t {
    auto opts = RewriteOptions{};
    auto mode = AppMode::ground;
    std::vector<std::string> files;
    std::vector<std::string> const_defs;
    auto log_level = Clingo::LogLevel::info;
    auto params_str = std::optional<std::string>{};

    CLI::App app{"ASP preprocessor that wants to become a grounder"};

    app.add_option("files", files, "files to parse");
    // later...
    // ->check(CLI::ExistingFile);
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
        app.parse(std::move(args));
    } catch (CLI::ParseError const &e) {
        return app.exit(e);
    }

    try {
        // TODO: maybe this should not happen here
        lib->log.set_level(log_level);
        // TODO: the mode should be passed to the solver object
        auto slv =
            Clingo::Control::Solver{lib->log, *lib->store, lib->scripts, opts, Clingo::Control::OutputMode::text};
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
            // TODO: from here onward, slv.main() should be used
            slv.parse(std::vector<std::string_view>{files.begin(), files.end()});
            if (mode == AppMode::parse) {
                slv.output_unprocessed_program(std::cout);
                return;
            }
            if (mode == AppMode::rewrite) {
                slv.output_program(std::cout);
                return;
            }
            if (params) {
                for (auto const &param : *params) {
                    if (!slv.ground(param)) {
                        break;
                    }
                }
            } else {
                std::ignore = slv.ground(Clingo::Input::ProgramParamVec{{lib->store->string("base"), {}}});
            }
        }();
    } catch (std::exception const &e) {
        fprintf(stderr, "%s: %s\n", lib->log.message_prefix(Clingo::MessageCode::error), e.what());
        fflush(stderr);
        return clingo_result_runtime;
    }
    return clingo_result_success;
}

} // namespace

extern "C" auto clingo_main(clingo_lib_t *lib, char const *const *arguments, size_t size) -> clingo_result_t {
    try {
        return run(lib, Clingo::Util::transform_n(arguments, size, [](auto const *str) { return std::string(str); }));
    } catch (std::exception const &e) {
        fprintf(stderr, "panic: %s\n", "unrecoverable error during startup");
        fflush(stderr);
        return clingo_result_runtime;
    }
}
