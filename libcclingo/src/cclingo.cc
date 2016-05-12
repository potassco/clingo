#include <cclingo.h>
#include <clingo/clingocontrol.hh>
#include <gringo/input/nongroundparser.hh>

using namespace Gringo;
using namespace Gringo::Input;

extern "C" void clingo_free(void *p) {
    ::operator delete[](p);
}

// {{{1 error

namespace {

struct ClingoError : std::exception {
    ClingoError(clingo_error_t err) : err(err) { }
    virtual ~ClingoError() noexcept = default;
    virtual const char* what() const noexcept {
        return clingo_error_str(err);
    }
    clingo_error_t const err;
};

}

extern "C" char const *clingo_error_str(clingo_error_t err) {
    switch (err) {
        case clingo_error_success:   { return nullptr; }
        case clingo_error_runtime:   { return "runtime error"; }
        case clingo_error_bad_alloc: { return "bad allocation"; }
        case clingo_error_logic:     { return "logic error"; }
        case clingo_error_unknown:   { return "unknown error"; }
    }
    return nullptr;
}

#define CCLINGO_TRY try {
#define CCLINGO_CATCH } \
catch(ClingoError &e)        { return e.err; } \
catch(std::bad_alloc &e)     { return clingo_error_bad_alloc; } \
catch(std::runtime_error &e) { std::cerr << "error message: " << e.what() << std::endl; return clingo_error_runtime; } \
catch(std::logic_error &e)   { std::cerr << "error message: " << e.what() << std::endl; return clingo_error_logic; } \
catch(...)                   { return clingo_error_unknown; } \
return clingo_error_success

// {{{1 value

namespace {

void init_value(Gringo::Value v, clingo_value_t *val) {
    Gringo::Value::POD p = v;
    val->a = p.type;
    val->b = p.value;
}

} // namespace

Gringo::Value toVal(clingo_value_t a) {
    return Gringo::Value::POD{a.a, a.b};
}

clingo_value_t toVal(Gringo::Value a) {
    Gringo::Value::POD v = a;
    return {v.type, v.value};
}

extern "C" void clingo_value_new_num(int num, clingo_value_t *val) {
    init_value(Gringo::Value::createNum(num), val);
}

extern "C" void clingo_value_new_sup(clingo_value_t *val) {
    init_value(Gringo::Value::createSup(), val);
}

extern "C" void clingo_value_new_inf(clingo_value *val) {
    init_value(Gringo::Value::createInf(), val);
}

