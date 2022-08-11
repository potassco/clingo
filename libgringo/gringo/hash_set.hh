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

#include <tsl/sparse_set.h>
#include <tl/optional.hpp>

// This shoud really be benchmarked on a large set of problems.
// I would expect double hashing to be more robust but slower than linear probing.
// Also the load factor should be benchmarked.
// The internet says 70% is good but I am using 80% at the moment.
// Double hashing is supposed to work with especially high load factors.
#define GRINGO_PROBE_LINEAR

namespace Gringo {

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
    // Note: to support lookup with key
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

template <typename Value, typename Hash=std::hash<Value>, typename EqualTo=std::equal_to<Value>>
class UniqueVec : private Hash, private EqualTo {
public:
    using SizeType = unsigned;
    using Vec = std::vector<Value>;
    using Set = HashSet<SizeType>;
    using ValueType = Value;
    using Iterator = typename Vec::iterator;
    using ConstIterator = typename Vec::const_iterator;

    template <typename T, typename... A>
    std::pair<Iterator,bool> findPush(T const &key, A&&... args) {
        auto offset = static_cast<SizeType>(vec_.size());
        auto res = set_.insert(
            [this, offset, &key](SizeType a) { return a != offset ? Hash::operator()(vec_[a]) : Hash::operator()(key); },
            [this, offset, &key](SizeType a, SizeType b) { return b != offset ? a == b : EqualTo::operator()(vec_[a], key); },
            offset);
        if (res.second) {
            vec_.emplace_back(std::forward<A>(args)...);
        }
        return {vec_.begin() + res.first, res.second};
    }
    std::pair<Iterator,bool> push(Value &&val) {
        auto offset = static_cast<SizeType>(vec_.size());
        auto res = set_.insert(
            [this, offset, &val](SizeType a) { return a != offset ? Hash::operator()(vec_[a]) : Hash::operator()(val); },
            [this, offset, &val](SizeType a, SizeType b) { return b != offset ? a == b : EqualTo::operator()(vec_[a], val); },
            offset);
        if (res.second) {
            vec_.push_back(std::forward<Value>(val));
        }
        return {vec_.begin() + res.first, res.second};
    }
    SizeType offset(ConstIterator it) const {
        return static_cast<SizeType>(it - begin());
    }
    template <typename... A>
    std::pair<Iterator,bool> push(A&&... args) {
        return push(Value(std::forward<A>(args)...));
    }
    template <class U>
    Iterator find(U const &val) {
        auto offset = static_cast<SizeType>(vec_.size());
        auto res = set_.find(
            [this, offset, &val](SizeType a) { return a != offset ? Hash::operator()(vec_[a]) : Hash::operator()(val); },
            [this, offset, &val](SizeType a, SizeType b) { return b != offset ? a == b : EqualTo::operator()(vec_[a], val); },
            offset);
        return res ? vec_.begin() + *res : vec_.end();
    }
    template <class U>
    ConstIterator find(U const &val) const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        return const_cast<UniqueVec*>(this)->find(val);
    }
    void pop() {
        assert(!vec_.empty());
        set_.erase(
            [this](SizeType a) { return Hash::operator()(vec_[a]); },
            [this](SizeType a, SizeType b) { return a == b; },
            numeric_cast<SizeType>(vec_.size() - 1));
        vec_.pop_back();
    }
    Value &back() { return vec_.back(); }
    Value const &back() const { return vec_.back(); }
    Value &front() { return vec_.front(); }
    Value const &front() const { return vec_.front(); }
    void rebuild() {
        set_.clear();
        for (SizeType i = 0, e = static_cast<SizeType>(vec_.size()); i != e; ++i) {
            set_.insert(
                [this](SizeType a) { return Hash::operator()(vec_[a]); },
                [](SizeType, SizeType) { return false; },
                i);
        }
    }
    void reserve(SizeType size) {
        vec_.reserve(size);
        set_.reserve(
            [this](SizeType a) { return Hash::operator()(vec_[a]); },
            [](SizeType, SizeType) { return false; },
            size);
    }
    template <class F>
    void erase(F f) {
        vec_.erase(std::remove_if(vec_.begin(), vec_.end(), f), vec_.end());
        rebuild();
    }
    // This function keeps an element in the vector but deletes it from the set.
    // Note that it is inserted again after the next call to rebuild, erase or sort.
    // It is best not to use this function!
    void hide(Iterator it) {
        auto offset = static_cast<SizeType>(it - begin());
        set_.erase(
            [this](SizeType a) { return Hash::operator()(vec_[a]); },
            [](SizeType a, SizeType b) { return a == b; },
            offset);
    }
    template <class F>
    void sort(F f = std::less<ValueType>()) {
        std::sort(vec_.begin(), vec_.end(), f);
        rebuild();
    }
    Iterator begin() { return vec_.begin(); }
    Iterator end() { return vec_.end(); }
    ConstIterator begin() const { return vec_.begin(); }
    ConstIterator end() const { return vec_.end(); }
    SizeType size() const { return static_cast<SizeType>(vec_.size()); }
    bool empty() const { return vec_.empty(); }
    void clear() {
        vec_.clear();
        set_.clear();
    }
    ValueType &at(SizeType offset) { return vec_.at(offset); }
    ValueType const &at(SizeType offset) const { return vec_.at(offset); }
    ValueType &operator[](SizeType offset) { return vec_[offset]; }
    ValueType const &operator[](SizeType offset) const { return vec_[offset]; }
    Vec to_vec() {
        set_.clear();
        return std::move(vec_);
    }

private:
    Vec vec_;
    Set set_;
};

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
        if (!impl_) {
            impl_ = std::make_unique<Impl>();
        }
        return impl_->insert(key);
    }
    tl::optional<index_type> find(key_type const &key) const {
        if (!impl_) {
            impl_ = std::make_unique<Impl>();
        }
        return impl_->find(key);
    }
    key_type at(index_type const &idx) const {
        return impl_->at(idx);
    }
    void clear() {
        impl_.reset();
    }
    bool empty() const {
        return !impl_ || impl_->empty();
    }
    template <class F>
    void erase_if(F f) {
        if (impl_) {
            impl_ = impl_->erase_if(std::move(f));
        }
    }
