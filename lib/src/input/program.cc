#include <util/algorithm.hh>

#include <input/program.hh>

#include <input/algo/evaluate.hh>
#include <input/algo/rewrite.hh>

#define ISINST GRINGO_IS_INSTANCE

// TODO: remove
#include <input/algo/print.hh>
#include <iostream>

namespace Gringo::Input {

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
                    // TODO: handle facts
                }
                std::get<1>(parts_.back()).emplace_back(std::move(stm));
            }
        },
        stm);
}

void Program::join(Logger &log, SymbolStore &store, UnprocessedProgram prg) {
    evaluate_const(log, store, prg.const_stms_, const_map_);

    for (auto &[program_stm, stms, facts] : prg.parts_) {
        auto part = parts_.try_emplace(Signature{program_stm.name, program_stm.args.size()});
        ParamMap param_map;
        param_map.insert(program_stm.args.begin(), program_stm.args.end());
        for (auto &stm : stms) {
            // TODO: protect parameters
            //   a transformer should be able to do the job!
            // TODO: apply constants
            //   a transformer should be able to do the job!
            // NOTE: the two above steps can share the same transformer
            //   the transformer needs a function in charge of the substitution
            //   the function has to be provided
            //   transforming literals or terms representing atoms has to be done with care
            //   because function symbols representing atoms must not be transformed
            //   this should be pleasantly straightforward to implement!
            // TODO: statement might become fact after simplification
            rewrite(log, store, param_map, const_map_, stm, opts_, part.first.value().stms);
        }
        // TODO: same for facts
        // TODO: tedious conversion between facts and rules could be avoided
        //       by replacing bound parameters right away
    }

    // TODO: for debugging
    for (auto const &[id, sym] : const_map_) {
        std::cerr << "#const " << id << "=" << sym.second << "." << std::endl;
    }
    for (auto const &stm : meta_stms_) {
        std::cerr << stm << std::endl;
    }
    for (auto const &[sig, part] : parts_) {
        std::cerr << "#program " << sig.first << "/" << sig.second << "." << std::endl;
        for (auto const &stm : part.stms) {
            std::cerr << stm << std::endl;
        }
    }
    std::cerr << "status: " << (log.has_error() ? "error" : "success") << std::endl;
}

#undef ISINST

} // namespace Gringo::Input
