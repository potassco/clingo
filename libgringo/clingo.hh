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

namespace Clingo {

// {{{1 basic types

// consider using upper case
using lit_t = clingo_lit_t;
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

// {{{1 span

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

class Signature : public clingo_signature_t {
public:
    Signature(char const *name, uint32_t arity, bool sign = false);
    char const *name() const;
    uint32_t arity() const;
    bool sign() const;
    size_t hash() const;

    bool clingo_signature_eq(clingo_signature_t a, clingo_signature_t b);
    bool clingo_signature_lt(clingo_signature_t a, clingo_signature_t b);
};

inline std::ostream &operator<<(std::ostream &out, Signature sig) {
    out << (sig.sign() ? "-" : "") << sig.name() << "/" << sig.arity();
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
    Inf = clingo_symbol_type_inf,
    Num = clingo_symbol_type_num,
    Str = clingo_symbol_type_str,
    Fun = clingo_symbol_type_fun,
    Sup = clingo_symbol_type_sup
};

class Symbol;
using SymbolSpan = Span<Symbol>;
using SymbolVector = std::vector<Symbol>;

class Symbol : public clingo_symbol_t {
public:
    Symbol();
    Symbol(clingo_symbol_t);
    int num() const;
    char const *name() const;
    char const *string() const;
    bool sign() const;
    SymbolSpan args() const;
    SymbolType type() const;
    std::string to_string() const;
    size_t hash() const;
};

Symbol Num(int num);
Symbol Sup();
Symbol Inf();
Symbol Str(char const *str);
Symbol Id(char const *str, bool sign = false);
Symbol Fun(char const *name, SymbolSpan args, bool sign = false);

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
    friend class SymbolicAtomIter;
public:
    SymbolicAtom(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iter_t range)
    : atoms_(atoms)
    , range_(range) { }
    Symbol symbol() const;
    lit_t literal() const;
    bool fact() const;
    bool external() const;
    operator clingo_symbolic_atom_iter_t() const { return range_; }
private:
    clingo_symbolic_atoms_t *atoms_;
    clingo_symbolic_atom_iter_t range_;
};

class SymbolicAtomIter : private SymbolicAtom, public std::iterator<std::input_iterator_tag, SymbolicAtom> {
public:
    SymbolicAtomIter(clingo_symbolic_atoms_t *atoms, clingo_symbolic_atom_iter_t range)
    : SymbolicAtom{atoms, range} { }
    SymbolicAtom &operator*() { return *this; }
    SymbolicAtom *operator->() { return this; }
    SymbolicAtomIter &operator++();
    SymbolicAtomIter operator++ (int) {
        auto range = range_;
        ++(*this);
        return {atoms_, range};
    }
    bool operator==(SymbolicAtomIter it) const;
    bool operator!=(SymbolicAtomIter it) const { return !(*this == it); }
    operator bool() const;
    operator clingo_symbolic_atom_iter_t() const { return range_; }
};

