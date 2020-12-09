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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#elif defined(_MSC_VER)
#pragma warning (disable : 4996) // 'strcpy' may be unsafe
#endif

#include <clingo/clingo_app.hh>
#include <clingo/clingocontrol.hh>
#include <clingo/script.h>
#include <gringo/input/groundtermparser.hh>
#include <gringo/input/programbuilder.hh>
#include <gringo/input/nongroundparser.hh>
#include <clingo/astv2.hh>

#if defined CLINGO_NO_THREAD_LOCAL && ! defined EMSCRIPTEN
#   include <thread>
#   include <mutex>
#endif

// {{{1 error handling

using namespace Gringo;

namespace {

// {{{1 declaration of ClingoError

struct ClingoError : std::exception {
    ClingoError()
    : code(clingo_error_code()) {
        try {
            char const *msg = clingo_error_message();
            message = msg ? msg : "no message";
        }
        catch (...) { }
    }
    char const *what() const noexcept {
        return message.c_str();
    }
    std::string message;
    clingo_error_t code;
};

void inline clingo_expect(bool expr) {
    if (!expr) { throw std::runtime_error("unexpected"); }
}

void handleCError(bool ret, std::exception_ptr *exc = nullptr);
void handleCXXError();

#define GRINGO_CLINGO_TRY try
#define GRINGO_CLINGO_CATCH catch (...) { handleCXXError(); return false; } return true

#define GRINGO_CALLBACK_TRY try
#define GRINGO_CALLBACK_CATCH(ref) catch (...){ (ref) = std::current_exception(); return false; } return true

template <class F>
size_t print_size(F f) {
    CountStream cs;
    f(cs);
    cs.flush();
    return cs.count() + 1;
}

template <class F>
void print(char *ret, size_t n, F f) {
    ArrayStream as(ret, n);
    f(as);
    as << '\0';
    as.flush();
}

#ifdef EMSCRIPTEN
    std::exception_ptr g_lastException;
    std::string g_lastMessage;
    clingo_error_t g_lastCode;
#elif ! defined CLINGO_NO_THREAD_LOCAL
    thread_local std::exception_ptr g_lastException;
    thread_local std::string g_lastMessage;
    thread_local clingo_error_t g_lastCode;
#else
    struct TLData {
        std::exception_ptr lastException;
        std::string lastMessage;
        clingo_error_t lastCode;
    };
    std::mutex g_tLMut;
    std::unordered_map<std::thread::id, TLData> g_tLData;
    std::exception_ptr &tLlastException() {
        std::lock_guard<std::mutex> lock(g_tLMut);
        return g_tLData[std::this_thread::get_id()].lastException;
    }
    std::string &tLlastMessage() {
        std::lock_guard<std::mutex> lock(g_tLMut);
        return g_tLData[std::this_thread::get_id()].lastMessage;
    }
    clingo_error_t &tLlastCode() {
        std::lock_guard<std::mutex> lock(g_tLMut);
        return g_tLData[std::this_thread::get_id()].lastCode;
    }
    #define g_lastException (tLlastException())
    #define g_lastMessage (tLlastMessage())
    #define g_lastCode (tLlastCode())
#endif

void handleCError(bool ret, std::exception_ptr *exc) {
    if (!ret) {
        if (exc && *exc) { std::rethrow_exception(*exc); }
        char const *msg = clingo_error_message();
        if (!msg) { msg = "no message"; }
        switch (static_cast<clingo_error>(clingo_error_code())) {
            case clingo_error_runtime:   { throw std::runtime_error(msg); }
            case clingo_error_logic:     { throw std::logic_error(msg); }
            case clingo_error_bad_alloc: { throw std::bad_alloc(); }
            case clingo_error_unknown:   { throw std::runtime_error(msg); }
            case clingo_error_success:   { throw std::runtime_error(msg); }
        }
    }
}

void forwardCError(bool ret, std::exception_ptr *exc) {
    if (!ret) {
        if (exc && *exc) { std::rethrow_exception(*exc); }
        else             { throw ClingoError(); }
    }
}

void handleCXXError() {
    try { throw; }
    catch (GringoError const &)         { g_lastException = std::current_exception(); g_lastCode = clingo_error_runtime; return; }
    // Note: a ClingoError is throw after an exception is set or a user error is thrown so either
    //       - g_lastException is already set, or
    //       - there was a user error (currently not associated to an error message)
    catch (ClingoError const &e)        { g_lastException = std::current_exception(); g_lastCode = e.code; }
    catch (MessageLimitError const &)   { g_lastException = std::current_exception(); g_lastCode = clingo_error_runtime; return; }
    catch (std::bad_alloc const &)      { g_lastException = std::current_exception(); g_lastCode = clingo_error_bad_alloc; return; }
    catch (std::runtime_error const &)  { g_lastException = std::current_exception(); g_lastCode = clingo_error_runtime; return; }
    catch (std::logic_error const &)    { g_lastException = std::current_exception(); g_lastCode = clingo_error_logic; return; }
    g_lastCode = clingo_error_unknown;
}


void clingo_terminate(char const *loc) {
    fprintf(stderr, "%s:\n %s\n", loc, clingo_error_message());
    fflush(stderr);
    std::_Exit(1);
}

// }}}1
// {{{1

clingo_location_t conv(Location const &loc) {
    return {loc.beginFilename.c_str(), loc.endFilename.c_str(), loc.beginLine, loc.endLine, loc.beginColumn, loc.endColumn};
}

Location conv(clingo_location_t const &loc) {
    return { loc.begin_file,
             static_cast<unsigned int>(loc.begin_line),
             static_cast<unsigned int>(loc.begin_column),
             loc.end_file,
             static_cast<unsigned int>(loc.end_line),
             static_cast<unsigned int>(loc.end_column)};
}

// 1}}}
} // namespace

// c interface

// {{{1 error handling

extern "C" void clingo_set_error(clingo_error_t code, char const *message) {
    g_lastCode = code;
    try         { g_lastException = std::make_exception_ptr(std::runtime_error(message)); }
    catch (...) { g_lastException = nullptr; }
}
extern "C" char const *clingo_error_message() {
    if (g_lastException) {
        try { std::rethrow_exception(g_lastException); }
        catch (std::bad_alloc const &) { return "bad_alloc"; }
        catch (std::exception const &e) {
            g_lastMessage = e.what();
            return g_lastMessage.c_str();
        }
    }
    return nullptr;
}

extern "C" clingo_error_t clingo_error_code() {
    return g_lastCode;
}

extern "C" char const *clingo_error_string(clingo_error_t code) {
    switch (static_cast<clingo_error>(code)) {
        case clingo_error_success:               { return "success"; }
        case clingo_error_runtime:               { return "runtime error"; }
        case clingo_error_bad_alloc:             { return "bad allocation"; }
        case clingo_error_logic:                 { return "logic error"; }
        case clingo_error_unknown:               { return "unknown error"; }
    }
    return nullptr;
}

extern "C" char const *clingo_warning_string(clingo_warning_t code) {
    switch (static_cast<clingo_warning>(code)) {
        case clingo_warning_operation_undefined: { return "operation undefined"; }
        case clingo_warning_runtime_error:       { return "runtime errer"; }
        case clingo_warning_atom_undefined:      { return "atom undefined"; }
        case clingo_warning_file_included:       { return "file included"; }
        case clingo_warning_variable_unbounded:  { return "variable unbounded"; }
        case clingo_warning_global_variable:     { return "global variable"; }
        case clingo_warning_other:               { return "other"; }
    }
    return "unknown message code";
}

// {{{1 signature

extern "C" bool clingo_signature_create(char const *name, uint32_t arity, bool positive, clingo_signature_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = Sig(name, arity, !positive).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" char const *clingo_signature_name(clingo_signature_t sig) {
    return Sig(sig).name().c_str();
}

