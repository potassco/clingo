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

#include <gringo/control.hh>

using namespace Gringo;

// {{{1 error handling

namespace {

clingo_solve_result_t convert(SolveResult r) {
    return static_cast<clingo_solve_result_t>(r.satisfiable()) |
           static_cast<clingo_solve_result_t>(r.interrupted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_interrupted) |
           static_cast<clingo_solve_result_t>(r.exhausted()) * static_cast<clingo_solve_result_t>(clingo_solve_result_exhausted);
}

}


extern "C" inline char const *clingo_error_str(clingo_error_t err) {
    switch (err) {
        case clingo_error_success:   { return nullptr; }
        case clingo_error_runtime:   { return "runtime error"; }
        case clingo_error_bad_alloc: { return "bad allocation"; }
        case clingo_error_logic:     { return "logic error"; }
        case clingo_error_unknown:   { return "unknown error"; }
    }
    return nullptr;
}

// {{{1 value

using namespace Gringo;

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
    GRINGO_CLINGO_TRY
        *val = Symbol::createStr(str);
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_new_id(char const *id, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY
        *val = Symbol::createId(id, sign);
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_new_fun(char const *name, clingo_symbol_span_t args, bool sign, clingo_symbol_t *val) {
    GRINGO_CLINGO_TRY
        *val = Symbol::createFun(name, SymSpan{static_cast<Symbol const *>(args.first), args.size}, sign);
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_num(clingo_symbol_t val, int *num) {
    GRINGO_CLINGO_TRY
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Num);
        *num = static_cast<Symbol&>(val).num();
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_name(clingo_symbol_t val, char const **name) {
    GRINGO_CLINGO_TRY
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        *name = static_cast<Symbol&>(val).name().c_str();
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_string(clingo_symbol_t val, char const **str) {
    GRINGO_CLINGO_TRY
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Str);
        *str = static_cast<Symbol&>(val).string().c_str();
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_sign(clingo_symbol_t val, bool *sign) {
    GRINGO_CLINGO_TRY
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        *sign = static_cast<Symbol&>(val).sign();
        return clingo_error_success;
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_symbol_args(clingo_symbol_t val, clingo_symbol_span_t *args) {
    GRINGO_CLINGO_TRY
        clingo_expect(static_cast<Symbol&>(val).type() == SymbolType::Fun);
        auto ret = static_cast<Symbol&>(val).args();
        *args = clingo_symbol_span_t{ret.first, ret.size};
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_symbol_type_t clingo_symbol_type(clingo_symbol_t val) {
    return static_cast<clingo_symbol_type_t>(static_cast<Symbol&>(val).type());
}

extern "C" clingo_error_t clingo_symbol_to_string(clingo_symbol_t val, clingo_string_callback *cb, void *data) {
    GRINGO_CLINGO_TRY
        std::ostringstream oss;
        static_cast<Symbol&>(val).print(oss);
        std::string s = oss.str();
        return cb(s.c_str(), data);
    GRINGO_CLINGO_CATCH;
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

// {{{1 module

struct clingo_module : GringoModule { };

// {{{1 model

struct clingo_model : Model { };

extern "C" bool clingo_model_contains(clingo_model_t *m, clingo_symbol_t atom) {
    return m->contains(static_cast<Symbol &>(atom));
}

extern "C" clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_symbol_span_t *ret) {
    GRINGO_CLINGO_TRY
        SymSpan atoms = m->atoms(show);
        *ret = {atoms.first, atoms.size};
    GRINGO_CLINGO_CATCH
}

// {{{1 solve_iter

struct clingo_solve_iter : SolveIter { };

extern "C" clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model **m) {
    GRINGO_CLINGO_TRY
        *m = static_cast<clingo_model*>(const_cast<Model*>(it->next()));
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY
        *ret = convert(it->get().satisfiable());
    GRINGO_CLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it) {
    GRINGO_CLINGO_TRY
        it->close();
    GRINGO_CLINGO_CATCH;
}

// {{{1 control

struct clingo_control : Control { };

extern "C" clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_control_t **ctl) {
    GRINGO_CLINGO_TRY
        *ctl = static_cast<clingo_control_t*>(mod->newControl(argc, argv));
    GRINGO_CLINGO_CATCH;
}

extern "C" void clingo_control_free(clingo_control_t *ctl) {
    delete ctl;
}

extern "C" clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const **params, char const *part) {
    GRINGO_CLINGO_TRY
        FWStringVec p;
        for (char const **param = params; *param; ++param) { p.emplace_back(*param); }
        ctl->add(name, p, part);
    GRINGO_CLINGO_CATCH;
}

namespace {

struct ClingoContext : Context {
    ClingoContext(clingo_ground_callback_t *cb, void *data)
    : cb(cb)
    , data(data) {}

    bool callable(String) const override {
        return cb;
    }

    SymVec call(Location const &, String name, SymSpan args) override {
        assert(cb);
        clingo_symbol_span_t args_c;
        args_c = { args.first, args.size };
        SymVec ret;
        auto err = cb(name.c_str(), args_c, data, [](clingo_symbol_span_t ret_c, void *ret) -> clingo_error_t {
            GRINGO_CLINGO_TRY
                for (auto it = ret_c.first, ie = it + ret_c.size; it != ie; ++it) {
                    static_cast<SymVec*>(ret)->emplace_back(static_cast<Symbol const &>(*it));
                }
            GRINGO_CLINGO_CATCH
        }, static_cast<void*>(&ret));
        if (err != 0) { throw ClingoError(err); }
        return ret;
    }
    virtual ~ClingoContext() noexcept = default;

    clingo_ground_callback_t *cb;
    void *data;
};

}

extern "C" clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY
        Control::GroundVec gv;
        gv.reserve(vec.size);
        for (auto it = vec.first, ie = it + vec.size; it != ie; ++it) {
            SymVec params;
            params.reserve(it->params.size);
            for (auto jt = it->params.first, je = jt + it->params.size; jt != je; ++jt) {
                params.emplace_back(static_cast<Symbol const &>(*jt));
            }
            gv.emplace_back(it->name, params);
        }
        ClingoContext cctx(cb, data);
        ctl->ground(gv, cb ? &cctx : nullptr);
    GRINGO_CLINGO_CATCH;
}

namespace {

Control::Assumptions toAss(clingo_symbolic_literal_span_t assumptions) {
    Control::Assumptions ass;
    for (auto it = assumptions.first, ie = it + assumptions.size; it != ie; ++it) {
        ass.emplace_back(static_cast<Symbol const>(it->atom), !it->sign);
    }
    return ass;
}

}

extern "C" clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_model_handler_t *model_handler, void *data, clingo_solve_result_t *ret) {
    GRINGO_CLINGO_TRY
        *ret = static_cast<clingo_solve_result_t>(ctl->solve([model_handler, data](Model const &m) {
            bool ret;
            auto err = model_handler(static_cast<clingo_model*>(const_cast<Model*>(&m)), data, &ret);
            if (err != 0) { throw ClingoError(err); }
            return ret;
        }, toAss(assumptions)));
    GRINGO_CLINGO_CATCH
}

