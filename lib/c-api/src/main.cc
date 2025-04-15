#include "control.hh" // IWYU pragma: keep
#include "lib.hh"     // IWYU pragma: keep

#include <clingo/app.h>

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <clasp/cli/clasp_app.h>

#include <forward_list>
#include <unordered_set>
#include <utility>

namespace {

using namespace Clingo::Input;

class AppOptions {
  public:
    using OptionParser = std::function<bool(char const *)>;

    void add_option(char const *group, char const *option, char const *description, OptionParser parser,
                    char const *argument = nullptr, bool multi = false) {
        using namespace Potassco::ProgramOptions;
        auto value = std::unique_ptr<Value>(
            parse([parser = std::move(parser)]([[maybe_unused]] std::string const &name, std::string const &value) {
                return parser(value.c_str());
            }));
        if (argument != nullptr) {
            value->arg(add_string_(argument));
        }
        if (multi) {
            value->composing();
        }
        add_option_value_(group, option, std::move(value), description);
    }

    void add_flag(char const *group, char const *option, char const *description, bool &target) {
        using namespace Potassco::ProgramOptions;
        std::unique_ptr<Value> value{flag(target)};
        value->negatable();
        add_option_value_(group, option, std::move(value), description);
    }

    void init(Potassco::ProgramOptions::OptionContext &root) {
        for (auto const &group : groups_) {
            root.add(group);
        }
    }

  private:
    auto add_string_(char const *name) -> char const * { return names_.emplace(name).first->c_str(); }

    void add_option_value_(char const *group, char const *option,
                           std::unique_ptr<Potassco::ProgramOptions::Value> value, char const *description) {
        auto init = add_option_group_(group).addOptions();
        auto const *copt = add_string_(option);
        auto const *cdesc = add_string_(description);
        init(copt, value.release(), cdesc);
    }

    auto add_option_group_(char const *group) -> Potassco::ProgramOptions::OptionGroup & {
        auto it = groups_.before_begin();
        for (auto &option_group : groups_) {
            if (option_group.caption() == group) {
                return option_group;
            }
            ++it;
        }
        return *groups_.emplace_after(it, group);
    }

    std::unordered_set<std::string> names_;
    std::forward_list<Potassco::ProgramOptions::OptionGroup> groups_;
};

auto c_cast(AppOptions *opts) -> clingo_options_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_options_t *>(opts);
}

auto cpp_cast(clingo_options_t *opts) -> AppOptions * {
    // NOLINTNEXTLINE
    return reinterpret_cast<AppOptions *>(opts);
}

class AppAdapter {
  public:
    AppAdapter(clingo_application_t *app, void *data) : app_(app), data_(data) {}

    [[nodiscard]] auto get_name() const -> char const * {
        return app_ != nullptr && app_->program_name != nullptr ? app_->program_name(data_) : "clingo";
    }
    [[nodiscard]] auto get_version() const -> char const * {
        return app_ != nullptr && app_->version != nullptr ? app_->version(data_) : CLINGO_VERSION;
    }

    void register_options(Potassco::ProgramOptions::OptionContext &root) {
        if (app_ != nullptr && app_->register_options != nullptr) {
            handle_error(app_->register_options(c_cast(&opts_), data_));
            opts_.init(root);
        }
    }

    void validateOptions() {
        if (app_ != nullptr && app_->validate_options != nullptr) {
            handle_error(app_->validate_options(data_));
        }
    }

    [[nodiscard]] auto has_print_model() const -> bool { return app_ != nullptr && app_->print_model != nullptr; }

    template <class T> void print_model(Clingo::Control::Model &mdl, T &prt) const {
        assert(has_print_model());
        auto cprt = [](void *data) -> clingo_result_t {
            CLINGO_TRY {
                (*static_cast<T const *>(data))();
            }
            CLINGO_CATCH;
        };
        // NOLINTNEXTLINE
        auto *cmdl = reinterpret_cast<clingo_model_t *>(&mdl);
        handle_error(app_->print_model(cmdl, cprt, static_cast<void *>(&prt), data_));
    }

    [[nodiscard]] auto has_main() const -> bool { return app_ != nullptr && app_->main != nullptr; }

