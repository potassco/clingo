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

#ifndef GRINGO_CLINGO_H
#define GRINGO_CLINGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// {{{1 errors and warnings

// Note: Errors can be recovered from. If a control function fails, the
// corresponding control object must be destroyed.  An exception is the
// clingo_control_parse function, which does not invalidate the corresponding
// control object (probably this function will become a free function in the
// future).
enum clingo_error {
    clingo_error_success   = 0,
    clingo_error_fatal     = 1,
    clingo_error_runtime   = 2,
    clingo_error_logic     = 3,
    clingo_error_bad_alloc = 4,
    clingo_error_unknown   = 5
};
typedef int clingo_error_t;

enum clingo_warning {
    clingo_warning_operation_undefined = -1, //< Undefined arithmetic operation or weight of aggregate.
    clingo_warning_atom_undefined      = -2, //< Undefined atom in program.
    clingo_warning_file_included       = -3, //< Same file included multiple times.
    clingo_warning_variable_unbounded  = -4, //< CSP Domain undefined.
    clingo_warning_global_variable     = -5, //< Global variable in tuple of aggregate element.
    clingo_warning_total               =  5, //< Total number of warnings.
};
typedef int clingo_warning_t;

// the union of error and warning codes
typedef int clingo_message_code_t;
char const *clingo_message_code_str(clingo_message_code_t code);

typedef void clingo_logger_t(clingo_message_code_t, char const *, void *);

// {{{1 symbol
// {{{2 types

enum clingo_symbol_type {
    clingo_symbol_type_inf = 0,
    clingo_symbol_type_num = 1,
    clingo_symbol_type_str = 4,
    clingo_symbol_type_fun = 5,
    clingo_symbol_type_sup = 7
};
typedef int clingo_symbol_type_t;
typedef struct clingo_symbol {
    uint64_t rep;
} clingo_symbol_t;
typedef struct clingo_symbol_span {
    clingo_symbol_t const *first;
    size_t size;
} clingo_symbol_span_t;
typedef clingo_error_t clingo_string_callback_t (char const *, void *);

// {{{2 construction

void clingo_symbol_new_num(int num, clingo_symbol_t *sym);
void clingo_symbol_new_sup(clingo_symbol_t *sym);
void clingo_symbol_new_inf(clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_str(char const *str, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_id(char const *id, bool sign, clingo_symbol_t *sym);
clingo_error_t clingo_symbol_new_fun(char const *name, clingo_symbol_span_t args, bool sign, clingo_symbol_t *sym);

// {{{2 inspection

clingo_error_t clingo_symbol_num(clingo_symbol_t sym, int *num);
clingo_error_t clingo_symbol_name(clingo_symbol_t sym, char const **name);
clingo_error_t clingo_symbol_string(clingo_symbol_t sym, char const **str);
clingo_error_t clingo_symbol_sign(clingo_symbol_t sym, bool *sign);
clingo_error_t clingo_symbol_args(clingo_symbol_t sym, clingo_symbol_span_t *args);
clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t sym);
clingo_error_t clingo_symbol_to_string(clingo_symbol_t sym, clingo_string_callback_t *cb, void *data);

// {{{2 comparison

size_t clingo_symbol_hash(clingo_symbol_t sym);
bool clingo_symbol_eq(clingo_symbol_t a, clingo_symbol_t b);
bool clingo_symbol_lt(clingo_symbol_t a, clingo_symbol_t b);

// {{{1 symbolic literals

typedef struct clingo_symbolic_literal {
    clingo_symbol_t atom;
    bool sign;
} clingo_symbolic_literal_t;

typedef struct clingo_symbolic_literal_span {
    clingo_symbolic_literal_t const *first;
    size_t size;
} clingo_symbolic_literal_span_t;

// {{{1 part

typedef struct clingo_part {
    char const *name;
    clingo_symbol_span_t params;
} clingo_part_t;
typedef struct clingo_part_span {
    clingo_part_t const *first;
    size_t size;
} clingo_part_span_t;

// {{{1 module (there should only ever be one module)

typedef struct clingo_module clingo_module_t;

clingo_error_t clingo_module_new(clingo_module_t **mod);
void clingo_module_free(clingo_module_t *mod);

// {{{1 model

enum clingo_show_type {
    clingo_show_type_csp   = 1,
    clingo_show_type_shown = 2,
    clingo_show_type_atoms = 4,
    clingo_show_type_terms = 8,
    clingo_show_type_comp  = 16,
    clingo_show_type_all   = 15
};
typedef unsigned clingo_show_type_t;
typedef struct clingo_model clingo_model_t;
bool clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom);
clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_symbol_span_t *ret);

// {{{1 solve result

