// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_LEXERSTATE_HH
#define _GRINGO_LEXERSTATE_HH

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>

namespace Gringo {

// {{{ declaration of LexerState

template <class T>
class LexerState {
private:
    struct State;

public:
    using Data = T;
    LexerState();
    void start();
    bool eof() const;
    bool push(std::string const &file, T &&data);
    bool push(std::unique_ptr<std::istream> in, T &&data);
    void pop();
    bool empty() const;
    std::string string(int start = 0, int end = 0);
    void step(char s);
    void step();
    int integer() const;
    int line() const;
    int column() const;
    T const &data() const;
    char *&cursor();
    char *&marker();
    char *&ctxmarker();
    char const *limit() const;
    void fill(size_t n);
    void seek(int offset);
private:
    State const &state() const;
    State &state();
private:
    std::vector<State> states_;
};

// }}}
// {{{ declaration of LexerState<T>::State

template <class T>
struct LexerState<T>::State {
    State(T &&data);
    State(State &&);
    void fill(size_t n);
    void step();
    void start();
    ~State();

    std::unique_ptr<std::istream> in_;
    T data_;
    size_t bufmin_;
    size_t bufsize_;
    char *buffer_;
    char *start_;
    char *offset_;
    char *cursor_;
    char *limit_;
    char *marker_;
    char *ctxmarker_;
    char *eof_;
    int line_;
};

// }}}

// {{{ defintion of LexerState<T>::State

template <class T>
LexerState<T>::State::State(T &&data)
    : data_(std::forward<T>(data))
    , bufmin_(4096), bufsize_(0)
    , buffer_(0), start_(0), offset_(0)
    , cursor_(0), limit_(0), marker_(0)
    , ctxmarker_(0), eof_(0), line_(1) { }

template <class T>
LexerState<T>::State::State(State &&x)
    : data_(std::forward<T>(x.data_))
    , bufmin_(x.bufmin_), bufsize_(x.bufsize_)
    , buffer_(0), start_(x.start_), offset_(x.offset_)
    , cursor_(x.cursor_), limit_(x.limit_), marker_(x.marker_)
    , ctxmarker_(x.ctxmarker_), eof_(x.eof_), line_(x.line_) {
    std::swap(x.in_, in_);
    std::swap(x.buffer_, buffer_);
}

template <class T>
void LexerState<T>::State::fill(size_t n) {
    if (eof_) return;
    if (start_ > buffer_) {
        size_t shift = start_ - buffer_;
        memmove(buffer_, start_, limit_ - start_);
        start_     = buffer_;
        offset_   -= shift;
        marker_   -= shift;
        ctxmarker_-= shift;
        limit_    -= shift;
        cursor_   -= shift;
    }
    size_t inc = n < bufmin_ ? bufmin_ : n;
    if(bufsize_ < inc + (limit_ - buffer_)) {
        bufsize_   = inc + (limit_ - buffer_);
        char *buf  = (char*)realloc(buffer_, bufsize_ * sizeof(char));
        start_     = buf + (start_ - buffer_);
        cursor_    = buf + (cursor_ - buffer_);
        limit_     = buf + (limit_ - buffer_);
        marker_    = buf + (marker_ - buffer_);
        ctxmarker_ = buf + (ctxmarker_ - buffer_);
        offset_    = buf + (offset_ - buffer_);
        buffer_    = buf;
    }
    in_->read(limit_, inc);
    limit_+= in_->gcount();
    if(size_t(in_->gcount()) < inc) {
        eof_ = limit_;
        *eof_++ = '\n';
    }
}

template <class T>
void LexerState<T>::State::step() {
    offset_ = cursor_;
    line_++;
}

template <class T>
void LexerState<T>::State::start() {
    start_ = cursor_;
}

template <class T>
LexerState<T>::State::~State() {
    if(buffer_) free(buffer_);
}

// }}}
// {{{ defintion of LexerState

template <class T>
LexerState<T>::LexerState() = default;

template <class T>
void LexerState<T>::start() {
    state().start();
}

template <class T>
bool LexerState<T>::eof() const {
    return state().cursor_ == state().eof_;
}

template <class T>
bool LexerState<T>::push(std::unique_ptr<std::istream> in, T &&data) {
    states_.emplace_back(std::forward<T>(data));
    state().in_ = std::move(in);
    return true;
}

template <class T>
bool LexerState<T>::push(std::string const &file, T &&data) {
    if (file == "-") {
        states_.emplace_back(std::forward<T>(data));
        state().in_.reset(new std::istream(std::cin.rdbuf(0)));
        return true;
    }
    else {
        std::unique_ptr<std::ifstream> ifs(new std::ifstream(file));
        if (ifs->is_open()) {
            states_.emplace_back(std::forward<T>(data));
            state().in_.reset(ifs.release());
            return true;
        }
        else { return false; }
    }
}

template <class T>
void LexerState<T>::pop() {
    states_.pop_back();
}

template <class T>
bool LexerState<T>::empty() const {
    return states_.empty();
}

template <class T>
std::string LexerState<T>::string(int start, int end) {
    return std::string(state().start_ + start, state().cursor_ - end);
}

template <class T>
void LexerState<T>::step(char s) {
    for (char *c = state().start_; c != state().cursor_; c++) {
        if (*c == s) { step(); }
    }
}

template <class T>
void LexerState<T>::step() {
    state().step();
}

template <class T>
int LexerState<T>::integer() const {
    int r = 0;
    int s = 0;
    if(*state().start_ == '-') s = 1;
    for(char *i = state().start_ + s; i != state().cursor_; i++) {
        r *= 10;
        r += *i - '0';
    }
    return s ? -r : r;
}

template <class T>
int LexerState<T>::line() const {
    return state().line_;
}

template <class T>
int LexerState<T>::column() const {
    return state().cursor_ - state().offset_ + 1;
}

template <class T>
T const &LexerState<T>::data() const {
    return state().data_;
}

template <class T>
char *&LexerState<T>::cursor() {
    return state().cursor_;
}

template <class T>
char *&LexerState<T>::marker() {
    return state().marker_;
}

template <class T>
char *&LexerState<T>::ctxmarker() {
    return state().ctxmarker_;
}

template <class T>
char const *LexerState<T>::limit() const {
    return state().limit_;
}

template <class T>
void LexerState<T>::fill(size_t n) {
    state().fill(n);
}

template <class T>
typename LexerState<T>::State const &LexerState<T>::state() const {
    assert(!empty());
    return states_.back();
}

template <class T>
typename LexerState<T>::State &LexerState<T>::state() {
    assert(!empty());
    return states_.back();
}

template <class T>
void LexerState<T>::seek(int offset) {
    state().cursor_ = state().start_ + offset;
    state().ctxmarker_ = state().cursor_;
}

// }}}

} // namespace Gringo

#endif // _GRINGO_LEXERSTATE_HH