    void main(clingo_control_t *ctl, std::span<std::string const> const &input) {
        assert(has_main());
        auto vec = Clingo::Util::transform(input, [](auto const &str) { return str.c_str(); });
        handle_error(app_->main(ctl, vec.data(), vec.size(), data_));
    }

  private:
    AppOptions opts_;
    clingo_application_t *app_;
    void *data_;
};

class ClingoApp : public Clasp::Cli::ClaspAppBase {
  public:
    ClingoApp(clingo_lib_t &lib, clingo_application_t *app = nullptr, void *data = nullptr)
        : ctl_{new clingo_control_t{&lib}}, app_{app, data} {}

    [[nodiscard]] auto getName() const -> char const * override { return app_.get_name(); }
    [[nodiscard]] auto getVersion() const -> char const * override { return app_.get_version(); }
    [[nodiscard]] auto getUsage() const -> char const * override { return "[number] [options] [files]"; }

  private:
    using AppMode = Clingo::Control::AppMode;
    enum class Mode : uint8_t {
        parse = static_cast<uint8_t>(AppMode::parse),
        rewrite = static_cast<uint8_t>(AppMode::rewrite),
        ground = static_cast<uint8_t>(AppMode::ground),
        solve = static_cast<uint8_t>(AppMode::solve),
        clasp = static_cast<uint8_t>(AppMode::solve) + 1,
    };
    using ClaspOutput = Clasp::Cli::Output;
    using ProblemType = Clasp::ProblemType;
    using BaseType = Clasp::Cli::ClaspAppBase;
    using BaseType::run;

    auto getProblemType() -> ProblemType override {
        return mode_ != Mode::clasp ? Clasp::ProblemType::asp : Clasp::ClaspFacade::detectProblemType(getStream());
    }

    void initOptions(Potassco::ProgramOptions::OptionContext &root) override {
        using namespace Potassco::ProgramOptions;
        BaseType::initOptions(root);
        auto group_grounder = OptionGroup{"Grounder Options"};
        auto parse_const = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *ctl_->lib->store->string("<const>"));
            auto def = parser_.parse_const_def();
            if (def) {
                const_defs_.emplace_back(*def);
            }
            return static_cast<bool>(def);
        };
        auto parse_parts = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *ctl_->lib->store->string("<parts>"));
            parts_ = parser_.parse_program_parts();
            return static_cast<bool>(parts_);
        };

        group_grounder.addOptions() //
            ("const,c", parse(parse_const)->arg("<id>=<term>")->composing(),
             "Replace term occurrences of <id> with <term>")               //
            ("parts", parse(parse_parts), "Parse program parts to ground") //
            ("projection-mode,@1",
             storeTo(rewrite_opts_.project_mode = ProjectionMode::pure, values<ProjectionMode>({
                                                                            {"none", ProjectionMode::disabled},
                                                                            {"anonymous", ProjectionMode::anonymous},
                                                                            {"pure", ProjectionMode::pure},
                                                                        })),
             "Select which variables to project") //
            ("project-anonymous,@1", flag(rewrite_opts_.project_anonymous = false), "Project anonymous variables");
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
        app_.register_options(root);
    }

    void validateOptions(const Potassco::ProgramOptions::OptionContext &root,
                         const Potassco::ProgramOptions::ParsedOptions &parsed,
                         const Potassco::ProgramOptions::ParsedValues &vals) override {
        BaseType::validateOptions(root, parsed, vals);
        setExitCode(Clasp::Cli::exit_no_run);
        ctl_->lib->log.set_level(log_level_);
        app_.validateOptions();
        setExitCode(0);
    }

    auto createTextOutput(const ClaspAppBase::TextOptions &options) -> ClaspOutput * override {
        if (mode_ == Mode::solve && app_.has_print_model()) {
            class CustomTextOutput : public Clasp::Cli::TextOutput {
              public:
                using BaseType = Clasp::Cli::TextOutput;
                CustomTextOutput(ClingoApp &app, Clasp::Cli::ClaspAppBase::TextOptions const &opts)
                    : TextOutput(opts.verbosity, opts.format, opts.catAtom, opts.ifs), self_{&app} {}

              private:
                void printModelValues(Clasp::OutputTable const &out, Clasp::Model const &mdl) override {
                    auto prt = [&]() { BaseType::printModelValues(out, mdl); };
                    // NOTE: the function can only be called while the solve handle is alive
                    auto guard = Clingo::Control::unlock_guard{self_->ctl_->slv->get_lock()};
                    self_->app_.print_model(self_->ctl_->slv->map_model(mdl), prt);
                }
                ClingoApp *self_;
            };
            return new CustomTextOutput(*this, options);
        }
        return mode_ == Mode::solve || mode_ == Mode::clasp ? BaseType::createTextOutput(options) : nullptr;
    }

    void run(Clasp::ClaspFacade &clasp) override {
        if (mode_ != Mode::clasp) {
            if (mode_ == Mode::solve) {
                clasp.startAsp(claspConfig_, false);
            }
            auto slv = Clingo::Control::Solver{clasp,
                                               claspConfig_,
                                               ctl_->lib->log,
                                               *ctl_->lib->store,
                                               ctl_->lib->scripts,
                                               rewrite_opts_,
                                               static_cast<AppMode>(mode_),
                                               stdout};
            for (auto const &[name, value] : const_defs_) {
                slv.add_const(*name, *value);
            }
            slv.parts() = std::move(parts_);
            // NOTE: member for createTextOutput
            ctl_->bind(&slv, &slv.clasp_config(), &slv.clasp_facade());
            if (app_.has_main()) {
                if (mode_ == Mode::solve) {
                    ctl_->clasp->enableProgramUpdates();
                }
                app_.main(ctl_.get(), claspAppOpts_.input);
            } else {
                ctl_->slv->main(std::vector<std::string_view>{claspAppOpts_.input.begin(), claspAppOpts_.input.end()});
            }
        } else {
            BaseType::run(clasp);
        }
    }

    struct release_control {
        void operator()(clingo_control_t *ctl) const { clingo_control_release(ctl); }
    };

    RewriteOptions rewrite_opts_;
    Clingo::LogLevel log_level_ = Clingo::LogLevel::info;
    std::optional<Clingo::Control::ProgramParamVec> parts_;
    std::vector<std::pair<Clingo::SharedString, Clingo::SharedSymbol>> const_defs_;
    Mode mode_ = Mode::solve;
    std::unique_ptr<clingo_control_t, release_control> ctl_;
    Parser parser_{ctl_->lib->log, *ctl_->lib->store};
    AppAdapter app_;
};

