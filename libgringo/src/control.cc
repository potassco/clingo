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

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#include <gringo/control.hh>
#include <clingo.hh>

using namespace Gringo;

// c interface

// {{{1 error handling

namespace {

clingo_solve_result_t convert(SolveResult r) {
    return static_cast<clingo_solve_result_t>(r.satisfiable()) |
           static_cast<clingo_solve_result_t>(r.interrupted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_interrupted) |
           static_cast<clingo_solve_result_t>(r.exhausted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_exhausted);
}

template <class F>
std::string to_string(F f) {
    std::vector<char> ret;
    size_t n;
    handleError(f(nullptr, &n));
    ret.resize(n);
    handleError(f(ret.data(), &n));
    return std::string(ret.begin(), ret.end()-1);
}

template <class F>
void print(char *ret, size_t *n, F f) {
    if (!n) { throw std::invalid_argument("size must not be null"); }
    if (!ret) {
        Gringo::CountStream cs;
        f(cs);
        cs.flush();
        *n = cs.count() + 1;
    }
    else {
        if (*n < 1) { throw std::length_error("not enough space"); }
        Gringo::ArrayStream as(ret, *n);
        f(as);
        as << '\0';
        as.flush();
    }
}

}

extern "C" inline char const *clingo_message_code_str(clingo_message_code_t code) {
    switch (code) {
        case clingo_error_success:               { return "success"; }
        case clingo_error_runtime:               { return "runtime error"; }
        case clingo_error_bad_alloc:             { return "bad allocation"; }
        case clingo_error_logic:                 { return "logic error"; }
        case clingo_error_unknown:               { return "unknown error"; }
        case clingo_warning_operation_undefined: { return "operation_undefined"; }
        case clingo_warning_atom_undefined:      { return "atom undefined"; }
        case clingo_warning_file_included:       { return "file included"; }
        case clingo_warning_variable_unbounded:  { return "variable unbounded"; }
        case clingo_warning_global_variable:     { return "global variable"; }
    }
    return "unknown message code";
}

// {{{1 signature

extern "C" clingo_error_t clingo_signature_new(char const *name, uint32_t arity, bool sign, clingo_signature_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = Sig(name, arity, sign);
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" char const *clingo_signature_name(clingo_signature_t sig) {
    return static_cast<Sig&>(sig).name().c_str();
}

extern "C" uint32_t clingo_signature_arity(clingo_signature_t sig) {
    return static_cast<Sig&>(sig).arity();
}

extern "C" bool clingo_signature_sign(clingo_signature_t sig) {
    return static_cast<Sig&>(sig).sign();
}

extern "C" size_t clingo_signature_hash(clingo_signature_t sig) {
    return static_cast<Sig&>(sig).hash();
}

extern "C" bool clingo_signature_eq(clingo_signature_t a, clingo_signature_t b) {
    return static_cast<Sig&>(a) == static_cast<Sig&>(b);
}

extern "C" bool clingo_signature_lt(clingo_signature_t a, clingo_signature_t b) {
    return static_cast<Sig&>(a) < static_cast<Sig&>(b);
}


// {{{1 value

extern "C" void clingo_symbol_new_num(int num, clingo_symbol_t *val) {
    *val = Symbol::createNum(num);
}

extern "C" void clingo_symbol_new_sup(clingo_symbol_t *val) {
    *val = Symbol::createSup();
}

extern "C" void clingo_symbol_new_inf(clingo_symbol_t *val) {
    *val = Symbol::createInf();
}

