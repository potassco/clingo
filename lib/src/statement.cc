#include <sstream>

#include <util/print.hh>

#include <statement.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

////////// Statement //////////

namespace {

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

struct ProjectAnonymous {
    auto operator()(SLiteral const &lit) -> std::optional<SLiteral> { return lit->project_anonymous(); };
    auto operator()(SHeadLiteral const &lit) -> std::optional<SHeadLiteral> { return lit->project_anonymous(); };
    auto operator()(SBodyLiteral const &lit) -> std::optional<SBodyLiteral> { return lit->project_anonymous(); };
};

auto tpa(auto const &x) { return Trans(x, ProjectAnonymous{}); }

void visit_body(VarVisitFun const &fun, VariableContext ctx, SBodyLiteralVec const &body) {
    for (auto const &lit : body) {
        lit->visit_variables(fun, ctx);
    }
}

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

using GlobalVarSelector = detail::VarVisitHelper<GlobalVarSelectorHelper>;

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
    detail::VarOccCounts counts_;
};

using GlobalVarCounter = detail::VarVisitHelper<GlobalVarCounterHelper>;

auto global_var_counts(auto const &...args) {
    GlobalVarSelector selector;
    (selector.add(args), ...);
    GlobalVarCounter counter{selector};
    (counter.add(args), ...);
    return std::move(counter).counts();
}

auto project_body_with(ProjectionMode mode, auto const *self, SBodyLiteralVec const &body_, bool in_classical_scope,
                       auto construct) -> std::optional<SStatement> {
    // count global variables
    auto counts = global_var_counts(*self);
    Projection project{mode, counts};

    // project body
    std::optional<SBodyLiteralVec> body = transform(
        [project, in_classical_scope](SBodyLiteral const &lit) { return lit->project(project, in_classical_scope); },
        body_);
    if (body.has_value()) {
        return construct(std::move(body).value());
    }
    return std::nullopt;
}

class RewriteAnonymousStm {
  public:
    RewriteAnonymousStm(Statement const &stm) : gen_{vars_(stm)} {}
    auto operator()(SBodyLiteral const &lit) -> std::optional<SBodyLiteral> { return lit->rewrite_anonymous(gen_); }
    auto operator()(SHeadLiteral const &lit) -> std::optional<SHeadLiteral> { return lit->rewrite_anonymous(gen_); }
    auto operator()(SLiteral const &lit) -> std::optional<SLiteral> { return lit->rewrite_anonymous(gen_); }
    auto operator()(STerm const &term) -> std::optional<STerm> { return term->rewrite_anonymous(gen_); }

  private:
    static auto vars_(Statement const &stm) -> VariableSet {
        VariableSet vars;
        stm.visit_variables([&vars](std::string const &var) { vars.emplace(var); }, VariableContext::all);
        return vars;
    }
    NameGen gen_;
};

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
    stm = stm->rewrite_anonymous().value_or(stm);
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

auto Statement::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
    stm.print(out);
    return out;
}

////////// Rule //////////

void Rule::print(std::ostream &out) const {
    out << *head_;
    if (head_->print_empty() || !body_.empty()) {
        out << " :- " << p_range(body_, "; ");
    }
    out << ".";
}

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

    return project_body_with(mode, this, body_, head_->is_classical(),
                             [&](auto body) { return construct_shared<Rule, Statement>(head_, std::move(body)); });
}

auto Rule::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<Rule, Statement>(tpa(head_), tpa(body_));
}

auto Rule::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<Rule, Statement>(Trans{head_, fun}, Trans{body_, fun});
}

////////// TheoryOpDefinition //////////

auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream & {
    out << def.op_ << " : " << def.prio_ << ", ";
    switch (def.type_) {
        case TheoryOpType::unary: {
            out << "unary";
            break;
        }
        case TheoryOpType::binary_left: {
            out << "binary, left";
            break;
        }
        case TheoryOpType::binary_right: {
            out << "binary, right";
            break;
        }
    }
    return out;
}

////////// TheoryTermDefinition //////////

auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream & {
    out << "  " << def.name_ << " {";
    if (def.op_defs_.empty()) {
        out << " }";
    } else if (def.op_defs_.size() == 1) {
        out << " " << def.op_defs_.front() << " }";
    } else {
        out << "\n"
            << p_range_with(def.op_defs_, ";\n", [](std::ostream &out, auto &op_def) { out << "    " << op_def; })
            << "\n  }";
    }
    return out;
}

