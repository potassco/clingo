#include <input/algo/project.hh>
#include <input/algo/rewrite.hh>
#include <input/algo/rewrite_anonymous.hh>
#include <input/algo/unpool.hh>

namespace Gringo::Input {

void rewrite(SymbolStore &store, Statement const &stm, RewriteOptions opts, StatementVec &stms) {
    if (opts.level < RewriteLevel::rewrite_anonymous) {
        stms.emplace_back(std::move(stm));
        return;
    }
    auto res = rewrite_anonymous(store, stm).value_or(stm);
    if (opts.level < RewriteLevel::unpool) {
        stms.emplace_back(std::move(res));
        return;
    }

    auto rewrite_unpooled = [&opts, &stms](Statement stm) {
        if (opts.level < RewriteLevel::project) {
            stms.emplace_back(std::move(stm));
            return;
        }
        stm = project(stm, opts.project_mode, opts.project_anonymous).value_or(stm);
        stms.emplace_back(std::move(stm));
    };
    auto unpooled = unpool(res);
    if (unpooled.has_value()) {
        for (auto &stm : unpooled.value()) {
            rewrite_unpooled(std::move(stm));
        }
    } else {
        rewrite_unpooled(std::move(res));
    }
}

} // namespace Gringo::Input
