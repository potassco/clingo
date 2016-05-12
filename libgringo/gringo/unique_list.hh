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

#ifndef _GRINGO_UNIQUE_LIST_HH
#define _GRINGO_UNIQUE_LIST_HH

#include <functional>
#include <algorithm>
#include <memory>
#include <cassert>
#include <gringo/utility.hh>

namespace Gringo {

// {{{ declaration of unique_list

template <class Value>
struct unique_list_node {
    template <class... Args>
    explicit unique_list_node(Args&&... args)
        : value(std::forward<Args>(args)...)
        , hash(0)
        , succ(nullptr)
        , prev(nullptr) { }
    using node_type     = unique_list_node;
    using node_ptr_type = std::unique_ptr<node_type>;

    Value             value;
    size_t            hash;
    unique_list_node *succ;
    unique_list_node *prev;
    node_ptr_type     eqSucc;
};

template <class Value>
class unique_list_iterator : public std::iterator<std::bidirectional_iterator_tag, Value, int> {
    using iterator = std::iterator<std::bidirectional_iterator_tag, Value, int>;
public:
    template <class V, class X, class H, class E>
    friend struct unique_list;

    template <class V>
    friend struct unique_list_const_iterator;

    unique_list_iterator() : _node(nullptr) { }
    unique_list_iterator(unique_list_iterator const &x) = default;

    unique_list_iterator& operator=(const unique_list_iterator&) = default;
    bool operator==(const unique_list_iterator &x) const { return _node == x._node; }
    bool operator!=(const unique_list_iterator &x) const { return _node != x._node; }

    unique_list_iterator& operator++() {
        _node = _node->succ;
        return *this;
    }
    unique_list_iterator operator++(int) {
        unique_list_iterator x = *this;
        ++(*this);
        return x;
    }
    unique_list_iterator& operator--() {
        _node = _node->prev;
        return *this;
    }
    unique_list_iterator operator--(int) {
        unique_list_iterator x = *this;
        --(*this);
        return x;
    }

    typename iterator::reference operator*() const { return _node->value; }
    typename iterator::pointer operator->() const  { return &_node->value; }
private:
    using node_type = unique_list_node<Value>;
    unique_list_iterator(node_type *node) : _node(node) { }
    node_type *_node;
};

template <class Value>
class unique_list_const_iterator : public std::iterator<std::bidirectional_iterator_tag, Value const, int> {
    using iterator = std::iterator<std::bidirectional_iterator_tag, Value const, int>;
public:
    template <class V, class X, class H, class E>
    friend struct unique_list;

    unique_list_const_iterator() : _node(nullptr) { }
    unique_list_const_iterator(unique_list_const_iterator const &x) = default;
    unique_list_const_iterator(unique_list_iterator<Value> const &x) : _node(x._node) { }

    unique_list_const_iterator& operator=(const unique_list_const_iterator&) = default;
    bool operator==(const unique_list_const_iterator &x) const { return _node == x._node; }
    bool operator!=(const unique_list_const_iterator &x) const { return _node != x._node; }

    unique_list_const_iterator& operator++() {
        _node = _node->succ;
        return *this;
    }
    unique_list_const_iterator operator++(int) {
        unique_list_const_iterator x = *this;
        ++(*this);
        return x;
    }
    unique_list_const_iterator& operator--() {
        _node = _node->prev;
        return *this;
    }
    unique_list_const_iterator operator--(int) {
        unique_list_const_iterator x = *this;
        --(*this);
        return x;
    }