extern "C" clingo_error_t clingo_symbol_new_str(char const *str, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createStr(str);
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_new_id(char const *id, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createId(id, sign);
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_new_fun(char const *name, clingo_symbol_t const *args, size_t n, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY {
        *val = Symbol::createFun(name, SymSpan{static_cast<Symbol const *>(args), n}, sign);
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_num(clingo_symbol_t val, int *num) {
    GRINGO_CLINGO_TRY {
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Num);
        *num = static_cast<Symbol&>(val).num();
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_name(clingo_symbol_t val, char const **name) {
    GRINGO_CLINGO_TRY {
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        *name = static_cast<Symbol&>(val).name().c_str();
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_string(clingo_symbol_t val, char const **str) {
    GRINGO_CLINGO_TRY {
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Str);
        *str = static_cast<Symbol&>(val).string().c_str();
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_sign(clingo_symbol_t val, bool *sign) {
    GRINGO_CLINGO_TRY {
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        *sign = static_cast<Symbol&>(val).sign();
        return clingo_error_success;
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_symbol_args(clingo_symbol_t val, clingo_symbol_t const **args, size_t *n) {
    GRINGO_CLINGO_TRY {
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        auto ret = static_cast<Symbol&>(val).args();
        *args = ret.first;
        *n = ret.size;
    } GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t val) {
    return static_cast<clingo_symbol_type_t>(static_cast<Symbol&>(val).type());
}

extern "C" clingo_error_t clingo_symbol_to_string(clingo_symbol_t val, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [&val](std::ostream &out) { static_cast<Symbol&>(val).print(out); }); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" bool clingo_symbol_eq(clingo_symbol_t a, clingo_symbol_t b) {
    return static_cast<Symbol&>(a) == static_cast<Symbol&>(b);
}

extern "C" bool clingo_symbol_lt(clingo_symbol_t a, clingo_symbol_t b) {
    return static_cast<Symbol&>(a) < static_cast<Symbol&>(b);
}

extern "C" size_t clingo_symbol_hash(clingo_symbol_t sym) {
    return static_cast<Symbol&>(sym).hash();
}

// {{{1 symbolic atoms

extern "C" clingo_error_t clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t *dom, clingo_signature_t *sig, clingo_symbolic_atom_iter_t *ret) {
    GRINGO_CLINGO_TRY { *ret = sig ? dom->begin(static_cast<Sig&>(*sig)) : dom->begin(); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_end(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->end(); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_find(clingo_symbolic_atoms_t *dom, clingo_symbol_t atom, clingo_symbolic_atom_iter_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->lookup(static_cast<Symbol&>(atom)); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_iter_eq(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t it, clingo_symbolic_atom_iter_t jt, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->eq(it, jt); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_signatures(clingo_symbolic_atoms_t *dom, clingo_signature_t *ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto sigs = dom->signatures();
        if (!n) { throw std::invalid_argument("size must be non-null"); }
        if (!ret) { *n = sigs.size(); }
        else {
            if (*n < sigs.size()) { throw std::length_error("not enough space"); }
            for (auto &sig : sigs) { *ret++ = sig; }
        }
    }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" size_t clingo_symbolic_atoms_length(clingo_symbolic_atoms_t *dom) {
    return dom->length();
}

extern "C" clingo_error_t clingo_symbolic_atoms_atom(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, clingo_symbol_t *sym) {
    GRINGO_CLINGO_TRY { *sym = dom->atom(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_literal(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, clingo_lit_t *lit) {
    GRINGO_CLINGO_TRY { *lit = dom->literal(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_fact(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, bool *fact) {
    GRINGO_CLINGO_TRY { *fact = dom->fact(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_external(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, bool *external) {
    GRINGO_CLINGO_TRY { *external = dom->external(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_next(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, clingo_symbolic_atom_iter_t *next) {
    GRINGO_CLINGO_TRY { *next = dom->next(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_valid(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t atm, bool *valid) {
    GRINGO_CLINGO_TRY { *valid = dom->valid(atm); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

// {{{1 theory atoms

extern "C" clingo_error_t clingo_theory_atoms_term_type(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_theory_term_type_t *ret) {
    GRINGO_CLINGO_TRY { *ret = static_cast<clingo_theory_term_type_t>(atoms->termType(value)); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_term_number(clingo_theory_atoms_t *atoms, clingo_id_t value, int *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termNum(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_term_name(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->termName(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_term_arguments(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->termArgs(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_element_tuple(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemTuple(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_element_condition(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_lit_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->elemCond(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_element_condition_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_lit_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->elemCondLit(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_elements(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t const **ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        auto span = atoms->atomElems(value);
        *ret = span.first;
        *n = span.size;
    }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_term(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_id_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomTerm(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_has_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomHasGuard(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_literal(clingo_theory_atoms_t *atoms, clingo_id_t value, clingo_lit_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->atomLit(value); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_guard(clingo_theory_atoms_t *atoms, clingo_id_t value, char const **ret_op, clingo_id_t *ret_term) {
    GRINGO_CLINGO_TRY {
        auto guard = atoms->atomGuard(value);
        *ret_op = guard.first;
        *ret_term = guard.second;
    }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_size(clingo_theory_atoms_t *atoms, size_t *ret) {
    GRINGO_CLINGO_TRY { *ret = atoms->numAtoms(); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_term_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->termStr(value); }); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_element_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

extern "C" clingo_error_t clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH(&atoms->owner().logger());
}

// {{{1 propagate init

extern "C" clingo_error_t clingo_propagate_init_map_literal(clingo_propagate_init_t *init, clingo_lit_t lit, clingo_lit_t *ret) {
    GRINGO_CLINGO_TRY { *ret = init->mapLit(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_propagate_init_add_watch(clingo_propagate_init_t *init, clingo_lit_t lit) {
    GRINGO_CLINGO_TRY { init->addWatch(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" int clingo_propagate_init_number_of_threads(clingo_propagate_init_t *init) {
    return init->threads();
}

extern "C" clingo_error_t clingo_propagate_init_symbolic_atoms(clingo_propagate_init_t *init, clingo_symbolic_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = &init->getDomain(); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_propagate_init_theory_atoms(clingo_propagate_init_t *init, clingo_theory_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = const_cast<Gringo::TheoryData*>(&init->theory()); }
    GRINGO_CLINGO_CATCH(nullptr);
}

// {{{1 assignment

struct clingo_assignment : public Potassco::AbstractAssignment { };

extern "C" bool clingo_assignment_has_conflict(clingo_assignment_t *ass) {
    return ass->hasConflict();
}

extern "C" uint32_t clingo_assignment_decision_level(clingo_assignment_t *ass) {
    return ass->level();
}

extern "C" bool clingo_assignment_has_literal(clingo_assignment_t *ass, clingo_lit_t lit) {
    return ass->hasLit(lit);
}

extern "C" clingo_error_t clingo_assignment_truth_value(clingo_assignment_t *ass, clingo_lit_t lit, clingo_truth_value_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->value(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_assignment_level(clingo_assignment_t *ass, clingo_lit_t lit, uint32_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->level(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_assignment_decision(clingo_assignment_t *ass, uint32_t level, clingo_lit_t *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->decision(level); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_assignment_is_fixed(clingo_assignment_t *ass, clingo_lit_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFixed(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_assignment_is_true(clingo_assignment_t *ass, clingo_lit_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isTrue(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_assignment_is_false(clingo_assignment_t *ass, clingo_lit_t lit, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ass->isFalse(lit); }
    GRINGO_CLINGO_CATCH(nullptr);
}

// {{{1 propagate control

struct clingo_propagate_control : Potassco::AbstractSolver { };

extern "C" clingo_id_t clingo_propagate_control_thread_id(clingo_propagate_control_t *ctl) {
    return ctl->id();
}

extern "C" clingo_assignment_t *clingo_propagate_control_assignment(clingo_propagate_control_t *ctl) {
    return const_cast<clingo_assignment *>(static_cast<clingo_assignment const *>(&ctl->assignment()));
}

extern "C" clingo_error_t clingo_propagate_control_add_clause(clingo_propagate_control_t *ctl, clingo_lit_t const *clause, size_t n, clingo_clause_type_t prop, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->addClause({clause, n}, Potassco::Clause_t(prop)); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_propagate_control_propagate(clingo_propagate_control_t *ctl, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = ctl->propagate(); }
    GRINGO_CLINGO_CATCH(nullptr);
}

// {{{1 model

extern "C" bool clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom) {
    return m->contains(static_cast<Symbol &>(atom));
}

extern "C" clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_symbol_t *ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        SymSpan atoms = m->atoms(show);
        if (!n) { throw std::invalid_argument("size must be non-null"); }
        if (!ret) { *n = atoms.size; }
        else {
            if (*n < atoms.size) { throw std::length_error("not enough space"); }
            for (auto it = atoms.first, ie = it + atoms.size; it != ie; ++it) { *ret++ = *it; }
        }
    }
    GRINGO_CLINGO_CATCH(&m->owner().logger());
}

extern "C" clingo_error_t clingo_model_optimization(clingo_model_t *m, int64_t *ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        // TODO: implement matching C++ functions ...
        auto opt = m->optimization();
        if (!n) { throw std::invalid_argument("size must be non-null"); }
        if (!ret) { *n = opt.size(); }
        else {
            if (*n < opt.size()) { throw std::length_error("not enough space"); }
            std::copy(opt.begin(), opt.end(), ret);
        }
    }
    GRINGO_CLINGO_CATCH(&m->owner().logger());
}

// {{{1 solve iter

struct clingo_solve_iter : SolveIter { };

extern "C" clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model **m) {
    GRINGO_CLINGO_TRY { *m = static_cast<clingo_model*>(const_cast<Model*>(it->next())); }
    GRINGO_CLINGO_CATCH(&it->owner().logger());
}

extern "C" clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY { *ret = convert(it->get().satisfiable()); }
    GRINGO_CLINGO_CATCH(&it->owner().logger());
}

extern "C" clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it) {
    GRINGO_CLINGO_TRY { it->close(); }
    GRINGO_CLINGO_CATCH(&it->owner().logger());
}

// {{{1 solve async

struct clingo_solve_async : SolveFuture { };

extern "C" clingo_error_t clingo_solve_async_cancel(clingo_solve_async_t *async) {
    GRINGO_CLINGO_TRY { async->cancel(); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_solve_async_get(clingo_solve_async_t *async, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY { *ret = async->get(); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_solve_async_wait(clingo_solve_async_t *async, double timeout, bool *ret) {
    GRINGO_CLINGO_TRY { *ret = async->wait(timeout); }
    GRINGO_CLINGO_CATCH(nullptr);
}

// {{{1 configuration

struct clingo_configuration : ConfigProxy { };

extern "C" clingo_error_t clingo_configuration_get_subkey(clingo_configuration_t *conf, unsigned key, char const *name, unsigned* subkey) {
    GRINGO_CLINGO_TRY { *subkey = conf->getSubKey(key, name); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_configuration_get_array_key(clingo_configuration_t *conf, unsigned key, unsigned idx, unsigned *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getArrKey(key, idx); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_configuration_get_info(clingo_configuration_t *conf, unsigned key, int* nsubkeys, int* arrlen, const char** help, int* nvalues) {
    GRINGO_CLINGO_TRY { conf->getKeyInfo(key, nsubkeys, arrlen, help, nvalues); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_configuration_root(clingo_configuration_t *conf, unsigned *ret) {
    GRINGO_CLINGO_TRY { *ret = conf->getRootKey(); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_configuration_get_value(clingo_configuration_t *conf, unsigned key, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY {
        std::string value;
        conf->getKeyValue(key, value);
        if (!n) { throw std::invalid_argument("size must be non-null"); }
        if (!ret) { *n = value.size(); }
        else {
            if (*n < value.size()) { throw std::length_error("not enough space"); }
            std::strcpy(ret, value.c_str());
        }
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_configuration_set_value(clingo_configuration_t *conf, unsigned key, const char *val) {
    GRINGO_CLINGO_TRY { conf->setKeyValue(key, val); }
    GRINGO_CLINGO_CATCH(nullptr);
}


// {{{1 global functions

extern "C" void clingo_version(int *major, int *minor, int *revision) {
    *major = CLINGO_VERSION_MAJOR;
    *minor = CLINGO_VERSION_MINOR;
    *revision = CLINGO_VERSION_REVISION;
}

// {{{1 backend

struct clingo_backend : clingo_control_t { };

extern "C" clingo_error_t clingo_backend_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_lit_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY { outputRule(*backend->backend(), choice, {head, head_n}, {body, body_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_weight_rule(clingo_backend_t *backend, bool choice, clingo_atom_t const *head, size_t head_n, clingo_weight_t lower, clingo_weight_lit_t const *body, size_t body_n) {
    GRINGO_CLINGO_TRY { outputRule(*backend->backend(), choice, {head, head_n}, lower, {reinterpret_cast<Potassco::WeightLit_t const *>(body), body_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_minimize(clingo_backend_t *backend, clingo_weight_t prio, clingo_weight_lit_t const* lits, size_t lits_n) {
    GRINGO_CLINGO_TRY { backend->backend()->minimize(prio, {reinterpret_cast<Potassco::WeightLit_t const *>(lits), lits_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_project(clingo_backend_t *backend, clingo_atom_t const *atoms, size_t n) {
    GRINGO_CLINGO_TRY { backend->backend()->project({atoms, n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_output(clingo_backend_t *backend, char const *name, clingo_lit_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->backend()->output({name, std::strlen(name)}, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_external(clingo_backend_t *backend, clingo_atom_t atom, clingo_external_type_t v) {
    GRINGO_CLINGO_TRY { backend->backend()->external(atom, Potassco::Value_t(v)); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_assume(clingo_backend_t *backend, clingo_lit_t const *literals, size_t n) {
    GRINGO_CLINGO_TRY { backend->backend()->assume({literals, n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_heuristic(clingo_backend_t *backend, clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_lit_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->backend()->heuristic(atom, Potassco::Heuristic_t(type), bias, priority, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_acyc_edge(clingo_backend_t *backend, int node_u, int node_v, clingo_lit_t const *condition, size_t condition_n) {
    GRINGO_CLINGO_TRY { backend->backend()->acycEdge(node_u, node_v, {condition, condition_n}); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

extern "C" clingo_error_t clingo_backend_add_atom(clingo_backend_t *backend, clingo_atom_t *ret) {
    GRINGO_CLINGO_TRY { *ret = backend->addProgramAtom(); }
    GRINGO_CLINGO_CATCH(&backend->logger());
}

// {{{1 control

extern "C" clingo_error_t clingo_control_new(clingo_module_t *mod, char const *const * args, size_t n, clingo_logger_t *logger, void *data, unsigned message_limit, clingo_control_t **ctl) {
    GRINGO_CLINGO_TRY {
        // NOTE: nullptr sentinel required by program options library
        // TODO: ask Benny about possible removal
        std::vector<char const *> argVec;
        for (auto it = args, ie = it + n; it != ie; ++it) {
            argVec.emplace_back(*it);
        }
        argVec.push_back(nullptr);
        *ctl = mod->newControl(n, argVec.data(), logger ? [logger, data](clingo_message_code_t code, char const *msg) { logger(code, msg, data); } : Gringo::Logger::Printer(nullptr), message_limit);
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" void clingo_control_free(clingo_control_t *ctl) {
    delete ctl;
}

extern "C" clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const * const *params, size_t n, char const *part) {
    GRINGO_CLINGO_TRY {
        FWStringVec p;
        for (char const * const *it = params, * const *ie = it + n; it != ie; ++it) {
            p.emplace_back(*it);
        }
        ctl->add(name, p, part);
    }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

namespace {

struct ClingoContext : Context {
    ClingoContext(clingo_control_t *ctl, clingo_ground_callback_t *cb, void *data)
    : ctl(ctl)
    , cb(cb)
    , data(data) {}

    bool callable(String) const override {
        return cb;
    }

    SymVec call(Location const &loc, String name, SymSpan args) override {
        assert(cb);
        clingo_location_t loc_c{loc.beginFilename.c_str(), loc.endFilename.c_str(), loc.beginLine, loc.endLine, loc.beginColumn, loc.endColumn};
        auto err = cb(loc_c, name.c_str(), args.first, args.size, data, [](clingo_symbol_t const * ret_c, size_t n, void *data) -> clingo_error_t {
            auto t = static_cast<ClingoContext*>(data);
            GRINGO_CLINGO_TRY {
                for (auto it = ret_c, ie = it + n; it != ie; ++it) {
                    t->ret.emplace_back(static_cast<Symbol const &>(*it));
                }
            } GRINGO_CLINGO_CATCH(&t->ctl->logger());
        }, static_cast<void*>(this));
        if (err != 0) { throw ClingoError(err); }
        return std::move(ret);
    }
    virtual ~ClingoContext() noexcept = default;

    clingo_control_t *ctl;
    clingo_ground_callback_t *cb;
    void *data;
    SymVec ret;
};

}

extern "C" clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_t const * vec, size_t n, clingo_ground_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        Control::GroundVec gv;
        gv.reserve(n);
        for (auto it = vec, ie = it + n; it != ie; ++it) {
            SymVec params;
            params.reserve(it->n);
            for (auto jt = it->params, je = jt + it->n; jt != je; ++jt) {
                params.emplace_back(static_cast<Symbol const &>(*jt));
            }
            gv.emplace_back(it->name, params);
        }
        ClingoContext cctx(ctl, cb, data);
        ctl->ground(gv, cb ? &cctx : nullptr);
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

namespace {

Control::Assumptions toAss(clingo_symbolic_literal_t const * assumptions, size_t n) {
    Control::Assumptions ass;
    for (auto it = assumptions, ie = it + n; it != ie; ++it) {
        ass.emplace_back(static_cast<Symbol const>(it->atom), !it->sign);
    }
    return ass;
}

}

extern "C" clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_model_callback_t *model_handler, void *data, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = static_cast<clingo_solve_result_t>(ctl->solve([model_handler, data](Model const &m) {
            bool ret;
            auto err = model_handler(static_cast<clingo_model*>(const_cast<Model*>(&m)), data, &ret);
            if (err != 0) { throw ClingoError(err); }
            return ret;
        }, toAss(assumptions, n)));
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_iter_t **it) {
    GRINGO_CLINGO_TRY {
        *it = static_cast<clingo_solve_iter_t*>(ctl->solveIter(toAss(assumptions, n)));
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value) {
    GRINGO_CLINGO_TRY {
        ctl->assignExternal(static_cast<Symbol const &>(atom), static_cast<Potassco::Value_t>(value));
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom) {
    GRINGO_CLINGO_TRY {
        ctl->assignExternal(static_cast<Symbol const &>(atom), Potassco::Value_t::Release);
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        ctl->parse(program, [data, cb](clingo_ast const &ast) {
            auto ret = cb(&ast, data);
            if (ret != 0) { throw ClingoError(ret); }
        });
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY {
        ctl->add([ctl, data, cb](std::function<void (clingo_ast const &)> f) {
            auto ref = std::make_pair(f, ctl);
            using RefType = decltype(ref);
            auto ret = cb(data, [](clingo_ast_t const *ast, void *data) -> clingo_error_t {
                auto &ref = *static_cast<RefType*>(data);
                GRINGO_CLINGO_TRY {
                    ref.first(static_cast<clingo_ast const &>(*ast));
                } GRINGO_CLINGO_CATCH(&ref.second->logger());
            }, static_cast<void*>(&ref));
            if (ret != 0) { throw ClingoError(ret); }
        });
    } GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_symbolic_atoms(clingo_control_t *ctl, clingo_symbolic_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = &ctl->getDomain(); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_theory_atoms(clingo_control_t *ctl, clingo_theory_atoms_t **ret) {
    GRINGO_CLINGO_TRY { *ret = const_cast<Gringo::TheoryData*>(&ctl->theory()); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

namespace {

class ClingoPropagator : public Gringo::Propagator {
public:
    ClingoPropagator(clingo_propagator_t prop, void *data)
    : prop_(prop)
    , data_(data) { }
    void init(Gringo::PropagateInit &init) override {
        auto ret = prop_.init(&init, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void propagate(Potassco::AbstractSolver& solver, const ChangeList& changes) override {
        auto ret = prop_.propagate(static_cast<clingo_propagate_control_t*>(&solver), changes.first, changes.size, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void undo(const Potassco::AbstractSolver& solver, const ChangeList& undo) override {
        auto ret = prop_.undo(static_cast<clingo_propagate_control_t*>(&const_cast<Potassco::AbstractSolver&>(solver)), undo.first, undo.size, data_);
        if (ret != 0) { throw ClingoError(ret); }
    }

    void check(Potassco::AbstractSolver& solver) override {
        auto ret = prop_.check(static_cast<clingo_propagate_control_t*>(&solver), data_);
        if (ret != 0) { throw ClingoError(ret); }
    }
private:
    clingo_propagator_t prop_;
    void *data_;
};

} // namespace

extern "C" clingo_error_t clingo_control_register_propagator(clingo_control_t *ctl, clingo_propagator_t propagator, void *data, bool sequential) {
    GRINGO_CLINGO_TRY { ctl->registerPropagator(gringo_make_unique<ClingoPropagator>(propagator, data), sequential); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_cleanup(clingo_control_t *ctl) {
    GRINGO_CLINGO_TRY { ctl->cleanupDomains(); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_has_const(clingo_control_t *ctl, char const *name, bool *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special;

    }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_get_const(clingo_control_t *ctl, char const *name, clingo_symbol_t *ret) {
    GRINGO_CLINGO_TRY {
        auto sym = ctl->getConst(name);
        *ret = sym.type() != SymbolType::Special ? sym : Symbol::createId(name);
    }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" void clingo_control_interrupt(clingo_control_t *ctl) {
    ctl->interrupt();
}

extern "C" clingo_error_t clingo_control_load(clingo_control_t *ctl, char const *file) {
    GRINGO_CLINGO_TRY { ctl->load(file); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_solve_async(clingo_control_t *ctl, clingo_model_callback_t *mh, void *mh_data, clingo_finish_callback_t *fh, void *fh_data, clingo_symbolic_literal_t const * assumptions, size_t n, clingo_solve_async_t **ret) {
    GRINGO_CLINGO_TRY {
        clingo_control::Assumptions ass;
        for (auto it = assumptions, ie = assumptions + n; it != ie; ++it) {
            ass.emplace_back(it->atom, it->sign);
        }
        *ret = static_cast<clingo_solve_async_t*>(ctl->solveAsync(
            [mh, mh_data](Gringo::Model const &m) {
                bool result;
                auto code = mh(&const_cast<Gringo::Model &>(m), mh_data, &result);
                if (code != 0) { throw ClingoError(code); }
                return result;
            }, [fh, fh_data](Gringo::SolveResult ret) {
                auto code = fh(ret, fh_data);
                if (code != 0) { throw ClingoError(code); }
            }, std::move(ass)));
    }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_use_enum_assumption(clingo_control_t *ctl, bool value) {
    GRINGO_CLINGO_TRY { ctl->useEnumAssumption(value); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_backend(clingo_control_t *ctl, clingo_backend_t **ret) {
    GRINGO_CLINGO_TRY {
        if (ctl->backend()) { *ret = static_cast<clingo_backend_t*>(ctl); }
        else { throw std::runtime_error("backend not available"); }
    }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

extern "C" clingo_error_t clingo_control_configuration(clingo_control_t *ctl, clingo_configuration_t **conf) {
    GRINGO_CLINGO_TRY { *conf = static_cast<clingo_configuration_t*>(&ctl->getConf()); }
    GRINGO_CLINGO_CATCH(&ctl->logger());
}

// }}}1

namespace Clingo {

// c++ interface

// {{{1 error handling

} namespace Gringo {

void handleError(clingo_error_t code, std::exception_ptr *exc) {
    switch (code) {
        case clingo_error_success:   { break; }
        case clingo_error_fatal:     { throw std::runtime_error("fatal error"); }
        case clingo_error_runtime:   { throw std::runtime_error("runtime error"); }
        case clingo_error_logic:     { throw std::logic_error("logic error"); }
        case clingo_error_bad_alloc: { throw std::bad_alloc(); }
        case clingo_error_unknown:   {
            if (exc && *exc) { std::rethrow_exception(*exc); }
            throw std::logic_error("unknown error");
        }
    }
}

} namespace Clingo {

// {{{1 signature

Signature::Signature(char const *name, uint32_t arity, bool sign) {
    handleError(clingo_signature_new(name, arity, sign, this));
}

char const *Signature::name() const {
    return clingo_signature_name(*this);
}

uint32_t Signature::arity() const {
    return clingo_signature_arity(*this);
}

bool Signature::sign() const {
    return clingo_signature_sign(*this);
}


size_t Signature::hash() const {
    return clingo_signature_hash(*this);
}

bool operator==(Signature a, Signature b) { return  clingo_signature_eq(a, b); }
bool operator!=(Signature a, Signature b) { return !clingo_signature_eq(a, b); }
bool operator< (Signature a, Signature b) { return  clingo_signature_lt(a, b); }
bool operator<=(Signature a, Signature b) { return !clingo_signature_lt(b, a); }
bool operator> (Signature a, Signature b) { return  clingo_signature_lt(b, a); }
bool operator>=(Signature a, Signature b) { return !clingo_signature_lt(a, b); }

// {{{1 symbol

Symbol::Symbol() {
    clingo_symbol_new_num(0, this);
}

Symbol::Symbol(clingo_symbol_t sym)
: clingo_symbol{sym.rep} { }

Symbol Num(int num) {
    clingo_symbol_t sym;
    clingo_symbol_new_num(num, &sym);
    return static_cast<Symbol&>(sym);
}

Symbol Sup() {
    clingo_symbol_t sym;
    clingo_symbol_new_sup(&sym);
    return static_cast<Symbol&>(sym);
}

Symbol Inf() {
    clingo_symbol_t sym;
    clingo_symbol_new_inf(&sym);
    return static_cast<Symbol&>(sym);
}

Symbol Str(char const *str) {
    clingo_symbol_t sym;
    handleError(clingo_symbol_new_str(str, &sym));
    return static_cast<Symbol&>(sym);
}

Symbol Id(char const *id, bool sign) {
    clingo_symbol_t sym;
    handleError(clingo_symbol_new_id(id, sign, &sym));
    return static_cast<Symbol&>(sym);
}

Symbol Fun(char const *name, SymbolSpan args, bool sign) {
    clingo_symbol_t sym;
    handleError(clingo_symbol_new_fun(name, args.begin(), args.size(), sign, &sym));
    return static_cast<Symbol&>(sym);
}

int Symbol::num() const {
    int ret;
    handleError(clingo_symbol_num(*this, &ret));
    return ret;
}

char const *Symbol::name() const {
    char const *ret;
    handleError(clingo_symbol_name(*this, &ret));
    return ret;
}

char const *Symbol::string() const {
    char const *ret;
    handleError(clingo_symbol_string(*this, &ret));
    return ret;
}

bool Symbol::sign() const {
    bool ret;
    handleError(clingo_symbol_sign(*this, &ret));
    return ret;
}

SymbolSpan Symbol::args() const {
    clingo_symbol_t const *ret;
    size_t n;
    handleError(clingo_symbol_args(*this, &ret, &n));
    return {static_cast<Symbol const *>(ret), n};
}

SymbolType Symbol::type() const {
    return static_cast<SymbolType>(clingo_symbol_type(*this));
}

#define CLINGO_CALLBACK_TRY try
#define CLINGO_CALLBACK_CATCH(ref) catch (...){ (ref) = std::current_exception(); return clingo_error_unknown; } return clingo_error_success

std::string Symbol::to_string() const {
    return ::to_string([this](char *ret, size_t *n) { return clingo_symbol_to_string(*this, ret, n); });
}

size_t Symbol::hash() const {
    return clingo_symbol_hash(*this);
}

std::ostream &operator<<(std::ostream &out, Symbol sym) {
    out << sym.to_string();
    return out;
}

bool operator==(Symbol a, Symbol b) { return  clingo_symbol_eq(a, b); }
bool operator!=(Symbol a, Symbol b) { return !clingo_symbol_eq(a, b); }
bool operator< (Symbol a, Symbol b) { return  clingo_symbol_lt(a, b); }
bool operator<=(Symbol a, Symbol b) { return !clingo_symbol_lt(b, a); }
bool operator> (Symbol a, Symbol b) { return  clingo_symbol_lt(b, a); }
bool operator>=(Symbol a, Symbol b) { return !clingo_symbol_lt(a, b); }

// {{{1 symbolic atoms

Symbol SymbolicAtom::symbol() const {
    Symbol ret;
    clingo_symbolic_atoms_atom(atoms_, range_, &ret);
    return ret;
}

clingo_lit_t SymbolicAtom::literal() const {
    clingo_lit_t ret;
    clingo_symbolic_atoms_literal(atoms_, range_, &ret);
    return ret;
}

bool SymbolicAtom::fact() const {
    bool ret;
    clingo_symbolic_atoms_fact(atoms_, range_, &ret);
    return ret;
}

bool SymbolicAtom::external() const {
    bool ret;
    clingo_symbolic_atoms_external(atoms_, range_, &ret);
    return ret;
}

SymbolicAtomIter &SymbolicAtomIter::operator++() {
    clingo_symbolic_atom_iter_t range;
    handleError(clingo_symbolic_atoms_next(atoms_, range_, &range));
    range_ = range;
    return *this;
}

SymbolicAtomIter::operator bool() const {
    bool ret;
    handleError(clingo_symbolic_atoms_valid(atoms_, range_, &ret));
    return ret;
}

bool SymbolicAtomIter::operator==(SymbolicAtomIter it) const {
    bool ret = atoms_ == it.atoms_;
    if (ret) { handleError(clingo_symbolic_atoms_iter_eq(atoms_, range_, it.range_, &ret)); }
    return ret;
}

SymbolicAtomIter SymbolicAtoms::begin() const {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_begin(atoms_, nullptr, &it));
    return {atoms_,  it};
}

SymbolicAtomIter SymbolicAtoms::begin(Signature sig) const {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_begin(atoms_, &sig, &it));
    return {atoms_, it};
}

SymbolicAtomIter SymbolicAtoms::end() const {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_end(atoms_, &it));
    return {atoms_, it};
}

SymbolicAtomIter SymbolicAtoms::find(Symbol atom) const {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_find(atoms_, atom, &it));
    return {atoms_, it};
}

std::vector<Signature> SymbolicAtoms::signatures() const {
    size_t n;
    clingo_symbolic_atoms_signatures(atoms_, nullptr, &n);
    Signature sig("", 0);
    std::vector<Signature> ret;
    ret.resize(n, sig);
    handleError(clingo_symbolic_atoms_signatures(atoms_, ret.data(), &n));
    return ret;
}

size_t SymbolicAtoms::length() const {
    return clingo_symbolic_atoms_length(atoms_);
}

// {{{1 theory atoms

TheoryTermType TheoryTerm::type() const {
    clingo_theory_term_type_t ret;
    handleError(clingo_theory_atoms_term_type(atoms_, id_, &ret));
    return static_cast<TheoryTermType>(ret);
}

int TheoryTerm::number() const {
    int ret;
    handleError(clingo_theory_atoms_term_number(atoms_, id_, &ret));
    return ret;
}

char const *TheoryTerm::name() const {
    char const *ret;
    handleError(clingo_theory_atoms_term_name(atoms_, id_, &ret));
    return ret;
}

TheoryTermSpan TheoryTerm::arguments() const {
    clingo_id_t const *ret;
    size_t n;
    handleError(clingo_theory_atoms_term_arguments(atoms_, id_, &ret, &n));
    return {ret, n, atoms_};
}

std::ostream &operator<<(std::ostream &out, TheoryTerm term) {
    out << term.to_string();
    return out;
}

std::string TheoryTerm::to_string() const {
    return ::to_string([this](char *ret, size_t *n) { return clingo_theory_atoms_term_to_string(atoms_, id_, ret, n); });
}


TheoryTermSpan TheoryElement::tuple() const {
    clingo_id_t const *ret;
    size_t n;
    handleError(clingo_theory_atoms_element_tuple(atoms_, id_, &ret, &n));
    return {ret, n, atoms_};
}

LitSpan TheoryElement::condition() const {
    clingo_lit_t const *ret;
    size_t n;
    handleError(clingo_theory_atoms_element_condition(atoms_, id_, &ret, &n));
    return {ret, n};
}

lit_t TheoryElement::condition_literal() const {
    clingo_lit_t ret;
    handleError(clingo_theory_atoms_element_condition_literal(atoms_, id_, &ret));
    return ret;
}

std::string TheoryElement::to_string() const {
    return ::to_string([this](char *ret, size_t *n) { return clingo_theory_atoms_element_to_string(atoms_, id_, ret, n); });
}

std::ostream &operator<<(std::ostream &out, TheoryElement term) {
    out << term.to_string();
    return out;
}

TheoryElementSpan TheoryAtom::elements() const {
    clingo_id_t const *ret;
    size_t n;
    handleError(clingo_theory_atoms_atom_elements(atoms_, id_, &ret, &n));
    return {ret, n, atoms_};
}

TheoryTerm TheoryAtom::term() const {
    clingo_id_t ret;
    handleError(clingo_theory_atoms_atom_term(atoms_, id_, &ret));
    return {atoms_, ret};
}

bool TheoryAtom::has_guard() const {
    bool ret;
    handleError(clingo_theory_atoms_atom_has_guard(atoms_, id_, &ret));
    return ret;
}

lit_t TheoryAtom::literal() const {
    clingo_lit_t ret;
    handleError(clingo_theory_atoms_atom_literal(atoms_, id_, &ret));
    return ret;
}

std::pair<char const *, TheoryTerm> TheoryAtom::guard() const {
    char const *name;
    clingo_id_t term;
    handleError(clingo_theory_atoms_atom_guard(atoms_, id_, &name, &term));
    return {name, {atoms_, term}};
}

std::string TheoryAtom::to_string() const {
    return ::to_string([this](char *ret, size_t *n) { return clingo_theory_atoms_atom_to_string(atoms_, id_, ret, n); });
}

std::ostream &operator<<(std::ostream &out, TheoryAtom term) {
    out << term.to_string();
    return out;
}

TheoryAtomIterator TheoryAtoms::begin() const {
    return {atoms_, 0};
}

TheoryAtomIterator TheoryAtoms::end() const {
    return {atoms_, clingo_id_t(size())};
}

size_t TheoryAtoms::size() const {
    size_t ret;
    handleError(clingo_theory_atoms_size(atoms_, &ret));
    return ret;
}

// {{{1 assignment

bool Assignment::has_conflict() const {
    return clingo_assignment_has_conflict(ass_);
}

uint32_t Assignment::decision_level() const {
    return clingo_assignment_decision_level(ass_);
}

bool Assignment::has_literal(lit_t lit) const {
    return clingo_assignment_has_literal(ass_, lit);
}

TruthValue Assignment::truth_value(lit_t lit) const {
    clingo_truth_value_t ret;
    handleError(clingo_assignment_truth_value(ass_, lit, &ret));
    return static_cast<TruthValue>(ret);
}

uint32_t Assignment::level(lit_t lit) const {
    uint32_t ret;
    handleError(clingo_assignment_level(ass_, lit, &ret));
    return ret;
}

lit_t Assignment::decision(uint32_t level) const {
    lit_t ret;
    handleError(clingo_assignment_decision(ass_, level, &ret));
    return ret;
}

bool Assignment::is_fixed(lit_t lit) const {
    bool ret;
    handleError(clingo_assignment_is_fixed(ass_, lit, &ret));
    return ret;
}

bool Assignment::is_true(lit_t lit) const {
    bool ret;
    handleError(clingo_assignment_is_true(ass_, lit, &ret));
    return ret;
}

bool Assignment::is_false(lit_t lit) const {
    bool ret;
    handleError(clingo_assignment_is_false(ass_, lit, &ret));
    return ret;
}

// {{{1 propagate control

lit_t PropagateInit::map_literal(lit_t lit) const {
    lit_t ret;
    handleError(clingo_propagate_init_map_literal(init_, lit, &ret));
    return ret;
}

void PropagateInit::add_watch(lit_t lit) {
    handleError(clingo_propagate_init_add_watch(init_, lit));
}

int PropagateInit::number_of_threads() const {
    return clingo_propagate_init_number_of_threads(init_);
}

SymbolicAtoms PropagateInit::symbolic_atoms() const {
    clingo_symbolic_atoms_t *ret;
    handleError(clingo_propagate_init_symbolic_atoms(init_, &ret));
    return ret;
}

TheoryAtoms PropagateInit::theory_atoms() const {
    clingo_theory_atoms_t *ret;
    handleError(clingo_propagate_init_theory_atoms(init_, &ret));
    return ret;
}

// {{{1 propagate control

id_t PropagateControl::thread_id() const {
    return clingo_propagate_control_thread_id(ctl_);
}

Assignment PropagateControl::assignment() const {
    return clingo_propagate_control_assignment(ctl_);
}

bool PropagateControl::add_clause(LitSpan clause, ClauseType type) {
    bool ret;
    handleError(clingo_propagate_control_add_clause(ctl_, clause.begin(), clause.size(), static_cast<clingo_clause_type_t>(type), &ret));
    return ret;
}

bool PropagateControl::propagate() {
    bool ret;
    handleError(clingo_propagate_control_propagate(ctl_, &ret));
    return ret;
}

// {{{1 propagator

void Propagator::init(PropagateInit &) { }
void Propagator::propagate(PropagateControl &, LitSpan) { }
void Propagator::undo(PropagateControl const &, LitSpan) { }
void Propagator::check(PropagateControl &) { }

// {{{1 model

Model::Model(clingo_model_t *model)
: model_(model) { }

bool Model::contains(Symbol atom) const {
    return clingo_model_contains(model_, atom);
}

OptimizationVector Model::optimization() const {
    OptimizationVector ret;
    size_t n;
    handleError(clingo_model_optimization(model_, nullptr, &n));
    ret.resize(n);
    handleError(clingo_model_optimization(model_, ret.data(), &n));
    return ret;
}

SymbolVector Model::atoms(ShowType show) const {
    SymbolVector ret;
    size_t n;
    handleError(clingo_model_atoms(model_, show, nullptr, &n));
    ret.resize(n);
    handleError(clingo_model_atoms(model_, show, ret.data(), &n));
    return ret;
}

// {{{1 solve iter

SolveIter::SolveIter()
: iter_(nullptr) { }

SolveIter::SolveIter(clingo_solve_iter_t *it)
: iter_(it) { }

SolveIter::SolveIter(SolveIter &&it)
: iter_(nullptr) { std::swap(iter_, it.iter_); }

SolveIter &SolveIter::operator=(SolveIter &&it) {
    std::swap(iter_, it.iter_);
    return *this;
}

Model SolveIter::next() {
    clingo_model_t *m = nullptr;
    if (iter_) { handleError(clingo_solve_iter_next(iter_, &m)); }
    return m;
}

SolveResult SolveIter::get() {
    clingo_solve_result_t ret = 0;
    if (iter_) { handleError(clingo_solve_iter_get(iter_, &ret)); }
    return ret;
}

void SolveIter::close() {
    if (iter_) {
        clingo_solve_iter_close(iter_);
        iter_ = nullptr;
    }
}

// {{{1 solve async

void SolveAsync::cancel() {
    handleError(clingo_solve_async_cancel(async_));
}

SolveResult SolveAsync::get() {
    clingo_solve_result_t ret;
    handleError(clingo_solve_async_get(async_, &ret));
    return ret;
}

bool SolveAsync::wait(double timeout) {
    bool ret;
    handleError(clingo_solve_async_wait(async_, timeout, &ret));
    return ret;
}

// {{{1 backend

void Backend::rule(bool choice, AtomSpan head, LitSpan body) {
    handleError(clingo_backend_rule(backend_, choice, head.begin(), head.size(), body.begin(), body.size()));
}

void Backend::weight_rule(bool choice, AtomSpan head, weight_t lower, WeightLitSpan body) {
    handleError(clingo_backend_weight_rule(backend_, choice, head.begin(), head.size(), lower, body.begin(), body.size()));
}

void Backend::minimize(weight_t prio, WeightLitSpan body) {
    handleError(clingo_backend_minimize(backend_, prio, body.begin(), body.size()));
}

void Backend::project(AtomSpan atoms) {
    handleError(clingo_backend_project(backend_, atoms.begin(), atoms.size()));
}

void Backend::output(char const *name, LitSpan condition) {
    handleError(clingo_backend_output(backend_, name, condition.begin(), condition.size()));
}

void Backend::external(atom_t atom, ExternalType type) {
    handleError(clingo_backend_external(backend_, atom, static_cast<clingo_external_type_t>(type)));
}

void Backend::assume(LitSpan lits) {
    handleError(clingo_backend_assume(backend_, lits.begin(), lits.size()));
}

void Backend::heuristic(atom_t atom, HeuristicType type, int bias, unsigned priority, LitSpan condition) {
    handleError(clingo_backend_heuristic(backend_, atom, static_cast<clingo_heuristic_type_t>(type), bias, priority, condition.begin(), condition.size()));
}

void Backend::acyc_edge(int node_u, int node_v, LitSpan condition) {
    handleError(clingo_backend_acyc_edge(backend_, node_u, node_v, condition.begin(), condition.size()));
}

atom_t Backend::add_atom() {
    clingo_atom_t ret;
    handleError(clingo_backend_add_atom(backend_, &ret));
    return ret;
}

// {{{1 configuration

Configuration Configuration::operator[](unsigned index) {
    unsigned ret;
    clingo_configuration_get_array_key(conf_, key_, index, &ret);
    return {conf_, ret};
}

ConfigurationArrayIterator Configuration::begin() {
    return {conf_, 0};
}

ConfigurationArrayIterator Configuration::end() {
    return {conf_, unsigned(size())};
}

size_t Configuration::size() const {
    int n;
    handleError(clingo_configuration_get_info(conf_, key_, &n, nullptr, nullptr, nullptr));
    if (n < 0) { std::runtime_error("not an array"); }
    return n;
}

bool Configuration::empty() const {
    return size() == 0;
}

Configuration Configuration::operator[](char const *name) {
    unsigned ret;
    handleError(clingo_configuration_get_subkey(conf_, key_, name, &ret));
    return {conf_, ret};
}

ConfigurationKeyRange Configuration::keys() const {
    int n;
    handleError(clingo_configuration_get_info(conf_, key_, &n, nullptr, nullptr, nullptr));
    return { {conf_, key_, 0}, {conf_, key_, unsigned(n)} };
}

bool Configuration::leaf() const {
    int n;
    handleError(clingo_configuration_get_info(conf_, key_, &n, nullptr, nullptr, nullptr));
    return n == 0;
}
bool Configuration::has_value() const {
    int ret;
    handleError(clingo_configuration_get_info(conf_, key_, nullptr, nullptr, nullptr, &ret));
    return ret == 1;
}

std::string Configuration::value() const {
    size_t n;
    handleError(clingo_configuration_get_value(conf_, key_, nullptr, &n));
    std::vector<char> ret(n);
    handleError(clingo_configuration_get_value(conf_, key_, ret.data(), &n));
    return std::string(ret.begin(), ret.end() - 1);
}

bool Configuration::assignable() const {
    int ret;
    handleError(clingo_configuration_get_info(conf_, key_, nullptr, nullptr, nullptr, &ret));
    return ret != -1;
}

Configuration &Configuration::operator=(char const *value) {
    handleError(clingo_configuration_set_value(conf_, key_, value));
    return *this;
}

char const *Configuration::decription() const {
    char const *ret;
    handleError(clingo_configuration_get_info(conf_, key_, nullptr, nullptr, &ret, nullptr));
    return ret;
}

// {{{1 control

Control::Control(clingo_control_t *ctl)
: ctl_(ctl) { }

Control::~Control() noexcept {
    clingo_control_free(ctl_);
}

void Control::add(char const *name, StringSpan params, char const *part) {
    handleError(clingo_control_add(ctl_, name, params.begin(), params.size(), part));
}

void Control::ground(PartSpan parts, GroundCallback cb) {
    using Data = std::pair<GroundCallback&, std::exception_ptr>;
    Data data(cb, nullptr);
    handleError(clingo_control_ground(ctl_, parts.begin(), parts.size(),
        [](clingo_location_t loc, char const *name, clingo_symbol_t const *args, size_t n, void *data, clingo_symbol_callback_t *cb, void *cbdata) -> clingo_error_t {
            auto &d = *static_cast<Data*>(data);
            CLINGO_CALLBACK_TRY {
                if (d.first) {
                    struct Ret { clingo_error_t ret; };
                    try {
                        d.first(loc, name, {static_cast<Symbol const *>(args), n}, [cb, cbdata](SymbolSpan symret) {
                            clingo_error_t ret = cb(symret.begin(), symret.size(), cbdata);
                            if (ret != clingo_error_success) { throw Ret { ret }; }
                        });
                    }
                    catch (Ret e) { return e.ret; }
                }
            }
            CLINGO_CALLBACK_CATCH(d.second);
        }, &data), &data.second);
}

Control::operator clingo_control_t*() const { return ctl_; }

SolveResult Control::solve(ModelCallback mh, SymbolicLiteralSpan assumptions) {
    clingo_solve_result_t ret;
    using Data = std::pair<ModelCallback&, std::exception_ptr>;
    Data data(mh, nullptr);
    handleError(clingo_control_solve(ctl_, [](clingo_model_t *m, void *data, bool *ret) -> clingo_error_t {
        auto &d = *static_cast<Data*>(data);
        CLINGO_CALLBACK_TRY { *ret = d.first(m); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, assumptions.begin(), assumptions.size(), &ret));
    return ret;
}

SolveIter Control::solve_iter(SymbolicLiteralSpan assumptions) {
    clingo_solve_iter_t *it;
    handleError(clingo_control_solve_iter(ctl_, assumptions.begin(), assumptions.size(), &it));
    return it;
}

void Control::assign_external(Symbol atom, TruthValue value) {
    handleError(clingo_control_assign_external(ctl_, atom, static_cast<clingo_truth_value_t>(value)));
}

void Control::release_external(Symbol atom) {
    handleError(clingo_control_release_external(ctl_, atom));
}

SymbolicAtoms Control::symbolic_atoms() const {
    clingo_symbolic_atoms_t *ret;
    handleError(clingo_control_symbolic_atoms(ctl_, &ret));
    return ret;
}

TheoryAtoms Control::theory_atoms() const {
    clingo_theory_atoms_t *ret;
    clingo_control_theory_atoms(ctl_, &ret);
    return ret;
}

namespace {

// NOTE: I see no easy way to pass the exception object through
static clingo_error_t g_init(clingo_propagate_init_t *ctl, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateInit pi(ctl);
        p->init(pi);
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

static clingo_error_t g_propagate(clingo_propagate_control_t *ctl, clingo_lit_t const *changes, size_t n, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->propagate(pc, {changes, n});
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

static clingo_error_t g_undo(clingo_propagate_control_t *ctl, clingo_lit_t const *changes, size_t n, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->undo(pc, {changes, n});
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

static clingo_error_t g_check(clingo_propagate_control_t *ctl, Propagator *p) {
    GRINGO_CLINGO_TRY {
        PropagateControl pc(ctl);
        p->check(pc);
    }
    GRINGO_CLINGO_CATCH(nullptr);
}

static clingo_propagator_t g_propagator = {
    reinterpret_cast<decltype(clingo_propagator_t::init)>(&g_init),
    reinterpret_cast<decltype(clingo_propagator_t::propagate)>(g_propagate),
    reinterpret_cast<decltype(clingo_propagator_t::undo)>(g_undo),
    reinterpret_cast<decltype(clingo_propagator_t::check)>(g_check)
};

}

void Control::register_propagator(Propagator &propagator, bool sequential) {
    handleError(clingo_control_register_propagator(ctl_, g_propagator, &propagator, sequential));
}

void Control::cleanup() {
    handleError(clingo_control_cleanup(ctl_));
}

bool Control::has_const(char const *name) const {
    bool ret;
    handleError(clingo_control_has_const(ctl_, name, &ret));
    return ret;
}

Symbol Control::get_const(char const *name) const {
    clingo_symbol_t ret;
    handleError(clingo_control_get_const(ctl_, name, &ret));
    return ret;
}

void Control::interrupt() noexcept {
    clingo_control_interrupt(ctl_);
}

void Control::load(char const *file) {
    handleError(clingo_control_load(ctl_, file));
}

SolveAsync Control::solve_async(ModelCallback &mh, FinishCallback &fh, SymbolicLiteralSpan assumptions) {
    clingo_solve_async_t *ret;
    handleError(clingo_control_solve_async(ctl_, [](clingo_model_t *m, void *data, bool *ret) -> clingo_error_t {
        GRINGO_CLINGO_TRY {
            auto &mh = *static_cast<ModelCallback*>(data);
            *ret = !mh || mh(m);
        }
        GRINGO_CLINGO_CATCH(nullptr);
    }, &mh, [](clingo_solve_result_t res, void *data) -> clingo_error_t {
        GRINGO_CLINGO_TRY {
            auto &fh = *static_cast<FinishCallback*>(data);
            if (fh) { fh(res); }
        }
        GRINGO_CLINGO_CATCH(nullptr);
    }, &fh, assumptions.begin(), assumptions.size(), &ret));
    return ret;
}

void Control::use_enum_assumption(bool value) {
    handleError(clingo_control_use_enum_assumption(ctl_, value));
}

Backend Control::backend() {
    clingo_backend_t *ret;
    handleError(clingo_control_backend(ctl_, &ret));
    return ret;
}

Configuration Control::configuration() {
    clingo_configuration_t *conf;
    handleError(clingo_control_configuration(ctl_, &conf));
    unsigned key;
    handleError(clingo_configuration_root(conf, &key));
    return {conf, key};
}

// }}}1

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

} // namespace Clingo

