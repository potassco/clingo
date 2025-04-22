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
    ClingoOptions(Logger &log, SymbolStore &store) : store_{&store}, parser_{log, store} {}

    void init(Potassco::ProgramOptions::OptionContext &root) {
        using namespace Potassco::ProgramOptions;
        auto group_grounder = OptionGroup{"Grounder Options"};
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
            auto ret = std::from_chars(value.data(), end, iopts_.imin);
            return ret.ec == std::errc{} && ret.ptr == end;
        };
        auto parse_imax = [this]([[maybe_unused]] std::string const &name, std::string const &value) {
            if (ieq(value, "none")) {
                iopts_.imax = std::nullopt;
                return true;
            }
            iopts_.imax = 0;
            const auto *end = value.data() + value.size();
            auto ret = std::from_chars(value.data(), end, *iopts_.imax);
            return ret.ec == std::errc{} && ret.ptr == end;
        };

        group_grounder.addOptions() //
            ("const,c", parse(parse_const)->arg("<id>=<term>")->composing(),
             "Replace term occurrences of <id> with <term>")                                      //
            ("parts", parse(parse_parts), "Parse comma-separated program parts to ground")        //
            ("imin", parse(parse_imin)->arg("<n>"), "Minimium number of steps for incmode")       //
            ("imax", parse(parse_imax)->arg("{none|<n>}"), "Maximum number of steps for incmode") //
            ("istop",
             storeTo(iopts_.istop, values<Control::IStop>({{"none", Control::IStop::none},
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
    }

    void apply(Control::Solver &slv) {
        for (auto const &[name, value] : const_defs_) {
            slv.add_const(*name, *value);
        }
        if (parts_) {
            slv.set_parts(parts_, Input::Precedence::override_);
        }
    }

    auto rewrite_options() -> Input::RewriteOptions const & { return rewrite_opts_; }
    auto ioptions() -> Control::IOptions const & { return iopts_; }

  private:
    SymbolStore *store_;
    Input::Parser parser_;
    Input::RewriteOptions rewrite_opts_;
    Control::IOptions iopts_;
    std::optional<Control::ProgramParamVec> parts_;
    std::vector<std::pair<SharedString, SharedSymbol>> const_defs_;
};

} // namespace Clingo::CAPI
