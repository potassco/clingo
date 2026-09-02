#pragma once

#include <clingo/input/statement.hh>

namespace CppClingo::Input {

//! Lower sort literals in a statement to ordinary auxiliary rules.
[[nodiscard]] auto lower_sort(SymbolStore &store, size_t &aux_predicate_id, Stm const &stm) -> std::optional<StmVec>;

} // namespace CppClingo::Input
