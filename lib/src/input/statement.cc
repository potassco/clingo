#include <sstream>

#include <input/statement.hh>

#include <input/algo/print.hh>
#include <input/algo/rewrite_anonymous.hh>

#include "algo/transform.hh"
#include "algo/unpool.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

////////// Statement //////////

namespace {

void visit_body(VarVisitFun const &fun, VariableContext ctx, SBodyLiteralVec const &body) {
    for (auto const &lit : body) {
        lit->visit_variables(fun, ctx);
    }
}

struct StatementUnpool {
    auto operator()(STerm const &term) const { return term->unpool(); }
    auto operator()(STermVec const &terms) const { return unpool_crossproduct(terms); }
    auto operator()(std::optional<STerm> const &term) const {
        return and_then_opt(term, [](STerm const &term) { return term->unpool(); });
    }
    auto operator()(SLiteralVec const &lits) const { return unpool_crossproduct(lits); }
    auto operator()(SHeadLiteral const &lit) const { return lit->unpool(); }
    auto operator()(SBodyLiteralVec const &body) const { return unpool_crossproduct(body); }
    auto operator()(StatementOptimize::Tuple const &tuple) const {
        return unpool_crossproducts(
            [](auto weight, auto prio, auto terms) {
                return StatementWeakConstraint::Tuple{std::move(weight), std::move(prio), std::move(terms)};
            },
            StatementUnpool{}, std::get<0>(tuple), std::get<1>(tuple), std::get<2>(tuple));
    }
    auto operator()(StatementOptimize::Element const &elem) const {
        return unpool_crossproducts(
            [](auto tuple, auto cond) {
                return StatementOptimize::Element{std::move(tuple), std::move(cond)};
            },
            StatementUnpool{}, std::get<0>(elem), std::get<1>(elem));
    }
    auto operator()(StatementEdge::Edge const &edge) const {
        return unpool_crossproducts(
            [](auto u, auto v) {
                return StatementEdge::Edge{std::move(u), std::move(v)};
            },
            StatementUnpool{}, std::get<0>(edge), std::get<1>(edge));
    }
    auto operator()(StatementEdge::EdgeVec const &edges) const { return unpool_union(edges, StatementUnpool{}); }
};

class GlobalVarSelectorHelper {
  public:
    [[nodiscard]] operator VariableSet const &() { return global_; }

    auto add(BodyLiteral const &x) { visit_(x, VariableContext::global); }
    auto add(HeadLiteral const &x) { visit_(x, VariableContext::global); }
    auto add(Statement const &x) { visit_(x, VariableContext::global); }

  protected:
    void visit_(auto const &x, auto... args) {
        x.visit_variables([this](std::string const &var) { global_.emplace(var); }, args...);
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
        x.visit_variables(
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
    auto operator()(SHeadLiteral const &lit) const { return lit->project(project); }
    auto operator()(SBodyLiteral const &lit) const { return lit->project(project, in_classical_scope); }
    Projection project;
    bool in_classical_scope;
};

struct ProjectAnonymous {
    auto operator()(SLiteral const &lit) -> std::optional<SLiteral> { return lit->project_anonymous(); };
    auto operator()(SHeadLiteral const &lit) -> std::optional<SHeadLiteral> { return lit->project_anonymous(); };
    auto operator()(SBodyLiteral const &lit) -> std::optional<SBodyLiteral> { return lit->project_anonymous(); };
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

auto Statement::unpool() const -> std::optional<SStatementVec> {
    auto stms = do_unpool();
    if (stms.has_value()) {
        VariableSet old_global;
        VariableSet new_global;
        visit_variables([&old_global](std::string const &var) { old_global.emplace(var); }, VariableContext::global);
        for (auto &stm : stms.value()) {
            new_global.clear();
            stm->visit_variables([&new_global](std::string const &var) { new_global.emplace(var); },
                                 VariableContext::global);
            stm->visit_variables(
                [&](std::string const &var) {
                    if (old_global.contains(var) != new_global.contains(var)) {
                        std::ostringstream oss;
                        oss << "variable " << var << " in\n"
                            << "  " << *this << "\n"
                            << "is unsafe";
                        throw std::runtime_error(oss.str());
                    }
                },
                VariableContext::all);
        }
    }
    return stms;
}

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

void rewrite(SStatement stm, RewriteOptions opts, SStatementVec &stms) {
    if (opts.level < RewriteLevel::rewrite_anonymous) {
        stms.emplace_back(std::move(stm));
        return;
    }
    stm = rewrite_anonymous(*stm).value_or(stm);
    if (opts.level < RewriteLevel::unpool) {
        stms.emplace_back(std::move(stm));
        return;
    }
    auto rewrite_unpooled = [&opts, &stms](SStatement stm) {
        if (opts.level < RewriteLevel::project) {
            stms.emplace_back(std::move(stm));
            return;
        }
        stm = stm->project(opts.project_mode, opts.project_anonymous).value_or(stm);
        stms.emplace_back(std::move(stm));
    };
    auto unpooled = stm->unpool();
    if (unpooled.has_value()) {
        for (auto &stm : unpooled.value()) {
            rewrite_unpooled(std::move(stm));
        }
    } else {
        rewrite_unpooled(std::move(stm));
    }
}

////////// Rule //////////

void Rule::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto Rule::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto head, auto body) { return construct_shared<Rule, Statement>(std::move(head), std::move(body)); },
        StatementUnpool{}, head_, body_);
}

void Rule::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    head_->visit_variables(fun, ctx);
    visit_body(fun, ctx, body_);
}