    typename iterator::reference operator*() const { return _node->value; }
    typename iterator::pointer operator->() const  { return &_node->value; }
private:
    using node_type = unique_list_node<Value>;
    unique_list_const_iterator(node_type *node) : _node(node) { }
    node_type *_node;
};

template <class T>
struct identity {
    using result_type = T;
    template <class U>
    T const &operator()(U const &x) const { return x; }
};

template <class T>
struct extract_first {
    using result_type = T;
    template <class U>
    T const &operator()(U const &x) const { return std::get<0>(x); }
};

template <
    class Value,
    class ExtractKey = identity<Value>,
    class Hasher     = std::hash<typename ExtractKey::result_type>,
    class EqualTo    = std::equal_to<typename ExtractKey::result_type>
>
class unique_list : Hasher, EqualTo, ExtractKey {
    using node_type     = unique_list_node<Value>;
    using node_ptr_type = typename node_type::node_ptr_type;
    using buckets_type  = std::unique_ptr<node_ptr_type[]>;

public:
    using hasher                 = Hasher;
    using key_equal              = EqualTo;
    using key_extract            = ExtractKey;
    using key_type               = typename key_extract::result_type;
    using value_type             = Value;
    using reference              = Value &;
    using const_reference        = Value const &;
    using difference_type        = int;
    using size_type              = unsigned;
    using iterator               = unique_list_iterator<Value>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_iterator         = unique_list_const_iterator<Value>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    unique_list()
        : _size(0)
        , _reserved(0)
        , _front(0)
        , _back(0)
        , _buckets(nullptr) { }
    unique_list(unique_list &&list)
        : hasher(std::move(list))
        , key_equal(std::move(list))
        , _size(list._size)
        , _reserved(list._reserved)
        , _front(list._front)
        , _back(list._back)
        , _buckets(std::move(list._buckets)) { list.clear(); }
    unique_list &operator=(unique_list &&list) {
        static_cast<Hasher&>(*this) = std::move(list);
        static_cast<EqualTo&>(*this) = std::move(list);
        _size = list._size;
        _reserved = list._reserved;
        _front = list._front;
        _back = list._back;
        _buckets = std::move(list._buckets);
        list.clear();
        return *this;
    }
    unsigned reserved() const      { return _reserved; }
    unsigned size() const          { return _size; }
    bool empty() const             { return !_size; }
    double max_load_factor() const { return 0.9; }
    double load_factor() const     { return (size() + 1.0) / reserved(); }

    void clear() {
        _size     = 0;
        _reserved = 0;
        _front    = nullptr;
        _back     = nullptr;
        _buckets.reset(nullptr);
    }
    void reserve(unsigned n) {
        if (_reserved < n) {
            unsigned reserved(_reserved * 1.5);
            if (n < 5 || (reserved <= n)) { reserved = n; }
            else {
                do { reserved *= 1.5; }
                while (reserved < n);
            }
            if (_buckets) {
                buckets_type buckets(new node_ptr_type[reserved]);
                std::swap(reserved, _reserved);
                std::swap(buckets, _buckets);
                for (auto it = buckets.get(), ie = buckets.get() + reserved; it != ie; ++it) {
                    node_ptr_type pos{std::move(*it)};
                    while (pos) {
                        node_ptr_type next{std::move(pos->eqSucc)};
                        move(std::move(pos));
                        pos = std::move(next);
                    }
                }
            }
            else {
                _buckets.reset(new node_ptr_type[reserved]);
                _reserved = reserved;
            }
        }
    }
    template <class... Args>
    std::pair<iterator, bool> emplace_back(Args&&... args) {
        if (load_factor() >= max_load_factor()) { reserve(reserved() + 1); }
        node_ptr_type node(new node_type(std::forward<Args>(args)...));
        node->hash = static_cast<hasher&>(*this)(static_cast<key_extract&>(*this)(node->value));
        return push_back(std::move(node));
    }

