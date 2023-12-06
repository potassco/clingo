#include <util/algorithm.hh>

#include <input/program.hh>

#include <input/algo/evaluate.hh>

#define ISINST GRINGO_IS_INSTANCE

// TODO: remove
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

auto Program::update(Logger &log, SymbolStore &store, UnprocessedProgram prg) -> bool {
    static_cast<void>(level_);
    // TODO: add parts and apply constants
    auto map = evaluate_const(log, store, prg.const_stms_);
    std::cerr << "size: " << map.size() << std::endl;

    return true;
}

#undef ISINST

} // namespace Gringo::Input
