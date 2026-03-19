#pragma once

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstring>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_print
//! @{

//! Create an output buffer that bears some similarities with C++'s iostreams.
//!
//! The buffer can optionally be constructed with a file handle for output.
//! Function endl() can be called to output and discard the buffer's content at
//! key points; its content will only be written if the buffer has at least
//! some fixed predefined size. Function flush() should be called to output the
//! current buffer content and flush the file.
class OutputBuffer {
  public:
    //! The value type of the buffer.
    using value_type = char;

    //! Construt the buffer with an optional file handle.
    OutputBuffer(FILE *out = nullptr) : out_{out} {}

    //! Flush the buffer.
    //!
    //! This function is a noop if there is no associated file.
    void flush() {
        if (out_ != nullptr) {
            fwrite(buf_.data(), sizeof(char), size_, out_);
            fflush(out_);
            size_ = 0;
        }
    }

    //! Flush the buffer if it has a predefined minimum size.
    //!
    //! This function is a noop if there is no associated file.
    void endl() {
        constexpr auto n = 8192;
        if (out_ != nullptr && size_ > n) {
            fwrite(buf_.data(), sizeof(char), size_, out_);
            size_ = 0;
        }
    }

    //! Get the number of bytes currently stored in the buffer.
    [[nodiscard]] auto size() const -> size_t { return static_cast<size_t>(size_); }

    //! Check if the buffer is currently emtpy.
    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }

    //! Get a string view of the current buffer content.
    [[nodiscard]] auto view() const -> std::string_view {
        return std::string_view{buf_.data(), static_cast<size_t>(size_)};
    }

    //! Get a char span of the current buffer content.
    [[nodiscard]] auto span() -> std::span<char> { return std::span{buf_.data(), static_cast<size_t>(size_)}; }

    //! Get a string with the current buffer content.
    [[nodiscard]] auto str() const -> std::string { return std::string{buf_.data(), static_cast<size_t>(size_)}; }

    //! Get a C string with the current buffer content.
    [[nodiscard]] auto c_str() -> char const * {
        *ensure_(1) = '\0';
        return buf_.data();
    }

    //! Empty the buffer.
    auto reset() -> OutputBuffer & {
        size_ = 0;
        return *this;
    }

    //! Empty the buffer and return a vector with the previous content.
    auto release() -> std::vector<char> {
        buf_.resize(size_);
        buf_.emplace_back('\0');
        auto ret = std::move(buf_);
        buf_.clear();
        size_ = 0;
        return ret;
    }

    //! Append a string to the buffer.
    void append(char const *str) {
        auto n = static_cast<std::ptrdiff_t>(std::strlen(str));
        std::copy(str, std::next(str, n), ensure_(n));
        size_ += n;
    }

    //! Append a string to the buffer.
    void append(std::string_view str) {
        auto n = static_cast<std::ptrdiff_t>(str.length());
        std::ranges::copy(str, ensure_(n));
        size_ += n;
    }

    //! Append a char to the buffer.
    void append(char c) {
        *ensure_(1) = c;
        ++size_;
    }

    //! Alias for append(char) to support std::back_inserter.
    void push_back(char c) { append(c); }

    //! Pop a char from the buffer.
    void pop() { --size_; }

    //! Append an integral to the buffer.
    template <std::integral T> void append(T num, int base = 10) { // NOLINT
        constexpr auto n = 256;
        auto *end = ensure_(n);
        auto res = std::to_chars(end, limit_(), num, base);
        size_ += res.ptr - end;
    }

    //! Append n bytes at the end of the buffer.
    //!
    //! The returned span should be filled by the calling code.
    auto reserve(std::ptrdiff_t n) -> std::span<char> {
        auto *begin = ensure_(n);
        size_ += n;
        return {begin, std::next(begin, n)};
    }

    //! Trim trailing zeros.
    //!
    //! @note Workaround for mp_int_string_len providing too high length values.
    void trim_zero(std::ptrdiff_t len) {
        auto sp = std::span{buf_.data(), static_cast<size_t>(size_)};
        auto ie = sp.end();
        auto ib = sp.begin() + (size_ - len);
        auto it = std::find(ib, ie, '\0');
        size_ -= ie - it;
    }

    //! Append the given integral to the buffer.
    template <std::integral T> friend auto operator<<(OutputBuffer &out, T num) -> OutputBuffer & {
        out.append(num);
        return out;
    }

    //! Append the given char to the buffer.
    friend auto operator<<(OutputBuffer &out, char c) -> OutputBuffer & {
        out.append(c);
        return out;
    }

    //! Append the given string to the buffer.
    friend auto operator<<(OutputBuffer &out, std::string_view str) -> OutputBuffer & {
        out.append(str);
        return out;
    }

    //! Append the given double to the buffer.
    friend auto operator<<(OutputBuffer &out, double value) -> OutputBuffer & {
        static constexpr std::ptrdiff_t n = 32;
        auto *begin = out.ensure_(n);
        auto *end = std::next(begin, n);
        auto [res, ec] = std::to_chars(begin, end, value);
        out.size_ += std::distance(begin, res);
        return out;
    }

    //! Append the given string to the buffer.
    friend auto operator<<(OutputBuffer &out, char const *str) -> OutputBuffer & {
        out.append(str);
        return out;
    }

  private:
    auto limit_() -> char * { return std::next(buf_.data(), static_cast<std::ptrdiff_t>(buf_.size())); }

    auto ensure_(std::ptrdiff_t n) -> char * {
        auto m = size_ + n;
        assert(n >= 0 && m >= 0);
        if (buf_.size() < static_cast<size_t>(m)) {
            buf_.reserve(2 * static_cast<size_t>(m));
            buf_.resize(buf_.capacity());
        }
        return std::next(buf_.data(), size_);
    }

    std::vector<char> buf_;
    std::ptrdiff_t size_ = 0;
    FILE *out_;
};

