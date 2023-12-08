#include <util/algorithm.hh>

#include <input/program.hh>

#include <input/algo/analyze.hh>
#include <input/algo/evaluate.hh>
#include <input/algo/rewrite.hh>
#include <input/algo/substitute.hh>

namespace Gringo::Input {

#define ISINST GRINGO_IS_INSTANCE

void UnprocessedProgram::add(SymbolStore &store, Statement stm) {
    std::visit(
        [&](auto &&stm) {
            if constexpr (ISINST(stm, StatementShowSig) || ISINST(stm, StatementProjectSig) ||
                          ISINST(stm, StatementScript) || ISINST(stm, StatementDefined)) {
                meta_stms_.emplace_back(std::move(stm));
            } else if constexpr (ISINST(stm, StatementInclude) || ISINST(stm, Comment)) {
                // ignore
            } else if constexpr (ISINST(stm, StatementConst)) {
                const_stms_.emplace_back(std::move(stm));
            } else if constexpr (ISINST(stm, StatementProgram)) {
                parts_.emplace_back(stm, StatementVec{}, SymbolVec{});
            } else {
                if (parts_.empty()) {
                    parts_.emplace_back(StatementProgram{location(stm), store.string("base"), {}}, StatementVec{},
                                        SymbolVec{});
                }
                if constexpr (ISINST(stm, Rule)) {
                    if (auto fact = is_fact(store, stm); fact) {
                        std::get<2>(parts_.back()).emplace_back(std::move(fact).value());
                        return;
                    }
                }
                std::get<1>(parts_.back()).emplace_back(std::move(stm));
            }
        },
        stm);
}

#undef ISINST

void Program::join(Logger &log, SymbolStore &store, UnprocessedProgram prg) {
    evaluate_const(log, store, prg.const_stms_, const_map_);

    for (auto &[program_stm, stms, facts] : prg.parts_) {
        auto part = parts_.try_emplace(Signature{program_stm.name, program_stm.args.size()}, program_stm);
        ParamMap param_map;
        param_map.insert(program_stm.args.begin(), program_stm.args.end());
        auto &res_part = part.first.value();

        // process facts
        auto ctx = RewriteContext{log, store, param_map, const_map_, {}, ""};
        for (auto &fact : facts) {
            std::visit(
                [&part](auto &&x) {
                    GRINGO_MATCH(x, Symbol) { part.first.value().facts.emplace_back(x); }
                    GRINGO_MATCH(x, Statement) { part.first.value().stms.emplace_back(std::move(x)); }
                },
                substitute(ctx, res_part.part.loc, fact));
        }

        // process rules
        for (auto &stm : stms) {
            size_t n = res_part.stms.size();
            rewrite(log, store, param_map, const_map_, stm, opts_, res_part.stms);
            auto jt = res_part.stms.begin() + n;
            for (auto it = jt, ie = res_part.stms.end(); it != ie; ++it) {
                if (auto fact = is_fact(store, *it); fact) {
                    res_part.facts.emplace_back(fact.value());
                } else {
                    if (it != jt) {
                        *jt = std::move(*it);
                    }
                    ++jt;
                }
            }
            res_part.stms.erase(jt, res_part.stms.end());
        }
    }
}

[[nodiscard]] auto Program::param_map_(SymbolStore &store, StatementProgram const &part)
    -> Util::ordered_map<String, String> {
    if (!part.args.empty()) {
        static_cast<void>(store);
        throw std::logic_error("implement me!!!");
    }
    return {};
}

[[nodiscard]] auto Program::unmap_(ParamUnmap const &pum, Statement const &stm) -> std::optional<Statement> {
    if (!pum.empty()) {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }
    return std::nullopt;
}

} // namespace Gringo::Input
