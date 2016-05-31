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

// {{{1 c interface

// {{{2 error handling

namespace {

clingo_solve_result_t convert(SolveResult r) {
    return static_cast<clingo_solve_result_t>(r.satisfiable()) |
           static_cast<clingo_solve_result_t>(r.interrupted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_interrupted) |
           static_cast<clingo_solve_result_t>(r.exhausted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_exhausted);
}

template <class F>
std::string to_string(F f) {
    std::string ret;
    size_t n;
    handleError(f(nullptr, &n));
    ret.resize(n-1);
    handleError(f(const_cast<char *>(ret.data()), &n));
    return ret;
}

template <class F>
void print(char *ret, size_t *n, F f) {
    if (!n) { throw std::invalid_argument("size must not be null"); }
    if (!ret) {
        Gringo::CountStream cs;
        f(cs);
        *n = cs.count() + 1;
    }
    else {
        if (*n < 1) { throw std::length_error("not enough space"); }
        Gringo::ArrayStream as(ret, *n - 1);
        f(as);
        ret[*n - 1] = '\0';
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

// {{{2 value

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


// {{{2 value

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
    GRINGO_CLINGO_TRY {
        if (!n) { throw std::invalid_argument("size must not be null"); }
        if (!ret) {
            Gringo::CountStream cs;
            static_cast<Symbol&>(val).print(cs);
            *n = cs.count() + 1;
        }
        else {
            if (*n < 1) { throw std::length_error("not enough space"); }
            Gringo::ArrayStream as(ret, *n - 1);
            static_cast<Symbol&>(val).print(as);
            ret[*n - 1] = '\0';
        }
    } GRINGO_CLINGO_CATCH(nullptr);
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

// {{{2 symbolic atoms

extern "C" clingo_error_t clingo_symbolic_atoms_begin(clingo_symbolic_atoms_t *dom, clingo_signature_t *sig, clingo_symbolic_atom_iter_t *ret) {
    GRINGO_CLINGO_TRY { *ret = sig ? dom->begin(static_cast<Sig&>(*sig)) : dom->begin(); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_end(clingo_symbolic_atoms_t *dom, clingo_symbolic_atom_iter_t *ret) {
    GRINGO_CLINGO_TRY { *ret = dom->end(); }
    GRINGO_CLINGO_CATCH(&dom->owner().logger());
}

extern "C" clingo_error_t clingo_symbolic_atoms_lookup(clingo_symbolic_atoms_t *dom, clingo_symbol_t atom, clingo_symbolic_atom_iter_t *ret) {
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

// {{{2 theory atoms

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
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_theory_atoms_element_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->elemStr(value); }); }
    GRINGO_CLINGO_CATCH(nullptr);
}

extern "C" clingo_error_t clingo_theory_atoms_atom_to_string(clingo_theory_atoms_t *atoms, clingo_id_t value, char *ret, size_t *n) {
    GRINGO_CLINGO_TRY { print(ret, n, [atoms, value](std::ostream &out) { out << atoms->atomStr(value); }); }
    GRINGO_CLINGO_CATCH(nullptr);
}

// {{{2 model

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
    } GRINGO_CLINGO_CATCH(&m->owner().logger());
}

// {{{2 solve_iter

struct clingo_solve_iter : SolveIter { };

extern "C" clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model **m) {
    GRINGO_CLINGO_TRY {
        *m = static_cast<clingo_model*>(const_cast<Model*>(it->next()));
    } GRINGO_CLINGO_CATCH(&it->owner().logger());
}

extern "C" clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY {
        *ret = convert(it->get().satisfiable());
    } GRINGO_CLINGO_CATCH(&it->owner().logger());
}

extern "C" clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it) {
    GRINGO_CLINGO_TRY {
        it->close();
    } GRINGO_CLINGO_CATCH(&it->owner().logger());
}

// {{{2 control

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
    } GRINGO_CLINGO_CATCH(nullptr);
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
    } GRINGO_CLINGO_CATCH(&ctl->logger());
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

extern "C" clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_model_handler_t *model_handler, void *data, clingo_symbolic_literal_t const *assumptions, size_t n, clingo_solve_result_t *ret) {
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

// }}}2

namespace Clingo {

// {{{1 c++ interface

// {{{2 error handling

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

// {{{2 signature

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

// {{{2 symbol

Symbol::Symbol() {
    clingo_symbol_new_num(0, this);
}

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

Symbol Fun(char const *name, SymSpan args, bool sign) {
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

SymSpan Symbol::args() const {
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
    std::string ret;
    size_t n;
    handleError(clingo_symbol_to_string(*this, nullptr, &n));
    ret.resize(n-1);
    handleError(clingo_symbol_to_string(*this, const_cast<char*>(ret.data()), &n));
    return ret;
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

// {{{2 symbolic atoms

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

SymbolicAtomIter SymbolicAtoms::begin() {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_begin(atoms_, nullptr, &it));
    return {atoms_,  it};
}

SymbolicAtomIter SymbolicAtoms::begin(Signature sig) {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_begin(atoms_, &sig, &it));
    return {atoms_, it};
}

SymbolicAtomIter SymbolicAtoms::end() {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_end(atoms_, &it));
    return {atoms_, it};
}

SymbolicAtom SymbolicAtoms::lookup(Symbol atom) {
    clingo_symbolic_atom_iter it;
    handleError(clingo_symbolic_atoms_lookup(atoms_, atom, &it));
    return {atoms_, it};
}

std::vector<Signature> SymbolicAtoms::signatures() {
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

// {{{2 theory atoms

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
    return {atoms_, ret, n};
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
    return {atoms_, ret, n};
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
    return {atoms_, ret, n};
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

// {{{2 model

Model::Model(clingo_model_t *model)
: model_(model) { }

bool Model::contains(Symbol atom) const {
    return clingo_model_contains(model_, atom);
}

SymVec Model::atoms(ShowType show) const {
    SymVec ret;
    size_t n;
    handleError(clingo_model_atoms(model_, show, nullptr, &n));
    ret.resize(n);
    handleError(clingo_model_atoms(model_, show, ret.data(), &n));
    return ret;
}

// {{{2 solve iter

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

// {{{2 control

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
                        d.first(loc, name, {static_cast<Symbol const *>(args), n}, [cb, cbdata](SymSpan symret) {
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

SolveResult Control::solve(ModelHandler mh, SymbolicLiteralSpan assumptions) {
    clingo_solve_result_t ret;
    using Data = std::pair<ModelHandler&, std::exception_ptr>;
    Data data(mh, nullptr);
    clingo_control_solve(ctl_, [](clingo_model_t *m, void *data, bool *ret) -> clingo_error_t {
        auto &d = *static_cast<Data*>(data);
        CLINGO_CALLBACK_TRY { *ret = d.first(m); }
        CLINGO_CALLBACK_CATCH(d.second);
    }, &data, assumptions.begin(), assumptions.size(), &ret);
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

SymbolicAtoms Control::symbolic_atoms() {
    clingo_symbolic_atoms_t *ret;
    handleError(clingo_control_symbolic_atoms(ctl_, &ret));
    return ret;
}

TheoryAtoms Control::theory_atoms() {
    clingo_theory_atoms_t *ret;
    clingo_control_theory_atoms(ctl_, &ret);
    return ret;
}

// }}}2

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

// }}}1

} // namespace Clingo

