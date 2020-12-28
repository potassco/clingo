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

#ifndef CLINGO_HH
#define CLINGO_HH

#include <clingo.h>
#include <string>
#include <cstring>
#include <functional>
#include <ostream>
#include <algorithm>
#include <vector>
#include <cassert>
#include <type_traits>
#include <memory>
#include <tuple>
#include <forward_list>
#include <atomic>

#include <iostream>

namespace Clingo {

// {{{1 variant

namespace Detail {

template <class T, class... U>
struct TypeInList : std::false_type { };

template <class T, class... U>
struct TypeInList<T, T, U...> : std::true_type { };

template <class T, class V, class... U>
struct TypeInList<T, V, U...> : TypeInList<T, U...> { };

template <unsigned, class... U>
struct VariantHolder;

template <unsigned n>
struct VariantHolder<n> {
    bool check_type() const { return type_ == 0; }
    void emplace() { }
    void emplace2() { }
    void copy(VariantHolder const &) { }
    void destroy() {
        type_ = 0;
        data_ = nullptr;
    }
    void print(std::ostream &) const { }
    void swap(VariantHolder &other) {
        std::swap(type_, other.type_);
        std::swap(data_, other.data_);
    }
    unsigned type_ = 0;
    void *data_ = nullptr;
};

template <unsigned n, class T, class... U>
struct VariantHolder<n, T, U...> : VariantHolder<n+1, U...>{
    using Helper = VariantHolder<n+1, U...>;
    using Helper::check_type;
    using Helper::emplace;
    using Helper::emplace2;
    using Helper::data_;
    using Helper::type_;
    bool check_type(T *) const { return type_ == n; }
    template <class... Args>
    void emplace(T *, Args&& ...x) {
        data_ = new T{std::forward<Args>(x)...};
        type_ = n;
    }
    // NOTE: http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1467
    template <class... Args>
    void emplace2(T *, Args&& ...x) {
        data_ = new T(std::forward<Args>(x)...);
        type_ = n;
    }
    void copy(VariantHolder const &src) {
        if (src.type_ == n) {
            data_ = new T(*static_cast<T const*>(src.data_));
            type_ = src.type_;
        }
        Helper::copy(src);
    }
    // NOTE: workaround for visual studio (C++14 can also simply use auto)
#   define CLINGO_VARIANT_RETURN(Type) decltype(std::declval<V>().visit(std::declval<Type&>(), std::declval<Args>()...))
    template <class V, class... Args>
    using Ret_ = CLINGO_VARIANT_RETURN(T);
    template <class V, class... Args>
    using ConstRet_ = CLINGO_VARIANT_RETURN(T const);
    // non-const
    template <class V, class U1, class... U2, class... Args>
    auto accept_(V &&visitor, Args &&... args) -> CLINGO_VARIANT_RETURN(T) {
        static_assert(std::is_same<Ret_<V, Args...>, typename Helper::template Ret_<V, Args...>>::value, "");
        return n == type_
            ? visitor.visit(*static_cast<T*>(data_), std::forward<Args>(args)...)
            : Helper::template accept<V>(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
    template <class V, class... Args>
    auto accept_(V &&visitor, Args &&... args) -> CLINGO_VARIANT_RETURN(T) {
        assert(n == type_);
        return visitor.visit(*static_cast<T*>(data_), std::forward<Args>(args)...);
    }
    template <class V, class... Args>
    auto accept(V &&visitor, Args &&... args) -> CLINGO_VARIANT_RETURN(T) {
        return accept_<V, U...>(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
    // const
    template <class V, class U1, class... U2, class... Args>
    auto accept_(V &&visitor, Args &&... args) const -> CLINGO_VARIANT_RETURN(T const) {
        static_assert(std::is_same<ConstRet_<V, Args...>, typename Helper::template ConstRet_<V, Args...>>::value, "");
        return n == type_
            ? visitor.visit(*static_cast<T const *>(data_), std::forward<Args>(args)...)
            : Helper::template accept<V>(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
    template <class V, class... Args>
    auto accept_(V &&visitor, Args &&... args) const -> CLINGO_VARIANT_RETURN(T const) {
        assert(n == type_);
        return visitor.visit(*static_cast<T const *>(data_), std::forward<Args>(args)...);
    }
    template <class V, class... Args>
    auto accept(V &&visitor, Args &&... args) const -> CLINGO_VARIANT_RETURN(T const) {
        return accept_<V, U...>(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
#   undef CLINGO_VARIANT_RETURN
    void destroy() {
        if (n == type_) { delete static_cast<T*>(data_); }
        Helper::destroy();
    }
    void print(std::ostream &out) const {
        if (n == type_) { out << *static_cast<T const*>(data_); }
        Helper::print(out);
    }
};

} // Detail

template <class T>
class Optional {
public:
    Optional() = default;
    ~Optional() = default;
    Optional(T const &x) : data_(new T(x)) { }
    Optional(T &x) : data_(new T(x)) { }
    Optional(T &&x) noexcept : data_(new T(std::move(x))) { }
    template <class... Args>
    Optional(Args&&... x) : data_(new T{std::forward<Args>(x)...}) { }
    Optional(Optional &&opt) noexcept : data_(opt.data_.release()) { }
    Optional(Optional &opt) : data_(opt ? new T(*opt) : nullptr) { }
    Optional(Optional const &opt) : data_(opt ? new T(*opt) : nullptr) { }
    Optional &operator=(T const &x) {
        clear();
        data_.reset(new T(x));
        return *this;
    }
    Optional &operator=(T &x) {
        clear();
        data_.reset(new T(x));
        return *this;
    }
    Optional &operator=(T &&x) {
        clear();
        data_.reset(new T(std::move(x)));
        return *this;
    }
    Optional &operator=(Optional &&opt) noexcept {
        data_ = std::move(opt.data_);
        return *this;
    }
    Optional &operator=(Optional const &opt) {
        clear();
        data_.reset(opt ? new T(*opt) : nullptr);
        return *this;
    }
    T *get() { return data_.get(); }
    T const *get() const { return data_.get(); }
    T *operator->() { return get(); }
    T const *operator->() const { return get(); }
    T &operator*() & { return *get(); }
    T const &operator*() const & { return *get(); }
    T &&operator*() && { return std::move(*get()); }
    T const &&operator*() const && { return std::move(*get()); }
    template <class... Args>
    void emplace(Args&&... x) {
        clear();
        data_(new T{std::forward<Args>(x)...});
    }
    void clear() { data_.reset(nullptr); }
    explicit operator bool() const { return data_ != nullptr; }
private:
    std::unique_ptr<T> data_;
};

template <class... T>
class Variant {
    using Holder = Detail::VariantHolder<1, T...>;
public:
    Variant(Variant const &other) : Variant(other.data_) { }
    Variant(Variant &&other) noexcept { data_.swap(other.data_); }
    template <class U>
    Variant(U &&u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace2<U>(std::forward<U>(u)); }
    template <class U>
    Variant(U &u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace2<U>(u); }
    template <class U>
    Variant(U const &u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace2<U>(u); }
    template <class U, class... Args>
    static Variant make(Args&& ...args) {
        Variant<T...> x;
        x.data_.emplace(static_cast<U*>(nullptr), std::forward<Args>(args)...);
        return x;
    }
    ~Variant() { data_.destroy(); }
    Variant &operator=(Variant const &other) { return *this = other.data_; }
    Variant &operator=(Variant &&other) noexcept { return *this = std::move(other.data_); }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U &&u) {
        emplace2<U>(std::forward<U>(u));
        return *this;
    }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U &u) {
        emplace2<U>(u);
        return *this;
    }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U const &u) {
        emplace2<U>(u);
        return *this;
    }
    template <class U>
    U &get() {
        if (!data_.check_type(static_cast<U*>(nullptr))) { throw std::bad_cast(); }
        return *static_cast<U*>(data_.data_);
    }
    template <class U>
    U const &get() const {
        if (!data_.check_type(static_cast<U*>(nullptr))) { throw std::bad_cast(); }
        return *static_cast<U*>(data_.data_);
    }
    template <class U, class... Args>
    void emplace(Args&& ...args) {
        Variant<T...> x;
        x.data_.emplace(static_cast<U*>(nullptr), std::forward<Args>(args)...);
        data_.swap(x.data_);
    }
    template <class U>
    bool is() const { return data_.check_type(static_cast<U*>(nullptr)); }
    void swap(Variant &other) { data_.swap(other.data_); }
    template <class V, class... Args>
    typename Holder::template Ret_<V, Args...> accept(V &&visitor, Args &&... args) {
        return data_.accept(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
    template <class V, class... Args>
    typename Holder::template ConstRet_<V, Args...> accept(V &&visitor, Args &&... args) const {
        return data_.accept(std::forward<V>(visitor), std::forward<Args>(args)...);
    }
    friend std::ostream &operator<<(std::ostream &out, Variant const &x) {
        x.data_.print(out);
        return out;
    }

private:
    Variant() = default;
    Variant(Holder const &data) {
        data_.copy(data);
    }
    Variant &operator=(Holder const &data) {
        Variant x(data);
        data_.swap(x.data_);
        return *this;
    }
    Variant &operator=(Holder &&data) noexcept {
        Holder x;
        x.swap(data);
        // Destroy the old data_ only after securing the new data
        // Otherwise, data would be destroyed together with data_ if it was a descendant of data_
        data_.destroy();
        x.swap(data_);
        return *this;
    }
    template <class U, class... Args>
    void emplace2(Args&& ...args) {
        Variant<T...> x;
        x.data_.emplace2(static_cast<U*>(nullptr), std::forward<Args>(args)...);
        data_.swap(x.data_);
    }

    Holder data_;
};

// {{{1 span

template <class Iterator>
class IteratorRange {
public:
    using reference = typename Iterator::reference;
    using difference_type = typename Iterator::difference_type;
    IteratorRange(Iterator begin, Iterator end)
    : begin_(begin)
    , end_(end) { }
    reference operator[](difference_type n) {
        auto it = begin_;
        std::advance(it, n);
        return *it;
    }
    difference_type size() { return std::distance(begin_, end_); }
    bool empty() { return begin_ == end_; }
    Iterator begin() { return begin_; }
    Iterator end() { return end_; }
private:
    Iterator begin_;
    Iterator end_;
};

template <class Iterator>
IteratorRange<Iterator> make_range(Iterator ib, Iterator ie) {
    return {ib, ie};
}

template <class T>
class ValuePointer {
public:
    ValuePointer(T value) : value_(value) { }
    T &operator*() { return value_; }
    T *operator->() { return &value_; }
private:
    T value_;
};

template <class T, class A=T*, class P=ValuePointer<T>>
class ArrayIterator : public std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, P, T> {
public:
    using base = std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, P, T>;
    using difference_type = typename base::difference_type;
    using reference = typename base::reference;
    using pointer = typename base::pointer;
    explicit ArrayIterator(A arr, size_t index = 0)
    : arr_(std::move(arr))
    , index_(index) { }
    ArrayIterator& operator++() { ++index_; return *this; }
    ArrayIterator operator++(int) {
        ArrayIterator t(*this);
        ++*this;
        return t;
    }
    ArrayIterator& operator--() { --index_; return *this; }
    ArrayIterator operator--(int) {
        ArrayIterator t(*this);
        --*this;
        return t;
    }
    ArrayIterator& operator+=(difference_type n) { index_ += n; return *this; }
    ArrayIterator& operator-=(difference_type n) { index_ -= n; return *this; }
    friend ArrayIterator operator+(ArrayIterator it, difference_type n) { return ArrayIterator{it.arr_, it.index_ + n}; }
    friend ArrayIterator operator+(difference_type n, ArrayIterator it) { return ArrayIterator{it.arr_, it.index_ + n}; }
    friend ArrayIterator operator-(ArrayIterator it, difference_type n) { return ArrayIterator{it.arr_, it.index_ - n}; }
    friend difference_type operator-(ArrayIterator a, ArrayIterator b)  { return a.index_ - b.index_; }
    reference operator*() { return arr_->at(index_); }
    pointer operator->() { return arr_->at(index_); }
    friend void swap(ArrayIterator& lhs, ArrayIterator& rhs) {
        std::swap(lhs.arr_, rhs.arr_);
        std::swap(lhs.index_, rhs.index_);
    }
    friend bool operator==(ArrayIterator lhs, ArrayIterator rhs) { return lhs.index_ == rhs.index_; }
    friend bool operator!=(ArrayIterator lhs, ArrayIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (ArrayIterator lhs, ArrayIterator rhs) { return (lhs.index_ + 1) < (rhs.index_ + 1); }
    friend bool operator> (ArrayIterator lhs, ArrayIterator rhs) { return rhs < lhs; }
    friend bool operator<=(ArrayIterator lhs, ArrayIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(ArrayIterator lhs, ArrayIterator rhs) { return !(lhs < rhs); }
private:
    A arr_;
    size_t index_;
};

template <class T>
struct ToIterator {
    T const *operator()(T const *x) const { return x; }
};

template <class T, class I = ToIterator<T>>
class Span : private I {
public:
    using IteratorType = typename std::result_of<I(T const *)>::type;
    using ReferenceType = decltype(*std::declval<IteratorType>());
    Span(I to_it = I())
    : Span(nullptr, size_t(0), to_it) { }
    template <class U>
    Span(U const *begin, size_t size)
    : Span(static_cast<T const *>(begin), size) { }
    Span(T const *begin, size_t size, I to_it = I())
    : Span(begin, begin + size, to_it) { }
    Span(std::initializer_list<T> c, I to_it = I())
    : Span(c.size() > 0 ? &*c.begin() : nullptr, c.size(), to_it) { }
    template <class U>
    Span(U const &c, I to_it = I())
    : Span(c.size() > 0 ? &*c.begin() : nullptr, c.size(), to_it) { }
    Span(T const *begin, T const *end, I to_it = I())
    : I(to_it)
    , begin_(begin)
    , end_(end) { }
    IteratorType begin() const { return I::operator()(begin_); }
    IteratorType end() const { return I::operator()(end_); }
    ReferenceType operator[](size_t offset) const { return *(begin() + offset); }
    ReferenceType front() const { return *begin(); }
    ReferenceType back() const { return *I::operator()(end_-1); }
    size_t size() const { return end_ - begin_; }
    bool empty() const { return begin_ == end_; }
private:
    T const *begin_;
    T const *end_;
};

template <class T, class I = ToIterator<T>>
inline Span<T, I> make_span(T const *begin, size_t size, I to_it = I()) {
    return {begin, size, std::move(to_it)};
}

template <class T, class U>
bool equal_range(T const &a, U const &b) {
    using namespace std;
    return a.size() == b.size() && std::equal(begin(a), end(a), begin(b));
}

template <class T, class I, class V>
bool operator==(Span<T, I> span, V const &v) { return equal_range(span, v); }
template <class T, class I, class V>
bool operator==(V const &v, Span<T, I> span) { return equal_range(span, v); }

template <class T, class I>
std::ostream &operator<<(std::ostream &out, Span<T, I> span) {
    out << "{";
    bool comma = false;
    for (auto &x : span) {
        if (comma) { out << ", "; }
        else { out << " "; }
        out << x;
        comma = true;
    }
    out << " }";
    return out;
}

// {{{1 basic types

// consider using upper case
using literal_t = clingo_literal_t;
using id_t = clingo_id_t;
using weight_t = clingo_weight_t;
using atom_t = clingo_atom_t;

class WeightedLiteral {
public:
    WeightedLiteral(clingo_literal_t lit, clingo_weight_t weight)
    : wlit_{lit, weight} { }
    explicit WeightedLiteral(clingo_weighted_literal_t wlit)
    : wlit_(wlit) { }
    literal_t literal() const { return wlit_.literal; }
    weight_t weight() const { return wlit_.weight; }
    clingo_weighted_literal_t const &to_c() const { return wlit_; }
    clingo_weighted_literal_t &to_c() { return wlit_; }
private:
    clingo_weighted_literal_t wlit_;
};
using WeightedLiteralSpan = Span<WeightedLiteral>;

enum class TruthValue {
    Free  = clingo_truth_value_free,
    True  = clingo_truth_value_true,
    False = clingo_truth_value_false
};

inline std::ostream &operator<<(std::ostream &out, TruthValue tv) {
    switch (tv) {
        case TruthValue::Free:  { out << "Free"; break; }
        case TruthValue::True:  { out << "True"; break; }
        case TruthValue::False: { out << "False"; break; }
    }
    return out;
}

// {{{1 signature

class Signature {
public:
    explicit Signature(clingo_signature_t sig)
    : sig_(sig) { }
    Signature(char const *name, uint32_t arity, bool positive = true);
    char const *name() const;
    uint32_t arity() const;
    bool positive() const;
    bool negative() const;
    size_t hash() const;

    clingo_signature_t const &to_c() const { return sig_; }
    clingo_signature_t &to_c() { return sig_; }
private:
    clingo_signature_t sig_;
};

inline std::ostream &operator<<(std::ostream &out, Signature sig) {
    out << (sig.negative() ? "-" : "") << sig.name() << "/" << sig.arity();
    return out;
}
bool operator==(Signature a, Signature b);
bool operator!=(Signature a, Signature b);
bool operator< (Signature a, Signature b);
bool operator<=(Signature a, Signature b);
bool operator> (Signature a, Signature b);
bool operator>=(Signature a, Signature b);

} namespace std {

template<>
struct hash<Clingo::Signature> {
    size_t operator()(Clingo::Signature sig) const { return sig.hash(); }
};

} namespace Clingo {

// {{{1 symbol

enum class SymbolType : clingo_symbol_type_t {
    Infimum  = clingo_symbol_type_infimum,
    Number   = clingo_symbol_type_number,
    String   = clingo_symbol_type_string,
    Function = clingo_symbol_type_function,
    Supremum = clingo_symbol_type_supremum
};

class Symbol;
using SymbolSpan = Span<Symbol>;
using SymbolVector = std::vector<Symbol>;
using SymbolSpanCallback = std::function<void (SymbolSpan)>;

class Symbol {
public:
    Symbol();
    explicit Symbol(clingo_symbol_t);
    int number() const;
    char const *name() const;
    char const *string() const;
    bool is_positive() const;
    bool is_negative() const;
    SymbolSpan arguments() const;
    SymbolType type() const;
    std::string to_string() const;
    bool match(char const *name, unsigned arity) const;
    size_t hash() const;
    clingo_symbol_t &to_c() { return sym_; }
    clingo_symbol_t const &to_c() const { return sym_; }
private:
    clingo_symbol_t sym_;
};

Symbol Number(int num);
Symbol Supremum();
Symbol Infimum();
Symbol String(char const *str);
Symbol Id(char const *id, bool positive = true);
Symbol Function(char const *name, SymbolSpan args, bool positive = true);

std::ostream &operator<<(std::ostream &out, Symbol sym);
bool operator==(Symbol a, Symbol b);
bool operator!=(Symbol a, Symbol b);
bool operator< (Symbol a, Symbol b);
bool operator<=(Symbol a, Symbol b);
bool operator> (Symbol a, Symbol b);
bool operator>=(Symbol a, Symbol b);

} namespace std {

template<>
struct hash<Clingo::Symbol> {
    size_t operator()(Clingo::Symbol sym) const { return sym.hash(); }
};

} namespace Clingo {

// {{{1 symbolic atoms

class SymbolicAtom {
    friend class SymbolicAtomIterator;
public:
    explicit SymbolicAtom(clingo_symbolic_atoms_t const *atoms, clingo_symbolic_atom_iterator_t range)
    : atoms_(atoms)
    , range_(range) { }
    Symbol symbol() const;
    literal_t literal() const;
    bool is_fact() const;
    bool is_external() const;
    bool match(char const *name, unsigned arity) const;
    clingo_symbolic_atom_iterator_t to_c() const { return range_; }
private:
    clingo_symbolic_atoms_t const *atoms_;
    clingo_symbolic_atom_iterator_t range_;
};

class SymbolicAtomIterator : private SymbolicAtom, public std::iterator<std::input_iterator_tag, SymbolicAtom> {
public:
    explicit SymbolicAtomIterator(clingo_symbolic_atoms_t const *atoms, clingo_symbolic_atom_iterator_t range)
    : SymbolicAtom{atoms, range} { }
    SymbolicAtom &operator*() { return *this; }
    SymbolicAtom *operator->() { return this; }
    SymbolicAtomIterator &operator++();
    SymbolicAtomIterator operator++ (int) {
        auto range = range_;
        ++(*this);
        return SymbolicAtomIterator{atoms_, range};
    }
    bool operator==(SymbolicAtomIterator it) const;
    bool operator!=(SymbolicAtomIterator it) const { return !(*this == it); }
    explicit operator bool() const;
    clingo_symbolic_atom_iterator_t to_c() const { return range_; }
};

class SymbolicAtoms {
public:
    explicit SymbolicAtoms(clingo_symbolic_atoms_t const *atoms)
    : atoms_(atoms) { }
    SymbolicAtomIterator begin() const;
    SymbolicAtomIterator begin(Signature sig) const;
    SymbolicAtomIterator end() const;
    SymbolicAtomIterator find(Symbol atom) const;
    std::vector<Signature> signatures() const;
    size_t length() const;
    clingo_symbolic_atoms_t const *to_c() const { return atoms_; }
    SymbolicAtom operator[](Symbol atom) const { return *find(atom); }
private:
    clingo_symbolic_atoms_t const *atoms_;
};

// {{{1 theory atoms

enum class TheoryTermType : clingo_theory_term_type_t {
    Tuple = clingo_theory_term_type_tuple,
    List = clingo_theory_term_type_list,
    Set = clingo_theory_term_type_set,
    Function = clingo_theory_term_type_function,
    Number = clingo_theory_term_type_number,
    Symbol = clingo_theory_term_type_symbol
};

template <class T>
class TheoryIterator : public std::iterator<std::random_access_iterator_tag, T const, ptrdiff_t, T*, T> {
public:
    using base = std::iterator<std::random_access_iterator_tag, T const>;
    using difference_type = typename base::difference_type;
    explicit TheoryIterator(clingo_theory_atoms_t const *atoms, clingo_id_t const* id)
    : elem_(atoms)
    , id_(id) { }
    TheoryIterator& operator++() { ++id_; return *this; }
    TheoryIterator operator++(int) {
        TheoryIterator t(*this);
        ++*this;
        return t;
    }
    TheoryIterator& operator--() { --id_; return *this; }
    TheoryIterator operator--(int) {
        TheoryIterator t(*this);
        --*this;
        return t;
    }
    TheoryIterator& operator+=(difference_type n) { id_ += n; return *this; }
    TheoryIterator& operator-=(difference_type n) { id_ -= n; return *this; }
    friend TheoryIterator operator+(TheoryIterator it, difference_type n) { return TheoryIterator{it.atoms(), it.id_ + n}; }
    friend TheoryIterator operator+(difference_type n, TheoryIterator it) { return TheoryIterator{it.atoms(), it.id_ + n}; }
    friend TheoryIterator operator-(TheoryIterator it, difference_type n) { return TheoryIterator{it.atoms(), it.id_ - n}; }
    friend difference_type operator-(TheoryIterator a, TheoryIterator b)  { return a.id_ - b.id_; }
    T operator*() { return elem_ = *id_; }
    T *operator->() { return &(elem_ = *id_); }
    friend void swap(TheoryIterator& lhs, TheoryIterator& rhs) {
        std::swap(lhs.id_, rhs.id_);
        std::swap(lhs.elem_, rhs.elem_);
    }
    friend bool operator==(TheoryIterator lhs, TheoryIterator rhs) { return lhs.id_ == rhs.id_; }
    friend bool operator!=(TheoryIterator lhs, TheoryIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (TheoryIterator lhs, TheoryIterator rhs) { return lhs.id_ < rhs.id_; }
    friend bool operator> (TheoryIterator lhs, TheoryIterator rhs) { return rhs < lhs; }
    friend bool operator<=(TheoryIterator lhs, TheoryIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(TheoryIterator lhs, TheoryIterator rhs) { return !(lhs < rhs); }

private:
    clingo_theory_atoms_t const *&atoms() { return elem_.atoms_; }

    T                  elem_;
    clingo_id_t const *id_;
};

template <class T>
class ToTheoryIterator {
public:
    explicit ToTheoryIterator(clingo_theory_atoms_t const *atoms)
    : atoms_(atoms) { }
    T operator ()(clingo_id_t const *id) const {
        return T{atoms_, id};
    }
private:
    clingo_theory_atoms_t const *atoms_;
};

class TheoryTerm;
using TheoryTermIterator = TheoryIterator<TheoryTerm>;
using TheoryTermSpan = Span<clingo_id_t, ToTheoryIterator<TheoryTermIterator>>;

class TheoryTerm {
    friend class TheoryIterator<TheoryTerm>;
public:
    explicit TheoryTerm(clingo_theory_atoms_t const *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermType type() const;
    int number() const;
    char const *name() const;
    TheoryTermSpan arguments() const;
    clingo_id_t to_c() const { return id_; }
    std::string to_string() const;
private:
    TheoryTerm(clingo_theory_atoms_t const *atoms)
    : TheoryTerm(atoms, 0) { }
    TheoryTerm &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }

    clingo_theory_atoms_t const *atoms_;
    clingo_id_t id_;
};
std::ostream &operator<<(std::ostream &out, TheoryTerm term);

class TheoryElement;
using TheoryElementIterator = TheoryIterator<TheoryElement>;
using TheoryElementSpan = Span<clingo_id_t, ToTheoryIterator<TheoryElementIterator>>;
using LiteralSpan = Span<literal_t>;

class TheoryElement {
    friend class TheoryIterator<TheoryElement>;
public:
    explicit TheoryElement(clingo_theory_atoms_t const *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermSpan tuple() const;
    LiteralSpan condition() const;
    literal_t condition_id() const;
    std::string to_string() const;
    clingo_id_t to_c() const { return id_; }
private:
    TheoryElement(clingo_theory_atoms_t const *atoms)
    : TheoryElement(atoms, 0) { }
    TheoryElement &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }

    clingo_theory_atoms_t const *atoms_;
    clingo_id_t id_;
};
std::ostream &operator<<(std::ostream &out, TheoryElement term);

class TheoryAtom {
    friend class TheoryAtomIterator;
public:
    explicit TheoryAtom(clingo_theory_atoms_t const *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryElementSpan elements() const;
    TheoryTerm term() const;
    bool has_guard() const;
    literal_t literal() const;
    std::pair<char const *, TheoryTerm> guard() const;
    std::string to_string() const;
    clingo_id_t to_c() const { return id_; }
private:
    TheoryAtom(clingo_theory_atoms_t const *atoms)
    : TheoryAtom(atoms, 0) { }
    TheoryAtom &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }

    clingo_theory_atoms_t const *atoms_;
    clingo_id_t id_;
};
std::ostream &operator<<(std::ostream &out, TheoryAtom term);

class TheoryAtomIterator : private TheoryAtom, public std::iterator<TheoryAtom, std::random_access_iterator_tag, ptrdiff_t, TheoryAtom*, TheoryAtom> {
public:
    explicit TheoryAtomIterator(clingo_theory_atoms_t const *atoms, clingo_id_t id)
    : TheoryAtom{atoms, id} { }
    TheoryAtomIterator& operator++() { ++id_; return *this; }
    TheoryAtomIterator operator++(int) {
        auto t = *this;
        ++*this;
        return t;
    }
    TheoryAtomIterator& operator--() { --id_; return *this; }
    TheoryAtomIterator operator--(int) {
        auto t = *this;
        --*this;
        return t;
    }
    TheoryAtomIterator& operator+=(difference_type n) { id_ += static_cast<clingo_id_t>(n); return *this; }
    TheoryAtomIterator& operator-=(difference_type n) { id_ -= static_cast<clingo_id_t>(n); return *this; }
    friend TheoryAtomIterator operator+(TheoryAtomIterator it, difference_type n) { return TheoryAtomIterator{it.atoms(), clingo_id_t(it.id() + n)}; }
    friend TheoryAtomIterator operator+(difference_type n, TheoryAtomIterator it) { return TheoryAtomIterator{it.atoms(), clingo_id_t(it.id() + n)}; }
    friend TheoryAtomIterator operator-(TheoryAtomIterator it, difference_type n) { return TheoryAtomIterator{it.atoms(), clingo_id_t(it.id() - n)}; }
    friend difference_type operator-(TheoryAtomIterator a, TheoryAtomIterator b)  { return a.id() - b.id(); }
    TheoryAtom operator*() { return *this; }
    TheoryAtom *operator->() { return this; }
    friend void swap(TheoryAtomIterator& lhs, TheoryAtomIterator& rhs) {
        std::swap(lhs.atoms(), rhs.atoms());
        std::swap(lhs.id(), rhs.id());
    }
    friend bool operator==(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return lhs.atoms() == rhs.atoms() && lhs.id() == rhs.id(); }
    friend bool operator!=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (TheoryAtomIterator lhs, TheoryAtomIterator rhs) { assert(lhs.atoms() == rhs.atoms()); return (lhs.id() + 1) < (rhs.id() + 1); }
    friend bool operator> (TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return rhs < lhs; }
    friend bool operator<=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs < rhs); }
private:
    clingo_theory_atoms_t const *&atoms() { return atoms_; }
    clingo_id_t &id() { return id_; }
};

class TheoryAtoms {
public:
    explicit TheoryAtoms(clingo_theory_atoms_t const *atoms)
    : atoms_(atoms) { }
    TheoryAtomIterator begin() const;
    TheoryAtomIterator end() const;
    size_t size() const;
    clingo_theory_atoms_t const *to_c() const { return atoms_; }
private:
    clingo_theory_atoms_t const *atoms_;
};

// {{{1 trail

template <class T, class U, class I>
class IndexIterator : public std::iterator<std::random_access_iterator_tag, typename T::value_type, I, typename T::value_type, typename T::value_type> {
public:
    using value_type = typename T::value_type;
    using base = std::iterator<std::random_access_iterator_tag, value_type, I, value_type*, value_type>;
    using difference_type = typename base::difference_type;
    explicit IndexIterator(T *con = nullptr, U index = 0)
    : con_(con)
    , index_(index) { }
    IndexIterator& operator++() { ++index_; return *this; }
    IndexIterator operator++(int) {
        IndexIterator t(*this);
        ++*this;
        return t;
    }
    IndexIterator& operator--() { --index_; return *this; }
    IndexIterator operator--(int) {
        IndexIterator t(*this);
        --*this;
        return t;
    }
    IndexIterator& operator+=(difference_type n) { index_ += n; return *this; }
    IndexIterator& operator-=(difference_type n) { index_ -= n; return *this; }
    friend IndexIterator operator+(IndexIterator it, difference_type n) { return IndexIterator{it.con_, it.index_ + n}; }
    friend IndexIterator operator+(difference_type n, IndexIterator it) { return IndexIterator{it.con_, it.index_ + n}; }
    friend IndexIterator operator-(IndexIterator it, difference_type n) { return IndexIterator{it.con_, it.index_ - n}; }
    friend difference_type operator-(IndexIterator a, IndexIterator b)  { return a.index_ - b.index_; }
    value_type operator*() { return con_->at(index_); }
    friend void swap(IndexIterator& lhs, IndexIterator& rhs) {
        std::swap(lhs.con_, rhs.con_);
        std::swap(lhs.index_, rhs.index_);
    }
    friend bool operator==(IndexIterator lhs, IndexIterator rhs) { return lhs.index_ == rhs.index_; }
    friend bool operator!=(IndexIterator lhs, IndexIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (IndexIterator lhs, IndexIterator rhs) { return lhs.index_ + 1 < rhs.index_ + 1; }
    friend bool operator> (IndexIterator lhs, IndexIterator rhs) { return rhs < lhs; }
    friend bool operator<=(IndexIterator lhs, IndexIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(IndexIterator lhs, IndexIterator rhs) { return !(lhs < rhs); }
private:
    T *con_;
    U index_;
};

class Trail {
public:
    using iterator = IndexIterator<Trail const, uint32_t, int32_t>;
    using value_type = literal_t;
    explicit Trail(clingo_assignment_t const *ass) : ass_{ass} { }
    uint32_t size() const;
    uint32_t begin_offset(uint32_t level) const;
    uint32_t end_offset(uint32_t level) const;
    iterator begin() const { return iterator{this, 0}; }
    iterator begin(uint32_t level) const { return iterator{this, begin_offset(level)}; }
    iterator end() const { return iterator{this, size()}; }
    iterator end(uint32_t level) const { return iterator{this, end_offset(level)}; }
    literal_t at(uint32_t offset) const;
    literal_t operator[](uint32_t offset) const { return at(offset); }
    clingo_assignment_t const *to_c() const { return ass_; }
private:
    clingo_assignment_t const *ass_;
};

// {{{1 assignment

class Assignment {
public:
    using iterator = IndexIterator<Assignment const, size_t, std::make_signed<size_t>::type>;
    using value_type = literal_t;
    explicit Assignment(clingo_assignment_t const *ass)
    : ass_(ass) { }
    bool has_conflict() const;
    uint32_t decision_level() const;
    uint32_t root_level() const;
    bool has_literal(literal_t lit) const;
    TruthValue truth_value(literal_t lit) const;
    uint32_t level(literal_t lit) const;
    literal_t decision(uint32_t level) const;
    bool is_fixed(literal_t lit) const;
    bool is_true(literal_t lit) const;
    bool is_false(literal_t lit) const;
    size_t size() const;
    bool is_total() const;
    literal_t at(size_t offset) const;
    literal_t operator[](size_t offset) const { return at(offset); }
    iterator begin() const { return iterator{this, 0}; }
    iterator end() const { return iterator{this, size()}; }
    Trail trail() const { return Trail{ass_}; }
    clingo_assignment_t const *to_c() const { return ass_; }
private:
    clingo_assignment_t const *ass_;
};

// {{{1 propagate init

enum PropagatorCheckMode : clingo_propagator_check_mode_t {
    None    = clingo_propagator_check_mode_none,
    Total   = clingo_propagator_check_mode_total,
    Partial = clingo_propagator_check_mode_fixpoint,
    Both    = clingo_propagator_check_mode_both,
};

enum WeightConstraintType : clingo_weight_constraint_type_t {
    LeftImplication  = clingo_weight_constraint_type_implication_left,
    RightImplication = clingo_weight_constraint_type_implication_right,
    Equivalence      = clingo_weight_constraint_type_equivalence,
};

class PropagateInit {
public:
    explicit PropagateInit(clingo_propagate_init_t *init)
    : init_(init) { }
    literal_t solver_literal(literal_t lit) const;
    void add_watch(literal_t lit);
    void add_watch(literal_t literal, id_t thread_id);
    int number_of_threads() const;
    Assignment assignment() const;
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    PropagatorCheckMode get_check_mode() const;
    void set_check_mode(PropagatorCheckMode mode);
    literal_t add_literal(bool freeze = true);
    bool add_clause(LiteralSpan clause);
    bool add_weight_constraint(literal_t literal, WeightedLiteralSpan literals, weight_t bound, WeightConstraintType type, bool compare_equal = false);
    void add_minimize(literal_t literal, weight_t weight, weight_t priority = 0);
    bool propagate();
    clingo_propagate_init_t *to_c() const { return init_; }
private:
    clingo_propagate_init_t *init_;
};

// {{{1 propagate control

enum class ClauseType : clingo_clause_type_t {
    Learnt = clingo_clause_type_learnt,
    Static = clingo_clause_type_static,
    Volatile = clingo_clause_type_volatile,
    VolatileStatic = clingo_clause_type_volatile_static
};

inline std::ostream &operator<<(std::ostream &out, ClauseType t) {
    switch (t) {
        case ClauseType::Learnt:         { out << "Learnt"; break; }
        case ClauseType::Static:         { out << "Static"; break; }
        case ClauseType::Volatile:       { out << "Volatile"; break; }
        case ClauseType::VolatileStatic: { out << "VolatileStatic"; break; }
    }
    return out;
}

class PropagateControl {
public:
    explicit PropagateControl(clingo_propagate_control_t *ctl)
    : ctl_(ctl) { }
    id_t thread_id() const;
    Assignment assignment() const;
    literal_t add_literal();
    void add_watch(literal_t literal);
    bool has_watch(literal_t literal) const;
    void remove_watch(literal_t literal);
    bool add_clause(LiteralSpan clause, ClauseType type = ClauseType::Learnt);
    bool propagate();
    clingo_propagate_control_t *to_c() const { return ctl_; }
private:
    clingo_propagate_control_t *ctl_;
};

// {{{1 propagator

class Propagator {
public:
    Propagator() = default;
    Propagator(Propagator const &) = default;
    Propagator(Propagator &&) noexcept = default;
    Propagator &operator=(Propagator const &) = default;
    Propagator &operator=(Propagator &&) noexcept = default;
    virtual void init(PropagateInit &init);
    virtual void propagate(PropagateControl &ctl, LiteralSpan changes);
    virtual void undo(PropagateControl const &ctl, LiteralSpan changes) noexcept;
    virtual void check(PropagateControl &ctl);
    virtual ~Propagator() = default;
};

class Heuristic : public Propagator {
public:
    Heuristic() = default;
    Heuristic(Heuristic const &) = default;
    Heuristic(Heuristic &&) noexcept = default;
    Heuristic &operator=(Heuristic const &) = default;
    Heuristic &operator=(Heuristic &&) noexcept = default;
    virtual literal_t decide(id_t thread_id, Assignment const &assign, literal_t fallback);
    ~Heuristic() override = default;
};

// {{{1 ground program observer

using IdSpan = Span<id_t>;
using AtomSpan = Span<atom_t>;

enum class HeuristicType : clingo_heuristic_type_t {
    Level  = clingo_heuristic_type_level,
    Sign   = clingo_heuristic_type_sign,
    Factor = clingo_heuristic_type_factor,
    Init   = clingo_heuristic_type_init,
    True   = clingo_heuristic_type_true,
    False  = clingo_heuristic_type_false
};

inline std::ostream &operator<<(std::ostream &out, HeuristicType t) {
    switch (t) {
        case HeuristicType::Level:  { out << "Level"; break; }
        case HeuristicType::Sign:   { out << "Sign"; break; }
        case HeuristicType::Factor: { out << "Factor"; break; }
        case HeuristicType::Init:   { out << "Init"; break; }
        case HeuristicType::True:   { out << "True"; break; }
        case HeuristicType::False:  { out << "False"; break; }
    }
    return out;
}

enum class ExternalType {
    Free    = clingo_external_type_free,
    True    = clingo_external_type_true,
    False   = clingo_external_type_false,
    Release = clingo_external_type_release
};

inline std::ostream &operator<<(std::ostream &out, ExternalType t) {
    switch (t) {
        case ExternalType::Free:    { out << "Free"; break; }
        case ExternalType::True:    { out << "True"; break; }
        case ExternalType::False:   { out << "False"; break; }
        case ExternalType::Release: { out << "Release"; break; }
    }
    return out;
}

class GroundProgramObserver {
public:
    GroundProgramObserver() = default;
    GroundProgramObserver(GroundProgramObserver const &) = default;
    GroundProgramObserver(GroundProgramObserver &&) noexcept = default;
    GroundProgramObserver &operator=(GroundProgramObserver const &) = default;
    GroundProgramObserver &operator=(GroundProgramObserver &&) noexcept = default;

    virtual void init_program(bool incremental);
    virtual void begin_step();
    virtual void end_step();

    virtual void rule(bool choice, AtomSpan head, LiteralSpan body);
    virtual void weight_rule(bool choice, AtomSpan head, weight_t lower_bound, WeightedLiteralSpan body);
    virtual void minimize(weight_t priority, WeightedLiteralSpan literals);
    virtual void project(AtomSpan atoms);
    virtual void output_atom(Symbol symbol, atom_t atom);
    virtual void output_term(Symbol symbol, LiteralSpan condition);
    virtual void output_csp(Symbol symbol, int value, LiteralSpan condition);
    virtual void external(atom_t atom, ExternalType type);
    virtual void assume(LiteralSpan literals);
    virtual void heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LiteralSpan condition);
    virtual void acyc_edge(int node_u, int node_v, LiteralSpan condition);

    virtual void theory_term_number(id_t term_id, int number);
    virtual void theory_term_string(id_t term_id, char const *name);
    virtual void theory_term_compound(id_t term_id, int name_id_or_type, IdSpan arguments);
    virtual void theory_element(id_t element_id, IdSpan terms, LiteralSpan condition);
    virtual void theory_atom(id_t atom_id_or_zero, id_t term_id, IdSpan elements);
    virtual void theory_atom_with_guard(id_t atom_id_or_zero, id_t term_id, IdSpan elements, id_t operator_id, id_t right_hand_side_id);
    virtual ~GroundProgramObserver() = default;
};

inline void GroundProgramObserver::init_program(bool) { }
inline void GroundProgramObserver::begin_step() { }
inline void GroundProgramObserver::end_step() { }

inline void GroundProgramObserver::rule(bool, AtomSpan, LiteralSpan) { }
inline void GroundProgramObserver::weight_rule(bool, AtomSpan, weight_t, WeightedLiteralSpan) { }
inline void GroundProgramObserver::minimize(weight_t, WeightedLiteralSpan) { }
inline void GroundProgramObserver::project(AtomSpan) { }
inline void GroundProgramObserver::output_atom(Symbol, atom_t) { }
inline void GroundProgramObserver::output_term(Symbol, LiteralSpan) { }
inline void GroundProgramObserver::output_csp(Symbol, int, LiteralSpan) { }
inline void GroundProgramObserver::external(atom_t, ExternalType) { }
inline void GroundProgramObserver::assume(LiteralSpan) { }
inline void GroundProgramObserver::heuristic(atom_t, HeuristicType, int, unsigned, LiteralSpan) { }
inline void GroundProgramObserver::acyc_edge(int, int, LiteralSpan) { }

inline void GroundProgramObserver::theory_term_number(id_t, int) { }
inline void GroundProgramObserver::theory_term_string(id_t, char const *) { }
inline void GroundProgramObserver::theory_term_compound(id_t, int, IdSpan) { }
inline void GroundProgramObserver::theory_element(id_t, IdSpan, LiteralSpan) { }
inline void GroundProgramObserver::theory_atom(id_t, id_t, IdSpan) { }
inline void GroundProgramObserver::theory_atom_with_guard(id_t, id_t, IdSpan, id_t, id_t) { }

// {{{1 symbolic literal

class SymbolicLiteral {
public:
    SymbolicLiteral(Symbol sym, bool positive)
    : symbol_{sym.to_c()}
    , positive_{positive} { }
    Symbol symbol() const { return Symbol{symbol_}; }
    bool is_positive() const { return positive_; }
    bool is_negative() const { return !positive_; }
private:
    clingo_symbol_t symbol_;
    bool positive_;
};

using SymbolicLiteralSpan = Span<SymbolicLiteral>;

inline std::ostream &operator<<(std::ostream &out, SymbolicLiteral sym) {
    if (sym.is_negative()) { out << "~"; }
    out << sym.symbol();
    return out;
}
inline bool operator==(SymbolicLiteral a, SymbolicLiteral b) { return a.is_negative() == b.is_negative() && a.symbol() == b.symbol(); }
inline bool operator!=(SymbolicLiteral a, SymbolicLiteral b) { return !(a == b); }
inline bool operator< (SymbolicLiteral a, SymbolicLiteral b) {
    if (a.is_negative() != b.is_negative()) { return !a.is_negative() && b.is_negative(); }
    return a.symbol() < b.symbol();
}
inline bool operator<=(SymbolicLiteral a, SymbolicLiteral b) { return !(b < a); }
inline bool operator> (SymbolicLiteral a, SymbolicLiteral b) { return  (b < a); }
inline bool operator>=(SymbolicLiteral a, SymbolicLiteral b) { return !(a < b); }

// {{{1 solve control

class SolveControl {
public:
    explicit SolveControl(clingo_solve_control_t *ctl)
    : ctl_(ctl) { }
    void add_clause(SymbolicLiteralSpan clause);
    void add_clause(LiteralSpan clause);
    SymbolicAtoms symbolic_atoms() const;
    clingo_solve_control_t *to_c() const { return ctl_; }
private:
    clingo_solve_control_t *ctl_;
};

// {{{1 model

enum class ModelType : clingo_model_type_t {
    StableModel = clingo_model_type_stable_model,
    BraveConsequences = clingo_model_type_brave_consequences,
    CautiousConsequences = clingo_model_type_cautious_consequences
};

class ShowType {
public:
    enum Type : clingo_show_type_bitset_t {
        CSP        = clingo_show_type_csp,
        Shown      = clingo_show_type_shown,
        Atoms      = clingo_show_type_atoms,
        Terms      = clingo_show_type_terms,
        Theory     = clingo_show_type_theory,
        All        = clingo_show_type_all,
        Complement = clingo_show_type_complement
    };
    ShowType(clingo_show_type_bitset_t type) : type_(type) { }
    operator clingo_show_type_bitset_t() const { return type_; }
private:
    clingo_show_type_bitset_t type_;
};

using CostVector = std::vector<int64_t>;

class Model {
public:
    explicit Model(clingo_model_t *model);
    Model(Model const &) = delete;
    Model(Model &&) noexcept = delete;
    Model &operator=(Model const &) = delete;
    Model &operator=(Model &&) = delete;
    ~Model() = default;
    bool contains(Symbol atom) const;
    bool is_true(literal_t literal) const;
    bool optimality_proven() const;
    CostVector cost() const;
    void extend(SymbolSpan symbols);
    SymbolVector symbols(ShowType show = ShowType::Shown) const;
    SolveControl context() const;
    ModelType type() const;
    id_t thread_id() const;
    uint64_t number() const;
    explicit operator bool() const { return model_ != nullptr; }
    clingo_model_t *to_c() const { return model_; }
private:
    clingo_model_t *model_;
};

inline std::ostream &operator<<(std::ostream &out, Model const &m) {
    out << SymbolSpan(m.symbols(ShowType::Shown));
    return out;
}

// {{{1 solve result

class SolveResult {
public:
    SolveResult() = default;
    explicit SolveResult(clingo_solve_result_bitset_t res)
    : res_(res) { }
    bool is_satisfiable() const { return (res_ & clingo_solve_result_satisfiable) != 0; }
    bool is_unsatisfiable() const { return (res_ & clingo_solve_result_unsatisfiable) != 0; }
    bool is_unknown() const { return (res_ & 3) == 0; }
    bool is_exhausted() const { return (res_ & clingo_solve_result_exhausted) != 0; }
    bool is_interrupted() const { return (res_ & clingo_solve_result_interrupted) != 0; }
    clingo_solve_result_bitset_t &to_c() { return res_; }
    clingo_solve_result_bitset_t const &to_c() const { return res_; }
    friend bool operator==(SolveResult a, SolveResult b) { return a.res_ == b.res_; }
    friend bool operator!=(SolveResult a, SolveResult b) { return a.res_ != b.res_; }
private:
    clingo_solve_result_bitset_t res_{0};
};

inline std::ostream &operator<<(std::ostream &out, SolveResult res) {
    if (res.is_satisfiable())    {
        out << "SATISFIABLE";
        if (!res.is_exhausted()) { out << "+"; }
    }
    else if (res.is_unsatisfiable())  { out << "UNSATISFIABLE"; }
    else { out << "UNKNOWN"; }
    if (res.is_interrupted()) { out << "/INTERRUPTED"; }
    return out;
}

// {{{1 location

class Location : public clingo_location_t {
public:
    explicit Location(clingo_location_t loc) : clingo_location_t(loc) { }
    Location(char const *begin_file, char const *end_file, size_t begin_line, size_t end_line, size_t begin_column, size_t end_column)
    : clingo_location_t{begin_file, end_file, begin_line, end_line, begin_column, end_column} { }
    char const *begin_file() const { return clingo_location_t::begin_file; }
    char const *end_file() const { return clingo_location_t::end_file; }
    size_t begin_line() const { return clingo_location_t::begin_line; }
    size_t end_line() const { return clingo_location_t::end_line; }
    size_t begin_column() const { return clingo_location_t::begin_column; }
    size_t end_column() const { return clingo_location_t::end_column; }
};

inline std::ostream &operator<<(std::ostream &out, Location loc) {
    out << loc.begin_file() << ":" << loc.begin_line() << ":" << loc.begin_column();
    bool dash = true;
    bool eq = std::strcmp(loc.begin_file(), loc.end_file()) == 0;
    if (!eq) { out << (dash ? "-" : ":") << loc.end_file(); dash = false; }
    eq = eq && (loc.begin_line() == loc.end_line());
    if (!eq) { out << (dash ? "-" : ":") << loc.end_line(); dash = false; }
    eq = eq && (loc.begin_column() == loc.end_column());
    if (!eq) { out << (dash ? "-" : ":") << loc.end_column(); }
    return out;
}

// {{{1 ast

namespace AST {

enum class ComparisonOperator : clingo_ast_comparison_operator_t {
    GreaterThan  = clingo_ast_comparison_operator_greater_than,
    LessThan     = clingo_ast_comparison_operator_less_than,
    LessEqual    = clingo_ast_comparison_operator_less_equal,
    GreaterEqual = clingo_ast_comparison_operator_greater_equal,
    NotEqual     = clingo_ast_comparison_operator_not_equal,
    Equal        = clingo_ast_comparison_operator_equal
};

inline std::ostream &operator<<(std::ostream &out, ComparisonOperator op) {
    switch (op) {
        case ComparisonOperator::GreaterThan:  { out << ">"; break; }
        case ComparisonOperator::LessThan:     { out << "<"; break; }
        case ComparisonOperator::LessEqual:    { out << "<="; break; }
        case ComparisonOperator::GreaterEqual: { out << ">="; break; }
        case ComparisonOperator::NotEqual:     { out << "!="; break; }
        case ComparisonOperator::Equal:        { out << "="; break; }
    }
    return out;
}

enum class Sign : clingo_ast_sign_t {
    None = clingo_ast_sign_none,
    Negation = clingo_ast_sign_negation,
    DoubleNegation = clingo_ast_sign_double_negation
};

inline std::ostream &operator<<(std::ostream &out, Sign op) {
    switch (op) {
        case Sign::None:           { out << ""; break; }
        case Sign::Negation:       { out << "not "; break; }
        case Sign::DoubleNegation: { out << "not not "; break; }
    }
    return out;
}

// {{{2 terms

// variable

struct Variable;
struct UnaryOperation;
struct BinaryOperation;
struct Interval;
struct Function;
struct Pool;

struct Term {
    Location location;
    Variant<Symbol, Variable, UnaryOperation, BinaryOperation, Interval, Function, Pool> data;
};
std::ostream &operator<<(std::ostream &out, Term const &x);

// Variable

struct Variable {
    char const *name;
};
std::ostream &operator<<(std::ostream &out, Variable const &x);

// unary operation

enum UnaryOperator : clingo_ast_unary_operator_t {
    Absolute = clingo_ast_unary_operator_absolute,
    Minus    = clingo_ast_unary_operator_minus,
    Negation = clingo_ast_unary_operator_negation
};

inline char const *left_hand_side(UnaryOperator op) {
    switch (op) {
        case UnaryOperator::Absolute: { return "|"; }
        case UnaryOperator::Minus:    { return "-"; }
        case UnaryOperator::Negation: { return "~"; }
    }
    return "";
}

inline char const *right_hand_side(UnaryOperator op) {
    switch (op) {
        case UnaryOperator::Absolute: { return "|"; }
        case UnaryOperator::Minus:
        case UnaryOperator::Negation: { return ""; }
    }
    return "";
}

struct UnaryOperation {
    UnaryOperator unary_operator;
    Term          argument;
};
std::ostream &operator<<(std::ostream &out, UnaryOperation const &x);

// binary operation

enum class BinaryOperator : clingo_ast_binary_operator_t {
    XOr            = clingo_ast_binary_operator_xor,
    Or             = clingo_ast_binary_operator_or,
    And            = clingo_ast_binary_operator_and,
    Plus           = clingo_ast_binary_operator_plus,
    Minus          = clingo_ast_binary_operator_minus,
    Multiplication = clingo_ast_binary_operator_multiplication,
    Division       = clingo_ast_binary_operator_division,
    Modulo         = clingo_ast_binary_operator_modulo,
    Power          = clingo_ast_binary_operator_power
};

inline std::ostream &operator<<(std::ostream &out, BinaryOperator op) {
    switch (op) {
        case BinaryOperator::XOr:            { out << "^"; break; }
        case BinaryOperator::Or:             { out << "?"; break; }
        case BinaryOperator::And:            { out << "&"; break; }
        case BinaryOperator::Plus:           { out << "+"; break; }
        case BinaryOperator::Minus:          { out << "-"; break; }
        case BinaryOperator::Multiplication: { out << "*"; break; }
        case BinaryOperator::Division:       { out << "/"; break; }
        case BinaryOperator::Modulo:         { out << "\\"; break; }
        case BinaryOperator::Power:          { out << "**"; break; }
    }
    return out;
}

struct BinaryOperation {
    BinaryOperator binary_operator;
    Term           left;
    Term           right;
};
std::ostream &operator<<(std::ostream &out, BinaryOperation const &x);

// interval

struct Interval {
    Term left;
    Term right;
};
std::ostream &operator<<(std::ostream &out, Interval const &x);

// function

struct Function {
    char const *name;
    std::vector<Term> arguments;
    bool external;
};
std::ostream &operator<<(std::ostream &out, Function const &x);

// pool

struct Pool {
    std::vector<Term> arguments;
};
std::ostream &operator<<(std::ostream &out, Pool const &x);

// {{{2 csp

struct CSPProduct {
    Location location;
    Term coefficient;
    Optional<Term> variable;
};
std::ostream &operator<<(std::ostream &out, CSPProduct const &x);

struct CSPSum {
    Location location;
    std::vector<CSPProduct> terms;
};
std::ostream &operator<<(std::ostream &out, CSPSum const &x);

struct CSPGuard {
    ComparisonOperator comparison;
    CSPSum term;
};
std::ostream &operator<<(std::ostream &out, CSPGuard const &x);

struct CSPLiteral {
    CSPSum term;
    std::vector<CSPGuard> guards;
};
std::ostream &operator<<(std::ostream &out, CSPLiteral const &x);

// {{{2 ids

struct Id {
    Location location;
    char const *id;
};
std::ostream &operator<<(std::ostream &out, Id const &x);

// {{{2 literals

struct Comparison {
    ComparisonOperator comparison;
    Term left;
    Term right;
};
std::ostream &operator<<(std::ostream &out, Comparison const &x);

struct Boolean {
    bool value;
};
std::ostream &operator<<(std::ostream &out, Boolean const &x);

struct Literal {
    Location location;
    Sign sign;
    Variant<Boolean, Term, Comparison, CSPLiteral> data;
};
std::ostream &operator<<(std::ostream &out, Literal const &x);

// {{{2 aggregates

enum class AggregateFunction : clingo_ast_aggregate_function_t {
    Count   = clingo_ast_aggregate_function_count,
    Sum     = clingo_ast_aggregate_function_sum,
    SumPlus = clingo_ast_aggregate_function_sump,
    Min     = clingo_ast_aggregate_function_min,
    Max     = clingo_ast_aggregate_function_max
};

inline std::ostream &operator<<(std::ostream &out, AggregateFunction op) {
    switch (op) {
        case AggregateFunction::Count:   { out << "#count"; break; }
        case AggregateFunction::Sum:     { out << "#sum"; break; }
        case AggregateFunction::SumPlus: { out << "#sum+"; break; }
        case AggregateFunction::Min:     { out << "#min"; break; }
        case AggregateFunction::Max:     { out << "#max"; break; }
    }
    return out;
}

struct AggregateGuard {
    ComparisonOperator comparison;
    Term term;
};

struct ConditionalLiteral {
    Literal literal;
    std::vector<Literal> condition;
};
std::ostream &operator<<(std::ostream &out, ConditionalLiteral const &x);

// lparse-style aggregate

struct Aggregate {
    std::vector<ConditionalLiteral> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};
std::ostream &operator<<(std::ostream &out, Aggregate const &x);

// body aggregate

struct BodyAggregateElement {
    std::vector<Term> tuple;
    std::vector<Literal> condition;
};
std::ostream &operator<<(std::ostream &out, BodyAggregateElement const &x);

struct BodyAggregate {
    AggregateFunction function;
    std::vector<BodyAggregateElement> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};
std::ostream &operator<<(std::ostream &out, BodyAggregate const &x);

// head aggregate

struct HeadAggregateElement {
    std::vector<Term> tuple;
    ConditionalLiteral condition;
};
std::ostream &operator<<(std::ostream &out, HeadAggregateElement const &x);

struct HeadAggregate {
    AggregateFunction function;
    std::vector<HeadAggregateElement> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};
std::ostream &operator<<(std::ostream &out, HeadAggregate const &x);

// disjunction

struct Disjunction {
    std::vector<ConditionalLiteral> elements;
};
std::ostream &operator<<(std::ostream &out, Disjunction const &x);

// disjoint

struct DisjointElement {
    Location location;
    std::vector<Term> tuple;
    CSPSum term;
    std::vector<Literal> condition;
};
std::ostream &operator<<(std::ostream &out, DisjointElement const &x);

struct Disjoint {
    std::vector<DisjointElement> elements;
};
std::ostream &operator<<(std::ostream &out, Disjoint const &x);

// {{{2 theory atom

enum class TheoryTermSequenceType : int {
    Tuple = 0,
    List  = 1,
    Set   = 2
};
inline char const *left_hand_side(TheoryTermSequenceType x) {
    switch (x) {
        case TheoryTermSequenceType::Tuple: { return "("; }
        case TheoryTermSequenceType::List:  { return "["; }
        case TheoryTermSequenceType::Set:   { return "{"; }
    }
    return "";
}
inline char const *right_hand_side(TheoryTermSequenceType x) {
    switch (x) {
        case TheoryTermSequenceType::Tuple: { return ")"; }
        case TheoryTermSequenceType::List:  { return "]"; }
        case TheoryTermSequenceType::Set:   { return "}"; }
    }
    return "";
}

struct TheoryFunction;
struct TheoryTermSequence;
struct TheoryUnparsedTerm;

struct TheoryTerm {
    Location location;
    Variant<Symbol, Variable, TheoryTermSequence, TheoryFunction, TheoryUnparsedTerm> data;
};
std::ostream &operator<<(std::ostream &out, TheoryTerm const &x);

struct TheoryTermSequence {
    TheoryTermSequenceType type;
    std::vector<TheoryTerm> terms;
};
std::ostream &operator<<(std::ostream &out, TheoryTermSequence const &x);

struct TheoryFunction {
    char const *name;
    std::vector<TheoryTerm> arguments;
};
std::ostream &operator<<(std::ostream &out, TheoryFunction const &x);

struct TheoryUnparsedTermElement {
    std::vector<char const *> operators;
    TheoryTerm term;
};
std::ostream &operator<<(std::ostream &out, TheoryUnparsedTermElement const &x);

struct TheoryUnparsedTerm {
    std::vector<TheoryUnparsedTermElement> elements;
};
std::ostream &operator<<(std::ostream &out, TheoryUnparsedTerm const &x);

struct TheoryAtomElement {
    std::vector<TheoryTerm> tuple;
    std::vector<Literal> condition;
};
std::ostream &operator<<(std::ostream &out, TheoryAtomElement const &x);

struct TheoryGuard {
    char const *operator_name;
    TheoryTerm term;
};
std::ostream &operator<<(std::ostream &out, TheoryGuard const &x);

struct TheoryAtom {
    Term term;
    std::vector<TheoryAtomElement> elements;
    Optional<TheoryGuard> guard;
};
std::ostream &operator<<(std::ostream &out, TheoryAtom const &x);

// {{{2 head literals

struct HeadLiteral {
    Location location;
    Variant<Literal, Disjunction, Aggregate, HeadAggregate, TheoryAtom> data;
};
std::ostream &operator<<(std::ostream &out, HeadLiteral const &x);

// {{{2 body literals

struct BodyLiteral {
    Location location;
    Sign sign;
    Variant<Literal, ConditionalLiteral, Aggregate, BodyAggregate, TheoryAtom, Disjoint> data;
};
std::ostream &operator<<(std::ostream &out, BodyLiteral const &x);

// {{{2 theory definitions

enum class TheoryOperatorType : clingo_ast_theory_operator_type_t {
     Unary       = clingo_ast_theory_operator_type_unary,
     BinaryLeft  = clingo_ast_theory_operator_type_binary_left,
     BinaryRight = clingo_ast_theory_operator_type_binary_right
};

inline std::ostream &operator<<(std::ostream &out, TheoryOperatorType op) {
    switch (op) {
        case TheoryOperatorType::Unary:       { out << "unary"; break; }
        case TheoryOperatorType::BinaryLeft:  { out << "binary, left"; break; }
        case TheoryOperatorType::BinaryRight: { out << "binary, right"; break; }
    }
    return out;
}

struct TheoryOperatorDefinition {
    Location location;
    char const *name;
    unsigned priority;
    TheoryOperatorType type;
};
std::ostream &operator<<(std::ostream &out, TheoryOperatorDefinition const &x);

struct TheoryTermDefinition {
    Location location;
    char const *name;
    std::vector<TheoryOperatorDefinition> operators;
};
std::ostream &operator<<(std::ostream &out, TheoryTermDefinition const &x);

struct TheoryGuardDefinition {
    char const *term;
    std::vector<char const *> operators;
};
std::ostream &operator<<(std::ostream &out, TheoryGuardDefinition const &x);

enum class TheoryAtomDefinitionType : clingo_ast_theory_atom_definition_type_t {
    Head      = clingo_ast_theory_atom_definition_type_head,
    Body      = clingo_ast_theory_atom_definition_type_body,
    Any       = clingo_ast_theory_atom_definition_type_any,
    Directive = clingo_ast_theory_atom_definition_type_directive
};

inline std::ostream &operator<<(std::ostream &out, TheoryAtomDefinitionType op) {
    switch (op) {
        case TheoryAtomDefinitionType::Head:      { out << "head"; break; }
        case TheoryAtomDefinitionType::Body:      { out << "body"; break; }
        case TheoryAtomDefinitionType::Any:       { out << "any"; break; }
        case TheoryAtomDefinitionType::Directive: { out << "directive"; break; }
    }
    return out;
}

struct TheoryAtomDefinition {
    Location location;
    TheoryAtomDefinitionType type;
    char const *name;
    unsigned arity;
    char const *elements;
    Optional<TheoryGuardDefinition> guard;
};
std::ostream &operator<<(std::ostream &out, TheoryAtomDefinition const &x);

struct TheoryDefinition {
    char const *name;
    std::vector<TheoryTermDefinition> terms;
    std::vector<TheoryAtomDefinition> atoms;
};
std::ostream &operator<<(std::ostream &out, TheoryDefinition const &x);

// {{{2 statements

// rule

struct Rule {
    HeadLiteral head;
    std::vector<BodyLiteral> body;
};
std::ostream &operator<<(std::ostream &out, Rule const &x);

// definition

struct Definition {
    char const *name;
    Term value;
    bool is_default;
};
std::ostream &operator<<(std::ostream &out, Definition const &x);

// show

struct ShowSignature {
    Signature signature;
    bool csp;
};
std::ostream &operator<<(std::ostream &out, ShowSignature const &x);

struct ShowTerm {
    Term term;
    std::vector<BodyLiteral> body;
    bool csp;
};
std::ostream &operator<<(std::ostream &out, ShowTerm const &x);

// signature

struct Defined {
    Signature signature;
};
std::ostream &operator<<(std::ostream &out, Defined const &x);

// minimize

struct Minimize {
    Term weight;
    Term priority;
    std::vector<Term> tuple;
    std::vector<BodyLiteral> body;
};
std::ostream &operator<<(std::ostream &out, Minimize const &x);

// script

enum class ScriptType : clingo_ast_script_type_t {
    Lua    = clingo_ast_script_type_lua,
    Python = clingo_ast_script_type_python
};

inline std::ostream &operator<<(std::ostream &out, ScriptType op) {
    switch (op) {
        case ScriptType::Lua:    { out << "lua"; break; }
        case ScriptType::Python: { out << "python"; break; }
    }
    return out;
}

struct Script {
    ScriptType type;
    char const *code;
};
std::ostream &operator<<(std::ostream &out, Script const &x);

// program

struct Program {
    char const *name;
    std::vector<Id> parameters;
};
std::ostream &operator<<(std::ostream &out, Program const &x);

// external

struct External {
    Term atom;
    std::vector<BodyLiteral> body;
    Term type;
};
std::ostream &operator<<(std::ostream &out, External const &x);

// edge

struct Edge {
    Term u;
    Term v;
    std::vector<BodyLiteral> body;
};
std::ostream &operator<<(std::ostream &out, Edge const &x);

// heuristic

struct Heuristic {
    Term atom;
    std::vector<BodyLiteral> body;
    Term bias;
    Term priority;
    Term modifier;
};
std::ostream &operator<<(std::ostream &out, Heuristic const &x);

// project

struct ProjectAtom {
    Term atom;
    std::vector<BodyLiteral> body;
};
std::ostream &operator<<(std::ostream &out, ProjectAtom const &x);

struct ProjectSignature {
    Signature signature;
};
std::ostream &operator<<(std::ostream &out, ProjectSignature const &x);

// statement

struct Statement {
    Location location;
    Variant<Rule, Definition, ShowSignature, ShowTerm, Minimize, Script, Program, External, Edge, Heuristic, ProjectAtom, ProjectSignature, TheoryDefinition, Defined> data;
};
std::ostream &operator<<(std::ostream &out, Statement const &x);

} // namespace AST

// {{{1 backend

class Backend {
public:
    explicit Backend(clingo_backend_t *backend);
    Backend(Backend const &) = delete;
    Backend(Backend &&) noexcept;
    Backend &operator=(Backend const &) = delete;
    Backend &operator=(Backend &&) noexcept;
    ~Backend();

    void rule(bool choice, AtomSpan head, LiteralSpan body);
    void weight_rule(bool choice, AtomSpan head, weight_t lower, WeightedLiteralSpan body);
    void minimize(weight_t prio, WeightedLiteralSpan body);
    void project(AtomSpan atoms);
    void external(atom_t atom, ExternalType type);
    void assume(LiteralSpan lits);
    void heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LiteralSpan condition);
    void acyc_edge(int node_u, int node_v, LiteralSpan condition);
    atom_t add_atom();
    atom_t add_atom(Symbol symbol);
    clingo_backend_t *to_c() const { return backend_; }
private:
    clingo_backend_t *backend_;
};

// {{{1 statistics

template <class T>
class KeyIterator : public std::iterator<std::random_access_iterator_tag, char const *, ptrdiff_t, ValuePointer<char const *>, char const *> {
public:
    explicit KeyIterator(T const *map, size_t index = 0)
    : map_(map)
    , index_(index) { }
    KeyIterator& operator++() { ++index_; return *this; }
    KeyIterator operator++(int) {
        KeyIterator t(*this);
        ++*this;
        return t;
    }
    KeyIterator& operator--() { --index_; return *this; }
    KeyIterator operator--(int) {
        KeyIterator t(*this);
        --*this;
        return t;
    }
    KeyIterator& operator+=(difference_type n) { index_ += n; return *this; }
    KeyIterator& operator-=(difference_type n) { index_ -= n; return *this; }
    friend KeyIterator operator+(KeyIterator it, difference_type n) { return KeyIterator{it.map_, it.index_ + n}; }
    friend KeyIterator operator+(difference_type n, KeyIterator it) { return KeyIterator{it.map_, it.index_ + n}; }
    friend KeyIterator operator-(KeyIterator it, difference_type n) { return KeyIterator{it.map_, it.index_ - n}; }
    friend difference_type operator-(KeyIterator a, KeyIterator b)  { return a.index_ - b.index_; }
    reference operator*() { return map_->key_name(index_); }
    pointer operator->() { return pointer(**this); }
    friend void swap(KeyIterator& lhs, KeyIterator& rhs) {
        std::swap(lhs.map_, rhs.map_);
        std::swap(lhs.index_, rhs.index_);
    }
    friend bool operator==(KeyIterator lhs, KeyIterator rhs) { return lhs.index_ == rhs.index_; }
    friend bool operator!=(KeyIterator lhs, KeyIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (KeyIterator lhs, KeyIterator rhs) { return (lhs.index_ + 1) < (rhs.index_ + 1); }
    friend bool operator> (KeyIterator lhs, KeyIterator rhs) { return rhs < lhs; }
    friend bool operator<=(KeyIterator lhs, KeyIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(KeyIterator lhs, KeyIterator rhs) { return !(lhs < rhs); }
private:
    T const *map_;
    size_t index_;
};

enum class StatisticsType : clingo_statistics_type_t {
    Value = clingo_statistics_type_value,
    Array = clingo_statistics_type_array,
    Map = clingo_statistics_type_map
};

template <bool constant>
class StatisticsBase {
    friend class KeyIterator<StatisticsBase>;
public:
    using statistics_t = typename std::conditional<constant, clingo_statistics_t const *, clingo_statistics_t*>::type;
    using KeyIteratorT = KeyIterator<StatisticsBase>;
    using ArrayIteratorT = ArrayIterator<StatisticsBase, StatisticsBase const *>;
    using KeyRangeT = IteratorRange<KeyIteratorT>;
    explicit StatisticsBase(statistics_t stats, uint64_t key)
    : stats_(stats)
    , key_(key) { }
    // generic
    StatisticsType type() const;
    // arrays
    size_t size() const;
    void ensure_size(size_t size, StatisticsType type);
    StatisticsBase operator[](size_t index) const;
    StatisticsBase at(size_t index) const { return operator[](index); }
    ArrayIteratorT begin() const;
    ArrayIteratorT end() const;
    StatisticsBase push(StatisticsType type);
    // maps
    StatisticsBase operator[](char const *name) const;
    StatisticsBase at(char const *name) const { return operator[](name); }
    bool has_subkey(char const *name) const;
    KeyRangeT keys() const;
    StatisticsBase add_subkey(char const *name, StatisticsType type);
    // leafs
    operator double() const { return value(); }
    double value() const;
    void set_value(double d);
    statistics_t to_c() const { return stats_; }
private:
    char const *key_name(size_t index) const;
    statistics_t stats_;
    uint64_t key_;
};

using Statistics = StatisticsBase<true>;
using UserStatistics = StatisticsBase<false>;
using UserStatisticCallback = std::function<void (UserStatistics &)>;

// {{{1 solve handle

class SolveEventHandler {
public:
    SolveEventHandler() = default;
    SolveEventHandler(SolveEventHandler const &) = default;
    SolveEventHandler(SolveEventHandler &&) noexcept = default;
    SolveEventHandler &operator=(SolveEventHandler const &) = default;
    SolveEventHandler &operator=(SolveEventHandler &&) noexcept = default;
    virtual bool on_model(Model &model);
    virtual void on_statistics(UserStatistics step, UserStatistics accu);
    virtual void on_finish(SolveResult result);
    virtual ~SolveEventHandler() = default;
};

inline bool SolveEventHandler::on_model(Model &) { return true; }
inline void SolveEventHandler::on_statistics(UserStatistics, UserStatistics) { }
inline void SolveEventHandler::on_finish(SolveResult) { }

namespace Detail {

class AssignOnce;

} // namespace Detail

class SolveHandle {
public:
    SolveHandle() = default;
    explicit SolveHandle(clingo_solve_handle_t *it, Detail::AssignOnce &ptr);
    SolveHandle(SolveHandle &&it) noexcept;
    SolveHandle(SolveHandle const &) = delete;
    SolveHandle &operator=(SolveHandle &&it) noexcept;
    SolveHandle &operator=(SolveHandle const &) = delete;
    clingo_solve_handle_t *to_c() const { return iter_; }
    void resume();
    void wait();
    bool wait(double timeout);
    Model const &model();
    Model const &next();
    LiteralSpan core();
    SolveResult get();
    void cancel();
    ~SolveHandle();
private:
    Model model_{nullptr};
    clingo_solve_handle_t *iter_{nullptr};
    Detail::AssignOnce *exception_{nullptr};
};

class ModelIterator : public std::iterator<Model, std::input_iterator_tag> {
public:
    explicit ModelIterator(SolveHandle &iter)
    : iter_(&iter) { }
    ModelIterator() = default;
    ModelIterator &operator++() {
        iter_->next();
        return *this;
    }
    // Warning: the resulting iterator should not be used
    //          because its model is no longer valid
    ModelIterator operator++(int) {
        ModelIterator t = *this;
        ++*this;
        return t;
    }
    Model const &operator*() { return iter_->model(); }
    Model const *operator->() { return &iter_->model(); }
    friend bool operator==(ModelIterator a, ModelIterator b) {
        auto *ap = (a.iter_ != nullptr ? a.iter_->model().to_c() : nullptr);
        auto *bp = (b.iter_ != nullptr ? b.iter_->model().to_c() : nullptr);
        return ap == bp;
    }
    friend bool operator!=(ModelIterator a, ModelIterator b) { return !(a == b); }
private:
    SolveHandle *iter_{nullptr};
};

inline ModelIterator begin(SolveHandle &it) { return ModelIterator(it); }
inline ModelIterator end(SolveHandle &) { return ModelIterator(); }

// {{{1 configuration

class Configuration;
using ConfigurationArrayIterator = ArrayIterator<Configuration>;
using ConfigurationKeyIterator = KeyIterator<Configuration>;
using ConfigurationKeyRange = IteratorRange<ConfigurationKeyIterator>;

class Configuration {
    friend class KeyIterator<Configuration>;
public:
    explicit Configuration(clingo_configuration_t *conf, clingo_id_t key)
    : conf_(conf)
    , key_(key) { }
    // arrays
    bool is_array() const;
    Configuration operator[](size_t index);
    Configuration at(size_t index) { return operator[](index); }
    ConfigurationArrayIterator begin();
    ConfigurationArrayIterator end();
    size_t size() const;
    bool empty() const;
    // maps
    bool is_map() const;
    Configuration operator[](char const *name);
    Configuration get(char const *name) { return operator[](name); }
    ConfigurationKeyRange keys() const;
    // values
    bool is_value() const;
    bool is_assigned() const;
    std::string value() const;
    operator std::string() const { return value(); }
    Configuration &operator=(char const *value);
    // generic
    char const *decription() const;
    clingo_configuration_t *to_c() const { return conf_; }
private:
    char const *key_name(size_t index) const;
    clingo_configuration_t *conf_;
    unsigned key_;
};

// {{{1 program builder

class ProgramBuilder {
public:
    explicit ProgramBuilder(clingo_program_builder_t *builder);
    ProgramBuilder() = delete;
    ProgramBuilder(ProgramBuilder const &) = delete;
    ProgramBuilder(ProgramBuilder &&) noexcept;
    ProgramBuilder &operator=(ProgramBuilder const &) = delete;
    ProgramBuilder &operator=(ProgramBuilder &&) noexcept;
    ~ProgramBuilder();

    void add(AST::Statement const &stm);
    clingo_program_builder_t *to_c() const { return builder_; }
private:
    clingo_program_builder_t *builder_;

};

// {{{1 control

class Part {
public:
    Part(char const *name, SymbolSpan params)
    : part_{name, reinterpret_cast<clingo_symbol_t const*>(params.begin()), params.size()} { }
    explicit Part(clingo_part_t part)
    : part_(part) { }
    char const *name() const { return part_.name; }
    SymbolSpan params() const { return {reinterpret_cast<Symbol const*>(part_.params), part_.size}; }
    clingo_part_t const &to_c() const { return part_; }
    clingo_part_t &to_c() { return part_; }
private:
    clingo_part_t part_;
};
using PartSpan = Span<Part>;
using GroundCallback = std::function<void (Location loc, char const *, SymbolSpan, SymbolSpanCallback)>;
using StringSpan = Span<char const *>;

enum class ErrorCode : clingo_error_t {
    Runtime = clingo_error_runtime,
    Logic = clingo_error_logic,
    BadAlloc = clingo_error_bad_alloc,
    Unknown = clingo_error_unknown,
};

inline std::ostream &operator<<(std::ostream &out, ErrorCode code) {
    out << clingo_error_string(static_cast<clingo_error_t>(code));
    return out;
}

enum class WarningCode : clingo_warning_t {
    OperationUndefined = clingo_warning_operation_undefined,
    RuntimeError = clingo_warning_runtime_error,
    AtomUndefined = clingo_warning_atom_undefined,
    FileIncluded = clingo_warning_file_included,
    VariableUnbounded = clingo_warning_variable_unbounded,
    GlobalVariable = clingo_warning_global_variable,
    Other = clingo_warning_other,
};

using Logger = std::function<void (WarningCode, char const *)>;

inline std::ostream &operator<<(std::ostream &out, WarningCode code) {
    out << clingo_warning_string(static_cast<clingo_warning_t>(code));
    return out;
}

class Control {
    struct Impl;
public:
    Control(StringSpan args = {}, Logger logger = nullptr, unsigned message_limit = 20);
    explicit Control(clingo_control_t *ctl, bool owns = true);
    Control(Control &&c) noexcept;
    Control(Control const &) = delete;
    Control &operator=(Control &&c) noexcept;
    Control &operator=(Control const &c) = delete;
    ~Control() noexcept;
    void add(char const *name, StringSpan params, char const *part);
    void ground(PartSpan parts, GroundCallback cb = nullptr);
    SolveHandle solve(LiteralSpan assumptions, SolveEventHandler *handler = nullptr, bool asynchronous = false, bool yield = true);
    SolveHandle solve(SymbolicLiteralSpan assumptions = {}, SolveEventHandler *handler = nullptr, bool asynchronous = false, bool yield = true);
    void assign_external(literal_t literal, TruthValue value);
    void assign_external(Symbol atom, TruthValue value);
    void release_external(literal_t literal);
    void release_external(Symbol atom);
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    void register_propagator(Propagator &propagator, bool sequential = false);
    void register_propagator(Heuristic &propagator, bool sequential = false);
    void register_observer(GroundProgramObserver &observer, bool replace = false);
    bool is_conflicting() const noexcept;
    bool has_const(char const *name) const;
    Symbol get_const(char const *name) const;
    void interrupt() noexcept;
    void *claspFacade();
    void load(char const *file);
    void enable_enumeration_assumption(bool value);
    bool enable_enumeration_assumption() const;
    void cleanup();
    void enable_cleanup(bool value);
    bool enable_cleanup() const;
    ProgramBuilder builder();
    template <class F>
    void with_builder(F f) {
        auto b = builder();
        f(b);
    }
    Backend backend();
    template <class F>
    void with_backend(F f) {
        auto b = backend();
        f(b);
    }
    Configuration configuration();
    Statistics statistics() const;
    clingo_control_t *to_c() const;
private:
    Impl *impl_;
};

// {{{1 ast v2

namespace ASTv2 {

enum class TheorySequenceType {
    Tuple = clingo_ast_theory_sequence_type_tuple,
    List = clingo_ast_theory_sequence_type_list,
    Set = clingo_ast_theory_sequence_type_set,
};

enum class ComparisonOperator {
    GreaterThan = clingo_ast_comparison_operator_greater_than,
    LessThan = clingo_ast_comparison_operator_less_than,
    LessEqual = clingo_ast_comparison_operator_less_equal,
    GreaterEqual = clingo_ast_comparison_operator_greater_equal,
    NotEqual = clingo_ast_comparison_operator_not_equal,
    Equal = clingo_ast_comparison_operator_equal,
};

enum class Sign {
    NoSign = clingo_ast_sign_no_sign,
    Negation = clingo_ast_sign_negation,
    DoubleNegation = clingo_ast_sign_double_negation,
};

enum class UnaryOperator {
    Minus = clingo_ast_unary_operator_minus,
    Negation = clingo_ast_unary_operator_negation,
    Absolute = clingo_ast_unary_operator_absolute,
};

enum class BinaryOperator {
    Xor = clingo_ast_binary_operator_xor,
    Or = clingo_ast_binary_operator_or,
    And = clingo_ast_binary_operator_and,
    Plus = clingo_ast_binary_operator_plus,
    Minus = clingo_ast_binary_operator_minus,
    Multiplication = clingo_ast_binary_operator_multiplication,
    Division = clingo_ast_binary_operator_division,
    Modulo = clingo_ast_binary_operator_modulo,
    Power = clingo_ast_binary_operator_power,
};

enum class AggregateFunction {
    Count = clingo_ast_aggregate_function_count,
    Sum = clingo_ast_aggregate_function_sum,
    Sump = clingo_ast_aggregate_function_sump,
    Min = clingo_ast_aggregate_function_min,
    Max = clingo_ast_aggregate_function_max,
};

enum class TheoryOperatorType {
     Unary = clingo_ast_theory_operator_type_unary,
     BinaryLeft = clingo_ast_theory_operator_type_binary_left,
     BinaryRight = clingo_ast_theory_operator_type_binary_right,
};

enum class TheoryAtomDefinitionType {
    Head = clingo_ast_theory_atom_definition_type_head,
    Body = clingo_ast_theory_atom_definition_type_body,
    Any = clingo_ast_theory_atom_definition_type_any,
    Directive = clingo_ast_theory_atom_definition_type_directive,
};

enum class ScriptType {
    Lua = clingo_ast_script_type_lua,
    Python = clingo_ast_script_type_python,
};

enum class Type {
    // terms
    Id = clingo_ast_type_id,
    Variable = clingo_ast_type_variable,
    SymbolicTerm = clingo_ast_type_symbolic_term,
    UnaryOperation = clingo_ast_type_unary_operation,
    BinaryOperation = clingo_ast_type_binary_operation,
    Interval = clingo_ast_type_interval,
    Function = clingo_ast_type_function,
    Pool = clingo_ast_type_pool,
    // csp terms
    CspProduct = clingo_ast_type_csp_product,
    CspSum = clingo_ast_type_csp_sum,
    CspGuard = clingo_ast_type_csp_guard,
    // simple atoms
    BooleanConstant = clingo_ast_type_boolean_constant,
    SymbolicAtom = clingo_ast_type_symbolic_atom,
    Comparison = clingo_ast_type_comparison,
    CspLiteral = clingo_ast_type_csp_literal,
    // aggregates
    AggregateGuard = clingo_ast_type_aggregate_guard,
    ConditionalLiteral = clingo_ast_type_conditional_literal,
    Aggregate = clingo_ast_type_aggregate,
    BodyAggregateElement = clingo_ast_type_body_aggregate_element,
    BodyAggregate = clingo_ast_type_body_aggregate,
    HeadAggregateElement = clingo_ast_type_head_aggregate_element,
    HeadAggregate = clingo_ast_type_head_aggregate,
    Disjunction = clingo_ast_type_disjunction,
    DisjointElement = clingo_ast_type_disjoint_element,
    Disjoint = clingo_ast_type_disjoint,
    // theory atoms
    TheorySequence = clingo_ast_type_theory_sequence,
    TheoryFunction = clingo_ast_type_theory_function,
    TheoryUnparsedTermElement = clingo_ast_type_theory_unparsed_term_element,
    TheoryUnparsedTerm = clingo_ast_type_theory_unparsed_term,
    TheoryGuard = clingo_ast_type_theory_guard,
    TheoryAtomElement = clingo_ast_type_theory_atom_element,
    TheoryAtom = clingo_ast_type_theory_atom,
    // literals
    Literal = clingo_ast_type_literal,
    // theory definition
    TheoryOperatorDefinition = clingo_ast_type_theory_operator_definition,
    TheoryTermDefinition = clingo_ast_type_theory_term_definition,
    TheoryGuardDefinition = clingo_ast_type_theory_guard_definition,
    TheoryAtomDefinition = clingo_ast_type_theory_atom_definition,
    // statements
    Rule = clingo_ast_type_rule,
    Definition = clingo_ast_type_definition,
    ShowSignature = clingo_ast_type_show_signature,
    ShowTerm = clingo_ast_type_show_term,
    Minimize = clingo_ast_type_minimize,
    Script = clingo_ast_type_script,
    Program = clingo_ast_type_program,
    External = clingo_ast_type_external,
    Edge = clingo_ast_type_edge,
    Heuristic = clingo_ast_type_heuristic,
    ProjectAtom = clingo_ast_type_project_atom,
    ProjectSignature = clingo_ast_type_project_signature,
    Defined = clingo_ast_type_defined,
    TheoryDefinition = clingo_ast_type_theory_definition,
};

enum class Attribute {
    Argument = clingo_ast_attribute_argument,
    Arguments = clingo_ast_attribute_arguments,
    Arity = clingo_ast_attribute_arity,
    Atom = clingo_ast_attribute_atom,
    Atoms = clingo_ast_attribute_atoms,
    AtomType = clingo_ast_attribute_atom_type,
    Bias = clingo_ast_attribute_bias,
    Body = clingo_ast_attribute_body,
    Code = clingo_ast_attribute_code,
    Coefficient = clingo_ast_attribute_coefficient,
    Comparison = clingo_ast_attribute_comparison,
    Condition = clingo_ast_attribute_condition,
    Csp = clingo_ast_attribute_csp,
    Elements = clingo_ast_attribute_elements,
    External = clingo_ast_attribute_external,
    ExternalType = clingo_ast_attribute_external_type,
    Function = clingo_ast_attribute_function,
    Guard = clingo_ast_attribute_guard,
    Guards = clingo_ast_attribute_guards,
    Head = clingo_ast_attribute_head,
    IsDefault = clingo_ast_attribute_is_default,
    Left = clingo_ast_attribute_left,
    LeftGuard = clingo_ast_attribute_left_guard,
    Literal = clingo_ast_attribute_literal,
    Location = clingo_ast_attribute_location,
    Modifier = clingo_ast_attribute_modifier,
    Name = clingo_ast_attribute_name,
    NodeU = clingo_ast_attribute_node_u,
    NodeV = clingo_ast_attribute_node_v,
    OperatorName = clingo_ast_attribute_operator_name,
    OperatorType = clingo_ast_attribute_operator_type,
    Operators = clingo_ast_attribute_operators,
    Parameters = clingo_ast_attribute_parameters,
    Positive = clingo_ast_attribute_positive,
    Priority = clingo_ast_attribute_priority,
    Right = clingo_ast_attribute_right,
    RightGuard = clingo_ast_attribute_right_guard,
    ScriptType = clingo_ast_attribute_script_type,
    SequenceType = clingo_ast_attribute_sequence_type,
    Sign = clingo_ast_attribute_sign,
    Symbol = clingo_ast_attribute_symbol,
    Term = clingo_ast_attribute_term,
    Terms = clingo_ast_attribute_terms,
    Value = clingo_ast_attribute_value,
    Variable = clingo_ast_attribute_variable,
    Weight = clingo_ast_attribute_weight,
};

class AST;
class ASTVector;
class StringVector;

using ASTValue = Variant<int, Symbol, Location, char const *, AST, Optional<AST>, StringVector, ASTVector>;

class AST {
public:
    explicit AST(clingo_ast_t *ast);
    template <class... Args>
    AST(Type type, Args&& ...args);
    AST(AST const &ast);
    AST(AST &&ast) noexcept;
    AST &operator=(AST const &ast);
    AST &operator=(AST &&ast) noexcept;
    ~AST();
    AST copy() const;
    AST deep_copy() const;
    Type type() const;
    ASTValue get(Attribute attribute) const;
    void set(Attribute attribute, ASTValue value);
    template <class Visitor>
    void visit_attribute(Visitor &&visitor);
    template <class Visitor>
    void visit_ast(Visitor &&visitor);
    std::string to_string() const;
    std::vector<AST> unpool(bool other=true, bool condition=true) const;
    clingo_ast_t *to_c() const { return ast_; }
    friend std::ostream &operator<<(std::ostream &out, AST const &ast);
    friend bool operator<(AST const &a, AST const &b);
    friend bool operator>(AST const &a, AST const &b);
    friend bool operator<=(AST const &a, AST const &b);
    friend bool operator>=(AST const &a, AST const &b);
    friend bool operator==(AST const &a, AST const &b);
    friend bool operator!=(AST const &a, AST const &b);
    size_t hash() const;
private:
    clingo_ast_t *ast_;
};

class ASTVector {
public:
    using value_type = AST;
    using iterator = ArrayIterator<AST, ASTVector*>;
    using const_iterator = ArrayIterator<AST const, ASTVector const *>;

    ASTVector(AST ast, clingo_ast_attribute_t attr);
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_t size() const;
    bool empty() const;
    iterator insert(iterator it, AST const &ast);
    iterator erase(iterator it);
    AST at(size_t idx) const;
    AST operator[](size_t idx) const { return at(idx); }
    void push_back(AST const &ast);
    void pop_back();
    void clear();
    AST &ast();
    AST const &ast() const;
private:
    AST ast_;
    clingo_ast_attribute_t attr_;
};

class StringVector;

class StringRef {
public:
    StringRef(StringVector *vec, size_t index);
    StringRef &operator=(char const *);
    char const *get() const;
    operator char const *() const;
private:
    StringVector *vec_;
    size_t index_;
};

class StringVector {
public:
    using value_type = char const *;
    using iterator = ArrayIterator<StringRef, StringVector*>;
    using const_iterator = ArrayIterator<char const *, StringVector const *>;

    StringVector(AST ast, clingo_ast_attribute_t attr);
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    size_t size() const;
    bool empty() const;
    iterator insert(iterator it, char const *str);
    void set(iterator it, char const *str);
    iterator erase(iterator it);
    StringRef at(size_t idx);
    StringRef operator[](size_t idx) { return at(idx); }
    char const *at(size_t idx) const;
    char const *operator[](size_t idx) const { return at(idx); }
    void push_back(char const *str);
    void pop_back();
    void clear();
    AST &ast();
    AST const &ast() const;
private:
    AST ast_;
    clingo_ast_attribute_t attr_;
};

class ProgramBuilder {
public:
    explicit ProgramBuilder(Control &ctl);
    explicit ProgramBuilder(clingo_program_builder_t *builder);
    ProgramBuilder() = delete;
    ProgramBuilder(ProgramBuilder const &) = delete;
    ProgramBuilder(ProgramBuilder &&) noexcept;
    ProgramBuilder &operator=(ProgramBuilder const &) = delete;
    ProgramBuilder &operator=(ProgramBuilder &&) noexcept;
    ~ProgramBuilder();

    void add(AST const &ast);
    clingo_program_builder_t *to_c() const { return builder_; }

private:
    clingo_program_builder_t *builder_;
};

template <class Callback>
void parse_string(char const *program, Callback &&cb, Logger logger = nullptr, unsigned message_limit = 20);

template <class Callback>
void parse_files(StringSpan files, Callback &&cb, Logger logger = nullptr, unsigned message_limit = 20);

template <class F>
inline void with_builder(Control &ctl, F f) {
    auto b = ProgramBuilder{ctl};
    f(b);
}

} // namespace ASTv2

} namespace std {

template<>
struct hash<Clingo::ASTv2::AST> {
    size_t operator()(Clingo::ASTv2::AST const &ast) const { return ast.hash(); }
};

} namespace Clingo {

// {{{1 clingo application

namespace Detail {
    using ParserList = std::forward_list<std::function<bool (char const *value)>>;
}

class ClingoOptions {
public:
    explicit ClingoOptions(clingo_options_t *options, Detail::ParserList &parsers)
    : options_{options}
    , parsers_{parsers} { }
    clingo_options_t *to_c() const { return options_; }
    void add(char const *group, char const *option, char const *description, std::function<bool (char const *value)> parser, bool multi = false, char const *argument = nullptr);
    void add_flag(char const *group, char const *option, char const *description, bool &target);
private:
    clingo_options_t *options_;
    Detail::ParserList &parsers_;
};

struct Application {
    Application() = default;
    Application(Application const &) = default;
    Application(Application &&) noexcept = default;
    Application &operator=(Application const &) = default;
    Application &operator=(Application &&) noexcept = default;
    virtual unsigned message_limit() const noexcept;
    virtual char const *program_name() const noexcept;
    virtual char const *version() const noexcept;
    virtual void main(Control &ctl, StringSpan files) = 0;
    virtual void log(WarningCode code, char const *message) noexcept;
    virtual void print_model(Model const &model, std::function<void()> default_printer) noexcept;
    virtual void register_options(ClingoOptions &app);
    virtual void validate_options();
    virtual ~Application() = default;
};

// {{{1 global functions

using StatementCallback = std::function<void (AST::Statement &&)>;

void parse_program(char const *program, StatementCallback cb, Logger logger = nullptr, unsigned message_limit = 20);
void parse_program(StringSpan files, StatementCallback cb, Logger logger = nullptr, unsigned message_limit = 20);
Symbol parse_term(char const *str, Logger logger = nullptr, unsigned message_limit = 20);
char const *add_string(char const *str);
std::tuple<int, int, int> version();

inline int clingo_main(Application &application, StringSpan arguments);

// }}}1

} // namespace Clingo

//{{{1 implementation

#define CLINGO_CALLBACK_TRY try
#define CLINGO_CALLBACK_CATCH(ref) catch (...){ (ref) = std::current_exception(); return false; } return true

#define CLINGO_TRY try
#define CLINGO_CATCH catch (...){ Detail::handle_cxx_error(); return false; } return true

namespace Clingo {

// {{{2 details

namespace Detail {

inline void handle_error(bool ret) {
    if (!ret) {
        char const *msg = clingo_error_message();
        if (msg == nullptr) { msg = "no message"; }
        switch (static_cast<clingo_error>(clingo_error_code())) {
            case clingo_error_runtime:   { throw std::runtime_error(msg); }
            case clingo_error_logic:     { throw std::logic_error(msg); }
            case clingo_error_bad_alloc: { throw std::bad_alloc(); }
            case clingo_error_unknown:
            case clingo_error_success:   { throw std::runtime_error(msg); }
        }
    }
}

inline void handle_error(bool ret, std::exception_ptr &exc) {
    if (!ret) {
        if (exc) {
            std::exception_ptr ptr = exc;
            exc = nullptr;
            std::rethrow_exception(ptr);
        }
        handle_error(false);
    }
}

enum class AssignState { Unassigned=0, Writing=1, Assigned=2 };

class AssignOnce {
public:
    AssignOnce &operator=(std::exception_ptr x) {
        auto ua = AssignState::Unassigned;
        if (state_.compare_exchange_strong(ua, AssignState::Writing)) {
            val_ = std::move(x);
            state_ = AssignState::Assigned;
        }
        return *this;
    }
    std::exception_ptr &operator*() {
        static std::exception_ptr null = nullptr;
        // NOTE: this is not necessarily the exception that caused the search to stop
        //       but just the first that has been set
        return state_ == AssignState::Assigned ? val_ : null;
    }
    void reset() {
        state_ = AssignState::Unassigned;
        val_ = nullptr;
    }
private:
    std::atomic<AssignState> state_{AssignState::Unassigned};
    std::exception_ptr val_ = nullptr;
};

inline void handle_error(bool ret, AssignOnce &exc) {
    if (!ret) { handle_error(ret, *exc); }
}

inline void handle_cxx_error() {
    try { throw; }
    catch (std::bad_alloc const &e)     { clingo_set_error(clingo_error_bad_alloc, e.what()); return; }
    catch (std::runtime_error const &e) { clingo_set_error(clingo_error_runtime, e.what()); return; }
    catch (std::logic_error const &e)   { clingo_set_error(clingo_error_logic, e.what()); return; }
    catch (...)                         { }
    clingo_set_error(clingo_error_unknown, "unknown error");
}

template <class S, class P, class ...Args>
std::string to_string(S size, P print, Args ...args) {
    std::vector<char> ret;
    size_t n;
    Detail::handle_error(size(std::forward<Args>(args)..., &n));
    ret.resize(n);
    Detail::handle_error(print(std::forward<Args>(args)..., ret.data(), n));
    return std::string(ret.begin(), ret.end()-1);
}

} // namespace Detail

// {{{2 signature

inline Signature::Signature(char const *name, uint32_t arity, bool positive) {
    Detail::handle_error(clingo_signature_create(name, arity, positive, &sig_));
}

inline char const *Signature::name() const {
    return clingo_signature_name(sig_);
}

inline uint32_t Signature::arity() const {
    return clingo_signature_arity(sig_);
}

inline bool Signature::positive() const {
    return clingo_signature_is_positive(sig_);
}

inline bool Signature::negative() const {
    return clingo_signature_is_negative(sig_);
}

inline size_t Signature::hash() const {
    return clingo_signature_hash(sig_);
}

inline bool operator==(Signature a, Signature b) { return  clingo_signature_is_equal_to(a.to_c(), b.to_c()); }
inline bool operator!=(Signature a, Signature b) { return !clingo_signature_is_equal_to(a.to_c(), b.to_c()); }
inline bool operator< (Signature a, Signature b) { return  clingo_signature_is_less_than(a.to_c(), b.to_c()); }
inline bool operator<=(Signature a, Signature b) { return !clingo_signature_is_less_than(b.to_c(), a.to_c()); }
inline bool operator> (Signature a, Signature b) { return  clingo_signature_is_less_than(b.to_c(), a.to_c()); }
inline bool operator>=(Signature a, Signature b) { return !clingo_signature_is_less_than(a.to_c(), b.to_c()); }

// {{{2 symbol

inline Symbol::Symbol() {
    clingo_symbol_create_number(0, &sym_);
}

inline Symbol::Symbol(clingo_symbol_t sym)
: sym_(sym) { }

inline Symbol Number(int num) {
    clingo_symbol_t sym;
    clingo_symbol_create_number(num, &sym);
    return Symbol(sym);
}

inline Symbol Supremum() {
    clingo_symbol_t sym;
    clingo_symbol_create_supremum(&sym);
    return Symbol(sym);
}

inline Symbol Infimum() {
    clingo_symbol_t sym;
    clingo_symbol_create_infimum(&sym);
    return Symbol(sym);
}

inline Symbol String(char const *str) {
    clingo_symbol_t sym;
    Detail::handle_error(clingo_symbol_create_string(str, &sym));
    return Symbol(sym);
}

inline Symbol Id(char const *id, bool positive) {
    clingo_symbol_t sym;
    Detail::handle_error(clingo_symbol_create_id(id, positive, &sym));
    return Symbol(sym);
}

inline Symbol Function(char const *name, SymbolSpan args, bool positive) {
    clingo_symbol_t sym;
    Detail::handle_error(clingo_symbol_create_function(name, reinterpret_cast<clingo_symbol_t const *>(args.begin()), args.size(), positive, &sym));
    return Symbol(sym);
}

inline int Symbol::number() const {
    int ret;
    Detail::handle_error(clingo_symbol_number(sym_, &ret));
    return ret;
}

inline char const *Symbol::name() const {
    char const *ret;
    Detail::handle_error(clingo_symbol_name(sym_, &ret));
    return ret;
}

inline char const *Symbol::string() const {
    char const *ret;
    Detail::handle_error(clingo_symbol_string(sym_, &ret));
    return ret;
}

inline bool Symbol::is_positive() const {
    bool ret;
    Detail::handle_error(clingo_symbol_is_positive(sym_, &ret));
    return ret;
}

inline bool Symbol::is_negative() const {
    bool ret;
    Detail::handle_error(clingo_symbol_is_negative(sym_, &ret));
    return ret;
}

inline SymbolSpan Symbol::arguments() const {
    clingo_symbol_t const *ret;
    size_t n;
    Detail::handle_error(clingo_symbol_arguments(sym_, &ret, &n));
    return {reinterpret_cast<Symbol const *>(ret), n};
}

inline SymbolType Symbol::type() const {
    return static_cast<SymbolType>(clingo_symbol_type(sym_));
}

inline bool Symbol::match(char const *name, unsigned arity) const {
    return type() == SymbolType::Function && strcmp(this->name(), name) == 0 && this->arguments().size() == arity;
}

inline std::string Symbol::to_string() const {
    return Detail::to_string(clingo_symbol_to_string_size, clingo_symbol_to_string, sym_);
}

inline size_t Symbol::hash() const {
    return clingo_symbol_hash(sym_);
}

inline std::ostream &operator<<(std::ostream &out, Symbol sym) {
    out << sym.to_string();
    return out;
}

inline bool operator==(Symbol a, Symbol b) { return  clingo_symbol_is_equal_to(a.to_c(), b.to_c()); }
inline bool operator!=(Symbol a, Symbol b) { return !clingo_symbol_is_equal_to(a.to_c(), b.to_c()); }
inline bool operator< (Symbol a, Symbol b) { return  clingo_symbol_is_less_than(a.to_c(), b.to_c()); }
inline bool operator<=(Symbol a, Symbol b) { return !clingo_symbol_is_less_than(b.to_c(), a.to_c()); }
inline bool operator> (Symbol a, Symbol b) { return  clingo_symbol_is_less_than(b.to_c(), a.to_c()); }
inline bool operator>=(Symbol a, Symbol b) { return !clingo_symbol_is_less_than(a.to_c(), b.to_c()); }

// {{{2 symbolic atoms

inline Symbol SymbolicAtom::symbol() const {
    clingo_symbol_t ret;
    clingo_symbolic_atoms_symbol(atoms_, range_, &ret);
    return Symbol(ret);
}

inline clingo_literal_t SymbolicAtom::literal() const {
    clingo_literal_t ret;
    clingo_symbolic_atoms_literal(atoms_, range_, &ret);
    return ret;
}

inline bool SymbolicAtom::is_fact() const {
    bool ret;
    clingo_symbolic_atoms_is_fact(atoms_, range_, &ret);
    return ret;
}

inline bool SymbolicAtom::is_external() const {
    bool ret;
    clingo_symbolic_atoms_is_external(atoms_, range_, &ret);
    return ret;
}

inline bool SymbolicAtom::match(char const *name, unsigned arity) const {
    return symbol().match(name, arity);
}

inline SymbolicAtomIterator &SymbolicAtomIterator::operator++() {
    clingo_symbolic_atom_iterator_t range;
    Detail::handle_error(clingo_symbolic_atoms_next(atoms_, range_, &range));
    range_ = range;
    return *this;
}

inline SymbolicAtomIterator::operator bool() const {
    bool ret;
    Detail::handle_error(clingo_symbolic_atoms_is_valid(atoms_, range_, &ret));
    return ret;
}

inline bool SymbolicAtomIterator::operator==(SymbolicAtomIterator it) const {
    bool ret = atoms_ == it.atoms_;
    if (ret) { Detail::handle_error(clingo_symbolic_atoms_iterator_is_equal_to(atoms_, range_, it.range_, &ret)); }
    return ret;
}

inline SymbolicAtomIterator SymbolicAtoms::begin() const {
    clingo_symbolic_atom_iterator_t it;
    Detail::handle_error(clingo_symbolic_atoms_begin(atoms_, nullptr, &it));
    return SymbolicAtomIterator{atoms_,  it};
}

inline SymbolicAtomIterator SymbolicAtoms::begin(Signature sig) const {
    clingo_symbolic_atom_iterator_t it;
    Detail::handle_error(clingo_symbolic_atoms_begin(atoms_, &sig.to_c(), &it));
    return SymbolicAtomIterator{atoms_, it};
}

inline SymbolicAtomIterator SymbolicAtoms::end() const {
    clingo_symbolic_atom_iterator_t it;
    Detail::handle_error(clingo_symbolic_atoms_end(atoms_, &it));
    return SymbolicAtomIterator{atoms_, it};
}

inline SymbolicAtomIterator SymbolicAtoms::find(Symbol atom) const {
    clingo_symbolic_atom_iterator_t it;
    Detail::handle_error(clingo_symbolic_atoms_find(atoms_, atom.to_c(), &it));
    return SymbolicAtomIterator{atoms_, it};
}

inline std::vector<Signature> SymbolicAtoms::signatures() const {
    size_t n;
    clingo_symbolic_atoms_signatures_size(atoms_, &n);
    Signature sig("", 0);
    std::vector<Signature> ret;
    ret.resize(n, sig);
    Detail::handle_error(clingo_symbolic_atoms_signatures(atoms_, reinterpret_cast<clingo_signature_t *>(ret.data()), n));
    return ret;
}

inline size_t SymbolicAtoms::length() const {
    size_t ret;
    Detail::handle_error(clingo_symbolic_atoms_size(atoms_, &ret));
    return ret;
}

// {{{2 theory atoms

inline TheoryTermType TheoryTerm::type() const {
    clingo_theory_term_type_t ret;
    Detail::handle_error(clingo_theory_atoms_term_type(atoms_, id_, &ret));
    return static_cast<TheoryTermType>(ret);
}

inline int TheoryTerm::number() const {
    int ret;
    Detail::handle_error(clingo_theory_atoms_term_number(atoms_, id_, &ret));
    return ret;
}

inline char const *TheoryTerm::name() const {
    char const *ret;
    Detail::handle_error(clingo_theory_atoms_term_name(atoms_, id_, &ret));
    return ret;
}

inline TheoryTermSpan TheoryTerm::arguments() const {
    clingo_id_t const *ret;
    size_t n;
    Detail::handle_error(clingo_theory_atoms_term_arguments(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryTermIterator>{atoms_}};
}

inline std::ostream &operator<<(std::ostream &out, TheoryTerm term) {
    out << term.to_string();
    return out;
}

inline std::string TheoryTerm::to_string() const {
    return Detail::to_string(clingo_theory_atoms_term_to_string_size, clingo_theory_atoms_term_to_string, atoms_, id_);
}


inline TheoryTermSpan TheoryElement::tuple() const {
    clingo_id_t const *ret;
    size_t n;
    Detail::handle_error(clingo_theory_atoms_element_tuple(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryTermIterator>{atoms_}};
}

inline LiteralSpan TheoryElement::condition() const {
    clingo_literal_t const *ret;
    size_t n;
    Detail::handle_error(clingo_theory_atoms_element_condition(atoms_, id_, &ret, &n));
    return {ret, n};
}

inline literal_t TheoryElement::condition_id() const {
    clingo_literal_t ret;
    Detail::handle_error(clingo_theory_atoms_element_condition_id(atoms_, id_, &ret));
    return ret;
}

inline std::string TheoryElement::to_string() const {
    return Detail::to_string(clingo_theory_atoms_element_to_string_size, clingo_theory_atoms_element_to_string, atoms_, id_);
}

inline std::ostream &operator<<(std::ostream &out, TheoryElement term) {
    out << term.to_string();
    return out;
}

inline TheoryElementSpan TheoryAtom::elements() const {
    clingo_id_t const *ret;
    size_t n;
    Detail::handle_error(clingo_theory_atoms_atom_elements(atoms_, id_, &ret, &n));
    return {ret, n, ToTheoryIterator<TheoryElementIterator>{atoms_}};
}

inline TheoryTerm TheoryAtom::term() const {
    clingo_id_t ret;
    Detail::handle_error(clingo_theory_atoms_atom_term(atoms_, id_, &ret));
    return TheoryTerm{atoms_, ret};
}

inline bool TheoryAtom::has_guard() const {
    bool ret;
    Detail::handle_error(clingo_theory_atoms_atom_has_guard(atoms_, id_, &ret));
    return ret;
}

inline literal_t TheoryAtom::literal() const {
    clingo_literal_t ret;
    Detail::handle_error(clingo_theory_atoms_atom_literal(atoms_, id_, &ret));
    return ret;
}

inline std::pair<char const *, TheoryTerm> TheoryAtom::guard() const {
    char const *name;
    clingo_id_t term;
    Detail::handle_error(clingo_theory_atoms_atom_guard(atoms_, id_, &name, &term));
    return {name, TheoryTerm{atoms_, term}};
}

inline std::string TheoryAtom::to_string() const {
    return Detail::to_string(clingo_theory_atoms_atom_to_string_size, clingo_theory_atoms_atom_to_string, atoms_, id_);
}

inline std::ostream &operator<<(std::ostream &out, TheoryAtom term) {
    out << term.to_string();
    return out;
}

inline TheoryAtomIterator TheoryAtoms::begin() const {
    return TheoryAtomIterator{atoms_, 0};
}

inline TheoryAtomIterator TheoryAtoms::end() const {
    return TheoryAtomIterator{atoms_, clingo_id_t(size())};
}

inline size_t TheoryAtoms::size() const {
    size_t ret;
    Detail::handle_error(clingo_theory_atoms_size(atoms_, &ret));
    return ret;
}

// {{{2 trail

inline uint32_t Trail::size() const {
    uint32_t ret;
    Detail::handle_error(clingo_assignment_trail_size(ass_, &ret));
    return ret;
}

inline uint32_t Trail::begin_offset(uint32_t level) const {
    uint32_t ret;
    Detail::handle_error(clingo_assignment_trail_begin(ass_, level, &ret));
    return ret;
}

inline uint32_t Trail::end_offset(uint32_t level) const {
    uint32_t ret;
    Detail::handle_error(clingo_assignment_trail_end(ass_, level, &ret));
    return ret;
}

inline literal_t Trail::at(uint32_t offset) const {
    clingo_literal_t ret;
    Detail::handle_error(clingo_assignment_trail_at(ass_, offset, &ret));
    return ret;
}

// {{{2 assignment

inline bool Assignment::has_conflict() const {
    return clingo_assignment_has_conflict(ass_);
}

inline uint32_t Assignment::decision_level() const {
    return clingo_assignment_decision_level(ass_);
}

inline uint32_t Assignment::root_level() const {
    return clingo_assignment_root_level(ass_);
}

inline bool Assignment::has_literal(literal_t lit) const {
    return clingo_assignment_has_literal(ass_, lit);
}

inline TruthValue Assignment::truth_value(literal_t lit) const {
    clingo_truth_value_t ret;
    Detail::handle_error(clingo_assignment_truth_value(ass_, lit, &ret));
    return static_cast<TruthValue>(ret);
}

inline uint32_t Assignment::level(literal_t lit) const {
    uint32_t ret;
    Detail::handle_error(clingo_assignment_level(ass_, lit, &ret));
    return ret;
}

inline literal_t Assignment::decision(uint32_t level) const {
    literal_t ret;
    Detail::handle_error(clingo_assignment_decision(ass_, level, &ret));
    return ret;
}

inline bool Assignment::is_fixed(literal_t lit) const {
    bool ret;
    Detail::handle_error(clingo_assignment_is_fixed(ass_, lit, &ret));
    return ret;
}

inline bool Assignment::is_true(literal_t lit) const {
    bool ret;
    Detail::handle_error(clingo_assignment_is_true(ass_, lit, &ret));
    return ret;
}

inline bool Assignment::is_false(literal_t lit) const {
    bool ret;
    Detail::handle_error(clingo_assignment_is_false(ass_, lit, &ret));
    return ret;
}

inline size_t Assignment::size() const {
    return clingo_assignment_size(ass_);
}

inline bool Assignment::is_total() const {
    return clingo_assignment_is_total(ass_);
}

inline literal_t Assignment::at(size_t offset) const {
    clingo_literal_t ret;
    Detail::handle_error(clingo_assignment_at(ass_, offset, &ret));
    return ret;
}

// {{{2 propagate init

inline literal_t PropagateInit::solver_literal(literal_t lit) const {
    literal_t ret;
    Detail::handle_error(clingo_propagate_init_solver_literal(init_, lit, &ret));
    return ret;
}

inline void PropagateInit::add_watch(literal_t lit) {
    Detail::handle_error(clingo_propagate_init_add_watch(init_, lit));
}

inline void PropagateInit::add_watch(literal_t lit, id_t thread_id) {
    Detail::handle_error(clingo_propagate_init_add_watch_to_thread(init_, lit, thread_id));
}

inline int PropagateInit::number_of_threads() const {
    return clingo_propagate_init_number_of_threads(init_);
}

inline Assignment PropagateInit::assignment() const {
    return Assignment{clingo_propagate_init_assignment(init_)};
}

inline SymbolicAtoms PropagateInit::symbolic_atoms() const {
    clingo_symbolic_atoms_t const *ret;
    Detail::handle_error(clingo_propagate_init_symbolic_atoms(init_, &ret));
    return SymbolicAtoms{ret};
}

inline TheoryAtoms PropagateInit::theory_atoms() const {
    clingo_theory_atoms_t const *ret;
    Detail::handle_error(clingo_propagate_init_theory_atoms(init_, &ret));
    return TheoryAtoms{ret};
}

inline PropagatorCheckMode PropagateInit::get_check_mode() const {
    return static_cast<PropagatorCheckMode>(clingo_propagate_init_get_check_mode(init_));
}

inline void PropagateInit::set_check_mode(PropagatorCheckMode mode) {
    clingo_propagate_init_set_check_mode(init_, mode);
}

inline literal_t PropagateInit::add_literal(bool freeze) {
    literal_t ret;
    Detail::handle_error(clingo_propagate_init_add_literal(init_, freeze, &ret));
    return ret;
}

inline bool PropagateInit::add_clause(LiteralSpan clause) {
    bool ret;
    Detail::handle_error(clingo_propagate_init_add_clause(init_, clause.begin(), clause.size(), &ret));
    return ret;
}

inline bool PropagateInit::add_weight_constraint(literal_t literal, WeightedLiteralSpan literals, weight_t bound, WeightConstraintType type, bool compare_equal) {
    bool ret;
    Detail::handle_error(clingo_propagate_init_add_weight_constraint(init_, literal, reinterpret_cast<clingo_weighted_literal_t const *>(literals.begin()), literals.size(), bound, type, compare_equal, &ret));
    return ret;
}

inline void PropagateInit::add_minimize(literal_t literal, weight_t weight, weight_t priority) {
    Detail::handle_error(clingo_propagate_init_add_minimize(init_, literal, weight, priority));
}

inline bool PropagateInit::propagate() {
    bool ret;
    Detail::handle_error(clingo_propagate_init_propagate(init_, &ret));
    return ret;
}

// {{{2 propagate control

inline id_t PropagateControl::thread_id() const {
    return clingo_propagate_control_thread_id(ctl_);
}

inline Assignment PropagateControl::assignment() const {
    return Assignment{clingo_propagate_control_assignment(ctl_)};
}

inline literal_t PropagateControl::add_literal() {
    clingo_literal_t ret;
    Detail::handle_error(clingo_propagate_control_add_literal(ctl_, &ret));
    return ret;
}

inline void PropagateControl::add_watch(literal_t literal) {
    Detail::handle_error(clingo_propagate_control_add_watch(ctl_, literal));
}

inline bool PropagateControl::has_watch(literal_t literal) const {
    return clingo_propagate_control_has_watch(ctl_, literal);
}

inline void PropagateControl::remove_watch(literal_t literal) {
    clingo_propagate_control_remove_watch(ctl_, literal);
}

inline bool PropagateControl::add_clause(LiteralSpan clause, ClauseType type) {
    bool ret;
    Detail::handle_error(clingo_propagate_control_add_clause(ctl_, clause.begin(), clause.size(), static_cast<clingo_clause_type_t>(type), &ret));
    return ret;
}

inline bool PropagateControl::propagate() {
    bool ret;
    Detail::handle_error(clingo_propagate_control_propagate(ctl_, &ret));
    return ret;
}

// {{{2 propagator

inline void Propagator::init(PropagateInit &) { }
inline void Propagator::propagate(PropagateControl &, LiteralSpan) { }
inline void Propagator::undo(PropagateControl const &, LiteralSpan) noexcept { }
inline void Propagator::check(PropagateControl &) { }
inline literal_t Heuristic::decide(id_t, Assignment const &, literal_t) { return 0; }

// {{{2 solve control

inline SymbolicAtoms SolveControl::symbolic_atoms() const {
    clingo_symbolic_atoms_t const *atoms;
    Detail::handle_error(clingo_solve_control_symbolic_atoms(ctl_, &atoms));
    return SymbolicAtoms{atoms};
}

inline void SolveControl::add_clause(SymbolicLiteralSpan clause) {
    std::vector<literal_t> lits;
    auto atoms = symbolic_atoms();
    for (auto const &x : clause) {
        auto it = atoms.find(x.symbol());
        if (it != atoms.end()) {
            auto lit = it->literal();
            lits.emplace_back(x.is_positive() ? lit : -lit);
        }
        else if (x.is_negative()) { return; }
    }
    add_clause(LiteralSpan{lits});
}

inline void SolveControl::add_clause(LiteralSpan clause) {
    Detail::handle_error(clingo_solve_control_add_clause(ctl_, reinterpret_cast<clingo_literal_t const *>(clause.begin()), clause.size()));
}

// {{{2 model

inline Model::Model(clingo_model_t *model)
: model_(model) { }

inline bool Model::contains(Symbol atom) const {
    bool ret;
    Detail::handle_error(clingo_model_contains(model_, atom.to_c(), &ret));
    return ret;
}

inline bool Model::is_true(literal_t literal) const {
    bool ret;
    Detail::handle_error(clingo_model_is_true(model_, literal, &ret));
    return ret;
}

inline CostVector Model::cost() const {
    CostVector ret;
    size_t n;
    Detail::handle_error(clingo_model_cost_size(model_, &n));
    ret.resize(n);
    Detail::handle_error(clingo_model_cost(model_, ret.data(), n));
    return ret;
}

inline void Model::extend(SymbolSpan symbols) {
    Detail::handle_error(clingo_model_extend(model_, reinterpret_cast<clingo_symbol_t const *>(symbols.begin()), symbols.size()));
}

inline SymbolVector Model::symbols(ShowType show) const {
    SymbolVector ret;
    size_t n;
    Detail::handle_error(clingo_model_symbols_size(model_, show, &n));
    ret.resize(n);
    Detail::handle_error(clingo_model_symbols(model_, show, reinterpret_cast<clingo_symbol_t *>(ret.data()), n));
    return ret;
}

inline uint64_t Model::number() const {
    uint64_t ret;
    Detail::handle_error(clingo_model_number(model_, &ret));
    return ret;
}

inline bool Model::optimality_proven() const {
    bool ret;
    Detail::handle_error(clingo_model_optimality_proven(model_, &ret));
    return ret;
}

inline SolveControl Model::context() const {
    clingo_solve_control_t *ret;
    Detail::handle_error(clingo_model_context(model_, &ret));
    return SolveControl{ret};
}

inline ModelType Model::type() const {
    clingo_model_type_t ret;
    Detail::handle_error(clingo_model_type(model_, &ret));
    return static_cast<ModelType>(ret);
}

inline id_t Model::thread_id() const {
    id_t ret;
    Detail::handle_error(clingo_model_thread_id(model_, &ret));
    return ret;
}

// {{{2 solve handle

namespace Detail {

} // namespace Detail

inline SolveHandle::SolveHandle(clingo_solve_handle_t *it, Detail::AssignOnce &ptr)
: iter_(it)
, exception_(&ptr) { }

inline SolveHandle::SolveHandle(SolveHandle &&it) noexcept
: SolveHandle() { *this = std::move(it); }

inline SolveHandle &SolveHandle::operator=(SolveHandle &&it) noexcept {
    std::swap(iter_, it.iter_);
    std::swap(exception_, it.exception_);
    return *this;
}

inline void SolveHandle::resume() {
    Detail::handle_error(clingo_solve_handle_resume(iter_), *exception_);
}

inline void SolveHandle::wait() {
    (void)wait(-1);
}

inline bool SolveHandle::wait(double timeout) {
    bool res = true;
    clingo_solve_handle_wait(iter_, timeout, &res);
    return res;
}

inline Model const &SolveHandle::model() {
    clingo_model_t const *m = nullptr;
    Detail::handle_error(clingo_solve_handle_model(iter_, &m), *exception_);
    new (&model_) Model{const_cast<clingo_model_t*>(m)};
    return model_;
}

inline LiteralSpan SolveHandle::core() {
    literal_t const *core;
    size_t size;
    Detail::handle_error(clingo_solve_handle_core(iter_, &core, &size), *exception_);
    return {core, size};
}

inline Model const &SolveHandle::next() {
    resume();
    return model();
}

inline SolveResult SolveHandle::get() {
    clingo_solve_result_bitset_t ret = 0;
    Detail::handle_error(clingo_solve_handle_get(iter_, &ret), *exception_);
    return SolveResult{ret};
}

inline void SolveHandle::cancel() {
    Detail::handle_error(clingo_solve_handle_cancel(iter_), *exception_);
}

inline SolveHandle::~SolveHandle() {
    if (iter_ != nullptr) { Detail::handle_error(clingo_solve_handle_close(iter_), *exception_); }
}

// {{{2 backend

inline Backend::Backend(Backend &&backend) noexcept
: backend_{nullptr} {
    *this = std::move(backend);
}

inline Backend &Backend::operator=(Backend &&backend) noexcept {
    std::swap(backend_, backend.backend_);
    return *this;
}

inline Backend::Backend(clingo_backend_t *backend)
: backend_{backend} {
    Detail::handle_error(clingo_backend_begin(backend_));
}

inline Backend::~Backend() {
    if (backend_ != nullptr) {
        Detail::handle_error(clingo_backend_end(backend_));
    }
}

inline void Backend::rule(bool choice, AtomSpan head, LiteralSpan body) {
    Detail::handle_error(clingo_backend_rule(backend_, choice, head.begin(), head.size(), body.begin(), body.size()));
}

inline void Backend::weight_rule(bool choice, AtomSpan head, weight_t lower, WeightedLiteralSpan body) {
    Detail::handle_error(clingo_backend_weight_rule(backend_, choice, head.begin(), head.size(), lower, reinterpret_cast<clingo_weighted_literal_t const *>(body.begin()), body.size()));
}

inline void Backend::minimize(weight_t prio, WeightedLiteralSpan body) {
    Detail::handle_error(clingo_backend_minimize(backend_, prio, reinterpret_cast<clingo_weighted_literal_t const *>(body.begin()), body.size()));
}

inline void Backend::project(AtomSpan atoms) {
    Detail::handle_error(clingo_backend_project(backend_, atoms.begin(), atoms.size()));
}

inline void Backend::external(atom_t atom, ExternalType type) {
    Detail::handle_error(clingo_backend_external(backend_, atom, static_cast<clingo_external_type_t>(type)));
}

inline void Backend::assume(LiteralSpan lits) {
    Detail::handle_error(clingo_backend_assume(backend_, lits.begin(), lits.size()));
}

inline void Backend::heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LiteralSpan condition) {
    Detail::handle_error(clingo_backend_heuristic(backend_, atom, static_cast<clingo_heuristic_type_t>(type), bias, priority, condition.begin(), condition.size()));
}

inline void Backend::acyc_edge(int node_u, int node_v, LiteralSpan condition) {
    Detail::handle_error(clingo_backend_acyc_edge(backend_, node_u, node_v, condition.begin(), condition.size()));
}

inline atom_t Backend::add_atom() {
    clingo_atom_t ret;
    Detail::handle_error(clingo_backend_add_atom(backend_, nullptr, &ret));
    return ret;
}

inline atom_t Backend::add_atom(Symbol symbol) {
    clingo_atom_t ret;
    clingo_symbol_t sym = symbol.to_c();
    Detail::handle_error(clingo_backend_add_atom(backend_, &sym, &ret));
    return ret;
}

// {{{2 statistics

template <bool constant>
inline StatisticsType StatisticsBase<constant>::type() const {
    clingo_statistics_type_t ret;
    Detail::handle_error(clingo_statistics_type(stats_, key_, &ret));
    return StatisticsType(ret);
}

template <bool constant>
inline size_t StatisticsBase<constant>::size() const {
    size_t ret;
    Detail::handle_error(clingo_statistics_array_size(stats_, key_, &ret));
    return ret;
}

template <bool constant>
void StatisticsBase<constant>::ensure_size(size_t size, StatisticsType type) {
    for (auto s = this->size(); s < size; ++s) { push(type); }
}

template <bool constant>
inline StatisticsBase<constant> StatisticsBase<constant>::operator[](size_t index) const {
    uint64_t ret;
    Detail::handle_error(clingo_statistics_array_at(stats_, key_, index, &ret));
    return StatisticsBase{stats_, ret};
}

template <bool constant>
inline StatisticsBase<constant> StatisticsBase<constant>::push(StatisticsType type) {
    uint64_t ret;
    Detail::handle_error(clingo_statistics_array_push(stats_, key_, static_cast<clingo_statistics_type_t>(type), &ret));
    return StatisticsBase{stats_, ret};
}


template <bool constant>
inline typename StatisticsBase<constant>::ArrayIteratorT StatisticsBase<constant>::begin() const {
    return ArrayIteratorT{this, 0};
}

template <bool constant>
inline typename StatisticsBase<constant>::ArrayIteratorT StatisticsBase<constant>::end() const {
    return ArrayIteratorT{this, size()};
}

template <bool constant>
inline StatisticsBase<constant> StatisticsBase<constant>::operator[](char const *name) const {
    uint64_t ret;
    Detail::handle_error(clingo_statistics_map_at(stats_, key_, name, &ret));
    return StatisticsBase{stats_, ret};
}

template <bool constant>
inline bool StatisticsBase<constant>::has_subkey(char const *name) const {
    bool ret;
    Detail::handle_error(clingo_statistics_map_has_subkey(stats_, key_, name, &ret));
    return ret;
}

template <bool constant>
inline StatisticsBase<constant> StatisticsBase<constant>::add_subkey(char const *name, StatisticsType type) {
    uint64_t ret;
    Detail::handle_error(clingo_statistics_map_add_subkey(stats_, key_, name, static_cast<clingo_statistics_type_t>(type), &ret));
    return StatisticsBase{stats_, ret};
}

template <bool constant>
inline typename StatisticsBase<constant>::KeyRangeT StatisticsBase<constant>::keys() const {
    size_t ret;
    Detail::handle_error(clingo_statistics_map_size(stats_, key_, &ret));
    return KeyRangeT{ KeyIteratorT{this, 0}, KeyIteratorT{this, ret} };
}

template <bool constant>
inline double StatisticsBase<constant>::value() const {
    double ret;
    Detail::handle_error(clingo_statistics_value_get(stats_, key_, &ret));
    return ret;
}

template <bool constant>
inline void StatisticsBase<constant>::set_value(double d) {
    Detail::handle_error(clingo_statistics_value_set(stats_, key_, d));
}

template <bool constant>
inline char const *StatisticsBase<constant>::key_name(size_t index) const {
    char const *ret;
    Detail::handle_error(clingo_statistics_map_subkey_name(stats_, key_, index, &ret));
    return ret;
}

// {{{2 configuration

inline Configuration Configuration::operator[](size_t index) {
    unsigned ret;
    Detail::handle_error(clingo_configuration_array_at(conf_, key_, index, &ret));
    return Configuration{conf_, ret};
}

inline ConfigurationArrayIterator Configuration::begin() {
    return ConfigurationArrayIterator{this, 0};
}

inline ConfigurationArrayIterator Configuration::end() {
    return ConfigurationArrayIterator{this, size()};
}

inline size_t Configuration::size() const {
    size_t n;
    Detail::handle_error(clingo_configuration_array_size(conf_, key_, &n));
    return n;
}

inline bool Configuration::empty() const {
    return size() == 0;
}

inline Configuration Configuration::operator[](char const *name) {
    clingo_id_t ret;
    Detail::handle_error(clingo_configuration_map_at(conf_, key_, name, &ret));
    return Configuration{conf_, ret};
}

inline ConfigurationKeyRange Configuration::keys() const {
    size_t n;
    Detail::handle_error(clingo_configuration_map_size(conf_, key_, &n));
    return ConfigurationKeyRange{ ConfigurationKeyIterator{this, size_t(0)}, ConfigurationKeyIterator{this, size_t(n)} };
}

inline bool Configuration::is_value() const {
    clingo_configuration_type_bitset_t type;
    Detail::handle_error(clingo_configuration_type(conf_, key_, &type));
    return (type & clingo_configuration_type_value) != 0;
}

inline bool Configuration::is_array() const {
    clingo_configuration_type_bitset_t type;
    Detail::handle_error(clingo_configuration_type(conf_, key_, &type));
    return (type & clingo_configuration_type_array) != 0;
}

inline bool Configuration::is_map() const {
    clingo_configuration_type_bitset_t type;
    Detail::handle_error(clingo_configuration_type(conf_, key_, &type));
    return (type & clingo_configuration_type_map) != 0;
}

inline bool Configuration::is_assigned() const {
    bool ret;
    Detail::handle_error(clingo_configuration_value_is_assigned(conf_, key_, &ret));
    return ret;
}

inline std::string Configuration::value() const {
    size_t n;
    Detail::handle_error(clingo_configuration_value_get_size(conf_, key_, &n));
    std::vector<char> ret(n);
    Detail::handle_error(clingo_configuration_value_get(conf_, key_, ret.data(), n));
    return std::string(ret.begin(), ret.end() - 1);
}

inline Configuration &Configuration::operator=(char const *value) {
    Detail::handle_error(clingo_configuration_value_set(conf_, key_, value));
    return *this;
}

inline char const *Configuration::decription() const {
    char const *ret;
    Detail::handle_error(clingo_configuration_description(conf_, key_, &ret));
    return ret;
}

inline char const *Configuration::key_name(size_t index) const {
    char const *ret;
    Detail::handle_error(clingo_configuration_map_subkey_name(conf_, key_, index, &ret));
    return ret;
}


// {{{2 ast v2

namespace ASTv2 {

namespace ASTDetail {

template <size_t j>
struct construct_ast {
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, int arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_number) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg);
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, char const *arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_string) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg);
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, Symbol const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_symbol) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg.to_c());
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, Location const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_location) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., &arg);
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, AST const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_ast) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg.to_c());
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, Optional<AST> const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_optional_ast) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg.get());
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, std::vector<AST> const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_ast_array) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg.data(), arg.size());
    }
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, std::vector<char const *> const &arg, Args&& ...args) {
        if (cons.size <= i || cons.arguments[i].type != clingo_ast_attribute_type_string_array) {
            throw std::runtime_error("invalid argument");
        }
        return construct_ast<j - 1>::template construct<i + 1>(type, cons, std::forward<Args>(args)..., arg.data(), arg.size());
    }
};

