#include <gringo/input/program.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/evaluate.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/rewrite_theory.hh>
#include <gringo/input/algo/substitute.hh>

#include <gringo/util/algorithm.hh>
#include <gringo/util/type_traits.hh>

namespace Gringo::Input {

void add(SymbolStore &store, Stm stm, UnprocessedProgram &prg) {
    std::visit(
        [&]<class T>(T const &stm) {
            if constexpr (Util::is_among_v<T, StmShowSig, StmProjectSig, StmScript, StmDefined>) {
                prg.meta_stms.emplace_back(std::move(stm));
            } else if constexpr (Util::is_among_v<T, StmInclude, StmComment>) {
                // ignore
            } else if constexpr (std::is_same_v<T, StmTheory>) {
                prg.thy_stms.emplace_back(std::move(stm));
            } else if constexpr (std::is_same_v<T, StmConst>) {
                prg.const_stms.emplace_back(std::move(stm));
            } else if constexpr (std::is_same_v<T, StmProgram>) {
                prg.parts.emplace_back(stm, StmVec{}, SymbolVec{});
            } else {
                if (prg.parts.empty()) {
                    prg.parts.emplace_back(StmProgram{location(stm), store.string("base"), {}}, StmVec{}, SymbolVec{});
                }
                if constexpr (std::is_same_v<T, StmRule>) {
                    if (auto fact = is_fact(store, stm); fact) {
                        std::get<2>(prg.parts.back()).emplace_back(std::move(fact).value());
                        return;
                    }
                }
                std::get<1>(prg.parts.back()).emplace_back(std::move(stm));
            }
        },
        stm);
}

void Program::join(Logger &log, SymbolStore &store, UnprocessedProgram prg) {
    // setup rewrite context
    thy_stms_.insert(thy_stms_.end(), prg.thy_stms.begin(), prg.thy_stms.end());
    auto parser = TheoryAtomParser{};
    for (auto const &stm : thy_stms_) {
        parser.add_theory(log, stm);
    }
    auto param_map = ParamMap{};
    auto ctx = RewriteContext{log, store, opts_, parser, param_map, const_map_};

    // process meta statements
    evaluate_const(log, store, prg.const_stms, const_map_);
    for (auto &stm : prg.meta_stms) {
        rewrite(ctx, stm, meta_stms_);
    }

    // process program parts
    for (auto &[program_stm, stms, facts] : prg.parts) {
        auto part = parts_.try_emplace(Signature{program_stm.name(), program_stm.args().size()}, program_stm);
        param_map.clear();
        param_map.insert(program_stm.args().begin(), program_stm.args().end());
        auto &res_part = part.first.value();

        // process facts
        for (auto &fact : facts) {
            std::visit(
                [&part]<class T>(T &&x) {
                    if constexpr (std::is_same_v<T, Symbol>) {
                        part.first.value().facts.emplace_back(x);
                    }
                    if constexpr (std::is_same_v<T, Stm>) {
                        part.first.value().stms.emplace_back(std::move(x));
                    }
                },
                map_params(ctx, res_part.part.loc(), fact));
        }

        // process rules
        for (auto &stm : stms) {
            size_t n = res_part.stms.size();
            rewrite(ctx, stm, res_part.stms);
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

[[nodiscard]] auto Program::param_map_(SymbolStore &store, ProgramPart const &part)
    -> Util::ordered_map<String, String> {
    Util::ordered_map<String, String> res;
    if (!part.part.args().empty()) {
        StringSet ids;
        for (auto const &facts : part.facts) {
            collect_ids(facts, ids);
        }
        for (auto const &stm : part.stms) {
            collect_ids(stm, ids);
        }
        auto gen = NameGen{store, std::move(ids), "__p_"};
        size_t i = 0;
        for (auto const &id : part.part.args()) {
            auto var = store.string("$" + std::to_string(i));
            res.emplace(var, gen.add_name(id) ? id : gen.new_name());
            ++i;
        }
    }
    return res;
}

[[nodiscard]] auto Program::unmap_(SymbolStore &store, ParamUnmap const &pum, Stm const &stm) -> std::optional<Stm> {
    if (!pum.empty()) {
        return unmap_params(store, pum, stm);
    }
    return std::nullopt;
}

} // namespace Gringo::Input