auto Rule::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    // do not project projection-like rules
    if (head_->is_atom()) {
        auto has_atom = std::any_of(body_.begin(), body_.end(), [](auto const &lit) { return lit->is_atom(); });
        size_t n_test = std::count_if(body_.begin(), body_.end(), [](auto const &lit) { return lit->is_test(); });
        if (has_atom && n_test == body_.size() - 1) {
            return std::nullopt;
        }
    }
    bool in_classical_scope = head_->is_classical();
    auto counts = vcp(*this);
    return transform_construct_shared<Rule, Statement>(tp(head_, mode, counts),
                                                       tp(body_, mode, counts, in_classical_scope));
}

auto Rule::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<Rule, Statement>(tpa(head_), tpa(body_));
}

////////// TheoryDefinition //////////

void TheoryDefinition::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto TheoryDefinition::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void TheoryDefinition::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto TheoryDefinition::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto TheoryDefinition::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementOptimize //////////

void StatementOptimize::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementOptimize::do_unpool() const -> std::optional<SStatementVec> {
    // TODO: consider turning into weak constraint
    return map_opt(unpool_union(elems_, StatementUnpool{}), [this](auto elems) {
        return make_vec<SStatement>(construct_shared<StatementOptimize, Statement>(type_, std::move(elems)));
    });
}

void StatementOptimize::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    if (ctx == VariableContext::all) {
        VarVisitor visit{fun};
        visit.add(elems_);
    }
}

auto StatementOptimize::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto fun = [mode](Element const &elem) -> std::optional<Element> {
        auto const &[tuple, cond] = elem;

        // add counts of variables
        auto counts = vcp(tuple, cond);
        auto project = Projection{mode, counts};

        // project literals in condition
        auto fun = [project](SLiteral const &lit) { return lit->project(project); };
        return transform_construct<Element>(tuple, Trans{cond, fun});
    };
    return transform_construct_shared<StatementOptimize, Statement>(type_, Trans{elems_, fun});
}

auto StatementOptimize::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementOptimize, Statement>(type_, tpa(elems_));
}

////////// StatementWeakConstraint //////////

void StatementWeakConstraint::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementWeakConstraint::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto body, auto tuple) {
            return construct_shared<StatementWeakConstraint, Statement>(std::move(body), std::move(tuple));
        },
        StatementUnpool{}, body_, tuple_);
}

void StatementWeakConstraint::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(tuple_);
    visit_body(fun, ctx, body_);
}

auto StatementWeakConstraint::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementWeakConstraint, Statement>(tp(body_, mode, counts), tuple_);
}

auto StatementWeakConstraint::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementWeakConstraint, Statement>(tpa(body_), tuple_);
}

////////// StatementShow //////////

void StatementShow::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementShow::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto term, auto body) {
            return construct_shared<StatementShow, Statement>(std::move(term), std::move(body));
        },
        StatementUnpool{}, term_, body_);
}

void StatementShow::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    term_->visit_variables(fun);
    visit_body(fun, ctx, body_);
}

auto StatementShow::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementShow, Statement>(term_, tp(body_, mode, counts));
}

auto StatementShow::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementShow, Statement>(term_, tpa(body_));
}

////////// StatementShowSig //////////

void StatementShowSig::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementShowSig::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementShowSig::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementShowSig::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementShowSig::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementProject //////////

