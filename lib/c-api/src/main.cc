#include "lib.hh"

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <clasp/cli/clasp_app.h>

#include <CLI/CLI.hpp>

namespace {

using namespace Clingo::Input;

class ClingoApp : public Clasp::Cli::ClaspAppBase {
  public:
    ClingoApp(clingo_lib_t &lib) : lib_{&lib} {}

    [[nodiscard]] auto getName() const -> char const * override { return "clingo"; }
    [[nodiscard]] auto getVersion() const -> char const * override { return CLINGO_VERSION; }
    [[nodiscard]] auto getUsage() const -> char const * override { return "[number] [options] [files]"; }

  private:
    using ClaspOutput = Clasp::Cli::Output;
    using ProblemType = Clasp::ProblemType;
    using BaseType = Clasp::Cli::ClaspAppBase;
    using OptionParser = std::function<bool(char const *)>;
    // enum class ConfigUpdate { KEEP, REPLACE };
    using AppMode = Clingo::Control::AppMode;
    enum class Mode : uint8_t {
        parse = static_cast<uint8_t>(AppMode::parse),
        rewrite = static_cast<uint8_t>(AppMode::rewrite),
        ground = static_cast<uint8_t>(AppMode::ground),
        solve = static_cast<uint8_t>(AppMode::solve),
        clasp = static_cast<uint8_t>(AppMode::solve) + 1,
    };

    void initOptions(Potassco::ProgramOptions::OptionContext &root) override {
        using namespace Potassco::ProgramOptions;
        BaseType::initOptions(root);
        auto group_grounder = OptionGroup{"Grounder Options"};
        auto parse_const = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *lib_->store->string("<const>"));
            auto def = parser_.parse_const_def();
            if (def) {
                const_defs_.emplace_back(*def);
            }
            return static_cast<bool>(def);
        };
        auto parse_parts = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *lib_->store->string("<parts>"));
            parts_ = parser_.parse_program_parts();
            return static_cast<bool>(parts_);
        };

        group_grounder.addOptions() //
            ("const,c", parse(parse_const)->arg("<id>=<term>")->composing(),
             "Replace term occurrences of <id> with <term>")               //
            ("parts", parse(parse_parts), "Parse program parts to ground") //
            ("projection-mode,@1",
             storeTo(rewrite_opts_.project_mode = Clingo::Input::ProjectionMode::pure,
                     values<Clingo::Input::ProjectionMode>({
                         {"none", ProjectionMode::disabled},
                         {"anonymous", ProjectionMode::anonymous},
                         {"pure", ProjectionMode::pure},
                     })),
             "Select which variables to project") //
            ("project-anonymous,@1", flag(rewrite_opts_.project_anonymous = false), "Project anonymous variables");
        // registerOptions(gringo, grOpts_, GringoOptions::AppType::Clingo);
        root.add(group_grounder);
        auto group_basic = OptionGroup{"Basic Options"};
        group_basic.addOptions() //
            ("mode",
             storeTo(mode_ = Mode::solve, values<Mode>({
                                              {"parse", Mode::parse},
                                              {"rewrite", Mode::rewrite},
                                              {"ground", Mode::ground},
                                              {"solve", Mode::solve},
                                              {"clasp", Mode::clasp},
                                          })),
             "Run in {parse|rewrite|ground|solve|clasp} mode") //
            ("log-level,@2",
             storeTo(log_level_ = Clingo::LogLevel::info, values<Clingo::LogLevel>({
                                                              {"error", Clingo::LogLevel::error},
                                                              {"warn", Clingo::LogLevel::warn},
                                                              {"info", Clingo::LogLevel::info},
                                                              {"debug", Clingo::LogLevel::debug},
                                                              {"trace", Clingo::LogLevel::trace},
                                                          })),
             "Select log level {error|warn|info|debug|trace}");
        root.add(group_basic);
    }

    void validateOptions(const Potassco::ProgramOptions::OptionContext &root,
                         const Potassco::ProgramOptions::ParsedOptions &parsed,
                         const Potassco::ProgramOptions::ParsedValues &vals) override {
        BaseType::validateOptions(root, parsed, vals);
        lib_->log.set_level(log_level_);
    }

    auto getProblemType() -> ProblemType override {
        return mode_ != Mode::clasp ? Clasp::ProblemType::asp : Clasp::ClaspFacade::detectProblemType(getStream());
    }

    void run(Clasp::ClaspFacade &clasp) override {
        if (mode_ != Mode::clasp) {
            if (mode_ == Mode::solve) {
                clasp.startAsp(claspConfig_, true);
            }
            auto slv = Clingo::Control::Solver{*clasp_,       lib_->log,     *lib_->store,
                                               lib_->scripts, rewrite_opts_, static_cast<AppMode>(mode_)};
            for (auto const &[name, value] : const_defs_) {
                slv.add_const(*name, *value);
            }
            slv.main(std::vector<std::string_view>{claspAppOpts_.input.begin(), claspAppOpts_.input.end()}, parts_);
        } else {
            BaseType::run(clasp);
        }
    }

    auto createTextOutput(const ClaspAppBase::TextOptions &options) -> ClaspOutput * override {
        return mode_ == Mode::solve || mode_ == Mode::clasp ? BaseType::createTextOutput(options) : nullptr;
    }

    RewriteOptions rewrite_opts_;
    Clingo::LogLevel log_level_ = Clingo::LogLevel::info;
    std::optional<std::vector<Clingo::Input::ProgramParamVec>> parts_;
    std::vector<std::pair<Clingo::SharedString, Clingo::SharedSymbol>> const_defs_;
    Mode mode_ = Mode::solve;
    clingo_lib_t *lib_;
    Parser parser_{lib_->log, *lib_->store};
};

auto run(clingo_lib_t *lib, std::span<char const *const> args) -> clingo_result_t {
    try {
        auto app = ClingoApp{*lib};
        auto c_args = std::vector<char *>{};
        c_args.reserve(args.size() + 2);
        c_args.emplace_back(const_cast<char *>(app.getName())); // NOLINT
        for (auto const &arg : args) {
            c_args.emplace_back(const_cast<char *>(arg)); // NOLINT
        }
        c_args.emplace_back(nullptr);
        app.main(static_cast<int>(args.size()), c_args.data());
    } catch (std::exception const &e) {
        GRINGO_REPORT(lib->log, error) << e.what();
        return handle_error();
    }
    return clingo_result_success;
}

} // namespace

extern "C" auto clingo_main(clingo_lib_t *lib, char const *const *arguments, size_t size) -> clingo_result_t {
    try {
        return run(lib, std::span{arguments, size});
    } catch (std::exception const &e) {
        fprintf(stderr, "panic: %s\n", e.what());
        fflush(stderr);
        return clingo_result_runtime;
    }
}
