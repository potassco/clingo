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

#ifndef UTIL_PROGRAM_HH
#define UTIL_PROGRAM_HH

#include <utility>
#include <memory>
#include <functional>
#include <vector>
#include <iostream>
#include <potassco/basic_types.h>

namespace Reify {

inline std::ostream &operator<<(std::ostream &out, Potassco::StringSpan str) {
    out.write(str.first, str.size);
    return out;
}

template<typename Lambda>
class ScopeGuard {
public:
    ScopeGuard(Lambda &&f) : f_(std::forward<Lambda>(f)) { }
    ~ScopeGuard() { f_(); }
private:
    Lambda f_;
};

template<typename Lambda>
ScopeGuard<Lambda> makeScopeGuard(Lambda &&f) {
    return {std::forward<Lambda>(f)};
}

template<typename T, typename ...Args>
std::unique_ptr<T> gringo_make_unique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
void printValue(std::ostream &out, T const &value) {
    out << value;
}

template<typename T, typename U>
void printValue(std::ostream &out, std::pair<T, U> const &value) {
    printValue(out, value.first);
    out << ",";
    printValue(out, value.second);
}

template<typename T>
void printComma(std::ostream &out, T const &t) {
    printValue(out, t);
}

template<typename T, typename U, typename... V>
void printComma(std::ostream &out, T const &t, U const &u, V const &...v) {
    printValue(out, t);
    out << ",";
    printComma(out, u, v...);
}

template<typename T>
struct Hash : std::hash<T> { };

template<typename T, typename U>
struct Hash<std::pair<T, U>> {
    size_t operator()(std::pair<T, U> const &p) const noexcept {
        size_t hash = std::hash<T>()(p.first);
        hash ^= Hash<U>()(p.second) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        return hash;
    }
};

template<typename T>
struct Hash<std::vector<T>> {
    size_t operator()(std::vector<T> const &vec) const noexcept {
        size_t hash = vec.size();
        for (auto &x : vec) {
            hash ^= Hash<typename std::vector<T>::value_type>()(x) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        }
        return hash;
    }
};

template <class T>
std::vector<T> toVec(Potassco::Span<T> span) {
    return {span.first, span.first + span.size};
}

} // namespace Reify

#endif // UTIL_PROGRAM_HH
