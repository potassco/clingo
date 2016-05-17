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

#ifndef _GRINGO_FLYWEIGHT_HH
#define _GRINGO_FLYWEIGHT_HH

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <functional>
#include <utility>
#include <iterator>
#include <cassert>
#include <stdexcept>
#include <array>
#include <gringo/utility.hh>
#include <gringo/hash_set.hh>

#include <iostream>

namespace Gringo {

// {{{ declaration of Flyweight

template <class T>
class Flyweight {
private:
    using ValueMap = UniqueVec<T>;
public:
    using SizeType = typename ValueMap::SizeType;
    using ValueType = T;

    Flyweight(ValueType &&val);
    Flyweight(SizeType uid);
    template<typename... Args>
    Flyweight(Args... args);

    static SizeType uid(ValueType &&val);
    SizeType uid() const;
    ValueType &operator*() const;
    ValueType *operator->() const;
    operator ValueType &() const;

    bool operator==(Flyweight const &other) const;
    bool operator!=(Flyweight const &other) const;
    bool operator<(Flyweight const &other) const;
    bool operator>(Flyweight const &other) const;
    bool operator<=(Flyweight const &other) const;
    bool operator>=(Flyweight const &other) const;

    template <typename F>
    static void erase(F f);
    static void clear();

private:
    static ValueMap values_;

    SizeType uid_;
};

// }}}
// {{{ declaration of FlyweightVec


template <class T>
class FlyweightVec {
private:
    using ValueMap = UniqueVecVec<32, T>;
public:
    using SizeType = typename ValueMap::SizeType;
    using ValueType = T;
    using ValueVec = std::vector<ValueType>;
    using ConstIterator = typename ValueVec::const_iterator;

    static class FromOffset { } fromOffset;

    FlyweightVec(ValueVec const &val);
    FlyweightVec(std::initializer_list<ValueType> val);
    FlyweightVec(FromOffset, SizeType size, SizeType offset);

    ValueType const &operator[](SizeType pos) const;
    ValueType const &front() const;
    ValueType const &back() const;
    ValueType const &at(SizeType pos) const;
    ConstIterator begin()const;
    ConstIterator end() const;
    SizeType size() const;
    bool empty() const;
    SizeType offset() const;
    bool operator==(FlyweightVec const &x) const;
    bool operator<(FlyweightVec const &x) const;

    template <typename F>
    static void erase(F f);
    static void clear();

private:
    template <class C>
    SizeType init(C const &val);

    static ValueMap values_;