void StatementProject::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementProject::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto term, auto body) {
            return construct_shared<StatementProject, Statement>(std::move(term), std::move(body));
        },
        StatementUnpool{}, term_, body_);
}

void StatementProject::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    term_->visit_variables(fun);
    visit_body(fun, ctx, body_);
}

auto StatementProject::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementProject, Statement>(term_, tp(body_, mode, counts));
}

auto StatementProject::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementProject, Statement>(term_, tpa(body_));
}

////////// StatementProjectSig //////////

void StatementProjectSig::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementProjectSig::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementProjectSig::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementProjectSig::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementProjectSig::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementDefined //////////

void StatementDefined::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementDefined::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementDefined::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementDefined::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementDefined::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementExternal //////////

void StatementExternal::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementExternal::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto term, auto body, auto type) {
            return construct_shared<StatementExternal, Statement>(std::move(term), std::move(body), std::move(type));
        },
        StatementUnpool{}, term_, body_, type_);
}

void StatementExternal::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(term_, type_);
    visit_body(fun, ctx, body_);
}

auto StatementExternal::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementExternal, Statement>(term_, tp(body_, mode, counts), type_);
}

auto StatementExternal::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementExternal, Statement>(term_, tpa(body_), type_);
}

////////// StatementEdge //////////

void StatementEdge::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementEdge::do_unpool() const -> std::optional<SStatementVec> {
    auto bodies = StatementUnpool{}(body_);
    auto edges = StatementUnpool{}(edges_);
    if (edges_.size() != 1 || edges.has_value() || bodies.has_value()) {
        SStatementVec ret;
        for (auto &body : bodies.value_or(make_vec<SBodyLiteralVec>(body_))) {
            for (auto &edge : edges.value_or(edges_)) {
                ret.emplace_back(construct_shared<StatementEdge, Statement>(make_vec<Edge>(edge), body));
            }
        }
        return ret;
    }
    return std::nullopt;
}

void StatementEdge::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(edges_);
    visit_body(fun, ctx, body_);
}

auto StatementEdge::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementEdge, Statement>(edges_, tp(body_, mode, counts));
}

auto StatementEdge::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementEdge, Statement>(edges_, tpa(body_));
}

////////// StatementHeuristic //////////

void StatementHeuristic::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementHeuristic::do_unpool() const -> std::optional<SStatementVec> {
    return unpool_crossproducts(
        [](auto atom, auto body, auto type, auto prio, auto mod) {
            return construct_shared<StatementHeuristic, Statement>(std::move(atom), std::move(body), std::move(type),
                                                                   std::move(prio), std::move(mod));
        },
        StatementUnpool{}, atom_, body_, type_, prio_, mod_);
}

void StatementHeuristic::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    VarVisitor visit{fun};
    visit.add(type_, prio_, mod_);
    visit_body(fun, ctx, body_);
}

auto StatementHeuristic::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    auto counts = vcp(*this);
    return transform_construct_shared<StatementHeuristic, Statement>(atom_, tp(body_, mode, counts), type_, prio_,
                                                                     mod_);
}

auto StatementHeuristic::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementHeuristic, Statement>(atom_, tpa(body_), type_, prio_, mod_);
}

////////// StatementScript //////////

void StatementScript::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementScript::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementScript::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementScript::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementScript::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementInclude //////////

void StatementInclude::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementInclude::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementInclude::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementInclude::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementInclude::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementProgram //////////

void StatementProgram::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementProgram::do_unpool() const -> std::optional<SStatementVec> { return std::nullopt; }

void StatementProgram::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementProgram::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementProgram::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementConst //////////

void StatementConst::accept(StatementVisitor const &visitor) const { visitor.visit(*this); }

auto StatementConst::do_unpool() const -> std::optional<SStatementVec> {
    auto ret = unpool_crossproducts(
        [this](auto value) { return construct_shared<StatementConst, Statement>(type_, name_, std::move(value)); },
        StatementUnpool{}, value_);
    if (ret.has_value() && ret->size() != 1) {
        throw std::runtime_error("const statements must not contain pools");
    }
    return ret;
}

void StatementConst::visit_variables(VarVisitFun const &fun, VariableContext ctx) const {
    static_cast<void>(fun);
    static_cast<void>(ctx);
}

auto StatementConst::do_project(ProjectionMode mode) const -> std::optional<SStatement> {
    static_cast<void>(mode);
    return std::nullopt;
}

auto StatementConst::do_project_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

} // namespace Gringo::Input
