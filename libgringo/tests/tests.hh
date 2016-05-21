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

#ifndef _GRINGO_TEST_TESTS_HH
#define _GRINGO_TEST_TESTS_HH

#include "catch.hpp"
#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/extensions/HelperMacros.h>
#include "gringo/utility.hh"
#include "gringo/logger.hh"
#include <algorithm>
#include <unordered_map>

namespace Gringo {

// {{{ declaration of helpers to print containers

namespace IO {

template <class T>
std::ostream &operator<<(std::ostream &out, std::unique_ptr<T> const &x);
template <class... T>
std::ostream &operator<<(std::ostream &out, std::vector<T...> const &x);
template <class... T>
std::ostream &operator<<(std::ostream &out, std::set<T...> const &x);
template <class T, class U>
std::ostream &operator<<(std::ostream &out, std::pair<T, U> const &x);
template <class... T>
std::ostream &operator<<(std::ostream &out, std::tuple<T...> const &x);
template <class... T>
std::ostream &operator<<(std::ostream &out, std::map<T...> const &x);
template <class... T>
std::ostream &operator<<(std::ostream &out, std::unordered_map<T...> const &x);
template <class T>
std::string to_string(T const &x);

} // namespace IO

// }}}

// {{{ definition of helpers to print containers

namespace IO {

template <class T>
std::ostream &operator<<(std::ostream &out, std::unique_ptr<T> const &x) {
    out << *x;
    return out;
}

template <class T, class U>
std::ostream &operator<<(std::ostream &out, std::pair<T, U> const &x) {
    out << "(" << x.first << "," << x.second << ")";
    return out;
}

template <class... T>
std::ostream &operator<<(std::ostream &out, std::vector<T...> const &vec) {
    out << "[";
    auto it(vec.begin()), end(vec.end());
    if (it != end) {
        out << *it;
        for (++it; it != end; ++it) { out << "," << *it; }
    }
    out << "]";
    return out;
}

template <class... T>
std::ostream &operator<<(std::ostream &out, std::set<T...> const &x) {
    out << "{";
    auto it(x.begin()), end(x.end());
    if (it != end) {
        out << *it;
        for (++it; it != end; ++it) { out << "," << *it; }
    }
    out << "}";
    return out;
}


namespace detail {
    template <int N>
    struct print {
        template <class... T>
        void operator()(std::ostream &out, std::tuple<T...> const &x) const {
            out << std::get<sizeof...(T) - N>(x) << ",";
            print<N-1>()(out, x);
        }
    };
    template <>
    struct print<1> {
        template <class... T>
        void operator()(std::ostream &out, std::tuple<T...> const &x) const {
            out << std::get<sizeof...(T) - 1>(x);
        }
    };
    template <>
    struct print<0> {
        template <class... T>
        void operator()(std::ostream &, std::tuple<T...> const &) const { }
    };
}

template <class... T>
std::ostream &operator<<(std::ostream &out, std::tuple<T...> const &x) {
    out << "(";
    detail::print<sizeof...(T)>()(out, x);
    out << ")";
    return out;
}

template <class... T>
std::ostream &operator<<(std::ostream &out, std::unordered_map<T...> const &x) {
    std::vector<std::pair<std::string, std::string>> vals;
    for (auto const &y : x) { vals.emplace_back(to_string(y.first), to_string(y.second)); }
    std::sort(vals.begin(), vals.end());
    out << "{";
    auto it = vals.begin(), end = vals.end();
    if (it != end) {
        out << it->first << ":" << it->second;
        for (++it; it != end; ++it) { out << "," << it->first << ":" << it->second; }
    }
    out << "}";
    return out;
}

template <class... T>
std::ostream &operator<<(std::ostream &out, std::map<T...> const &x) {
    out << "{";
    auto it = x.begin(), end = x.end();
    if (it != end) {
        out << it->first << ":" << it->second;
        for (++it; it != end; ++it) { out << "," << it->first << ":" << it->second; }
    }
    out << "}";
    return out;
}

template <class T>
std::string to_string(T const &x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

} // namesapce IO

// }}}

inline std::string &replace_all(std::string &haystack, std::string const &needle, std::string const &replace) {
    size_t index = 0;
    while (true) {
        index = haystack.find(needle, index);
        if (index == std::string::npos) break;
        haystack.replace(index, needle.length(), replace);
        index += replace.length();
    }
    return haystack;
}

inline std::string replace_all(std::string &&haystack, std::string const &needle, std::string const &replace) {
	replace_all(haystack, needle, replace);
	return std::move(haystack);
}

}

namespace Gringo { namespace Test {

// {{{ definition of helpers to initialize vectors

namespace Detail {

template <int, class, int, class...>
struct walker;

template <class, int, int, int, class...>
struct emplacer;

template <class V, int S1, int S2, int S3, class A, class... T>
struct emplacer<V, S1, S2, S3, A, T...> {
    void operator()(V& v, A&& a, T&&... args) const {
        // reverse
        emplacer<V, S1-1, S2, S3, T..., A>()(v, std::forward<T>(args)..., std::forward<A>(a));
    }
};

template <class V, int S2, int S3, class A, class... T>
struct emplacer<V, 0, S2, S3, A, T...> {
    void operator()(V& v, A&&, T&&... args) const {
        // drop
        emplacer<V, 0, S2-1, S3, T...>()(v, std::forward<T>(args)...);
    }
};

template <class V, int S3, class A, class... T>
struct emplacer<V, 0, 0, S3, A, T...> {
    void operator()(V& v, A&& a, T&&... args) const {
        // reverse
        emplacer<V, 0, 0, S3-1, T..., A>()(v, std::forward<T>(args)..., std::forward<A>(a));
    }
};

template <class V, class A, class... T>
struct emplacer<V, 0, 0, 0, A, T...> {
    void operator()(V &v, A&& a, T&&... args) const {
        v.emplace_back(std::forward<A>(a), std::forward<T>(args)...);
    }
};

template <int N, class V, int I, class A, class... T>
struct walker<N, V, I, A, T...> {
    void operator()(V& v, A&&, T&&... args) const {
        walker<N, V, I-1, T...>()(v, std::forward<T>(args)...);
    }
};

template <int N, class V, class A, class... T>
struct walker<N, V, 0, A, T...> {
    void operator()(V& v, A&& a, T&&... args) const {
        emplacer<V, N, sizeof...(T)+1-N, N, A, T...>()(v, std::forward<A>(a), std::forward<T>(args)...);
        walker<N, V, N-1, T...>()(v, std::forward<T>(args)...);
    }
};

// Note: optimization for N=1
template <class V, class A, class... T>
struct walker<1, V, 0, A, T...> {
    void operator()(V& v, A&& a, T&&... args) const {
        v.emplace_back(std::forward<A>(a));
        walker<1, V, 0, T...>()(v, std::forward<T>(args)...);
    }
};

template <int N, class V, int I>
struct walker<N, V, I> {
    static_assert(N > 0, "think - makes no sense!");
    void operator()(V&) const { }
};

} // namespace Detail

template <class V, class... T>
V init(T&&... args) {
    V v;
    Detail::walker<1, V, 0, T...>()(v, std::forward<T>(args)...);
    return std::move(v);
}

template <int N, class V, class... T>
V init(T&&... args) {
    static_assert(N > 0, "think - makes no sense!");
    V v;
    Detail::walker<N, V, 0, T...>()(v, std::forward<T>(args)...);
    return std::move(v);
}

// }}}
// {{{ defintion of TestMessagePrinter

struct TestMessagePrinter : MessagePrinter {
    TestMessagePrinter(std::vector<std::string> &messages)
        : messages_(messages)                  { }
    virtual bool check(Errors)                 { error_ = true; return true; }
    virtual bool check(Warnings)               { return true; }
    virtual bool hasError() const              { return error_; }
    virtual void enable(Warnings)              { }
    virtual void disable(Warnings)             { }
    virtual void print(std::string const &msg) { messages_.emplace_back(msg); }
    virtual ~TestMessagePrinter() { }
private:
    std::vector<std::string> &messages_;
    bool error_ = false;
};

struct Messages {
    Messages()
        : oldPrinter(std::move(message_printer())) {
        message_printer() = gringo_make_unique<Gringo::Test::TestMessagePrinter>(messages);
    }
    void clear() {
        messages.clear();
        message_printer() = gringo_make_unique<Gringo::Test::TestMessagePrinter>(messages);
    }
    ~Messages() {
        message_printer() = std::move(oldPrinter);
    }
    std::vector<std::string>        messages;
private:
    std::unique_ptr<MessagePrinter> oldPrinter;
};

inline std::ostream &operator<<(std::ostream &out, Messages const &x) {
    out << IO::to_string(x.messages);
    return out;
}

// }}}

} } // namespace Test Gringo

#endif // _GRINGO_TEST_TESTS_HH
