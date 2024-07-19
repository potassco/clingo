#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <istream>
#include <memory>

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
    LexerState(std::unique_ptr<std::istream> in) : in_{std::move(in)} {
        buffer_.resize(default_buffer_size, '\0');
        buffer_.resize(buffer_.capacity());
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        cursor_ = marker_ = ctxmarker_ = limit_ = buffer_.data() + buffer_.size() - 1;
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    //! Move construct a lexer state.
    LexerState(LexerState &&state) = default;

    //! Lexer states are not copyable due to the underlying stream.
    auto operator=(LexerState const &other) -> LexerState & = delete;
    //! Move assign a lexer state.
    auto operator=(LexerState &&other) noexcept -> LexerState &;

    //! Mark the beginning of a token.
    void start() { token_ = cursor_; }
    //! Pointer to the current input position.
    auto cursor() -> char *& { return cursor_; }
    //! Pointer to the position of latest matched rule.
    auto marker() -> char *& { return marker_; }
    //! Pointer to the position of the trailing context.
    auto ctxmarker() -> char *& { return ctxmarker_; }
    //! Pointer marking the end of the input.
    auto limit() -> char *& { return limit_; }
    //! State variable to capture start conditions.
    auto condition() -> int & { return condition_; }
    //! Fill the input buffer discarding input before the position marked with start.
    //!
    //! Reallocates if no characters can be discarded
    //! noting the that the buffer is always completely filled
    //! unless the end of input has been reached.
    auto fill() -> bool;

  private:
    std::unique_ptr<std::istream> in_;
    std::vector<char> buffer_;
    char *cursor_ = nullptr;
    char *marker_ = nullptr;
    char *ctxmarker_ = nullptr;
    char *token_ = nullptr;
    char *limit_ = nullptr;
    int condition_ = 0;
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
        cursor_ -= shift;
        marker_ -= shift;
        ctxmarker_ -= shift;
        token_ -= shift;
        limit_ -= shift;
    } else {
        // we have to reallocate (unlikely due to large buffer)
        buffer_.resize(buffer_.size() * 2);
        buffer_.resize(buffer_.capacity());
        cursor_ = buffer_.data() + (cursor_ - buffer);
        marker_ = buffer_.data() + (marker_ - buffer);
        ctxmarker_ = buffer_.data() + (ctxmarker_ - buffer);
        token_ = buffer_.data() + (token_ - buffer);
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