private:
    struct Impl : private Hash, private KeyEqual {
        struct Index {
            index_type index{0, 0};
            size_t hash{0};
        };
        struct Hasher {
            size_t operator()(Index const &idx) const {
                return idx.hash;
            }
            friend void swap(Hasher &a, Hasher &b) {
                static_cast<void>(a);
                static_cast<void>(b);
            }
        };
        struct EqualTo {
            using is_transparent = void;
            bool operator()(Index const &a, Index const &b) const {
                auto it_a = values->begin() + a.index.first;
                auto it_b = values->begin() + b.index.first;
                return
                    a.index.second == b.index.second &&
                    std::equal(it_a, it_a + a.index.second, it_b);
            }
            bool operator()(key_type const &a, Index const &b) const {
                auto it_a = begin(a);
                auto it_b = values->begin() + b.index.first;
                return a.size == b.index.second && std::equal(it_a, it_a + a.size, it_b);
            }
            friend void swap(EqualTo &a, EqualTo &b) {
                std::swap(a.values, b.values);
            }
            std::vector<Key> const *values;
        };

        Impl()
        : data{0, Hasher{}, EqualTo{&values}} { }

        std::pair<index_type, bool> insert(key_type const &key) {
            size_t hash = hash_range(begin(key), end(key), static_cast<Hash&>(*this));
            uint32_t index = numeric_cast<uint32_t>(values.size());
            values.insert(values.end(), begin(key), end(key));
            auto ret = data.insert(Index{{index, numeric_cast<uint32_t>(key.size)}, hash});
            if (!ret.second) {
                values.resize(index);
            }
            return {ret.first->index, ret.second};
        }

        tl::optional<index_type> find(key_type const &key) const {
            size_t hash = hash_range(begin(key), end(key), static_cast<Hash&>(*this));
            auto it = data.find(key, hash);
            return it == data.end() ? tl::nullopt : tl::make_optional(it->index);
        }

        key_type at(index_type const &idx) const {
            return {values.data() + idx.first, idx.second};
        }

        bool empty() const {
            return data.empty();
        }

        template <class F>
        void erase_if(F f) {
            // Note: could be made more space efficient by grouping elements by size
            //       and then backfilling removed elements
            auto impl = std::make_unique<Impl>();
            for (auto &idx : data) {
                auto key = at(idx);
                if (f(key)) {
                    impl->insert(key);
                }
            }
        }
        std::vector<Key> values;
        tsl::sparse_set<Index, Hasher, EqualTo> data;
    };

    std::unique_ptr<Impl> impl_;
};

} // namespace Gringo

#endif // GRINGO_HASH_SET_HH
