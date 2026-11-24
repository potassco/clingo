#pragma once

#include <clingo/control/solver.hh>

#include <potassco/program_opts/errors.h>
#include <potassco/program_opts/program_options.h>
#include <potassco/program_opts/typed_value.h>

namespace CppClingo::CAPI {

inline auto split(std::string_view const &source, char const *delimiter = " ") -> std::vector<std::string_view> {
    std::vector<std::string_view> results;
    size_t prev = 0;
    size_t next = 0;
    while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
        if (next - prev != 0) {
            results.push_back(source.substr(prev, next - prev));
        }
        prev = next + 1;
    }
    if (prev < source.size()) {
        results.push_back(source.substr(prev));
    }
    return results;
}

class ClingoOptions {
  public:
    ClingoOptions(Logger &log, SymbolStore &store) : log_{&log}, store_{&store}, parser_{log, store} {}

    void init(Potassco::ProgramOptions::OptionContext &root) {
        using namespace Potassco::ProgramOptions;
        auto parse_const = [this](std::string_view value) {
            // NOTE: this might use the logger.
            parser_.init(value, *store_->string("<const>"));
            auto def = parser_.parse_const_def();
            if (def) {
                const_defs_.emplace_back(*def);
            }
            return static_cast<bool>(def);
        };
        auto parse_parts = [&, this](std::string_view value) {
            // NOTE: this might use the logger.
            parser_.init(value, *store_->string("<parts>"));
            parts_ = parser_.parse_program_parts();
            return static_cast<bool>(parts_);
        };
        auto parse_imin = [this](std::string_view value) {
            return Potassco::stringTo(value, solver_opts_.imin) == std::errc{};
        };
        auto parse_imax = [this](std::string_view value) {
            if (Potassco::Parse::eqIgnoreCase(value, "none")) {
                solver_opts_.imax = std::nullopt;
                return true;
            }
            solver_opts_.imax = 0;
            return Potassco::stringTo(value, *solver_opts_.imax) == std::errc{};
        };
        auto parse_iquery = [this](std::string_view value) {
            bool is_id = false;
            for (auto c : value) {
                if ('a' <= c && c <= 'z') {
                    is_id = true;
                    continue;
                }
                if (c == '_' || c == '\'') {
                    continue;
                }
                if (is_id && (('A' <= c && c <= 'Z') || ('0' <= c && c <= '9'))) {
                    continue;
                }
                return false;
            }
            solver_opts_.iquery = value;
            return is_id;
        };
        auto parse_level = [this](std::string_view value) {
            if (auto lvl = LogLevel::info; values<LogLevel>({
                    {"error", LogLevel::error},
                    {"warn", LogLevel::warn},
                    {"info", LogLevel::info},
                    {"debug", LogLevel::debug},
                    {"trace", LogLevel::trace},
                })(value, lvl)) {
                log_->set_level(lvl);
                return true;
            }
            return false;
        };
        auto parse_info = [this](std::string_view value) {
            auto vals = values<std::pair<MessageCode, bool>>({
                {"all", std::pair{MessageCode::info, true}},
                {"none", std::pair{MessageCode::info, false}},
                {"operation-undefined", std::pair{MessageCode::info_operation_undefined, true}},
                {"atom-undefined", std::pair{MessageCode::info_atom_undefined, true}},
                {"file-included", std::pair{MessageCode::info_file_included, true}},
                {"global-variable", std::pair{MessageCode::info_global_variable, true}},
                {"no-operation-undefined", std::pair{MessageCode::info_operation_undefined, false}},
                {"no-atom-undefined", std::pair{MessageCode::info_atom_undefined, false}},
                {"no-file-included", std::pair{MessageCode::info_file_included, false}},
                {"no-global-variable", std::pair{MessageCode::info_global_variable, false}},
            });
            if (auto val = std::pair{MessageCode::info, true}; vals(value, val)) {
                if (val.first == MessageCode::info) {
                    for (auto const &[k, v] : vals.values) {
                        if (v.second == val.second && v.first != val.first) {
                            log_->enable(v.first, v.second);
                        }
                    }
                } else {
                    log_->enable(val.first, val.second);
                }
                return true;
            }
            return false;
        };
        auto parse_sigs = [this](std::string_view str) {
            for (auto &sig : split(str, ",")) {
                auto x = split(sig, "/");
                if (x.size() != 2) {
                    return false;
                }
                size_t arity = 0;
                const auto *begin = x[1].data();
                const auto *end = x[1].data() + x[1].size(); // NOLINT
                auto [ptr, ec] = std::from_chars(begin, end, arity);
                if (ec != std::errc{} || ptr != end) {
                    return false;
                }
                bool sign = !x[0].empty() && x[0][0] == '-';
                if (sign) {
                    x[0] = x[0].substr(1);
                }
                show_.emplace_back(store_->string(x[0]), arity, sign);
            }
            return true;
        };
        auto parse_profile = [this](std::string_view str) {
            auto ieq = [](std::string_view a, std::string_view b) {
                return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char ac, char bc) {
                           return std::tolower(static_cast<unsigned char>(ac)) ==
                                  std::tolower(static_cast<unsigned char>(bc));
                       });
            };

