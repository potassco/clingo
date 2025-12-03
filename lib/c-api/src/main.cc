#include "control.hh" // IWYU pragma: keep
#include "lib.hh"     // IWYU pragma: keep
#include "opts.hh"

#include <clingo/app.h>

#include <clingo/control/solver.hh>

#include <clingo/input/parser.hh>

#include <clingo/output/text.hh>

#include <clasp/cli/clasp_app.h>

#include <utility>

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

using namespace CppClingo::Input;
class AppOptions {
  public:
    using OptionParser = std::function<bool(std::string_view)>;
    explicit AppOptions(Potassco::ProgramOptions::OptionContext &root) : root_(&root) {}

    void add_option(std::string_view group, std::string_view option, std::string_view description, OptionParser parser,
                    std::optional<std::string_view> argument, bool multi = false) {
        using namespace Potassco::ProgramOptions;
        auto value = parse(std::move(parser));
        if (argument) {
            value.arg(*argument);
        }
        if (multi) {
            value.composing();
        }
        add_option_value_(group, option, std::move(value), description);
    }

    void add_flag(std::string_view group, std::string_view option, std::string_view description, bool &target) {
        using namespace Potassco::ProgramOptions;
        auto value{flag(target)};
        value.negatable();
        add_option_value_(group, option, std::move(value), description);
    }

    auto set_default_value(std::string_view option, std::string_view value) -> bool {
        if (!root_->option(option).assignDefault(value)) {
            throw std::invalid_argument(std::string("Invalid value for option '").append(option).append("'"));
        }
        return true;
    }

  private:
    void add_option_value_(std::string_view group, std::string_view option, Potassco::ProgramOptions::ValueDesc value,
                           std::string_view description) {
        auto init = root_->addOptions(group);
        init(option, std::move(value), description);
    }

    Potassco::ProgramOptions::OptionContext *root_ = nullptr;
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
            AppOptions opts(root);
            handle_error(app_->register_options(c_cast(&opts), data_));
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
            CppClingo::Util::to_vec(input, [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
        handle_error(app_->main(ctl, vec.data(), vec.size(), data_));
    }

  private:
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
        return mode_ != Mode::clasp ? Clasp::ProblemType::asp : detectProblemType();
    }

    auto onEvent(const Clasp::Event &ev) -> void override {
        BaseType::onEvent(ev);
        if (const auto *g = Clasp::event_cast<Control::Grounded>(ev); g != nullptr && !g->params.empty()) {
            ctl_->slv->print_summary(false);
        }
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
        app_.validate_options();
        setExitCode(0);
    }

    auto createOutput(Clasp::Cli::OutputSink sink, ProblemType f, Clasp::Cli::ClaspAppOptions::OutputFormat outf)
        -> std::unique_ptr<Clasp::Cli::Output> override {
        if (mode_ != Mode::solve && mode_ != Mode::clasp) {
            return nullptr;
        }
        auto om = mode_ == Mode::clasp ? Clasp::Cli::Output::mode_default : Clasp::Cli::Output::mode_clingo;
        if (!app_.has_print_model() || !Clasp::Cli::ClaspAppOptions::isTextOutput(outf)) {
            return ClaspAppBase::createOutput(sink, f, outf, om);
        }
        auto output = createTextOutput(sink, f, om);
        output->setModelPrinter(
            [this](Clasp::Cli::TextOutput &out, const Clasp::SharedContext &ctx, const Clasp::Model &mdl) {
                auto prt = [&]() { out.printModelValues(ctx, mdl); };
                // NOTE: the function can only be called while the solve handle is alive
                auto guard = CppClingo::Control::unlock_guard{ctl_->slv->get_lock()};
                app_.print_model(ctl_->slv->map_model(mdl), prt);
            });
        return output;
    }

    void run(Clasp::ClaspFacade &clasp) override {
        if (mode_ != Mode::clasp) {
            if (mode_ == Mode::solve) {
                clasp.startAsp(config(), false);
            }
            opts_.mode() = static_cast<AppMode>(mode_);
            auto slv = CppClingo::Control::Solver{clasp,
                                                  config(),
                                                  ctl_->lib->log,
                                                  *ctl_->lib->store,
                                                  ctl_->lib->scripts,
                                                  opts_.rewrite_options(),
                                                  opts_.solver_options(),
                                                  stdout};
            opts_.apply(slv);
            // NOTE: member for createTextOutput
            ctl_->bind(&slv, &slv.config().clasp(), &slv.clasp_facade());

            POTASSCO_SCOPE_EXIT({ ctl_->slv->print_summary(true); });

            if (auto in = input(); app_.has_main()) {
                if (mode_ == Mode::solve) {
                    ctl_->clasp->enableProgramUpdates();
                }
                app_.main(ctl_.get(), in);
            } else {
                ctl_->slv->main(std::vector<std::string_view>{in.begin(), in.end()});
            }
        } else {
            BaseType::run(clasp);
        }
    }

    struct release_control {
        void operator()(clingo_control_t *ctl) const { clingo_control_release(ctl); }
    };

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
                if (!parser(value.data(), value.size(), data)) {
                    clingo_result_t code = clingo_result_success;
                    clingo_get_error(&code, nullptr);
                    if (code == clingo_result_invalid) {
                        clingo_clear_error();
                        return false;
                    }
                    raise_error();
                }
                return true;
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

extern "C" auto clingo_options_set_default_value(clingo_options_t *options, char const *option, size_t option_size,
                                                 char const *value, size_t value_size) -> bool {
    CLINGO_TRY {
        auto *opts = cpp_cast(options);
        opts->set_default_value({option, option_size}, {value, value_size});
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
        auto args = CppClingo::Util::to_vec(std::span{arguments, size},
                                            [](auto const &str) { return std::string{str.data, str.size}; });
        auto cargs = CppClingo::Util::to_vec(args, [](auto const &str) { return str.c_str(); });
        auto res = capp.main(cargs);
        if (code != nullptr) {
            *code = res;
        }
    }
    CLINGO_CATCH;
}
