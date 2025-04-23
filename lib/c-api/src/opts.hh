#pragma once

#include <clingo/control/solver.hh>

#include <potassco/program_opts/program_options.h>
#include <potassco/program_opts/typed_value.h>

namespace Clingo::CAPI {

namespace {

auto ieq(auto const &a, char const *b) -> bool {
    return std::ranges::equal(a, std::string_view{b},
                              [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
}

} // namespace

class ClingoOptions {
  public:
    ClingoOptions(Logger &log, SymbolStore &store) : log_{&log}, store_{&store}, parser_{log, store} {}

    void init(Potassco::ProgramOptions::OptionContext &root) {
        using namespace Potassco::ProgramOptions;
        auto parse_const = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *store_->string("<const>"));
            auto def = parser_.parse_const_def();
            if (def) {
                const_defs_.emplace_back(*def);
            }
            return static_cast<bool>(def);
        };
        auto parse_parts = [&, this]([[maybe_unused]] std::string const &name, std::string const &value) {
            // NOTE: this might use the logger.
            parser_.init(value, *store_->string("<parts>"));
            parts_ = parser_.parse_program_parts();
            return static_cast<bool>(parts_);
        };
        auto parse_imin = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            const auto *end = value.data() + value.size();
            auto ret = std::from_chars(value.data(), end, solver_opts_.imin);
            return ret.ec == std::errc{} && ret.ptr == end;
        };
        auto parse_imax = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            if (ieq(value, "none")) {
                solver_opts_.imax = std::nullopt;
                return true;
            }
            solver_opts_.imax = 0;
            const auto *end = value.data() + value.size();
            auto ret = std::from_chars(value.data(), end, *solver_opts_.imax);
            return ret.ec == std::errc{} && ret.ptr == end;
        };
        auto parse_level = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            if (auto lvl = LogLevel::info; parseValue(values<LogLevel>({{"error", LogLevel::error},
                                                                        {"warn", LogLevel::warn},
                                                                        {"info", LogLevel::info},
                                                                        {"debug", LogLevel::debug},
                                                                        {"trace", LogLevel::trace}}),
                                                      value, lvl)) {
                log_->set_level(lvl);
                return true;
            }
            return false;
        };
        auto parse_info = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            auto vals = values<std::pair<MessageCode, bool>>({
                {"all", std::pair{MessageCode::info, true}},
                {"none", std::pair{MessageCode::info, false}},
                {"operation-undefined", std::pair{MessageCode::info_operation_undefined, true}},
                {"atom-undefined", std::pair{MessageCode::info_atom_undefined, true}},
                {"file-included", std::pair{MessageCode::info_file_included, true}},
                {"global-varibale", std::pair{MessageCode::info_global_variable, true}},
                {"no-operation-undefined", std::pair{MessageCode::info_operation_undefined, false}},
                {"no-atom-undefined", std::pair{MessageCode::info_atom_undefined, false}},
                {"no-file-included", std::pair{MessageCode::info_file_included, false}},
                {"no-global-varibale", std::pair{MessageCode::info_global_variable, false}},
            });
            if (auto val = std::pair{MessageCode::info, true}; parseValue(vals, value, val)) {
                if (val.first == MessageCode::info) {
                    for (auto const &[k, v] : vals) {
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

        auto group_grounder = OptionGroup{"Grounder Options"};
        group_grounder.addOptions() //
            ("const,c", parse(parse_const)->arg("<id>=<term>")->composing(),
             "Replace term occurrences of <id> with <term>")                                      //
            ("parts", parse(parse_parts), "Parse comma-separated program parts to ground")        //
            ("imin", parse(parse_imin)->arg("<n>"), "Minimium number of steps for incmode")       //
            ("imax", parse(parse_imax)->arg("{none|<n>}"), "Maximum number of steps for incmode") //
            ("istop",
             storeTo(solver_opts_.istop, values<Control::IStop>({{"none", Control::IStop::none},
                                                                 {"sat", Control::IStop::sat},
                                                                 {"unsat", Control::IStop::unsat},
                                                                 {"unknown", Control::IStop::unknown}})),

             "Stop when {none|sat|unsat|unknown} in incmode") //
            ("projection-mode,@1",
             storeTo(rewrite_opts_.project_mode = Input::ProjectionMode::pure,
                     values<Input::ProjectionMode>({
                         {"none", Input::ProjectionMode::disabled},
                         {"anonymous", Input::ProjectionMode::anonymous},
                         {"pure", Input::ProjectionMode::pure},
                     })),
             "Project {none|anonymous|pure} variables") //
            ("project-anonymous,@1", flag(rewrite_opts_.project_anonymous = false),
             "Project anonymous variables in negative literals");
        root.add(group_grounder);

        auto group_basic = OptionGroup{"Basic Options"};
        group_basic.addOptions()                                                                   //
            ("single-shot", flag(solver_opts_.single_shot = false), "Force single shot solving")   //
            ("log-level,@1", parse(parse_level), "Select log level {error|warn|info|debug|trace}") //
            ("info,W,@1", parse(parse_info), R"(Enable/disable specific info messages:
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
        if (parts_) {
            slv.set_parts(parts_, Input::Precedence::override_);
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
    std::optional<Control::ProgramParamVec> parts_;
    std::vector<std::pair<SharedString, SharedSymbol>> const_defs_;
};

} // namespace Clingo::CAPI