auto map(char const *name, char const *const *args, size_t size) {
    auto c_args = std::vector<char *>{};
    c_args.reserve(size + 2);
    c_args.emplace_back(const_cast<char *>(name)); // NOLINT
    for (auto const &arg : std::span{args, size}) {
        c_args.emplace_back(const_cast<char *>(arg)); // NOLINT
    }
    c_args.emplace_back(nullptr);
    return c_args;
}

} // namespace

extern "C" auto clingo_options_add(clingo_options_t *options, char const *group, char const *option,
                                   char const *description, clingo_option_parser_t parser, void *data, bool multi,
                                   char const *argument) -> clingo_result_t {
    CLINGO_TRY {
        auto *opts = cpp_cast(options);
        opts->add_option(
            group, option, description,
            [parser, data](char const *value) {
                auto result = false;
                handle_error(parser(value, data, &result));
                return result;
            },
            argument, multi);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_options_add_flag(clingo_options_t *options, char const *group, char const *option,
                                        char const *description, bool *target) -> clingo_result_t {
    CLINGO_TRY {
        auto *opts = cpp_cast(options);
        opts->add_flag(group, option, description, *target);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_main(clingo_lib_t *lib, char const *const *arguments, size_t size, clingo_application_t *app,
                            void *data, int *code) -> clingo_result_t {
    CLINGO_TRY {
        if (code != nullptr) {
            *code = 1;
        }
        if (lib == nullptr || (arguments == nullptr && size > 0)) {
            return clingo_result_invalid;
        }
        auto capp = ClingoApp{*lib, app, data};
        auto args = map(capp.getName(), arguments, size);
        auto res = capp.main(static_cast<int>(args.size() - 1), args.data());
        if (code != nullptr) {
            *code = res;
        }
    }
    CLINGO_CATCH;
}