class SymbolicAtoms {
public:
    SymbolicAtoms(clingo_symbolic_atoms_t *atoms)
    : atoms_(atoms) { }
    SymbolicAtomIter begin() const;
    SymbolicAtomIter begin(Signature sig) const;
    SymbolicAtomIter end() const;
    SymbolicAtomIter find(Symbol atom) const;
    std::vector<Signature> signatures() const;
    size_t length() const;
    operator clingo_symbolic_atoms_t*() const { return atoms_; }
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
class TheoryIterator : public std::iterator<std::bidirectional_iterator_tag, const T, ptrdiff_t, T*, T> {
public:
    using difference_type = typename std::iterator<std::bidirectional_iterator_tag, const T>::difference_type;
    TheoryIterator(clingo_theory_atoms_t *atoms, clingo_id_t const* id)
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
    friend TheoryIterator operator+(TheoryIterator it, difference_type n) { return {it.atoms(), it.id_ + n}; }
    friend TheoryIterator operator+(difference_type n, TheoryIterator it) { return {it.atoms(), it.id_ + n}; }
    friend TheoryIterator operator-(TheoryIterator it, difference_type n) { return {it.atoms(), it.id_ - n}; }
    friend difference_type operator-(TheoryIterator a, TheoryIterator b)  { return b.id_ - a.id_; }
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
    ToTheoryIterator(clingo_theory_atoms_t *atoms)
    : atoms_(atoms) { }
    T operator ()(clingo_id_t const *id) const {
        return {atoms_, id};
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
    TheoryTerm(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermType type() const;
    int number() const;
    char const *name() const;
    TheoryTermSpan arguments() const;
    operator clingo_id_t() const { return id_; }
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
using LitSpan = Span<lit_t>;

class TheoryElement {
    friend class TheoryIterator<TheoryElement>;
public:
    TheoryElement(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryTermSpan tuple() const;
    LitSpan condition() const;
    lit_t condition_literal() const;
    std::string to_string() const;
    operator clingo_id_t() const { return id_; }
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
    TheoryAtom(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryElementSpan elements() const;
    TheoryTerm term() const;
    bool has_guard() const;
    lit_t literal() const;
    std::pair<char const *, TheoryTerm> guard() const;
    std::string to_string() const;
    operator clingo_id_t() const { return id_; }
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
    TheoryAtomIterator(clingo_theory_atoms_t *atoms, clingo_id_t id)
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
    friend TheoryAtomIterator operator+(TheoryAtomIterator it, difference_type n) { return {it.atoms(), clingo_id_t(it.id() + n)}; }
    friend TheoryAtomIterator operator+(difference_type n, TheoryAtomIterator it) { return {it.atoms(), clingo_id_t(it.id() + n)}; }
    friend TheoryAtomIterator operator-(TheoryAtomIterator it, difference_type n) { return {it.atoms(), clingo_id_t(it.id() - n)}; }
    friend difference_type operator-(TheoryAtomIterator a, TheoryAtomIterator b)  { return b.id() - a.id(); }
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
    TheoryAtoms(clingo_theory_atoms_t *atoms)
    : atoms_(atoms) { }
    TheoryAtomIterator begin() const;
    TheoryAtomIterator end() const;
    size_t size() const;
    operator clingo_theory_atoms_t*() const { return atoms_; }
private:
    clingo_theory_atoms_t *atoms_;
};

// {{{1 propagate init

class PropagateInit {
public:
    PropagateInit(clingo_propagate_init_t *init)
    : init_(init) { }
    lit_t map_literal(lit_t lit) const;
    void add_watch(lit_t lit);
    int number_of_threads() const;
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    operator clingo_propagate_init_t*() const { return init_; }
private:
    clingo_propagate_init_t *init_;
};

// {{{1 assignment

class Assignment {
public:
    Assignment(clingo_assignment_t *ass)
    : ass_(ass) { }
    bool has_conflict() const;
    uint32_t decision_level() const;
    bool has_literal(lit_t lit) const;
    TruthValue truth_value(lit_t lit) const;
    uint32_t level(lit_t lit) const;
    lit_t decision(uint32_t level) const;
    bool is_fixed(lit_t lit) const;
    bool is_true(lit_t lit) const;
    bool is_false(lit_t lit) const;
    operator clingo_assignment_t*() const { return ass_; }
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
    PropagateControl(clingo_propagate_control_t *ctl)
    : ctl_(ctl) { }
    id_t thread_id() const;
    Assignment assignment() const;
    bool add_clause(LitSpan clause, ClauseType type = ClauseType::Learnt);
    bool propagate();
    operator clingo_propagate_control_t*() const { return ctl_; }
private:
    clingo_propagate_control_t *ctl_;
};

// {{{1 propagator

class Propagator {
public:
    virtual void init(PropagateInit &init);
    virtual void propagate(PropagateControl &ctl, LitSpan changes);
    virtual void undo(PropagateControl const &ctl, LitSpan changes);
    virtual void check(PropagateControl &ctl);
    virtual ~Propagator() noexcept = default;
};

// {{{1 symbolic literal

class SymbolicLiteral : public clingo_symbolic_literal_t{
public:
    SymbolicLiteral(Symbol atom, bool sign)
    : clingo_symbolic_literal_t{atom, sign} { }
    Symbol atom() const { return clingo_symbolic_literal_t::atom; }
    bool sign() const { return clingo_symbolic_literal_t::sign; }
};

using SymbolicLiteralSpan = Span<SymbolicLiteral>;

inline std::ostream &operator<<(std::ostream &out, SymbolicLiteral sym) {
    if (sym.sign()) { out << "~"; }
    out << sym.atom();
    return out;
}
inline bool operator==(SymbolicLiteral a, SymbolicLiteral b) { return a.sign() == b.sign() && a.atom() == b.atom(); }
inline bool operator!=(SymbolicLiteral a, SymbolicLiteral b) { return !(a == b); }
inline bool operator< (SymbolicLiteral a, SymbolicLiteral b) {
    if (a.sign() != b.sign()) { return a.sign() < b.sign(); }
    return a.atom() < b.atom();
}
inline bool operator<=(SymbolicLiteral a, SymbolicLiteral b) { return !(b < a); }
inline bool operator> (SymbolicLiteral a, SymbolicLiteral b) { return  (b < a); }
inline bool operator>=(SymbolicLiteral a, SymbolicLiteral b) { return !(a < b); }

// {{{1 model

class ShowType {
public:
    enum Type : clingo_show_type_t {
        CSP = clingo_show_type_csp,
        Shown = clingo_show_type_shown,
        Atoms = clingo_show_type_atoms,
        Terms = clingo_show_type_terms,
        Comp = clingo_show_type_comp,
        All = clingo_show_type_all
    };
    ShowType(clingo_show_type_t type) : type_(type) { }
    operator clingo_show_type_t() const { return type_; }
private:
    clingo_show_type_t type_;
};

using OptimizationVector = std::vector<int64_t>;

class Model {
public:
    Model(clingo_model_t *model);
    bool contains(Symbol atom) const;
    OptimizationVector optimization() const;
    operator bool() const { return model_; }
    operator clingo_model_t*() const { return model_; }
    SymbolVector atoms(ShowType show = ShowType::Shown) const;
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
    SolveResult(clingo_solve_result_t res)
    : res_(res) { }
    bool sat() const { return res_ & clingo_solve_result_sat; }
    bool unsat() const { return res_ & clingo_solve_result_unsat; }
    bool unknown() const { return (res_ & 3) == 0; }
    bool exhausted() const { return res_ & clingo_solve_result_exhausted; }
    bool interrupted() const { return res_ & clingo_solve_result_interrupted; }
    operator clingo_solve_result_t() const { return res_; }
private:
    clingo_solve_result_t res_;
};

inline std::ostream &operator<<(std::ostream &out, SolveResult res) {
    if (res.sat())    {
        out << "SATISFIABLE";
        if (!res.exhausted()) { out << "+"; }
    }
    else if (res.unsat())  { out << "UNSATISFIABLE"; }
    else { out << "UNKNOWN"; }
    if (res.interrupted()) { out << "/INTERRUPTED"; }
    return out;
}

// {{{1 solve iter

class SolveIter {
public:
    SolveIter();
    SolveIter(clingo_solve_iter_t *it);
    SolveIter(SolveIter &&it);
    SolveIter(SolveIter const &) = delete;
    SolveIter &operator=(SolveIter &&it);
    SolveIter &operator=(SolveIter const &) = delete;
    operator clingo_solve_iter_t*() const { return iter_; }
    Model next();
    SolveResult get();
    void close();
    ~SolveIter() { close(); }
private:
    clingo_solve_iter_t *iter_;
};

class ModelIterator : public std::iterator<Model, std::input_iterator_tag> {
public:
    ModelIterator(SolveIter &iter)
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
        return static_cast<clingo_model_t*>(a.model_) == static_cast<clingo_model_t*>(b.model_);
    }
    friend bool operator!=(ModelIterator a, ModelIterator b) { return !(a == b); }
private:
    SolveIter *iter_;
    Model model_;
};

inline ModelIterator begin(SolveIter &it) { return ModelIterator(it); };
inline ModelIterator end(SolveIter &) { return ModelIterator(); };

// {{{1 solve async

class SolveAsync {
public:
    SolveAsync(clingo_solve_async_t *async)
    : async_(async) { }
    void cancel();
    SolveResult get();
    bool wait(double timeout = std::numeric_limits<double>::infinity());
    operator clingo_solve_async_t*() const { return async_; }
private:
    clingo_solve_async_t *async_;
};

// {{{1 ast

class Location : public clingo_location_t {
public:
    Location(clingo_location_t loc) : clingo_location_t(loc) { }
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

class AST;
using ASTSpan = Span<AST>;

class AST : public clingo_ast_t {
public:
    AST(Location location, Symbol value, ASTSpan children)
    : clingo_ast_t{location, value, children.begin(), children.size()} { }
    Location location() const { return clingo_ast_t::location; }
    Symbol value() const { return clingo_ast_t::value; }
    ASTSpan children() const { return {clingo_ast_t::children, clingo_ast_t::n}; }
};
using ASTCallback = std::function<void (AST ast)>;
using AddASTCallback = std::function<void (ASTCallback)>;

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

class WeightLit : public clingo_weight_lit_t {
public:
    WeightLit(clingo_lit_t lit, clingo_weight_t weight)
    : clingo_weight_lit_t{lit, weight} { }
    WeightLit(clingo_weight_lit_t wlit)
    : clingo_weight_lit_t(wlit) { }
    lit_t literal() const { return clingo_weight_lit_t::literal; }
    weight_t weight() const { return clingo_weight_lit_t::weight; }
};

using AtomSpan = Span<atom_t>;
using WeightLitSpan = Span<WeightLit>;

class Backend {
public:
    Backend(clingo_backend_t *backend)
    : backend_(backend) { }
    void rule(bool choice, AtomSpan head, LitSpan body);
    void weight_rule(bool choice, AtomSpan head, weight_t lower, WeightLitSpan body);
    void minimize(weight_t prio, WeightLitSpan body);
    void project(AtomSpan atoms);
    void output(char const *name, LitSpan condition);
    void external(atom_t atom, ExternalType type);
    void assume(LitSpan lits);
    void heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LitSpan condition);
    void acyc_edge(int node_u, int node_v, LitSpan condition);
    atom_t add_atom();
    operator clingo_backend_t*() const { return backend_; }
private:
    clingo_backend_t *backend_;
};

// {{{1 control

class Part : public clingo_part_t {
public:
    Part(char const *name, SymbolSpan params)
    : clingo_part_t{name, params.begin(), params.size()} { }
    char const *name() const { return clingo_part_t::name; }
    SymbolSpan params() const { return {clingo_part_t::params, clingo_part_t::n}; }
};
using SymbolSpanCallback = std::function<void (SymbolSpan)>;
using PartSpan = Span<Part>;
using GroundCallback = std::function<void (Location loc, char const *, SymbolSpan, SymbolSpanCallback)>;
using StringSpan = Span<char const *>;
using ModelCallback = std::function<bool (Model)>;
using FinishCallback = std::function<void (SolveResult)>;

class Control {
public:
    Control(clingo_control_t *ctl);
    ~Control() noexcept;
    // TODO: consider removing the name/param part
    void add(char const *name, StringSpan params, char const *part);
    void add(AddASTCallback cb);
    void ground(PartSpan parts, GroundCallback cb = nullptr);
    SolveResult solve(ModelCallback mh = nullptr, SymbolicLiteralSpan assumptions = {});
    SolveIter solve_iter(SymbolicLiteralSpan assumptions = {});
    void assign_external(Symbol atom, TruthValue value);
    void release_external(Symbol atom);
    SymbolicAtoms symbolic_atoms() const;
    TheoryAtoms theory_atoms() const;
    void register_propagator(Propagator &propagator, bool sequential);
    void cleanup();
    bool has_const(char const *name) const;
    Symbol get_const(char const *name) const;
    void interrupt() noexcept;
    void load(char const *file);
    SolveAsync solve_async(ModelCallback &mh, FinishCallback &fh, SymbolicLiteralSpan assumptions = {});
    void use_enum_assumption(bool value);
    Backend backend();
    operator clingo_control_t*() const;
private:
    clingo_control_t *ctl_;
};

// {{{1 module

class MessageCode {
public:
    enum Error : clingo_message_code_t {
        Fatal = clingo_error_fatal,
        Runtime = clingo_error_runtime,
        Logic = clingo_error_logic,
        BadAlloc = clingo_error_bad_alloc,
        Unknown = clingo_error_unknown,
    };
    enum Warning : clingo_message_code_t {
        OperationUndefined = clingo_warning_operation_undefined,
        AtomUndefined = clingo_warning_atom_undefined,
        FileIncluded = clingo_warning_file_included,
        VariableUnbounded = clingo_warning_variable_unbounded,
        GlobalVariable = clingo_warning_global_variable,
    };
    MessageCode(clingo_message_code_t code) : code_(code) { }
    operator clingo_message_code_t() const { return code_; }
private:
    clingo_message_code_t code_;
};
using Logger = std::function<void (MessageCode, char const *)>;

inline std::ostream &operator<<(std::ostream &out, MessageCode code) {
    out << clingo_message_code_str(code);
    return out;
}

class Module {
public:
    Module();
    Module(clingo_module_t *module)
    : module_(module) { }
    Module(Module const &) = delete;
    Module(Module &&m)
    : module_(nullptr) { *this = std::move(m); }
    Module &operator=(Module &&m) {
        std::swap(module_, m.module_);
        return *this;
    }
    Control create_control(StringSpan args, Logger &logger, unsigned message_limit);
    Module &operator=(Module const &) = delete;
    operator clingo_module_t*() const { return module_; }
    ~Module();
private:
    clingo_module_t *module_;
};

// }}}1

}

#endif
