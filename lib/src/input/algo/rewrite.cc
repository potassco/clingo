#include <algorithm>

#include <logger.hh>

#include <input/algo/print.hh>
#include <input/algo/project.hh>
#include <input/algo/rewrite.hh>
#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/simplify.hh>
#include <input/algo/substitute.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

namespace Gringo::Input {

/*
whole process as in gringo atm
1. apply #const statements (partially done)
2. unpool (done)
3. init theory
4. simplify (done for terms, literals)
  0. evaluate (done)
  1. extract atoms to project (done)
     - add option to forbid completely
     - only check in simplify
  2. dots/script (done)
     - simply remove them all starting from nested contexts
     - they should be ignored in specific settings to make the simplify function idempotent
       this has to be handled by the surrounding literal class
     (done for terms)
  4. terms that can fail (done)
     - needs option to avoid if unnecessary
     - applies to unary, binary, abs, and external in n-ary comparison literals with n > 2
       (probably the only context)
     - 1+a < 5 < 10
     - X < 5 < 10, X=1+a
  5. make matchable (done)
    - can be part of simplify (per option to avoid if unnecessary for example in negated literals)
    - probably best solved using a separate traversal
    - p(X+5,X*X)
      -> p(X+5,Aux), Aux=X*X
    -> p(X+5,@f(g(X*X))),
      -> p(X+5,Aux), Aux=@f(g(X*X)))
      - no traversal into external functions/intervals
6. unpool comparison
   the comparison
     not 1+a < 5 < 10
   is equivalent to
     X=1+a, not X < 5 < 10
   so any term that can fail to evaluate should be stripped during simplification
   this also includes intervals and scripts!
7. rewrite
  1. aggregates
  2. arithmetics
  4. comparisons to intervals
  5. assignment aggregates
*/

void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Statement const &stm,
             RewriteOptions opts, StatementVec &stms) {
    RewriteContext ctx{log, store, param_map, const_map, select_variables(stm, VariableContext::all), "__A_"};
    GRINGO_REPORT(log, trace) << "rewrite: " << stm;
    if (opts.level < RewriteLevel::rewrite_anonymous) {
        stms.emplace_back(std::move(stm));
        return;
    }
    auto opt = rewrite_anonymous(store, stm);
    if (opt.has_value()) {
        GRINGO_REPORT(log, trace) << "rewrite anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);
    if (opts.level < RewriteLevel::unpool) {
        stms.emplace_back(std::move(res));
        return;
    }

    auto rewrite_unpooled = [&opts, &stms, &ctx](Statement stm) {
        if (opts.level < RewriteLevel::project) {
            stms.emplace_back(std::move(stm));
            return;
        }
        auto res_project = project(stm, opts.project_mode, opts.project_anonymous);
        if (res_project.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "project anonymous: " << *res_project;
        }
        stm = std::move(res_project).value_or(std::move(stm));
        if (opts.level < RewriteLevel::simplify) {
            stms.emplace_back(std::move(stm));
            return;
        }
        GRINGO_REPORT(ctx.logger(), trace) << "has params: " << ctx.has_params();
        auto res_subst = substitute(ctx, stm);
        if (res_subst.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "substitute params: " << *res_subst;
        }
        stm = std::move(res_subst).value_or(std::move(stm));
        auto [state_stm, res_stm] = simplify(ctx, stm);
        if (res_stm.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "simplify: " << *res_stm;
        }
        stm = std::move(res_stm).value_or(std::move(stm));
        if (state_stm != TruthValue::top) {
            stms.emplace_back(std::move(stm));
        }
        return;
    };
    auto unpooled = unpool(ctx, res);
    if (unpooled.has_value()) {
        for (auto &stm : unpooled.value()) {
            GRINGO_REPORT(log, trace) << "unpool: " << stm;
            rewrite_unpooled(std::move(stm));
        }
        return;
    }
    return rewrite_unpooled(std::move(res));
}

} // namespace Gringo::Input
