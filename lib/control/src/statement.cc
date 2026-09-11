#include <clingo/control/aggregate.hh>
#include <clingo/control/condlit.hh>
#include <clingo/control/context.hh>
#include <clingo/control/literal.hh>
#include <clingo/control/statement.hh>
#include <clingo/control/theory.hh>

#include <clingo/ground/condlit.hh>
#include <clingo/ground/sort.hh>

namespace CppClingo::Control {

namespace {

void build_sort(BuildContext &ctx, Input::BdLitSort const &lit, Ground::ProfileNodeInternal *node) {
    auto vars_body = Ground::VariableSet{};
    for (auto const &body_lit : ctx.body()) {
        body_lit->vars(vars_body, Ground::VarSelectMode::all);
    }
    auto vars_global = Ground::VariableSet{};
    auto elems = std::vector<std::pair<Ground::UTerm, Ground::ULitVec>>{};
    auto domain = true;
    auto single_pass = true;
    for (auto const &elem : lit.elems()) {
        auto elem_vars = Ground::VariableSet{};
        auto value = build_term(ctx.var_map(), elem.tuple().front());
        value->vars(elem_vars);
        auto cond = Ground::ULitVec{};
        for (auto const &condition : elem.cond()) {
            single_pass = single_pass && ctx.single_pass(condition);
            build_lit(ctx, condition, [&cond, &elem_vars, &domain]<class Lit>(Lit &&ground_lit) {
                ground_lit->vars(elem_vars, Ground::VarSelectMode::all);
                domain = domain && ground_lit->domain();
                cond.emplace_back(std::forward<Lit>(ground_lit));
            });
        }
        for (auto var : elem_vars) {
            if (vars_body.contains(var)) {
                vars_global.emplace(var);
            }
        }
        elems.emplace_back(std::move(value), std::move(cond));
    }
    // FIXME: this will lead to runtime errors if the tuple is not a pair of terms
    // we also do not need to check that the term is a tuple but can simply use the term
    auto const &tuple = std::get<Input::TermTuple>(lit.lhs());
    auto const &outputs = std::get<Input::ArgumentTuple>(tuple.pool().front()).elems();
    auto prev = build_term(ctx.var_map(), std::get<Input::Term>(outputs[0]));
    auto next = build_term(ctx.var_map(), std::get<Input::Term>(outputs[1]));
    auto priority = ctx.inc_priority();
    auto member_index = domain && single_pass ? Ground::stratified_index : ctx.next_index();
    auto &state =
        ctx.state<Ground::StateSort>(ctx.mbr(), vars_global.release(), prev->copy(), next->copy(), member_index);

    if (!domain || !single_pass) {
        Ground::ProfileNodeInternal *sub_node = nullptr;
        auto get_node = [&]() {
            if (node != nullptr && sub_node == nullptr) {
                sub_node =
                    &node->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::BdLitSort>>(lit, false));
            }
            return sub_node;
        };

        for (auto &[value, cond] : elems) {
            auto body = Ground::ULitVec{};
            for (auto const &condition : cond) {
                body.emplace_back(condition->copy());
            }
            auto num_cond = body.size();
            auto prefix = Ground::copy_uvec(ctx.body());
            body.insert(body.end(), std::make_move_iterator(prefix.begin()), std::make_move_iterator(prefix.end()));
            ctx.gcomp().add(std::make_unique<Ground::StmSortMember>(state, value->copy(), std::move(body), num_cond,
                                                                    priority, get_node()));
        }

        ctx.body().emplace_back(std::make_unique<Ground::LitSortMember>(state, prev->copy()));
        ctx.body().emplace_back(std::make_unique<Ground::LitSortMember>(state, next->copy()));
        ctx.body().emplace_back(std::make_unique<Ground::LitComparison>(prev->copy(), Relation::less, next->copy()));

        auto pair_vars = Ground::VariableSet{state.global().begin(), state.global().end()};
        prev->vars(pair_vars);
        next->vars(pair_vars);
        for (auto &[value, cond] : elems) {
            auto elem_vars = Ground::VariableSet{};
            value->vars(elem_vars);
            for (auto const &condition : cond) {
                condition->vars(elem_vars, Ground::VarSelectMode::all);
            }
            for (auto var : pair_vars) {
                elem_vars.erase(var);
            }
            auto empty_index = ctx.next_index();
            auto premise_index = ctx.next_index();
            auto &condition = ctx.state<Ground::StateCondLit>(ctx.mbr(), elem_vars.release(),
                                                              Ground::VariableVec{pair_vars.begin(), pair_vars.end()},
                                                              premise_index, false, false, false);

            ctx.gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::empty, condition,
                                                                 Ground::copy_uvec(ctx.body()), ctx.inc_priority(),
                                                                 empty_index, get_node()));
            auto premise = Ground::ULitVec{};
            premise.emplace_back(
                std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, condition, empty_index));
            for (auto &elem_condition : cond) {
                premise.emplace_back(std::move(elem_condition));
            }
            premise.emplace_back(std::make_unique<Ground::LitComparison>(prev->copy(), Relation::less, value->copy()));
            premise.emplace_back(std::make_unique<Ground::LitComparison>(value->copy(), Relation::less, next->copy()));
            ctx.gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::premise, condition,
                                                                 std::move(premise), ctx.inc_priority(), premise_index,
                                                                 get_node()));
            ctx.body().emplace_back(
                std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::lit, condition, premise_index));
        }
        return;
    }

    auto statements = std::vector<Ground::StmSortElem>{};
    statements.reserve(elems.size());
    Ground::ProfileNodeInternal *sub_node = nullptr;
    for (auto &[value, cond] : elems) {
        if (node != nullptr && sub_node == nullptr) {
            sub_node = &node->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::BdLitSort>>(lit, true));
        }
        auto num_cond = cond.size();
        cond.emplace_back(std::make_unique<Ground::LitTuple>(state.global(), state.symbols()));
        statements.emplace_back(state, std::move(value), std::move(cond), num_cond, priority, sub_node);
    }
    ctx.body().emplace_back(std::make_unique<Ground::LitSortStrat>(state, std::move(statements)));
}

