#pragma once

#include <ostream>

namespace Gringo::Util {

//! @addtogroup util_print
//! @{

//! Helper to print an object.
struct p_self {
    //! Print the given object.
    void operator()(std::ostream &out, auto const &x) { out << x; }
};

//! Helper to inject a function to print something.
template <class F> class p_fun {
  public:
    //! Construct the helper.
    p_fun(F fun) : fun_{std::move(fun)} {}
    //! Call the function while outputting.
    friend auto operator<<(std::ostream &out, p_fun const &x) -> std::ostream & {
        x.fun_(out);
        return out;
    }

  private:
    F fun_;
};

//! Wrapper for range with a separator and a mapper for printing.
template <class T, class F = p_self> struct p_range : private F {
    //! Print the range with the given separator.
    p_range(T const &rng, char const *sep, F fun = F{}) : F{std::move(fun)}, rng_{&rng}, sep_{sep} {}
    //! Print the range with a comma separator.
    p_range(T const &rng, F fun = F{}) : F{std::move(fun)}, rng_{&rng}, sep_{","} {}
    //! Output the range.
    friend auto operator<<(std::ostream &out, p_range rng) -> std::ostream & {
        bool comma = false;
        for (auto &elem : *rng.rng_) {
            if (comma) {
                out << rng.sep_;
            }
            comma = true;
            static_cast<F &>(rng)(out, elem);
        }
        return out;
    }

  private:
    T const *rng_;
    char const *sep_;
};

//! Print a string in quotes escaping special characters.
inline void print_quoted(std::ostream &out, std::string_view str) {
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