enum clingo_solve_result {
    clingo_solve_result_sat         = 1,
    clingo_solve_result_unsat       = 2,
    clingo_solve_result_exhausted   = 4,
    clingo_solve_result_interrupted = 8
};
typedef unsigned clingo_solve_result_t;

// {{{1 solve iter

typedef struct clingo_solve_iter clingo_solve_iter_t;
clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model_t **m);
clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret);
clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it);

// {{{1 truth value

enum clingo_truth_value {
    clingo_truth_value_free = 0,
    clingo_truth_value_true = 1,
    clingo_truth_value_false = 2
};
typedef int clingo_truth_value_t;

// {{{1 ast

typedef struct clingo_location {
    char const *begin_file;
    char const *end_file;
    size_t begin_line;
    size_t end_line;
    size_t begin_column;
    size_t end_column;
} clingo_location_t;
typedef struct clingo_ast clingo_ast_t;
typedef struct clingo_ast_span {
    clingo_ast_t const *first;
    size_t size;
} clingo_ast_span_t;
struct clingo_ast {
    clingo_location_t location;
    clingo_symbol_t value;
    clingo_ast_span_t children;
};
typedef clingo_error_t clingo_ast_callback_t (clingo_ast_t const *, void *);
typedef clingo_error_t clingo_add_ast_callback_t (void *, clingo_ast_callback_t *, void *);

// {{{1 control

typedef struct clingo_string_span {
    char const **first;
    size_t size;
} clingo_string_span_t;
typedef clingo_error_t clingo_model_handler_t (clingo_model_t*, void *, bool *);
typedef clingo_error_t clingo_symbol_span_callback_t (clingo_symbol_span_t, void *);
typedef clingo_error_t clingo_ground_callback_t (char const *, clingo_symbol_span_t, void *, clingo_symbol_span_callback_t *, void *);
typedef struct clingo_control clingo_control_t;
clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_control_t **ctl);
void clingo_control_free(clingo_control_t *ctl);
clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, clingo_string_span_t params, char const *part);
clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data);
clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_model_handler_t *mh, void *data, clingo_solve_result_t *ret);
clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_solve_iter_t **it);
clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value);
clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom);
clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data);
clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data);

// }}}1

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string>
#include <functional>
#include <ostream>
#include <bitset>

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
    T const *begin() { return static_cast<T const *>(C::first); }
    T const *end() { return begin() + size(); }
    size_t size() { return C::size; }
};

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
    std::string toString() const;
    size_t hash() const;
};

Symbol Num(int num);
Symbol Sup();
Symbol Inf();
Symbol Str(char const *str);
Symbol Id(char const *str, bool sign = false);
Symbol Fun(char const *name, SymSpan args, bool sign = false);

std::ostream &operator<<(std::ostream &out, Symbol sym);
bool operator==(Symbol const &a, Symbol const &b);
bool operator!=(Symbol const &a, Symbol const &b);
bool operator< (Symbol const &a, Symbol const &b);
bool operator<=(Symbol const &a, Symbol const &b);
bool operator> (Symbol const &a, Symbol const &b);
bool operator>=(Symbol const &a, Symbol const &b);

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
    // currently the model stores the symspan up to the next call to atoms - bad?
    SymSpan atoms(ShowType show) const;
private:
    clingo_model_t *model_;
};

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

// {{{1 control

enum class TruthValue : clingo_truth_value_t {
    Free = clingo_truth_value_free,
    True = clingo_truth_value_true,
    False = clingo_truth_value_false
};

class Part : public clingo_part_t {
public:
    Part(char const *name, SymSpan params)
    : clingo_part_t{name, params} { }
    char const *name() const { return clingo_part_t::name; }
    SymSpan params() const { return clingo_part_t::params; }
};
using PartSpan = Span<Part, clingo_part_span_t>;
using GroundCallback = std::function<void (char const *, SymSpan, std::function<void (SymSpan)>)>;
using StringSpan = Span<char const *, clingo_string_span_t>;
using ModelHandler = std::function<bool (Model)>;

class Control {
public:
    Control(clingo_control_t *ctl);
    ~Control() noexcept;
    void add(char const *name, StringSpan params, char const *part);
    void add(clingo_add_ast_callback_t *cb, void *data);
    void ground(PartSpan parts, GroundCallback cb);
    SolveResult solve(SymbolicLiteralSpan assumptions, ModelHandler mh);
    SolveIter solve_iter(SymbolicLiteralSpan assumptions);
    void assign_external(Symbol atom, TruthValue value);
    void release_external(Symbol atom);
    operator clingo_control_t*() const;
private:
    clingo_control_t *ctl_;
};

// }}}1

}

#endif

#endif
