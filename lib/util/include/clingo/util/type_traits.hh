#pragma once

#include <type_traits>

namespace CppClingo::Util {

//! @addtogroup util_traits
//! @{

//! Check if the type S is among the types in L.
template <class S, class... L> inline constexpr bool is_among_v = (std::is_same_v<S, L> || ...);

//! Check if the type S stripped from cv-qualifiers and references is among the types in L.
template <class S, class... L> inline constexpr bool matches = is_among_v<std::remove_cvref_t<S>, L...>;

//! @}

} // namespace CppClingo::Util
