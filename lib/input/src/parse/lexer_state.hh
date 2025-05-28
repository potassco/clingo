#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <istream>

namespace CppClingo {

class LexerState {
  public:
    //! Initial size of the input buffer.
    static constexpr size_t default_buffer_size = 4096;

    //! Construct an uninitialized lexer state.
    LexerState() = default;

    //! Reset the lexer state.
    void reset() {
        in_ = nullptr;
        buffer_.clear();
        token_ = nullptr;
        column_ = nullptr;
        cursor_ = nullptr;
        marker_ = nullptr;
        ctxmarker_ = nullptr;
        limit_ = nullptr;
        cursor_column_ = 1;
        cursor_line_ = 1;
        token_line_ = 1;
        token_column_ = 1;
        eof_ = false;
    }

    //! Initialize the lexer state for reading from the given stream.
    //!
    //! This initializes the buffer filling it with zeros.
    void init(std::istream &in, size_t padding) {
        reset();
        in_ = &in;
        size_t n = default_buffer_size;
        while (n < padding) {
            n *= 2;
        }
        buffer_.resize(n, '\0');
        token_ = column_ = cursor_ = marker_ = ctxmarker_ = limit_ = buffer_.data();
    }

    //! Initialize the lexer state for reading from the given string view.
    //!
    //! Note that the string_view is copied because of the required terminating
    //! null bytes.
    void init(std::string_view in, size_t padding) {
        reset();
        eof_ = true;
        buffer_.reserve(in.size() + padding);
        buffer_.assign(in.begin(), in.end());
        buffer_.resize(in.size() + padding, '\0');
        token_ = column_ = cursor_ = marker_ = ctxmarker_ = buffer_.data();
        limit_ = std::next(buffer_.data(), static_cast<std::ptrdiff_t>(in.size() + padding));
    }

    //! Move construct a lexer state.
    LexerState(LexerState &&state) = default;

    //! Lexer states are not copyable due to the underlying stream.
    auto operator=(LexerState const &other) -> LexerState & = delete;
    //! Move assign a lexer state.
    auto operator=(LexerState &&other) noexcept -> LexerState &;

    //! Mark the beginning of a token.
    void start() {
        assert(eof_ || in_ != nullptr);
        cursor_column_ = cursor_column();
        token_ = cursor_;
        token_line_ = cursor_line_;
        token_column_ = cursor_column_;
    }
    //! Pointer to the current input position.
    auto cursor() -> char const *& { return cursor_; }
    //! Pointer to the position of latest matched rule.
    auto marker() -> char const *& { return marker_; }
    //! Pointer to the position of the trailing context.
    auto ctxmarker() -> char const *& { return ctxmarker_; }
    //! Pointer marking the end of the input.
    [[nodiscard]] auto limit() const -> char const * { return limit_; }
    //! Pointer to the current input position.
    [[nodiscard]] auto token() const -> char const * { return token_; }
    //! The line number of the cursor.
    [[nodiscard]] auto token_line() const -> size_t { return token_line_; }
    //! The column number of the cursor.
    //!
    //! Currently, does not support unicode.
    [[nodiscard]] auto token_column() const -> size_t { return token_column_; }
    //! The line number of the cursor.
    [[nodiscard]] auto cursor_line() const -> size_t { return cursor_line_; }
    //! The column number of the cursor.
    //!
    //! Assumes that the input is valid UTF8 and counts code points.
    [[nodiscard]] auto cursor_column() -> size_t {
        assert(column_ <= cursor_);
        for (; column_ != cursor_; column_ = std::next(column_)) {
            if (*column_ == '\n') {
                cursor_column_ = 0;
                cursor_line_ += 1;
            }
            // NOLINTNEXTLINE(readability-magic-numbers)
            if ((*column_ & 0xc0) != 0x80) {
                ++cursor_column_;
            }
        }
        return cursor_column_;
    }
    //! Get the string view from start till current cursor.
    [[nodiscard]] auto view() -> std::string_view { return {token_, cursor_}; }

    //! Fill the input buffer discarding input before the position marked with start.
    //!
    //! Reallocates if no characters can be discarded
    //! noting the that the buffer is always completely filled
    //! unless the end of input has been reached.
    auto fill(size_t n, size_t padding) -> bool;

    //! Check if the current token points to the terminating zero byte.
    [[nodiscard]] auto end(size_t padding) const -> bool {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return token() == limit() - padding;
    }

  private:
    std::istream *in_ = nullptr;
    std::vector<char> buffer_;
    char const *token_ = nullptr;
    char const *column_ = nullptr;
    char const *cursor_ = nullptr;
    char const *marker_ = nullptr;
    char const *ctxmarker_ = nullptr;
    char *limit_ = nullptr;
    size_t cursor_column_ = 1;
    size_t cursor_line_ = 1;
    size_t token_line_ = 1;
    size_t token_column_ = 1;
    bool eof_ = false;
};

inline auto LexerState::fill(size_t n, size_t padding) -> bool {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (eof_) {
        return false;
    }

    assert(in_ != nullptr);

    auto *buffer = buffer_.data();
    auto shift = token_ - buffer;
    auto used = limit_ - token_;

    if (shift > 0) {
        // make room to read more characters
        std::memmove(buffer, token_, used);
        token_ -= shift;
        column_ -= shift;
        cursor_ -= shift;
        marker_ -= shift;
        ctxmarker_ -= shift;
        limit_ -= shift;
    }
    while (buffer_.size() - used - padding < n) {
        // we have to reallocate (unlikely due to large buffer)
        buffer_.resize(buffer_.size() * 2);
        token_ = buffer_.data() + (token_ - buffer);
        column_ = buffer_.data() + (column_ - buffer);
        cursor_ = buffer_.data() + (cursor_ - buffer);
        marker_ = buffer_.data() + (marker_ - buffer);
        ctxmarker_ = buffer_.data() + (ctxmarker_ - buffer);
        limit_ = buffer_.data() + (limit_ - buffer);
    }

    auto read = static_cast<std::ptrdiff_t>(buffer_.size() - used - padding);
    limit_ += in_->read(limit_, read).gcount();

    if (limit_ < buffer_.data() + buffer_.size() - padding) {
        eof_ = true;
        memset(limit_, 0, padding);
        limit_ += padding;
    }
    return true;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

} // namespace CppClingo