//! Helper to use OutputBuffer with iostreams.
class OutputStream : public std::ostream {
  public:
    // Construct the stream with an optional file handle.
    OutputStream(FILE *out = nullptr) : std::ostream{&buf_}, buf_{out} {}
    //! Get the underlying output buffer.
    auto buffer() -> OutputBuffer & { return buf_.buffer(); }

  private:
    class Buffer : public std::streambuf {
      public:
        Buffer(FILE *out) : out_{out} {}
        auto buffer() -> OutputBuffer & { return out_; }

      protected:
        auto overflow(int ch) -> int override {
            if (ch != traits_type::eof()) {
                out_.push_back(static_cast<char>(ch));
                return ch;
            }
            return traits_type::eof();
        }

        auto xsputn(char const *s, std::streamsize n) -> std::streamsize override {
            out_.append(std::string_view{s, static_cast<size_t>(n)});
            return n;
        }

        auto sync() -> int override {
            out_.flush();
            return 0;
        }

      private:
        OutputBuffer out_;
    } buf_;
};

namespace Detail {

//! Helper to print an object.
struct PrintSelf {
    //! Print the given object.
    template <class Out> void operator()(Out &out, auto const &x) { out << x; }
};

//! Wrapper for range with a separator and a mapper for printing.
template <class It, class F> class PrintRange {
  public:
    //! Print the range with the given separator.
    template <class A>
    PrintRange(It first, It last, char const *sep, A &&fun)
        : first_{first}, last_{last}, sep_{sep}, fun_{std::forward<A>(fun)} {}
    //! Output the range.
    template <class Out> friend auto operator<<(Out &out, PrintRange rng) -> Out & {
        if (rng.first_ != rng.last_) {
            rng.fun_(out, *rng.first_);
            for (++rng.first_; rng.first_ != rng.last_; ++rng.first_) {
                out << rng.sep_;
                rng.fun_(out, *rng.first_);
            }
        }
        return out;
    }

  private:
    It first_;
    It last_;
    char const *sep_;
    [[no_unique_address]] F fun_;
};

template <class It, class F> PrintRange(It, It, char const *, F &&) -> PrintRange<It, std::unwrap_ref_decay_t<F>>;

//! Helper to inject a function to print something.
template <class F> class PrintFun {
  public:
    //! Construct the helper.
    template <class A> PrintFun([[maybe_unused]] int tag, A &&fun) : fun_{std::forward<A>(fun)} {}
    //! Call the function while outputting.
    template <class Out> friend auto operator<<(Out &out, PrintFun x) -> Out & {
        x.fun_(out);
        return out;
    }

  private:
    F fun_;
};

template <class F> PrintFun(int, F &&) -> PrintFun<std::unwrap_ref_decay_t<F>>;

//! Helper to print something in quotes.
class PrintQuoted {
  public:
    //! Construct the helper.
    PrintQuoted(std::string_view str, bool fstring) : str_{str}, fstring_{fstring} {}
    //! Output quoted.
    template <class Out> friend auto operator<<(Out &out, PrintQuoted x) -> Out & {
        if (!x.fstring_) {
            out << '"';
        }
        for (auto c : x.str_) {
            if (c == '\\') {
                out << "\\\\";
            } else if (x.fstring_ && c == '{') {
                out << "{{";
            } else if (x.fstring_ && c == '}') {
                out << "}}";
            } else if (c == '\n') {
                out << "\\n";
            } else if (c == '\t') {
                out << "\\t";
            } else if (c == '"') {
                out << "\\\"";
            } else {
                out << c;
            }
        }
        if (!x.fstring_) {
            out << '"';
        }
        return out;
    }

  private:
    std::string_view str_;
    bool fstring_ = false;
};

} // namespace Detail

//! Helper for iostreams to fill with a fixed number of characters.
class fill {
  public:
    //! The constructor.
    fill(size_t n, char c = ' ') : n_{n}, c_{c} {}
    //! Operator to print the fill helper.
    friend auto operator<<(CppClingo::Util::OutputBuffer &out, fill const &x) -> CppClingo::Util::OutputBuffer & {
        std::ranges::fill(out.reserve(static_cast<std::ptrdiff_t>(x.n_)), x.c_);
        return out;
    }

  private:
    size_t n_;
    char c_;
};

//! Print with a function.
template <class F> auto p_fun(F &&fun) {
    return Detail::PrintFun(0, std::forward<F>(fun));
}

//! Print a range with a separator.
template <class T, class F> auto p_range(T const &rng, char const *sep, F &&fun) {
    using std::begin, std::end;
    return Detail::PrintRange{begin(rng), end(rng), sep, std::forward<F>(fun)};
}

//! Print a range separated by comma.
template <class T> auto p_range(T const &rng) {
    return p_range(rng, ",", Detail::PrintSelf{});
}

//! Print a range separated by comma.
template <class T, class F> auto p_range(T const &rng, F &&fun) {
    return p_range(rng, ",", std::forward<F>(fun));
}

//! Print a range with a separator.
template <class T> auto p_range(T const &rng, char const *sep) {
    return p_range(rng, sep, Detail::PrintSelf{});
}

//! Quote and print the given string.
inline auto p_quoted(std::string_view str, bool fstring = false) {
    return Detail::PrintQuoted{str, fstring};
}

//! @}

} // namespace CppClingo::Util
