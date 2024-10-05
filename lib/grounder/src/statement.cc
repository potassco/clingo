#include <gringo/grounder/aggregate.hh>
#include <gringo/grounder/condlit.hh>
#include <gringo/grounder/context.hh>
#include <gringo/grounder/literal.hh>
#include <gringo/grounder/statement.hh>
#include <gringo/grounder/theory.hh>

namespace Gringo::Grounder {

namespace {

//! Translator for head literals.
class BuilderHdLit {
  public:
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    void operator()(Input::HdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::HdLitTheoryAtom const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitDisjunction const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitAggregate const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitSimple const &lit) const {
        ctx_->gcomp().add(
            std::make_unique<Ground::StmRule>(ctx_->simple_lit(lit.lit()), std::move(ctx_->body()), false));
    }

  private:
    BuildContext *ctx_;
};

//! Translator for body literals.
class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    void operator()(Input::BdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
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
        std::ostringstream oss;
        oss << "implement me: handle statement " << stm;
        throw std::logic_error(oss.str());
    }

    void operator()(Input::StmRule const &stm) const {
        build_body_(stm.body(), 1);
        std::visit(BuilderHdLit{*ctx_}, stm.head());
    }

    void operator()(Input::StmWeakConstraint const &stm) const {
        build_body_(stm.body());
        auto const &tuple = stm.tuple();
        auto prio = build_term_(tuple.prio());
        auto weight = build_term_(tuple.weight());
        auto terms = build_term_vec_(tuple.terms());
        ctx_->gcomp().add(std::make_unique<Ground::StmWeakConstraint>(std::move(weight), std::move(prio),
                                                                      std::move(terms), std::move(ctx_->body())));
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

void build_stm(BuildContext &ctx, Input::Stm const &stm) { std::visit(BuilderStm{ctx}, stm); }

} // namespace Gringo::Grounder
