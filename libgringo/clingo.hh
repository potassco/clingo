// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include <iostream>

namespace Clingo {

// {{{1 basic types

// consider using upper case
using literal_t = clingo_literal_t;
using id_t = clingo_id_t;
using weight_t = clingo_weight_t;
using atom_t = clingo_atom_t;

enum class TruthValue {
    Free = clingo_truth_value_free,
    True = clingo_truth_value_true,
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
    void copy(VariantHolder const &) { }
    void destroy() {
        type_ = 0;
        data_ = nullptr;
    }
    void swap(VariantHolder &other) {
        std::swap(type_, other.type_);
        std::swap(data_, other.data_);
    }
    template <class V>
    typename std::enable_if<std::is_copy_constructible<V>::value>::type copy_if_possible(void *src) {
        data_ = new V(*static_cast<V const*>(src));
    }
    template <class V>
    typename std::enable_if<!std::is_copy_constructible<V>::value>::type copy_if_possible(void *) {
        throw std::runtime_error("variant not copyable");
    }
    template <class V>
    void accept(V &&) const { }
    unsigned type_ = 0;
    void *data_ = nullptr;
};

template <unsigned n, class T, class... U>
struct VariantHolder<n, T, U...> : VariantHolder<n+1, U...>{
    using Helper = VariantHolder<n+1, U...>;
    using Helper::check_type;
    using Helper::emplace;
    using Helper::data_;
    using Helper::type_;
    bool check_type(T *) const { return type_ == n; }
    template <class... Args>
    void emplace(T *, Args&& ...x) {
        data_ = new T{std::forward<Args>(x)...};
        type_ = n;
    }
    void copy(VariantHolder const &src) {
        if (src.type_ == n) {
            Helper::template copy_if_possible<T>(src.data_);
            type_ = src.type_;
        }
        Helper::copy(src);
    }
    template <class V>
    void accept(V &&visitor) {
        if (n == type_) { visitor.visit(*static_cast<T*>(data_)); }
        Helper::template accept<V>(std::forward<V>(visitor));
    }
    template <class V>
    void accept(V &&visitor) const {
        if (n == type_) { visitor.visit(*static_cast<T const*>(data_)); }
        Helper::template accept<V>(std::forward<V>(visitor));
    }
    void destroy() {
        if (n == type_) { delete static_cast<T*>(data_); }
        Helper::destroy();
    }
};

} // Detail

template <class T>
class Optional {
public:
    Optional() { }
    Optional(T const &x) : data_(new T(x)) { }
    Optional(T &x) : data_(new T(x)) { }
    Optional(T &&x) : data_(new T(std::move(x))) { }
    template <class... Args>
    Optional(Args&&... x) : data_(new T{std::forward<Args>(x)...}) { }
    Optional(Optional &&opt) : data_(opt.data_.release()) { }
    Optional(Optional const &opt) : data_(opt ? new T(*opt.get()) : nullptr) { }
    Optional &operator=(T const &x) {
        clear();
        data_.reset(new T(x));
    }
    Optional &operator=(T &x) {
        clear();
        data_.reset(new T(x));
    }
    Optional &operator=(T &&x) {
        clear();
        data_.reset(new T(std::move(x)));
    }
    Optional &operator=(Optional &&opt) {
        data_ = std::move(opt.data_);
    }
    Optional &operator=(Optional const &opt) {
        clear();
        data_.reset(opt ? new T(*opt.get()) : nullptr);
    }
    T *get() { return data_.get(); }
    T const *get() const { return data_.get(); }
    template <class... Args>
    void emplace(Args&&... x) {
        clear();
        data_(new T{std::forward<Args>(x)...});
    }
    void clear() { data_.reset(nullptr); }
    explicit operator bool() const { return data_.get(); }
private:
    std::unique_ptr<T> data_;
};

