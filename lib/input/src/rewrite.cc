#include <clingo/input/print.hh>
#include <clingo/input/rewrite.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/compute_bounds.hh>
#include <clingo/input/rewrite/project.hh>
#include <clingo/input/rewrite/rewrite_anonymous.hh>
#include <clingo/input/rewrite/rewrite_theory.hh>
#include <clingo/input/rewrite/safety.hh>
#include <clingo/input/rewrite/simplify.hh>
#include <clingo/input/rewrite/substitute.hh>
#include <clingo/input/rewrite/unpool.hh>
#include <clingo/input/rewrite/unpool_relations.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/core/logger.hh>

namespace CppClingo::Input {

void rewrite(RewriteContext &ctx, Stm const &stm, StmVec &stms) {
    ctx.init(select_variables(stm, VariableContext::all), "__A_");
    CLINGO_REPORT(ctx.logger(), debug) << "rewrite: " << stm;
    auto opt = rewrite_anonymous(ctx.store(), stm);
    if (opt) {
        CLINGO_REPORT(ctx.logger(), debug) << "  anonymous: " << *opt;
    }
    auto res = std::move(opt).value_or(stm);

    auto rewrite_unpooled = [&stms, &ctx](Stm stm, char const *indent) {
        auto rewrite_unpooled = [&stms, &ctx, indent](Stm stm, char const *sub_indent) {
            auto [state_sub, res_sub] = substitute(ctx, stm);
            if (res_sub) {
                CLINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "substitute assignments: " << *res_sub;
                stm = *std::move(res_sub);
            }
            if (state_sub != TruthValue::top) {
                auto [state_cb, res_cb] = compute_bounds(ctx, stm);
                if (res_cb) {
                    CLINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "compute bounds: " << *res_cb;
                }
                if (state_cb) {
                    stm = std::move(res_cb).value_or(std::move(stm));
                    if (auto [state_cs, res_cs] = check_safety(ctx.logger(), stm); state_cs) {
                        if (res_cs) {
                            CLINGO_REPORT(ctx.logger(), debug) << indent << sub_indent << "check safety: " << *res_cs;
                        }
                        stm = std::move(res_cs).value_or(std::move(stm));
                        auto res_thy = rewrite_theory(ctx, stm);
                        if (res_thy) {
                            CLINGO_REPORT(ctx.logger(), debug) << indent << "theory: " << *res_thy;
                        }
                        stm = std::move(res_thy).value_or(std::move(stm));
                        stms.emplace_back(std::move(stm));
                    } else {
                        ctx.set_error();
                    }
                }
            }
        };

        auto res_project = project(ctx.options(), stm);
        if (res_project) {
            CLINGO_REPORT(ctx.logger(), debug) << indent << "project: " << *res_project;
        }
        stm = std::move(res_project).value_or(std::move(stm));
        auto res_subst = map_params(ctx, stm);
        if (res_subst) {
            CLINGO_REPORT(ctx.logger(), debug) << indent << "substitute parameters: " << *res_subst;
        }
        stm = std::move(res_subst).value_or(std::move(stm));
        auto [state_stm, res_stm] = simplify(ctx, stm);
        if (res_stm) {
            CLINGO_REPORT(ctx.logger(), debug) << indent << "simplify: " << *res_stm;
        }
        stm = std::move(res_stm).value_or(std::move(stm));
        if (state_stm != TruthValue::top) {
            if (auto res_stms = unpool_relations(ctx, stm); res_stms) {
                for (auto &stm : *res_stms) {
                    CLINGO_REPORT(ctx.logger(), debug) << indent << "unpool relations: " << stm;
                    rewrite_unpooled(std::move(stm), "  ");
                }
            } else {
                rewrite_unpooled(std::move(stm), "");
            }
        }
    };

    if (auto unpooled = unpool(ctx, res); unpooled) {
        for (auto &stm : *unpooled) {
            CLINGO_REPORT(ctx.logger(), debug) << "  unpool: " << stm;
            rewrite_unpooled(std::move(stm), "    ");
        }
    } else {
        rewrite_unpooled(std::move(res), "  ");
    }
    if (ctx.clear_error()) {
        throw rewrite_error();
    }
}

} // namespace CppClingo::Input
