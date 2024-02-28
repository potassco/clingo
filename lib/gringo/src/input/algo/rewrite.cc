#include <gringo/input/algo/compute_bounds.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/project.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/rewrite_anonymous.hh>
#include <gringo/input/algo/safety.hh>
#include <gringo/input/algo/simplify.hh>
#include <gringo/input/algo/substitute.hh>
#include <gringo/input/algo/unpool.hh>
#include <gringo/input/algo/unpool_relations.hh>
#include <gringo/input/algo/visit_variables.hh>

#include <gringo/logger.hh>

namespace Gringo::Input {

void rewrite(Logger &log, SymbolStore &store, ParamMap &param_map, ConstMap &const_map, Stm const &stm,
             RewriteOptions opts, StmVec &stms) {
    RewriteContext ctx{log, store, param_map, const_map, select_variables(stm, VariableContext::all), "__A_"};
    GRINGO_REPORT(log, debug) << "rewrite: " << stm;
    auto opt = rewrite_anonymous(store, stm);
    if (opt) {
        GRINGO_REPORT(log, debug) << "  anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);

    auto rewrite_unpooled = [&opts, &stms, &ctx](Stm stm, char const *indent) {
        auto rewrite_unpooled = [&stms, &ctx, indent](Stm stm, char const *sub_indent) {
            auto [state_cb, res_cb] = compute_bounds(ctx, stm);
            if (res_cb) {
                GRINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "compute bounds: " << *res_cb;
            }
            if (state_cb) {
                stm = std::move(res_cb).value_or(std::move(stm));
                auto [state_cs, res_cs] = check_safety(ctx.logger(), stm);
                if (res_cs) {
                    GRINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "check safety: " << *res_cs;
                }
                if (state_cs) {
                    stm = std::move(res_cs).value_or(std::move(stm));
                    stms.emplace_back(std::move(stm));
                }
            }
        };

        auto res_project = project(stm, opts.project_mode, opts.project_anonymous);
        if (res_project) {
            GRINGO_REPORT(ctx.logger(), debug) << indent << "project: " << *res_project;
        }
        stm = std::move(res_project).value_or(std::move(stm));
        auto res_subst = map_params(ctx, stm);
        if (res_subst) {
            GRINGO_REPORT(ctx.logger(), debug) << indent << "substitute: " << *res_subst;
        }
        stm = std::move(res_subst).value_or(std::move(stm));
        auto [state_stm, res_stm] = simplify(ctx, stm);
        if (res_stm) {
            GRINGO_REPORT(ctx.logger(), debug) << indent << "simplify: " << *res_stm;
        }
        stm = std::move(res_stm).value_or(std::move(stm));
        if (state_stm != TruthValue::top) {
            if (auto res_stms = unpool_relations(ctx, stm); res_stms) {
                for (auto &stm : *res_stms) {
                    GRINGO_REPORT(ctx.logger(), debug) << indent << "unpool relations: " << stm;
                    rewrite_unpooled(std::move(stm), "  ");
                }
            } else {
                rewrite_unpooled(std::move(stm), "");
            }
        }
    };

    if (auto unpooled = unpool(ctx, res); unpooled) {
        for (auto &stm : *unpooled) {
            GRINGO_REPORT(log, debug) << "  unpool: " << stm;
            rewrite_unpooled(std::move(stm), "    ");
        }
    } else {
        return rewrite_unpooled(std::move(res), "  ");
    }
}

} // namespace Gringo::Input
