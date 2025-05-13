#pragma once

#include <clingo/input/program.hh>

#include <clingo/util/optional.hh>

namespace CppClingo::Input {

//! @addtogroup input_rewrite
//! @{

//! Check whether the given statement is safe.
//!
//! Literals in bodies and conditions are brought into a groundable order.
//! The function returns the rewritten statement if this leads to changes.
[[nodiscard]] auto check_safety(Logger &log, Stm const &stm) -> Util::ResultState<Stm>;

//! @}

} // namespace CppClingo::Input