//! Translator for head literals.
class BuilderHdLit {
  public:
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    void operator()(Input::HdLitSetAggregate const &lit, Ground::ProfileNodeInternal *node) const {
        std::ignore = node;
        CLINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::HdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node) const {
        build_hd_lit(*ctx_, lit, node);
    }
    void operator()(Input::HdLitDisjunction const &lit, Ground::ProfileNodeInternal *node) const {
        build_hd_lit(*ctx_, lit, node);
    }
    void operator()(Input::HdLitAggregate const &lit, Ground::ProfileNodeInternal *node) const {
        build_hd_lit(*ctx_, lit, node);
    }
    void operator()(Input::HdLitSimple const &lit, Ground::ProfileNodeInternal *node) const {
        ctx_->gcomp().add(std::make_unique<Ground::StmRule>(ctx_->simple_lit(lit.lit()), std::move(ctx_->body()),
                                                            Ground::RuleType::normal, node));
    }

  private:
    BuildContext *ctx_;
};

//! Translator for body literals.
class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    void operator()(Input::BdLitSetAggregate const &lit, Ground::ProfileNodeInternal *node) const {
        std::ignore = node;
        CLINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::BdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node) const {
        build_bd_lit(*ctx_, lit, node);
    }
    void operator()(Input::BdLitAggregate const &lit, Ground::ProfileNodeInternal *node) const {
        build_bd_lit(*ctx_, lit, node);
    }
    void operator()(Input::BdLitSort const &lit, Ground::ProfileNodeInternal *node) const {
        build_sort(*ctx_, lit, node);
    }
    void operator()(Input::BdLitSimple const &lit, Ground::ProfileNodeInternal *node) const {
        std::ignore = node;
        build_lit(*ctx_, lit.lit(),
                  [this]<class Lit>(Lit &&glit) { ctx_->body().emplace_back(std::forward<Lit>(glit)); });
    }
    void operator()(Input::BdLitConjunction const &lit, Ground::ProfileNodeInternal *node) const {
        build_bd_lit(*ctx_, lit, node);
    }

  private:
    BuildContext *ctx_;
};