extern "C" clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t assumptions, clingo_solve_iter_t **it) {
    GRINGO_CLINGO_TRY
        *it = static_cast<clingo_solve_iter_t*>(ctl->solveIter(toAss(assumptions)));
    GRINGO_CLINGO_CATCH
}

extern "C" clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_symbol_t atom, clingo_truth_value_t value) {
    GRINGO_CLINGO_TRY
        ctl->assignExternal(static_cast<Symbol const &>(atom), static_cast<Potassco::Value_t>(value));
    GRINGO_CLINGO_CATCH
}

extern "C" clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_symbol_t atom) {
    GRINGO_CLINGO_TRY
        ctl->assignExternal(static_cast<Symbol const &>(atom), Potassco::Value_t::Release);
    GRINGO_CLINGO_CATCH
}

extern "C" clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY
        ctl->parse(program, [data, cb](clingo_ast const &ast) {
            auto ret = cb(&ast, data);
            if (ret != 0) { throw ClingoError(ret); }
        });
    GRINGO_CLINGO_CATCH
}

extern "C" clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data) {
    GRINGO_CLINGO_TRY
        ctl->add([data, cb](std::function<void (clingo_ast const &)> f) {
            auto ret = cb(data, [](clingo_ast_t const *ast, void *data) -> clingo_error_t {
                GRINGO_CLINGO_TRY
                    (*static_cast<std::function<void (clingo_ast const &)>*>(data))(static_cast<clingo_ast const &>(*ast));
                GRINGO_CLINGO_CATCH
            }, static_cast<void*>(&f));
            if (ret != 0) { throw ClingoError(ret); }
        });
    GRINGO_CLINGO_CATCH
}

// }}}1