            solver_opts_.ropts.profile = Input::ProfileFlags::off;
            auto x = split(str, ",");
            if (x.empty() || x.size() >= 3) {
                return false;
            }
            if (ieq(x[0], "off")) {
                return x.size() == 1;
            }
            if (ieq(x[0], "detailed")) {
                solver_opts_.ropts.profile |= Input::ProfileFlags::detailed;
            } else if (ieq(x[0], "compact")) {
                solver_opts_.ropts.profile -= Input::ProfileFlags::detailed;
            } else {
                return false;
            }
            if (x.size() >= 2) {
                if (ieq(x[1], "step")) {
                    solver_opts_.ropts.profile |= Input::ProfileFlags::step;
                } else if (ieq(x[1], "accu")) {
                    solver_opts_.ropts.profile |= Input::ProfileFlags::accu;
                } else if (ieq(x[1], "both")) {
                    solver_opts_.ropts.profile |= Input::ProfileFlags::step;
                    solver_opts_.ropts.profile |= Input::ProfileFlags::accu;
                } else {
                    return false;
                }
            } else {
                solver_opts_.ropts.profile |= Input::ProfileFlags::accu;
            }
            return true;
        };
        auto parse_convert = [this](std::string_view str) {
            namespace Parse = Potassco::Parse;
            using namespace std::string_view_literals;

            if (auto key = "text"sv; Parse::eqIgnoreCase(str, key)) {
                ground_mode_ = true;
                return true;
            }

            static constexpr auto items = std::array{
                std::pair{Control::BackendType::aspif, "aspif"sv},
                std::pair{Control::BackendType::smodels, "smodels"sv},
                std::pair{Control::BackendType::reify, "reify"sv},
                std::pair{Control::BackendType::clasp, "no"sv},
            };
            // NOLINTNEXTLINE
            auto it = std::ranges::find_if(
                items, [&](auto const &kv) { return Parse::eqIgnoreCase(str, kv.second, kv.second.size()); });
            if (it == items.end()) {
                return false;
            }
            auto backend_type = it->first;
            str.remove_prefix(it->second.size());

            auto reify_flags = Control::ReifyFlags{};
            if (backend_type == Control::BackendType::reify) {
                while (Parse::matchOpt(str, ',')) {
                    if (auto key = "sccs"sv; Parse::eqIgnoreCase(str, key, key.size())) {
                        reify_flags |= Control::ReifyFlags::reify_scc;
                        str.remove_prefix(key.size());
                    } else if (key = "steps"sv; Parse::eqIgnoreCase(str, key, key.size())) {
                        reify_flags |= Control::ReifyFlags::reify_step;
                        str.remove_prefix(key.size());
                    } else {
                        return false;
                    }
                }
            }

            if (str.empty()) {
                solver_opts_.backend_type = backend_type;
                solver_opts_.reify_flags = reify_flags;
                return true;
            }
            return false;
        };

        root.addOptions("Grounder Options") //
            ("-c+,const", parse(parse_const).arg("<id>=<term>"),
             "Replace term occurrences of <id> with <term>")                                       //
            ("parts", parse(parse_parts).arg("<list>"), "Ground comma-separated program parts %A") //
            ("imin", parse(parse_imin).arg("<n>"), "Run incmode for at least %A steps")            //
            ("imax", parse(parse_imax).arg("<n>"),
             "Run incmode for at most %A steps [none]") //
            ("@2,iquery", parse(parse_iquery).arg("<id>"),
             "Use %A as query atom in incmode [query]") //
            ("istop",
             storeTo(solver_opts_.istop, values<Control::IStop>({
                                             {"none", Control::IStop::none},
                                             {"sat", Control::IStop::sat},
                                             {"unsat", Control::IStop::unsat},
                                             {"unknown", Control::IStop::unknown},
                                         })),
             "Stop incmode on {none|sat|unsat|unknown} [sat]") //
            ("@1,projection-mode",
             storeTo(solver_opts_.ropts.project_mode = Input::ProjectionMode::pure,
                     values<Input::ProjectionMode>({
                         {"none", Input::ProjectionMode::disabled},
                         {"anonymous", Input::ProjectionMode::anonymous},
                         {"pure", Input::ProjectionMode::pure},
                     })),
             "Project {none|anonymous|pure} variables [pure]") //
            ("@1,project-anonymous", flag(solver_opts_.ropts.project_anonymous = false),
             "Project anonymous variables in negative literals") //
            ("@1,estimate-function",
             storeTo(solver_opts_.gopts.fun = EstimateFunction::average, values<EstimateFunction>({
                                                                             {"min", EstimateFunction::minimum},
                                                                             {"max", EstimateFunction::maximum},
                                                                             {"avg", EstimateFunction::average},
                                                                         })),
             "Combine domain estimates with {min|max|avg} [avg]") //
            ("@1,estimate-selector",
             storeTo(solver_opts_.gopts.sel = EstimateSelector::pred, values<EstimateSelector>({
                                                                          {"predicate", EstimateSelector::pred},
                                                                          {"all", EstimateSelector::all},
                                                                      })),
             "Use {predicate|all} literals in estimates [predicate]") //
            ("show", parse(parse_sigs).arg("<sigs>"),
             "Show comma-separated predicates %A") //
            ("profile", parse(parse_profile).implicit("detailed"),
             "Profile grounding\n"
             "      %A: {off|detailed|compact}[,<type>] (implicit: %I)\n"
             "        detailed: Print detailed profiling information\n"
             "        compact : Print compact profiling information\n"
             "        <type>  : Print profiles for {step|accu|both} [accu]\n"
             "          step  : Each grounding step\n"
             "          accu  : All grounding steps accumulated\n"
             "          both  : Both of the above");

        root.addOptions("Basic Options")                                                                          //
            ("single-shot", flag(solver_opts_.single_shot = false), "Force single shot solving")                  //
            ("@1,log-level", parse(parse_level).arg("<level>"), "Set log level to {error|warn|info|debug|trace}") //
            ("@1-W+,info", parse(parse_info),
             "Enable or disable info messages\n"
             "      %A: {all|none|[no-]<msg>}\n"
             "        atom-undefined     : Undefined atom as in a :- b.\n"
             "        file-included      : File included more than once\n"
             "        operation-undefined: Undefined operation as in p(1/0).\n"
             "        global-variable    : Global variable in aggregate") //
            ("convert", parse(parse_convert),
             "Print converted program and exit\n"
             "      %A: {no|text|aspif|smodels|reify}[,<opts>]\n"
             "        no     : Do not convert the program\n"
             "        text   : Print ground program in text format\n"
             "        aspif  : Print program in ASP intermediate format\n"
             "        smodels: Print program in smodels format\n"
             "        reify  : Print program as reified facts with <opts>\n"
             "          sccs : Compute and print SCCs\n"
             "          steps: Add step numbers");
    }

    void apply(Control::Solver &slv) {
        for (auto const &[name, value] : const_defs_) {
            slv.add_const(*name, *value);
        }
        for (auto const &sig : show_) {
            slv.show(sig);
        }
        if (parts_) {
            slv.set_parts(*parts_);
        } else {
            slv.set_parts(std::nullopt);
        }
    }

    auto init_app_mode(std::string_view str) -> bool {
        namespace Parse = Potassco::Parse;
        using namespace std::string_view_literals;

        static constexpr auto items = std::array{
            std::pair{Control::AppMode::parse, "parse"sv},
            std::pair{Control::AppMode::rewrite, "rewrite"sv},
            std::pair{Control::AppMode::solve, "default"sv},
        };
        // NOLINTNEXTLINE
        auto it = std::ranges::find_if(items, [&](auto const &kv) { return Parse::eqIgnoreCase(str, kv.second); });
        if (it == items.end()) {
            return false;
        }
        solver_opts_.mode = it->first;
        return true;
    }

    void validate_options([[maybe_unused]] Potassco::ProgramOptions::ParsedOptions const &parsed) {
        if ((mode() != Control::AppMode::solve && ground_mode_) ||
            (backend_type() != Control::BackendType::clasp && mode() != Control::AppMode::solve)) {
            throw Potassco::ProgramOptions::Error("incompatible values for options '--convert' and '--mode'");
        }
        if (std::exchange(ground_mode_, false)) {
            assert(backend_type() == Control::BackendType::clasp);
            mode() = Control::AppMode::ground;
        }
    }

    auto mode() -> Control::AppMode & { return solver_opts_.mode; }
    auto backend_type() -> Control::BackendType & { return solver_opts_.backend_type; }

    auto solver_options() -> Control::SolverOptions const & { return solver_opts_; }

  private:
    Logger *log_;
    SymbolStore *store_;
    Input::Parser parser_;
    Control::SolverOptions solver_opts_;
    std::vector<CppClingo::Input::SharedSig> show_;
    std::optional<Control::ProgramParamVec> parts_;
    std::vector<std::pair<SharedString, SharedSymbol>> const_defs_;
    bool ground_mode_ = false;
};

} // namespace CppClingo::CAPI
