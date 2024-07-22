#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <istream>

namespace Gringo {

class LexerState {
  public:
    //! Initial size of the input buffer.
    static constexpr size_t default_buffer_size = 4096;

    //! Construct an invalid lexer state.
    LexerState() = default;
    //! Construct a lexer state reading from the given stream.
    //!
    //! This initializes the buffer filling it with zeros and moving the cursor to the end
    //! triggering a call to fill() when calling a lexer the first time.
    LexerState(std::istream &in) : in_{&in} {
        buffer_.resize(default_buffer_size, '\0');
        buffer_.resize(buffer_.capacity());
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        token_ = column_ = cursor_ = marker_ = ctxmarker_ = limit_ = buffer_.data() + buffer_.size() - 1;
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    //! Move construct a lexer state.
    LexerState(LexerState &&state) = default;

    //! Lexer states are not copyable due to the underlying stream.
    auto operator=(LexerState const &other) -> LexerState & = delete;
    //! Move assign a lexer state.
    auto operator=(LexerState &&other) noexcept -> LexerState &;

    //! Mark the beginning of a token.
    void start() {
        cursor_column_ = cursor_column();
        token_ = cursor_;
        token_line_ = cursor_line_;
        token_column_ = cursor_column_;
    }
    //! Pointer to the current input position.
    auto cursor() -> char *& { return cursor_; }
    //! Pointer to the position of latest matched rule.
    auto marker() -> char *& { return marker_; }
    //! Pointer to the position of the trailing context.
    auto ctxmarker() -> char *& { return ctxmarker_; }
    //! Pointer marking the end of the input.
    [[nodiscard]] auto limit() const -> char * { return limit_; }
    //! Pointer to the current input position.
    [[nodiscard]] auto token() const -> char * { return token_; }
    //! Mark the beginning of a new line.
    void step() {
        column_ = cursor_;
        cursor_column_ = 1;
        cursor_line_ += 1;
    }
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
            // NOLINTNEXTLINE(readability-magic-numbers)
            if ((*column_ & 0xc0) != 0x80) {
                ++cursor_column_;
            }
        }
        return cursor_column_;
    }
    //! Fill the input buffer discarding input before the position marked with start.
    //!
    //! Reallocates if no characters can be discarded
    //! noting the that the buffer is always completely filled
    //! unless the end of input has been reached.
    auto fill() -> bool;

  private:
    std::istream *in_ = nullptr;
    std::vector<char> buffer_;
    char *token_ = nullptr;
    char *column_ = nullptr;
    char *cursor_ = nullptr;
    char *marker_ = nullptr;
    char *ctxmarker_ = nullptr;
    char *limit_ = nullptr;
    size_t cursor_column_ = 1;
    size_t cursor_line_ = 1;
    size_t token_line_ = 1;
    size_t token_column_ = 1;
    bool eof_ = false;
};

auto LexerState::fill() -> bool {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (eof_) {
        return false;
    }

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
    } else {
        // we have to reallocate (unlikely due to large buffer)
        buffer_.resize(buffer_.size() * 2);
        buffer_.resize(buffer_.capacity());
        token_ = buffer_.data() + (token_ - buffer);
        column_ = buffer_.data() + (column_ - buffer);
        cursor_ = buffer_.data() + (cursor_ - buffer);
        marker_ = buffer_.data() + (marker_ - buffer);
        ctxmarker_ = buffer_.data() + (ctxmarker_ - buffer);
        limit_ = buffer_.data() + (limit_ - buffer);
    }

    auto read = static_cast<ssize_t>(buffer_.size() - 1) - used;
    auto count = in_->read(limit_, read).gcount();
    limit_ += count;
    *limit_ = '\0';
    if (count < read) {
        eof_ = true;
    }
    return true;
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

} // namespace Gringo
