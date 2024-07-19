#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <istream>
#include <memory>

namespace Gringo {

static constexpr size_t default_buffer_size = 4096;

class LexerState {
  public:
    LexerState() = default;
    LexerState(std::unique_ptr<std::istream> in) : in_{std::move(in)} {}

    auto operator=(LexerState const &other) -> LexerState & = delete;
    auto operator=(LexerState &&other) noexcept -> LexerState &;

    // interface for lexer
    void start() { token_ = cursor_; }
    auto cursor() -> char *& { return cursor_; }
    auto marker() -> char *& { return marker_; }
    auto ctxmarker() -> char *& { return ctxmarker_; }
    auto token() -> char *& { return token_; }
    auto limit() -> char *& { return limit_; }
    void fill();

  private:
    std::unique_ptr<std::istream> in_;
    std::vector<char> buffer_;
    char *cursor_ = nullptr;
    char *marker_ = nullptr;
    char *ctxmarker_ = nullptr;
    char *token_ = nullptr;
    char *limit_ = nullptr;
    bool eof_ = false;
};

void LexerState::fill() {
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (eof_) {
        return;
    }

    auto shift = token_ - buffer_.data();
    auto used = limit_ - token_;

    if (shift > 0) {
        // make room to read more characters
        std::memmove(buffer_.data(), token_, used);
        cursor_ -= shift;
        marker_ -= shift;
        ctxmarker_ -= shift;
        token_ -= shift;
        limit_ -= shift;
    } else {
        // we have to reallocate
        // - the buffer is quite large
        // - this should usually only happen intially
        auto offset_cursor = cursor_ - buffer_.data();
        auto offset_marker = marker_ - buffer_.data();
        auto offset_ctxmarker = ctxmarker_ - buffer_.data();
        auto offset_token = token_ - buffer_.data();
        auto offset_limit = limit_ - buffer_.data();
        buffer_.resize(buffer_.empty() ? default_buffer_size : buffer_.size() * 2);
        buffer_.resize(buffer_.capacity());
        cursor_ = buffer_.data() + offset_cursor;
        marker_ = buffer_.data() + offset_marker;
        ctxmarker_ = buffer_.data() + offset_ctxmarker;
        token_ = buffer_.data() + offset_token;
        limit_ = buffer_.data() + offset_limit;
    }

    auto count = static_cast<ssize_t>(buffer_.size()) - used - 1;
    auto read = in_->read(limit_, count).gcount();
    limit_ += read;
    *limit_ = '\0';
    if (read < count) {
        eof_ = true;
    }
    // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
}

} // namespace Gringo
