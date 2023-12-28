#include <logger.hh>

#include <input/algo/compute_bounds.hh>
#include <input/algo/print.hh>
#include <input/algo/project.hh>
#include <input/algo/rewrite.hh>
#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/simplify.hh>
#include <input/algo/substitute.hh>
#include <input/algo/unpool.hh>
#include <input/algo/unpool_relations.hh>
#include <input/algo/visit_variables.hh>

namespace Gringo::Input {

/*
whole process as in gringo atm
1. apply #const statements (done)
2. unpool (done)
3. init theory (todo)
4. simplify (done)
6. unpool comparison (done)
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
  5. assignment aggregates (todo; let's see)
*/

void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Statement const &stm,
             RewriteOptions opts, StatementVec &stms) {
    RewriteContext ctx{log, store, param_map, const_map, select_variables(stm, VariableContext::all), "__A_"};
    GRINGO_REPORT(log, trace) << "rewrite: " << stm;
    auto opt = rewrite_anonymous(store, stm);
    if (opt) {
        GRINGO_REPORT(log, trace) << "  anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);

    auto rewrite_unpooled = [&opts, &stms, &ctx](Statement stm, char const *indent) {
        auto rewrite_unpooled = [&stms, &ctx, indent](Statement stm, char const *sub_indent) {
            auto [state_cb, res_cb] = compute_bounds(ctx, stm);
            if (res_cb) {
                GRINGO_REPORT(ctx.logger(), trace) << indent << sub_indent << "compute bounds: " << *res_cb;
            }
            if (state_cb) {
                stm = std::move(res_cb).value_or(std::move(stm));
                stms.emplace_back(std::move(stm));
            }
        };

        auto res_project = project(stm, opts.project_mode, opts.project_anonymous);
        if (res_project) {
            GRINGO_REPORT(ctx.logger(), trace) << indent << "project: " << *res_project;
        }
        stm = std::move(res_project).value_or(std::move(stm));
        auto res_subst = map_params(ctx, stm);
        if (res_subst) {
            GRINGO_REPORT(ctx.logger(), trace) << indent << "substitute: " << *res_subst;
        }
        stm = std::move(res_subst).value_or(std::move(stm));
        auto [state_stm, res_stm] = simplify(ctx, stm);
        if (res_stm) {
            GRINGO_REPORT(ctx.logger(), trace) << indent << "simplify: " << *res_stm;
        }
        stm = std::move(res_stm).value_or(std::move(stm));
        if (state_stm != TruthValue::top) {
            if (auto res_stms = unpool_relations(ctx, stm); res_stms) {
                for (auto &stm : *res_stms) {
                    GRINGO_REPORT(ctx.logger(), trace) << indent << "unpool relations: " << stm;
                    rewrite_unpooled(std::move(stm), "  ");
                }
            } else {
                rewrite_unpooled(std::move(stm), "");
            }
        }
    };

    if (auto unpooled = unpool(ctx, res); unpooled) {
        for (auto &stm : *unpooled) {
            GRINGO_REPORT(log, trace) << "  unpool: " << stm;
            rewrite_unpooled(std::move(stm), "    ");
        }
    } else {
        return rewrite_unpooled(std::move(res), "  ");
    }
}

} // namespace Gringo::Input
