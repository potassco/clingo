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
1. apply #const statements (done)
2. unpool (done)
3. init theory (todo)
4. simplify (done)
6. unpool comparison (todo)
   the comparison
     not 1+a < 5 < 10
   is equivalent to
     X=1+a, not X < 5 < 10
   so any term that can fail to evaluate should be stripped during simplification
   this also includes intervals and scripts!
7. rewrite
  1. aggregates (done)
  2. arithmetics (done)
  4. comparisons to intervals (todo)
  5. assignment aggregates (todo)
*/

void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Statement const &stm,
             RewriteOptions opts, StatementVec &stms) {
    RewriteContext ctx{log, store, param_map, const_map, select_variables(stm, VariableContext::all), "__A_"};
    GRINGO_REPORT(log, trace) << "rewrite: " << stm;
    auto opt = rewrite_anonymous(store, stm);
    if (opt.has_value()) {
        GRINGO_REPORT(log, trace) << "  anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);

    auto rewrite_unpooled = [&opts, &stms, &ctx](Statement stm) {
        auto res_project = project(stm, opts.project_mode, opts.project_anonymous);
        if (res_project.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "    project: " << *res_project;
        }
        stm = std::move(res_project).value_or(std::move(stm));
        auto res_subst = map_params(ctx, stm);
        if (res_subst.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "    substitute: " << *res_subst;
        }
        stm = std::move(res_subst).value_or(std::move(stm));
        auto [state_stm, res_stm] = simplify(ctx, stm);
        if (res_stm.has_value()) {
            GRINGO_REPORT(ctx.logger(), trace) << "    simplify: " << *res_stm;
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
            GRINGO_REPORT(log, trace) << "  unpool: " << stm;
            rewrite_unpooled(std::move(stm));
        }
        return;
    }
    return rewrite_unpooled(std::move(res));
}

} // namespace Gringo::Input
