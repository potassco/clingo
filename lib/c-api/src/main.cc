#include "control.hh" // IWYU pragma: keep
#include "lib.hh"     // IWYU pragma: keep
#include "opts.hh"

#include <clingo/app.h>

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <clasp/cli/clasp_app.h>

#include <forward_list>
#include <utility>

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

using namespace CppClingo::Input;

class AppOptions {
  public:
    using OptionParser = std::function<bool(std::string_view)>;

    void add_option(std::string_view group, std::string_view option, std::string_view description, OptionParser parser,
                    std::optional<std::string_view> argument, bool multi = false) {
        using namespace Potassco::ProgramOptions;
        auto value = std::unique_ptr<Value>(parse(std::move(parser)));
        if (argument) {
            value->arg(strings_.emplace_front(*argument).c_str());
        }
        if (multi) {
            value->composing();
        }
        add_option_value_(group, option, std::move(value), description);
    }

    void add_flag(std::string_view group, std::string_view option, std::string_view description, bool &target) {
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
    void add_option_value_(std::string_view group, std::string_view option,
                           std::unique_ptr<Potassco::ProgramOptions::Value> value, std::string_view description) {
        auto init = add_option_group_(group).addOptions();
        init(strings_.emplace_front(option).c_str(), value.release(), strings_.emplace_front(description).c_str());
    }

    auto add_option_group_(std::string_view group) -> Potassco::ProgramOptions::OptionGroup & {
        auto it = groups_.before_begin();
        for (auto &option_group : groups_) {
            if (option_group.caption() == group) {
                return option_group;
            }
            ++it;
        }
        return *groups_.emplace_after(it, group);
    }

    std::forward_list<Potassco::ProgramOptions::OptionGroup> groups_;
    std::forward_list<std::string> strings_;
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
    AppAdapter(clingo_application_t const *app, void *data) : app_(app), data_(data) {}

    [[nodiscard]] auto get_name() const -> std::string_view {
        if (app_ != nullptr && app_->program_name != nullptr) {
            clingo_string_t str;
            app_->program_name(data_, &str);
            return {str.data, str.size};
        }
        return CLINGO_EXECUTABLE;
    }
    [[nodiscard]] auto get_version() const -> std::string_view {
        if (app_ != nullptr && app_->version != nullptr) {
            clingo_string_t str;
            app_->version(data_, &str);
            return {str.data, str.size};
        }
        return CLINGO_VERSION;
    }

    void register_options(Potassco::ProgramOptions::OptionContext &root) {
        if (app_ != nullptr && app_->register_options != nullptr) {
            handle_error(app_->register_options(c_cast(&opts_), data_));
            opts_.init(root);
        }
    }

    void validate_options() const {
        if (app_ != nullptr && app_->validate_options != nullptr) {
            handle_error(app_->validate_options(data_));
        }
    }

    [[nodiscard]] auto has_print_model() const -> bool { return app_ != nullptr && app_->print_model != nullptr; }

    template <class T> void print_model(CppClingo::Control::Model &mdl, T &prt) const {
        assert(has_print_model());
        auto cprt = [](void *data) -> bool {
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
        auto vec =
            CppClingo::Util::transform(input, [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
        handle_error(app_->main(ctl, vec.data(), vec.size(), data_));
    }

  private:
    AppOptions opts_;
    clingo_application_t const *app_;
    void *data_;
};

class ClingoApp : public Clasp::Cli::ClaspAppBase {
  public:
    ClingoApp(clingo_lib_t &lib, clingo_application_t const *app = nullptr, void *data = nullptr)
        : ctl_{new clingo_control_t{&lib}}, app_{app, data}, opts_{ctl_->lib->log, *ctl_->lib->store} {}

    [[nodiscard]] auto getName() const -> std::string_view override { return app_.get_name(); }
    [[nodiscard]] auto getVersion() const -> std::string_view override { return app_.get_version(); }
    [[nodiscard]] auto getUsage() const -> std::string_view override { return "[number] [options] [files]"; }

  private:
    using AppMode = CppClingo::Control::AppMode;
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
        opts_.init(root);
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
             "Run in {parse|rewrite|ground|solve|clasp} mode");
        root.add(group_basic);
        app_.register_options(root);
    }

    void validateOptions(const Potassco::ProgramOptions::OptionContext &root,
                         const Potassco::ProgramOptions::ParsedOptions &parsed) override {
        BaseType::validateOptions(root, parsed);
        setExitCode(Clasp::Cli::exit_no_run);
        ctl_->lib->log.set_level(log_level_);
        app_.validate_options();
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
                    auto guard = CppClingo::Control::unlock_guard{self_->ctl_->slv->get_lock()};
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
            opts_.mode() = static_cast<AppMode>(mode_);
            auto slv = CppClingo::Control::Solver{clasp,
                                                  claspConfig_,
                                                  ctl_->lib->log,
                                                  *ctl_->lib->store,
                                                  ctl_->lib->scripts,
                                                  opts_.rewrite_options(),
                                                  opts_.solver_options(),
                                                  stdout};
            opts_.apply(slv);
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

    CppClingo::LogLevel log_level_ = CppClingo::LogLevel::info;
    Mode mode_ = Mode::solve;
    std::unique_ptr<clingo_control_t, release_control> ctl_;
    AppAdapter app_;
    CppClingo::CAPI::ClingoOptions opts_;
};

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_options_add(clingo_options_t *options, char const *group, size_t group_size, char const *option,
                                   size_t option_size, char const *description, size_t description_size,
                                   clingo_option_parser_t parser, void *data, bool multi, char const *argument,
                                   size_t argument_size) -> bool {
    CLINGO_TRY {
        auto *opts = cpp_cast(options);
        opts->add_option(
            {group, group_size}, {option, option_size}, {description, description_size},
            [parser, data](std::string_view value) {
                auto result = false;
                handle_error(parser(value.data(), value.size(), data, &result));
                return result;
            },
            argument != nullptr ? std::make_optional<std::string_view>(argument, argument_size) : std::nullopt, multi);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_options_add_flag(clingo_options_t *options, char const *group, size_t group_size,
                                        char const *option, size_t option_size, char const *description,
                                        size_t description_size, bool *target) -> bool {
    CLINGO_TRY {
        auto *opts = cpp_cast(options);
        opts->add_flag({group, group_size}, {option, option_size}, {description, description_size}, *target);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_main(clingo_lib_t *lib, clingo_string_t const *arguments, size_t size,
                            clingo_application_t const *app, void *data, int *code) -> bool {
    CLINGO_TRY {
        if (code != nullptr) {
            *code = 1;
        }
        if (lib == nullptr || (arguments == nullptr && size > 0)) {
            return fail_arguments();
        }
        auto capp = ClingoApp{*lib, app, data};
        auto args = CppClingo::Util::transform(std::span{arguments, size},
                                               [](auto const &str) { return std::string{str.data, str.size}; });
        auto cargs = CppClingo::Util::transform(args, [](auto const &str) { return str.c_str(); });
        auto res = capp.main(cargs);
        if (code != nullptr) {
            *code = res;
        }
    }
    CLINGO_CATCH;
}