template <>
struct construct_ast<0> {
    template <size_t i, class... Args>
    static clingo_ast_t *construct(clingo_ast_type_t type, clingo_ast_constructor_t const &cons, Args&& ...args) {
        if (cons.size != i) {
            throw std::runtime_error("invalid argument");
        }
        clingo_ast_t *ret;
        clingo_ast_build(type, &ret, std::forward<Args>(args)...);
        return ret;
    }
};

template <class V>
struct ASTVisitor {
    void operator()(ASTValue &value) {
        if (value.is<AST>()) {
            v(value.get<AST>());
        }
        else if (value.is<Optional<AST>>()) {
            auto *ast = value.get<Optional<AST>>().get();
            if (ast != nullptr) {
                v(*ast);
            }
        }
        else if (value.is<ASTVector>()) {
            for (auto ast : value.get<ASTVector>()) {
                v(ast);
            }
        }
    }
    V &v;
};

} // namespace ASTDetail

// AST

inline AST::AST(clingo_ast_t *ast)
: ast_{ast} { }

template <class... Args>
AST::AST(Type type, Args&& ...args)
: ast_{ASTDetail::construct_ast<sizeof...(Args)>::template construct<0>(
    static_cast<clingo_ast_type_t>(type),
    g_clingo_ast_constructors.constructors[static_cast<size_t>(type)],
    std::forward<Args>(args)...)} {
}

