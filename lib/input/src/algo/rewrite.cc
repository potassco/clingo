#include <gringo/input/algo/compute_bounds.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/project.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/rewrite_anonymous.hh>
#include <gringo/input/algo/rewrite_theory.hh>
#include <gringo/input/algo/safety.hh>
#include <gringo/input/algo/simplify.hh>
#include <gringo/input/algo/substitute.hh>
#include <gringo/input/algo/unpool.hh>
#include <gringo/input/algo/unpool_relations.hh>
#include <gringo/input/algo/visit_variables.hh>

#include <gringo/core/logger.hh>

namespace Gringo::Input {

void rewrite(RewriteContext &ctx, Stm const &stm, StmVec &stms) {
    ctx.init(select_variables(stm, VariableContext::all), "__A_");
    GRINGO_REPORT(ctx.logger(), debug) << "rewrite: " << stm;
    auto opt = rewrite_anonymous(ctx.store(), stm);
    if (opt) {
        GRINGO_REPORT(ctx.logger(), debug) << "  anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);

    auto rewrite_unpooled = [&stms, &ctx](Stm stm, char const *indent) {
        auto rewrite_unpooled = [&stms, &ctx, indent](Stm stm, char const *sub_indent) {
            auto [state_cb, res_cb] = compute_bounds(ctx, stm);
            if (res_cb) {
                GRINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "compute bounds: " << *res_cb;
            }
            if (state_cb) {
                stm = std::move(res_cb).value_or(std::move(stm));
                if (auto [state_cs, res_cs] = check_safety(ctx.logger(), stm); state_cs) {
                    if (res_cs) {
                        GRINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "check safety: " << *res_cs;
                    }
                    stm = std::move(res_cs).value_or(std::move(stm));
                    auto res_thy = rewrite_theory(ctx, stm);
                    if (res_thy) {
                        GRINGO_REPORT(ctx.logger(), debug) << indent << "theory: " << *res_thy;
                    }
                    stm = std::move(res_thy).value_or(std::move(stm));
                    stms.emplace_back(std::move(stm));
                } else {
                    ctx.set_error();
                }
            }
        };

        auto res_project = project(ctx.options(), stm);
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
            GRINGO_REPORT(ctx.logger(), debug) << "  unpool: " << stm;
            rewrite_unpooled(std::move(stm), "    ");
        }
    } else {
        rewrite_unpooled(std::move(res), "  ");
    }
}

} // namespace Gringo::Input
