#include <string>

namespace CppClingo::Util {

//! @addtogroup util_debug
//! @{

//! Replace all occurrences of `from` by `to` in `str`.
inline auto replace_all(std::string str, std::string_view from, const std::string_view to) -> std::string {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

//! Replaces complex types used in this project by shorter aliases.
inline auto type_short(std::string sig) -> std::string {
    sig = replace_all(std::move(sig),
                      "std::variant<CppClingo::Input::LiteralBoolean, CppClingo::Input::LiteralRelation, "
                      "CppClingo::Input::LiteralSymbolic>",
                      "CppClingo::Input::Literal");
    return sig;
}

//! Return a nice string representation of T.
template <class T> constexpr auto type_name() -> std::string_view {
    // NOLINTBEGIN(readability-magic-numbers)
    using namespace std;
#ifdef __clang__
    string_view p = __PRETTY_FUNCTION__;
    return {p.data() + 34, p.size() - 34 - 1};
#elif defined(__GNUC__)
    string_view p = __PRETTY_FUNCTION__;
#if __cplusplus < 201402
    return {p.data() + 36, p.size() - 36 - 1};
#else
    return {p.data() + 49, p.find(';', 49) - 49};
#endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return {p.data() + 84, p.size() - 84 - 7};
#endif
    // NOLINTEND(readability-magic-numbers)
}

//! @}

} // namespace CppClingo::Util