inline AST::AST(AST const &ast)
: ast_{ast.ast_} {
    clingo_ast_acquire(ast_);
}

inline AST::AST(AST &&ast) noexcept
: ast_{ast.ast_} {
    ast.ast_ = nullptr;
}

inline AST &AST::operator=(AST const &ast) {
    if (ast_ != ast.ast_) {
        if (ast_ != nullptr) {
            clingo_ast_release(ast_);
        }
        ast_ = ast.ast_;
        if (ast_ != nullptr) {
            clingo_ast_acquire(ast_);
        }
    }
    return *this;
}

inline AST &AST::operator=(AST &&ast) noexcept {
    if (ast_ != ast.ast_) {
        if (ast_ != nullptr) {
            clingo_ast_release(ast_);
            ast_ = nullptr;
        }
        std::swap(ast_, ast.ast_);
    }
    return *this;
}

inline AST::~AST() {
    if (ast_ != nullptr) {
        clingo_ast_release(ast_);
    }
}

inline AST AST::copy() const {
    clingo_ast_t *ast;
    Detail::handle_error(clingo_ast_copy(ast_, &ast));
    return AST{ast};
}

inline AST AST::deep_copy() const {
    clingo_ast_t *ast;
    Detail::handle_error(clingo_ast_deep_copy(ast_, &ast));
    return AST{ast};
}

