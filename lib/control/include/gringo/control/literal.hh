#pragma once

#include <gringo/control/context.hh>
#include <gringo/control/term.hh>

#include <gringo/input/literal.hh>
#include <gringo/input/print.hh>

#include <gringo/ground/literal.hh>

#include <gringo/input/rewrite/analyze.hh>

#include <sstream>

namespace Gringo::Control {

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
    //! Translate comparision literals.
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
            std::ostringstream oss;
            oss << "implement me: handle external function call " << lit;
            throw std::logic_error(oss.str());
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
        auto dom_it = ctx_->add_base(Input::signature(lit.term()).value());
        if (has_projection) {
            auto [p_term, state] = ctx_->add_project(term, *dom_it.value());
            cb_(std::make_unique<Ground::LitProject>(*state, lit.sign(), std::move(term), std::move(p_term), idx,
                                                     ctx_->gcomp().domain()));
        } else {
            cb_(std::make_unique<Ground::LitSymbolic>(*dom_it.value(), lit.sign(), std::move(term), idx,
                                                      ctx_->gcomp().domain()));
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

} // namespace Gringo::Control