extern "C" uint32_t clingo_signature_arity(clingo_signature_t sig) {
    return Sig(sig).arity();
}

extern "C" bool clingo_signature_is_negative(clingo_signature_t sig) {
    return Sig(sig).sign();
}

extern "C" bool clingo_signature_is_positive(clingo_signature_t sig) {
    return !Sig(sig).sign();
}

extern "C" size_t clingo_signature_hash(clingo_signature_t sig) {
    return Sig(sig).hash();
}

extern "C" bool clingo_signature_is_equal_to(clingo_signature_t a, clingo_signature_t b) {
    return Sig(a) == Sig(b);
}

extern "C" bool clingo_signature_is_less_than(clingo_signature_t a, clingo_signature_t b) {
    return Sig(a) < Sig(b);
}


// {{{1 value

extern "C" void clingo_symbol_create_number(int num, clingo_symbol_t *val) {
    *val = Symbol::createNum(num).rep();
}

extern "C" void clingo_symbol_create_supremum(clingo_symbol_t *val) {
    *val = Symbol::createSup().rep();
}

extern "C" void clingo_symbol_create_infimum(clingo_symbol_t *val) {
    *val = Symbol::createInf().rep();
}

extern "C" bool clingo_symbol_create_string(char const *str, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createStr(str).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_create_id(char const *id, bool positive, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createId(id, !positive).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_create_function(char const *name, clingo_symbol_t const *args, size_t n, bool positive, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createFun(name, SymSpan{reinterpret_cast<Symbol const *>(args), n}, !positive).rep();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_number(clingo_symbol_t val, int *num) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Num);
        *num = Symbol(val).num();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_name(clingo_symbol_t val, char const **name) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        *name = Symbol(val).name().c_str();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_string(clingo_symbol_t val, char const **str) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Str);
        *str = Symbol(val).string().c_str();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_is_negative(clingo_symbol_t val, bool *sign) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        *sign = Symbol(val).sign();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_is_positive(clingo_symbol_t val, bool *sign) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        *sign = !Symbol(val).sign();
    } GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_arguments(clingo_symbol_t val, clingo_symbol_t const **args, size_t *n) {
    GRINGO_CLINGO_TRY {
        clingo_expect(Symbol(val).type() == SymbolType::Fun);
        auto ret = Symbol(val).args();
        *args = reinterpret_cast<clingo_symbol_t const *>(ret.first);
        *n = ret.size;
    } GRINGO_CLINGO_CATCH;
}

extern "C" clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t val) {
    return static_cast<clingo_symbol_type_t>(Symbol(val).type());
}