inline Type AST::type() const {
    clingo_ast_type_t type;
    Detail::handle_error(clingo_ast_get_type(ast_, &type));
    return static_cast<Type>(type);
}

inline ASTValue AST::get(Attribute attribute) const {
    bool has_attribute;
    clingo_ast_attribute_t attr = static_cast<clingo_ast_attribute_t>(attribute);
    Detail::handle_error(clingo_ast_has_attribute(ast_, attr, &has_attribute));
    if (!has_attribute) {
        throw std::runtime_error("unknown attribute");
    }
    clingo_ast_attribute_type_t type;
    Detail::handle_error(clingo_ast_attribute_type(ast_, attr, &type));
    switch (static_cast<enum clingo_ast_attribute_type>(type)) {
        case clingo_ast_attribute_type_number: {
            int ret;
            Detail::handle_error(clingo_ast_attribute_get_number(ast_, attr, &ret));
            return {ret};
        }
        case clingo_ast_attribute_type_symbol: {
            clingo_symbol_t ret;
            Detail::handle_error(clingo_ast_attribute_get_symbol(ast_, attr, &ret));
            return {Clingo::Symbol{ret}};
        }
        case clingo_ast_attribute_type_location: {
            clingo_location_t ret;
            Detail::handle_error(clingo_ast_attribute_get_location(ast_, attr, &ret));
            return {Clingo::Location{ret}};
        }
        case clingo_ast_attribute_type_string: {
            char const *ret;
            Detail::handle_error(clingo_ast_attribute_get_string(ast_, attr, &ret));
            return {ret};
        }
        case clingo_ast_attribute_type_ast: {
            clingo_ast_t *ret;
            Detail::handle_error(clingo_ast_attribute_get_ast(ast_, attr, &ret));
            return {AST{ret}};
        }
        case clingo_ast_attribute_type_optional_ast: {
            clingo_ast_t *ret;
            Detail::handle_error(clingo_ast_attribute_get_optional_ast(ast_, attr, &ret));
            return ret == nullptr ? Optional<AST>{} : Optional<AST>{AST{ret}};
        }
        case clingo_ast_attribute_type_string_array: {
            return {StringVector{*this, attr}};
        }
        case clingo_ast_attribute_type_ast_array: {
            break;
        }
    }
    return {ASTVector{*this, attr}};
}

