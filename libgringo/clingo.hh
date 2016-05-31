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

namespace Clingo {

// {{{1 basic types

using lit_t = clingo_lit_t;
using id_t = clingo_id_t;

// {{{1 span

template <class T, class C, class I>
class Span {
public:
    Span()
    : Span(nullptr, nullptr, size_t(0)) { }
    Span(C *atoms, clingo_id_t const *begin, clingo_id_t const *end)
    : data_(atoms), begin_(begin), end_(end) { }
    Span(clingo_theory_atoms_t *atoms, clingo_id_t const *begin, size_t size)
    : Span{atoms, begin, begin + size} { }
    Span(std::initializer_list<T> c)
    : C{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    template <class U>
    Span(U const &c)
    : C{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    Span(Span const &span) = default;
    I begin() const { return {data_, begin_}; }
    I end() const { return {data_, end_}; }
    size_t size() const { return end_ - begin_; }
private:
    C *data_;
    clingo_id_t const *begin_;
    clingo_id_t const *end_;
};

template <class T>
class Span<T, void, void> {
public:
    Span()
    : Span{nullptr, nullptr} { }
    Span(std::initializer_list<T> c)
    : Span{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    template <class U>
    Span(U const &c)
    : Span{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    template <class I>
    Span(I const *begin, size_t size)
    : Span{static_cast<T const *>(begin), size} { }
    Span(T const *begin, size_t size)
    : Span{begin, begin + size} { }
    Span(T const *begin, T const *end)
    : begin_(begin)
    , end_(end) { }
    Span(Span const &span) = default;
    T const *begin() const { return begin_; }
    T const *end() const { return end_; }
    size_t size() const { return end_ - begin_; }
private:
    T const *begin_;
    T const *end_;
};

template <class T, class U>
bool equal_range(T const &a, U const &b) {
    using namespace std;
    return a.size() == b.size() && std::equal(begin(a), end(a), begin(b));
}

template <class T, class C, class I, class V>
bool operator==(Span<T, C, I> span, V const &v) { return equal_range(span, v); }
template <class T, class C, class I, class V>
bool operator==(V const &v, Span<T, C, I> span) { return equal_range(span, v); }

template <class T, class C, class I>
std::ostream &operator<<(std::ostream &out, Span<T, C, I> span) {
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
using SymSpan = Span<Symbol, void, void>;
using SymVec = std::vector<Symbol>;

class Symbol : public clingo_symbol_t {
public:
    Symbol();
    Symbol(clingo_symbol_t);
    int num() const;
    char const *name() const;
    char const *string() const;
    bool sign() const;
    SymSpan args() const;
    SymbolType type() const;
    std::string to_string() const;
    size_t hash() const;
};

Symbol Num(int num);
Symbol Sup();
Symbol Inf();
Symbol Str(char const *str);
Symbol Id(char const *str, bool sign = false);
Symbol Fun(char const *name, SymSpan args, bool sign = false);

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
    operator bool() const;
    operator clingo_symbolic_atom_iter_t() const { return range_; }
};

class SymbolicAtoms {
public:
    SymbolicAtoms(clingo_symbolic_atoms_t *atoms)
    : atoms_(atoms) { }
    SymbolicAtomIter begin();
    SymbolicAtomIter begin(Signature sig);
    SymbolicAtomIter end();
    SymbolicAtom lookup(Symbol atom);
    std::vector<Signature> signatures();
    size_t length() const;
    operator clingo_symbolic_atoms_t*() const { return atoms_; }
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
class TheoryIterator : public std::iterator<std::bidirectional_iterator_tag, const T> {
public:
    TheoryIterator(clingo_theory_atoms_t *atoms, clingo_id_t const* id)
    : elem_(atoms)
    , id_(id) { }
    TheoryIterator& operator++() { ++elem_; return *this; }
    TheoryIterator operator++(int) {
        TheoryIterator t(*this);
        ++*this;
        return t;
    }
    TheoryIterator& operator--() { --elem_; return *this; }
    TheoryIterator operator--(int) {
        TheoryIterator t(*this);
        --*this;
        return t;
    }
    T &operator*() { return elem_ = *id_; }
    T *operator->() { return &**this; }
    friend void swap(TheoryIterator& lhs, TheoryIterator& rhs) {
        std::swap(lhs.data_, rhs.data_);
        std::swap(lhs.elem_, rhs.elem_);
    }
    friend bool operator==(const TheoryIterator& lhs, const TheoryIterator& rhs) { return lhs.data_ == rhs.data_ && lhs.elem_ == rhs.elem_; }
    friend bool operator!=(const TheoryIterator& lhs, const TheoryIterator& rhs) { return !(lhs == rhs); }
private:
    T                  elem_;
    clingo_id_t const *id_;
};

class TheoryTerm;
using TheoryTermIterator = TheoryIterator<TheoryTerm>;
using TheoryTermSpan = Span<TheoryTerm, clingo_theory_atoms_t, TheoryTermIterator>;

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
using TheoryElementSpan = Span<TheoryElement, clingo_theory_atoms_t, TheoryElementIterator>;
using LitSpan = Span<lit_t, void, void>;

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

enum class TheoryAtomType : clingo_theory_atom_type_t {
    Head = clingo_theory_atom_type_head,
    Body = clingo_theory_atom_type_body,
    Directive = clingo_theory_atom_type_directive
};

class TheoryAtom {
    friend class TheoryAtomIterator;
public:
    TheoryAtom(clingo_theory_atoms_t *atoms, clingo_id_t id)
    : atoms_(atoms)
    , id_(id) { }
    TheoryAtomType type() const;
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

class TheoryAtomIterator : private TheoryAtom, public std::iterator<TheoryAtom, std::random_access_iterator_tag> {
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
    friend TheoryAtomIterator operator+(TheoryAtomIterator it, difference_type n) { return {it.atoms_, clingo_id_t(it.id_ + n)}; }
    friend TheoryAtomIterator operator+(difference_type n, TheoryAtomIterator it) { return {it.atoms_, clingo_id_t(it.id_ + n)}; }
    friend TheoryAtomIterator operator-(TheoryAtomIterator it, difference_type n) { return {it.atoms_, clingo_id_t(it.id_ - n)}; }
    friend difference_type operator-(TheoryAtomIterator a, TheoryAtomIterator b)  { return b.id_ - a.id_; }
    TheoryAtom &operator*() { return *this; }
    TheoryAtom *operator->() { return this; }
    friend void swap(TheoryAtomIterator& lhs, TheoryAtomIterator& rhs) {
        std::swap(lhs.atoms_, rhs.atoms_);
        std::swap(lhs.id_, rhs.id_);
    }
    friend bool operator==(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return lhs.atoms_ == rhs.atoms_ && lhs.id_ == rhs.id_; }
    friend bool operator!=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs == rhs); }
    friend bool operator< (TheoryAtomIterator lhs, TheoryAtomIterator rhs) { assert(lhs.atoms_ == rhs.atoms_); return (lhs.id_ + 1) < (rhs.id_ + 1); }
    friend bool operator> (TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return rhs < lhs; }
    friend bool operator<=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs > rhs); }
    friend bool operator>=(TheoryAtomIterator lhs, TheoryAtomIterator rhs) { return !(lhs < rhs); }
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

// {{{1 symbolic literal

class SymbolicLiteral : public clingo_symbolic_literal_t{
public:
    SymbolicLiteral(Symbol atom, bool sign)
    : clingo_symbolic_literal_t{atom, sign} { }
    Symbol atom() const { return clingo_symbolic_literal_t::atom; }
    bool sign() const { return clingo_symbolic_literal_t::sign; }
};

using SymbolicLiteralSpan = Span<SymbolicLiteral, void, void>;

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

class Model {
public:
    Model(clingo_model_t *model);
    bool contains(Symbol atom) const;
    operator bool() const { return model_; }
    operator clingo_model_t*() const { return model_; }
    SymVec atoms(ShowType show) const;
private:
    clingo_model_t *model_;
};

inline std::ostream &operator<<(std::ostream &out, Model m) {
    out << SymSpan(m.atoms(ShowType::Shown));
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
using ASTSpan = Span<AST, void, void>;

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

class TruthValue {
public:
    enum Type : clingo_truth_value_t {
        Free = clingo_truth_value_free,
        True = clingo_truth_value_true,
        False = clingo_truth_value_false
    };
    TruthValue(clingo_truth_value_t type) : type_(type) { }
    operator clingo_truth_value_t() const { return type_; }
private:
    clingo_truth_value_t type_;
};

inline std::ostream &operator<<(std::ostream &out, TruthValue tv) {
    switch (tv) {
        case TruthValue::Free:  { out << "Free"; break; }
        case TruthValue::True:  { out << "True"; break; }
        case TruthValue::False: { out << "False"; break; }
    }
    return out;
}

class Part : public clingo_part_t {
public:
    Part(char const *name, SymSpan params)
    : clingo_part_t{name, params.begin(), params.size()} { }
    char const *name() const { return clingo_part_t::name; }
    SymSpan params() const { return {clingo_part_t::params, clingo_part_t::n}; }
};
using SymSpanCallback = std::function<void (SymSpan)>;
using PartSpan = Span<Part, void, void>;
using GroundCallback = std::function<void (Location loc, char const *, SymSpan, SymSpanCallback)>;
using StringSpan = Span<char const *, void, void>;
using ModelHandler = std::function<bool (Model)>;

class Control {
public:
    Control(clingo_control_t *ctl);
    ~Control() noexcept;
    // TODO: consider removing the name/param part
    void add(char const *name, StringSpan params, char const *part);
    void add(AddASTCallback cb);
    void ground(PartSpan parts, GroundCallback cb = nullptr);
    SolveResult solve(ModelHandler mh = nullptr, SymbolicLiteralSpan assumptions = {});
    SolveIter solve_iter(SymbolicLiteralSpan assumptions = {});
    void assign_external(Symbol atom, TruthValue value);
    void release_external(Symbol atom);
    SymbolicAtoms symbolic_atoms();
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