extern "C" clingo_error_t clingo_value_new_str(char const *str, clingo_value *val) {
    CCLINGO_TRY
        init_value(Gringo::Value::createStr(str), val);
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_value_new_id(char const *id, bool_t sign, clingo_value *val) {
    CCLINGO_TRY
        init_value(Gringo::Value::createId(id, sign), val);
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_value_new_fun(char const *name, clingo_value_span_t args, bool_t sign, clingo_value *val) {
    CCLINGO_TRY
        Gringo::ValVec vals;
        vals.reserve(args.size);
        for (auto it = args.data, ie = it + args.size; it != ie; ++it) {
            vals.emplace_back(Gringo::Value::POD{it->a, it->b});
        }
        init_value(Gringo::Value::createFun(name, vals, sign), val);
    CCLINGO_CATCH;
}

extern "C" int clingo_value_num(clingo_value_t val) {
    return toVal(val).num();
}

extern "C" char const *clingo_value_name(clingo_value_t val) {
    return toVal(val).name()->c_str();
}

extern "C" char const *clingo_value_str(clingo_value_t val) {
    return toVal(val).string()->c_str();
}

extern "C" bool_t clingo_value_sign(clingo_value_t val) {
    return toVal(val).sign();
}

extern "C" clingo_value_span_t clingo_value_args(clingo_value_t val) {
    auto args = toVal(val).args();
    return {&reinterpret_cast<clingo_value_t const&>(*args.begin()), args.size()};
}

extern "C" clingo_value_type_t clingo_value_type(clingo_value_t val) {
    return static_cast<clingo_value_type_t>(toVal(val).type());
}

extern "C" clingo_error_t clingo_value_to_string(clingo_value_t val, char **str) {
    CCLINGO_TRY
        std::ostringstream oss;
        toVal(val).print(oss);
        std::string s = oss.str();
        *str = reinterpret_cast<char *>(::operator new[](sizeof(char) * (s.size() + 1)));
        std::strcpy(*str, s.c_str());
    CCLINGO_CATCH;
}

extern "C" bool_t clingo_value_eq(clingo_value_t a, clingo_value_t b) {
    return toVal(a) == toVal(b);
}

extern "C" bool_t clingo_value_lt(clingo_value_t a, clingo_value_t b) {
    return toVal(a) < toVal(b);
}

// {{{1 module

struct clingo_module : DefaultGringoModule { };

extern "C" clingo_error_t clingo_module_new(clingo_module_t **mod) {
    CCLINGO_TRY
        *mod = new clingo_module();
    CCLINGO_CATCH;
}

extern "C" void clingo_module_free(clingo_module_t *mod) {
    delete mod;
}

// {{{1 model

struct clingo_model : Gringo::Model { };

extern "C" bool_t clingo_model_contains(clingo_model_t *m, clingo_value_t atom) {
    return m->contains(toVal(atom));
}

extern "C" clingo_error_t clingo_model_atoms(clingo_model_t *m, clingo_show_type_t show, clingo_value_span_t *ret) {
    CCLINGO_TRY
        Gringo::ValVec const &atoms = m->atoms(show);
        ret->data = reinterpret_cast<clingo_value_t const*>(atoms.data());
        ret->size = atoms.size();
    CCLINGO_CATCH;
}

// {{{1 solve iter

struct clingo_solve_iter : Gringo::SolveIter { };

extern "C" clingo_error_t clingo_solve_iter_next(clingo_solve_iter_t *it, clingo_model **m) {
    CCLINGO_TRY
        *m = reinterpret_cast<clingo_model*>(const_cast<Gringo::Model*>(it->next()));
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iter_get(clingo_solve_iter_t *it, clingo_solve_result_t *ret) {
    CCLINGO_TRY
        *ret = static_cast<int>(it->get());
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_solve_iter_close(clingo_solve_iter_t *it) {
    CCLINGO_TRY
        it->close();
    CCLINGO_CATCH;
}

// {{{1 statistics

// {{{1 configurations

// {{{1 domain

// {{{1 control

struct clingo_control : Gringo::Control { };

extern "C" clingo_error_t clingo_control_new(clingo_module_t *mod, int argc, char const **argv, clingo_control_t **ctl) {
    CCLINGO_TRY
        *ctl = static_cast<clingo_control_t*>(mod->newControl(argc, argv));
    CCLINGO_CATCH;
}

extern "C" void clingo_control_free(clingo_control_t *ctl) {
    delete ctl;
}

extern "C" clingo_error_t clingo_control_add(clingo_control_t *ctl, char const *name, char const **params, char const *part) {
    CCLINGO_TRY
        Gringo::FWStringVec p;
        for (char const **param = params; *param; ++param) { p.emplace_back(*param); }
        ctl->add(name, p, part);
    CCLINGO_CATCH;
}

namespace {

struct ClingoContext : Gringo::Context {
    ClingoContext(clingo_ground_callback_t *cb, void *data)
    : cb(cb)
    , data(data) {}

    bool callable(Gringo::FWString) const override {
        return cb;
    }

    Gringo::ValVec call(Gringo::Location const &, Gringo::FWString name, Gringo::ValVec const &args) override {
        assert(cb);
        clingo_value_span_t args_c;
        args_c.size = args.size();
        args_c.data = reinterpret_cast<clingo_value_t const*>(args.data());
        clingo_value_span_t ret_c;
        auto err = cb(name->c_str(), args_c, data, &ret_c);
        if (err != 0) { throw ClingoError(err); }
        Gringo::ValVec ret;
        for (auto it = ret_c.data, ie = it + ret_c.size; it != ie; ++it) {
            ret.emplace_back(toVal(*it));
        }
        return ret;
    }
    virtual ~ClingoContext() noexcept = default;

    clingo_ground_callback_t *cb;
    void *data;
};

}

extern "C" clingo_error_t clingo_control_ground(clingo_control_t *ctl, clingo_part_span_t vec, clingo_ground_callback_t *cb, void *data) {
    CCLINGO_TRY
        Gringo::Control::GroundVec gv;
        gv.reserve(vec.size);
        for (auto it = vec.data, ie = it + vec.size; it != ie; ++it) {
            Gringo::ValVec params;
            params.reserve(it->params.size);
            for (auto jt = it->params.data, je = jt + it->params.size; jt != je; ++jt) {
                params.emplace_back(Gringo::Value::POD{jt->a, jt->b});
            }
            gv.emplace_back(it->name, params);
        }
        ClingoContext cctx(cb, data);
        ctl->ground(gv, cb ? &cctx : nullptr);
    CCLINGO_CATCH;
}

Gringo::Control::Assumptions toAss(clingo_symbolic_literal_span_t *assumptions) {
    Gringo::Control::Assumptions ass;
    if (assumptions) {
        for (auto it = assumptions->data, ie = it + assumptions->size; it != ie; ++it) {
            ass.emplace_back(toVal(it->atom), !it->sign);
        }
    }
    return ass;
}

extern "C" clingo_error_t clingo_control_solve(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_model_handler_t *model_handler, void *data, clingo_solve_result_t *ret) {
    CCLINGO_TRY
        *ret = static_cast<clingo_solve_result_t>(ctl->solve([model_handler, data](Gringo::Model const &m) {
            return model_handler(static_cast<clingo_model*>(const_cast<Gringo::Model*>(&m)), data);
        }, toAss(assumptions)));
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_solve_iter(clingo_control_t *ctl, clingo_symbolic_literal_span_t *assumptions, clingo_solve_iter_t **it) {
    CCLINGO_TRY
        *it = reinterpret_cast<clingo_solve_iter_t*>(ctl->solveIter(toAss(assumptions)));
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_assign_external(clingo_control_t *ctl, clingo_value_t atom, clingo_truth_value_t value) {
    CCLINGO_TRY
        ctl->assignExternal(toVal(atom), static_cast<Potassco::Value_t>(value));
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_release_external(clingo_control_t *ctl, clingo_value_t atom) {
    CCLINGO_TRY
        ctl->assignExternal(toVal(atom), Potassco::Value_t::Release);
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_parse(clingo_control_t *ctl, char const *program, clingo_parse_callback_t *cb, void *data) {
    CCLINGO_TRY
        ctl->parse(program, [data, cb](AST const &ast) {
            auto ret = cb(reinterpret_cast<clingo_ast_t const *>(&ast), data);
            if (ret != 0) { throw ClingoError(ret); }
        });
    CCLINGO_CATCH;
}

extern "C" clingo_error_t clingo_control_add_ast(clingo_control_t *ctl, clingo_add_ast_callback_t *cb, void *data) {
    CCLINGO_TRY
        ctl->add([data, cb]() {
            clingo_ast_t const *ast;
            auto ret = cb(&ast, data);
            if (ret != 0) { throw ClingoError(ret); }
            return reinterpret_cast<AST const *>(ast);
        });
    CCLINGO_CATCH;
}

// }}}1