////////// TheoryAtomDefinition //////////

auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream & {
    switch (type) {
        case TheoryAtomType::head: {
            out << "head";
            break;
        }
        case TheoryAtomType::body: {
            out << "body";
            break;
        }
        case TheoryAtomType::any: {
            out << "any";
            break;
        }
        case TheoryAtomType::directive: {
            out << "directive";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream & {
    out << "  &" << def.name_ << "/" << def.arity_ << ": " << def.term_ << ", ";
    if (def.rhs_) {
        out << "{" << p_range(def.rhs_->first, ",") << "}, " << def.rhs_->second << ", ";
    }
    out << def.type_;
    return out;
}

////////// TheoryDefinition //////////

void TheoryDefinition::print(std::ostream &out) const {
    out << "#theory " << name_ << (term_defs_.empty() && atom_defs_.empty() ? " { " : " {\n");
    out << p_range(term_defs_, ";\n");
    if (!term_defs_.empty()) {
        if (!atom_defs_.empty()) {
            out << ";";
        }
        out << "\n";
    }
    out << p_range(atom_defs_, ";\n");
    if (!atom_defs_.empty()) {
        out << "\n";
    }
    out << "}.";
}

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

auto TheoryDefinition::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementOptimize //////////

auto operator<<(std::ostream &out, OptimizeType type) -> std::ostream & {
    switch (type) {
        case OptimizeType::maximize: {
            out << "#maximize";
            break;
        }
        case OptimizeType::minimize: {
            out << "#minimize";
            break;
        }
    }
    return out;
}

void StatementOptimize::print(std::ostream &out) const {
    out << type_ << " { "
        << p_range_with(elems_, "; ",
                        [](std::ostream &out, auto const &elem) {
                            auto const &[tuple, cond] = elem;
                            auto const &[weight, prio, terms] = tuple;
                            out << *weight;
                            if (prio) {
                                out << "@" << *prio.value();
                            }
                            if (!terms.empty()) {
                                out << "," << p_range(terms);
                            }
                            if (!cond.empty()) {
                                out << ": " << p_range(cond, ", ");
                            }
                        })
        << (elems_.empty() ? "}" : " }") << ".";
}

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
        auto counts = global_var_counts(tuple, cond);
        auto sub_project = Projection{mode, counts};

        // project literals in condition
        auto fun = [sub_project](SLiteral const &lit) { return lit->project(sub_project); };
        return transform_construct<Element>(tuple, Trans{cond, fun});
    };
    return transform_construct_shared<StatementOptimize, Statement>(type_, Trans{elems_, fun});
}

auto StatementOptimize::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementOptimize, Statement>(type_, tpa(elems_));
}

auto StatementOptimize::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementOptimize, Statement>(type_, Trans{elems_, fun});
}

////////// StatementWeakConstraint //////////

void StatementWeakConstraint::print(std::ostream &out) const {
    auto const &[weight, prio, terms] = tuple_;
    out << " :~ " << p_range(body_, "; ") << ". [" << *weight;
    if (prio) {
        out << "@" << *prio.value();
    }
    if (!terms.empty()) {
        out << "," << p_range(terms);
    }
    out << "]";
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementWeakConstraint, Statement>(std::move(body), tuple_);
    });
}

auto StatementWeakConstraint::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementWeakConstraint, Statement>(tpa(body_), tuple_);
}

auto StatementWeakConstraint::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementWeakConstraint, Statement>(Trans{body_, fun}, Trans{tuple_, fun});
}

////////// StatementShow //////////

void StatementShow::print(std::ostream &out) const {
    char const *lp = "";
    char const *rp = "";
    if (term_->check_type(TermCheckType::sig, nullptr)) {
        lp = "(";
        rp = ")";
    }
    out << "#show " << lp << *term_ << rp << ": " << p_range(body_, "; ") << ".";
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementShow, Statement>(term_, std::move(body));
    });
}

auto StatementShow::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementShow, Statement>(term_, tpa(body_));
}

auto StatementShow::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementShow, Statement>(Trans{term_, fun}, Trans{body_, fun});
}

////////// StatementShowSig //////////

void StatementShowSig::print(std::ostream &out) const {
    out << "#show " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

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

auto StatementShowSig::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementProject //////////

void StatementProject::print(std::ostream &out) const {
    out << "#project " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementProject, Statement>(term_, std::move(body));
    });
}

auto StatementProject::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementProject, Statement>(term_, tpa(body_));
}

