// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef GRINGO_HASH_SET_HH
#define GRINGO_HASH_SET_HH

#include "potassco/basic_types.h"
#include <iterator>
#include <memory>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <array>
#include <gringo/primes.hh>
#include <gringo/utility.hh>
#include <tl/optional.hpp>
#include <tsl/ordered_set.h>
#include <tsl/ordered_map.h>
#include <unordered_set>
#if CLINGO_MAP_TYPE == 0
#   include <tsl/hopscotch_set.h>
#   include <tsl/hopscotch_map.h>
#elif CLINGO_MAP_TYPE == 1
#   include <tsl/sparse_set.h>
#   include <tsl/sparse_map.h>
#endif

namespace Gringo {

template <class Key,
          class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>,
          class ValueTypeContainer = std::vector<Key, Allocator>,
          class IndexType = std::uint_least32_t>
using ordered_set = tsl::ordered_set<Key, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

template <class Key,
          class Value,
          class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<std::pair<Key, Value>>,
          class ValueTypeContainer = std::vector<std::pair<Key, Value>, Allocator>,
          class IndexType = std::uint_least32_t>
using ordered_map = tsl::ordered_map<Key, Value, Hash, KeyEqual, Allocator, ValueTypeContainer, IndexType>;

#if CLINGO_MAP_TYPE == 0

template <class Key, class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>,
          unsigned int NeighborhoodSize = 62, bool StoreHash = false>
using hash_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

template <class Key,
          class Value,
          class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<std::pair<Key, Value>>,
          unsigned int NeighborhoodSize = 62,
          bool StoreHash = false>
using hash_map = tsl::hopscotch_map<Key, Value, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

#elif CLINGO_MAP_TYPE == 1

template <class Key, class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>>
using hash_set = tsl::sparse_set<Key, mix_hash, KeyEqual, Allocator>;

template <class Key,
          class Value,
          class Hash = mix_hash<Key>,
          class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<std::pair<Key, Value>>>
using hash_map = tsl::sparse_map<Key, Value, mix_Hash, KeyEqual, Allocator>;

#endif

// To Be Removed...
#define GRINGO_PROBE_LINEAR

template <typename Value>
struct HashSetLiterals {
    static constexpr Value deleted = std::numeric_limits<Value>::max() - 1;
    static constexpr Value open    = std::numeric_limits<Value>::max();
};

template <typename Value>
constexpr Value HashSetLiterals<Value>::deleted;

template <typename Value>
constexpr Value HashSetLiterals<Value>::open;

template <typename Value, typename Literals = HashSetLiterals<Value>>
class HashSet {
public:
    using ValueType = Value;
    using SizeType = uint32_t;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    using TableType = std::unique_ptr<ValueType[]>;

