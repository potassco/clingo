#pragma once

#include <clingo/control/context.hh>
#include <clingo/control/term.hh>

#include <clingo/input/literal.hh>
#include <clingo/input/print.hh>

#include <clingo/ground/literal.hh>

#include <clingo/input/rewrite/analyze.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

namespace Detail {

//! Translate input literals to their ground representation.
//!
//! Assumes that literals have been rewritten.
template <class F, bool stratify = false> class BuilderLit {
  public:
    //! Construct the translator.
    BuilderLit(BuildContext &ctx, F cb) : cb_{std::move(cb)}, ctx_{&ctx} {}
    //! Translate Boolean literals.
    //!
    //! @note: This should never be called on rewritten programs.
    void operator()(Input::LitBool const &lit) const { cb_(std::make_unique<Ground::LitBool>(lit.value())); }
    //! Translate comparison literals.
    //!
    //! This function also handles intervals and external functions.
    //!
    //! @todo: External functions have not yet been implemented.
    void operator()(Input::LitComparison const &lit) const {
        if (Input::is_interval(lit.rhs().front().second)) {
            auto lhs = build_term(ctx_->var_map(), lit.lhs());
            auto const &rng = std::get<Input::TermBinary>(lit.rhs().front().second);
            auto lower = build_term(ctx_->var_map(), *rng.lhs());
            auto upper = build_term(ctx_->var_map(), *rng.rhs());
            cb_(std::make_unique<Ground::LitInterval>(std::move(lhs), std::move(lower), std::move(upper)));
        } else if (Input::is_external(lit.rhs().front().second)) {
            if (ctx_->context() == nullptr) {
                CLINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "script context unavailable";
                throw std::runtime_error("script context unavailable");
            }
            auto lhs = build_term(ctx_->var_map(), lit.lhs());
            auto const &rng = std::get<Input::TermFunction>(lit.rhs().front().second);
            auto args = CppClingo::Util::to_vec(rng.pool().front().elems(), [this](auto const &elem) {
                return build_term(ctx_->var_map(), std::get<Input::Term>(elem));
            });
            cb_(std::make_unique<Ground::LitExternal>(*ctx_->context(), lit.loc(), rng.name(), std::move(lhs),
                                                      std::move(args)));
        } else {
            auto add_cmp = [this](auto const &lhs, auto rel, auto const &rhs) {
                auto l = build_term(ctx_->var_map(), lhs);
                auto r = build_term(ctx_->var_map(), rhs);
                cb_(std::make_unique<Ground::LitComparison>(std::move(l), rel, std::move(r)));
            };
            auto const &lhs = lit.lhs();
            auto const &rhs = lit.rhs().front().second;
            auto rel = lit.rhs().front().first;
            add_cmp(lhs, rel, rhs);
            if (rel == Relation::equal && Input::is_matchable(rhs) && !Input::is_symbol(rhs)) {
                add_cmp(rhs, rel, lhs);
            }
        }
    }
    //! Translate symbolic literals.
    void operator()(Input::LitSymbolic const &lit) const {
        auto has_projection = false;
        auto term = build_term(ctx_->var_map(), lit.term(), has_projection);
        auto idx = stratify && lit.sign() == Sign::none ? Ground::stratified_index : ctx_->index(lit);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto &base = ctx_->add_base(Input::signature(lit.term()).value());
        if (has_projection) {
            auto [p_term, state] = ctx_->add_project(term, base);
            cb_(std::make_unique<Ground::LitProject>(*state, lit.sign(), std::move(term), std::move(p_term), idx,
                                                     ctx_->gcomp().domain()));
        } else {
            cb_(std::make_unique<Ground::LitSymbolic>(base, lit.sign(), std::move(term), idx, ctx_->gcomp().domain()));
        }
    }

  private:
    F cb_;
    BuildContext *ctx_;
};

} // namespace Detail

//! Translate input literals to their ground representation.
//!
//! Assumes that literals have been rewritten.
template <class F> void build_lit(BuildContext &ctx, Input::Lit const &lit, F &&fun) {
    std::visit(Detail::BuilderLit{ctx, std::forward<F>(fun)}, lit);
}

//! Translate input literals to their ground representation.
//!
//! Assumes that literals have been rewritten.
template <class F> void build_stratified_lit(BuildContext &ctx, Input::Lit const &lit, F &&fun) {
    std::visit(Detail::BuilderLit<std::decay_t<F>, true>{ctx, std::forward<F>(fun)}, lit);
}

//! @}

} // namespace CppClingo::Control