auto StatementProject::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementProject, Statement>(Trans{term_, fun}, Trans{body_, fun});
}

////////// StatementProjectSig //////////

void StatementProjectSig::print(std::ostream &out) const {
    out << "#project " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

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

auto StatementProjectSig::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementDefined //////////

void StatementDefined::print(std::ostream &out) const {
    out << "#defined " << (has_sign_ ? "-" : "") << name_ << "/" << arity_ << ".";
}

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

auto StatementDefined::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementExternal //////////

void StatementExternal::print(std::ostream &out) const {
    out << "#external " << *term_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
    if (type_.has_value()) {
        out << " [" << *type_.value() << "]";
    }
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementExternal, Statement>(term_, std::move(body), type_);
    });
}

auto StatementExternal::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementExternal, Statement>(term_, tpa(body_), type_);
}

auto StatementExternal::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementExternal, Statement>(Trans{term_, fun}, Trans{body_, fun},
                                                                    Trans{type_, fun});
}

////////// StatementEdge //////////

void StatementEdge::print(std::ostream &out) const {
    out << "#edge ("
        << p_range_with(edges_, ";", [](std::ostream &out, auto &edge) { out << *edge.first << "," << *edge.second; })
        << ")" << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ".";
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementEdge, Statement>(edges_, std::move(body));
    });
}

auto StatementEdge::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementEdge, Statement>(edges_, tpa(body_));
}

auto StatementEdge::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementEdge, Statement>(Trans{edges_, fun}, Trans{body_, fun});
}

////////// StatementHeuristic //////////

void StatementHeuristic::print(std::ostream &out) const {
    out << "#heuristic " << *atom_ << (body_.empty() ? "" : ": ") << p_range(body_, "; ") << ". [" << *type_;
    if (prio_) {
        out << "@" << *prio_.value();
    }
    out << "," << *mod_ << "]";
}

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
    return project_body_with(mode, this, body_, true, [&](auto body) {
        return construct_shared<StatementHeuristic, Statement>(atom_, std::move(body), type_, prio_, mod_);
    });
}

auto StatementHeuristic::do_project_anonymous() const -> std::optional<SStatement> {
    return transform_construct_shared<StatementHeuristic, Statement>(atom_, tpa(body_), type_, prio_, mod_);
}

auto StatementHeuristic::rewrite_anonymous() const -> std::optional<SStatement> {
    RewriteAnonymousStm fun{*this};
    return transform_construct_shared<StatementHeuristic, Statement>(
        Trans{atom_, fun}, Trans{body_, fun}, Trans{type_, fun}, Trans{prio_, fun}, Trans{mod_, fun});
}

////////// StatementScript //////////

auto operator<<(std::ostream &out, ScriptType type) -> std::ostream & {
    switch (type) {
        case ScriptType::lua: {
            out << "lua";
            break;
        }
        case ScriptType::python: {
            out << "python";
            break;
        }
    }
    return out;
}

void StatementScript::print(std::ostream &out) const { out << "#script (" << type_ << ")" << content_ << "#end."; }

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

auto StatementScript::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementInclude //////////

auto operator<<(std::ostream &out, IncludeType type) -> std::ostream & {
    switch (type) {
        case IncludeType::inbuild: {
            out << "lua";
            break;
        }
        case IncludeType::system: {
            out << "python";
            break;
        }
    }
    return out;
}

void StatementInclude::print(std::ostream &out) const {
    if (type_ == IncludeType::inbuild) {
        out << "#include <" << path_ << ">.";
    } else {
        out << "#include ";
        print_quoted(out, path_);
        out << ".";
    }
}

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

auto StatementInclude::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementProgram //////////

void StatementProgram::print(std::ostream &out) const {
    out << "#program " << name_;
    if (!args_.empty()) {
        out << "(" << p_range(args_) << ")";
    }
    out << ".";
}

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

auto StatementProgram::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }

////////// StatementConst //////////

auto operator<<(std::ostream &out, ConstType type) -> std::ostream & {
    switch (type) {
        case ConstType::default_: {
            out << "default";
            break;
        }
        case ConstType::override_: {
            out << "override";
            break;
        }
    }
    return out;
}

void StatementConst::print(std::ostream &out) const {
    out << "#const " << name_ << "=" << *value_ << ". [" << type_ << "]";
}

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

auto StatementConst::rewrite_anonymous() const -> std::optional<SStatement> { return std::nullopt; }