    iterator erase(const_iterator x) {
        (x._node->prev ? x._node->prev->succ : _front) = x._node->succ;
        (x._node->succ ? x._node->succ->prev : _back ) = x._node->prev;
        iterator ret(x._node->succ);
        std::reference_wrapper<node_ptr_type> pos{get_bucket(x._node->hash)};
        while (pos.get().get() != x._node) { pos = pos.get()->eqSucc; }
        pos.get() = std::move(pos.get()->eqSucc);
        --_size;
        return ret;
    }
    size_type erase(key_type const &x) {
        auto it(static_cast<unique_list const*>(this)->find(x));
        if (it != end()) {
            erase(it);
            return 1;
        }
        else { return 0; }
    }
    iterator erase(const_iterator a, const_iterator b) {
        if (a != b) {
            (a._node->prev ? a._node->prev->succ : _front) = b._node;
            (b._node       ? b._node->prev       : _back ) = a._node->prev;
            while (a != b) {
                auto c = a++;
                std::reference_wrapper<node_ptr_type> pos{get_bucket(c._node->hash)};
                while (pos.get().get() != c._node) { pos = pos.get()->eqSucc; }
                pos.get() = std::move(pos.get()->eqSucc);
                --_size;
            }
        }
        return b._node;
    }
    void swap(unique_list &list) {
        std::swap(static_cast<hasher&>(*this),      static_cast<hasher&>(list));
        std::swap(static_cast<key_equal&>(*this),   static_cast<key_equal&>(list));
        std::swap(static_cast<key_extract&>(*this), static_cast<key_extract&>(list));
        std::swap(_size,                            list._size);
        std::swap(_reserved,                        list._reserved);
        std::swap(_front,                           list._front);
        std::swap(_back,                            list._back);
        std::swap(_buckets,                         list._buckets);
    }

    iterator find(key_type const &x) {
        return static_cast<unique_list const*>(this)->find(x)._node;
    }
    const_iterator find(key_type const &x) const {
        if (!empty()) {
            std::reference_wrapper<node_ptr_type> pos{get_bucket((this->*(&Hasher::operator()))(x))};
            while (pos.get()) {
                if (static_cast<key_equal const &>(*this)(
                    static_cast<key_extract const &>(*this)(pos.get()->value),
                    x
                )) { return pos.get().get(); }
                pos = pos.get()->eqSucc;
            }
        }
        return end();
    }
    const_iterator begin() const          { return const_iterator(_front); }
    const_iterator end() const            { return const_iterator(); }
    iterator begin()                      { return iterator(_front); }
    iterator end()                        { return iterator(); }
    reverse_iterator rbegin()             { return reverse_iterator(end()); }
    reverse_iterator rend()               { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()   const { return const_reverse_iterator(begin()); }
    value_type const &front() const       { return _front->value; }
    value_type const &back()  const       { return _back->value; }
    value_type &front()                   { return _front->value; }
    value_type &back()                    { return _back->value; }

private:
    std::pair<iterator, bool> push_back(node_ptr_type&& node) {
        std::reference_wrapper<node_ptr_type> pos{get_bucket(node)};
        while (pos.get()) {
            if (static_cast<key_equal&>(*this)(
                static_cast<key_extract&>(*this)(pos.get()->value),
                static_cast<key_extract&>(*this)(node->value))) { return {iterator(pos.get().get()), false}; }
            pos = pos.get()->eqSucc;
        }
        pos.get() = std::move(node);
        ++_size;
        if (_back) {
            pos.get()->prev = _back;
            _back->succ    = pos.get().get();
        }
        else { _front = pos.get().get(); }
        _back = pos.get().get();
        return {iterator(pos.get().get()), true};
    }
    node_ptr_type& get_bucket(size_t hash) const {
        assert(_reserved);
        return _buckets[hash_mix(hash) % _reserved];
    }
    node_ptr_type& get_bucket(node_ptr_type const &node) const {
        assert(_reserved);
        return get_bucket(node->hash);
    }
    void move(node_ptr_type&& node) {
        node_ptr_type& ret = get_bucket(node);
        node->eqSucc = std::move(ret);
        ret = std::move(node);
    }

    unsigned     _size;
    unsigned     _reserved;
    node_type   *_front;
    node_type   *_back;
    buckets_type _buckets;
};

// }}}

} // namespace Gringo

#endif // _GRINGO_UNIQUE_LIST_HH
