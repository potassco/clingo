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

namespace Clingo {

// {{{1 span

template <class T, class C>
class Span : public C {
public:
    Span()
    : C{nullptr, 0} { }
    Span(C const &span)
    : C{span.first, span.size} { }
    Span(std::initializer_list<T> c)
    : C{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    template <class U>
    Span(U const &c)
    : C{c.size() > 0 ? &*c.begin() : nullptr, c.size()} { }
    Span(T const *begin, T const *end)
    : C{begin, end - begin} { }
    Span(T const *begin, size_t size)
    : C{begin, size} { }
    T const *begin() const { return static_cast<T const *>(C::first); }
    T const *end() const { return begin() + size(); }
    size_t size() const { return C::size; }
};

template <class T, class U>
bool equal_range(T const &a, U const &b) {
    using namespace std;
    return a.size() == b.size() && std::equal(begin(a), end(a), begin(b));
}

template <class T, class C, class V>
bool operator==(Span<T, C> span, V const &v) { return equal_range(span, v); }
template <class T, class C, class V>
bool operator==(V const &v, Span<T, C> span) { return equal_range(span, v); }

template <class T, class C>
std::ostream &operator<<(std::ostream &out, Span<T, C> span) {
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
using SymSpan = Span<Symbol, clingo_symbol_span_t>;
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

// {{{1 symbolic literal

class SymbolicLiteral : public clingo_symbolic_literal_t{
public:
    SymbolicLiteral(Symbol atom, bool sign)
    : clingo_symbolic_literal_t{atom, sign} { }
    Symbol atom() const { return clingo_symbolic_literal_t::atom; }
    bool sign() const { return clingo_symbolic_literal_t::sign; }
};

using SymbolicLiteralSpan = Span<SymbolicLiteral, clingo_symbolic_literal_span_t>;

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
using ASTSpan = Span<AST, clingo_ast_span_t>;

class AST : public clingo_ast_t {
public:
    AST(Location location, Symbol value, ASTSpan children)
    : clingo_ast_t{location, value, children} { }
    Location location() const { return clingo_ast_t::location; }
    Symbol value() const { return clingo_ast_t::value; }
    ASTSpan children() const { return clingo_ast_t::children; }
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
    : clingo_part_t{name, params} { }
    char const *name() const { return clingo_part_t::name; }
    SymSpan params() const { return clingo_part_t::params; }
};
using SymSpanCallback = std::function<void (SymSpan)>;
using PartSpan = Span<Part, clingo_part_span_t>;
using GroundCallback = std::function<void (Location loc, char const *, SymSpan, SymSpanCallback)>;
using StringSpan = Span<char const *, clingo_string_span_t>;
using ModelHandler = std::function<bool (Model)>;

class Control {
public:
    Control(clingo_control_t *ctl);
    ~Control() noexcept;
    // TODO: consider removing the name/param part
    void add(char const *name, StringSpan params, char const *part);
    void add(AddASTCallback cb);
    void ground(PartSpan parts, GroundCallback cb = nullptr);
    // TODO: consider changing order of arguments
    SolveResult solve(ModelHandler mh = nullptr, SymbolicLiteralSpan assumptions = {});
    SolveIter solve_iter(SymbolicLiteralSpan assumptions = {});
    void assign_external(Symbol atom, TruthValue value);
    void release_external(Symbol atom);
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
