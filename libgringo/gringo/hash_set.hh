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

#ifndef _GRINGO_HASH_SET_HH
#define _GRINGO_HASH_SET_HH

#include <memory>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <gringo/primes.hh>
#include <gringo/utility.hh>

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
    using TableType = std::unique_ptr<ValueType[]>;

    // at least n value can be inserted without reallocation
    // and the container is larger by a constant factor c > 1 than c*r
    HashSet(SizeType n = 0, SizeType r = 0) : size_(0), reserved_(0) {
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
    size_t size() const { return size_; }
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
        if (n <= 11) { return n > reserved(); }
        else {
            double load = double(n) / reserved_;
            return (load > loadMax() && reserved_ < maxSize()) || n > maxSize();
        }
    }
    template <typename Hasher, typename EqualTo>
    void reserve(Hasher const &hasher, EqualTo const &equalTo, SizeType n) {
        if (reserveNeedsRebuild(n)) {
            SizeType rOld = reserved_;
            reserved_ = grow_(n, reserved_);
            assert(rOld < reserved_);
            if (table_) {
                TableType table(new ValueType[reserved_]);
                std::fill(table.get(), table.get() + reserved_, Literals::open);
                std::swap(table, table_);
                for (auto it = table.get(), ie = table.get() + rOld; it != ie; ++it) {
                    if (!(*it == Literals::open) && !(*it == Literals::deleted)) { insert_(hasher, equalTo, std::move(*it)); }
                }
            }
            else {
                table_.reset(new ValueType[reserved_]);
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
    SizeType offset(ValueType &val) const { return &val - table_.get(); }
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
        if (n > 11) { n = std::min<SizeType>(std::max(n / loadMax() + 1.0, r * 2.0), maxSize()); }
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
                else if (table_[i] == Literals::deleted) {
                    if (!first) { first = &table_[i]; }
                    continue;
                }
                else if (equalTo(table_[i], val...)) {
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
private:
    SizeType size_;
    SizeType reserved_;
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
    size_t operator()(T const &a, T const &b) const {
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
        SizeType offset = vec_.size();
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
        SizeType offset = vec_.size();
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
        return it - begin();
    }
    template <typename... A>
    std::pair<Iterator,bool> push(A&&... args) {
        return push(Value(std::forward<A>(args)...));
    }
    template <class U>
    Iterator find(U const &val) {
        SizeType offset = vec_.size();
        auto res = set_.find(
            [this, offset, &val](SizeType a) { return a != offset ? Hash::operator()(vec_[a]) : Hash::operator()(val); },
            [this, offset, &val](SizeType a, SizeType b) { return b != offset ? a == b : EqualTo::operator()(vec_[a], val); },
            offset);
        return res ? vec_.begin() + *res : vec_.end();
    }
    template <class U>
    ConstIterator find(U const &val) const {
        return const_cast<UniqueVec*>(this)->find(val);
    }
    void pop() {
        assert(!vec_.empty());
        set_.erase(
            [this](SizeType a) { return Hash::operator()(vec_[a]); },
            [this](SizeType a, SizeType b) { return a == b; },
            vec_.size() - 1);
        vec_.pop_back();
    }
    Value &back() { return vec_.back(); }
    Value const &back() const { return vec_.back(); }
    Value &front() { return vec_.front(); }
    Value const &front() const { return vec_.front(); }
    void rebuild() {
        set_.clear();
        for (SizeType i = 0, e = vec_.size(); i != e; ++i) {
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
        SizeType offset = it - begin();
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
    SizeType size() const { return vec_.size(); }
    bool empty() const { return vec_.empty(); }
    void clear() {
        vec_.clear();
        set_.clear();
    }
    ValueType &operator[](SizeType offset) { return vec_[offset]; }
    ValueType const &operator[](SizeType offset) const { return vec_[offset]; }

private:
    Vec vec_;
    Set set_;
};

template <class It>
class IteratorRange {
public:
    using ReferenceType = decltype(*std::declval<It>());
    using ValueType = typename std::remove_reference<ReferenceType>::type;
    template <class T>
    IteratorRange(T &&t)
    : begin_(std::begin(t))
    , end_(std::end(t)) { }
    IteratorRange(It a, It b)
    : begin_(a)
    , end_(b) { }
    It begin() const { return begin_; }
    It end() const { return end_; }
    template <class T>
    ReferenceType front() { return *begin(); }
    ReferenceType const front() const { return *begin(); }
    ReferenceType back() { return *(end()-1); }
    ReferenceType const back() const { *(end()-1); }
    bool empty() const { return begin_ == end_; }
    size_t size() const { return std::distance(begin_, end_); }
private:
    It begin_;
    It end_;
};

template <class It>
IteratorRange<It> make_range(It a, It b) {
    return IteratorRange<It>(a, b);
}

template <class T>
auto make_range(T &&c) -> IteratorRange<decltype(std::begin(c))> {
    return IteratorRange<decltype(std::begin(c))>(std::begin(c), std::end(c));
}

template <unsigned small, typename Value, typename Hash=std::hash<Value>, typename EqualTo=std::equal_to<Value>>
class UniqueVecVec : private Hash, private EqualTo {
public:
    using SizeType = unsigned;
    using Vec = std::vector<Value>;
    using ValueType = Value;
    using Iterator = typename Vec::iterator;
    using ConstIterator = typename Vec::const_iterator;

    template <typename It>
    std::pair<SizeType, bool> push(It ib, SizeType size) {
        auto &d = data(size);
        SizeType offset = size > 0 ? d.values.size() / size : 0;
        auto ret = d.set.insert(
            [this, offset, &d, ib, size](SizeType a) {
                auto it = d.values.begin() + size_t(a) * size;
                return a == offset ? hash_range(ib, ib + size, hasher()) : hash_range(it, it + size, hasher());
            },
            [this, offset, &d, ib, size](SizeType a, SizeType b) {
                auto it = d.values.begin() + size_t(a) * size;
                return b == offset ? std::equal(it, it + size, ib, equalTo()) : a == b;
            },
            offset);
        if (ret.second) {
            d.values.insert(d.values.end(), ib, ib + size);
        }
        return ret;
    }
    template <typename It>
    std::pair<SizeType,bool> push(It ib, It ie) {
        return push(ib, std::distance(ie, ib));
    }
    std::pair<SizeType,bool> push(Vec const &val) {
        return push(val.begin(), val.size());
    }
    std::pair<SizeType,bool> push(std::initializer_list<ValueType> val) {
        return push(val.begin(), val.size());
    }
    template <typename It>
    std::pair<SizeType,bool> find(It it, SizeType size) const {
        auto &d = data(size);
        SizeType offset = size > 0 ? d.values.size() / size : 0;
        auto res = d.set.find(
            [this, offset, &d, it, size](SizeType a) {
                auto jt = d.values.begin() + size_t(a) * size;
                return a == offset ? hash_range(it, it + size, hasher()) : hash_range(jt, jt + size, hasher());
            },
            [this, offset, &d, it, size](SizeType a, SizeType b) {
                auto jt = d.values.begin() + size_t(a) * size;
                return b == offset ? std::equal(it, it + size, jt, equalTo()) : a == b;
            },
            offset);
        return {res ? *res : 0, static_cast<bool>(res)};
    }
    template <typename It>
    std::pair<SizeType,bool> find(It ib, It ie) const {
        return find(ib, std::distance(ie, ib));
    }
    std::pair<SizeType,bool> find(Vec const &val) const {
        return find(val.begin(), val.size());
    }
    std::pair<SizeType,bool> find(std::initializer_list<ValueType> val) const {
        return find(val.begin(), val.size());
    }
    Iterator at(SizeType offset, SizeType size) {
        return data(size).values.begin() + size_t(offset) * size;
    }
    Iterator const &at(SizeType offset, SizeType size) const {
        return data(size).values.begin() + size_t(offset) * size;;
    }
    template <class F>
    void erase(F f) {
        auto eraseBySize = [this,f](SizeType size, Data &d) {
            if (size > 0) {
                auto jt = d.values.begin();
                for (auto it = jt, ie = d.values.end(); it != ie; it+= size) {
                    if (!f(it, it + size)) {
                        if (it != jt) { std::move(it, it+size, jt); }
                        jt += size;
                    }
                }
                d.values.erase(jt, d.values.end());
                d.set.clear();
                for (SizeType i = 0, e = d.values.size() / size; i != e; ++i) {
                    d.set.insert(
                        [this, &d, size](SizeType a) {
                            auto it = d.values.begin() + size_t(a) * size;
                            return hash_range(it, it + size, hasher());
                        },
                        [](SizeType, SizeType) { return false; },
                        i);
                }
            }
            else if (!d.set.empty() && f(d.values.begin(), d.values.begin())) {
                d.set.clear();
            }
        };
        SizeType size = 0;;
        for (auto &d : small_) {
            eraseBySize(size, d);
            ++size;
        }
        for (auto it = big_.begin(), ie = big_.end(); it != ie; ++it) {
            eraseBySize(it->first, it->second);
        }
        big_.erase([](typename BigMap::ValueType const &d) { return d.second.set.empty(); });
    }
    void clear() {
        for (auto &d : small_) {
            d.values.clear();
            d.set.clear();
        }
        big_.clear();
    }
    bool empty() const {
        for (auto &d : small_) {
            if (!d.set.empty()) { return false; }
        }
        for (auto &d : big_) {
            if (!d.second.set.empty()) { return false; }
        }
        return true;

    }
    Hash hasher() const { return *this; }
    EqualTo equalTo() const { return *this; }

private:
    using Set = HashSet<SizeType>;
    struct Data {
        Set set;
        Vec values;
    };
    using SmallVec = std::array<Data, small>;
    using BigMap = UniqueVec<std::pair<SizeType, Data>, HashFirst<SizeType>, EqualToFirst<SizeType>>;
    Data &data(SizeType size) {
        if (size < small) { return small_[size]; }
        else {
            auto ret = big_.find(size);
            if (ret != big_.end()) { return ret->second; }
            else { return big_.push(size, Data()).first->second; }
        }
    }
    Data const &data(SizeType size) const {
        return const_cast<UniqueVecVec*>(this)->data(size);
    }

    BigMap big_;
    SmallVec small_;
};

} // namespace Gringo

#endif // _GRINGO_HASH_SET_HH