template <class... T>
class Variant {
public:
    Variant(Variant const &other) : Variant(other.data_) { }
    Variant(Variant &&other)      { data_.swap(other.data_); }
    template <class U>
    Variant(U &&u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace<U>(std::forward<U>(u)); }
    template <class U>
    Variant(U &u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace<U>(u); }
    template <class U>
    Variant(U const &u, typename std::enable_if<Detail::TypeInList<U, T...>::value>::type * = nullptr) { emplace<U>(u); }
    template <class U, class... Args>
    static Variant make(Args&& ...args) {
        Variant<T...> x;
        x.data_.emplace(static_cast<U*>(nullptr), std::forward<Args>(args)...);
        return std::move(x);
    }
    ~Variant() { data_.destroy(); }
    Variant &operator=(Variant const &other) { return *this = other.data_; }
    Variant &operator=(Variant &&other) { return *this = std::move(other.data_); }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U &&u) {
        emplace<U>(std::forward<U>(u));
        return *this;
    }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U &u) {
        emplace<U>(u);
        return *this;
    }
    template <class U>
    typename std::enable_if<Detail::TypeInList<U, T...>::value, Variant>::type &operator=(U const &u) {
        emplace<U>(u);
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
    template <class V>
    void accept(V &&visitor) {
        data_.template accept<V>(std::forward<V>(visitor));
    }
private:
    using Holder = Detail::VariantHolder<1, T...>;
    Variant() { }
    Variant(Holder const &data) {
        data_.copy(data);
    }
    Variant &operator=(Holder const &data) {
        Variant x(data);
        data_.swap(x.data_);
        return *this;
    }
    Variant &operator=(Holder &&data) {
        data_.swap(data);
        data.destroy();
        return *this;
    }

private:
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

template <class T>
class ValuePointer {
public:
    ValuePointer(T value) : value_(value) { }
    T &operator*() { return value_; }
    T *operator->() { return &value_; }
private:
    T value_;
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
    Infimum = clingo_symbol_type_infimum,
    Number = clingo_symbol_type_number,
    String = clingo_symbol_type_string,
    Function = clingo_symbol_type_function,
    Supremum = clingo_symbol_type_supremum
};

class Symbol;
using SymbolSpan = Span<Symbol>;
using SymbolVector = std::vector<Symbol>;

class Symbol {
public:
    Symbol();
    explicit Symbol(clingo_symbol_t);
    int num() const;
    char const *name() const;
    char const *string() const;
    bool is_positive() const;
    bool is_negative() const;
    SymbolSpan args() const;
    SymbolType type() const;
    std::string to_string() const;
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
Symbol Id(char const *str, bool positive = true);
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
    explicit SymbolicAtom(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t range)
    : atoms_(atoms)
    , range_(range) { }
    Symbol symbol() const;
    literal_t literal() const;
    bool is_fact() const;
    bool is_external() const;
    clingo_symbolic_atom_iterator_t to_c() const { return range_; }
private:
    clingo_symbolic_atoms_t *atoms_;
    clingo_symbolic_atom_iterator_t range_;
};

class SymbolicAtomIterator : private SymbolicAtom, public std::iterator<std::input_iterator_tag, SymbolicAtom> {
public:
    explicit SymbolicAtomIterator(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iterator_t range)
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
    explicit SymbolicAtoms(clingo_symbolic_atoms_t *atoms)
    : atoms_(atoms) { }
    SymbolicAtomIterator begin() const;
    SymbolicAtomIterator begin(Signature sig) const;
    SymbolicAtomIterator end() const;
    SymbolicAtomIterator find(Symbol atom) const;
    std::vector<Signature> signatures() const;
    size_t length() const;
    clingo_symbolic_atoms_t* to_c() const { return atoms_; }
    SymbolicAtom operator[](Symbol atom) { return *find(atom); }
private:
    clingo_symbolic_atoms_t *atoms_;
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
class TheoryIterator : public std::iterator<std::random_access_iterator_tag, const T, ptrdiff_t, T*, T> {
public:
    using base = std::iterator<std::random_access_iterator_tag, const T>;
    using difference_type = typename base::difference_type;
    explicit TheoryIterator(clingo_theory_atoms_t *atoms, clingo_id_t const* id)
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
    friend bool operator==(const TheoryIterator& lhs, const TheoryIterator& rhs) { return lhs.id_ == rhs.id_; }
    friend bool operator!=(const TheoryIterator& lhs, const TheoryIterator& rhs) { return !(lhs == rhs); }
    friend bool operator< (TheoryIterator lhs, TheoryIterator rhs) { return lhs.id_ < rhs.id_; }
    friend bool operator> (TheoryIterator lhs, TheoryIterator rhs) { return rhs < lhs; }
    friend bool operator<=(TheoryIterator lhs, TheoryIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(TheoryIterator lhs, TheoryIterator rhs) { return !(lhs < rhs); }
private:
    clingo_theory_atoms_t *&atoms() { return elem_.atoms_; }
private:
    T                  elem_;
    clingo_id_t const *id_;
};

template <class T>
class ToTheoryIterator {
public:
    explicit ToTheoryIterator(clingo_theory_atoms_t *atoms)
    : atoms_(atoms) { }
    T operator ()(clingo_id_t const *id) const {
        return T{atoms_, id};
    }
private:
    clingo_theory_atoms_t *atoms_;
};

class TheoryTerm;
using TheoryTermIterator = TheoryIterator<TheoryTerm>;
using TheoryTermSpan = Span<clingo_id_t, ToTheoryIterator<TheoryTermIterator>>;

class TheoryTerm {
    friend class TheoryIterator<TheoryTerm>;
public:
    explicit TheoryTerm(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermType type() const;
    int number() const;
    char const *name() const;
    TheoryTermSpan arguments() const;
    clingo_id_t to_c() const { return id_; }
    std::string to_string() const;
private:
    TheoryTerm(clingo_theory_atoms_t *atoms)
    : TheoryTerm(atoms, 0) { }
    TheoryTerm &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }
private:
    clingo_theory_atoms_t *atoms_;
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
    explicit TheoryElement(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermSpan tuple() const;
    LiteralSpan condition() const;
    literal_t condition_literal() const;
    std::string to_string() const;
    clingo_id_t to_c() const { return id_; }
private:
    TheoryElement(clingo_theory_atoms_t *atoms)
    : TheoryElement(atoms, 0) { }
    TheoryElement &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }
private:
    clingo_theory_atoms_t *atoms_;
    clingo_id_t id_;
};
std::ostream &operator<<(std::ostream &out, TheoryElement term);

class TheoryAtom {
    friend class TheoryAtomIterator;
public:
    explicit TheoryAtom(clingo_theory_atoms_t *atoms, clingo_id_t id)
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
    TheoryAtom(clingo_theory_atoms_t *atoms)
    : TheoryAtom(atoms, 0) { }
    TheoryAtom &operator=(clingo_id_t id) {
        id_ = id;
        return *this;
    }
private:
    clingo_theory_atoms_t *atoms_;
    clingo_id_t id_;
};
std::ostream &operator<<(std::ostream &out, TheoryAtom term);

class TheoryAtomIterator : private TheoryAtom, public std::iterator<TheoryAtom, std::random_access_iterator_tag, ptrdiff_t, TheoryAtom*, TheoryAtom> {
public:
    explicit TheoryAtomIterator(clingo_theory_atoms_t *atoms, clingo_id_t id)
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
    TheoryAtomIterator& operator+=(difference_type n) { id_ += n; return *this; }
    TheoryAtomIterator& operator-=(difference_type n) { id_ -= n; return *this; }
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
    clingo_theory_atoms_t *&atoms() { return atoms_; }
    clingo_id_t &id() { return id_; }
};

class TheoryAtoms {
public:
    explicit TheoryAtoms(clingo_theory_atoms_t *atoms)
    : atoms_(atoms) { }
    TheoryAtomIterator begin() const;
    TheoryAtomIterator end() const;
    size_t size() const;
    clingo_theory_atoms_t *to_c() const { return atoms_; }
private:
    clingo_theory_atoms_t *atoms_;
};

// {{{1 propagate init

class PropagateInit {
public:
    explicit PropagateInit(clingo_propagate_init_t *init)
    : init_(init) { }
    literal_t map_literal(literal_t lit) const;
    void add_watch(literal_t lit);
    int number_of_threads() const;
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    clingo_propagate_init_t *to_c() const { return init_; }
private:
    clingo_propagate_init_t *init_;
};

// {{{1 assignment

class Assignment {
public:
    explicit Assignment(clingo_assignment_t *ass)
    : ass_(ass) { }
    bool has_conflict() const;
    uint32_t decision_level() const;
    bool has_literal(literal_t lit) const;
    TruthValue truth_value(literal_t lit) const;
    uint32_t level(literal_t lit) const;
    literal_t decision(uint32_t level) const;
    bool is_fixed(literal_t lit) const;
    bool is_true(literal_t lit) const;
    bool is_false(literal_t lit) const;
    clingo_assignment_t *to_c() const { return ass_; }
private:
    clingo_assignment_t *ass_;
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
    bool add_clause(LiteralSpan clause, ClauseType type = ClauseType::Learnt);
    bool propagate();
    clingo_propagate_control_t *to_c() const { return ctl_; }
private:
    clingo_propagate_control_t *ctl_;
};

// {{{1 propagator

class Propagator {
public:
    virtual void init(PropagateInit &init);
    virtual void propagate(PropagateControl &ctl, LiteralSpan changes);
    virtual void undo(PropagateControl const &ctl, LiteralSpan changes);
    virtual void check(PropagateControl &ctl);
    virtual ~Propagator() noexcept = default;
};

// {{{1 symbolic literal

class SymbolicLiteral {
public:
    SymbolicLiteral(Symbol sym, bool sign)
    : sym_{sym.to_c(), sign} { }
    explicit SymbolicLiteral(clingo_symbolic_literal_t sym)
    : sym_(sym) { }
    Symbol symbol() const { return Symbol(sym_.symbol); }
    bool is_positive() const { return sym_.positive; }
    bool is_negative() const { return !sym_.positive; }
    clingo_symbolic_literal_t &to_c() { return sym_; }
    clingo_symbolic_literal_t const &to_c() const { return sym_; }
private:
    clingo_symbolic_literal_t sym_;
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
    if (a.is_negative() != b.is_negative()) { return a.is_negative() < b.is_negative(); }
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
    id_t thread_id() const;
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
    bool contains(Symbol atom) const;
    bool optimality_proven() const;
    CostVector cost() const;
    SymbolVector atoms(ShowType show = ShowType::Shown) const;
    SolveControl context() const;
    ModelType type() const;
    uint64_t number() const;
    explicit operator bool() const { return model_; }
    clingo_model_t *to_c() const { return model_; }
private:
    clingo_model_t *model_;
};

inline std::ostream &operator<<(std::ostream &out, Model m) {
    out << SymbolSpan(m.atoms(ShowType::Shown));
    return out;
}

// {{{1 solve result

class SolveResult {
public:
    SolveResult() : res_(0) { }
    explicit SolveResult(clingo_solve_result_bitset_t res)
    : res_(res) { }
    bool is_satisfiable() const { return res_ & clingo_solve_result_satisfiable; }
    bool is_unsatisfiable() const { return res_ & clingo_solve_result_unsatisfiable; }
    bool is_unknown() const { return (res_ & 3) == 0; }
    bool is_exhausted() const { return res_ & clingo_solve_result_exhausted; }
    bool is_interrupted() const { return res_ & clingo_solve_result_interrupted; }
    clingo_solve_result_bitset_t &to_c() { return res_; }
    clingo_solve_result_bitset_t const &to_c() const { return res_; }
    friend bool operator==(SolveResult a, SolveResult b) { return a.res_ == b.res_; }
    friend bool operator!=(SolveResult a, SolveResult b) { return a.res_ != b.res_; }
private:
    clingo_solve_result_bitset_t res_;
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

// {{{1 solve iteratively

class SolveIteratively {
public:
    SolveIteratively();
    explicit SolveIteratively(clingo_solve_iteratively_t *it);
    SolveIteratively(SolveIteratively &&it);
    SolveIteratively(SolveIteratively const &) = delete;
    SolveIteratively &operator=(SolveIteratively &&it);
    SolveIteratively &operator=(SolveIteratively const &) = delete;
    clingo_solve_iteratively_t *to_c() const { return iter_; }
    Model next();
    SolveResult get();
    void close();
    ~SolveIteratively() { close(); }
private:
    clingo_solve_iteratively_t *iter_;
};

class ModelIterator : public std::iterator<Model, std::input_iterator_tag> {
public:
    explicit ModelIterator(SolveIteratively &iter)
    : iter_(&iter)
    , model_(nullptr) { model_ = iter_->next(); }
    ModelIterator()
    : iter_(nullptr)
    , model_(nullptr) { }
    ModelIterator &operator++() {
        model_ = iter_->next();
        return *this;
    }
    // Warning: the resulting iterator should not be used
    //          because its model is no longer valid
    ModelIterator operator++(int) {
        ModelIterator t = *this;
        ++*this;
        return t;
    }
    Model &operator*() { return model_; }
    Model *operator->() { return &**this; }
    friend bool operator==(ModelIterator a, ModelIterator b) {
        return a.model_.to_c() == b.model_.to_c();
    }
    friend bool operator!=(ModelIterator a, ModelIterator b) { return !(a == b); }
private:
    SolveIteratively *iter_;
    Model model_;
};

inline ModelIterator begin(SolveIteratively &it) { return ModelIterator(it); };
inline ModelIterator end(SolveIteratively &) { return ModelIterator(); };

// {{{1 solve async

class SolveAsync {
public:
    explicit SolveAsync(clingo_solve_async_t *async)
    : async_(async) { }
    void cancel();
    SolveResult get();
    bool wait(double timeout = std::numeric_limits<double>::infinity());
    clingo_solve_async_t *to_c() const { return async_; }
private:
    clingo_solve_async_t *async_;
};

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
    if (!eq) { out << (dash ? "-" : ":") << loc.begin_file(); dash = false; }
    eq = eq && (loc.begin_line() == loc.end_line());
    if (!eq) { out << (dash ? "-" : ":") << loc.begin_line(); dash = false; }
    eq = eq && (loc.begin_column() == loc.end_column());
    if (!eq) { out << (dash ? "-" : ":") << loc.end_column(); dash = false; }
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

// Variable

struct Variable {
    char const *name;
};

// unary operation

enum UnaryOperator : clingo_ast_unary_operator_t {
    Absolute = clingo_ast_unary_operator_absolute,
    Minus    = clingo_ast_unary_operator_minus,
    Negate   = clingo_ast_unary_operator_negate
};

inline std::ostream &operator<<(std::ostream &out, UnaryOperator op) {
    switch (op) {
        case UnaryOperator::Absolute: { out << "|"; break; }
        case UnaryOperator::Minus:    { out << "-"; break; }
        case UnaryOperator::Negate:   { out << "~"; break; }
    }
    return out;
}

struct UnaryOperation {
    UnaryOperator unary_operator;
    Term          argument;
};

// binary operation

enum class BinaryOperator : clingo_ast_binary_operator_t {
    XOr      = clingo_ast_binary_operator_xor,
    Or       = clingo_ast_binary_operator_or,
    And      = clingo_ast_binary_operator_and,
    Add      = clingo_ast_binary_operator_add,
    Subtract = clingo_ast_binary_operator_subtract,
    Multiply = clingo_ast_binary_operator_multiply,
    Divide   = clingo_ast_binary_operator_divide,
    Modulo   = clingo_ast_binary_operator_modulo
};

inline std::ostream &operator<<(std::ostream &out, BinaryOperator op) {
    switch (op) {
        case BinaryOperator::XOr:      { out << "^"; break; }
        case BinaryOperator::Or:       { out << "?"; break; }
        case BinaryOperator::And:      { out << "&"; break; }
        case BinaryOperator::Add:      { out << "+"; break; }
        case BinaryOperator::Subtract: { out << "-"; break; }
        case BinaryOperator::Multiply: { out << "*"; break; }
        case BinaryOperator::Divide:   { out << "/"; break; }
        case BinaryOperator::Modulo:   { out << "\\"; break; }
    }
    return out;
}

struct BinaryOperation {
    BinaryOperator binary_operator;
    Term           left;
    Term           right;
};

// interval

struct Interval {
    Term left;
    Term right;
};

// function

struct Function {
    char const *name;
    std::vector<Term> arguments;
    bool external;
};

// pool

struct Pool {
    std::vector<Term> arguments;
};

// {{{2 csp

struct CSPMultiply {
    Location location;
    Term coefficient;
    Optional<Term> variable;
};

struct CSPAdd {
    Location location;
    std::vector<CSPMultiply> terms;
};

struct CSPGuard {
    ComparisonOperator comparison;
    CSPAdd term;
};

struct CSPLiteral {
    CSPAdd term;
    std::vector<CSPGuard> guards;
};

// {{{2 ids

struct Id {
    Location location;
    char const *id;
};

// {{{2 literals

struct Comparison {
    ComparisonOperator comparison;
    Term left;
    Term right;
};

struct Literal {
    Location location;
    Sign sign;
    Variant<bool, Term, Comparison, CSPLiteral> data;
};

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

// lparse-style aggregate

struct Aggregate {
    std::vector<ConditionalLiteral> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};

// body aggregate

struct BodyAggregateElement {
    std::vector<Term> tuple;
    std::vector<Literal> condition;
};

struct BodyAggregate {
    AggregateFunction function;
    std::vector<BodyAggregateElement> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};

// head aggregate

struct HeadAggregateElement {
    std::vector<Term> tuple;
    ConditionalLiteral condition;
};

struct HeadAggregate {
    AggregateFunction function;
    std::vector<HeadAggregateElement> elements;
    Optional<AggregateGuard> left_guard;
    Optional<AggregateGuard> right_guard;
};

// disjunction

struct Disjunction {
    std::vector<ConditionalLiteral> elements;
};

// disjoint

struct DisjointElement {
    Location location;
    std::vector<Term> tuple;
    CSPAdd term;
    std::vector<Literal> condition;
};

struct Disjoint {
    std::vector<DisjointElement> elements;
};

// {{{2 theory atom

enum class TheoryTermSequenceType : int {
    Tuple = 0,
    List  = 1,
    Set   = 2
};

struct TheoryFunction;
struct TheoryTermSequence;
struct TheoryUnparsedTerm;

struct TheoryTerm {
    Location location;
    Variant<Symbol, Variable, TheoryTermSequence, TheoryFunction, std::vector<TheoryUnparsedTerm>> data;
};

struct TheoryTermSequence {
    TheoryTermSequenceType type;
    std::vector<TheoryTerm> terms;
};

struct TheoryFunction {
    char const *name;
    std::vector<TheoryTerm> arguments;
};

struct TheoryUnparsedTerm {
    std::vector<char const *> operators;
    TheoryTerm term;
};

struct TheoryAtomElement {
    std::vector<TheoryTerm> tuple;
    std::vector<Literal> condition;
};

struct TheoryGuard {
    char const *operator_name;
    TheoryTerm term;
};

struct TheoryAtom {
    Term term;
    std::vector<TheoryAtomElement> elements;
    Optional<TheoryGuard> guard;
};

// {{{2 head literals

struct HeadLiteral {
    Location location;
    Variant<Literal, Disjunction, Aggregate, HeadAggregate, TheoryAtom> data;
};

// {{{2 body literals

struct BodyLiteral {
    Location location;
    Sign sign;
    Variant<Literal, ConditionalLiteral, Aggregate, BodyAggregate, TheoryAtom, Disjoint> data;
};

// {{{2 theory definitions

enum class TheoryOperatorType : clingo_ast_theory_operator_type_t {
     Unary       = clingo_ast_theory_operator_type_unary,
     BinaryLeft  = clingo_ast_theory_operator_type_binary_left,
     BinaryRight = clingo_ast_theory_operator_type_binary_right
};

inline std::ostream &operator<<(std::ostream &out, TheoryOperatorType op) {
    switch (op) {
        case TheoryOperatorType::Unary:       { out << "unary"; break; }
        case TheoryOperatorType::BinaryLeft:  { out << "unary"; break; }
        case TheoryOperatorType::BinaryRight: { out << "unary"; break; }
    }
    return out;
}

struct TheoryOperatorDefinition {
    Location location;
    char const *name;
    unsigned priority;
    TheoryOperatorType type;
};

struct TheoryTermDefinition {
    Location location;
    char const *name;
    std::vector<TheoryOperatorDefinition> operators;
};

struct TheoryGuardDefinition {
    char const *guard;
    std::vector<char const *> operators;
};

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

struct TheoryDefinition {
    char const *name;
    std::vector<TheoryTermDefinition> terms;
    std::vector<TheoryAtomDefinition> atoms;
};

// {{{2 statements

// rule

struct Rule {
    HeadLiteral head;
    std::vector<BodyLiteral> body;
};

// definition

struct Definition {
    char const *name;
    Term value;
    bool is_default;
};

// show

struct ShowSignature {
    Signature signature;
    bool csp;
};

struct ShowTerm {
    Term term;
    std::vector<BodyLiteral> body;
    bool csp;
};

// minimize

struct Minimize {
    Term weight;
    Term priority;
    std::vector<Term> tuple;
    std::vector<BodyLiteral> body;
};

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

// program

struct Program {
    char const *name;
    std::vector<Id> parameters;
};

// external

struct External {
    Term atom;
    std::vector<BodyLiteral> body;
};

// edge

struct Edge {
    Term u;
    Term v;
    std::vector<BodyLiteral> body;
};

// heuristic

struct Heuristic {
    Term atom;
    std::vector<BodyLiteral> body;
    Term bias;
    Term priority;
    Term modifier;
};

// project

struct Project {
    Term atom;
    std::vector<BodyLiteral> body;
};

struct ProjectSignature {
    Signature signature;
};

// statement

struct Statement {
    Location location;
    Variant<Rule, Definition, ShowSignature, ShowTerm, Minimize, Script, Program, External, Edge, Heuristic, Project, ProjectSignature, TheoryDefinition> data;
};

} // namespace

// {{{1 control

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

using AtomSpan = Span<atom_t>;
using WeightedLiteralSpan = Span<WeightedLiteral>;

class Backend {
public:
    explicit Backend(clingo_backend_t *backend)
    : backend_(backend) { }
    void rule(bool choice, AtomSpan head, LiteralSpan body);
    void weight_rule(bool choice, AtomSpan head, weight_t lower, WeightedLiteralSpan body);
    void minimize(weight_t prio, WeightedLiteralSpan body);
    void project(AtomSpan atoms);
    void external(atom_t atom, ExternalType type);
    void assume(LiteralSpan lits);
    void heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LiteralSpan condition);
    void acyc_edge(int node_u, int node_v, LiteralSpan condition);
    atom_t add_atom();
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
    friend bool operator==(const KeyIterator& lhs, const KeyIterator& rhs) { return lhs.index_ == rhs.index_; }
    friend bool operator!=(const KeyIterator& lhs, const KeyIterator& rhs) { return !(lhs == rhs); }
    friend bool operator< (KeyIterator lhs, KeyIterator rhs) { return (lhs.index_ + 1) < (rhs.index_ + 1); }
    friend bool operator> (KeyIterator lhs, KeyIterator rhs) { return rhs < lhs; }
    friend bool operator<=(KeyIterator lhs, KeyIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(KeyIterator lhs, KeyIterator rhs) { return !(lhs < rhs); }
private:
    T const *map_;
    size_t index_;
};

template <class T, class P=T*>
class ArrayIterator : public std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, ValuePointer<T>, T> {
public:
    using base = std::iterator<std::random_access_iterator_tag, T, ptrdiff_t, ValuePointer<T>, T>;
    using difference_type = typename base::difference_type;
    using reference = typename base::reference;
    using pointer = typename base::pointer;
    explicit ArrayIterator(P arr, size_t index = 0)
    : arr_(arr)
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
    reference operator*() { return (*arr_)[index_]; }
    pointer operator->() { return pointer(**this); }
    friend void swap(ArrayIterator& lhs, ArrayIterator& rhs) {
        std::swap(lhs.arr_, rhs.arr_);
        std::swap(lhs.index_, rhs.index_);
    }
    friend bool operator==(const ArrayIterator& lhs, const ArrayIterator& rhs) { return lhs.index_ == rhs.index_; }
    friend bool operator!=(const ArrayIterator& lhs, const ArrayIterator& rhs) { return !(lhs == rhs); }
    friend bool operator< (ArrayIterator lhs, ArrayIterator rhs) { return (lhs.index_ + 1) < (rhs.index_ + 1); }
    friend bool operator> (ArrayIterator lhs, ArrayIterator rhs) { return rhs < lhs; }
    friend bool operator<=(ArrayIterator lhs, ArrayIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(ArrayIterator lhs, ArrayIterator rhs) { return !(lhs < rhs); }
private:
    P arr_;
    size_t index_;
};

enum class StatisticsType : clingo_statistics_type_t {
    Value = clingo_statistics_type_value,
    Array = clingo_statistics_type_array,
    Map = clingo_statistics_type_map
};

class Statistics;
using StatisticsKeyIterator = KeyIterator<Statistics>;
using StatisticsArrayIterator = ArrayIterator<Statistics, Statistics const *>;
using StatisticsKeyRange = IteratorRange<StatisticsKeyIterator>;

class Statistics {
    friend class KeyIterator<Statistics>;
public:
    explicit Statistics(clingo_statistics_t *stats, unsigned key)
    : stats_(stats)
    , key_(key) { }
    // generic
    StatisticsType type() const;
    // arrays
    size_t size() const;
    Statistics operator[](size_t index) const;
    Statistics at(size_t index) const { return operator[](index); }
    StatisticsArrayIterator begin() const;
    StatisticsArrayIterator end() const;
    // maps
    Statistics operator[](char const *name) const;
    Statistics get(char const *name) { return operator[](name); }
    StatisticsKeyRange keys() const;
    // leafs
    double value() const;
    operator double() const { return value(); }
    clingo_statistics_t *to_c() const { return stats_; }
private:
    char const *key_name(size_t index) const;
    clingo_statistics_t *stats_;
    unsigned key_;
};

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
using SymbolSpanCallback = std::function<void (SymbolSpan)>;
using PartSpan = Span<Part>;
using GroundCallback = std::function<void (Location loc, char const *, SymbolSpan, SymbolSpanCallback)>;
using StringSpan = Span<char const *>;
using ModelCallback = std::function<bool (Model)>;
using FinishCallback = std::function<void (SolveResult)>;

enum class ErrorCode : clingo_error_t {
    Fatal = clingo_error_fatal,
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
    explicit Control(clingo_control_t *ctl);
    Control(Control &&c);
    Control(Control const &) = delete;
    Control &operator=(Control &&c);
    Control &operator=(Control const &c) = delete;
    ~Control() noexcept;
    void add(char const *name, StringSpan params, char const *part);
    void ground(PartSpan parts, GroundCallback cb = nullptr);
    SolveResult solve(ModelCallback mh = nullptr, SymbolicLiteralSpan assumptions = {});
    SolveIteratively solve_iteratively(SymbolicLiteralSpan assumptions = {});
    void assign_external(Symbol atom, TruthValue value);
    void release_external(Symbol atom);
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    void register_propagator(Propagator &propagator, bool sequential = false);
    void cleanup();
    bool has_const(char const *name) const;
    Symbol get_const(char const *name) const;
    void interrupt() noexcept;
    void load(char const *file);
    SolveAsync solve_async(ModelCallback mh = nullptr, FinishCallback fh = nullptr, SymbolicLiteralSpan assumptions = {});
    void use_enum_assumption(bool value);
    Backend backend();
    Configuration configuration();
    Statistics statistics() const;
    clingo_control_t *to_c() const;
private:
    std::unique_ptr<Impl> impl_;
};

// {{{1 global functions

using StatementCallback = std::function<void (AST::Statement &&)>;

void parse_program(char const *program, StatementCallback cb, Logger logger = nullptr, unsigned message_limit = 20);
Symbol parse_term(char const *str, Logger logger = nullptr, unsigned message_limit = 20);
char const *add_string(char const *str);

// }}}1

}

#endif
