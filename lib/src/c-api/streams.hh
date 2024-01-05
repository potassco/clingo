#include <iostream>
#include <streambuf>

namespace {

class CountBuf : public std::streambuf {
  public:
    [[nodiscard]] auto count() const -> size_t { return static_cast<size_t>(count_); }

  protected:
    auto overflow(int_type ch) -> int_type override {
        count_++;
        return ch;
    }
    auto xsputn(char_type const *c, std::streamsize count) -> std::streamsize override {
        static_cast<void>(c);
        count_ += count;
        return count;
    }

  private:
    std::streamsize count_ = 0;
};

class CountStream : public std::ostream {
  public:
    CountStream() : std::ostream(&buf_) {
        exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
    }
    [[nodiscard]] auto count() -> size_t {
        flush();
        return buf_.count();
    }

  private:
    CountBuf buf_;
};

class ArrayBuf : public std::streambuf {
  public:
    ArrayBuf(char *begin, size_t size) {
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        setg(begin, begin, begin + size);
        setp(begin, begin + size);
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    auto seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) -> pos_type override {
        if (dir == std::ios_base::cur) {
            off += offset(which);
        } else if (dir == std::ios_base::end) {
            off = size() - off;
        }
        return seekpos(off, which);
    }
    auto seekpos(pos_type off, std::ios_base::openmode which) -> pos_type override {
        if (off >= 0 && off <= size()) {
            if ((which & std::ios_base::in) != 0) {
                gbump(static_cast<int>(off - offset(which)));
            } else {
                pbump(static_cast<int>(off - offset(which)));
            }
            return off;
        }
        return std::streambuf::seekpos(off, which);
    }

  private:
    [[nodiscard]] auto size() const -> off_type { return egptr() - eback(); }
    [[nodiscard]] auto offset(std::ios_base::openmode which) const -> off_type {
        return ((which & std::ios_base::out) != 0) ? pptr() - pbase() : gptr() - eback();
    }
};

class ArrayStream : public std::iostream {
  public:
    ArrayStream(char *begin, size_t size) : std::iostream(&buf_), buf_(begin, size) {
        exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
    }

  private:
    ArrayBuf buf_;
};

template <class E> auto print_size(E const &x) -> size_t {
    CountStream cs;
    cs << x;
    return cs.count() + 1;
}

template <class E> void print(char *ret, size_t n, E const &x) {
    ArrayStream as(ret, n);
    as << x;
    as << '\0';
    as.flush();
}

} // namespace
