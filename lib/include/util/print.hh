#pragma once

#include <ostream>

namespace Gringo::Util {

//! @addtogroup util
//! @{

//! An identity mapper used for printing.
struct p_map {
    //! Map the given object to itself.
    auto operator()(auto const &x) -> auto const & { return x; }
};

//! Wrapper for range with a separator and a mapper for printing.
template <class T, class M = p_map> struct p_range : private M {
    //! Print the range with the given separator.
    p_range(T const &rng, char const *sep, M map = M{}) : M{std::move(map)}, rng_{rng}, sep_{sep} {}
    //! Print the range with a comma separator.
    p_range(T const &rng, M map = M{}) : M{std::move(map)}, rng_{rng}, sep_{","} {}
    //! Output the range.
    friend auto operator<<(std::ostream &out, p_range rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : rng.rng_) {
            if (comma) {
                out << rng.sep_;
            }
            comma = true;
            out << static_cast<M &>(rng)(elem);
        }
        return out;
    }

  private:
    T const &rng_;
    char const *sep_;
};

//! Print a string in quotes escaping special characters.
inline void print_quoted(std::ostream &out, std::string const &str) {
    // TODO: in principle there are the codepoints too...
    out << '"';
    for (auto c : str) {
        if (c == '\\') {
            out << "\\\\";
        } else if (c == '\n') {
            out << "\\n";
        } else if (c == '\t') {
            out << "\\t";
        } else {
            out << c;
        }
    }
    out << '"';
}

//! @}

} // namespace Gringo::Util