    // at least n value can be inserted without reallocation
    // and the container is larger by a constant factor c > 1 than c*r
    HashSet(SizeType n = 0, SizeType r = 0) {
        if (n > 0) {
            reserved_ = grow_(n, r);
            table_.reset(new ValueType[reserved_]);
            std::fill(table_.get(), table_.get() + reserved_, Literals::open);
        }
    }
#ifdef GRINGO_PROBE_LINEAR
    double loadMax() const { return 0.7; }
#else
    double loadMax() const { return 0.8; }
#endif
    double load() const { return static_cast<double>(size()) / reserved_; }
    SizeType size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() {
        std::fill(table_.get(), table_.get() + reserved(), Literals::open);
        size_ = 0;
    }
    void swap(HashSet &other) {
        std::swap(table_, other.table_);
        std::swap(reserved_, other.reserved_);
        std::swap(size_, other.size_);
    }
    SizeType reserved() const { return reserved_; }
    SizeType maxSize() const { return maxPrime<SizeType>(); }
    bool reserveNeedsRebuild(SizeType n) const {
        if (n <= 11) {
            return n > reserved();
        }
        double load = double(n) / reserved_;
        return (load > loadMax() && reserved_ < maxSize()) || n > maxSize();
    }
    template <typename Hasher, typename EqualTo>
    void reserve(Hasher const &hasher, EqualTo const &equalTo, SizeType n) {
        if (reserveNeedsRebuild(n)) {
            SizeType rOld = reserved_;
            SizeType rNew = grow_(n, reserved_);
            assert(rOld < rNew);
            if (table_) {
                TableType table(new ValueType[rNew]);
                reserved_ = rNew;
                std::fill(table.get(), table.get() + reserved_, Literals::open);
                std::swap(table, table_);
                for (auto it = table.get(), ie = table.get() + rOld; it != ie; ++it) {
                    if (!(*it == Literals::open) && !(*it == Literals::deleted)) { insert_(hasher, equalTo, std::move(*it)); }
                }
            }
            else {
                table_.reset(new ValueType[rNew]);
                reserved_ = rNew;
                std::fill(table_.get(), table_.get() + reserved_, Literals::open);
            }
        }
    }
    template <typename Hasher, typename EqualTo, typename... Args>
    ValueType* find(Hasher const &hasher, EqualTo const &equalTo, Args const&... val) {
        auto ret = !empty() ? find_(hasher, equalTo, val...) : std::make_pair(nullptr, false);
        return ret.second ? ret.first : nullptr;
    }
    template <typename Hasher, typename EqualTo, typename T>
    std::pair<ValueType&, bool> insert(Hasher const &hasher, EqualTo const &equalTo, T &&val) {
        reserve(hasher, equalTo, size() + 1);
        assert(size() < reserved());
        auto ret = insert_(hasher, equalTo, std::forward<T>(val));
        if (ret.second) {
            ++size_;
        }
        return ret;
    }
    ValueType &at(SizeType offset) { return table_[offset]; }
    ValueType const &at(SizeType offset) const { return table_[offset]; }
    SizeType offset(ValueType &val) const { return static_cast<SizeType>(&val - table_.get()); }
    template <typename Hasher, typename EqualTo, typename... Args>
    bool erase(Hasher const &hasher, EqualTo const &equalTo, Args const&... val) {
        if (auto ret = find(hasher, equalTo, val...)) {
            *ret = Literals::deleted;
            --size_;
            return true;
        }
        return false;
    }
    void eraseRef(SizeType &ref) {
        ref = Literals::deleted;
        --size_;
    }
private:
    SizeType grow_(SizeType n, SizeType r) {
        if (n > maxSize()) { throw std::overflow_error("container size exceeded"); }
        if (n > 11) { n = std::min(static_cast<SizeType>(std::max(n / loadMax() + 1.0, r * 2.0)), maxSize()); }
        return n < 4 ? n : nextPrime(n);
    }
#ifdef GRINGO_PROBE_LINEAR
    template <typename Hasher, typename... Args>
    SizeType hash_(Hasher const &hasher, Args const &... val) {
        size_t seed = hasher(val...);
        return hash_mix(seed) % reserved();
    }
    template <typename Hasher, typename EqualTo, typename... Args>
    std::pair<ValueType*, bool> find_(Hasher const &hasher, EqualTo const &equalTo, Args&&... val) {
        ValueType *first = nullptr;
        for (SizeType pos = hash_(hasher, val...), end = reserved();;) {
            for (SizeType i = pos; i < end; ++i) {
                if (table_[i] == Literals::open) {
                    if (!first) { first = &table_[i]; }
                    return {first, false};
                }
                if (table_[i] == Literals::deleted) {
                    if (!first) { first = &table_[i]; }
                    continue;
                }
                if (equalTo(table_[i], val...)) {
                    return {&table_[i], true};
                }
            }
            if (pos > 0) {
                end = pos;
                pos = 0;
            }
            else { break; }
        }
        return {first, false};
    }
#else
    // Double hashing
    template <typename Hasher, typename... Args>
    std::pair<SizeType, SizeType> hash_(Hasher const &hasher, Args const &... val) {
        size_t seed = hasher(val...);
        SizeType r = reserved();
        //if (r > 1) { return {seed % r, 1 + (hash_mix(seed) % (r - 1))}; }
        if (r > 1) { return {hash_mix(seed) % r, 1 + (seed % (r-1))}; }
        return {0, 1};
    }
    template <typename Hasher, typename EqualTo, typename... Args>
    std::pair<ValueType*, bool> find_(Hasher const &hasher, EqualTo const &equalTo, Args&&... val) {
        ValueType *first = nullptr;
        auto h = hash_(hasher, val...);
        for (SizeType i = 0, e = reserved(); i < e; ++i, h.first = (h.first + h.second) % e) {
            if (table_[h.first] == Literals::open) {
                if (!first) { first = &table_[h.first]; }
                return {first, false};
            }
            else if (table_[h.first] == Literals::deleted) {
                if (!first) { first = &table_[h.first]; }
                continue;
            }
            else if (equalTo(table_[h.first], val...)) {
                return {&table_[h.first], true};
            }
        }
        return {first, false};
    }
#endif
    template <typename Hasher, typename EqualTo, typename T>
    std::pair<ValueType&, bool> insert_(Hasher const &hasher, EqualTo const &equalTo, T &&val) {
        assert(size() < reserved());
        auto ret = find_(hasher, equalTo, val);
        if (!ret.second) {
            assert(ret.first);
            *ret.first = std::forward<T>(val);
        }
        return {*ret.first, !ret.second};
    }

