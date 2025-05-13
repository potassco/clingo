#pragma once

#include <clingo/ground/term.hh>
#include <clingo/ground/theory_term.hh>

#include <clingo/input/term.hh>
#include <clingo/input/theory.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Translates input theory terms to their ground representation.
auto build_term(Util::unordered_map<String, size_t> const &var_map, Input::Term const &term, bool &has_projection)
    -> Ground::UTerm;

//! Translates input theory terms to their ground representation.
inline auto build_term(Util::unordered_map<String, size_t> const &var_map, Input::Term const &term) -> Ground::UTerm {
    bool has_projection = false;
    auto res = build_term(var_map, term, has_projection);
    assert(!has_projection);
    return res;
}

//! Translates input theory terms to their ground representation.
auto build_theory_term(Util::unordered_map<String, size_t> const &var_map, Input::TheoryTerm const &term)
    -> Ground::UTheoryTerm;

//! @}

} // namespace CppClingo::Control