//! Translator for statements.
class BuilderStm {
  public:
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &stm, Ground::ProfileNode *node) const {
        static_assert(Util::matches<T, Input::StmComment, Input::StmConst, Input::StmParts, Input::StmDefined,
                                    Input::StmInclude, Input::StmOptimize, Input::StmProgram, Input::StmProjectSig,
                                    Input::StmScript, Input::StmShowNothing, Input::StmShowSig, Input::StmTheory>);
        std::ignore = node;
        CLINGO_REPORT(ctx_->logger(), error) << "unexpected statement: " << stm;
        throw std::logic_error("unexpected statement");
    }

    void operator()(Input::StmRule const &stm, Ground::ProfileNodeInternal *node) const {
        build_body_(stm.body(), node, 1);
        std::visit(BuilderHdLit{*ctx_}, stm.head(), std::variant<Ground::ProfileNodeInternal *>{node});
    }

    void operator()(Input::StmWeakConstraint const &stm, Ground::ProfileNodeInternal *node) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body(), node);
        auto const &tuple = stm.tuple();
        auto prio = build_term_(tuple.prio());
        auto weight = build_term_(tuple.weight());
        auto terms = build_term_vec_(tuple.terms());
        ctx_->gcomp().add(std::make_unique<Ground::StmWeakConstraint>(
            location(tuple.weight()), std::move(weight),
            prio ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*tuple.prio()), *std::move(prio))
                 : std::nullopt,
            std::move(terms), std::move(ctx_->body()), node));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmHeuristic const &stm, Ground::ProfileNodeInternal *node) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body(), node);
        auto atom = build_term_(stm.atom());
        auto &base = ctx_->add_base(*signature(stm.atom()));
        auto prio = build_term_(stm.prio());
        auto weight = build_term_(stm.weight());
        auto type = build_term_(stm.type());
        ctx_->gcomp().add(std::make_unique<Ground::StmHeuristic>(
            std::move(atom), base, std::move(ctx_->body()), location(stm.weight()), std::move(weight),
            prio ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*stm.prio()), *std::move(prio))
                 : std::nullopt,
            location(stm.type()), std::move(type), node));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmEdge const &stm, Ground::ProfileNodeInternal *node) const {
        build_body_(stm.body(), node);
        assert(stm.edges().size() == 1);
        auto u = build_term_(stm.edges().front().src());
        auto v = build_term_(stm.edges().front().dst());
        ctx_->gcomp().add(std::make_unique<Ground::StmEdge>(std::move(u), std::move(v), std::move(ctx_->body()), node));
    }

    void operator()(Input::StmExternal const &stm, Ground::ProfileNodeInternal *node) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body(), node);
        auto [atom, base, indices] = ctx_->simple_lit(stm.atom());
        auto type = build_term_(stm.type());
        ctx_->gcomp().add(std::make_unique<Ground::StmExternal>(
            std::move(atom), base, std::move(indices), std::move(ctx_->body()),
            type ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*stm.type()), *std::move(type))
                 : std::nullopt,
            node));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmShow const &stm, Ground::ProfileNodeInternal *node) const {
        build_body_(stm.body(), node);
        auto term = build_term_(stm.term());
        ctx_->gcomp().add(std::make_unique<Ground::StmShow>(std::move(term), std::move(ctx_->body()), node));
    }

    void operator()(Input::StmProject const &stm, Ground::ProfileNodeInternal *node) const {
        build_body_(stm.body(), node);
        auto atom = build_term_(stm.atom());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto &base = ctx_->add_base(*signature(stm.atom()));
        ctx_->gcomp().add(std::make_unique<Ground::StmProject>(std::move(atom), base, std::move(ctx_->body()), node));
    }

  private:
    [[nodiscard]] auto build_term_(std::optional<Input::Term> const &term) const -> std::optional<Ground::UTerm> {
        if (term) {
            return build_term(ctx_->var_map(), *term);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto build_term_(Input::Term const &term) const -> Ground::UTerm {
        return build_term(ctx_->var_map(), term);
    }

    [[nodiscard]] auto build_term_vec_(Input::TermArray const &terms) const -> Ground::UTermVec {
        Ground::UTermVec res;
        res.reserve(terms.size());
        for (auto const &term : terms) {
            res.emplace_back(build_term_(term));
        }
        return res;
    }

    void build_body_(Input::BdLitArray const &body, Ground::ProfileNodeInternal *node, size_t extra = 0) const {
        auto bld_bd = BuilderBdLit{*ctx_};
        ctx_->body().reserve(body.size() + extra);
        for (auto const &lit : body) {
            std::visit(bld_bd, lit, std::variant<Ground::ProfileNodeInternal *>{node});
        }
    }

    BuildContext *ctx_;
};

} // namespace

void build_stm(BuildContext &ctx, Input::Stm const &stm, Input::Stm const *src) {
    Ground::ProfileNodeInternal *root = nullptr;
    if (src != nullptr) {
        root = &ctx.profile().add(*src);
        if (stm != *src) {
            root = &root->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::Stm const>>(stm));
        }
    }
    std::visit(BuilderStm{ctx}, stm, std::variant<Ground::ProfileNodeInternal *>{root});
}

} // namespace CppClingo::Control
