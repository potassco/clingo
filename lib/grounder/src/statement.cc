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
    //! Construct the translator.
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &stm) const {
        std::ostringstream oss;
        oss << "implement me: handle statement " << stm;
        throw std::logic_error(oss.str());
    }

    //! Translate rules.
    void operator()(Input::StmRule const &stm) const {
        auto bld_bd = BuilderBdLit{*ctx_};
        auto bld_hd = BuilderHdLit{*ctx_};
        ctx_->body().reserve(stm.body().size() + 1);
        for (auto const &lit : stm.body()) {
            std::visit(bld_bd, lit);
        }
        std::visit(bld_hd, stm.head());
    }

  private:
    BuildContext *ctx_;
};

} // namespace

void build_stm(BuildContext &ctx, Input::Stm const &stm) { std::visit(BuilderStm{ctx}, stm); }

} // namespace Gringo::Grounder
