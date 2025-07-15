#pragma once

#include <clingo/control/solver.hh>

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
        auto parse_level = [this](std::string_view value) {
            if (auto lvl = LogLevel::info; values<LogLevel>({{"error", LogLevel::error},
                                                             {"warn", LogLevel::warn},
                                                             {"info", LogLevel::info},
                                                             {"debug", LogLevel::debug},
                                                             {"trace", LogLevel::trace}})(value, lvl)) {
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
                const auto *end = x[1].data() + x[1].size();
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

            rewrite_opts_.profile = Input::ProfileFlags::off;
            auto x = split(str, ",");
            if (x.empty() || x.size() >= 3) {
                return false;
            }
            if (ieq(x[0], "off")) {
                return x.size() == 1;
            }
            if (ieq(x[0], "detailed")) {
                rewrite_opts_.profile |= Input::ProfileFlags::detailed;
            } else if (ieq(x[0], "compact")) {
                rewrite_opts_.profile -= Input::ProfileFlags::detailed;
            } else {
                return false;
            }
            if (x.size() >= 2) {
                if (ieq(x[1], "step")) {
                    rewrite_opts_.profile |= Input::ProfileFlags::step;
                } else if (ieq(x[1], "accu")) {
                    rewrite_opts_.profile |= Input::ProfileFlags::accu;
                } else if (ieq(x[1], "both")) {
                    rewrite_opts_.profile |= Input::ProfileFlags::step;
                    rewrite_opts_.profile |= Input::ProfileFlags::accu;
                } else {
                    return false;
                }
            } else {
                rewrite_opts_.profile |= Input::ProfileFlags::accu;
            }
            return true;
        };

        auto group_grounder = OptionGroup{"Grounder Options"};
        group_grounder.addOptions() //
            ("-c,const", parse(parse_const).arg("<id>=<term>").composing(),
             "Replace term occurrences of <id> with <term>")                                     //
            ("parts", parse(parse_parts), "Parse comma-separated program parts to ground")       //
            ("imin", parse(parse_imin).arg("<n>"), "Minimum number of steps for incmode")        //
            ("imax", parse(parse_imax).arg("{none|<n>}"), "Maximum number of steps for incmode") //
            ("istop",
             storeTo(solver_opts_.istop, values<Control::IStop>({{"none", Control::IStop::none},
                                                                 {"sat", Control::IStop::sat},
                                                                 {"unsat", Control::IStop::unsat},
                                                                 {"unknown", Control::IStop::unknown}})),

             "Stop when {none|sat|unsat|unknown} in incmode") //
            ("@1,projection-mode",
             storeTo(rewrite_opts_.project_mode = Input::ProjectionMode::pure,
                     values<Input::ProjectionMode>({
                         {"none", Input::ProjectionMode::disabled},
                         {"anonymous", Input::ProjectionMode::anonymous},
                         {"pure", Input::ProjectionMode::pure},
                     })),
             "Project {none|anonymous|pure} variables") //
            ("@1,project-anonymous", flag(rewrite_opts_.project_anonymous = false),
             "Project anonymous variables in negative literals")                      //
            ("show", parse(parse_sigs), "Comma-separated list of predicates to show") //
            ("profile", parse(parse_profile).implicit("detailed").arg("off|<detail>[,<type>]"),
             R"(Enable profiling of grounding
      <detail>: {detailed|compact} [detailed]
        detailed: output detailed profiling information
        compact : output compact profiling information
      <type>: {step|accu|both} [accu]
        step    : output profiling information for each grounding step
        accu    : output accumulated profiling information [default]
        both    : enable both step and accu profiling)");
        root.add(group_grounder);

        auto group_basic = OptionGroup{"Basic Options"};
        group_basic.addOptions()                                                                   //
            ("single-shot", flag(solver_opts_.single_shot = false), "Force single shot solving")   //
            ("@1,log-level", parse(parse_level), "Select log level {error|warn|info|debug|trace}") //
            ("@1-W,info", parse(parse_info).composing(), R"(Enable/disable specific info messages:
      none                    : disable all
      all                     : enable all
      [no-]atom-undefined     : a :- b.
      [no-]file-included      : #include "a.lp". #include "a.lp".
      [no-]operation-undefined: p(1/0).
      [no-]global-variable    : :- #count { X } = 1, X = 1.)");
        root.add(group_basic);
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

    auto mode() -> Control::AppMode & { return solver_opts_.mode; }

    auto rewrite_options() -> Input::RewriteOptions const & { return rewrite_opts_; }
    auto solver_options() -> Control::SolverOptions const & { return solver_opts_; }

  private:
    Logger *log_;
    SymbolStore *store_;
    Input::Parser parser_;
    Input::RewriteOptions rewrite_opts_;
    Control::SolverOptions solver_opts_;
    std::vector<CppClingo::Input::SharedSig> show_;
    std::optional<Control::ProgramParamVec> parts_;
    std::vector<std::pair<SharedString, SharedSymbol>> const_defs_;
};

} // namespace CppClingo::CAPI