    SizeType size_;
    SizeType offset_;
};

template <class T>
typename FlyweightVec<T>::FromOffset FlyweightVec<T>::fromOffset;

// }}}

// {{{ defintion of Flyweight<T>

template <class T>
Flyweight<T>::Flyweight(ValueType &&val)
    : uid_(Flyweight::uid(std::move(val))) { }

template <class T>
typename Flyweight<T>::SizeType Flyweight<T>::uid(ValueType &&val) {
    auto ret = values_.push(std::move(val)).first;
    return ret - values_.begin();
}


template <class T>
Flyweight<T>::Flyweight(SizeType uid)
    : uid_(uid) { }

template <class T>
template<typename... Args>
Flyweight<T>::Flyweight(Args... args)
    : Flyweight(ValueType(args...)) { }

template <class T>
typename Flyweight<T>::SizeType Flyweight<T>::uid() const {
    return uid_;
}

template <class T>
bool Flyweight<T>::operator==(Flyweight const &other) const {
    return uid_ == other.uid_;
}

template <class T>
bool Flyweight<T>::operator!=(Flyweight const &other) const {
    return uid_ != other.uid_;
}

template <class T>
bool Flyweight<T>::operator<(Flyweight const &other) const {
    return **this < *other;
}

template <class T>
bool Flyweight<T>::operator>(Flyweight const &other) const {
    return **this > *other;
}

template <class T>
bool Flyweight<T>::operator<=(Flyweight const &other) const {
    return **this <= *other;
}

template <class T>
bool Flyweight<T>::operator>=(Flyweight const &other) const {
    return **this >= *other;
}

template <class T>
typename Flyweight<T>::ValueType &Flyweight<T>::operator*() const {
    return values_[uid_];
}

template <class T>
typename Flyweight<T>::ValueType *Flyweight<T>::operator->() const {
    return &values_[uid_];
}

template <class T>
Flyweight<T>::operator typename Flyweight<T>::ValueType &() const {
    return values_[uid_];
}

template <class T>
template <class F>
void Flyweight<T>::erase(F f) {
    values_.erase(f);
}

template <class T>
void Flyweight<T>::clear() {
    values_.clear();
}

template <class T>
typename Flyweight<T>::ValueMap Flyweight<T>::values_;

// }}}
// {{{ defintion of FlyweightVec<T>

template <class T>
FlyweightVec<T>::FlyweightVec(ValueVec const &val)
    : size_(val.size())
    , offset_(init(val)) { }

template <class T>
FlyweightVec<T>::FlyweightVec(std::initializer_list<ValueType> val)
    : size_(val.size())
    , offset_(init(val)) { }

template <class T>
template <class C>
typename FlyweightVec<T>::SizeType FlyweightVec<T>::init(C const &val) {
    return values_.push(val).first;
}

template <class T>
FlyweightVec<T>::FlyweightVec(FromOffset, SizeType size, SizeType offset)
    : size_(size)
    , offset_(offset) { }

template <class T>
typename FlyweightVec<T>::ValueType const &FlyweightVec<T>::operator[](SizeType pos) const {
    assert(pos < size_);
    return at(pos);
}

template <class T>
typename FlyweightVec<T>::ValueType const &FlyweightVec<T>::front() const { return *begin(); }

template <class T>
typename FlyweightVec<T>::ValueType const &FlyweightVec<T>::back() const  { return *(end() - 1); }

template <class T>
typename FlyweightVec<T>::ValueType const &FlyweightVec<T>::at(SizeType pos) const {
    return *(begin() + pos);
}

template <class T>
typename FlyweightVec<T>::ConstIterator FlyweightVec<T>::begin() const {
    return values_.at(offset_, size_);
}

template <class T>
typename FlyweightVec<T>::ConstIterator FlyweightVec<T>::end() const {
    return begin() + size_;
}

template <class T>
typename FlyweightVec<T>::SizeType FlyweightVec<T>::size() const {
    return size_;
}

template <class T>
bool FlyweightVec<T>::empty() const {
    return size_ == 0;
}

template <class T>
typename FlyweightVec<T>::SizeType FlyweightVec<T>::offset() const {
    return offset_;
}

template <class T>
bool FlyweightVec<T>::operator==(FlyweightVec const &x) const {
    return offset_ == x.offset_ && size_ == x.size_;
}

template <class T>
bool FlyweightVec<T>::operator<(FlyweightVec const &x) const {
    if (size() != x.size()) { return size() < x.size(); }
    return std::lexicographical_compare(begin(), end(), x.begin(), x.end());
}

template <class T>
template <class F>
void FlyweightVec<T>::erase(F f) {
    values_.erase(f);
}

template <class T>
void FlyweightVec<T>::clear() {
    values_.clear();
}

template <class T>
typename FlyweightVec<T>::ValueMap FlyweightVec<T>::values_;

// }}}

} // namespace Gringo

namespace std {

// {{{ definition of hash functions

template <class T>
struct hash<Gringo::Flyweight<T>> {
    size_t operator()(Gringo::Flyweight<T> const &f) const {
        return f.uid();
    }
};

template <class T>
struct hash<Gringo::FlyweightVec<T>> {
    size_t operator()(Gringo::FlyweightVec<T> const &f) const {
        size_t seed = typeid(Gringo::FlyweightVec<T>).hash_code();
        for (auto &x : f) { Gringo::hash_combine(seed, std::hash<T>()(x)); }
        return seed;
    }
};

// }}}

} // namespace std

#endif  // _GRINGO_FLYWEIGHT_HH
