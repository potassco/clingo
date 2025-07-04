#include <clingo/input/program.hh>
#include <clingo/input/rewrite.hh>

#include <clingo/input/print.hh>
#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/dependency.hh>
#include <clingo/input/rewrite/evaluate.hh>
#include <clingo/input/rewrite/rewrite_theory.hh>
#include <clingo/input/rewrite/substitute.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/checked_math.hh>
#include <clingo/util/type_traits.hh>

#include <iostream>

namespace CppClingo::Input {

void UnprocessedProgram::mark(SymbolCollector &gc) const {
    for (auto const &[part, stms, srcs, facts] : parts_) {
        for (auto const &sym : facts) {
            gc.mark(sym);
        }
    }
}

void UnprocessedProgram::join(UnprocessedProgram const &other) {
    meta_stms_.insert(meta_stms_.end(), other.meta_stms_.begin(), other.meta_stms_.end());
    parts_.insert(parts_.end(), other.parts_.begin(), other.parts_.end());
}

void UnprocessedProgram::clear() {
    parts_.clear();
    meta_stms_.clear();
}

auto UnprocessedProgram::empty() const -> bool {
    return parts_.empty() && meta_stms_.empty();
}

void UnprocessedProgram::add(SymbolStore &store, Stm stm) {
    std::visit(
        [&]<class T>(T const &stm) {
            if constexpr (Util::is_among_v<T, StmShowNothing, StmShowSig, StmScript, StmProjectSig, StmDefined,
                                           StmConst, StmTheory, StmParts>) {
                meta_stms_.emplace_back(std::move(stm));
            } else if constexpr (Util::is_among_v<T, StmInclude, StmComment>) {
                // ignore
            } else if constexpr (Util::matches<T, StmProgram>) {
                parts_.emplace_back(stm, StmVec{}, SourceVec{}, SymbolVec{});
            } else {
                if (parts_.empty()) {
                    parts_.emplace_back(StmProgram{location(stm), store.string_ref("base"), StringSpan{}}, StmVec{},
                                        SourceVec{}, SymbolVec{});
                }
                if constexpr (Util::matches<T, StmRule>) {
                    if (auto fact = is_fact(store, stm); fact) {
                        parts_.back().facts.emplace_back(std::move(fact).value());
                        return;
                    }
                }
                parts_.back().stms.emplace_back(std::move(stm));
            }
        },
        stm);
}

void Program::join(Logger &log, SymbolStore &store, UnprocessedProgram const &prg) {
    // process meta statements
    std::vector<StmConst> const_stms;
    for (auto const &stm : prg.meta_stms()) {
        CppClingo::Input::analyze(stm, provide_, depend_);
        std::visit(
            [&, this]<class T>(T const &stm) {
                if constexpr (Util::is_among_v<T, StmTheory>) {
                    thy_stms_.emplace_back(stm);
                } else if constexpr (Util::is_among_v<T, StmConst>) {
                    const_stms.emplace_back(stm);
                } else if constexpr (Util::is_among_v<T, StmScript>) {
                    script_stms_.emplace_back(stm);
                } else if constexpr (Util::is_among_v<T, StmDefined>) {
                    defined_stms_.emplace_back(stm);
                } else if constexpr (Util::is_among_v<T, StmParts>) {
                    if (!default_parts_ || default_parts_->type() < stm.type()) {
                        default_parts_ = stm;
                    } else if (default_parts_ && default_parts_->type() == stm.type()) {
                        CLINGO_REPORT_LOC(log, error, default_parts_->loc())
                            << "multiple parts directives with the same precedence: " << stm;
                        throw std::runtime_error("joining failed");
                    }
                } else {
                    meta_stms_.emplace_back(stm);
                }
            },
            stm);
    }
    evaluate_const(log, store, const_stms, const_map_);

    // setup rewrite context
    auto parser = TheoryAtomParser{};
    for (auto const &stm : thy_stms_) {
        parser.add_theory(log, stm);
    }
    auto param_map = ParamMap{};
    auto ctx = RewriteContext{log, store, opts_, parser, param_map, const_map_};

    // process program parts
    for (auto const &[program_stm, stms, srcs, facts] : prg.parts()) {
        auto part = parts_.try_emplace(Signature{program_stm.name(), program_stm.args().size()}, program_stm);
        param_map.clear();
        std::for_each(program_stm.args().begin(), program_stm.args().end(),
                      [&param_map](auto const &x) { param_map.emplace(x); });
        auto &res_part = part.first.value();

        // process facts
        for (auto const &fact : facts) {
            provide_.emplace(fact.name(), fact.args().size(), fact.has_classical_sign());
            std::visit(
                [&part]<class T>(T &&x) {
                    if constexpr (Util::matches<T, Symbol>) {
                        part.first.value().facts.emplace_back(x);
                    }
                    if constexpr (Util::matches<T, Stm>) {
                        part.first.value().stms.emplace_back(std::forward<T>(x));
                    }
                },
                map_params(ctx, res_part.part.loc(), fact));
        }

        auto dst = StmVec{};
        // process rules
        for (auto const &stm : stms) {
            auto src = SourceStm{};
            dst.clear();
            rewrite(ctx, stm, dst);
            for (auto &&rew : dst) {
                CppClingo::Input::analyze(rew, provide_, depend_);
                if (auto fact = is_fact(store, rew); fact) {
                    res_part.facts.emplace_back(fact.value());
                } else {
                    if (opts_.track_sources && rew != stm) {
                        if (!src.has_value()) {
                            src = stm;
                        }
                        res_part.srcs.resize(res_part.stms.size());
                        res_part.srcs.emplace_back(src);
                    }
                    res_part.stms.emplace_back(std::move(rew));
                }
            }
        }
    }
    if (ctx.has_error() || parser.has_error()) {
        throw rewrite_error();
    }
}

[[nodiscard]] auto Program::param_map_(SymbolStore &store, ProgramPart const &part) -> ParamUnmap {
    ParamUnmap res;
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

void Program::check(Logger &log) {
    for (auto it = depend_.begin() + static_cast<std::ptrdiff_t>(depend_offset_), ie = depend_.end(); it != ie; ++it) {
        if (!provide_.contains(it.key())) {
            auto const &[name, arity, sign] = it.key();
            CLINGO_REPORT_LOC(log, info_atom_undefined, it.value())
                << "undefined predicate " << (sign ? "-" : "") << *name << "/" << arity;
        }
    }
    depend_offset_ = depend_.size();
}

auto Program::theory_directives() const -> TheorySigVec {
    auto res = TheorySigVec{};
    for (auto const &thy : thy_stms_) {
        for (auto const &def : thy.atom_defs()) {
            if (def.type() == Input::TheoryAtomType::directive) {
                res.emplace_back(def.name(), def.arity());
            }
        }
    }
    std::ranges::sort(res);
    res.erase(std::ranges::unique(res).begin(), res.end());
    res.shrink_to_fit();
    return res;
}

auto Program::analyze(SymbolStore &store, ProgramParamVec const &params, DependencyBuilder &bld) const -> bool {
    bld.meta(meta_stms_);
    std::vector<Stm> stms;
    // TODO: need to map the sources here
    Util::unordered_set<Signature> sigs;
    Util::unordered_set<std::reference_wrapper<ProgramParam const>> seen;
    seen.reserve(params.size());
    sigs.reserve(params.size());
    for (auto const &param : params) {
        if (!seen.emplace(param).second) {
            continue;
        }
        auto [sig_it, sig_ins] = sigs.emplace(Signature{*param.first, param.second.size()});
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
                    name.reserve(std::strlen(prefix) + sig_it->first->size());
                    name += prefix;
                    name += sig_it->first->c_str();
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
    return bld.components(CppClingo::Input::analyze(store, stms));
}

void Program::mark(SymbolCollector &gc) const {
    for (auto const &[sig, part] : parts_) {
        for (auto const &sym : part.facts) {
            gc.mark(sym);
        }
    }
}

void Program::mark_sig(Input::Sig const &sig) {
    if (provide_.empty() || provide_.back() != sig) {
        provide_.emplace(sig);
    }
}

} // namespace CppClingo::Input