inline void AST::set(Attribute attribute, ASTValue value) {
    bool has_attribute;
    clingo_ast_attribute_t attr = static_cast<clingo_ast_attribute_t>(attribute);
    Detail::handle_error(clingo_ast_has_attribute(ast_, attr, &has_attribute));
    if (!has_attribute) {
        throw std::runtime_error("unknow attribute");
    }
    clingo_ast_attribute_type_t type;
    Detail::handle_error(clingo_ast_attribute_type(ast_, attr, &type));
    switch (static_cast<enum clingo_ast_attribute_type>(type)) {
        case clingo_ast_attribute_type_number: {
            return Detail::handle_error(clingo_ast_attribute_set_number(ast_, attr, value.get<int>()));
        }
        case clingo_ast_attribute_type_symbol: {
            return Detail::handle_error(clingo_ast_attribute_set_symbol(ast_, attr, value.get<Symbol>().to_c()));
        }
        case clingo_ast_attribute_type_location: {
            return Detail::handle_error(clingo_ast_attribute_set_location(ast_, attr, &value.get<Location>()));
        }
        case clingo_ast_attribute_type_string: {
            return Detail::handle_error(clingo_ast_attribute_set_string(ast_, attr, value.get<char const*>()));
        }
        case clingo_ast_attribute_type_ast: {
            return Detail::handle_error(clingo_ast_attribute_set_ast(ast_, attr, value.get<AST>().ast_));
        }
        case clingo_ast_attribute_type_optional_ast: {
            auto *ast = value.get<Optional<AST>>().get();
            return Detail::handle_error(clingo_ast_attribute_set_optional_ast(ast_, attr, ast != nullptr ? ast->ast_ : nullptr));
        }
        case clingo_ast_attribute_type_string_array: {
            auto val = get(attribute);
            auto &a = val.get<StringVector>();
            auto &b = value.get<StringVector>();
            if (a.ast().to_c() != b.ast().to_c()) {
                a.clear();
                for (auto x : b) {
                    a.push_back(x);
                }
            }
            return;
        }
        case clingo_ast_attribute_type_ast_array: {
            auto val = get(attribute);
            auto &a = val.get<ASTVector>();
            auto &b = value.get<ASTVector>();
            if (a.ast().to_c() != b.ast().to_c()) {
                a.clear();
                for (auto x : b) {
                    a.push_back(x);
                }
            }
            return;
        }
    }
}

template <class Visitor>
inline void AST::visit_attribute(Visitor &&visitor) {
    auto const &cons = g_clingo_ast_constructors.constructors[static_cast<size_t>(type())];
    for (auto &x : make_span(cons.arguments, cons.size)) {
        auto attr = static_cast<Attribute>(x.attribute);
        visitor(attr, get(attr));
    }
}

template <class Visitor>
inline void AST::visit_ast(Visitor &&visitor) {
    ASTDetail::ASTVisitor<Visitor> v{visitor};
    visit_attribute(v);
}

