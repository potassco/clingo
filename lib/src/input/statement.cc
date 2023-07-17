#include <sstream>

#include <input/statement.hh>

#include <input/algo/check_type.hh>
#include <input/algo/print.hh>
#include <input/algo/project.hh>
#include <input/algo/project_anonymous.hh>
#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "algo/transform.hh"
#include "algo/unpool.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

/*
namespace {

class GlobalVarSelectorHelper {
  public:
    [[nodiscard]] operator VariableSet const &() { return global_; }

    auto add(BodyLiteral const &x) { visit_(x, VariableContext::global); }
    auto add(HeadLiteral const &x) { visit_(x, VariableContext::global); }
    auto add(Statement const &x) { visit_(x, VariableContext::global); }

  protected:
    void visit_(auto const &x, auto... args) {
        visit_variables(
            x, [this](std::string const &var) { global_.emplace(var); }, args...);
    }

  private:
    VariableSet global_;
};

using GlobalVarSelector = Detail::VarVisitHelper<GlobalVarSelectorHelper>;

class GlobalVarCounterHelper {
  public:
    GlobalVarCounterHelper(VariableSet const &global) : global_{global} {}

    auto counts() && { return std::move(counts_); }

    auto add(BodyLiteral const &x) { visit_(x, VariableContext::all); }
    auto add(HeadLiteral const &x) { visit_(x, VariableContext::all); }
    auto add(Statement const &x) { visit_(x, VariableContext::all); }

  protected:
    void visit_(auto const &x, auto... args) {
        visit_variables(
            x,
            [this](std::string const &var) {
                if (global_.contains(var)) {
                    ++counts_[var];
                }
            },
            args...);
    }

  private:
    VariableSet const &global_;
    Detail::VarOccCounts counts_;
};

using GlobalVarCounter = Detail::VarVisitHelper<GlobalVarCounterHelper>;

struct Project {
    auto operator()(HeadLiteral const &lit) const { return Gringo::Input::project(lit, project); }
    auto operator()(BodyLiteral const &lit) const { return Gringo::Input::project(lit, project, in_classical_scope); }
    Projection project;
    bool in_classical_scope;
};

struct ProjectAnonymous {
    auto operator()(Literal const &lit) -> std::optional<Literal> { return project_anonymous(lit); };
    auto operator()(HeadLiteral const &lit) -> std::optional<HeadLiteral> { return project_anonymous(lit); };
    auto operator()(BodyLiteral const &lit) -> std::optional<BodyLiteral> { return project_anonymous(lit); };
};

auto vcp(auto const &...args) {
    GlobalVarSelector selector;
    (selector.add(args), ...);
    GlobalVarCounter counter{selector};
    (counter.add(args), ...);
    return std::move(counter).counts();
}

auto tp(auto const &x, ProjectionMode mode, std::unordered_map<std::string, size_t> const &counts,
        bool in_classical_scope = true) {
    return Trans{x, Project{Projection{mode, counts}, in_classical_scope}};
}

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

} // namespace

auto Statement::project(ProjectionMode mode, bool project_anonymous) const -> std::optional<SStatement> {
    std::optional<SStatement> res;
    if (mode != ProjectionMode::disabled) {
        res = do_project(mode);
    }
    if (project_anonymous) {
        if (res.has_value()) {
            auto tmp = res.value()->do_project_anonymous();
            if (tmp.has_value()) {
                res = std::move(tmp);
            }
        } else {
            res = do_project_anonymous();
        }
    }
    return res;
}

////////// Rule //////////

auto Rule::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    // do not project projection-like rules
    if (is_atom(head_)) {
        auto has_atom = std::any_of(body_.begin(), body_.end(), [](auto const &lit) { return is_atom(lit); });
        size_t n_test = std::count_if(body_.begin(), body_.end(), [](auto const &lit) { return is_test(lit); });
        if (has_atom && n_test == body_.size() - 1) {
            return std::nullopt;
        }
    }
    bool in_classical_scope = is_classical(head_);
    auto counts = vcp(*this);
    return transform_construct_shared<Rule, Statement>(tp(head_, mode, counts),
                                                       tp(body_, mode, counts, in_classical_scope));
}

////////// TheoryDefinition //////////

auto TheoryDefinition::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementOptimize //////////

auto StatementOptimize::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    using Gringo::Input::project;
    auto fun = [mode](Element const &elem) -> std::optional<Element> {
        auto const &[tuple, cond] = elem;

        // add counts of variables
        auto counts = vcp(tuple, cond);
        auto prj = Projection{mode, counts};

        // project literals in condition
        auto fun = [prj](Literal const &lit) { return project(lit, prj); };
        return transform_construct<Element>(tuple, Trans{cond, fun});
    };
    return transform_construct_shared<StatementOptimize, Statement>(type_, Trans{elems_, fun});
}

////////// StatementWeakConstraint //////////

auto StatementWeakConstraint::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementWeakConstraint, Statement>(tp(body_, mode, counts), tuple_);
}

////////// StatementShow //////////

auto StatementShow::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementShow, Statement>(term_, tp(body_, mode, counts));
}

////////// StatementShowSig //////////

auto StatementShowSig::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementProject //////////

auto StatementProject::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementProject, Statement>(term_, tp(body_, mode, counts));
}

////////// StatementProjectSig //////////

auto StatementProjectSig::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementDefined //////////

auto StatementDefined::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementExternal //////////

auto StatementExternal::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementExternal, Statement>(term_, tp(body_, mode, counts), type_);
}

////////// StatementEdge //////////

auto StatementEdge::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementEdge, Statement>(edges_, tp(body_, mode, counts));
}

////////// StatementHeuristic //////////

auto StatementHeuristic::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementHeuristic, Statement>(atom_, tp(body_, mode, counts), type_, prio_,
                                                                     mod_);
}
////////// StatementScript //////////

auto StatementScript::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementInclude //////////

auto StatementInclude::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementProgram //////////

auto StatementProgram::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

////////// StatementConst //////////

auto StatementConst::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}
*/

} // namespace Gringo::Input
