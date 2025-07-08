#include <clingo/control/aggregate.hh>
#include <clingo/control/condlit.hh>
#include <clingo/control/context.hh>
#include <clingo/control/literal.hh>
#include <clingo/control/statement.hh>
#include <clingo/control/theory.hh>

namespace CppClingo::Control {

namespace {

//! Translator for head literals.
class BuilderHdLit {
  public:
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    void operator()(Input::HdLitSetAggregate const &lit) const {
        CLINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::HdLitTheoryAtom const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitDisjunction const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitAggregate const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitSimple const &lit) const {
        // TODO:
        // The context here:
        // - the source
        // - the rewritten statement (if different from the original)
        // The context should be a list of printable objects.
        ctx_->gcomp().add(std::make_unique<Ground::StmRule>(ctx_->simple_lit(lit.lit()), std::move(ctx_->body()),
                                                            Ground::RuleType::normal));
    }

  private:
    BuildContext *ctx_;
};

//! Translator for body literals.
class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    void operator()(Input::BdLitSetAggregate const &lit) const {
        CLINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::BdLitTheoryAtom const &lit) const { build_bd_lit(*ctx_, lit); }
    void operator()(Input::BdLitAggregate const &lit) const { build_bd_lit(*ctx_, lit); }
    void operator()(Input::BdLitSimple const &lit) const {
        build_lit(*ctx_, lit.lit(),
                  [this]<class Lit>(Lit &&glit) { ctx_->body().emplace_back(std::forward<Lit>(glit)); });
    }
    void operator()(Input::BdLitConjunction const &lit) const { build_bd_lit(*ctx_, lit); }

  private:
    BuildContext *ctx_;
};

//! Translator for statements.
class BuilderStm {
  public:
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &stm) const {
        static_assert(Util::matches<T, Input::StmComment, Input::StmConst, Input::StmParts, Input::StmDefined,
                                    Input::StmInclude, Input::StmOptimize, Input::StmProgram, Input::StmProjectSig,
                                    Input::StmScript, Input::StmShowNothing, Input::StmShowSig, Input::StmTheory>);
        CLINGO_REPORT(ctx_->logger(), error) << "unexpected statement: " << stm;
        throw std::logic_error("unexpected statement");
    }

    void operator()(Input::StmRule const &stm) const {
        build_body_(stm.body(), 1);
        std::visit(BuilderHdLit{*ctx_}, stm.head());
    }

    void operator()(Input::StmWeakConstraint const &stm) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body());
        auto const &tuple = stm.tuple();
        auto prio = build_term_(tuple.prio());
        auto weight = build_term_(tuple.weight());
        auto terms = build_term_vec_(tuple.terms());
        ctx_->gcomp().add(std::make_unique<Ground::StmWeakConstraint>(
            location(tuple.weight()), std::move(weight),
            prio ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*tuple.prio()), *std::move(prio))
                 : std::nullopt,
            std::move(terms), std::move(ctx_->body())));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmHeuristic const &stm) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body());
        auto atom = build_term_(stm.atom());
        auto &base = ctx_->add_base(*signature(stm.atom()));
        auto prio = build_term_(stm.prio());
        auto weight = build_term_(stm.weight());
        auto type = build_term_(stm.type());
        ctx_->gcomp().add(std::make_unique<Ground::StmHeuristic>(
            std::move(atom), base, std::move(ctx_->body()), location(stm.weight()), std::move(weight),
            prio ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*stm.prio()), *std::move(prio))
                 : std::nullopt,
            location(stm.type()), std::move(type)));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmEdge const &stm) const {
        build_body_(stm.body());
        assert(stm.edges().size() == 1);
        auto u = build_term_(stm.edges().front().src());
        auto v = build_term_(stm.edges().front().dst());
        ctx_->gcomp().add(std::make_unique<Ground::StmEdge>(std::move(u), std::move(v), std::move(ctx_->body())));
    }

    void operator()(Input::StmExternal const &stm) const {
        // NOLINTBEGIN(bugprone-unchecked-optional-access)
        build_body_(stm.body());
        auto [atom, base, indices] = ctx_->simple_lit(stm.atom());
        auto type = build_term_(stm.type());
        ctx_->gcomp().add(std::make_unique<Ground::StmExternal>(
            std::move(atom), base, std::move(indices), std::move(ctx_->body()),
            type ? std::make_optional<std::pair<Location, Ground::UTerm>>(location(*stm.type()), *std::move(type))
                 : std::nullopt));
        // NOLINTEND(bugprone-unchecked-optional-access)
    }

    void operator()(Input::StmShow const &stm) const {
        build_body_(stm.body());
        auto term = build_term_(stm.term());
        ctx_->gcomp().add(std::make_unique<Ground::StmShow>(std::move(term), std::move(ctx_->body())));
    }

    void operator()(Input::StmProject const &stm) const {
        build_body_(stm.body());
        auto atom = build_term_(stm.atom());
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto &base = ctx_->add_base(*signature(stm.atom()));
        ctx_->gcomp().add(std::make_unique<Ground::StmProject>(std::move(atom), base, std::move(ctx_->body())));
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

    void build_body_(Input::BdLitArray const &body, size_t extra = 0) const {
        auto bld_bd = BuilderBdLit{*ctx_};
        ctx_->body().reserve(body.size() + extra);
        for (auto const &lit : body) {
            std::visit(bld_bd, lit);
        }
    }

    BuildContext *ctx_;
};

} // namespace

void build_stm(BuildContext &ctx, Input::Stm const &stm, Input::Stm const *src) {
    // TODO: handle source statement
    std::ignore = src;
    std::visit(BuilderStm{ctx}, stm);
}

} // namespace CppClingo::Control