inline std::string AST::to_string() const {
    return Detail::to_string(clingo_ast_to_string_size, clingo_ast_to_string, ast_);
}

inline std::vector<AST> AST::unpool(bool other, bool condition) const {
    clingo_ast_unpool_type_bitset_t type = 0;
    if (other) {
        type |= clingo_ast_unpool_type_other;
    }
    if (condition) {
        type |= clingo_ast_unpool_type_condition;
    }
    using Data = std::pair<std::vector<AST>, std::exception_ptr>;
    Data data({}, nullptr);
    Detail::handle_error(clingo_ast_unpool(ast_, type, [](clingo_ast_t *ast, void *data) -> bool {
        auto &d = *static_cast<Data*>(data);
        clingo_ast_acquire(ast);
        CLINGO_CALLBACK_TRY { d.first.emplace_back(AST{ast}); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data));
    return std::move(data.first);
}

inline std::ostream &operator<<(std::ostream &out, AST const &ast) {
    out << ast.to_string();
    return out;
}

inline bool operator<(AST const &a, AST const &b) {
    if (a.ast_ == nullptr || b.ast_ == nullptr) {
        throw std::runtime_error("invalid AST");
    }
    return clingo_ast_less_than(a.ast_, b.ast_);
}

inline bool operator>(AST const &a, AST const &b) {
    return b < a;
}

inline bool operator<=(AST const &a, AST const &b) {
    return !(b < a);
}

inline bool operator>=(AST const &a, AST const &b) {
    return !(a < b);
}

inline bool operator==(AST const &a, AST const &b) {
    if (a.ast_ == nullptr || b.ast_ == nullptr) {
        throw std::runtime_error("invalid AST");
    }
    return clingo_ast_equal(a.ast_, b.ast_);
}

inline bool operator!=(AST const &a, AST const &b) {
    return !(a == b);
}

inline size_t AST::hash() const {
    return clingo_ast_hash(ast_);
}

// ASTVector

inline ASTVector::ASTVector(AST ast, clingo_ast_attribute_t attr)
: ast_{std::move(ast)}
, attr_{attr} { }

inline ASTVector::iterator ASTVector::begin() {
    return iterator{this, 0};
}

inline ASTVector::iterator ASTVector::end() {
    return iterator{this, size()};
}

inline ASTVector::const_iterator ASTVector::begin() const {
    return const_iterator{this, 0};
}

inline ASTVector::const_iterator ASTVector::end() const {
    return const_iterator{this, size()};
}

inline size_t ASTVector::size() const {
    size_t ret;
    Detail::handle_error(clingo_ast_attribute_size_ast_array(ast_.to_c(), attr_, &ret));
    return ret;
}

inline bool ASTVector::empty() const {
    return size() == 0;
}

inline ASTVector::iterator ASTVector::insert(iterator it, AST const &ast) {
    Detail::handle_error(clingo_ast_attribute_insert_ast_at(ast_.to_c(), attr_, it - begin(), ast.to_c()));
    return it;
}

inline ASTVector::iterator ASTVector::erase(iterator it) {
    Detail::handle_error(clingo_ast_attribute_delete_ast_at(ast_.to_c(), attr_, it - begin()));
    return it;
}

inline AST ASTVector::at(size_t idx) const {
    clingo_ast_t *ret;
    Detail::handle_error(clingo_ast_attribute_get_ast_at(ast_.to_c(), attr_, idx, &ret));
    return AST{ret};
}

inline void ASTVector::push_back(AST const &ast) {
    insert(end(), ast);
}

inline void ASTVector::pop_back() {
    erase(end() - 1);
}

inline void ASTVector::clear() {
    for (size_t n = size(); n > 0; --n) {
        Detail::handle_error(clingo_ast_attribute_delete_ast_at(ast_.to_c(), attr_, n - 1));
    }
}

inline AST &ASTVector::ast() {
    return ast_;
}

inline AST const &ASTVector::ast() const {
    return ast_;
}

// StringRef

inline StringRef::StringRef(StringVector *vec, size_t index)
: vec_{vec}
, index_{index} { }

inline StringRef &StringRef::operator=(char const *str) {
    vec_->set(vec_->begin() + index_, str);
    return *this;
}

inline char const *StringRef::get() const {
    return static_cast<StringVector const *>(vec_)->at(index_);
}

inline StringRef::operator char const *() const {
    return get();
}

// StringVector

inline StringVector::StringVector(AST ast, clingo_ast_attribute_t attr)
: ast_{std::move(ast)}
, attr_{attr} { }

inline StringVector::iterator StringVector::begin() {
    return iterator{this, 0};
}

inline StringVector::iterator StringVector::end() {
    return iterator{this, size()};
}

inline StringVector::const_iterator StringVector::begin() const {
    return const_iterator{this, 0};
}

inline StringVector::const_iterator StringVector::end() const {
    return const_iterator{this, size()};
}

inline size_t StringVector::size() const {
    size_t ret;
    Detail::handle_error(clingo_ast_attribute_size_string_array(ast_.to_c(), attr_, &ret));
    return ret;
}

inline bool StringVector::empty() const {
    return size() == 0;
}

inline StringVector::iterator StringVector::insert(iterator it, char const *str) {
    Detail::handle_error(clingo_ast_attribute_insert_string_at(ast_.to_c(), attr_, it - begin(), str));
    return it;
}

inline StringVector::iterator StringVector::erase(iterator it) {
    Detail::handle_error(clingo_ast_attribute_delete_string_at(ast_.to_c(), attr_, it - begin()));
    return it;
}

inline StringRef StringVector::at(size_t idx)  {
    return {this, idx};
}

inline char const *StringVector::at(size_t idx) const {
    char const *ret;
    Detail::handle_error(clingo_ast_attribute_get_string_at(ast_.to_c(), attr_, idx, &ret));
    return ret;
}

inline void StringVector::push_back(char const *str) {
    insert(end(), str);
}

inline void StringVector::pop_back() {
    erase(end() - 1);
}

inline void StringVector::clear() {
    for (size_t n = size(); n > 0; --n) {
        Detail::handle_error(clingo_ast_attribute_delete_string_at(ast_.to_c(), attr_, n - 1));
    }
}

inline AST &StringVector::ast() {
    return ast_;
}

inline AST const &StringVector::ast() const {
    return ast_;
}

// ProgramBuilder

inline ProgramBuilder::ProgramBuilder(Control &ctl)
: builder_{nullptr} {
    Detail::handle_error(clingo_control_program_builder(ctl.to_c(), &builder_));
    Detail::handle_error(clingo_program_builder_begin(builder_));
}

inline ProgramBuilder::ProgramBuilder(clingo_program_builder_t *builder)
: builder_{builder} {
    Detail::handle_error(clingo_program_builder_begin(builder_));
}

inline ProgramBuilder::ProgramBuilder(ProgramBuilder &&builder) noexcept
: builder_{nullptr} {
    std::swap(builder_, builder.builder_);
}

inline ProgramBuilder::~ProgramBuilder() {
    if (builder_ != nullptr) {
        Detail::handle_error(clingo_program_builder_end(builder_));
    }
}

inline void ProgramBuilder::add(AST const &ast) {
    Detail::handle_error(clingo_program_builder_add_ast(builder_, ast.to_c()));
}

// functions

template <class Callback>
inline void parse_string(char const *program, Callback &&cb, Logger logger, unsigned message_limit) {
    using Data = std::pair<Callback&, std::exception_ptr>;
    Data data(cb, nullptr);
    Detail::handle_error(clingo_ast_parse_string(program, [](clingo_ast_t *ast, void *data) -> bool {
        auto &d = *static_cast<Data*>(data);
        clingo_ast_acquire(ast);
        CLINGO_CALLBACK_TRY { d.first(AST{ast}); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit), data.second);
}

template <class Callback>
inline void parse_files(StringSpan files, Callback cb, Logger logger, unsigned message_limit) {
    using Data = std::pair<Callback&, std::exception_ptr>;
    Data data(cb, nullptr);
    Detail::handle_error(clingo_ast_parse_files(files.begin(), files.size(), [](clingo_ast_t *ast, void *data) -> bool {
        auto &d = *static_cast<Data*>(data);
        clingo_ast_acquire(ast);
        CLINGO_CALLBACK_TRY { d.first(AST{ast}); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit), data.second);
}

} // namespace ASTv2

// {{{2 program builder

namespace AST { namespace Detail {

struct ASTToC {
    // {{{3 term
    ASTToC() = default;
    ASTToC(ASTToC const &) = delete;
    ASTToC(ASTToC &&) noexcept = delete;
    ASTToC &operator=(ASTToC const &) = delete;
    ASTToC &operator=(ASTToC &&) noexcept = delete;

    clingo_ast_id_t convId(Id const &id) {
        return {id.location, id.id};
    }

    struct TermTag {};

    clingo_ast_term_t visit(Symbol const &x, TermTag) {
        clingo_ast_term_t ret;
        ret.type     = clingo_ast_term_type_symbol;
        ret.symbol   = x.to_c();
        return ret;
    }
    clingo_ast_term_t visit(Variable const &x, TermTag) {
        clingo_ast_term_t ret;
        ret.type     = clingo_ast_term_type_variable;
        ret.variable = x.name;
        return ret;
    }
    clingo_ast_term_t visit(UnaryOperation const &x, TermTag) {
        auto *unary_operation = create_<clingo_ast_unary_operation_t>();
        unary_operation->unary_operator = static_cast<clingo_ast_unary_operator_t>(x.unary_operator);
        unary_operation->argument       = convTerm(x.argument);
        clingo_ast_term_t ret;
        ret.type            = clingo_ast_term_type_unary_operation;
        ret.unary_operation = unary_operation;
        return ret;
    }
    clingo_ast_term_t visit(BinaryOperation const &x, TermTag) {
        auto *binary_operation = create_<clingo_ast_binary_operation_t>();
        binary_operation->binary_operator = static_cast<clingo_ast_binary_operator_t>(x.binary_operator);
        binary_operation->left            = convTerm(x.left);
        binary_operation->right           = convTerm(x.right);
        clingo_ast_term_t ret;
        ret.type             = clingo_ast_term_type_binary_operation;
        ret.binary_operation = binary_operation;
        return ret;
    }
    clingo_ast_term_t visit(Interval const &x, TermTag) {
        auto *interval = create_<clingo_ast_interval_t>();
        interval->left  = convTerm(x.left);
        interval->right = convTerm(x.right);
        clingo_ast_term_t ret;
        ret.type     = clingo_ast_term_type_interval;
        ret.interval = interval;
        return ret;
    }
    clingo_ast_term_t visit(Function const &x, TermTag) {
        auto *function = create_<clingo_ast_function_t>();
        function->name      = x.name;
        function->arguments = convTermVec(x.arguments);
        function->size      = x.arguments.size();
        clingo_ast_term_t ret;
        ret.type     = x.external ? clingo_ast_term_type_external_function : clingo_ast_term_type_function;
        ret.function = function;
        return ret;
    }
    clingo_ast_term_t visit(Pool const &x, TermTag) {
        auto *pool = create_<clingo_ast_pool_t>();
        pool->arguments = convTermVec(x.arguments);
        pool->size      = x.arguments.size();
        clingo_ast_term_t ret;
        ret.type     = clingo_ast_term_type_pool;
        ret.pool     = pool;
        return ret;
    }
    clingo_ast_term_t convTerm(Term const &x) {
        auto ret = x.data.accept(*this, TermTag{});
        ret.location = x.location;
        return ret;
    }
    clingo_ast_term_t *convTerm(Optional<Term> const &x) {
        return x ? create_(convTerm(*x)) : nullptr;
    }
    clingo_ast_term_t *convTermVec(std::vector<Term> const &x) {
        return createArray_(x, static_cast<clingo_ast_term_t (ASTToC::*)(Term const &)>(&ASTToC::convTerm));
    }

    clingo_ast_csp_product_term_t convCSPProduct(CSPProduct const &x) {
        clingo_ast_csp_product_term_t ret;
        ret.location    = x.location;
        ret.variable    = convTerm(x.variable);
        ret.coefficient = convTerm(x.coefficient);
        return ret;
    }
    clingo_ast_csp_sum_term_t convCSPAdd(CSPSum const &x) {
        clingo_ast_csp_sum_term_t ret;
        ret.location = x.location;
        ret.terms    = createArray_(x.terms, &ASTToC::convCSPProduct);
        ret.size     = x.terms.size();
        return ret;
    }

    clingo_ast_theory_unparsed_term_element_t convTheoryUnparsedTermElement(TheoryUnparsedTermElement const &x) {
        clingo_ast_theory_unparsed_term_element_t ret;
        ret.term      = convTheoryTerm(x.term);
        ret.operators = createArray_(x.operators, &ASTToC::identity<char const *>);
        ret.size      = x.operators.size();
        return ret;
    }

    struct TheoryTermTag { };

    clingo_ast_theory_term_t visit(Symbol const &term, TheoryTermTag) {
        clingo_ast_theory_term_t ret;
        ret.type     = clingo_ast_theory_term_type_symbol;
        ret.symbol   = term.to_c();
        return ret;
    }
    clingo_ast_theory_term_t visit(Variable const &term, TheoryTermTag) {
        clingo_ast_theory_term_t ret;
        ret.type     = clingo_ast_theory_term_type_variable;
        ret.variable = term.name;
        return ret;
    }
    clingo_ast_theory_term_t visit(TheoryTermSequence const &term, TheoryTermTag) {
        auto *sequence = create_<clingo_ast_theory_term_array_t>();
        sequence->terms = convTheoryTermVec(term.terms);
        sequence->size  = term.terms.size();
        clingo_ast_theory_term_t ret;
        switch (term.type) {
            case TheoryTermSequenceType::Set:   { ret.type = clingo_ast_theory_term_type_set; break; }
            case TheoryTermSequenceType::List:  { ret.type = clingo_ast_theory_term_type_list; break; }
            case TheoryTermSequenceType::Tuple: { ret.type = clingo_ast_theory_term_type_tuple; break; }
        }
        ret.set = sequence;
        return ret;
    }
    clingo_ast_theory_term_t visit(TheoryFunction const &term, TheoryTermTag) {
        auto *function = create_<clingo_ast_theory_function_t>();
        function->name      = term.name;
        function->arguments = convTheoryTermVec(term.arguments);
        function->size      = term.arguments.size();
        clingo_ast_theory_term_t ret;
        ret.type     = clingo_ast_theory_term_type_function;
        ret.function = function;
        return ret;
    }
    clingo_ast_theory_term_t visit(TheoryUnparsedTerm const &term, TheoryTermTag) {
        auto *unparsed_term = create_<clingo_ast_theory_unparsed_term>();
        unparsed_term->elements = createArray_(term.elements, &ASTToC::convTheoryUnparsedTermElement);
        unparsed_term->size     = term.elements.size();
        clingo_ast_theory_term_t ret;
        ret.type          = clingo_ast_theory_term_type_unparsed_term;
        ret.unparsed_term = unparsed_term;
        return ret;
    }
    clingo_ast_theory_term_t convTheoryTerm(TheoryTerm const &x) {
        auto ret = x.data.accept(*this, TheoryTermTag{});
        ret.location = x.location;
        return ret;
    }
    clingo_ast_theory_term_t *convTheoryTermVec(std::vector<TheoryTerm> const &x) {
        return createArray_(x, &ASTToC::convTheoryTerm);
    }

    // {{{3 literal

    clingo_ast_csp_guard_t convCSPGuard(CSPGuard const &x) {
        clingo_ast_csp_guard_t ret;
        ret.comparison = static_cast<clingo_ast_comparison_operator_t>(x.comparison);
        ret.term       = convCSPAdd(x.term);
        return ret;
    }

    clingo_ast_literal_t visit(Boolean const &x) {
        clingo_ast_literal_t ret;
        ret.type     = clingo_ast_literal_type_boolean;
        ret.boolean  = x.value;
        return ret;
    }
    clingo_ast_literal_t visit(Term const &x) {
        clingo_ast_literal_t ret;
        ret.type     = clingo_ast_literal_type_symbolic;
        ret.symbol   = create_<clingo_ast_term_t>(convTerm(x));
        return ret;
    }
    clingo_ast_literal_t visit(Comparison const &x) {
        auto *comparison = create_<clingo_ast_comparison_t>();
        comparison->comparison = static_cast<clingo_ast_comparison_operator_t>(x.comparison);
        comparison->left       = convTerm(x.left);
        comparison->right      = convTerm(x.right);
        clingo_ast_literal_t ret;
        ret.type       = clingo_ast_literal_type_comparison;
        ret.comparison = comparison;
        return ret;
    }
    clingo_ast_literal_t visit(CSPLiteral const &x) {
        auto *csp = create_<clingo_ast_csp_literal_t>();
        csp->term   = convCSPAdd(x.term);
        csp->guards = createArray_(x.guards, &ASTToC::convCSPGuard);
        csp->size   = x.guards.size();
        clingo_ast_literal_t ret;
        ret.type        = clingo_ast_literal_type_csp;
        ret.csp_literal = csp;
        return ret;
    }
    clingo_ast_literal_t convLiteral(Literal const &x) {
        auto ret = x.data.accept(*this);
        ret.sign     = static_cast<clingo_ast_sign_t>(x.sign);
        ret.location = x.location;
        return ret;
    }
    clingo_ast_literal_t *convLiteralVec(std::vector<Literal> const &x) {
        return createArray_(x, &ASTToC::convLiteral);
    }

    // {{{3 aggregates

    clingo_ast_aggregate_guard_t *convAggregateGuard(Optional<AggregateGuard> const &guard) {
        return guard
            ? create_<clingo_ast_aggregate_guard_t>({static_cast<clingo_ast_comparison_operator_t>(guard->comparison), convTerm(guard->term)})
            : nullptr;
    }

    clingo_ast_conditional_literal_t convConditionalLiteral(ConditionalLiteral const &x) {
        clingo_ast_conditional_literal_t ret;
        ret.literal   = convLiteral(x.literal);
        ret.condition = convLiteralVec(x.condition);
        ret.size      = x.condition.size();
        return ret;
    }

    clingo_ast_theory_guard_t *convTheoryGuard(Optional<TheoryGuard> const &x) {
        return x
            ? create_<clingo_ast_theory_guard_t>({x->operator_name, convTheoryTerm(x->term)})
            : nullptr;
    }

    clingo_ast_theory_atom_element_t convTheoryAtomElement(TheoryAtomElement const &x) {
        clingo_ast_theory_atom_element_t ret;
        ret.tuple          = convTheoryTermVec(x.tuple);
        ret.tuple_size     = x.tuple.size();
        ret.condition      = convLiteralVec(x.condition);
        ret.condition_size = x.condition.size();
        return ret;
    }

    clingo_ast_body_aggregate_element_t convBodyAggregateElement(BodyAggregateElement const &x) {
        clingo_ast_body_aggregate_element_t ret;
        ret.tuple          = convTermVec(x.tuple);
        ret.tuple_size     = x.tuple.size();
        ret.condition      = convLiteralVec(x.condition);
        ret.condition_size = x.condition.size();
        return ret;
    }

    clingo_ast_head_aggregate_element_t convHeadAggregateElement(HeadAggregateElement const &x) {
        clingo_ast_head_aggregate_element_t ret;
        ret.tuple               = convTermVec(x.tuple);
        ret.tuple_size          = x.tuple.size();
        ret.conditional_literal = convConditionalLiteral(x.condition);
        return ret;
    }

    clingo_ast_aggregate_t convAggregate(Aggregate const &x) {
        clingo_ast_aggregate_t ret;
        ret.left_guard  = convAggregateGuard(x.left_guard);
        ret.right_guard = convAggregateGuard(x.right_guard);
        ret.size        = x.elements.size();
        ret.elements    = createArray_(x.elements, &ASTToC::convConditionalLiteral);
        return ret;
    }

    clingo_ast_theory_atom_t convTheoryAtom(TheoryAtom const &x) {
        clingo_ast_theory_atom_t ret;
        ret.term     = convTerm(x.term);
        ret.guard    = convTheoryGuard(x.guard);
        ret.elements = createArray_(x.elements, &ASTToC::convTheoryAtomElement);
        ret.size     = x.elements.size();
        return ret;
    }

    clingo_ast_disjoint_element_t convDisjointElement(DisjointElement const &x) {
        clingo_ast_disjoint_element_t ret;
        ret.location       = x.location;
        ret.tuple          = convTermVec(x.tuple);
        ret.tuple_size     = x.tuple.size();
        ret.term           = convCSPAdd(x.term);
        ret.condition      = convLiteralVec(x.condition);
        ret.condition_size = x.condition.size();
        return ret;
    }

    // {{{3 head literal

    struct HeadLiteralTag { };

    clingo_ast_head_literal_t visit(Literal const &x, HeadLiteralTag) {
        clingo_ast_head_literal_t ret;
        ret.type     = clingo_ast_head_literal_type_literal;
        ret.literal  = create_<clingo_ast_literal_t>(convLiteral(x));
        return ret;
    }
    clingo_ast_head_literal_t visit(Disjunction const &x, HeadLiteralTag) {
        auto *disjunction = create_<clingo_ast_disjunction_t>();
        disjunction->size     = x.elements.size();
        disjunction->elements = createArray_(x.elements, &ASTToC::convConditionalLiteral);
        clingo_ast_head_literal_t ret;
        ret.type        = clingo_ast_head_literal_type_disjunction;
        ret.disjunction = disjunction;
        return ret;
    }
    clingo_ast_head_literal_t visit(Aggregate const &x, HeadLiteralTag) {
        clingo_ast_head_literal_t ret;
        ret.type      = clingo_ast_head_literal_type_aggregate;
        ret.aggregate = create_<clingo_ast_aggregate_t>(convAggregate(x));
        return ret;
    }
    clingo_ast_head_literal_t visit(HeadAggregate const &x, HeadLiteralTag) {
        auto *head_aggregate = create_<clingo_ast_head_aggregate_t>();
        head_aggregate->left_guard  = convAggregateGuard(x.left_guard);
        head_aggregate->right_guard = convAggregateGuard(x.right_guard);
        head_aggregate->function    = static_cast<clingo_ast_aggregate_function_t>(x.function);
        head_aggregate->size        = x.elements.size();
        head_aggregate->elements    = createArray_(x.elements, &ASTToC::convHeadAggregateElement);
        clingo_ast_head_literal_t ret;
        ret.type           = clingo_ast_head_literal_type_head_aggregate;
        ret.head_aggregate = head_aggregate;
        return ret;
    }
    clingo_ast_head_literal_t visit(TheoryAtom const &x, HeadLiteralTag) {
        clingo_ast_head_literal_t ret;
        ret.type        = clingo_ast_head_literal_type_theory_atom;
        ret.theory_atom = create_<clingo_ast_theory_atom_t>(convTheoryAtom(x));
        return ret;
    }
    clingo_ast_head_literal_t convHeadLiteral(HeadLiteral const &lit) {
        auto ret = lit.data.accept(*this, HeadLiteralTag{});
        ret.location = lit.location;
        return ret;
    }

    // {{{3 body literal

    struct BodyLiteralTag { };

    clingo_ast_body_literal_t visit(Literal const &x, BodyLiteralTag) {
        clingo_ast_body_literal_t ret;
        ret.type     = clingo_ast_body_literal_type_literal;
        ret.literal  = create_<clingo_ast_literal_t>(convLiteral(x));
        return ret;
    }
    clingo_ast_body_literal_t visit(ConditionalLiteral const &x, BodyLiteralTag) {
        clingo_ast_body_literal_t ret;
        ret.type        = clingo_ast_body_literal_type_conditional;
        ret.conditional = create_<clingo_ast_conditional_literal_t>(convConditionalLiteral(x));
        return ret;
    }
    clingo_ast_body_literal_t visit(Aggregate const &x, BodyLiteralTag) {
        clingo_ast_body_literal_t ret;
        ret.type      = clingo_ast_body_literal_type_aggregate;
        ret.aggregate = create_<clingo_ast_aggregate_t>(convAggregate(x));
        return ret;
    }
    clingo_ast_body_literal_t visit(BodyAggregate const &x, BodyLiteralTag) {
        auto *body_aggregate = create_<clingo_ast_body_aggregate_t>();
        body_aggregate->left_guard  = convAggregateGuard(x.left_guard);
        body_aggregate->right_guard = convAggregateGuard(x.right_guard);
        body_aggregate->function    = static_cast<clingo_ast_aggregate_function_t>(x.function);
        body_aggregate->size        = x.elements.size();
        body_aggregate->elements    = createArray_(x.elements, &ASTToC::convBodyAggregateElement);
        clingo_ast_body_literal_t ret;
        ret.type     = clingo_ast_body_literal_type_body_aggregate;
        ret.body_aggregate = body_aggregate;
        return ret;
    }
    clingo_ast_body_literal_t visit(TheoryAtom const &x, BodyLiteralTag) {
        clingo_ast_body_literal_t ret;
        ret.type        = clingo_ast_body_literal_type_theory_atom;
        ret.theory_atom = create_<clingo_ast_theory_atom_t>(convTheoryAtom(x));
        return ret;
    }
    clingo_ast_body_literal_t visit(Disjoint const &x, BodyLiteralTag) {
        auto *disjoint = create_<clingo_ast_disjoint_t>();
        disjoint->size     = x.elements.size();
        disjoint->elements = createArray_(x.elements, &ASTToC::convDisjointElement);
        clingo_ast_body_literal_t ret;
        ret.type     = clingo_ast_body_literal_type_disjoint;
        ret.disjoint = disjoint;
        return ret;
    }

    clingo_ast_body_literal_t convBodyLiteral(BodyLiteral const &x) {
        auto ret = x.data.accept(*this, BodyLiteralTag{});
        ret.sign     = static_cast<clingo_ast_sign_t>(x.sign);
        ret.location = x.location;
        return ret;
    }

    clingo_ast_body_literal_t* convBodyLiteralVec(std::vector<BodyLiteral> const &x) {
        return createArray_(x, &ASTToC::convBodyLiteral);
    }

    // {{{3 theory definitions

    clingo_ast_theory_operator_definition_t convTheoryOperatorDefinition(TheoryOperatorDefinition const &x) {
        clingo_ast_theory_operator_definition_t ret;
        ret.type     = static_cast<clingo_ast_theory_operator_type_t>(x.type);
        ret.priority = x.priority;
        ret.location = x.location;
        ret.name     = x.name;
        return ret;
    }

    clingo_ast_theory_term_definition_t convTheoryTermDefinition(TheoryTermDefinition const &x) {
        clingo_ast_theory_term_definition_t ret;
        ret.name      = x.name;
        ret.location  = x.location;
        ret.operators = createArray_(x.operators, &ASTToC::convTheoryOperatorDefinition);
        ret.size      = x.operators.size();
        return ret;
    }

    clingo_ast_theory_guard_definition_t convTheoryGuardDefinition(TheoryGuardDefinition const &x) {
        clingo_ast_theory_guard_definition_t ret;
        ret.term      = x.term;
        ret.operators = createArray_(x.operators, &ASTToC::identity<char const *>);
        ret.size      = x.operators.size();
        return ret;
    }

    clingo_ast_theory_atom_definition_t convTheoryAtomDefinition(TheoryAtomDefinition const &x) {
        clingo_ast_theory_atom_definition_t ret;
        ret.name     = x.name;
        ret.arity    = x.arity;
        ret.location = x.location;
        ret.type     = static_cast<clingo_ast_theory_atom_definition_type_t>(x.type);
        ret.elements = x.elements;
        ret.guard    = x.guard ? create_<clingo_ast_theory_guard_definition_t>(convTheoryGuardDefinition(*x.guard)) : nullptr;
        return ret;
    }

    // {{{3 statement

    clingo_ast_statement_t visit(Rule const &x) {
        auto *rule = create_<clingo_ast_rule_t>();
        rule->head = convHeadLiteral(x.head);
        rule->size = x.body.size();
        rule->body = convBodyLiteralVec(x.body);
        clingo_ast_statement_t ret;
        ret.type     = clingo_ast_statement_type_rule;
        ret.rule     = rule;
        return ret;
    }
    clingo_ast_statement_t visit(Definition const &x) {
        auto *definition = create_<clingo_ast_definition_t>();
        definition->is_default = x.is_default;
        definition->name       = x.name;
        definition->value      = convTerm(x.value);
        clingo_ast_statement_t ret;
        ret.type       = clingo_ast_statement_type_const;
        ret.definition = definition;
        return ret;
    }
    clingo_ast_statement_t visit(ShowSignature const &x) {
        auto *show_signature = create_<clingo_ast_show_signature_t>();
        show_signature->csp       = x.csp;
        show_signature->signature = x.signature.to_c();
        clingo_ast_statement_t ret;
        ret.type           = clingo_ast_statement_type_show_signature;
        ret.show_signature = show_signature;
        return ret;
    }
    clingo_ast_statement_t visit(Defined const &x) {
        auto *defined = create_<clingo_ast_defined_t>();
        defined->signature = x.signature.to_c();
        clingo_ast_statement_t ret;
        ret.type  = clingo_ast_statement_type_defined;
        ret.defined = defined;
        return ret;
    }
    clingo_ast_statement_t visit(ShowTerm const &x) {
        auto *show_term = create_<clingo_ast_show_term_t>();
        show_term->csp  = x.csp;
        show_term->term = convTerm(x.term);
        show_term->body = convBodyLiteralVec(x.body);
        show_term->size = x.body.size();
        clingo_ast_statement_t ret;
        ret.type      = clingo_ast_statement_type_show_term;
        ret.show_term = show_term;
        return ret;
    }
    clingo_ast_statement_t visit(Minimize const &x) {
        auto *minimize = create_<clingo_ast_minimize_t>();
        minimize->weight     = convTerm(x.weight);
        minimize->priority   = convTerm(x.priority);
        minimize->tuple      = convTermVec(x.tuple);
        minimize->tuple_size = x.tuple.size();
        minimize->body       = convBodyLiteralVec(x.body);
        minimize->body_size  = x.body.size();
        clingo_ast_statement_t ret;
        ret.type     = clingo_ast_statement_type_minimize;
        ret.minimize = minimize;
        return ret;
    }
    clingo_ast_statement_t visit(Script const &x) {
        auto *script = create_<clingo_ast_script_t>();
        script->type = static_cast<clingo_ast_script_type_t>(x.type);
        script->code = x.code;
        clingo_ast_statement_t ret;
        ret.type   = clingo_ast_statement_type_script;
        ret.script = script;
        return ret;
    }
    clingo_ast_statement_t visit(Program const &x) {
        auto *program = create_<clingo_ast_program_t>();
        program->name       = x.name;
        program->parameters = createArray_(x.parameters, &ASTToC::convId);
        program->size       = x.parameters.size();
        clingo_ast_statement_t ret;
        ret.type    = clingo_ast_statement_type_program;
        ret.program = program;
        return ret;
    }
    clingo_ast_statement_t visit(External const &x) {
        auto *external = create_<clingo_ast_external_t>();
        external->atom = convTerm(x.atom);
        external->body = convBodyLiteralVec(x.body);
        external->size = x.body.size();
        external->type = convTerm(x.type);
        clingo_ast_statement_t ret;
        ret.type     = clingo_ast_statement_type_external;
        ret.external = external;
        return ret;
    }
    clingo_ast_statement_t visit(Edge const &x) {
        auto *edge = create_<clingo_ast_edge_t>();
        edge->u    = convTerm(x.u);
        edge->v    = convTerm(x.v);
        edge->body = convBodyLiteralVec(x.body);
        edge->size = x.body.size();
        clingo_ast_statement_t ret;
        ret.type = clingo_ast_statement_type_edge;
        ret.edge = edge;
        return ret;
    }
    clingo_ast_statement_t visit(Heuristic const &x) {
        auto *heuristic = create_<clingo_ast_heuristic_t>();
        heuristic->atom     = convTerm(x.atom);
        heuristic->bias     = convTerm(x.bias);
        heuristic->priority = convTerm(x.priority);
        heuristic->modifier = convTerm(x.modifier);
        heuristic->body     = convBodyLiteralVec(x.body);
        heuristic->size     = x.body.size();
        clingo_ast_statement_t ret;
        ret.type      = clingo_ast_statement_type_heuristic;
        ret.heuristic = heuristic;
        return ret;
    }
    clingo_ast_statement_t visit(ProjectAtom const &x) {
        auto *project = create_<clingo_ast_project_t>();
        project->atom = convTerm(x.atom);
        project->body = convBodyLiteralVec(x.body);
        project->size = x.body.size();
        clingo_ast_statement_t ret;
        ret.type         = clingo_ast_statement_type_project_atom;
        ret.project_atom = project;
        return ret;
    }
    clingo_ast_statement_t visit(ProjectSignature const &x) {
        clingo_ast_statement_t ret;
        ret.type              = clingo_ast_statement_type_project_atom_signature;
        ret.project_signature = x.signature.to_c();
        return ret;
    }
    clingo_ast_statement_t visit(TheoryDefinition const &x) {
        auto *theory_definition = create_<clingo_ast_theory_definition_t>();
        theory_definition->name       = x.name;
        theory_definition->terms = createArray_(x.terms, &ASTToC::convTheoryTermDefinition);
        theory_definition->terms_size = x.terms.size();
        theory_definition->atoms = createArray_(x.atoms, &ASTToC::convTheoryAtomDefinition);
        theory_definition->atoms_size = x.atoms.size();
        clingo_ast_statement_t ret;
        ret.type = clingo_ast_statement_type_theory_definition;
        ret.theory_definition = theory_definition;
        return ret;
    }

    // {{{3 aux

    template <class T>
    T identity(T t) { return t; }

    template <class T>
    T *create_() {
        data_.emplace_back(operator new(sizeof(T)));
        return reinterpret_cast<T*>(data_.back());
    }
    template <class T>
    T *create_(T x) {
        auto *r = create_<T>();
        *r = x;
        return r;
    }
    template <class T>
    T *createArray_(size_t size) {
        arrdata_.emplace_back(operator new[](sizeof(T) * size));
        return reinterpret_cast<T*>(arrdata_.back());
    }
    template <class T, class F>
    auto createArray_(std::vector<T> const &vec, F f) -> decltype((this->*f)(std::declval<T>()))* {
        using U = decltype((this->*f)(std::declval<T>()));
        auto *r = createArray_<U>(vec.size());
        auto *jt = r;
        for (auto it = vec.begin(), ie = vec.end(); it != ie; ++it, ++jt) { *jt = (this->*f)(*it); }
        return r;
    }

    ~ASTToC() noexcept {
        for (auto &x : data_) { operator delete(x); }
        for (auto &x : arrdata_) { operator delete[](x); }
        data_.clear();
        arrdata_.clear();
    }

    std::vector<void *> data_;
    std::vector<void *> arrdata_;

    // }}}3
};

} } // namespace AST Detail

inline ProgramBuilder::ProgramBuilder(clingo_program_builder_t *builder)
: builder_{builder} {
    Detail::handle_error(clingo_program_builder_begin(builder_));
}

inline ProgramBuilder::ProgramBuilder(ProgramBuilder &&builder) noexcept
: builder_{nullptr} {
    std::swap(builder_, builder.builder_);
}

inline ProgramBuilder::~ProgramBuilder() {
    if (builder_ != nullptr) {
        Detail::handle_error(clingo_program_builder_end(builder_));
    }
}

inline void ProgramBuilder::add(AST::Statement const &stm) {
    AST::Detail::ASTToC a;
    auto x = stm.data.accept(a);
    x.location = stm.location;
    Detail::handle_error(clingo_program_builder_add(builder_, &x));
}

// {{{2 control

struct Control::Impl {
    Impl() = delete;
    Impl(Impl const &) = delete;
    Impl(Impl &&) noexcept = delete;
    Impl &operator=(Impl const &) = delete;
    Impl &operator=(Impl &&) noexcept = delete;
    Impl(Logger logger)
    : ctl(nullptr)
    , handler(nullptr)
    , logger(std::move(logger))
    , owns(true) { }
    Impl(clingo_control_t *ctl, bool owns)
    : ctl(ctl)
    , handler(nullptr)
    , owns(owns) { }
    ~Impl() {
        if (ctl != nullptr && owns) { clingo_control_free(ctl); }
    }
    operator clingo_control_t *() const { return ctl; }
    clingo_control_t *ctl;
    SolveEventHandler *handler;
    Detail::AssignOnce ptr;
    Logger logger;
    std::forward_list<std::pair<Propagator&, Detail::AssignOnce&>> propagators_;
    std::forward_list<std::pair<GroundProgramObserver&, Detail::AssignOnce&>> observers_;
    bool owns;
};

inline Clingo::Control::Control(StringSpan args, Logger logger, unsigned message_limit)
: impl_(new Clingo::Control::Impl(std::move(logger)))
{
    clingo_logger_t f = [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    };
    Detail::handle_error(clingo_control_new(args.begin(), args.size(), impl_->logger ? f : nullptr, impl_->logger ? &impl_->logger : nullptr, message_limit, &impl_->ctl));
}

inline Control::Control(clingo_control_t *ctl, bool owns)
    : impl_(new Impl(ctl, owns)) { }

inline Control::Control(Control &&c) noexcept
: impl_(nullptr) {
    *this = std::move(c);
}

inline Control &Control::operator=(Control &&c) noexcept {
    std::swap(impl_, c.impl_);
    return *this;
}

inline Control::~Control() noexcept {
    delete impl_;
}

inline void Control::add(char const *name, StringSpan params, char const *part) {
    Detail::handle_error(clingo_control_add(*impl_, name, params.begin(), params.size(), part));
}

inline void Control::ground(PartSpan parts, GroundCallback cb) {
    using Data = std::pair<GroundCallback&, Detail::AssignOnce&>;
    Data data(cb, impl_->ptr);
    impl_->ptr.reset();
    Detail::handle_error(clingo_control_ground(*impl_, reinterpret_cast<clingo_part_t const *>(parts.begin()), parts.size(),
        [](clingo_location_t const *loc, char const *name, clingo_symbol_t const *args, size_t n, void *data, clingo_symbol_callback_t cb, void *cbdata) -> bool {
            auto &d = *static_cast<Data*>(data);
            CLINGO_CALLBACK_TRY {
                if (d.first) {
                    struct Ret : std::exception { };
                    try {
                        d.first(Location(*loc), name, {reinterpret_cast<Symbol const *>(args), n}, [cb, cbdata](SymbolSpan symret) {
                            if (!cb(reinterpret_cast<clingo_symbol_t const *>(symret.begin()), symret.size(), cbdata)) { throw Ret(); }
                        });
                    }
                    catch (Ret const &e) { return false; }
                }
            }
            CLINGO_CALLBACK_CATCH(d.second);
        }, &data), data.second);
}

inline clingo_control_t *Control::to_c() const { return *impl_; }

inline SolveHandle Control::solve(SymbolicLiteralSpan assumptions, SolveEventHandler *handler, bool asynchronous, bool yield) {
    std::vector<literal_t> lits;
    auto atoms = symbolic_atoms();
    for (auto const &x : assumptions) {
        auto it = atoms.find(x.symbol());
        if (it != atoms.end()) {
            auto lit = it->literal();
            lits.emplace_back(x.is_positive() ? lit : -lit);
        }
        else if (x.is_positive()) {
            lits.emplace_back(1);
            lits.emplace_back(-1);
        }
    }
    return solve(LiteralSpan{lits}, handler, asynchronous, yield);
}

inline SolveHandle Control::solve(LiteralSpan assumptions, SolveEventHandler *handler, bool asynchronous, bool yield) {
    clingo_solve_handle_t *it;
    clingo_solve_mode_bitset_t mode = 0;
    if (asynchronous) { mode |= clingo_solve_mode_async; }
    if (yield) { mode |= clingo_solve_mode_yield; }
    impl_->handler = handler;
    impl_->ptr.reset();
    clingo_solve_event_callback_t on_event = [](clingo_solve_event_type_t type, void *event, void *pdata, bool *goon) {
        Impl &data = *static_cast<Impl*>(pdata);
        switch (type) {
            case clingo_solve_event_type_model: {
                CLINGO_CALLBACK_TRY {
                    Model m{static_cast<clingo_model_t*>(event)};
                    *goon = data.handler->on_model(m);
                }
                CLINGO_CALLBACK_CATCH(data.ptr);
            }
            case clingo_solve_event_type_statistics: {
                CLINGO_CALLBACK_TRY {
                    auto **stats = static_cast<clingo_statistics_t**>(event);
                    uint64_t step_root, accu_root;
                    Detail::handle_error(clingo_statistics_root(stats[0], &step_root));
                    Detail::handle_error(clingo_statistics_root(stats[1], &accu_root));
                    data.handler->on_statistics(UserStatistics{stats[0], step_root}, UserStatistics{stats[1], accu_root});
                    *goon = true;
                    return true;
                }
                CLINGO_CALLBACK_CATCH(data.ptr);
            }
            case clingo_solve_event_type_finish: {
                CLINGO_CALLBACK_TRY {
                    data.handler->on_finish(SolveResult{*static_cast<clingo_solve_result_bitset_t*>(event)});
                    *goon = true;
                    return true;
                }
                CLINGO_CALLBACK_CATCH(data.ptr);
            }
        }
        return false;
    };
    Detail::handle_error(clingo_control_solve(*impl_, mode, assumptions.begin(), assumptions.size(), handler != nullptr ? on_event : nullptr, impl_, &it), impl_->ptr);
    return SolveHandle{it, impl_->ptr};
}

inline void Control::assign_external(literal_t literal, TruthValue value) {
    Detail::handle_error(clingo_control_assign_external(*impl_, literal, static_cast<clingo_truth_value_t>(value)));
}

inline void Control::assign_external(Symbol atom, TruthValue value) {
    auto atoms = symbolic_atoms();
    auto it = atoms.find(atom);
    if (it != atoms.end()) { assign_external(it->literal(), value); }
}

inline void Control::release_external(literal_t literal) {
    Detail::handle_error(clingo_control_release_external(*impl_, literal));
}

inline void Control::release_external(Symbol atom) {
    auto atoms = symbolic_atoms();
    auto it = atoms.find(atom);
    if (it != atoms.end()) { release_external(it->literal()); }
}

inline SymbolicAtoms Control::symbolic_atoms() const {
    clingo_symbolic_atoms_t const *ret;
    Detail::handle_error(clingo_control_symbolic_atoms(*impl_, &ret));
    return SymbolicAtoms{ret};
}

inline TheoryAtoms Control::theory_atoms() const {
    clingo_theory_atoms_t const *ret;
    clingo_control_theory_atoms(*impl_, &ret);
    return TheoryAtoms{ret};
}

namespace Detail {

using PropagatorData = std::pair<Propagator&, AssignOnce&>;

inline static bool g_init(clingo_propagate_init_t *ctl, void *pdata) {
    PropagatorData &data = *static_cast<PropagatorData*>(pdata);
    CLINGO_CALLBACK_TRY {
        PropagateInit pi(ctl);
        data.first.init(pi);
    }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline static bool g_propagate(clingo_propagate_control_t *ctl, clingo_literal_t const *changes, size_t n, void *pdata) {
    PropagatorData &data = *static_cast<PropagatorData*>(pdata);
    CLINGO_CALLBACK_TRY {
        PropagateControl pc(ctl);
        data.first.propagate(pc, {changes, n});
    }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline static void g_undo(clingo_propagate_control_t const *ctl, clingo_literal_t const *changes, size_t n, void *pdata) {
    PropagatorData &data = *static_cast<PropagatorData*>(pdata);
    PropagateControl pc(const_cast<clingo_propagate_control_t*>(ctl));
    data.first.undo(pc, {changes, n});
}

inline static bool g_check(clingo_propagate_control_t *ctl, void *pdata) {
    PropagatorData &data = *static_cast<PropagatorData*>(pdata);
    CLINGO_CALLBACK_TRY {
        PropagateControl pc(ctl);
        data.first.check(pc);
    }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline static bool g_decide(clingo_id_t ti, clingo_assignment_t const *a,  clingo_literal_t f, void *pdata, clingo_literal_t *l) {
    PropagatorData &data = *static_cast<PropagatorData*>(pdata);
    CLINGO_CALLBACK_TRY {
        Assignment ass{a};
        *l = static_cast<Heuristic&>(data.first).decide(ti, ass, f);
    }
    CLINGO_CALLBACK_CATCH(data.second);
}

} // namespace Detail

inline void Control::register_propagator(Propagator &propagator, bool sequential) {
    impl_->propagators_.emplace_front(propagator, impl_->ptr);
    static clingo_propagator_t g_propagator = {
        Detail::g_init,
        Detail::g_propagate,
        Detail::g_undo,
        Detail::g_check,
        nullptr,
    };
    Detail::handle_error(clingo_control_register_propagator(*impl_, &g_propagator, &impl_->propagators_.front(), sequential));
}

inline void Control::register_propagator(Heuristic &propagator, bool sequential) {
    impl_->propagators_.emplace_front(propagator, impl_->ptr);
    static clingo_propagator_t g_propagator = {
        Detail::g_init,
        Detail::g_propagate,
        Detail::g_undo,
        Detail::g_check,
        Detail::g_decide,
    };
    Detail::handle_error(clingo_control_register_propagator(*impl_, &g_propagator, &impl_->propagators_.front(), sequential));
}

namespace Detail {

using ObserverData = std::pair<GroundProgramObserver&, AssignOnce&>;

inline bool g_init_program(bool incremental, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.init_program(incremental); }
    CLINGO_CALLBACK_CATCH(data.second);
}
inline bool g_begin_step(void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.begin_step(); }
    CLINGO_CALLBACK_CATCH(data.second);
}
inline bool g_end_step(void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.end_step(); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.rule(choice, AtomSpan(head, head_size), LiteralSpan(body, body_size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_weight_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound, clingo_weighted_literal_t const *body, size_t body_size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.weight_rule(choice, AtomSpan(head, head_size), lower_bound, WeightedLiteralSpan(reinterpret_cast<WeightedLiteral const*>(body), body_size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_minimize(clingo_weight_t priority, clingo_weighted_literal_t const* literals, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.minimize(priority, WeightedLiteralSpan(reinterpret_cast<WeightedLiteral const*>(literals), size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_project(clingo_atom_t const *atoms, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.project(AtomSpan(atoms, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_output_atom(clingo_symbol_t symbol, clingo_atom_t atom, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.output_atom(Symbol{symbol}, atom); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_output_term(clingo_symbol_t symbol, clingo_literal_t const *condition, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.output_term(Symbol{symbol}, LiteralSpan{condition, size}); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_output_csp(clingo_symbol_t symbol, int value, clingo_literal_t const *condition, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.output_csp(Symbol{symbol}, value, LiteralSpan{condition, size}); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_external(clingo_atom_t atom, clingo_external_type_t type, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.external(atom, static_cast<ExternalType>(type)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_assume(clingo_literal_t const *literals, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.assume(LiteralSpan(literals, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_heuristic(clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.heuristic(atom, static_cast<HeuristicType>(type), bias, priority, LiteralSpan(condition, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_acyc_edge(int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.acyc_edge(node_u, node_v, LiteralSpan(condition, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_term_number(clingo_id_t term_id, int number, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_term_number(term_id, number); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_term_string(clingo_id_t term_id, char const *name, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_term_string(term_id, name); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_term_compound(clingo_id_t term_id, int name_id_or_type, clingo_id_t const *arguments, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_term_compound(term_id, name_id_or_type, IdSpan(arguments, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_element(clingo_id_t element_id, clingo_id_t const *terms, size_t terms_size, clingo_literal_t const *condition, size_t condition_size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_element(element_id, IdSpan(terms, terms_size), LiteralSpan(condition, condition_size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_atom(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_atom(atom_id_or_zero, term_id, IdSpan(elements, size)); }
    CLINGO_CALLBACK_CATCH(data.second);
}

inline bool g_theory_atom_with_guard(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, clingo_id_t operator_id, clingo_id_t right_hand_side_id, void *pdata) {
    ObserverData &data = *static_cast<ObserverData*>(pdata);
    CLINGO_CALLBACK_TRY { data.first.theory_atom_with_guard(atom_id_or_zero, term_id, IdSpan(elements, size), operator_id, right_hand_side_id); }
    CLINGO_CALLBACK_CATCH(data.second);
}

} // namespace Detail

inline void Control::register_observer(GroundProgramObserver &observer, bool replace) {
    impl_->observers_.emplace_front(observer, impl_->ptr);
    static clingo_ground_program_observer_t g_observer = {
        Detail::g_init_program,
        Detail::g_begin_step,
        Detail::g_end_step,
        Detail::g_rule,
        Detail::g_weight_rule,
        Detail::g_minimize,
        Detail::g_project,
        Detail::g_output_atom,
        Detail::g_output_term,
        Detail::g_output_csp,
        Detail::g_external,
        Detail::g_assume,
        Detail::g_heuristic,
        Detail::g_acyc_edge,
        Detail::g_theory_term_number,
        Detail::g_theory_term_string,
        Detail::g_theory_term_compound,
        Detail::g_theory_element,
        Detail::g_theory_atom,
        Detail::g_theory_atom_with_guard
    };
    Detail::handle_error(clingo_control_register_observer(*impl_, &g_observer, replace, &impl_->observers_.front()));
}

inline bool Control::is_conflicting() const noexcept {
    return clingo_control_is_conflicting(*impl_);
}

inline bool Control::has_const(char const *name) const {
    bool ret;
    Detail::handle_error(clingo_control_has_const(*impl_, name, &ret));
    return ret;
}

inline Symbol Control::get_const(char const *name) const {
    clingo_symbol_t ret;
    Detail::handle_error(clingo_control_get_const(*impl_, name, &ret));
    return Symbol(ret);
}

inline void Control::interrupt() noexcept {
    clingo_control_interrupt(*impl_);
}

inline void *Control::claspFacade() {
    void *ret;
    Detail::handle_error(clingo_control_clasp_facade(impl_->ctl, &ret));
    return ret;
}

inline void Control::load(char const *file) {
    Detail::handle_error(clingo_control_load(*impl_, file));
}

inline void Control::enable_enumeration_assumption(bool value) {
    Detail::handle_error(clingo_control_set_enable_enumeration_assumption(*impl_, value));
}

inline bool Control::enable_enumeration_assumption() const {
    return clingo_control_get_enable_enumeration_assumption(*impl_);
}

inline void Control::cleanup() {
    Detail::handle_error(clingo_control_cleanup(*impl_));
}

inline void Control::enable_cleanup(bool value) {
    Detail::handle_error(clingo_control_set_enable_cleanup(*impl_, value));
}

inline bool Control::enable_cleanup() const {
    return clingo_control_get_enable_cleanup(*impl_);
}

inline Backend Control::backend() {
    clingo_backend_t *ret;
    Detail::handle_error(clingo_control_backend(*impl_, &ret));
    return Backend{ret};
}

inline Configuration Control::configuration() {
    clingo_configuration_t *conf;
    Detail::handle_error(clingo_control_configuration(*impl_, &conf));
    unsigned key;
    Detail::handle_error(clingo_configuration_root(conf, &key));
    return Configuration{conf, key};
}

inline Statistics Control::statistics() const {
    clingo_statistics_t const *stats;
    Detail::handle_error(clingo_control_statistics(impl_->ctl, &stats), impl_->ptr);
    uint64_t key;
    Detail::handle_error(clingo_statistics_root(stats, &key));
    return Statistics{stats, key};
}

inline ProgramBuilder Control::builder() {
    clingo_program_builder_t *ret;
    Detail::handle_error(clingo_control_program_builder(impl_->ctl, &ret));
    return ProgramBuilder{ret};
}

// {{{2 clingo application

inline void ClingoOptions::add(char const *group, char const *option, char const *description, std::function<bool (char const *value)> parse, bool multi, char const *argument) {
    parsers_.emplace_front(std::move(parse));
    Detail::handle_error(clingo_options_add(to_c(), group, option, description, [](char const *value, void *data) {
        auto& p = *static_cast<Detail::ParserList::value_type*>(data);
        try         { return p(value); }
        catch (...) { return false; }
    }, &parsers_.front(), multi, argument));
}

inline void ClingoOptions::add_flag(char const *group, char const *option, char const *description, bool &target) {
    Detail::handle_error(clingo_options_add_flag(to_c(), group, option, description, &target));
}

inline unsigned Application::message_limit() const noexcept {
    return 20;
}
inline char const *Application::program_name() const noexcept {
    return "clingo";
}
inline char const *Application::version() const noexcept {
    return CLINGO_VERSION;
}
inline void Application::print_model(Model const &, std::function<void()> default_printer) noexcept {
    default_printer();
}
inline void Application::log(WarningCode, char const *message) noexcept {
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
}
inline void Application::register_options(ClingoOptions &) {
}
inline void Application::validate_options() {
}

namespace Detail {

struct ApplicationData {
    Application &app;
    ParserList parsers;
};

inline static unsigned g_message_limit(void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    return data.app.message_limit();
}

inline static char const *g_program_name(void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    return data.app.program_name();
}

inline static char const *g_version(void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    return data.app.version();
}

inline static bool g_main(clingo_control_t *control, char const *const * files, size_t size, void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    CLINGO_TRY {
        Control ctl{control, false};
        data.app.main(ctl, {files, size});
    }
    CLINGO_CATCH;
}

inline static void g_logger(clingo_warning_t code, char const *message, void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    return data.app.log(static_cast<WarningCode>(code), message);
}

inline static bool g_model_printer(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data, void *data) {
    ApplicationData &app_data = *static_cast<ApplicationData*>(data);
    CLINGO_TRY {
        app_data.app.print_model(Model(const_cast<clingo_model_t*>(model)), [&]() {
            Detail::handle_error(printer(printer_data));
        });
    }
    CLINGO_CATCH;
}

inline static bool g_register_options(clingo_options_t *options, void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    CLINGO_TRY {
        ClingoOptions opts{options, data.parsers};
        data.app.register_options(opts);
    }
    CLINGO_CATCH;
}

inline static bool g_validate_options(void *adata) {
    ApplicationData &data = *static_cast<ApplicationData*>(adata);
    CLINGO_TRY { data.app.validate_options(); }
    CLINGO_CATCH;
}

} // namespace

// {{{2 global functions

inline Symbol parse_term(char const *str, Logger logger, unsigned message_limit) {
    clingo_symbol_t ret;
    Detail::handle_error(clingo_parse_term(str, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit, &ret));
    return Symbol(ret);
}

inline char const *add_string(char const *str) {
    char const *ret;
    Detail::handle_error(clingo_add_string(str, &ret));
    return ret;
}

inline std::tuple<int, int, int> version() {
    std::tuple<int, int, int> ret;
    clingo_version(&std::get<0>(ret), &std::get<1>(ret), &std::get<2>(ret));
    return ret;
}

namespace AST {

namespace Detail {

template <class T>
struct PrintWrapper {
    T const &vec;
    char const *pre;
    char const *sep;
    char const *post;
    bool empty;
    friend std::ostream &operator<<(std::ostream &out, PrintWrapper x) {
        using namespace std;
        auto it = std::begin(x.vec);
        auto ie = std::end(x.vec);
        if (it != ie) {
            out << x.pre;
            out << *it;
            for (++it; it != ie; ++it) {
                out << x.sep << *it;
            }
            out << x.post;
        }
        else if (x.empty) {
            out << x.pre;
            out << x.post;
        }
        return out;
    }
};


template <class T>
PrintWrapper<T> print(T const &vec, char const *pre, char const *sep, char const *post, bool empty) {
    return {vec, pre, sep, post, empty};
}

inline PrintWrapper<std::vector<BodyLiteral>> print_body(std::vector<BodyLiteral> const &vec, char const *pre = " : ") {
    return print(vec, vec.empty() ? "" : pre, "; ", ".", true);
}

// {{{3 C -> C++

#define CLINGO_ARRAY(in, out) \
inline std::vector<out> conv ## out ## Vec(in const *arr, size_t size) { \
    std::vector<out> ret; \
    for (auto it = arr, ie = arr + size; it != ie; ++it) { \
        ret.emplace_back(conv ## out(*it)); \
    } \
    return ret; \
}

// {{{4 terms

inline Id convId(clingo_ast_id_t const &id) {
    return {Location(id.location), id.id};
}
CLINGO_ARRAY(clingo_ast_id_t, Id)

inline Term convTerm(clingo_ast_term_t const &term);
CLINGO_ARRAY(clingo_ast_term_t, Term)

inline Term convTerm(clingo_ast_term_t const &term) {
    switch (static_cast<enum clingo_ast_term_type>(term.type)) {
        case clingo_ast_term_type_symbol: {
            return {Location{term.location}, Symbol{term.symbol}};
        }
        case clingo_ast_term_type_variable: {
            return {Location{term.location}, Variable{term.variable}};
        }
        case clingo_ast_term_type_unary_operation: {
            auto const &op = *term.unary_operation;
            return {Location{term.location}, UnaryOperation{static_cast<UnaryOperator>(op.unary_operator), convTerm(op.argument)}};
        }
        case clingo_ast_term_type_binary_operation: {
            auto const &op = *term.binary_operation;
            return {Location{term.location}, BinaryOperation{static_cast<BinaryOperator>(op.binary_operator), convTerm(op.left), convTerm(op.right)}};
        }
        case clingo_ast_term_type_interval: {
            auto const &x = *term.interval;
            return {Location{term.location}, Interval{convTerm(x.left), convTerm(x.right)}};
        }
        case clingo_ast_term_type_function: {
            auto const &x = *term.function;
            return {Location{term.location}, Function{x.name, convTermVec(x.arguments, x.size), false}};
        }
        case clingo_ast_term_type_external_function: {
            auto const &x = *term.external_function;
            return {Location{term.location}, Function{x.name, convTermVec(x.arguments, x.size), true}};
        }
        case clingo_ast_term_type_pool: {
            auto const &x = *term.pool;
            return {Location{term.location}, Pool{convTermVec(x.arguments, x.size)}};
        }
    }
    throw std::logic_error("cannot happen");
}

inline Optional<Term> convTerm(clingo_ast_term_t const *term) {
    return term != nullptr ? Optional<Term>{convTerm(*term)} : Optional<Term>{};
}

// csp

inline CSPProduct convCSPProduct(clingo_ast_csp_product_term const &term) {
    return {Location{term.location}, convTerm(term.coefficient), convTerm(term.variable)};
}
CLINGO_ARRAY(clingo_ast_csp_product_term, CSPProduct)

inline CSPSum convCSPAdd(clingo_ast_csp_sum_term_t const &term) {
    return {Location{term.location}, convCSPProductVec(term.terms, term.size)};
}

// theory

inline TheoryTerm convTheoryTerm(clingo_ast_theory_term_t const &term);
CLINGO_ARRAY(clingo_ast_theory_term_t, TheoryTerm)

inline TheoryUnparsedTermElement convTheoryUnparsedTermElement(clingo_ast_theory_unparsed_term_element_t const &term) {
    return {std::vector<char const *>{term.operators, term.operators + term.size}, convTheoryTerm(term.term)};
}
CLINGO_ARRAY(clingo_ast_theory_unparsed_term_element_t, TheoryUnparsedTermElement)

inline TheoryTerm convTheoryTerm(clingo_ast_theory_term_t const &term) {
    switch (static_cast<enum clingo_ast_theory_term_type>(term.type)) {
        case clingo_ast_theory_term_type_symbol: {
            return {Location{term.location}, Symbol{term.symbol}};
        }
        case clingo_ast_theory_term_type_variable: {
            return {Location{term.location}, Variable{term.variable}};
        }
        case clingo_ast_theory_term_type_list: {
            auto const &x = *term.list;
            return {Location{term.location}, TheoryTermSequence{TheoryTermSequenceType::List, convTheoryTermVec(x.terms, x.size)}};
        }
        case clingo_ast_theory_term_type_set: {
            auto const &x = *term.list;
            return {Location{term.location}, TheoryTermSequence{TheoryTermSequenceType::Set, convTheoryTermVec(x.terms, x.size)}};
        }
        case clingo_ast_theory_term_type_tuple: {
            auto const &x = *term.list;
            return {Location{term.location}, TheoryTermSequence{TheoryTermSequenceType::Tuple, convTheoryTermVec(x.terms, x.size)}};
        }
        case clingo_ast_theory_term_type_function: {
            auto const &x = *term.function;
            return {Location{term.location}, TheoryFunction{x.name, convTheoryTermVec(x.arguments, x.size)}};
        }
        case clingo_ast_theory_term_type_unparsed_term: {
            auto const &x = *term.unparsed_term;
            return {Location{term.location}, TheoryUnparsedTerm{convTheoryUnparsedTermElementVec(x.elements, x.size)}};
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{4 literal

inline CSPGuard convCSPGuard(clingo_ast_csp_guard_t const &guard) {
    return {static_cast<ComparisonOperator>(guard.comparison), convCSPAdd(guard.term)};
}
CLINGO_ARRAY(clingo_ast_csp_guard_t, CSPGuard)

inline Literal convLiteral(clingo_ast_literal_t const &lit) {
    switch (static_cast<enum clingo_ast_literal_type>(lit.type)) {
        case clingo_ast_literal_type_boolean: {
            return {Location(lit.location), static_cast<Sign>(lit.sign), Boolean{lit.boolean}};
        }
        case clingo_ast_literal_type_symbolic: {
            return {Location(lit.location), static_cast<Sign>(lit.sign), convTerm(*lit.symbol)};
        }
        case clingo_ast_literal_type_comparison: {
            auto const &c = *lit.comparison;
            return {Location(lit.location), static_cast<Sign>(lit.sign), Comparison{static_cast<ComparisonOperator>(c.comparison), convTerm(c.left), convTerm(c.right)}};
        }
        case clingo_ast_literal_type_csp: {
            auto const &c = *lit.csp_literal;
            return {Location(lit.location), static_cast<Sign>(lit.sign), CSPLiteral{convCSPAdd(c.term), convCSPGuardVec(c.guards, c.size)}};
        }
    }
    throw std::logic_error("cannot happen");
}
CLINGO_ARRAY(clingo_ast_literal_t, Literal)

// {{{4 aggregates

inline Optional<AggregateGuard> convAggregateGuard(clingo_ast_aggregate_guard_t const *guard) {
    return guard != nullptr
        ? Optional<AggregateGuard>{AggregateGuard{static_cast<ComparisonOperator>(guard->comparison), convTerm(guard->term)}}
        : Optional<AggregateGuard>{};
}

inline ConditionalLiteral convConditionalLiteral(clingo_ast_conditional_literal_t const &lit) {
    return {convLiteral(lit.literal), convLiteralVec(lit.condition, lit.size)};
}
CLINGO_ARRAY(clingo_ast_conditional_literal_t, ConditionalLiteral)

inline Aggregate convAggregate(clingo_ast_aggregate_t const &aggr) {
    return {convConditionalLiteralVec(aggr.elements, aggr.size), convAggregateGuard(aggr.left_guard), convAggregateGuard(aggr.right_guard)};
}

// theory atom

inline Optional<TheoryGuard> convTheoryGuard(clingo_ast_theory_guard_t const *guard) {
    return guard != nullptr
        ? Optional<TheoryGuard>{TheoryGuard{guard->operator_name, convTheoryTerm(guard->term)}}
        : Optional<TheoryGuard>{};
}

inline TheoryAtomElement convTheoryAtomElement(clingo_ast_theory_atom_element_t const &elem) {
    return {convTheoryTermVec(elem.tuple, elem.tuple_size), convLiteralVec(elem.condition, elem.condition_size)};
}
CLINGO_ARRAY(clingo_ast_theory_atom_element_t, TheoryAtomElement)

inline TheoryAtom convTheoryAtom(clingo_ast_theory_atom_t const &atom) {
    return {convTerm(atom.term), convTheoryAtomElementVec(atom.elements, atom.size), convTheoryGuard(atom.guard)};
}

// disjoint

inline DisjointElement convDisjointElement(clingo_ast_disjoint_element_t const &elem) {
    return {Location{elem.location}, convTermVec(elem.tuple, elem.tuple_size), convCSPAdd(elem.term), convLiteralVec(elem.condition, elem.condition_size)};
}
CLINGO_ARRAY(clingo_ast_disjoint_element_t, DisjointElement)

// head aggregates

inline HeadAggregateElement convHeadAggregateElement(clingo_ast_head_aggregate_element_t const &elem) {
    return {convTermVec(elem.tuple, elem.tuple_size), convConditionalLiteral(elem.conditional_literal)};
}
CLINGO_ARRAY(clingo_ast_head_aggregate_element_t, HeadAggregateElement)

// body aggregates

inline BodyAggregateElement convBodyAggregateElement(clingo_ast_body_aggregate_element_t const &elem) {
    return {convTermVec(elem.tuple, elem.tuple_size), convLiteralVec(elem.condition, elem.condition_size)};
}
CLINGO_ARRAY(clingo_ast_body_aggregate_element_t, BodyAggregateElement)

// {{{4 head literal

inline HeadLiteral convHeadLiteral(clingo_ast_head_literal_t const &head) {
    switch (static_cast<enum clingo_ast_head_literal_type>(head.type)) {
        case clingo_ast_head_literal_type_literal: {
            return {Location{head.location}, convLiteral(*head.literal)};
        }
        case clingo_ast_head_literal_type_disjunction: {
            auto const &d = *head.disjunction;
            return {Location{head.location}, Disjunction{convConditionalLiteralVec(d.elements, d.size)}};
        }
        case clingo_ast_head_literal_type_aggregate: {
            return {Location{head.location}, convAggregate(*head.aggregate)};
        }
        case clingo_ast_head_literal_type_head_aggregate: {
            auto const &a = *head.head_aggregate;
            return {Location{head.location}, HeadAggregate{static_cast<AggregateFunction>(a.function), convHeadAggregateElementVec(a.elements, a.size), convAggregateGuard(a.left_guard), convAggregateGuard(a.right_guard)}};
        }
        case clingo_ast_head_literal_type_theory_atom: {
            return {Location{head.location}, convTheoryAtom(*head.theory_atom)};
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{4 body literal

inline BodyLiteral convBodyLiteral(clingo_ast_body_literal_t const &body) {
    switch (static_cast<enum clingo_ast_body_literal_type>(body.type)) {
        case clingo_ast_body_literal_type_literal: {
            return {Location{body.location}, static_cast<Sign>(body.sign), convLiteral(*body.literal)};
        }
        case clingo_ast_body_literal_type_conditional: {
            return {Location{body.location}, static_cast<Sign>(body.sign), convConditionalLiteral(*body.conditional)};
        }
        case clingo_ast_body_literal_type_aggregate: {
            return {Location{body.location}, static_cast<Sign>(body.sign), convAggregate(*body.aggregate)};
        }
        case clingo_ast_body_literal_type_body_aggregate: {
            auto const &a = *body.body_aggregate;
            return {Location{body.location}, static_cast<Sign>(body.sign), BodyAggregate{static_cast<AggregateFunction>(a.function), convBodyAggregateElementVec(a.elements, a.size), convAggregateGuard(a.left_guard), convAggregateGuard(a.right_guard)}};
        }
        case clingo_ast_body_literal_type_theory_atom: {
            return {Location{body.location}, static_cast<Sign>(body.sign), convTheoryAtom(*body.theory_atom)};
        }
        case clingo_ast_body_literal_type_disjoint: {
            auto const &d = *body.disjoint;
            return {Location{body.location}, static_cast<Sign>(body.sign), Disjoint{convDisjointElementVec(d.elements, d.size)}};
        }
    }
    throw std::logic_error("cannot happen");
}
CLINGO_ARRAY(clingo_ast_body_literal_t, BodyLiteral)

// {{{4 statement

inline TheoryOperatorDefinition convTheoryOperatorDefinition(clingo_ast_theory_operator_definition_t const &def) {
    return {Location{def.location}, def.name, def.priority, static_cast<TheoryOperatorType>(def.type)};
}
CLINGO_ARRAY(clingo_ast_theory_operator_definition_t, TheoryOperatorDefinition)

inline Optional<TheoryGuardDefinition> convTheoryGuardDefinition(clingo_ast_theory_guard_definition_t const *def) {
    return def != nullptr
        ? Optional<TheoryGuardDefinition>{TheoryGuardDefinition{def->term, std::vector<char const *>{def->operators, def->operators + def->size}}}
        : Optional<TheoryGuardDefinition>{};
}

inline TheoryTermDefinition convTheoryTermDefinition(clingo_ast_theory_term_definition_t const &def) {
    return {Location{def.location}, def.name, convTheoryOperatorDefinitionVec(def.operators, def.size)};
    std::vector<TheoryOperatorDefinition> operators;
}
CLINGO_ARRAY(clingo_ast_theory_term_definition_t, TheoryTermDefinition)

inline TheoryAtomDefinition convTheoryAtomDefinition(clingo_ast_theory_atom_definition_t const &def) {
    return {Location{def.location}, static_cast<TheoryAtomDefinitionType>(def.type), def.name, def.arity, def.elements, convTheoryGuardDefinition(def.guard)};
}
CLINGO_ARRAY(clingo_ast_theory_atom_definition_t, TheoryAtomDefinition)

inline void convStatement(clingo_ast_statement_t const *stm, StatementCallback &cb) {
    switch (static_cast<enum clingo_ast_statement_type>(stm->type)) {
        case clingo_ast_statement_type_rule: {
            cb({Location(stm->location), Rule{convHeadLiteral(stm->rule->head), convBodyLiteralVec(stm->rule->body, stm->rule->size)}});
            break;
        }
        case clingo_ast_statement_type_const: {
            cb({Location(stm->location), Definition{stm->definition->name, convTerm(stm->definition->value), stm->definition->is_default}});
            break;
        }
        case clingo_ast_statement_type_show_signature: {
            cb({Location(stm->location), ShowSignature{Signature(stm->show_signature->signature), stm->show_signature->csp}});
            break;
        }
        case clingo_ast_statement_type_show_term: {
            cb({Location(stm->location), ShowTerm{convTerm(stm->show_term->term), convBodyLiteralVec(stm->show_term->body, stm->show_term->size), stm->show_term->csp}});
            break;
        }
        case clingo_ast_statement_type_minimize: {
            auto const &min = *stm->minimize;
            cb({Location(stm->location), Minimize{convTerm(min.weight), convTerm(min.priority), convTermVec(min.tuple, min.tuple_size), convBodyLiteralVec(min.body, min.body_size)}});
            break;
        }
        case clingo_ast_statement_type_script: {
            cb({Location(stm->location), Script{static_cast<ScriptType>(stm->script->type), stm->script->code}});
            break;
        }
        case clingo_ast_statement_type_program: {
            cb({Location(stm->location), Program{stm->program->name, convIdVec(stm->program->parameters, stm->program->size)}});
            break;
        }
        case clingo_ast_statement_type_external: {
            cb({Location(stm->location), External{convTerm(stm->external->atom), convBodyLiteralVec(stm->external->body, stm->external->size), convTerm(stm->external->type)}});
            break;
        }
        case clingo_ast_statement_type_edge: {
            cb({Location(stm->location), Edge{convTerm(stm->edge->u), convTerm(stm->edge->v), convBodyLiteralVec(stm->edge->body, stm->edge->size)}});
            break;
        }
        case clingo_ast_statement_type_heuristic: {
            auto const &heu = *stm->heuristic;
            cb({Location(stm->location), Heuristic{convTerm(heu.atom), convBodyLiteralVec(heu.body, heu.size), convTerm(heu.bias), convTerm(heu.priority), convTerm(heu.modifier)}});
            break;
        }
        case clingo_ast_statement_type_project_atom: {
            cb({Location(stm->location), ProjectAtom{convTerm(stm->project_atom->atom), convBodyLiteralVec(stm->project_atom->body, stm->project_atom->size)}});
            break;
        }
        case clingo_ast_statement_type_project_atom_signature: {
            cb({Location(stm->location), ProjectSignature{Signature(stm->project_signature)}});
            break;
        }
        case clingo_ast_statement_type_theory_definition: {
            auto const &def = *stm->theory_definition;
            cb({Location(stm->location), TheoryDefinition{def.name, convTheoryTermDefinitionVec(def.terms, def.terms_size), convTheoryAtomDefinitionVec(def.atoms, def.atoms_size)}});
            break;
        }
        case clingo_ast_statement_type_defined: {
            cb({Location(stm->location), Defined{Signature{stm->defined->signature}}});
            break;
        }
    }
}

// }}}4

#undef CLINGO_ARRAY

// }}}3

} // namespace Detail

// {{{3 printing

// {{{4 statement

inline std::ostream &operator<<(std::ostream &out, TheoryDefinition const &x) {
    out << "#theory " << x.name << " {\n";
    bool comma = false;
    for (auto const &y : x.terms) {
        if (comma) { out << ";\n"; }
        else       { comma = true; }
        out << "  " << y.name << " {\n" << Detail::print(y.operators, "    ", ";\n", "\n", true) << "  }";
    }
    for (auto const &y : x.atoms) {
        if (comma) { out << ";\n"; }
        else       { comma = true; }
        out << "  " << y;
    }
    if (comma) { out << "\n"; }
    out << "}.";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryAtomDefinition const &x) {
    out << "&" << x.name << "/" << x.arity << " : " << x.elements;
    if (x.guard) { out << ", " << *x.guard; }
    out << ", " << x.type;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryGuardDefinition const &x) {
    out << "{ " << Detail::print(x.operators, "", ", ", "", false) << " }, " << x.term;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryTermDefinition const &x) {
    out << x.name << " {\n" << Detail::print(x.operators, "  ", ";\n", "\n", true) << "}";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryOperatorDefinition const &x) {
    out << x.name << " : " << x.priority << ", " << x.type;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, BodyLiteral const &x) {
    out << x.sign << x.data;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, HeadLiteral const &x) {
    out << x.data;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryAtom const &x) {
    out << "&" << x.term << " { " << Detail::print(x.elements, "", "; ", "", false) << " }";
    if (x.guard) { out << " " << *x.guard; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryGuard const &x) {
    out << x.operator_name << " " << x.term;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryAtomElement const &x) {
    out << Detail::print(x.tuple, "", ",", "", false) << " : " << Detail::print(x.condition, "", ",", "", false);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryUnparsedTermElement const &x) {
    out << Detail::print(x.operators, " ", " ", " ", false) << x.term;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryFunction const &x) {
    out << x.name << Detail::print(x.arguments, "(", ",", ")", !x.arguments.empty());
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryTermSequence const &x) {
    bool tc = x.terms.size() == 1 && x.type == TheoryTermSequenceType::Tuple;
    out << Detail::print(x.terms, left_hand_side(x.type), ",", "", true);
    if (tc) { out << ",)"; }
    else    { out << right_hand_side(x.type); }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryTerm const &x) {
    out << x.data;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, TheoryUnparsedTerm const &x) {
    if (x.elements.size() > 1) { out << "("; }
    out << Detail::print(x.elements, "", "", "", false);
    if (x.elements.size() > 1) { out << ")"; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Disjoint const &x) {
    out << "#disjoint { " << Detail::print(x.elements, "", "; ", "", false) << " }";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, DisjointElement const &x) {
    out << Detail::print(x.tuple, "", ",", "", false) << " : " << x.term << " : " << Detail::print(x.condition, "", ",", "", false);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Disjunction const &x) {
    out << Detail::print(x.elements, "", "; ", "", false);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, HeadAggregate const &x) {
    if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
    out << x.function << " { " << Detail::print(x.elements, "", "; ", "", false) << " }";
    if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, HeadAggregateElement const &x) {
    out << Detail::print(x.tuple, "", ",", "", false) << " : " << x.condition;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, BodyAggregate const &x) {
    if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
    out << x.function << " { " << Detail::print(x.elements, "", "; ", "", false) << " }";
    if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, BodyAggregateElement const &x) {
    out << Detail::print(x.tuple, "", ",", "", false) << " : " << Detail::print(x.condition, "", ", ", "", false);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Aggregate const &x) {
    if (x.left_guard) { out << x.left_guard->term << " " << x.left_guard->comparison << " "; }
    out << "{ " << Detail::print(x.elements, "", "; ", "", false) << " }";
    if (x.right_guard) { out << " " << x.right_guard->comparison << " " << x.right_guard->term; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, ConditionalLiteral const &x) {
    out << x.literal << Detail::print(x.condition, " : ", ", ", "", true);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Literal const &x) {
    out << x.sign << x.data;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Boolean const &x) {
    out << (x.value ? "#true" : "#false");
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Comparison const &x) {
    out << x.left << x.comparison << x.right;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Id const &x) {
    out << x.id;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPLiteral const &x) {
    out << x.term;
    for (auto const &y : x.guards) { out << y; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPGuard const &x) {
    out << "$" << x.comparison << x.term;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPSum const &x) {
    if (x.terms.empty()) { out << "0"; }
    else                 { out << Detail::print(x.terms, "", "$+", "", false); }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, CSPProduct const &x) {
    if (x.variable) { out << x.coefficient << "$*$" << *x.variable; }
    else            { out << x.coefficient; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Pool const &x) {
    // NOTE: there is no representation for an empty pool
    auto const &args = x.arguments;
    if (args.empty()) { out << "(1/0)"; }
    if (args.size() == 1) { out << args[0]; }
    else {
        bool equal{true};
        Function const *old{nullptr};
        for (auto const &arg : args) {
            if (arg.data.is<Function>()) {
                auto const &fun = arg.data.get<Function>();
                if (old == nullptr) { old = &fun; }
                else if (strcmp(fun.name, old->name) != 0 || fun.external != old->external) {
                    equal = false;
                    break;
                }
            }
            else {
                equal = false;
                break;
            }
        }
        if (old != nullptr && equal) {
            out << (old->external ? "@" : "") << old->name << "(";
            bool sem = false;
            for (auto const &arg : args) {
                if (sem) { out << ";"; }
                else { sem = true; }
                auto const &pargs = arg.data.get<Function>().arguments;
                bool tc = old->name[0] == '\0' && pargs.size() == 1;
                out << Detail::print(pargs, "", ",", tc ? "," : "", true);
            }
            out << ")";
        }
        else {
            out << Detail::print(x.arguments, "(", ";", ")", true);
        }
    }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Function const &x) {
    bool tc = x.name[0] == '\0' && x.arguments.size() == 1;
    bool ey = x.name[0] == '\0' || !x.arguments.empty();
    out << (x.external ? "@" : "") << x.name << Detail::print(x.arguments, "(", ",", tc ? ",)" : ")", ey);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Interval const &x) {
    out << "(" << x.left << ".." << x.right << ")";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, BinaryOperation const &x) {
    out << "(" << x.left << x.binary_operator << x.right << ")";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, UnaryOperation const &x) {
    out << left_hand_side(x.unary_operator) << x.argument << right_hand_side(x.unary_operator);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Variable const &x) {
    out << x.name;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Term const &x) {
    out << x.data;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Rule const &x) {
    out << x.head << Detail::print_body(x.body, " :- ");
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Definition const &x) {
    out << "#const " << x.name << " = " << x.value << ".";
    if (!x.is_default) { out << " [override]"; }
    return out;
}

inline std::ostream &operator<<(std::ostream &out, ShowSignature const &x) {
    out << "#show " << (x.csp ? "$" : "") << x.signature << ".";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Defined const &x) {
    out << "#defined " << x.signature << ".";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, ShowTerm const &x) {
    out << "#show " << (x.csp ? "$" : "") << x.term << Detail::print_body(x.body);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Minimize const &x) {
    out << Detail::print_body(x.body, ":~ ") << " [" << x.weight << "@" << x.priority << Detail::print(x.tuple, ",", ",", "", false) << "]";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Script const &x) {
    std::string s = x.code;
    if (!s.empty() && s.back() == '\n') {
        s.back() = '.';
    }
    out << s;
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Program const &x) {
    out << "#program " << x.name << Detail::print(x.parameters, "(", ",", ")", false) << ".";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, External const &x) {
    out << "#external " << x.atom << Detail::print_body(x.body) << " [" << x.type << "]";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Edge const &x) {
    out << "#edge (" << x.u << "," << x.v << ")" << Detail::print_body(x.body);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Heuristic const &x) {
    out << "#heuristic " << x.atom << Detail::print_body(x.body) << " [" << x.bias<< "@" << x.priority << "," << x.modifier << "]";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, ProjectAtom const &x) {
    out << "#project " << x.atom << Detail::print_body(x.body);
    return out;
}

inline std::ostream &operator<<(std::ostream &out, ProjectSignature const &x) {
    out << "#project " << x.signature << ".";
    return out;
}

inline std::ostream &operator<<(std::ostream &out, Statement const &x) {
    out << x.data;
    return out;
}

// }}}4

// }}}3

} // namespace AST

inline void parse_program(char const *program, StatementCallback cb, Logger logger, unsigned message_limit) {
    using Data = std::pair<StatementCallback &, std::exception_ptr>;
    Data data(cb, nullptr);
    Detail::handle_error(clingo_parse_program(program, [](clingo_ast_statement_t const *stm, void *data) -> bool {
        auto &d = *static_cast<Data*>(data);
        CLINGO_CALLBACK_TRY { AST::Detail::convStatement(stm, d.first); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit), data.second);
}

inline void parse_program(StringSpan files, StatementCallback cb, Logger logger, unsigned message_limit) {
    using Data = std::pair<StatementCallback &, std::exception_ptr>;
    Data data(cb, nullptr);
    Detail::handle_error(clingo_parse_files(files.begin(), files.size(), [](clingo_ast_statement_t const *stm, void *data) -> bool {
        auto &d = *static_cast<Data*>(data);
        CLINGO_CALLBACK_TRY { AST::Detail::convStatement(stm, d.first); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, [](clingo_warning_t code, char const *msg, void *data) {
        try { (*static_cast<Logger*>(data))(static_cast<WarningCode>(code), msg); }
        catch (...) { }
    }, &logger, message_limit), data.second);
}

inline int clingo_main(Application &application, StringSpan arguments) {
    Detail::ApplicationData data{application, Detail::ParserList{}};
    static clingo_application_t g_app = {
        Detail::g_program_name,
        Detail::g_version,
        Detail::g_message_limit,
        Detail::g_main,
        Detail::g_logger,
        Detail::g_model_printer,
        Detail::g_register_options,
        Detail::g_validate_options
    };
    return ::clingo_main(&g_app, arguments.begin(), arguments.size(), &data);
}

// }}}2

} // namespace Clingo

#undef CLINGO_CALLBACK_TRY
#undef CLINGO_CALLBACK_CATCH
#undef CLINGO_TRY
#undef CLINGO_CATCH

// }}}1

#endif