extern "C" bool clingo_symbol_to_string_size(clingo_symbol_t val, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([&val](std::ostream &out) { Symbol(val).print(out); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_to_string(clingo_symbol_t val, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [&val](std::ostream &out) { Symbol(val).print(out); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbol_is_equal_to(clingo_symbol_t a, clingo_symbol_t b) {
    return Symbol(a) == Symbol(b);
}

extern "C" bool clingo_symbol_is_less_than(clingo_symbol_t a, clingo_symbol_t b) {
    return Symbol(a) < Symbol(b);
}

extern "C" size_t clingo_symbol_hash(clingo_symbol_t sym) {
    return Symbol(sym).hash();
}

// {{{1 symbolic atoms

extern "C" bool clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t const *dom, clingo_signature_t const *sig, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = sig ? dom->begin(Sig(*sig)) : dom->begin(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_end(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->end(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_find(clingo_symbolic_atoms_t const *dom, clingo_symbol_t atom, clingo_symbolic_atom_iterator_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->lookup(Symbol(atom)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_iterator_is_equal_to(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t it, clingo_symbolic_atom_iterator_t jt, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->eq(it, jt); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_signatures_size(clingo_symbolic_atoms_t const *dom, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto sigs = dom->signatures();
        *n = sigs.size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t const *dom, clingo_signature_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto sigs = dom->signatures();
        if (n < sigs.size()) { throw std::length_error("not enough space"); }
        for (auto &sig : sigs) { *ret++ = sig.rep(); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_size(clingo_symbolic_atoms_t const *dom, size_t *size) {
    GRINGO_CLINGO_TRY { *size = dom->length(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_symbol(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbol_t *sym) {
    GRINGO_CLINGO_TRY { *sym = dom->atom(atm).rep(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, clingo_literal_t *lit) {
    GRINGO_CLINGO_TRY { *lit = dom->literal(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_is_fact(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, bool *fact) {
    GRINGO_CLINGO_TRY { *fact = dom->fact(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_is_external(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, bool *external) {
    GRINGO_CLINGO_TRY { *external = dom->external(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_next(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, clingo_symbolic_atom_iterator_t *next) {
    GRINGO_CLINGO_TRY { *next = dom->next(atm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_symbolic_atoms_is_valid(clingo_symbolic_atoms_t const *dom, clingo_symbolic_atom_iterator_t atm, bool *valid) {
    GRINGO_CLINGO_TRY { *valid = dom->valid(atm); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 theory atoms

struct clingo_theory_atoms : Gringo::Output::DomainData { };

extern "C" bool clingo_theory_atoms_term_type(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_theory_term_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_theory_term_type_t>(atoms->termType(value)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_term_number(clingo_theory_atoms_t const *atoms, clingo_id_t value, int *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termNum(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_term_name(clingo_theory_atoms_t const *atoms, clingo_id_t value, char const **ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termName(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_term_arguments(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->termArgs(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_element_tuple(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemTuple(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_element_condition(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_literal_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemCond(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_element_condition_id(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->elemCondLit(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_elements(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->atomElems(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_term(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomTerm(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_has_guard(clingo_theory_atoms_t const *atoms, clingo_id_t value, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomHasGuard(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_literal(clingo_theory_atoms_t const *atoms, clingo_id_t value, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomLit(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_guard(clingo_theory_atoms_t const *atoms, clingo_id_t value, char const **ret_op, clingo_id_t *ret_term) {
    GRINGO_CLINGO_TRY {
        auto guard = atoms->atomGuard(value);
        *ret_op = guard.first;
        *ret_term = guard.second;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_size(clingo_theory_atoms_t const *atoms, size_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->numAtoms(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_term_to_string_size(clingo_theory_atoms_t const *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->termStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_term_to_string(clingo_theory_atoms_t const *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->termStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_element_to_string_size(clingo_theory_atoms_t const *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_element_to_string(clingo_theory_atoms_t const *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_to_string_size(clingo_theory_atoms_t const *atoms, clingo_id_t value, size_t *n) {
    GRINGO_CLINGO_TRY { *n = print_size([atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t const *atoms, clingo_id_t value, char *ret, size_t n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 assignment

struct clingo_assignment : public Potassco::AbstractAssignment { };

extern "C" bool clingo_assignment_has_conflict(clingo_assignment_t const *ass) {
    return ass->hasConflict();
}

extern "C" uint32_t clingo_assignment_decision_level(clingo_assignment_t const *ass) {
    return ass->level();
}

extern "C" uint32_t clingo_assignment_root_level(clingo_assignment_t const *ass) {
    return ass->rootLevel();
}

extern "C" bool clingo_assignment_has_literal(clingo_assignment_t const *ass, clingo_literal_t lit) {
    return ass->hasLit(lit);
}

extern "C" bool clingo_assignment_truth_value(clingo_assignment_t const *ass, clingo_literal_t lit, clingo_truth_value_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->value(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_level(clingo_assignment_t const *ass, clingo_literal_t lit, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->level(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_decision(clingo_assignment_t const *ass, uint32_t level, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->decision(level); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_is_fixed(clingo_assignment_t const *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFixed(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_is_true(clingo_assignment_t const *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isTrue(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_is_false(clingo_assignment_t const *ass, clingo_literal_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFalse(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" size_t clingo_assignment_size(clingo_assignment_t const *assignment) {
    return assignment->size();
}

extern "C" bool clingo_assignment_is_total(clingo_assignment_t const *assignment) {
    return assignment->isTotal();
}

extern "C" bool clingo_assignment_at(clingo_assignment_t const *assignment, size_t offset, clingo_literal_t *literal) {
    GRINGO_CLINGO_TRY {
        if (offset >= assignment->size()) {
            throw std::runtime_error("invalid offset");
        }
        *literal = numeric_cast<clingo_literal_t>(offset + 1);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_trail_size(clingo_assignment_t const *assignment, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = assignment->trailSize(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_trail_begin(clingo_assignment_t const *assignment, uint32_t level, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = assignment->trailBegin(level); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_trail_end(clingo_assignment_t const *assignment, uint32_t level, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = assignment->trailEnd(level); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_assignment_trail_at(clingo_assignment_t const *assignment, uint32_t offset, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = assignment->trailAt(offset); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 propagate init

extern "C" bool clingo_propagate_init_solver_literal(clingo_propagate_init_t const *init, clingo_literal_t lit, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = init->mapLit(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_literal_t lit) {
    GRINGO_CLINGO_TRY { init->addWatch(lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_add_watch_to_thread(clingo_propagate_init_t *init, clingo_literal_t lit, uint32_t thread_id) {
    GRINGO_CLINGO_TRY { init->addWatch(thread_id, lit); }
    GRINGO_CLINGO_CATCH;
}

extern "C" int clingo_propagate_init_number_of_threads(clingo_propagate_init_t const *init) {
    return init->threads();
}

extern "C" bool clingo_propagate_init_symbolic_atoms(clingo_propagate_init_t const *init, clingo_symbolic_atoms_t const **ret) {
    GRINGO_CLINGO_TRY { *ret = &init->getDomain(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_theory_atoms(clingo_propagate_init_t const *init, clingo_theory_atoms_t const **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_theory_atoms const*>(&init->theory()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_propagate_init_set_check_mode(clingo_propagate_init_t *init, clingo_propagator_check_mode_t mode) {
    init->setCheckMode(mode);
}

extern "C" clingo_propagator_check_mode_t clingo_propagate_init_get_check_mode(clingo_propagate_init_t const *init) {
    return init->getCheckMode();
}

extern "C" clingo_assignment_t const *clingo_propagate_init_assignment(clingo_propagate_init_t const *init) {
    return static_cast<clingo_assignment_t const *>(&init->assignment());
}

extern "C" bool clingo_propagate_init_add_literal(clingo_propagate_init_t *init, bool freeze, clingo_literal_t *ret) {
    GRINGO_CLINGO_TRY { *ret = init->addLiteral(freeze); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_add_clause(clingo_propagate_init_t *init, clingo_literal_t const *literals, size_t size, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = init->addClause(Potassco::LitSpan{literals, size}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_add_weight_constraint(clingo_propagate_init_t *init, clingo_literal_t literal, clingo_weighted_literal_t const *literals, size_t size, clingo_weight_t bound, clingo_weight_constraint_type_t type, bool compare_equal, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = init->addWeightConstraint(literal, Potassco::WeightLitSpan{reinterpret_cast<Potassco::WeightLit_t const *>(literals), size}, bound, type, compare_equal); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_add_minimize(clingo_propagate_init_t *init, clingo_literal_t literal, clingo_weight_t weight, clingo_weight_t priority) {
    GRINGO_CLINGO_TRY { init->addMinimize(literal, weight, priority); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_init_propagate(clingo_propagate_init_t *init, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = init->propagate(); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 propagate control

struct clingo_propagate_control : Potassco::AbstractSolver { };

extern "C" clingo_id_t clingo_propagate_control_thread_id(clingo_propagate_control_t const *ctl) {
    return ctl->id();
}

extern "C" clingo_assignment_t const *clingo_propagate_control_assignment(clingo_propagate_control_t const *ctl) {
    return static_cast<clingo_assignment_t const *>(&ctl->assignment());
}

extern "C" bool clingo_propagate_control_add_clause(clingo_propagate_control_t *ctl, clingo_literal_t const *clause, size_t n, clingo_clause_type_t prop, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->addClause({clause, n}, Potassco::Clause_t(prop)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_control_propagate(clingo_propagate_control_t *ctl, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->propagate(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_control_add_literal(clingo_propagate_control_t *control, clingo_literal_t *result) {
    GRINGO_CLINGO_TRY { *result = control->addVariable(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_control_add_watch(clingo_propagate_control_t *control, clingo_literal_t literal) {
    GRINGO_CLINGO_TRY { control->addWatch(literal); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_propagate_control_has_watch(clingo_propagate_control_t const *control, clingo_literal_t literal) {
    return control->hasWatch(literal);
}

extern "C" void clingo_propagate_control_remove_watch(clingo_propagate_control_t *control, clingo_literal_t literal) {
    control->removeWatch(literal);
}

// {{{1 model

struct clingo_solve_control : clingo_model { };

extern "C" bool clingo_model_thread_id(clingo_model_t const *ctl, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->threadId(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_solve_control_add_clause(clingo_solve_control_t *ctl, clingo_literal_t const *clause, size_t n) {
    GRINGO_CLINGO_TRY { ctl->addClause(Potassco::toSpan(clause, n)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_solve_control_symbolic_atoms(clingo_solve_control_t const *control, clingo_symbolic_atoms_t const **atoms) {
    GRINGO_CLINGO_TRY { *atoms = &control->getDomain(); }
    GRINGO_CLINGO_CATCH;
}
extern "C" bool clingo_model_contains(clingo_model_t const *m, clingo_symbol_t atom, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = m->contains(Symbol(atom)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_symbols_size(clingo_model_t const *m, clingo_show_type_bitset_t show, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        SymSpan atoms = m->atoms(show);
        *n = atoms.size;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_symbols(clingo_model_t const *m, clingo_show_type_bitset_t show, clingo_symbol_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        SymSpan atoms = m->atoms(show);
        if (n < atoms.size) { throw std::length_error("not enough space"); }
        for (auto it = atoms.first, ie = it + atoms.size; it != ie; ++it) { *ret++ = it->rep(); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_optimality_proven(clingo_model_t const *m, bool *proven) {
    GRINGO_CLINGO_TRY { *proven = m->optimality_proven(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_cost_size(clingo_model_t const *m, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto opt = m->optimization();
        *n = opt.size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_cost(clingo_model_t const *m, int64_t *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto opt = m->optimization();
        if (n < opt.size()) { throw std::length_error("not enough space"); }
        std::copy(opt.begin(), opt.end(), ret);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_extend(clingo_model_t *model, clingo_symbol_t const *symbols, size_t size) {
    GRINGO_CLINGO_TRY { model->add({reinterpret_cast<Symbol const *>(symbols), size}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_context(clingo_model_t const *m, clingo_solve_control_t **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_solve_control_t*>(const_cast<clingo_model_t *>(m)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_number(clingo_model_t const *m, uint64_t *n) {
    GRINGO_CLINGO_TRY { *n = m->number(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_type(clingo_model_t const *m, clingo_model_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_model_type_t>(m->type()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_model_is_true(clingo_model_t const *m, clingo_literal_t lit, bool *result) {
    GRINGO_CLINGO_TRY { *result = static_cast<clingo_model_type_t>(m->isTrue(lit)); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 configuration

struct clingo_configuration : ConfigProxy { };

extern "C" bool clingo_configuration_type(clingo_configuration_t const *conf, clingo_id_t key, clingo_configuration_type_bitset_t *ret) {
    GRINGO_CLINGO_TRY {
        int map_size, array_size, value_size;
        conf->getKeyInfo(key, &map_size, &array_size, nullptr, &value_size);
        *ret = 0;
        if (map_size >= 0)   { *ret |= clingo_configuration_type_map; }
        if (array_size >= 0) { *ret |= clingo_configuration_type_array; }
        if (value_size >= 0) { *ret |= clingo_configuration_type_value; }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_map_at(clingo_configuration_t const *conf, clingo_id_t key, char const *name, clingo_id_t* subkey) {
    GRINGO_CLINGO_TRY { *subkey = conf->getSubKey(key, name); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_map_has_subkey(clingo_configuration_t const *conf, clingo_id_t key, char const *name, bool *result) {
    GRINGO_CLINGO_TRY { *result = conf->hasSubKey(key, name); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_map_subkey_name(clingo_configuration_t const *conf, clingo_id_t key, size_t index, char const **name) {
    GRINGO_CLINGO_TRY { *name = conf->getSubKeyName(key, numeric_cast<unsigned>(index)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_map_size(clingo_configuration_t const *conf, clingo_id_t key, size_t* ret) {
    GRINGO_CLINGO_TRY {
        int n;
        conf->getKeyInfo(key, &n, nullptr, nullptr, nullptr);
        if (n < 0) { throw std::runtime_error("not an array"); }
        *ret = n;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_array_at(clingo_configuration_t const *conf, clingo_id_t key, size_t idx, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getArrKey(key, numeric_cast<unsigned>(idx)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_array_size(clingo_configuration_t const *conf, clingo_id_t key, size_t *ret) {
    GRINGO_CLINGO_TRY {
        int n;
        conf->getKeyInfo(key, nullptr, &n, nullptr, nullptr);
        if (n < 0) { throw std::runtime_error("not an array"); }
        *ret = n;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_root(clingo_configuration_t const *conf, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getRootKey(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_description(clingo_configuration_t const *conf, clingo_id_t key, char const **ret) {
    GRINGO_CLINGO_TRY {
        conf->getKeyInfo(key, nullptr, nullptr, ret, nullptr);
        if (!ret) { throw std::runtime_error("no description"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_value_get_size(clingo_configuration_t const *conf, clingo_id_t key, size_t *n) {
    GRINGO_CLINGO_TRY {
        std::string value;
        conf->getKeyValue(key, value);
        *n = value.size() + 1;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_value_get(clingo_configuration_t const *conf, clingo_id_t key, char *ret, size_t n) {
    GRINGO_CLINGO_TRY {
        std::string value;
        conf->getKeyValue(key, value);
        if (n < value.size() + 1) { throw std::length_error("not enough space"); }
        std::strcpy(ret, value.c_str());
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_value_set(clingo_configuration_t *conf, clingo_id_t key, char const *val) {
    GRINGO_CLINGO_TRY { conf->setKeyValue(key, val); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_configuration_value_is_assigned(clingo_configuration_t const *conf, clingo_id_t key, bool *ret) {
    GRINGO_CLINGO_TRY {
        int n = 0;
        conf->getKeyInfo(key, nullptr, nullptr, nullptr, &n);
        if (n < 0) { throw std::runtime_error("not a value"); }
        *ret = n > 0;
    }
    GRINGO_CLINGO_CATCH;
}

// {{{1 statistics

struct clingo_statistic : public Potassco::AbstractStatistics { };

extern "C" bool clingo_statistics_root(clingo_statistics_t const *stats, uint64_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->root(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_type(clingo_statistics_t const *stats, uint64_t key, clingo_statistics_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->type(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_array_size(clingo_statistics_t const *stats, uint64_t key, size_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->size(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_array_at(clingo_statistics_t const *stats, uint64_t key, size_t index, uint64_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->at(key, index); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_array_push(clingo_statistics_t *stats, uint64_t key, clingo_statistics_type_t type, uint64_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->push(key, static_cast<Potassco::Statistics_t>(type)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_map_size(clingo_statistics_t const *stats, uint64_t key, size_t *n) {
    GRINGO_CLINGO_TRY { *n = stats->size(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_map_has_subkey(clingo_statistics_t const *stats, uint64_t key, char const *name, bool* result) {
    GRINGO_CLINGO_TRY { uint64_t temp; *result = stats->find(key, name, &temp); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_map_subkey_name(clingo_statistics_t const *stats, uint64_t key, size_t index, char const **name) {
    GRINGO_CLINGO_TRY { *name = stats->key(key, index); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_map_at(clingo_statistics_t const *stats, uint64_t key, char const *name, uint64_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->get(key, name); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_map_add_subkey(clingo_statistics_t *stats, uint64_t key, char const *name, clingo_statistics_type_t type, uint64_t *ret) {
    GRINGO_CLINGO_TRY { *ret = stats->add(key, name, static_cast<Potassco::Statistics_t>(type)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_value_get(clingo_statistics_t const *stats, uint64_t key, double *value) {
    GRINGO_CLINGO_TRY { *value = stats->value(key); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_statistics_value_set(clingo_statistics_t *stats, uint64_t key, double value) {
    GRINGO_CLINGO_TRY { stats->set(key, value); }
    GRINGO_CLINGO_CATCH;
}


// {{{1 global functions

extern "C" bool clingo_parse_term(char const *str, clingo_logger_t logger, void *data, unsigned message_limit, clingo_symbol_t *ret) {
    GRINGO_CLINGO_TRY {
        Input::GroundTermParser parser;
        Logger::Printer printer;
        if (logger) {
            printer = [logger, data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, data); };
        }
        Logger log(printer, message_limit);
        Symbol sym = parser.parse(str, log);
        if (sym.type() == SymbolType::Special) { throw std::runtime_error("parsing failed"); }
        *ret = sym.rep();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_parse_program(char const *program, clingo_ast_callback_t cb, void *cb_data, clingo_logger_t logger, void *logger_data, unsigned message_limit) {
    GRINGO_CLINGO_TRY {
        Input::ASTBuilder builder([cb, cb_data](clingo_ast_statement_t const &stm) { handleCError(cb(&stm, cb_data)); });
        bool incmode = false;
        Input::NonGroundParser parser(builder, incmode);
        Logger::Printer printer;
        if (logger) { printer = [logger, logger_data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, logger_data); }; }
        Logger log(printer, message_limit);
        parser.pushStream("<string>", gringo_make_unique<std::istringstream>(program), log);
        parser.parse(log);
        if (log.hasError()) { throw std::runtime_error("syntax error"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_parse_files(char const * const *file, size_t n, clingo_ast_callback_t cb, void *cb_data, clingo_logger_t logger, void *logger_data, unsigned message_limit) {
    GRINGO_CLINGO_TRY {
        Input::ASTBuilder builder([cb, cb_data](clingo_ast_statement_t const &stm) { handleCError(cb(&stm, cb_data)); });
        bool incmode = false;
        Input::NonGroundParser parser(builder, incmode);
        Logger::Printer printer;
        if (logger) { printer = [logger, logger_data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, logger_data); }; }
        Logger log(printer, message_limit);
        for (auto it = file, ie = file + n; it != ie; ++it) {
            parser.pushFile(std::string{*it}, log);
        }
        if (n == 0) {
            parser.pushFile("-", log);
        }
        parser.parse(log);
        if (log.hasError()) { throw std::runtime_error("syntax error"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_add_string(char const *str, char const **ret) {
    GRINGO_CLINGO_TRY { *ret = String(str).c_str(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_version(int *major, int *minor, int *revision) {
    *major = CLINGO_VERSION_MAJOR;
    *minor = CLINGO_VERSION_MINOR;
    *revision = CLINGO_VERSION_REVISION;
}

// {{{1 backend

struct clingo_backend : clingo_control_t { };

extern "C" bool clingo_backend_begin(clingo_backend_t *backend) {
    GRINGO_CLINGO_TRY {
        if (!backend->beginAddBackend()) { throw std::runtime_error("backend not available"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_end(clingo_backend_t *backend) {
    GRINGO_CLINGO_TRY { backend->endAddBackend(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_literal_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY {
        if (body_n == 0 && head_n == 1 && !choice) { backend->addFact(*head); }
        outputRule(*backend->getBackend(), choice, {head, head_n}, {body, body_n});
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_weight_t lower, clingo_weighted_literal_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY { outputRule(*backend->getBackend(), choice, {head, head_n}, lower, {reinterpret_cast<Potassco::WeightLit_t const *>(body), body_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t prio, clingo_weighted_literal_t const* lits, size_t lits_n) {
    GRINGO_CLINGO_TRY { backend->getBackend()->minimize(prio, {reinterpret_cast<Potassco::WeightLit_t const *>(lits), lits_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t n) {
    GRINGO_CLINGO_TRY { backend->getBackend()->project({atoms, n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t v) {
    GRINGO_CLINGO_TRY { backend->getBackend()->external(atom, Potassco::Value_t(v)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_assume(clingo_backend_t *backend, clingo_literal_t const *literals, size_t n) {
    GRINGO_CLINGO_TRY { backend->getBackend()->assume({literals, n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->getBackend()->heuristic(atom, Potassco::Heuristic_t(type), bias, priority, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v, clingo_literal_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->getBackend()->acycEdge(node_u, node_v, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_backend_add_atom(clingo_backend_t *backend, clingo_symbol_t *symbol, clingo_atom_t *ret) {
    GRINGO_CLINGO_TRY {
        if (symbol) {
            if (Symbol{*symbol}.type() != SymbolType::Fun) {
                throw std::runtime_error("function expected");
            }
            *ret = backend->addAtom(Symbol{*symbol});
        }
        else { *ret = backend->addProgramAtom(); }
    }
    GRINGO_CLINGO_CATCH;
}

// {{{1 solve handle

struct clingo_solve_handle : public Gringo::SolveFuture { };

extern "C" bool clingo_solve_handle_get(clingo_solve_handle_t *handle, clingo_solve_result_bitset_t *result) {
    GRINGO_CLINGO_TRY { *result = handle->get(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_solve_handle_wait(clingo_solve_handle_t *handle, double timeout, bool *result) {
    try { *result = handle->wait(timeout); }
    catch (...) { std::terminate(); }
}
extern "C" bool clingo_solve_handle_cancel(clingo_solve_handle_t *handle) {
    GRINGO_CLINGO_TRY { handle->cancel(); }
    GRINGO_CLINGO_CATCH;
}
extern "C" bool clingo_solve_handle_close(clingo_solve_handle_t *handle) {
    GRINGO_CLINGO_TRY { if (handle) { delete handle; } }
    GRINGO_CLINGO_CATCH;
}
extern "C" bool clingo_solve_handle_model(clingo_solve_handle_t *handle, clingo_model_t const **model) {
    GRINGO_CLINGO_TRY {
        *model = handle->model();
    }
    GRINGO_CLINGO_CATCH;
}
extern "C" bool clingo_solve_handle_core(clingo_solve_handle_t *handle, clingo_literal_t const **core, size_t *size) {
    GRINGO_CLINGO_TRY {
        auto core_span = handle->unsatCore();
        *core = core_span.first;
        *size = core_span.size;
    }
    GRINGO_CLINGO_CATCH;
}
extern "C" bool clingo_solve_handle_resume(clingo_solve_handle_t *handle) {
    GRINGO_CLINGO_TRY { handle->resume(); }
    GRINGO_CLINGO_CATCH;
}

// {{{1 ast

struct clingo_ast {
    Input::AST ast;
};

extern "C" bool clingo_ast_get_type(clingo_ast_t *ast, clingo_ast_type_t *type) {
    GRINGO_CLINGO_TRY {
        *type = ast->ast.type();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_ast_acquire(clingo_ast_t *ast) {
    ast->ast.incRef();
}
extern "C" void clingo_ast_release(clingo_ast_t *ast) {
    ast->ast.decRef();
}

template <class T>
T &get_attr(clingo_ast_t *ast, clingo_ast_attribute_t attribute) {
    return mpark::get<T>(ast->ast.value(static_cast<clingo_ast_attribute>(attribute)));
}

extern "C" bool clingo_ast_has_attribute(clingo_ast_t *ast, clingo_ast_attribute_t attribute, bool *has_attribute) {
    GRINGO_CLINGO_TRY {
        *has_attribute = ast->ast.hasValue(static_cast<clingo_ast_attribute>(attribute));
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_type(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_attribute_type_t *type) {
    GRINGO_CLINGO_TRY {
        *type = static_cast<clingo_ast_attribute_type_t>(ast->ast.value(static_cast<clingo_ast_attribute>(attribute)).index());
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_clear(clingo_ast_t *ast, clingo_ast_attribute_t attribute) {
    GRINGO_CLINGO_TRY {
        get_attr<mpark::monostate>(ast, attribute) = mpark::monostate{};
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value) {
    GRINGO_CLINGO_TRY {
        *value = get_attr<int>(ast, attribute);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int value) {
    GRINGO_CLINGO_TRY {
        get_attr<int>(ast, attribute) = value;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_location_t *value) {
    GRINGO_CLINGO_TRY {
        *value = conv(get_attr<Location>(ast, attribute));
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_location_t const *value) {
    GRINGO_CLINGO_TRY {
        get_attr<Location>(ast, attribute) = conv(*value);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute, char const **value) {
    GRINGO_CLINGO_TRY {
        *value = get_attr<String>(ast, attribute).c_str();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute, char const *value) {
    GRINGO_CLINGO_TRY {
        get_attr<String>(ast, attribute) = value;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t **value) {
    GRINGO_CLINGO_TRY {
        *value = reinterpret_cast<clingo_ast_t*>(get_attr<Input::SAST>(ast, attribute).get());
        (*value)->ast.incRef();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t *value) {
    GRINGO_CLINGO_TRY {
        get_attr<Input::SAST>(ast, attribute) = Input::SAST{reinterpret_cast<Input::AST*>(value)};
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_string_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, char const **value) {
    GRINGO_CLINGO_TRY {
        *value = get_attr<Input::AST::StrVec>(ast, attribute).at(index).c_str();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_string_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, char const *value) {
    GRINGO_CLINGO_TRY {
        get_attr<Input::AST::StrVec>(ast, attribute).assign(index, value);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_delete_string_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index) {
    GRINGO_CLINGO_TRY {
        auto &arr = get_attr<Input::AST::StrVec>(ast, attribute);
        arr.erase(arr.begin() + index);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_size_string_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t *size) {
    GRINGO_CLINGO_TRY {
        *size = get_attr<Input::AST::StrVec>(ast, attribute).size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_insert_string_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, char const *value) {
    GRINGO_CLINGO_TRY {
        auto &arr = get_attr<Input::AST::StrVec>(ast, attribute);
        arr.insert(arr.begin() + index, value);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_get_ast_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, clingo_ast_t **value) {
    GRINGO_CLINGO_TRY {
        *value = reinterpret_cast<clingo_ast_t*>(get_attr<Input::AST::ASTVec>(ast, attribute).at(index).get());
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_set_ast_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, clingo_ast_t *value) {
    GRINGO_CLINGO_TRY {
        get_attr<Input::AST::ASTVec>(ast, attribute).assign(index, Input::SAST{reinterpret_cast<Input::AST*>(value)});
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_delete_ast_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index) {
    GRINGO_CLINGO_TRY {
        auto &arr = get_attr<Input::AST::ASTVec>(ast, attribute);
        arr.erase(arr.begin() + index);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_size_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t *size) {
    GRINGO_CLINGO_TRY {
        *size = get_attr<Input::AST::ASTVec>(ast, attribute).size();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_attribute_insert_ast_at(clingo_ast_t *ast, clingo_ast_attribute_t attribute, size_t index, clingo_ast_t *value) {
    GRINGO_CLINGO_TRY {
        auto &arr = get_attr<Input::AST::ASTVec>(ast, attribute);
        arr.insert(arr.begin() + index, Input::SAST{reinterpret_cast<Input::AST*>(value)});
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_parse_string(char const *program, clingo_ast_callback_v2_t cb, void *cb_data, clingo_logger_t logger, void *logger_data, unsigned message_limit) {
    GRINGO_CLINGO_TRY {
        auto builder = Input::build([cb, cb_data](Input::SAST ast) {
            handleCError(cb(reinterpret_cast<clingo_ast_t*>(ast.get()), cb_data));
        });
        bool incmode = false;
        Input::NonGroundParser parser{*builder, incmode};
        Logger::Printer printer;
        if (logger != nullptr) { printer = [logger, logger_data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, logger_data); }; }
        Logger log(printer, message_limit);
        parser.pushStream("<string>", gringo_make_unique<std::istringstream>(program), log);
        parser.parse(log);
        if (log.hasError()) { throw std::runtime_error("syntax error"); }
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_ast_parse_files(char const * const *file, size_t n, clingo_ast_callback_v2_t cb, void *cb_data, clingo_logger_t logger, void *logger_data, unsigned message_limit) {
    GRINGO_CLINGO_TRY {
        auto builder = Input::build([cb, cb_data](Input::SAST ast) {
            handleCError(cb(reinterpret_cast<clingo_ast_t*>(ast.get()), cb_data));
        });
        bool incmode = false;
        Input::NonGroundParser parser(*builder, incmode);
        Logger::Printer printer;
        if (logger != nullptr) { printer = [logger, logger_data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, logger_data); }; }
        Logger log(printer, message_limit);
        for (auto it = file, ie = file + n; it != ie; ++it) {
            parser.pushFile(std::string{*it}, log);
        }
        if (n == 0) {
            parser.pushFile("-", log);
        }
        parser.parse(log);
        if (log.hasError()) { throw std::runtime_error("syntax error"); }
    }
    GRINGO_CLINGO_CATCH;
}

// {{{1 control

struct clingo_program_builder : clingo_control_t { };
extern "C" bool clingo_program_builder_begin(clingo_program_builder_t *bld) {
    GRINGO_CLINGO_TRY { bld->beginAdd(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_program_builder_add(clingo_program_builder_t *bld, clingo_ast_statement_t const *stm) {
    GRINGO_CLINGO_TRY { bld->add(*stm); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_program_builder_end(clingo_program_builder_t *bld) {
    GRINGO_CLINGO_TRY { bld->endAdd(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_control_free(clingo_control_t *ctl) {
    delete ctl;
}

extern "C" bool clingo_control_add(clingo_control_t *ctl, char const *name, char const * const *params, size_t n, char const *part) {
    GRINGO_CLINGO_TRY {
        StringVec p;
        for (char const * const *it = params, * const *ie = it + n; it != ie; ++it) {
            p.emplace_back(*it);
        }
        ctl->add(name, p, part);
    }
    GRINGO_CLINGO_CATCH;
}

namespace {

struct ClingoContext : Context {
    ClingoContext(clingo_control_t *ctl, clingo_ground_callback_t cb, void *data)
    : ctl(ctl)
    , cb(cb)
    , data(data) {}

    bool callable(String) override {
        return cb != nullptr;
    }

    SymVec call(Location const &loc, String name, SymSpan args, Logger &) override {
        assert(cb);
        clingo_location_t loc_c{loc.beginFilename.c_str(), loc.endFilename.c_str(), loc.beginLine, loc.endLine, loc.beginColumn, loc.endColumn};
        auto ret = cb(&loc_c, name.c_str(), reinterpret_cast<clingo_symbol_t const *>(args.first), args.size, data, [](clingo_symbol_t const * ret_c, size_t n, void *data) -> bool {
            auto t = static_cast<ClingoContext*>(data);
            GRINGO_CLINGO_TRY {
                for (auto it = ret_c, ie = it + n; it != ie; ++it) {
                    t->ret.emplace_back(Symbol(*it));
                }
            } GRINGO_CLINGO_CATCH;
        }, this);
        if (!ret) { throw ClingoError(); }
        return std::move(this->ret);
    }
    void exec(ScriptType, Location, String) override {
        throw std::logic_error("Context::exec: not supported");
    }
    ~ClingoContext() noexcept = default;

    clingo_control_t *ctl;
    clingo_ground_callback_t cb;
    void *data;
    SymVec ret;
};

}

extern "C" bool clingo_control_ground(clingo_control_t *ctl, clingo_part_t const * vec, size_t n, clingo_ground_callback_t cb, void *data) {
    GRINGO_CLINGO_TRY {
        Control::GroundVec gv;
        gv.reserve(n);
        for (auto it = vec, ie = it + n; it != ie; ++it) {
            SymVec params;
            params.reserve(it->size);
            for (auto jt = it->params, je = jt + it->size; jt != je; ++jt) {
                params.emplace_back(Symbol(*jt));
            }
            gv.emplace_back(it->name, params);
        }
        ClingoContext cctx(ctl, cb, data);
        ctl->ground(gv, cb ? &cctx : nullptr);
    } GRINGO_CLINGO_CATCH;
}

namespace {

class ClingoSolveEventHandler : public SolveEventHandler {
public:
    ClingoSolveEventHandler(clingo_solve_event_callback_t cb, void *data)
    : cb_(cb)
    , data_(data) { }
private:
    bool on_model(Model &model) override {
        bool goon = true;
        if (!cb_(clingo_solve_event_type_model, &model, data_, &goon)) { throw ClingoError(); }
        return goon;
    }
    void on_finish(SolveResult ret, Potassco::AbstractStatistics *step, Potassco::AbstractStatistics *accu) override {
        bool goon = true;
        clingo_statistics_t *stats[] = {static_cast<clingo_statistics_t*>(step), static_cast<clingo_statistics_t*>(accu)};
        if (step && accu && !cb_(clingo_solve_event_type_statistics, &stats, data_, &goon)) {
            clingo_terminate("error in SolveEventHandler::on_statistics going to terminate");
        }
        if (!cb_(clingo_solve_event_type_finish, &ret, data_, &goon)) {
            clingo_terminate("error in SolveEventHandler::on_finish going to terminate");
        }
    }
private:
    clingo_solve_event_callback_t cb_;
    void *data_;
};

} // namespace

extern "C" bool clingo_control_solve(clingo_control_t *control, clingo_solve_mode_bitset_t mode, clingo_literal_t const *assumptions, size_t assumptions_size, clingo_solve_event_callback_t notify, void *data, clingo_solve_handle_t **handle) {
    GRINGO_CLINGO_TRY { *handle = static_cast<clingo_solve_handle_t*>(control->solve(
        Potassco::toSpan(assumptions, assumptions_size),
        mode,
        notify ? gringo_make_unique<ClingoSolveEventHandler>(notify, data) : nullptr
    ).release()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_assign_external(clingo_control_t *ctl, clingo_literal_t literal, clingo_truth_value_t value) {
    GRINGO_CLINGO_TRY {
        if (literal < 0) {
            literal = -literal;
            if      (value == Potassco::Value_t::True)  { value = Potassco::Value_t::False; }
            else if (value == Potassco::Value_t::False) { value = Potassco::Value_t::True; }
        }
        ctl->assignExternal(literal, static_cast<Potassco::Value_t>(value));
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_release_external(clingo_control_t *ctl, clingo_literal_t literal) {
    GRINGO_CLINGO_TRY { ctl->assignExternal(std::abs(literal), Potassco::Value_t::Release); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_program_builder(clingo_control_t *ctl, clingo_program_builder_t **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_program_builder_t*>(ctl); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_symbolic_atoms(clingo_control_t const *ctl, clingo_symbolic_atoms_t const **ret) {
    GRINGO_CLINGO_TRY { *ret = &ctl->getDomain(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_theory_atoms(clingo_control_t const *ctl, clingo_theory_atoms_t const **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_theory_atoms const *>(&ctl->theory()); }
    GRINGO_CLINGO_CATCH;
}

namespace {

class ClingoPropagator : public Propagator {
public:
    ClingoPropagator(clingo_propagator_t prop, void *data)
    : prop_(prop)
    , data_(data) { }
    void init(PropagateInit &init) override {
        if (prop_.init && !prop_.init(&init, data_)) { throw ClingoError(); }
    }

    void propagate(Potassco::AbstractSolver& solver, ChangeList const &changes) override {
        if (prop_.propagate && !prop_.propagate(static_cast<clingo_propagate_control_t*>(&solver), changes.first, changes.size, data_)) { throw ClingoError(); }
    }

    void undo(Potassco::AbstractSolver const &solver, ChangeList const &undo) override {
        if (prop_.undo) { prop_.undo(static_cast<clingo_propagate_control_t const *>(&solver), undo.first, undo.size, data_); }
    }

    void check(Potassco::AbstractSolver& solver) override {
        if (prop_.check && !prop_.check(static_cast<clingo_propagate_control_t*>(&solver), data_)) { throw ClingoError(); }
    }

    bool hasHeuristic() const override {
        return prop_.decide;
    }

    Lit decide(Id_t solverId, Potassco::AbstractAssignment const &assignment, Lit fallback) override {
        clingo_literal_t decision = 0;
        if (prop_.decide && !prop_.decide(solverId, const_cast<clingo_assignment_t*>(static_cast<clingo_assignment_t const*>(&assignment)), fallback, data_, &decision)) { throw ClingoError(); }
        return decision;
    }
private:
    clingo_propagator_t prop_;
    void *data_;
};

} // namespace

extern "C" bool clingo_control_register_propagator(clingo_control_t *ctl, clingo_propagator_t const *propagator, void *data, bool sequential) {
    GRINGO_CLINGO_TRY { ctl->registerPropagator(gringo_make_unique<ClingoPropagator>(*propagator, data), sequential); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_has_const(clingo_control_t const *ctl, char const *name, bool *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special;
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_get_const(clingo_control_t const *ctl, char const *name, clingo_symbol_t *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special ? sym.rep() : Symbol::createId(name).rep();
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_control_interrupt(clingo_control_t *ctl) {
    ctl->interrupt();
}

extern "C" bool clingo_control_load(clingo_control_t *ctl, char const *file) {
    GRINGO_CLINGO_TRY { ctl->load(file); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_set_enable_enumeration_assumption(clingo_control_t *ctl, bool value) {
    GRINGO_CLINGO_TRY { ctl->useEnumAssumption(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_get_enable_enumeration_assumption(clingo_control_t *ctl) {
    return ctl->useEnumAssumption();
}

extern "C" bool clingo_control_cleanup(clingo_control_t *ctl) {
    GRINGO_CLINGO_TRY { ctl->cleanup(); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_set_enable_cleanup(clingo_control_t *ctl, bool value) {
    GRINGO_CLINGO_TRY { ctl->enableCleanup(value); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_get_enable_cleanup(clingo_control_t *ctl) {
    return ctl->enableCleanup();
}

extern "C" bool clingo_control_backend(clingo_control_t *ctl, clingo_backend_t **ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_backend_t*>(ctl); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_configuration(clingo_control_t *ctl, clingo_configuration_t **conf) {
    GRINGO_CLINGO_TRY { *conf = static_cast<clingo_configuration_t*>(&ctl->getConf()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_is_conflicting(clingo_control_t const *control) {
    return control->isConflicting();
}

extern "C" bool clingo_control_statistics(clingo_control_t const *ctl, clingo_statistics_t const **stats) {
    GRINGO_CLINGO_TRY { *stats = static_cast<clingo_statistics_t const *>(ctl->statistics()); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_clasp_facade(clingo_control_t *ctl, void **clasp) {
    GRINGO_CLINGO_TRY { *clasp = ctl->claspFacade(); }
    GRINGO_CLINGO_CATCH;
}

namespace {

class Observer : public Backend {
public:
    Observer(clingo_ground_program_observer_t obs, void *data) : obs_(obs), data_(data) { }
    ~Observer() override = default;

    void initProgram(bool incremental) override {
        call(obs_.init_program, incremental);
    }
    void beginStep() override {
        call(obs_.begin_step);
    }
    void endStep() override {
        call(obs_.end_step);
    }

    void rule(Potassco::Head_t ht, Potassco::AtomSpan const& head,Potassco::LitSpan const &body) override {
        call(obs_.rule, ht == Potassco::Head_t::Choice, head.first, head.size, body.first, body.size);
    }
    void rule(Potassco::Head_t ht, Potassco::AtomSpan const &head, Weight_t bound,Potassco::WeightLitSpan const &body) override {
        call(obs_.weight_rule, ht == Potassco::Head_t::Choice, head.first, head.size, bound, reinterpret_cast<clingo_weighted_literal_t const *>(body.first), body.size);
    }
    void minimize(Weight_t prio, Potassco::WeightLitSpan const &lits) override {
        call(obs_.minimize, prio, reinterpret_cast<clingo_weighted_literal_t const *>(lits.first), lits.size);
    }
    void project(Potassco::AtomSpan const &atoms) override {
        call(obs_.project, atoms.first, atoms.size);
    }
    void output(Symbol sym, Potassco::Atom_t atom) override {
        call(obs_.output_atom, sym.rep(), atom);
    }
    void output(Symbol sym, Potassco::LitSpan const& condition) override {
        call(obs_.output_term, sym.rep(), condition.first, condition.size);
    }
    void output(Symbol sym, int value, Potassco::LitSpan const& condition) override {
        call(obs_.output_csp, sym.rep(), value, condition.first, condition.size);
    }
    void external(Atom_t a, Potassco::Value_t v) override {
        call(obs_.external, a, v);
    }
    void assume(Potassco::LitSpan const &lits) override {
        call(obs_.assume, lits.first, lits.size);
    }
    void heuristic(Atom_t a, Potassco::Heuristic_t t, int bias, unsigned prio, Potassco::LitSpan const &condition) override {
        call(obs_.heuristic, a, t, bias, prio, condition.first, condition.size);
    }
    void acycEdge(int s, int t, Potassco::LitSpan const &condition) override {
        call(obs_.acyc_edge, s, t, condition.first, condition.size);
    }

    void theoryTerm(Id_t termId, int number) override {
        call(obs_.theory_term_number, termId, number);
    }
    void theoryTerm(Id_t termId, StringSpan const &name) override {
        std::string s{name.first, name.size};
        call(obs_.theory_term_string, termId, s.c_str());
    }
    void theoryTerm(Id_t termId, int cId, Potassco::IdSpan const &args) override {
        call(obs_.theory_term_compound, termId, cId, args.first, args.size);
    }
    void theoryElement(Id_t elementId, Potassco::IdSpan const &terms, Potassco::LitSpan const &cond) override {
        call(obs_.theory_element, elementId, terms.first, terms.size, cond.first, cond.size);
    }
    void theoryAtom(Id_t atomOrZero, Id_t termId, Potassco::IdSpan const &elements) override {
        call(obs_.theory_atom, atomOrZero, termId, elements.first, elements.size);
    }
    void theoryAtom(Id_t atomOrZero, Id_t termId, Potassco::IdSpan const &elements, Id_t op, Id_t rhs) override {
        call(obs_.theory_atom_with_guard, atomOrZero, termId, elements.first, elements.size, op, rhs);
    }
private:
    template <class CB, class... Args>
    void call(CB *cb, Args&&... args) {
        if (cb && !(*cb)(std::forward<Args>(args)..., data_)) { throw ClingoError(); }
    }
private:
    clingo_ground_program_observer_t obs_;
    void *data_;
};

} // namespace

extern "C" bool clingo_control_register_observer(clingo_control_t *control, clingo_ground_program_observer_t const *observer, bool replace, void *data) {
    GRINGO_CLINGO_TRY { control->registerObserver(gringo_make_unique<Observer>(*observer, data), replace); }
    GRINGO_CLINGO_CATCH;
}

extern "C" bool clingo_control_new(char const *const * args, size_t n, clingo_logger_t logger, void *data, unsigned message_limit, clingo_control_t **ctl) {
    GRINGO_CLINGO_TRY {
        static std::mutex mut;
        std::lock_guard<std::mutex> grd(mut);
        *ctl = new ClingoLib(g_scripts(), numeric_cast<int>(n), args, logger ? [logger, data](Warnings code, char const *msg) { logger(static_cast<clingo_warning_t>(code), msg, data); } : Logger::Printer(nullptr), message_limit);
    }
    GRINGO_CLINGO_CATCH;
}

namespace {

class CScript : public Script {
public:
    CScript(clingo_script_t_ script, void *data) : script_(script), data_(data) { }
    ~CScript() noexcept override {
        if (script_.free) { script_.free(data_); }
    }
private:
    void exec(ScriptType, Location loc, String code) override {
        if (script_.execute) {
            auto l = conv(loc);
            handleCError(script_.execute(&l, code.c_str(), data_));
        }
    }
    SymVec call(Location const &loc, String name, SymSpan args, Logger &) override {
        using Data = std::pair<SymVec, std::exception_ptr>;
        Data data;
        auto l = conv(loc);
        forwardCError(script_.call(
            &l, name.c_str(), reinterpret_cast<clingo_symbol_t const *>(args.first), args.size,
            [](clingo_symbol_t const *symbols, size_t symbols_size, void *pdata) {
                auto &data = *static_cast<Data*>(pdata);
                GRINGO_CALLBACK_TRY {
                    for (auto it = symbols, ie = it + symbols_size; it != ie; ++it) {
                        data.first.emplace_back(Symbol{*it});
                    }
                }
                GRINGO_CALLBACK_CATCH(data.second);
            },
            &data, data_), &data.second);
        return data.first;
    }
    bool callable(String name) override {
        bool ret;
        handleCError(script_.callable(name.c_str(), &ret, data_));
        return ret;
    }
    void main(Control &ctl) override {
        handleCError(script_.main(&ctl, data_));
    }
    char const *version() override {
        return script_.version;
    }
private:
    clingo_script_t_ script_;
    void *data_;
};

} // namespace

extern "C" CLINGO_VISIBILITY_DEFAULT bool clingo_register_script_(clingo_ast_script_type_t type, clingo_script_t_ const *script, void *data) {
    GRINGO_CLINGO_TRY { g_scripts().registerScript(static_cast<clingo_ast_script_type>(type), gringo_make_unique<CScript>(*script, data)); }
    GRINGO_CLINGO_CATCH;
}

extern "C" CLINGO_VISIBILITY_DEFAULT char const *clingo_script_version_(clingo_ast_script_type_t type) {
    return g_scripts().version(static_cast<clingo_ast_script_type>(type));
}

extern "C" CLINGO_VISIBILITY_DEFAULT int clingo_main_(int argc, char *argv[]) {
    Gringo::ClingoApp app;
    return app.main(argc, argv);
}

char *str_duplicate(char const *str) {
    char *ret = new char[strlen(str) + 1];
    std::strcpy(ret, str);
    return ret;
}

struct clingo_options : ClingoApp {};

namespace {

class CClingoApp : public IClingoApp {
public:
    CClingoApp(clingo_application_t app, void *data)
    : app_(app)
    , data_{data} {
        name_ = app_.program_name ? app_.program_name(data_) : IClingoApp::program_name();
        version_ = app_.version ? app_.version(data_) : IClingoApp::version();
    }
    unsigned message_limit() const override {
        if (app_.message_limit) {
            return app_.message_limit(data_);
        }
        else {
            return IClingoApp::message_limit();
        }
    }
    char const *program_name() const override {
        return name_;
    }
    char const *version() const override {
        return version_;
    }
    bool has_main() const override {
        return app_.main;
    }
    void main(ClingoControl &ctl, std::vector<std::string> const &files) override {
        assert(has_main());
        std::vector<char const *> c_files;
        for (auto &x : files) {
            c_files.emplace_back(x.c_str());
        }
        handleCError(app_.main(&ctl, c_files.data(), c_files.size(), data_));
    }
    bool has_log() const override { return app_.logger; }
    void log(Gringo::Warnings code, char const *message) noexcept override {
        assert(has_log());
        app_.logger(static_cast<clingo_warning_t>(code), message, data_);
    }
    bool has_printer() const override { return app_.printer; }
    void print_model(Model *model, std::function<void()> printer) override {
        handleCError(app_.printer(model, [](void *data) {
            GRINGO_CLINGO_TRY {
                (*static_cast<std::function<void()>*>(data))();
            }
            GRINGO_CLINGO_CATCH;
        }, &printer, data_));
    }

    void register_options(ClingoApp &app) override {
        if (app_.register_options) {
            handleCError(app_.register_options(static_cast<clingo_options_t*>(&app), data_));
        }
    }
    void validate_options() override {
        if (app_.validate_options) {
            handleCError(app_.validate_options(data_));
        }
    }
private:
    clingo_application_t app_;
    void *data_;
    char const *name_;
    char const *version_;
};

} // namespace

extern "C" CLINGO_VISIBILITY_DEFAULT bool clingo_options_add(clingo_options_t *options, char const *group, char const *option, char const *description, bool (*parse) (char const *value, void *data), void *data, bool multi, char const *argument) {
    GRINGO_CLINGO_TRY {
        options->addOption(group, option, description, [parse, data](char const *value) { return parse(value, data); }, argument, multi);
    }
    GRINGO_CLINGO_CATCH;
}

extern "C" CLINGO_VISIBILITY_DEFAULT bool clingo_options_add_flag(clingo_options_t *options, char const *group, char const *option, char const *description, bool *target) {
    GRINGO_CLINGO_TRY { options->addFlag(group, option, description, *target); }
    GRINGO_CLINGO_CATCH;
}

extern "C" CLINGO_VISIBILITY_DEFAULT int clingo_main(clingo_application *application, char const *const * arguments, size_t size, void *data) {
    try {
        UIClingoApp app = gringo_make_unique<CClingoApp>(*application, data);
        std::vector<std::unique_ptr<char[]>> args_buf;
        std::vector<char *> args;
        args_buf.emplace_back(str_duplicate(app->program_name()));
        for (auto arg = arguments, end = arguments + size; arg != end; ++arg) {
            args_buf.emplace_back(str_duplicate(*arg));
        }
        args_buf.emplace_back(nullptr);
        for (auto &x : args_buf) { args.emplace_back(x.get()); }
        return Gringo::ClingoApp{std::move(app)}.main(args.size() - 1, args.data());
    }
    catch (...) {
        handleCXXError();
        std::cerr << "error during initialization: going to terminate:\n" << clingo_error_message() << std::endl;
        std::terminate();
    }
}

// }}}1

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

