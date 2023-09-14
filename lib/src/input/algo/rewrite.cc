#include <algorithm>

#include <logger.hh>

#include <input/algo/print.hh>
#include <input/algo/project.hh>
#include <input/algo/rewrite.hh>
#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/unpool.hh>

namespace Gringo::Input {

void rewrite(Logger &log, SymbolStore &store, Statement const &stm, RewriteOptions opts, StatementVec &stms) {
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

    auto rewrite_unpooled = [&opts, &stms, &log](Statement stm) {
        if (opts.level < RewriteLevel::project) {
            stms.emplace_back(std::move(stm));
            return;
        }
        auto opt = project(stm, opts.project_mode, opts.project_anonymous);
        if (opt.has_value()) {
            GRINGO_REPORT(log, trace) << "project anonymous: " << *opt;
        }
        stm = std::move(opt).value_or(stm);
        stms.emplace_back(std::move(stm));
    };
    auto unpooled = unpool(log, res);
    if (unpooled.has_value()) {
        for (auto &stm : unpooled.value()) {
            GRINGO_REPORT(log, trace) << "unpool: " << stm;
            rewrite_unpooled(std::move(stm));
        }
    } else {
        rewrite_unpooled(std::move(res));
    }
}

} // namespace Gringo::Input