    SizeType size_{0};
    SizeType reserved_{0};
    TableType table_;
};

struct CallHash {
    template <typename T>
    size_t operator()(T const &x) const {
        return x.hash();
    }
};

struct EqualTo {
    template <typename T, typename U>
    size_t operator()(T const &a, U const &b) const {
        return a == b;
    }
};

template <typename T>
struct Cast {
    template <typename U>
    T const &operator()(U const &x) const {
        return static_cast<T const &>(x);
    }
};

template <typename T>
struct First {
    template <typename U>
    T const &operator()(U const &x) const {
        return std::get<0>(x);
    }
};

template <typename T, typename Get=Cast<T>, typename Hash=std::hash<T>>
struct HashKey : private Get, private Hash {
    // Note: to support lookup with key
    size_t operator()(T const &x) const {
        return Hash::operator()(x);
    }
    template <class U>
    size_t operator()(U const &x) const {
        return Hash::operator()(Get::operator()(x));
    }
};

template <typename T, typename Get=Cast<T>, typename EqualTo=std::equal_to<T>>
struct EqualToKey : private Get, private EqualTo {
    using is_transparent = void;
    // Note: to support lookup with key
    template <typename U>
    bool operator()(T const &a, U const &b) const {
        return EqualTo::operator()(a, Get::operator()(b));
    }
    template <typename U>
    bool operator()(U const &a, T const &b) const {
        return EqualTo::operator()(Get::operator()(a), b);
    }
    template <typename U>
    bool operator()(U const &a, U const &b) const {
        return EqualTo::operator()(Get::operator()(a), Get::operator()(b));
    }
};

template <typename T, typename Hash=std::hash<T>>
using HashFirst = HashKey<T,First<T>,Hash>;

template <typename T, typename EqualTo=std::equal_to<T>>
using EqualToFirst = EqualToKey<T,First<T>,EqualTo>;

template <typename C>
Potassco::Span<typename C::value_type> make_span(C const &container) {
    return {container.data(), container.size()};
}

template <typename Key, typename Hash=std::hash<Key>, typename KeyEqual=std::equal_to<Key>>
class array_set {
public:
    using key_type = typename Potassco::Span<Key>;
    using value_type = typename Potassco::Span<Key>;
    using index_type = std::pair<uint32_t, uint32_t>;
    using size_type = uint32_t;
    using difference_type = int32_t;
    using hasher = Hash;
    using key_equal = KeyEqual;

    std::pair<index_type, bool> insert(key_type const &key) {
        if (key.size > 0) {
            return impl_[key.size].insert(key);
        }
        // the special case that the key is empty is handled by inserting an
        // empty Impl object
        auto it = impl_.find(0);
        if (it == impl_.end()) {
            impl_.emplace(0, Impl{});
            return {{0, 0}, true};
        }
        return {{0, 0}, false};
    }
    tl::optional<index_type> find(key_type const &key) const {
        auto it = impl_.find(key.size);
        if (it == impl_.end()) {
            return tl::nullopt;
        }
        if (key.size > 0) {
            return impl_->find(key);
        }
        // if the key is empty and there is an Impl object, then it has already
        // been inserted
        return index_type{0, 0};
    }
    key_type at(index_type const &idx) const {
        if (idx.second > 0) {
            return impl_.find(idx.second)->second.at(idx);
        }
        return key_type{nullptr, 0};
    }
    void clear() {
        impl_.clear();
    }
    bool empty() const {
        return impl_.empty();
    }
private:
    struct Impl : private Hash, private KeyEqual {
        struct Hasher {
            size_t operator()(index_type const &idx) const {
                return impl->hash_(impl->at(idx));
            }
            friend void swap(Hasher &a, Hasher &b) {
                std::swap(a.impl, b.impl);
            }
            Impl *impl;
        };
        struct EqualTo {
            using is_transparent = void;
            bool operator()(index_type const &a, index_type const &b) const {
                auto span_a = impl->at(a);
                auto span_b = impl->at(b);
                return std::equal(begin(span_a), end(span_a), begin(span_b));
            }
            bool operator()(index_type const &a, key_type const &b) const {
                auto span_a = impl->at(a);
                return std::equal(begin(span_a), end(span_a), begin(b));
            }
            bool operator()(key_type const &a, index_type const &b) const {
                return operator()(b, a);
            }
            friend void swap(EqualTo &a, EqualTo &b) {
                std::swap(a.impl, b.impl);
            }
            Impl *impl;
        };

        Impl()
        : data{0, Hasher{this}, EqualTo{this}} { }

        size_t hash_(key_type const &key) {
            return hash_mix(hash_range(begin(key), end(key), static_cast<Hash&>(*this)));
        }

        std::pair<index_type, bool> insert(key_type const &key) {
            size_t index = values.size();
            values.insert(values.end(), begin(key), end(key));
            auto ret = data.insert({numeric_cast<uint32_t>(index / key.size),
                                    numeric_cast<uint32_t>(key.size)});
            if (!ret.second) {
                values.resize(index);
            }
            return {*ret.first, ret.second};
        }

        tl::optional<index_type> find(key_type const &key) const {
            auto it = data.find(key, hash_(key));
            return it == data.end() ? tl::nullopt : tl::make_optional(it->index);
        }

        key_type at(index_type const &idx) const {
            return {values.data() + static_cast<size_t>(idx.first) * idx.second, idx.second};
        }

        std::vector<Key> values;
        hash_set<index_type, Hasher, EqualTo> data;
    };

    std::unordered_map<size_t, Impl> impl_;
};

} // namespace Gringo

#endif // GRINGO_HASH_SET_HH
