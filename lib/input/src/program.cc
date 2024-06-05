#include <gringo/input/program.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/dependency.hh>
#include <gringo/input/algo/evaluate.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/rewrite_theory.hh>
#include <gringo/input/algo/substitute.hh>

#include <gringo/util/algorithm.hh>
#include <gringo/util/checked_math.hh>
#include <gringo/util/type_traits.hh>

namespace Gringo::Input {

void UnprocessedProgram::clear() {
    parts.clear();
    const_stms.clear();
    thy_stms.clear();
    meta_stms.clear();
}

void UnprocessedProgram::add(SymbolStore &store, Stm stm) {
    std::visit(
        [&]<class T>(T const &stm) {
            if constexpr (Util::is_among_v<T, StmShowSig, StmProjectSig, StmScript, StmDefined>) {
                meta_stms.emplace_back(std::move(stm));
            } else if constexpr (Util::is_among_v<T, StmInclude, StmComment>) {
                // ignore
            } else if constexpr (Util::matches<T, StmTheory>) {
                thy_stms.emplace_back(std::move(stm));
            } else if constexpr (Util::matches<T, StmConst>) {
                const_stms.emplace_back(std::move(stm));
            } else if constexpr (Util::matches<T, StmProgram>) {
                ensure_base = false;
                parts.emplace_back(stm, StmVec{}, SymbolRefVec{});
            } else {
                if (parts.empty() || ensure_base) {
                    if (parts.empty() || std::get<0>(parts.back()).name() != "base" ||
                        !std::get<0>(parts.back()).args().empty()) {
                        parts.emplace_back(StmProgram{location(stm), store.string_ref("base"), {}}, StmVec{},
                                           SymbolRefVec{});
                    }
                    ensure_base = false;
                }
                if constexpr (Util::matches<T, StmRule>) {
                    if (auto fact = is_fact(store, stm); fact) {
                        std::get<2>(parts.back()).emplace_back(std::move(fact).value());
                        return;
                    }
                }
                std::get<1>(parts.back()).emplace_back(std::move(stm));
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
                    if constexpr (Util::matches<T, SymbolRef>) {
                        part.first.value().facts.emplace_back(x);
                    }
                    if constexpr (Util::matches<T, Stm>) {
                        part.first.value().stms.emplace_back(std::forward<T>(x));
                    }
                },
                map_params(ctx, res_part.part.loc(), fact));
        }

        // process rules
        for (auto &stm : stms) {
            auto n = std::ssize(res_part.stms);
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

[[nodiscard]] auto Program::param_map_(SymbolStore &store,
                                       ProgramPart const &part) -> Util::ordered_map<StringRef, StringRef> {
    Util::ordered_map<StringRef, StringRef> res;
    if (!part.part.args().empty()) {
        StringRefSet ids;
        for (auto const &facts : part.facts) {
            collect_ids(facts, ids);
        }
        for (auto const &stm : part.stms) {
            collect_ids(stm, ids);
        }
        auto gen = NameGen{store, std::move(ids), "__p_"};
        size_t i = 0;
        for (auto const &id : part.part.args()) {
            auto var = store.string_ref("$" + std::to_string(i));
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

auto Program::analyze(SymbolStore &store, ProgramParamVec const &params, DependencyBuilder &bld) const -> bool {
    bld.meta(meta_stms_);
    std::vector<Stm> stms;
    Util::unordered_set<Signature> sigs;
    Util::unordered_set<std::reference_wrapper<ProgramParam const>> seen;
    seen.reserve(params.size());
    sigs.reserve(params.size());
    for (auto const &param : params) {
        if (!seen.emplace(param).second) {
            continue;
        }
        auto [sig_it, sig_ins] = sigs.emplace(Signature{param.first, param.second.size()});
        if (auto it = parts_.find(*sig_it); it != parts_.end()) {
            // note that facts are not subject to parameters
            bld.fact(it->second.facts);
            if (it->first.second == 0) {
                stms.insert(stms.end(), it->second.stms.begin(), it->second.stms.end());
            } else {
                bld.param(param);
                if (sig_ins) {
                    auto loc = it->second.part.loc();
                    std::vector<Argument> args;
                    args.reserve(sig_it->second);
                    for (size_t i = 0; i < sig_it->second; ++i) {
                        args.emplace_back(TermVariable{loc, store.string_ref("$" + std::to_string(i))});
                    }
                    auto name = std::string{};
                    auto const *prefix = "#program_";
                    name.reserve(std::strlen(prefix) + sig_it->first.size());
                    name += prefix;
                    name += sig_it->first.c_str();
                    auto fun =
                        TermFunction{loc, store.string_ref(name),
                                     Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{std::move(args)}), false};
                    auto lit = BdLit{BdLitSimple{LitSymbolic{loc, Sign::none, std::move(fun)}}};
                    for (auto const &stm : it->second.stms) {
                        std::visit(
                            [&]<class T>(T const &stm) {
                                if constexpr (requires(T const &x) { x.body(); }) {
                                    std::vector<BdLit> body;
                                    body.reserve(stm.body().size() + 1);
                                    body.emplace_back(lit);
                                    body.insert(body.end(), stm.body().begin(), stm.body().end());
                                    stms.emplace_back(stm.update(a_body = std::move(body)));
                                } else {
                                    throw std::runtime_error("unexpected statement in analyze");
                                }
                            },
                            stm);
                    }
                }
            }
        }
    }
    return bld.components(Gringo::Input::analyze(store, stms));
}

} // namespace Gringo::Input
