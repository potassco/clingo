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

#ifndef _GRINGO_UTILITY_HH
#define _GRINGO_UTILITY_HH

#include <memory>
#include <vector>
#include <set>
#include <functional>
#include <sstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cassert>

namespace Gringo {
 
// {{{ declaration of gringo_make_unique<T, Args>

template<typename T, typename ...Args>
std::unique_ptr<T> gringo_make_unique(Args&& ...args);

// }}}
// {{{ declaration of helpers to perform comparisons

template <class T>
struct value_equal_to {
    bool operator()(T const &a, T const &b) const;
};

template <class T>
struct value_equal_to<T*> {
    bool operator()(T const *a, T const *b) const;
};

template <class T>
struct value_equal_to<std::unique_ptr<T>> {
    bool operator()(std::unique_ptr<T> const &a, std::unique_ptr<T> const &b) const;
};

template <class T>
struct value_equal_to<std::reference_wrapper<T>> {
    bool operator()(std::reference_wrapper<T> const &a, std::reference_wrapper<T> const &b) const;
};

template <class T, class U>
struct value_equal_to<std::pair<T, U>> {
    bool operator()(std::pair<T, U> const &a, std::pair<T, U> const &b) const;
};

template <class... T>
struct value_equal_to<std::tuple<T...>> {
    bool operator()(std::tuple<T...> const &a, std::tuple<T...> const &b) const;
};

template <class... T>
struct value_equal_to<std::vector<T...>> {
    bool operator()(std::vector<T...> const &a, std::vector<T...> const &b) const;
};

template <class T>
bool is_value_equal_to(T const &a, T const &b);

// }}}
// {{{ declaration of helpers to calculate hashes

template <class T>
void hash_combine(std::size_t& seed, const T& v);

template <class T>
struct value_hash {
    size_t operator()(T const &x) const;
};

template <class T>
struct value_hash<T*> {
    size_t operator()(T const *x) const;
};

template <class T>
struct value_hash<std::unique_ptr<T>> {
    size_t operator()(std::unique_ptr<T> const &p) const;
};

template <class T>
struct value_hash<std::reference_wrapper<T>> {
    size_t operator()(std::reference_wrapper<T> const &x) const;
};

template <class T, class U>
struct value_hash<std::pair<T, U>> {
    size_t operator()(std::pair<T, U> const &v) const;
};

template <class... T>
struct value_hash<std::tuple<T...>> {
    size_t operator()(std::tuple<T...> const &x) const;
};

template <class T>
struct value_hash<std::vector<T>> {
    size_t operator()(std::vector<T> const &v) const;
};

template <class T>
inline size_t get_value_hash(T const &x);

template <class T, class U, class... V>
inline size_t get_value_hash(T const &x, U const &y, V const &... args);

// }}}
// {{{ declaration of helpers to perform clone copies

template <class T>
struct clone {
    T operator()(T const &x) const;
};

template <class T>
struct clone<T*> {
    T *operator()(T *x) const;
};

template <class T>
struct clone<std::unique_ptr<T>> {
    std::unique_ptr<T> operator()(std::unique_ptr<T> const &x) const;
};

template <class... T>
struct clone<std::tuple<T...>> {
    std::tuple<T...> operator()(std::tuple<T...> const &x) const;
};

template <class T, class U>
struct clone<std::pair<T, U>> {
    std::pair<T, U> operator()(std::pair<T, U> const &x) const;
};

template <class T>
struct clone<std::vector<T>> {
    std::vector<T> operator()(std::vector<T> const &x) const;
};

template <class T>
T get_clone(T const &x);

// }}}
// {{{ declaration of algorithms

template <class T, class U>
void print_comma(std::ostream &out, T const &x, const char *sep, U const &f);

template <class T>
void print_comma(std::ostream &out, T const &x, const char *sep);

template <class T>
void cross_product(std::vector<std::vector<T>> &vec);

// }}}

// {{{ definition of gringo_make_unique

template<typename T, typename ...Args>
std::unique_ptr<T> gringo_make_unique(Args&& ...args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// }}}
// {{{ definition of helpers to perform comparisons

template <class T>
bool value_equal_to<T>::operator()(T const &a, T const &b) const {
    return std::equal_to<T>()(a, b);
}

template <class T>
bool value_equal_to<T*>::operator()(T const *a, T const *b) const {
    return is_value_equal_to(*a, *b);
}

template <class T>
bool value_equal_to<std::unique_ptr<T>>::operator()(std::unique_ptr<T> const &a, std::unique_ptr<T> const &b) const {
    assert(a && b);
    return is_value_equal_to(*a, *b);
}

template <class T>
bool value_equal_to<std::reference_wrapper<T>>::operator()(std::reference_wrapper<T> const &a, std::reference_wrapper<T> const &b) const {
    return is_value_equal_to(a.get(), b.get());
}

template <class T, class U>
bool value_equal_to<std::pair<T, U>>::operator()(std::pair<T, U> const &a, std::pair<T, U> const &b) const {
    return is_value_equal_to(a.first, b.first) && is_value_equal_to(a.second, b.second);
}

namespace detail {
    template <int N>
    struct equal {
        template <class... T>
        bool operator()(std::tuple<T...> const &a, std::tuple<T...> const &b) const {
            return is_value_equal_to(std::get<sizeof...(T) - N>(a), std::get<sizeof...(T) - N>(b)) && equal<N - 1>()(a, b);
        }
    };
    template <>
    struct equal<0> {
        template <class... T>
        bool operator()(std::tuple<T...> const &, std::tuple<T...> const &) const {
            return true;
        }
    };
}

template <class... T>
bool value_equal_to<std::tuple<T...>>::operator()(std::tuple<T...> const &a, std::tuple<T...> const &b) const {
    return detail::equal<sizeof...(T)>()(a, b);
}

template <class... T>
bool value_equal_to<std::vector<T...>>::operator()(std::vector<T...> const &a, std::vector<T...> const &b) const {
    if (a.size() != b.size()) { return false; }
    for (auto i = a.begin(), j = b.begin(), end = a.end(); i != end; ++i, ++j)
    {
        if (!is_value_equal_to(*i, *j)) { return false; }
    }
    return true;
}

template <class T>
inline bool is_value_equal_to(T const &a, T const &b) {
    return value_equal_to<T>()(a, b);
}

// }}}
// {{{ definition of helpers to calculate hashes

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

template <class T>
inline size_t value_hash<T>::operator()(T const &x) const {
    return std::hash<T>()(x);
}

template <class T>
size_t value_hash<T*>::operator()(T const *x) const {
    return get_value_hash(*x);
}

template <class T>
size_t value_hash<std::unique_ptr<T>>::operator()(std::unique_ptr<T> const &p) const {
    assert(p);
    return get_value_hash(*p);
}

template <class T>
size_t value_hash<std::reference_wrapper<T>>::operator()(std::reference_wrapper<T> const &x) const {
    return get_value_hash(x.get());
}

template <class T, class U>
size_t value_hash<std::pair<T, U>>::operator()(std::pair<T, U> const &v) const {
    size_t seed = 1;
    hash_combine(seed, get_value_hash(v.first));
    hash_combine(seed, get_value_hash(v.second));
    return seed;
}

namespace detail
{
    template <int N>
    struct hash {
        template <class... T>
        size_t operator()(std::tuple<T...> const &x) const {
            size_t seed = get_value_hash(std::get<sizeof...(T) - N>(x));
            hash_combine(seed, hash<N-1>()(x));
            return seed;
        }
    };
    template <>
    struct hash<0> {
        template <class... T>
        size_t operator()(std::tuple<T...> const &) const {
            return 2;
        }
    };
}

template <class... T>
inline size_t value_hash<std::tuple<T...>>::operator()(std::tuple<T...> const &x) const {
    return detail::hash<sizeof...(T)>()(x);
}

template <class T>
inline size_t value_hash<std::vector<T>>::operator()(std::vector<T> const &v) const {
    size_t seed = 3;
    for (auto const &x : v) {
        hash_combine(seed, get_value_hash(x));
    }
    return seed;
}

template <class T>
inline size_t get_value_hash(T const &x) {
    return value_hash<T>()(x);
}

template <class T, class U, class... V>
inline size_t get_value_hash(T const &x, U const &y, V const &... args) {
    size_t seed = value_hash<T>()(x);
    hash_combine(seed, get_value_hash(y, args...));
    return seed;
}

// }}}
// {{{ definition of helpers to perform clone copies

template <class T>
inline T clone<T>::operator()(T const &x) const {
    return x;
}

template <class T>
inline T *clone<T*>::operator()(T *x) const {
    assert(x);
    return x->clone();
}

template <class T>
inline std::unique_ptr<T> clone<std::unique_ptr<T>>::operator()(std::unique_ptr<T> const &x) const {
    T *y(get_clone(x.get()));
    assert(x.get() != y);
    return std::unique_ptr<T>(y);
}

template <class T, class U>
inline std::pair<T, U> clone<std::pair<T, U>>::operator()(std::pair<T, U> const &x) const {
    return std::make_pair(get_clone(x.first), get_clone(x.second));
}

namespace detail
{
    template <int ...>
    struct seq { };

    template <int N, int ...S>
    struct gens : gens<N-1, N-1, S...> { };

    template <int ...S>
    struct gens<0, S...> {
        typedef seq<S...> type;
    };

    template <class... T, int... S>
    std::tuple<T...> clone(const std::tuple<T...> &x, seq<S...>) {
        return std::tuple<T...> { get_clone(std::get<S>(x))... };
    }
}

template <class... T>
std::tuple<T...> clone<std::tuple<T...>>::operator()(std::tuple<T...> const &x) const {
    return detail::clone(x, typename detail::gens<sizeof...(T)>::type());
}

template <class T>
inline std::vector<T> clone<std::vector<T>>::operator()(std::vector<T> const &x) const {
    std::vector<T> res;
    res.reserve(x.size());
    for (auto &y : x) { res.emplace_back(get_clone(y)); }
    return res;
}

template <class T>
T get_clone(T const &x) {
    return clone<T>()(x);
}

// }}}
// {{{ definition of algorithms

template <class T, class U>
void print_comma(std::ostream &out, T const &x, const char *sep, U const &f) {
    auto it(std::begin(x)), end(std::end(x));
    if (it != end) {
        f(out, *it);
        for (++it; it != end; ++it) { out << sep; f(out, *it); }
    }
}

template <class T>
void print_comma(std::ostream &out, T const &x, const char *sep) {
    auto it(std::begin(x)), end(std::end(x));
    if (it != end) {
        out << *it;
        for (++it; it != end; ++it) { out << sep << *it; }
    }
}

template <class T>
inline void cross_product(std::vector<std::vector<T>> &vec) {
    unsigned size = 1;
    for (auto &x : vec) {
        unsigned n = x.size();
        if (n == 0) {
            vec.clear();
            return;
        }
        size *= n;
    }
    std::vector<std::vector<T>> res;
    res.reserve(size);
    res.emplace_back();
    res.back().reserve(vec.size());
    for (auto &x : vec) {
        auto it = res.begin();
        for (auto lt = x.begin(), mt = x.end() - 1; lt != mt; ++lt) {
            auto jt = it;
            it = res.end();
            auto kt = jt;
            for (; kt != it; ++kt) { res.emplace_back(get_clone(*kt)); }
            for (kt = jt; kt != it - 1; ++kt) { kt->emplace_back(get_clone(*lt)); }
            kt->emplace_back(std::move(*lt));
        }
        for (auto kt = res.end() - 1; it != kt; ++it) { it->emplace_back(get_clone(x.back())); }
        it->emplace_back(std::move(x.back()));
    }
    vec = std::move(res);
}

// }}}

} // namespace Gringo

#endif // _GRINGO_UTILITY_HH
