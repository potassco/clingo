#pragma once

#include <cassert>
#include <charconv>
#include <cstring>
#include <ostream>
#include <span>
#include <vector>

namespace Gringo::Util {

//! @addtogroup util_print
//! @{

class OutputBuffer {
  public:
    [[nodiscard]] auto size() const -> size_t { return static_cast<size_t>(size_); }

    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }

    [[nodiscard]] auto view() const -> std::string_view {
        return std::string_view{buf_.data(), static_cast<size_t>(size_)};
    }

    [[nodiscard]] auto str() const -> std::string { return std::string{buf_.data(), static_cast<size_t>(size_)}; }

    [[nodiscard]] auto c_str() -> char const * {
        *ensure_(1) = '\0';
        return buf_.data();
    }

    void reset() { size_ = 0; }

    void append(char const *str) {
        auto n = static_cast<ssize_t>(std::strlen(str));
        std::copy(str, std::next(str, n), ensure_(n));
        size_ += n;
    }

    void append(std::string_view str) {
        auto n = static_cast<ssize_t>(str.length());
        std::copy(str.begin(), str.end(), ensure_(n));
        size_ += n;
    }

    void append(char c) {
        *ensure_(1) = c;
        ++size_;
    }

    template <std::integral T> void append(T num) {
        constexpr auto n = 128;
        auto *end = ensure_(n);
        auto res = std::to_chars(end, limit_(), num);
        size_ += res.ptr - end;
    }

    auto reserve(ssize_t n) -> std::span<char> {
        auto *begin = ensure_(n);
        size_ += n;
        return {begin, std::next(begin, n)};
    }

    template <std::integral T> friend auto operator<<(OutputBuffer &out, T num) -> OutputBuffer & {
        out.append(num);
        return out;
    }

    friend auto operator<<(OutputBuffer &out, char c) -> OutputBuffer & {
        out.append(c);
        return out;
    }

    friend auto operator<<(OutputBuffer &out, std::string_view str) -> OutputBuffer & {
        out.append(str);
        return out;
    }

    friend auto operator<<(OutputBuffer &out, char const *str) -> OutputBuffer & {
        out.append(str);
        return out;
    }

  private:
    auto limit_() -> char * { return std::next(buf_.data(), static_cast<ssize_t>(buf_.size())); }

    auto ensure_(ssize_t n) -> char * {
        auto m = size_ + n;
        assert(n >= 0 && m >= 0);
        if (buf_.size() < static_cast<size_t>(m)) {
            buf_.reserve(static_cast<size_t>(m));
            buf_.resize(buf_.capacity());
        }
        return std::next(buf_.data(), size_);
    }

    std::vector<char> buf_;
    ssize_t size_ = 0;
};

//! Helper to print an object.
struct p_self {
    //! Print the given object.
    template <class Out> void operator()(Out &out, auto const &x) { out << x; }
};

//! Helper to inject a function to print something.
template <class F> class p_fun {
  public:
    //! Construct the helper.
    p_fun(F fun) : fun_{std::move(fun)} {}
    //! Call the function while outputting.
    template <class Out> friend auto operator<<(Out &out, p_fun const &x) -> std::ostream & {
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
    template <class Out> friend auto operator<<(Out &out, p_range rng) -> Out & {
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
template <class Out> inline void print_quoted(Out &out, std::string_view str) {
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
