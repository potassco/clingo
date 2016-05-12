// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

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

#ifdef WITH_PYTHON

#include <Python.h>

#include "gringo/python.hh"
#include "gringo/version.hh"
#include "gringo/value.hh"
#include "gringo/locatable.hh"
#include "gringo/logger.hh"
#include "gringo/control.hh"
#include <iostream>
#include <sstream>

#if PY_MAJOR_VERSION >= 3
#define PyString_FromString PyUnicode_FromString
#if PY_MINOR_VERSION >= 3
#define PyString_AsString PyUnicode_AsUTF8
#else
#define PyString_AsString _PyUnicode_AsString
#endif
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_FromFormat PyUnicode_FromFormat
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#define PyInt_Check PyLong_Check
#define PyString_Check PyUnicode_Check
#define OBBASE(x) (&(x)->ob_base)
#else
#define OBBASE(x) x

#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

namespace Gringo {

namespace {

// {{{ auxiliary functions and objects

struct Object {
    Object() : obj(nullptr)                               { }
    Object(PyObject *obj, bool inc = false) : obj(obj)    { if (inc) { Py_XINCREF(obj); } }
    Object(Object const &other) : Object(other.obj, true) { }
    Object(Object &&other) : Object(other.obj, false)     { other.obj = nullptr; }
    bool none() const                                     { return obj == Py_None; } 
    bool valid() const                                    { return obj; }
    PyObject *get() const                                 { return obj; }
    PyObject *release()                                   { PyObject *ret = obj; obj = nullptr; return ret; }
    PyObject *operator->() const                          { return get(); }
    operator bool() const                                 { return valid(); }
    operator PyObject*() const                            { return get(); }
    Object &operator=(Object const &other)                { Py_XDECREF(obj); obj = other.obj; Py_XINCREF(obj); return *this; }
    ~Object()                                             { Py_XDECREF(obj); }
    PyObject *obj;
};

struct PyUnblock {
    PyUnblock() : state(PyEval_SaveThread()) { }
    ~PyUnblock() { PyEval_RestoreThread(state); }
    PyThreadState *state;
};

struct PyBlock {
    PyBlock() : state(PyGILState_Ensure()) { }
    ~PyBlock() { PyGILState_Release(state); }
    PyGILState_STATE state;
};

Object pyExec(char const *str, char const *filename, PyObject *globals, PyObject *locals = Py_None) {
    if (locals == Py_None) { locals = globals; }
    Object x = Py_CompileString(str, filename, Py_file_input);
    if (!x) { return nullptr; }
#if PY_MAJOR_VERSION >= 3
    return PyEval_EvalCode(x.get(), globals, locals);
#else
    return PyEval_EvalCode((PyCodeObject*)x.get(), globals, locals);
#endif
}

bool pyToVal(Object obj, Value &val);
bool pyToVal(PyObject *obj, Value &val) { return pyToVal({obj, true}, val); }
bool pyToVals(Object obj, ValVec &vals);
bool pyToVals(PyObject *obj, ValVec &vals) { return pyToVals({obj, true}, vals); }

PyObject *valToPy(Value v);
template <class T>
PyObject *valsToPy(T const & vals);

template <typename T>
bool protect(T f) {
    try                             { f(); return true; }
    catch (std::bad_alloc const &e) { PyErr_SetString(PyExc_MemoryError, e.what()); }
    catch (std::exception const &e) { PyErr_SetString(PyExc_RuntimeError, e.what()); }
    catch (...)                     { PyErr_SetString(PyExc_RuntimeError, "unknown error"); }
    return false;
}
#define CHECK_CMP(a, b, op) \
    if ((a)->ob_type != (b)->ob_type) { \
        if ((a)->ob_type != (b)->ob_type) { \
            Py_RETURN_FALSE; \
        } \
        else if ((op) == Py_NE && (a)->ob_type != (b)->ob_type) { \
            Py_RETURN_TRUE; \
        } \
    } \
    if (!checkCmp((a), (b), (op))) { return nullptr; }
template <class T>
bool checkCmp(T *self, PyObject *b, int op) {
    if (b->ob_type == self->ob_type) { return true; }
    else {
        const char *ops = "<";
        switch (op) {
            case Py_LT: { ops = "<";  break; }
            case Py_LE: { ops = "<="; break; }
            case Py_EQ: { ops = "=="; break; }
            case Py_NE: { ops = "!="; break; }
            case Py_GT: { ops = ">";  break; }
            case Py_GE: { ops = ">="; break; }
        }
        PyErr_Format(PyExc_TypeError, "unorderable types: %s() %s %s()", self->ob_type->tp_name, ops, b->ob_type->tp_name);
        return false;
    }
}

template <class T>
PyObject *doCmp(T const &a, T const &b, int op) {
    switch (op) {
        case Py_LT: { if (a <  b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
        case Py_LE: { if (a <= b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
        case Py_EQ: { if (a == b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
        case Py_NE: { if (a != b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
        case Py_GT: { if (a >  b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
        case Py_GE: { if (a >= b) { Py_RETURN_TRUE; } else { Py_RETURN_FALSE; } }
    }
    Py_RETURN_FALSE;
}

std::string errorToString() {
    Object type, value, traceback;
    PyErr_Fetch(&type.obj, &value.obj, &traceback.obj);
    if (!type)        { PyErr_Clear(); return "  error during error handling"; }
    PyErr_NormalizeException(&type.obj, &value.obj, &traceback.obj);
    Object tbModule  = PyImport_ImportModule("traceback");
    if (!tbModule)    { PyErr_Clear(); return "  error during error handling"; }
    PyObject *tbDict = PyModule_GetDict(tbModule);
    if (!tbDict)      { PyErr_Clear(); return "  error during error handling"; }
    PyObject *tbFE   = PyDict_GetItemString(tbDict, "format_exception");
    if (!tbFE)        { PyErr_Clear(); return "  error during error handling"; }
    Object ret       = PyObject_CallFunctionObjArgs(tbFE, type.get(), value ? value.get() : Py_None, traceback ? traceback.get() : Py_None, nullptr);
    if (!ret)         { PyErr_Clear(); return "  error during error handling"; }
    Object it        = PyObject_GetIter(ret);
    if (!it)          { PyErr_Clear(); return "  error during error handling"; }
    std::ostringstream oss;
    while (Object line = PyIter_Next(it)) {
        char const *msg = PyString_AsString(line);
        if (!msg) { break; }
        oss << "  " << msg;
    }
    if (PyErr_Occurred()) { PyErr_Clear(); return "  error during error handling"; }
    PyErr_Clear();
    return oss.str();
}
void handleError(Location const &loc, char const *msg) {
    std::string s = errorToString();
    GRINGO_REPORT(E_ERROR)
        << loc << ": error: " << msg << ":\n"
        << s
        ;
    throw std::runtime_error("grounding stopped because of errors");
}


// }}}
// {{{ wrap Fun

struct Fun {
    PyObject_HEAD
    Value val;
    static PyTypeObject type;
    static PyMethodDef methods[];

    static PyObject *new_(PyTypeObject *type, PyObject *, PyObject *) {
        Fun *self;
        self = reinterpret_cast<Fun*>(type->tp_alloc(type, 0));
        if (!self) { return nullptr; }
        self->val = Value();
        return reinterpret_cast<PyObject*>(self);
    }

    static int init(Fun *self, PyObject *args, PyObject *kwds) {
        static char const *kwlist[] = {"name", "args", nullptr};
        char const *name;
        PyObject   *params = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|O", const_cast<char**>(kwlist), &name, &params)) { return -1; }
        if (strcmp(name, "") == 0) {
            PyErr_SetString(PyExc_RuntimeError, "The name of a Fun object must not be empty");
            return -1;
        }
        if (params) {
            ValVec vals;
            if (!pyToVals(params, vals)) { return -1; }
            if (!protect([name, &vals, self]() { self->val = vals.empty() ? Value::createId(name) : Value::createFun(name, vals); })) { return -1; }
        }
        else {
            if (!protect([name, self]() { self->val = Value::createId(name); })) { return -1; }
        }
        return 0;
    }

    static PyObject *name(Fun *self) {
        // Note: should not throw
        return PyString_FromString((*FWString(self->val.name())).c_str());
    }

    static PyObject *args(Fun *self) {
        if (self->val.type() == Value::FUNC) {
            // Note: should not throw
            return valsToPy(self->val.args());
        }
        else {
            ValVec vals;
            return valsToPy(vals);
        }
    }

    static PyObject *str(Fun *self) {
        std::string s;
        protect([self, &s]() -> void { std::ostringstream oss; oss << self->val; s = oss.str(); });
        return PyString_FromString(s.c_str());
    }

    static long hash(Fun *self) {
        return self->val.hash();
    }
    static PyObject *cmp(Fun *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        // Note: should not throw
        return doCmp(self->val, reinterpret_cast<Fun*>(b)->val, op);
    }
};

PyMethodDef Fun::methods[] = {
    {"name", (PyCFunction)Fun::name, METH_NOARGS, "name(self) -> string object\n\nReturn the name of the Fun object."},
    {"args", (PyCFunction)Fun::args, METH_NOARGS, "args(self) -> list object\n\nReturn the arguments of the Fun object."},
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject Fun::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.Fun",                             // tp_name
    sizeof(Fun),                              // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)str,                            // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    (hashfunc)hash,                           // tp_hash
    0,                                        // tp_call
    (reprfunc)str,                            // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Fun(name, args) -> Fun object

Represents a gringo function term.

This also includes symbolic terms, which have to be created by either omitting
the arguments or passing an empty sequence.

Arguments:
name -- string representing the name of the function symbol
        (must follow gringo's identifier syntax)
args -- optional sequence of terms representing the arguments of the function
        symbol (Default: [])

Fun objects are ordered like in gringo and their string representation
corresponds to their gringo representation.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    (richcmpfunc)cmp,                         // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    (initproc)init,                           // tp_init
    0,                                        // tp_alloc
    new_,                                     // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap SupType 

struct SupType {
    PyObject_HEAD
    static PyTypeObject type;

    static int init() {
        sup = type.tp_alloc(&type, 0);
        return sup ? 0 : -1;
    }

    static PyObject *str(SupType *, PyObject *, PyObject *) {
        return PyString_FromString("#sup");
    }

    static PyObject *cmp(SupType *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(0, 0, op);
    }

    static long hash(SupType *) {
        return Value::createSup().hash();
    }

    static PyMethodDef methods[];
    static PyObject *sup;
};

PyObject *SupType::sup = nullptr;

PyMethodDef SupType::methods[] = {
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject SupType::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.SupType",                             // tp_name
    sizeof(SupType),                              // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)str,                            // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    (hashfunc)hash,                           // tp_hash
    0,                                        // tp_call
    (reprfunc)str,                            // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(SupType object

Represents a gringo #sup term.

SupType objects cannot be created and there is just one instance: Sup. They are
ordered like in gringo and their string representation corresponds to their
gringo representation.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    (richcmpfunc)cmp,                         // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap InfType

struct InfType {
    PyObject_HEAD
    static PyTypeObject type;

    static int init() {
        inf = type.tp_alloc(&type, 0);
        return inf ? 0 : -1;
    }

    static PyObject *str(InfType *, PyObject *, PyObject *) {
        return PyString_FromString("#inf");
    }

    static PyObject *cmp(InfType *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(0, 0, op);
    }

    static long hash(InfType *) {
        return Value::createInf().hash();
    }

    static PyMethodDef methods[];
    static PyObject *inf;
};

PyObject *InfType::inf = nullptr;

PyMethodDef InfType::methods[] = {
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject InfType::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.InfType",                             // tp_name
    sizeof(InfType),                              // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)str,                            // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    (hashfunc)hash,                           // tp_hash
    0,                                        // tp_call
    (reprfunc)str,                            // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(InfType object

Represents a gringo #inf term.

InfType objects cannot be created and there is just one instance: Inf. They are
ordered like in gringo and their string representation corresponds to their
gringo representation.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    (richcmpfunc)cmp,                         // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap SolveResult

struct SolveResult {
    PyObject_HEAD
    Gringo::SolveResult ret;
    static PyTypeObject type;
    static PyMethodDef methods[];

    static PyObject *new_(Gringo::SolveResult ret) {
        SolveResult *self;
        self = reinterpret_cast<SolveResult*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->ret = ret;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *str(SolveResult *self) {
        switch (self->ret) {
            case Gringo::SolveResult::SAT:     { return PyString_FromString("SAT"); }
            case Gringo::SolveResult::UNSAT:   { return PyString_FromString("UNSAT"); }
            case Gringo::SolveResult::UNKNOWN: { return PyString_FromString("UNKNOWN"); }
        }
        return PyString_FromString("UNKNOWN");
    }

    static PyObject *get(Gringo::SolveResult ret) {
        PyObject *res = nullptr;
        switch (ret) {
            case Gringo::SolveResult::SAT:     { res = PyDict_GetItemString(type.tp_dict, "SAT"); break; }
            case Gringo::SolveResult::UNSAT:   { res = PyDict_GetItemString(type.tp_dict, "UNSAT"); break; }
            case Gringo::SolveResult::UNKNOWN: { res = PyDict_GetItemString(type.tp_dict, "UNKNOWN"); break; }
        }
        Py_XINCREF(res);
        return res;
    }

    static int addAttr() {
        Object sat(new_(Gringo::SolveResult::SAT));
        if (!sat) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "SAT", sat) < 0) { return -1; }
        Object unsat(new_(Gringo::SolveResult::UNSAT));
        if (!unsat) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "UNSAT", unsat) < 0) { return -1; }
        Object unknown(new_(Gringo::SolveResult::UNKNOWN));
        if (!unknown) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "UNKNOWN", unknown) < 0) { return -1; }
        return 0;
    }

    static long hash(SolveResult *self) {
        return static_cast<long>(self->ret);
    }

    static PyObject *cmp(SolveResult *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(self->ret, reinterpret_cast<SolveResult*>(b)->ret, op);
    }
};

PyMethodDef SolveResult::methods[] = {
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject SolveResult::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.SolveResult",                     // tp_name
    sizeof(SolveResult),                      // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)str,                            // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    (hashfunc)hash,                           // tp_hash
    0,                                        // tp_call
    (reprfunc)str,                            // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Captures the result of a solve call.

SolveResult objects cannot be constructed from python. Instead the
preconstructed objects SolveResult.SAT, SolveResult.UNSAT, and
SolveResult.UNKNOWN have to be used.

SolveResult.SAT     -- solve call during which at least one model has been found.
SolveResult.UNSAT   -- solve call during which no model has been found.
SolveResult.UNKNOWN -- an canceled solve call - e.g. by SolveFuture.cancel, 
                       or a signal)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    (richcmpfunc)cmp,                         // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap Statistics

PyObject *getStatistics(Statistics const *stats, char const *prefix) {
    Statistics::Quantity ret(0);
    if (!protect([stats, prefix, &ret]{ ret = stats->getStat(prefix); })) { return nullptr; }
    switch (ret.error()) {
        case Statistics::error_none: { 
            double val = ret;
            return val == (int)val ? PyLong_FromDouble(val) : PyFloat_FromDouble(val);
        }
        case Statistics::error_not_available: {
            return PyErr_Format(PyExc_RuntimeError, "error_not_available: %s", prefix);
        }
        case Statistics::error_unknown_quantity: { 
            return PyErr_Format(PyExc_RuntimeError, "error_unknown_quantity: %s", prefix);
        }
        case Statistics::error_ambiguous_quantity: {
            char const *keys;
            if (!protect([stats, prefix, &keys]{ keys = stats->getKeys(prefix); })) { return nullptr; }
            if (!keys) { return PyErr_Format(PyExc_RuntimeError, "error zero keys string: %s", prefix); }
            if (strcmp(keys, "__len") == 0) {
                int len;
                if (!protect([stats, prefix, &len]() -> void {
                    std::string lenPrefix;
                    lenPrefix += prefix;
                    lenPrefix += "__len";
                    len = (int)(double)stats->getStat(lenPrefix.c_str()); 
                })) { return nullptr; }
                Object list = PyList_New(len);
                if (!list) { return nullptr; }
                for (int i = 0; i < len; ++i) {
                    Object objPrefix = PyString_FromFormat("%s%d.", prefix, i);
                    if (!objPrefix) { return nullptr; }
                    char const *subPrefix = PyString_AsString(objPrefix);
                    if (!subPrefix) { return nullptr; }
                    Object subStats = getStatistics(stats, subPrefix);
                    if (!subStats) { return nullptr; }
                    if (PyList_SetItem(list, i, subStats.release()) < 0) { return nullptr; }
                }
                return list.release();
            }
            else {
                Object dict = PyDict_New();
                if (!dict) { return nullptr; }
                for (char const *it = keys; *it; it+= strlen(it) + 1) {
                    int len = strlen(it);
                    Object key = PyString_FromStringAndSize(it, len - (it[len-1] == '.'));
                    if (!key) { return nullptr; }
                    Object objPrefix = PyString_FromFormat("%s%s", prefix, it);
                    if (!objPrefix) { return nullptr; }
                    char const *subPrefix = PyString_AsString(objPrefix);
                    if (!subPrefix) { return nullptr; }
                    Object subStats = getStatistics(stats, subPrefix);
                    if (!subStats) { return nullptr; }
                    if (PyDict_SetItem(dict, key, subStats) < 0) { return nullptr; }
                }
                return dict.release();
            }
        }
    }
    return PyErr_Format(PyExc_RuntimeError, "error unhandled prefix: %s", prefix);
}

// }}}
// {{{ wrap SolveControl

struct SolveControl {
    PyObject_HEAD
    Gringo::Model const *model;
    static PyTypeObject type;
    static PyMethodDef methods[];

    static PyObject *new_(Gringo::Model const &model) {
        SolveControl *self;
        self = reinterpret_cast<SolveControl*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->model = &model;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *getClause(SolveControl *self, PyObject *pyLits, bool invert) {
        Object it = PyObject_GetIter(pyLits);
        if (!it) { return nullptr; }
        Gringo::Model::LitVec lits;
        while (Object pyPair = PyIter_Next(it)) {
            Object pyPairIt = PyObject_GetIter(pyPair);
            if (!pyPairIt) { return nullptr; }
            Object pyAtom = PyIter_Next(pyPairIt);
            if (!pyAtom) { return PyErr_Occurred() ? nullptr : PyErr_Format(PyExc_RuntimeError, "tuple of atom and boolean expected"); }
            Object pyBool = PyIter_Next(pyPairIt);
            if (!pyBool) { return PyErr_Occurred() ? nullptr : PyErr_Format(PyExc_RuntimeError, "tuple of atom and boolean expected"); }
            Value atom;
            if (!pyToVal(pyAtom, atom)) { return nullptr; }
            int truth = PyObject_IsTrue(pyBool);
            if (truth == -1) { return nullptr; }
            if (!protect([invert, atom, truth, &lits]() { lits.emplace_back(bool(truth) ^ invert, atom); })) { return nullptr; }
        }
        if (PyErr_Occurred()) { return nullptr; }
        if (!protect([self, &lits]() { self->model->addClause(lits); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *add_clause(SolveControl *self, PyObject *pyLits) {
        return getClause(self, pyLits, false);
    }
    static PyObject *add_nogood(SolveControl *self, PyObject *pyLits) {
        return getClause(self, pyLits, true);
    }

};

PyMethodDef SolveControl::methods[] = {
    // add_clause
    {"add_clause",          (PyCFunction)add_clause,          METH_O,
R"(add_clause(self, lits) -> None

Adds a clause to the solver during the search.

Arguments:
lits -- A list of literals represented as pairs of atoms and Booleans
        representing the clause.

Note that this function can only be called in the model callback (or while
iterating when using a SolveIter).)"},
    // add_nogood
    {"add_nogood",          (PyCFunction)add_nogood,          METH_O,
R"(add_nogood(self, lits) -> None

Equivalent to add_clause with the literals inverted.

Arguments:
lits -- A list of pairs of Booleans and atoms representing the nogood.)"},
    {nullptr, nullptr, 0, nullptr}

};

PyTypeObject SolveControl::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.SolveControl",                    // tp_name
    sizeof(SolveControl),                     // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Object that allows for controlling a running search.

Note that SolveControl objects cannot be constructed from python.  Instead
they are available as properties of Model objects.)", // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap Model

struct Model {
    PyObject_HEAD
    Gringo::Model const *model;
    static PyTypeObject type;
    static PyMethodDef methods[];
    static PyGetSetDef getset[];

    static PyObject *new_(Gringo::Model const &model) {
        Model *self;
        self = reinterpret_cast<Model*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->model = &model;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *contains(Model *self, PyObject *arg) {
        Value val;
        if(!pyToVal(arg, val)) { return nullptr; }
        bool ret;
        if (!protect([self, val, &ret]() { ret = self->model->contains(val); })) { return nullptr; }
        if (ret) { Py_RETURN_TRUE; } 
        else     { Py_RETURN_FALSE; }
    }
    static PyObject *atoms(Model *self, PyObject *args) {
        int atomset = Gringo::Model::SHOWN;
        if (!PyArg_ParseTuple(args, "|i", &atomset)) { return nullptr; }
        ValVec vals;
        if (!protect([self, &vals, atomset]() { vals = self->model->atoms(atomset); })) { return nullptr; }
        Object list = PyList_New(vals.size());
        if (!list) { return nullptr; }
        int i = 0;
        for (auto x : vals) { 
            Object val(valToPy(x));
            if (!val) { return nullptr; }
            if (PyList_SetItem(list, i, val.release()) < 0) { return nullptr; }
            ++i;
        }
        return list.release();
    }
    static PyObject *optimization(Model *self) {
        Int64Vec values(self->model->optimization());
        Object list = PyList_New(values.size());
        if (!list) { return nullptr; }
        int i = 0;
        for (auto x : values) {
            Object val = PyInt_FromLong(x);
            if (!val) { return nullptr; }
            if (PyList_SetItem(list, i, val.release()) < 0) { return nullptr; }
            ++i;
        }
        return list.release();
    }
    static PyObject *str(Model *self, PyObject *) {
        std::string s;
        if (!protect([self, &s]() -> void {
            auto printAtom = [](std::ostream &out, Value val) {
            if (val.type() == Value::FUNC && *val.sig() == Signature("$", 2)) { out << val.args().front() << "=" << val.args().back(); }
            else { out << val; }
            };
            std::ostringstream oss;
            print_comma(oss, self->model->atoms(Gringo::Model::SHOWN), " ", printAtom);
            s = oss.str();
        })) { return nullptr; }
        return PyString_FromString(s.c_str());
    }
    static int addAttr() {
        Object csp(PyInt_FromLong(Gringo::Model::CSP));
        if (!csp) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "CSP", csp) < 0) { return -1; }
        Object atoms(PyInt_FromLong(Gringo::Model::ATOMS));
        if (!atoms) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "ATOMS", atoms) < 0) { return -1; }
        Object terms(PyInt_FromLong(Gringo::Model::TERMS));
        if (!terms) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "TERMS", terms) < 0) { return -1; }
        Object shown(PyInt_FromLong(Gringo::Model::SHOWN));
        if (!shown) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "SHOWN", shown) < 0) { return -1; }
        Object comp(PyInt_FromLong(Gringo::Model::COMP));
        if (!comp) { return -1; }
        if (PyDict_SetItemString(type.tp_dict, "COMP", comp) < 0) { return -1; }
        return 0;
    }
    static PyObject *getContext(Model *self, void *) {
        return SolveControl::new_(*self->model);
    }
};

PyGetSetDef Model::getset[] = {
    {(char*)"context", (getter)getContext, nullptr, (char*)"SolveControl object that allows for controlling the running search.", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyMethodDef Model::methods[] = {
    {"atoms",    (PyCFunction)atoms,    METH_VARARGS, 
R"(atoms(self, atomset=SHOWN) -> list of terms

Return the list of atoms, terms, or CSP assignments in the model.

Argument atomset is a bitset to select what kind of atoms or terms are returned:
Model.ATOMS -- selects all atoms in the model (independent of #show statements)
Model.TERMS -- selects all terms displayed with #show statements in the model
Model.SHOWN -- selects all atoms and terms as outputted by clingo's default
               output format
Model.CSP   -- selects all csp assignments (independent of #show statements)
Model.COMP  -- return the complement of the answer set w.r.t. to the Herbrand
               base accumulated so far (does not affect csp assignments)

The string representation of a model object is similar to the output of models
by clingo using the default output.

Note that atoms are represented using Fun objects, and that CSP assignments are
represented using function symbols with name "$" where the first argument is
the name of the CSP variable and the second its value.)"},
    {"contains", (PyCFunction)contains, METH_O,       
R"(contains(self, a) -> Boolean

Returns true if atom a is contained in the model.

Atom a must be represented using a Fun term.)"},
    {"optimization", (PyCFunction)optimization, METH_NOARGS,
R"(optimization(self) -> [int]

Returns the list of optimization values of the model. This corresponds to
clasp's optimization output.)"},
    {nullptr, nullptr, 0, nullptr}

};

PyTypeObject Model::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.Model",                           // tp_name
    sizeof(Model),                            // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    (reprfunc)str,                            // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    (reprfunc)str,                            // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Provides access to a model during a solve call.

Note that model objects cannot be constructed from python.  Instead they are
passed as argument to a model callback (see Control.solve and 
Control.solve_async).  Furthermore, the lifetime of a model object is limited 
to the scope of the callback. They must not be stored for later use in other
places like - e.g., the main function.)",     // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    getset,                                   // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap SolveFuture

struct SolveFuture {
    PyObject_HEAD
    Gringo::SolveFuture *future;
    static PyTypeObject type;
    static PyMethodDef methods[];

    static PyObject *new_(Gringo::SolveFuture &future) {
        SolveFuture *self;
        self = reinterpret_cast<SolveFuture*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->future = &future;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *get(SolveFuture *self, PyObject *) {
        Gringo::SolveResult ret;
        if (!protect([self, &ret]() -> void { PyUnblock b; (void)b; ret = self->future->get(); })) { return nullptr; }
        return SolveResult::get(ret);
    }
    static PyObject *wait(SolveFuture *self, PyObject *args) {
        PyObject *timeout = nullptr;
        if (!PyArg_ParseTuple(args, "|O", &timeout)) { return nullptr; }
        if (!timeout) {
            if (!protect([self]() -> void { PyUnblock b; (void)b; self->future->wait(); })) { return nullptr; }
            Py_RETURN_NONE;
        }
        else {
            double time = PyFloat_AsDouble(timeout);
            if (PyErr_Occurred()) { return nullptr; }
            bool ret;
            if (!protect([self, time, &ret]() { PyUnblock b; (void)b; ret = self->future->wait(time); })) { return nullptr; }
            if (ret) { Py_RETURN_TRUE; } 
            else     { Py_RETURN_FALSE; }
        }
    }
    static PyObject *cancel(SolveFuture *self, PyObject *) {
        if (!protect([self]() { PyUnblock b; (void)b; self->future->cancel(); })) { return nullptr; }
        Py_RETURN_NONE;
    }
};

PyMethodDef SolveFuture::methods[] = {
    {"get",       (PyCFunction)get,       METH_NOARGS,  
R"(get(self) -> SolveResult object

Get the result of an solve_async call. If the search is not completed yet, the
function blocks until the result is ready.)"},
    {"wait",      (PyCFunction)wait,      METH_VARARGS, 
R"(wait(self, timeout) -> None or Boolean

Wait for solve_async call to finish. If a timeout is given, the function waits
at most timeout seconds and returns a Boolean indicating whether the search has
finished. Otherwise, the function blocks until the search is finished and
returns nothing.

Arguments:
timeout -- optional timeout in seconds 
           (permits floating point values))"},
    {"cancel", (PyCFunction)cancel, METH_NOARGS,  
R"(cancel(self) -> None
    
Interrupts the running search.

Note that unlike other functions of this class, this function can safely be
called from other threads.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject SolveFuture::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.SolveFuture",                     // tp_name
    sizeof(SolveFuture),                      // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Handle for asynchronous solve calls.

SolveFuture objects cannot be created from python. Instead they are returned by
Control.solve_async, which performs a search in the background.  A SolveFuture
object can be used to wait for such a background search or cancel it.

Functions in this object release the GIL. They are not thread-safe though.

See Control.solve_async for an example.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap SolveIter

struct SolveIter {
    PyObject_HEAD
    Gringo::SolveIter *solve_iter;
    static PyTypeObject type;
    static PyMethodDef methods[];

    static PyObject *new_(Gringo::SolveIter &iter) {
        SolveIter *self;
        self = reinterpret_cast<SolveIter*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->solve_iter = &iter;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject* iter(PyObject *self) {
        Py_INCREF(self);
        return self;
    }
    static PyObject* get(SolveIter *self) {
        Gringo::SolveResult ret;
        if (!protect([self, &ret]() -> void { PyUnblock b; (void)b; ret = self->solve_iter->get(); })) { return nullptr; }
        return SolveResult::get(ret);
    }
    static PyObject* iternext(SolveIter *self) {
        Gringo::Model const *m;
        if (!protect([self, &m]() { PyUnblock b; m = self->solve_iter->next(); } )) { return nullptr; }
        if (m) {
            return Model::new_(*m);
        } else {
            PyErr_SetNone(PyExc_StopIteration);
            return nullptr;
        }
    }
    static PyObject *enter(SolveIter *self) {
        Py_INCREF(self);
        return (PyObject*)self;
    }
    static PyObject *exit(SolveIter *self, PyObject *) {
        if (!protect([self]() { PyUnblock b; self->solve_iter->close(); } )) { return nullptr; }
        Py_RETURN_FALSE;
    }
};

PyMethodDef SolveIter::methods[] = {
    {"__enter__",      (PyCFunction)enter,      METH_NOARGS,  
R"(__exit__(self) -> SolveIter

Returns self.)"},
    {"get",            (PyCFunction)get,        METH_NOARGS,  
R"(get(self) -> SolveResult

Returns the result of the search. Note that this might start a search for the
next model and then returns a result accordingly. The function might be called
after iteration to check if the search has been canceled.)"},
    {"__exit__",       (PyCFunction)exit,       METH_VARARGS,  
R"(__exit__(self, type, value, traceback) -> Boolean

Follows python __exit__ conventions. Does not suppress exceptions.

Stops the current search. It is necessary to call this method after each search.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject SolveIter::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.SolveIter",                       // tp_name
    sizeof(SolveIter),                        // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Object to conveniently iterate over all models.

During solving the GIL is released. The functions in this object are not
thread-safe though.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    SolveIter::iter,                          // tp_iter
    (iternextfunc)SolveIter::iternext,        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap ConfigProxy

struct ConfigProxy {
    PyObject_HEAD
    unsigned key;
    int nSubkeys;
    int arrLen;
    int nValues;
    char const* help;
    Gringo::ConfigProxy *proxy;
    static PyTypeObject type;
    static PyMethodDef methods[];
    static PySequenceMethods as_sequence;

    static PyObject *new_(unsigned key, Gringo::ConfigProxy &proxy) {
        Object ret(type.tp_alloc(&type, 0));
        if (!ret) { return nullptr; }
        ConfigProxy *self = reinterpret_cast<ConfigProxy*>(ret.get());
        self->proxy = &proxy;
        self->key   = key;
        if (!protect([self] { self->proxy->getKeyInfo(self->key, &self->nSubkeys, &self->arrLen, &self->help, &self->nValues); })) { return nullptr; }
        return ret.release();
    }
    static PyObject *keys(ConfigProxy *self) {
        if (self->nSubkeys < 0) { Py_RETURN_NONE; }
        else {
            Object list = PyList_New(self->nSubkeys);
            if (!list) { return nullptr; }
            for (int i = 0; i < self->nSubkeys; ++i) {
                char const *key;
                if (!protect([self, i, &key] { key = self->proxy->getSubKeyName(self->key, i); })) { return nullptr; }
                Object pyString = PyString_FromString(key);
                if (!pyString) { return nullptr; }
                if (PyList_SetItem(list, i, pyString.release()) < 0) { return nullptr; }
            }
            return list.release();
        }
    }
    static PyObject *getattro(ConfigProxy *self, PyObject *name) {
        char const *current = PyString_AsString(name);
        if (!current) { return nullptr; }
        bool desc = strncmp("__desc_", current, 7) == 0;
        if (desc) { current += 7; }
        unsigned key;
        bool hasSubKey;
        if (!protect([self, current, &hasSubKey, &key] { hasSubKey = self->proxy->hasSubKey(self->key, current, &key); })) { return nullptr; }
        if (hasSubKey) {
            Object subKey(new_(key, *self->proxy));
            if (!subKey) { return nullptr; }
            ConfigProxy *sub = reinterpret_cast<ConfigProxy*>(subKey.get());
            if (desc) { return PyString_FromString(sub->help); }
            else if (sub->nValues < 0) { return subKey.release(); }
            else {
                bool ret;
                std::string value;
                if (!protect([sub, &value, &ret]() { ret = sub->proxy->getKeyValue(sub->key, value); })) { return nullptr; }
                if (!ret) { Py_RETURN_NONE; }
                return PyString_FromString(value.c_str());
            }
        }
        return PyObject_GenericGetAttr(reinterpret_cast<PyObject*>(self), name);
    }
    static int setattro(ConfigProxy *self, PyObject *name, PyObject *pyValue) {
        char const *current = PyString_AsString(name);
        if (!current) { return -1; }
        unsigned key;
        bool hasSubKey;
        if (!protect([self, current, &hasSubKey, &key] { hasSubKey = self->proxy->hasSubKey(self->key, current, &key); })) { return -1; }
        if (hasSubKey) {
            Object pyStr(PyObject_Str(pyValue));
            if (!pyStr) { return -1; }
            char const *value = PyString_AsString(pyStr);
            if (!value) { return -1; }
            if (!protect([self, key, value]() { self->proxy->setKeyValue(key, value); })) { return -1; }
            return 0;
        }
        return PyObject_GenericSetAttr(reinterpret_cast<PyObject*>(self), name, pyValue);
    }
    static Py_ssize_t length(ConfigProxy *self) { 
        return self->arrLen;
    }
    static PyObject* item(ConfigProxy *self, Py_ssize_t index) {
        unsigned key;
        if (!protect([self, index, &key]() { key = self->proxy->getArrKey(self->key, index); })) { return nullptr; }
        return new_(key, *self->proxy);
    }
};

PyMethodDef ConfigProxy::methods[] = {
    // ground
    {"keys",         (PyCFunction)keys,               METH_NOARGS,                  
R"(keys(self) -> [string]

Returns the list of sub-option groups or options of this proxy. Returns None if
the proxy is not an option group.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PySequenceMethods ConfigProxy::as_sequence = {
    (lenfunc)length, 
    nullptr,
    nullptr,
    (ssizeargfunc)item,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

PyTypeObject ConfigProxy::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.ConfigProxy",                     // tp_name
    sizeof(ConfigProxy),                      // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    &as_sequence,                             // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    (getattrofunc)getattro,                   // tp_getattro
    (setattrofunc)setattro,                   // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Proxy object that allows for changing the configuration of the
underlying solver.

Options are organized hierarchically. To change and inspect an option use:

  proxy.group.subgroup.option = "value"
  value = proxy.group.subgroup.option

There are also arrays of option groups that can be accessed using integer
indices:

  proxy.group.subgroup[0].option = "value1"
  proxy.group.subgroup[1].option = "value2"

To list the subgroups of an option group, use the keys() method. Array option
groups, like solver, have have a non-negative length and can be iterated.
Furthermore, there are meta options having key "configuration". Assigning a
meta option sets a number of related options.  To get further information about
an option or option group <opt>, use property __desc_<opt> to retrieve a
description.

Example:

#script (python)
import gringo

def main(prg):
    prg.conf.solve.models = 0
    prg.ground([("base", [])])
    prg.solve()

#end.

{a; c}.

Expected Answer Sets:

{ {}, {a}, {c}, {a,c} } 
)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap DomainElement

struct DomainElement {
    PyObject_HEAD
    Gringo::DomainProxy::Element *elem;
    static PyTypeObject type;
    static PyGetSetDef getset[];
    static PyMethodDef methods[];
    static PyObject *new_(Gringo::DomainProxy::ElementPtr &&elem) {
        Object ret(type.tp_alloc(&type, 0));
        if (!ret) { return nullptr; }
        DomainElement *self = reinterpret_cast<DomainElement*>(ret.get());
        self->elem = elem.release();
        return ret.release();
    }
    static PyObject *atom(DomainElement *self, void *) {
        Value atom;
        if (!protect([self, &atom](){ atom = self->elem->atom(); })) { return nullptr; }
        return valToPy(atom);
    }
    static PyObject *is_fact(DomainElement *self, void *) {
        bool ret;
        if (!protect([self, &ret](){ ret = self->elem->fact(); })) { return nullptr; }
        if (ret) { Py_RETURN_TRUE; }
        else     { Py_RETURN_FALSE; }
    }
    static PyObject *is_external(DomainElement *self, void *) {
        bool ret;
        if (!protect([self, &ret](){ ret = self->elem->external(); })) { return nullptr; }
        if (ret) { Py_RETURN_TRUE; }
        else     { Py_RETURN_FALSE; }
    }
    static void free(DomainElement *self) {
        delete self->elem;
        self->elem = nullptr;
    }
};

PyGetSetDef DomainElement::getset[] = {
    {(char *)"atom",         (getter)atom,               nullptr,                  
(char *)R"(atom -> Fun

Returns the representation of the domain element in form of a term (Fun
object).
)", nullptr},
    {(char *)"is_fact",         (getter)is_fact,               nullptr,                  
(char *)R"(is_fact -> bool

Returns wheather the domain element is a is_fact.
)", nullptr},
    {(char *)"is_external",     (getter)is_external,           nullptr,                  
(char *)R"(is_external -> bool

Returns wheather the domain element is external.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr},
};

PyMethodDef DomainElement::methods[] = {
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject DomainElement::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.DomainElement",                   // tp_name
    sizeof(DomainElement),                    // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Captures a domain element and provides properties to inspect its state.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    getset,                                   // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    (freefunc)free,                           // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap DomainIter

struct DomainIter {
    PyObject_HEAD
    DomainElement *elem;
    static PyTypeObject type;
    static PyMethodDef methods[];
    static PyObject *new_(Gringo::DomainProxy::ElementPtr &&elem) {
        Object ret(type.tp_alloc(&type, 0));
        if (!ret) { return nullptr; }
        DomainIter *self = reinterpret_cast<DomainIter*>(ret.get());
        if (elem) {
            self->elem = reinterpret_cast<DomainElement*>(DomainElement::new_(std::move(elem)));
            if (!self->elem) { return nullptr; }
        }
        else { self->elem = nullptr; }
        return ret.release();
    }
    static PyObject* iter(DomainIter *self) {
        Py_XINCREF(self);
        return reinterpret_cast<PyObject*>(self);
    }
    static void free(DomainIter *self) {
        if (self->elem) {
            Py_XDECREF(self->elem);
            self->elem = nullptr;
        }
    }
    static PyObject* iternext(DomainIter *self) {
        if (self->elem) {
            Gringo::DomainProxy::ElementPtr elem;
            if (!protect([self, &elem]() { elem = self->elem->elem->next(); })) { return nullptr; }
            Object oldElem = reinterpret_cast<PyObject*>(self->elem);
            if (elem) {
                self->elem = reinterpret_cast<DomainElement*>(DomainElement::new_(std::move(elem)));
                if (!self->elem) { return nullptr; }
            }
            else { self->elem = nullptr; }
            return oldElem.release();
        }
        PyErr_SetNone(PyExc_StopIteration);
        return nullptr;
    }
};

PyMethodDef DomainIter::methods[] = {
    {nullptr, nullptr, 0, nullptr}
};

PyTypeObject DomainIter::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.DomainIter",                      // tp_name
    sizeof(DomainIter),                       // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(Class to iterate over domains.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    (getiterfunc)iter,                        // tp_iter
    (iternextfunc)iternext,                   // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    (freefunc)free,                           // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap DomainProxy

struct DomainProxy {
    PyObject_HEAD
    Gringo::DomainProxy *proxy;
    static PyTypeObject type;
    static PyMethodDef methods[];
    static PyMappingMethods as_mapping;

    static PyObject *new_(Gringo::DomainProxy &proxy) {
        Object ret(type.tp_alloc(&type, 0));
        if (!ret) { return nullptr; }
        DomainProxy *self = reinterpret_cast<DomainProxy*>(ret.get());
        self->proxy = &proxy;
        return ret.release();
    }

    static Py_ssize_t length(DomainProxy *self) {
        return self->proxy->length();
    }

    static PyObject* iter(DomainProxy *self) {
        Gringo::DomainProxy::ElementPtr elem;
        if (!protect([self, &elem]() { elem = self->proxy->iter(); })) { return nullptr; }
        return DomainIter::new_(std::move(elem));
    }

    static PyObject* subscript(DomainProxy *self, PyObject *key) {
        Gringo::Value atom;
        if (!pyToVal(key, atom)) { return nullptr; }
        Gringo::DomainProxy::ElementPtr elem;
        if (!protect([self, atom, &elem]() { elem = self->proxy->lookup(atom); })) { return nullptr; }
        if (!elem) { Py_RETURN_NONE; }
        return DomainElement::new_(std::move(elem));
    }

    static PyObject* by_signature(DomainProxy *self, PyObject *pyargs) {
        char const *name;
        int arity;
        if (!PyArg_ParseTuple(pyargs, "si", &name, &arity)) { return nullptr; }

        Gringo::DomainProxy::ElementPtr elem;
        if (!protect([self, name, arity, &elem]() { elem = self->proxy->iter(Signature(name, arity)); })) { return nullptr; }
        return DomainIter::new_(std::move(elem));
    }

    static PyObject* signatures(DomainProxy *self) {
        std::vector<FWSignature> ret;
        if (!protect([self, &ret]() { ret = self->proxy->signatures(); })) { return nullptr; }
        Object pyRet = PyList_New(ret.size());
        if (!pyRet) { return nullptr; }
        int i = 0;
        for (auto &sig : ret) {
            Object pySig = Py_BuildValue("(si)", (*(*sig).name()).c_str(), (int)(*sig).length());
            if (!pySig) { return nullptr; }
            if (PyList_SetItem(pyRet, i, pySig.release()) < 0) { return nullptr; }
            ++i;
        }
        return pyRet.release();
    }
};

PyMethodDef DomainProxy::methods[] = {
    {"by_signature", (PyCFunction)by_signature, METH_VARARGS,
R"(by_signature(self, name, arity) -> DomainIter

Returns an iterator over the domain elements with the given signature.

Arguments:
name  -- the name of the signature
arity -- the arity of the signature
)"},
    {"signatures", (PyCFunction)signatures, METH_NOARGS,
R"(signatures(self) -> list((str, int))

Returns the list of predicate signatures occurring in the program.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyMappingMethods DomainProxy::as_mapping = {
    (lenfunc)length,
    (binaryfunc)subscript,
    nullptr,
};

PyTypeObject DomainProxy::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.Domain",                          // tp_name
    sizeof(DomainProxy),                      // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    &as_mapping,                              // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
R"(This class provides read-only acces to the domains of the grounder.

Example:

p(1).
{ p(3) }. 
#external p(1..3).

q(X) :- p(X).

#script (python)

import gringo

def main(prg):
    prg.ground([("base", [])])
    print "universe:", len(prg.domains)
    for x in prg.domains:
        print x.atom, x.is_fact, x.is_external
    print "p(2) is in domain:", prg.domains[gringo.Fun("p", [3])] is not None
    print "p(4) is in domain:", prg.domains[gringo.Fun("p", [6])] is not None
    print "domain of p/1:"
    for x in prg.domains.by_signature(("p", 1)):
        print x.atom, x.is_fact, x.is_external
    print "signatures:", prg.domains.signatures()
    
#end. 

Expected Output:

universe: 6
p(1) True False
p(3) False False
p(2) False True
q(1) True False
q(3) False False
q(2) False False
p(2) is in domain: True
p(4) is in domain: False
domain of p/1:
p(1) True False
p(3) False False
p(2) False True
signatures: [('p', 1), ('q', 1)]
)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    (getiterfunc)iter,                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    0,                                        // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    0,                                        // tp_init
    0,                                        // tp_alloc
    0,                                        // tp_new
    0,                                        // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap Control

struct ControlWrap {
    PyObject_HEAD
    Gringo::Control *ctl;
    Gringo::Control *freeCtl;
    PyObject        *stats;

    static PyTypeObject type;
    static PyGetSetDef getset[];
    static PyMethodDef methods[];

    static bool checkBlocked(ControlWrap *self, char const *function) {
        if (self->ctl->blocked()) {
            PyErr_Format(PyExc_RuntimeError, "Control.%s must not be called during solve call", function);
            return false;
        }
        return true;
    }
    static PyObject *new_(Gringo::Control &ctl) {
        PyObject *self = new2_(&type, nullptr, nullptr);
        if (!self) { return nullptr; }
        reinterpret_cast<ControlWrap*>(self)->ctl = &ctl;
        return self;
    }
    static Gringo::GringoModule *module;
    static PyObject *new2_(PyTypeObject *type, PyObject *, PyObject *) {
        ControlWrap *self;
        self = reinterpret_cast<ControlWrap*>(type->tp_alloc(type, 0));
        if (!self) { return nullptr; }
        self->ctl     = nullptr;
        self->freeCtl = nullptr;
        self->stats   = nullptr;
        return reinterpret_cast<PyObject*>(self);
    }
    static void free(ControlWrap *self) {
        module->freeControl(self->freeCtl);
        self->ctl = self->freeCtl = nullptr;
        Py_XDECREF(self->stats);
    }
    static int init(ControlWrap *self, PyObject *pyargs, PyObject *pykwds) {
        static char const *kwlist[] = {"args", nullptr};
        PyObject *params = nullptr;
        if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "|O", const_cast<char**>(kwlist), &params)) { return -1; }
        std::vector<char const *> args;
        if (!protect([&args]() { args.emplace_back("clingo"); })) { return -1; }
        if (params) {
            Object it = PyObject_GetIter(params);
            if (!it) { return -1; }
            while (Object pyVal = PyIter_Next(it)) {
                char const *x = PyString_AsString(pyVal);
                if (!x) { return -1; }
                if (!protect([x, &args]() { args.emplace_back(x); })) { return -1; }
            }
            if (PyErr_Occurred()) { return -1; }
        }
        if (!protect([&args]() { args.emplace_back(nullptr); })) { return -1; }
        if (!protect([&args, self]() { self->ctl = self->freeCtl = module->newControl(args.size(), args.data()); })) { return -1; }
        return 0;
    }
    static PyObject *add(ControlWrap *self, PyObject *args) { 
        if (!checkBlocked(self, "add")) { return nullptr; }
        char     *name;
        PyObject *pyParams;
        char     *part;
        if (!PyArg_ParseTuple(args, "sOs", &name, &pyParams, &part)) { return nullptr; }
        FWStringVec params;
        Object it = PyObject_GetIter(pyParams);
        if (!it) { return nullptr; }
        while (Object pyVal = PyIter_Next(it)) {
            char const *val = PyString_AsString(pyVal);
            if (!val) { return nullptr; }
            if (!protect([val, &params]() { params.emplace_back(val); })) { return nullptr; }
        }
        if (PyErr_Occurred()) { return nullptr; }
        if (!protect([self, name, &params, part]() { self->ctl->add(name, params, part); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *load(ControlWrap *self, PyObject *args) { 
        if (!checkBlocked(self, "load")) { return nullptr; }
        char *filename;
        if (!PyArg_ParseTuple(args, "s", &filename)) { return nullptr; }
        if (!filename) { return nullptr; }
        if (!protect([self, filename]() { self->ctl->load(filename); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *ground(ControlWrap *self, PyObject *args, PyObject *kwds) { 
        if (!checkBlocked(self, "ground")) { return nullptr; }
        Gringo::Control::GroundVec parts;
        static char const *kwlist[] = {"parts", "context", nullptr};
        PyObject *pyParts, *context = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", const_cast<char**>(kwlist), &pyParts, &context)) { return nullptr; }
        Object it = PyObject_GetIter(pyParts);
        if (!it) { return nullptr; }
        while (Object pyVal = PyIter_Next(it)) {
            Object jt = PyObject_GetIter(pyVal);
            if (!jt) { return nullptr; }
            Object pyName = PyIter_Next(jt);
            if (!pyName) { return PyErr_Occurred() ? nullptr : PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
            Object pyArgs = PyIter_Next(jt);
            if (!pyArgs) { return PyErr_Occurred() ? nullptr : PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
            if (PyIter_Next(jt)) { return PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
            char *name = PyString_AsString(pyName);
            if (!name) { return nullptr; }
            ValVec args;
            if (!pyToVals(pyArgs, args)) { return nullptr; }
            if (!protect([self, name, &args, &parts]() { parts.emplace_back(name, args); })) { return nullptr; }
        }
        if (PyErr_Occurred()) { return nullptr; }
        if (!protect([self, &parts, context]() { self->ctl->ground(parts, context ? Any(context) : Any()); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *getConst(ControlWrap *self, PyObject *args) { 
        if (!checkBlocked(self, "get_const")) { return nullptr; }
        char *name;
        if (!PyArg_ParseTuple(args, "s", &name)) { return nullptr; }
        Value val;
        if (!protect([self, name, &val]() { val = self->ctl->getConst(name); })) { return nullptr; }
        if (val.type() == Value::SPECIAL) { Py_RETURN_NONE; }
        else { return valToPy(val); }
    }
    static bool on_model(Gringo::Model const &m, Object const &mh) {
        Object model(Model::new_(m));
        if (!model) {
            Location loc("<on_model>", 1, 1, "<on_model>", 1, 1);
            handleError(loc, "error in model callback");
            throw std::runtime_error("error in model callback");
        }
        Object ret = PyObject_CallFunction(mh, const_cast<char*>("O"), model.get());
        if (!ret) {
            Location loc("<on_model>", 1, 1, "<on_model>", 1, 1);
            handleError(loc, "error in model callback");
            throw std::runtime_error("error in model callback");
        }
        if (ret == Py_None)       { return true; }
        else if (ret == Py_True)  { return true; }
        else if (ret == Py_False) { return false; }
        else {
            PyErr_Format(PyExc_RuntimeError, "unexpected %s() object as result of on_model", ret->ob_type->tp_name);
            Location loc("<on_model>", 1, 1, "<on_model>", 1, 1);
            handleError(loc, "error in model callback");
            throw std::runtime_error("error in model callback");
        }

    }
    static void on_finish(Gringo::SolveResult ret, bool canceled, Object const &fh) {
        Object pyRet = SolveResult::get(ret);
        if (!pyRet) {
            Location loc("<on_finish>", 1, 1, "<on_finish>", 1, 1);
            handleError(loc, "error in finish callback");
            throw std::runtime_error("error in finish callback");
        }
        Object pyInterrupted = PyBool_FromLong(canceled);
        if (!pyInterrupted) {
            Location loc("<on_finish>", 1, 1, "<on_finish>", 1, 1);
            handleError(loc, "error in finish callback");
            throw std::runtime_error("error in finish callback");
        }
        Object fhRet = PyObject_CallFunction(fh, const_cast<char*>("OO"), pyRet.get(), pyInterrupted.get());
        if (!fhRet) {
            Location loc("<on_finish>", 1, 1, "<on_finish>", 1, 1);
            handleError(loc, "error in finish callback");
            throw std::runtime_error("error in finish callback");
        }
    }
    static bool getAssumptions(PyObject *pyAss, Gringo::Control::Assumptions &ass) {
        if (pyAss && pyAss != Py_None) {
            Object it = PyObject_GetIter(pyAss);
            if (!it) { return false; }
            while (Object pyPair = PyIter_Next(it)) {
                Object pyPairIt = PyObject_GetIter(pyPair);
                if (!pyPairIt) { return false; }
                Object pyAtom = PyIter_Next(pyPairIt);
                if (!pyAtom) { 
                    if (!PyErr_Occurred()) { PyErr_Format(PyExc_RuntimeError, "tuple expected"); }
                    return false;
                }
                Object pyBool = PyIter_Next(pyPairIt);
                if (!pyBool) { 
                    if (!PyErr_Occurred()) { PyErr_Format(PyExc_RuntimeError, "tuple expected"); }
                    return false;
                }
                Value atom;
                if (!pyToVal(pyAtom, atom)) { return false; }
                int truth = PyObject_IsTrue(pyBool);
                if (truth == -1) { return false; }
                if (!protect([atom, truth, &ass]() { ass.emplace_back(atom, truth); })) { return false; }
            }
            if (PyErr_Occurred()) { return false; }
        }
        return true;
    }
    static PyObject *solve_async(ControlWrap *self, PyObject *args, PyObject *kwds) {
        if (!checkBlocked(self, "solve_async")) { return nullptr; }
        Py_XDECREF(self->stats);
        self->stats = nullptr;
        static char const *kwlist[] = {"assumptions", "on_model", "on_finish", nullptr};
        PyObject *pyAss = nullptr, *mh = Py_None, *fh = Py_None;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", const_cast<char **>(kwlist), &pyAss, &mh, &fh)) { return nullptr; }
        Gringo::Control::Assumptions ass;
        if (!getAssumptions(pyAss, ass)) { return nullptr; }
        Gringo::SolveFuture *future;
        Object omh(mh, true);
        Object ofh(fh, true);
        if (!protect([self, &ass]() { self->ctl->prepareSolve(std::move(ass)); })) { return nullptr; }
        if (!protect([self, omh, ofh, &future]() {
            PyUnblock unblock;
            future = (self->ctl->solveAsync(
                omh == Py_None ? Control::ModelHandler(nullptr) : [omh](Gringo::Model const &m) -> bool { PyBlock b; (void)b; return on_model(m, omh); },
                ofh == Py_None ? Control::FinishHandler(nullptr) : [ofh](Gringo::SolveResult ret, bool canceled) -> void { PyBlock b; (void)b; on_finish(ret, canceled, ofh); }
            ));
        })) { return nullptr; }
        PyObject *ret = SolveFuture::new_(*future);
        return ret;
    }
    static PyObject *solve_iter(ControlWrap *self, PyObject *args, PyObject *kwds) {
        if (!checkBlocked(self, "solve_iter")) { return nullptr; }
        Py_XDECREF(self->stats);
        self->stats = nullptr;
        PyObject *pyAss = nullptr;
        static char const *kwlist[] = {"assumptions", nullptr};
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", const_cast<char **>(kwlist), &pyAss)) { return nullptr; }
        Gringo::Control::Assumptions ass;
        if (!getAssumptions(pyAss, ass)) { return nullptr; }
        if (!protect([self, &ass]() { self->ctl->prepareSolve(std::move(ass)); })) { return nullptr; }
        Gringo::SolveIter *iter;
        if (!protect([self, &iter]() {
            PyUnblock unblock;
            iter = self->ctl->solveIter();
        })) { return nullptr; }
        PyObject *ret = SolveIter::new_(*iter);
        return ret;
    }
    static PyObject *solve(ControlWrap *self, PyObject *args, PyObject *kwds) {
        if (!checkBlocked(self, "solve")) { return nullptr; }
        Py_XDECREF(self->stats);
        self->stats = nullptr;
        static char const *kwlist[] = {"assumptions", "on_model", nullptr};
        PyObject *mh = Py_None;
        PyObject *pyAss = nullptr;
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", const_cast<char **>(kwlist), &pyAss, &mh)) { return nullptr; }
        Gringo::SolveResult ret;
        Gringo::Control::Assumptions ass;
        if (!getAssumptions(pyAss, ass)) { return nullptr; }
        if (!protect([self, &ass]() { self->ctl->prepareSolve(std::move(ass)); })) { return nullptr; }
        if (!protect([self, mh, &ret]() {
            PyUnblock unblock;
            ret = (self->ctl->solve(
                mh == Py_None ? Control::ModelHandler(nullptr) : [mh](Gringo::Model const &m) { PyBlock block; return on_model(m, Object(mh, true)); }
            ));
        })) { return nullptr; }
        return SolveResult::get(ret);
    }
    static PyObject *cleanup_domains(ControlWrap *self) {
        if (!checkBlocked(self, "cleanup_domains")) { return nullptr; }
        if (!protect([self]() { self->ctl->cleanupDomains(); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *assign_external(ControlWrap *self, PyObject *args) {
        if (!checkBlocked(self, "assign_external")) { return nullptr; }
        PyObject *pyExt, *pyVal;
        if (!PyArg_ParseTuple(args, "OO", &pyExt, &pyVal)) { return nullptr; }
        TruthValue val;
        if (pyVal == Py_True)       { val = TruthValue::True; }
        else if (pyVal == Py_False) { val = TruthValue::False; }
        else if (pyVal == Py_None)  { val = TruthValue::Open; }
        else {
            PyErr_Format(PyExc_RuntimeError, "unexpected %s() object as second argumet", pyVal->ob_type->tp_name);
            return nullptr;
        }
        Value ext;
        if (!pyToVal(pyExt, ext)) { return nullptr; }
        if (!protect([self, ext, val]() { self->ctl->assignExternal(ext, val); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *release_external(ControlWrap *self, PyObject *args) {
        if (!checkBlocked(self, "release_external")) { return nullptr; }
        PyObject *pyExt;
        if (!PyArg_ParseTuple(args, "O", &pyExt)) { return nullptr; }
        Value ext;
        if (!pyToVal(pyExt, ext)) { return nullptr; }
        if (!protect([self, ext]() { self->ctl->assignExternal(ext, TruthValue::Free); })) { return nullptr; }
        Py_RETURN_NONE;
    }
    static PyObject *getStats(ControlWrap *self, void *) {
        if (!checkBlocked(self, "stats")) { return nullptr; }
        if (!self->stats) {
            Statistics *stats;
            if (!protect([self, &stats]() { stats = self->ctl->getStats(); })) { return nullptr; }
            self->stats = getStatistics(stats, "");
        }
        Py_XINCREF(self->stats);
        return self->stats;
    }
    static int set_use_enum_assumption(ControlWrap *self, PyObject *pyEnable, void *) {
        if (!checkBlocked(self, "use_enum_assumption")) { return -1; }
        int enable = PyObject_IsTrue(pyEnable);
        if (enable < 0) { return -1; }
        if (!protect([self, enable]() { self->ctl->useEnumAssumption(enable); })) { return -1; }
        return 0;
    }
    static PyObject *get_use_enum_assumption(ControlWrap *self, void *) {
        bool enable;
        if (!protect([self, &enable]() { enable = self->ctl->useEnumAssumption(); })) { return nullptr; }
        return PyBool_FromLong(enable);
    }
    static PyObject *conf(ControlWrap *self, void *) {
        Gringo::ConfigProxy *proxy;
        unsigned key;
        if (!protect([self, &key, &proxy]() -> void { 
            proxy = &self->ctl->getConf();
            key   = proxy->getRootKey();
        })) { return nullptr; }
        return ConfigProxy::new_(key, *proxy);
    }
    static PyObject *get_domains(ControlWrap *self, void *) {
        return DomainProxy::new_(self->ctl->getDomain());
    }
};

Gringo::GringoModule *ControlWrap::module  = nullptr;

PyMethodDef ControlWrap::methods[] = {
    // ground
    {"ground",         (PyCFunction)ground,         METH_KEYWORDS | METH_VARARGS,
R"(ground(self, parts, context) -> None

Grounds the given list programs specified by tuples of names and arguments.

Keyword Arguments:
parts   -- list of tuples of program names and program arguments to ground
context -- an context object whose methods are called during grounding using
           the @-syntax (if ommitted methods from the main module are used)

Note that parts of a logic program without an explicit #program specification
are by default put into a program called base without arguments. 

Example:

#script (python)
import gringo

def main(prg):
    parts = []
    parts.append(("p", [1]))
    parts.append(("p", [2]))
    prg.ground(parts)
    prg.solve()

#end.

#program p(t).
q(t).

Expected Answer Set:
q(1) q(2))"},
    // get_const
    {"get_const",       (PyCFunction)getConst,       METH_VARARGS,                  
R"(get_const(self, name) -> Fun, integer, string, or tuple object

Returns the term for a constant definition of form: #const name = term.)"},
    // add
    {"add",            (PyCFunction)add,            METH_VARARGS,                  
R"(add(self, name, params, program) -> None

Extend the logic program with the given non-ground logic program in string form.

Arguments:
name    -- name of program block to add
params  -- parameters of program block
program -- non-ground program as string

Example:

#script (python)
import gringo

def main(prg):
    prg.add("p", ["t"], "q(t).")
    prg.ground([("p", [2])])
    prg.solve()

#end.

Expected Answer Set:
q(2))"},
    // load
    {"load",            (PyCFunction)load,            METH_VARARGS,                  
R"(load(self, path) -> None

Extend the logic program with a (non-ground) logic program in a file.

Arguments:
path -- path to program)"},
    // solve_async
    {"solve_async",         (PyCFunction)solve_async,         METH_KEYWORDS | METH_VARARGS,  
R"(solve_async(self, assumptions, on_model, on_finish) -> SolveFuture

Start a search process in the background and return a SolveFuture object.

Keyword Arguments:
assumptions -- a list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(Fun("a"),
               True)] only admits answer sets that contain atom a
on_model    -- optional callback for intercepting models
               a Model object is passed to the callback
on_finish   -- optional callback once search has finished
               a SolveResult and a Boolean indicating whether the solve call 
               has been canceled is passed to the callback

Note that this function is only available in clingo with thread support
enabled. Both the on_model and the on_finish callbacks are called from another
thread.  To ensure that the methods can be called, make sure to not use any
functions that block the GIL indefinitely. Furthermore, you might want to start
clingo using the --outf=3 option to disable all output from clingo.

Example:

#script (python)
import gringo

def on_model(model):
    print model

def on_finish(res, canceled):
    print res, canceled

def main(prg):
    prg.ground([("base", [])])
    f = prg.solve_async(None, on_model, on_finish)
    f.wait()

#end.

q.

Expected Output:
q
SAT False)"},
    // solve_iter
    {"solve_iter",          (PyCFunction)solve_iter,          METH_KEYWORDS | METH_VARARGS,  
R"(solve_iter(self, assumptions) -> SolveIter

Returns a SolveIter object, which can be used to iterate over models.

Keyword Arguments:
assumptions -- a list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(Fun("a"),
               True)] only admits answer sets that contain atom a

Example:
 
#script (python)
import gringo
 
def main(prg):
    prg.add("p", "{a;b;c}.")
    prg.ground([("p", [])])
    with prg.solve_iter() as it:
        for m in it: print m
 
#end.)"},
    // solve
    {"solve",          (PyCFunction)solve,          METH_KEYWORDS | METH_VARARGS,  
R"(solve(self, assumptions, on_model) -> SolveResult

Starts a search process and returns a SolveResult.

Keyword Arguments:
assumptions -- a list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(Fun("a"),
               True)] only admits answer sets that contain atom a
on_model    -- optional callback for intercepting models
               a Model object is passed to the callback

Note that in gringo or in clingo with lparse or text output enabled this
functions just grounds and returns SolveResult.UNKNOWN. Furthermore, you might
want to start clingo using the --outf=3 option to disable all output from
clingo.

This function releases the GIL but it is not thread-safe.

Take a look at Control.solve_async for an example on how to use the model
callback.)"},
    // cleanup_domains
    {"cleanup_domains", (PyCFunction)cleanup_domains, METH_NOARGS,
R"(cleanup_domains(self) -> None

This functions cleans up the domains used for grounding.  This is done by first
simplifying the current program representation (falsifying released external
atoms).  Afterwards, the top-level implications are used to either remove atoms
from domains or mark them as facts.

Note that any atoms falsified are completely removed from the logic program.
Hence, a definition for such an atom in a successive step introduces a fresh atom.)"},
    // assign_external
    {"assign_external", (PyCFunction)assign_external, METH_VARARGS,                  
R"(assign_external(self, external, truth) -> None

Assign a truth value to an external atom (represented as a term). It is
possible to assign a Boolean or None.  A Boolean fixes the external to the
respective truth value; and None leaves its truth value open.

The truth value of an external atom can be changed before each solve call. An
atom is treated as external if it has been declared using an #external
directive, and has not been forgotten by calling Control.release_external or
defined in a logic program with some rule. If the given atom is not external,
then the function has no effect.

Arguments:
external -- term representing the external atom
truth    -- Boolean or None indicating the truth value 

See Control.release_external for an example.)"},
    // release_external
    {"release_external", (PyCFunction)release_external, METH_VARARGS,                  
R"(release_external(self, term) -> None

Release an external represented by the given term.

This function causes the corresponding atom to become permanently false if
there is no definition for the atom in the program. In all other cases this
function has no effect.

Example:

#script (python)
from gringo import Fun

def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(Fun("b"), True)
    prg.solve()
    prg.release_external(Fun("b"))
    prg.solve()

#end.

a.
#external b.

Expected Answer Sets:
a b
a)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef ControlWrap::getset[] = {
    {(char*)"conf", (getter)conf, nullptr, (char*)"ConfigProxy to change the configuration.", nullptr},
    {(char*)"domains", (getter)get_domains, nullptr, (char*)"Domain object to inspect the domains.", nullptr},
    {(char*)"use_enum_assumption", (getter)get_use_enum_assumption, (setter)set_use_enum_assumption, 
(char*)R"(Boolean determining how learnt information from enumeration modes is treated.

If the enumeration assumption is enabled, then all information learnt from
clasp's various enumeration modes is removed after a solve call. This includes
enumeration of cautious or brave consequences, enumeration of answer sets with
or without projection, or finding optimal models; as well as clauses/nogoods
added with Model.add_clause()/Model.add_nogood().

Note that initially the enumeration assumption is enabled.)", nullptr},
    {(char*)"stats", (getter)getStats, nullptr,
(char*)R"(A dictionary containing solve statistics of the last solve call.

Contains the statistics of the last Control.solve, Control.solve_async, or
Control.solve_iter call. The statistics correspond to the --stats output of
clingo.  The detail of the statistics depends on what level is requested on the
command line. Furthermore, you might want to start clingo using the --outf=3
option to disable all output from clingo.

Note that this (read-only) property is only available in clingo.

Example:
import json
json.dumps(prg.stats, sort_keys=True, indent=4, separators=(',', ': ')))", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyTypeObject ControlWrap::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    "gringo.Control",                         // tp_name
    sizeof(ControlWrap),                      // tp_basicsize
    0,                                        // tp_itemsize
    0,                                        // tp_dealloc
    0,                                        // tp_print
    0,                                        // tp_getattr
    0,                                        // tp_setattr
    0,                                        // tp_compare
    0,                                        // tp_repr
    0,                                        // tp_as_number
    0,                                        // tp_as_sequence
    0,                                        // tp_as_mapping
    0,                                        // tp_hash
    0,                                        // tp_call
    0,                                        // tp_str
    0,                                        // tp_getattro
    0,                                        // tp_setattro
    0,                                        // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, // tp_flags
    R"(Control(args) -> Control object

Control object for the grounding/solving process.

Arguments:
args -- optional arguments to the grounder and solver (default: []).

Note that only gringo options (without --text) and clasp's search options are
supported. Furthermore, a Control object is blocked while a search call is
active; you must not call any member function during search.)"
    ,                                         // tp_doc
    0,                                        // tp_traverse
    0,                                        // tp_clear
    0,                                        // tp_richcompare
    0,                                        // tp_weaklistoffset
    0,                                        // tp_iter
    0,                                        // tp_iternext
    methods,                                  // tp_methods
    0,                                        // tp_members
    getset,                                   // tp_getset
    0,                                        // tp_base
    0,                                        // tp_dict
    0,                                        // tp_descr_get
    0,                                        // tp_descr_set
    0,                                        // tp_dictoffset
    (initproc)init,                           // tp_init
    0,                                        // tp_alloc
    (newfunc)new2_,                           // tp_new
    (freefunc)free,                           // tp_free
    0,                                        // tp_is_gc
    0,                                        // tp_bases
    0,                                        // tp_mro
    0,                                        // tp_cache
    0,                                        // tp_subclasses
    0,                                        // tp_weaklist
    0,                                        // tp_del
    0,                                        // tp_version_tag
};

// }}}
// {{{ wrap module functions

static PyObject *cmpVal(PyObject *, PyObject *args) {
    PyObject *a, *b;
    if (!PyArg_ParseTuple(args, "OO", &a, &b)) { return nullptr; }
    Value va, vb;
    if (!pyToVal(a, va)) { return nullptr; }
    if (!pyToVal(b, vb)) { return nullptr; }
    int ret;
    if (va == vb)     { ret =  0; }
    else if (va < vb) { ret = -1; }
    else              { ret =  1; }
    return PyInt_FromLong(ret);
}

static PyObject *parseTerm(PyObject *, PyObject *objString) {
    char const *current = PyString_AsString(objString);
    Value value;
    if (!protect([current, &value] () { value = ControlWrap::module->parseValue(current); })) { return nullptr; }
    if (value.type() == Value::SPECIAL) { Py_RETURN_NONE; }
    else { return valToPy(value); }
}

// }}}
// {{{ gringo module

static PyMethodDef gringoMethods[] = {
    {"cmp",  (PyCFunction)cmpVal, METH_VARARGS, 
R"(cmp(a, b) -> { -1, 0, 1 }

Compare terms a and b using gringo's inbuilt comparison function.

Returns:
    -1 if a < b, 
     0 if a = b, and 
     1 otherwise.)"},
    {"parse_term", (PyCFunction)parseTerm, METH_O,
R"(parse_term(s) -> term

Parses the given string s using gringo's term parser for ground terms. The
function also evaluates arithmetic functions.

Example: 

parse_term('p(1+2)') == Fun("p", [3])
)"},
    {nullptr, nullptr, 0, nullptr}
};
static char const *strGrMod =
"The gringo-" GRINGO_VERSION R"( module.

This module provides functions and classes to work with ground terms and to
control the instantiation process.  In clingo builts, additional functions to
control and inspect the solving process are available.

Functions defined in a python script block are callable during the
instantiation process using @-syntax. The default grounding/solving process can
be customized if a main function is provided.

Note that gringo terms are wrapped in python classes provided in this module.
For string terms, numbers, and tuples the respective inbuilt python classes are
used.  Functions called during the grounding process from the logic program
must either return a term or a sequence of terms.  If a sequence is returned,
the corresponding @-term is successively substituted by the values in the
sequence.

Static Objects:

__version__ -- version of the gringo module ()" GRINGO_VERSION  R"()
Inf         -- represents #inf term
Sup         -- represents #sup term

Functions:

cmp()        -- compare terms as gringo does
parse_term() -- parse ground terms

Classes:

Control       -- control object for the grounding/solving process
ConfigProxy   -- proxy to change configuration
Domain        -- inspection of domains
DomainElement -- captures information about domain element
DomainIter    -- to iterate over domains
Fun           -- capture function terms - e.g., f, f(x), f(g(x)), etc.
InfType       -- capture #inf terms
Model         -- provides access to a model during solve call
SolveControl  -- object to control running search
SolveFuture   -- handle for asynchronous solve calls
SolveIter     -- handle to iterate models
SolveResult   -- result of a solve call
SupType       -- capture #sup terms

Example:

#script (python)
import gringo
def id(x):
    return x

def seq(x, y):
    return [x, y]

def main(prg):
    prg.ground([("base", [])])
    prg.solve()

#end.

p(@id(10)).
q(@seq(1,2)).
)";

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef gringoModule = {
    PyModuleDef_HEAD_INIT,
    "gringo",
    strGrMod,
    -1,
    gringoMethods,
    NULL,
    NULL,
    NULL,
    NULL
}; 
#endif

PyObject *initgringo_() {
    if (!PyEval_ThreadsInitialized()) { PyEval_InitThreads(); }
    if (PyType_Ready(&SupType::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&SupType::type);
    if (PyType_Ready(&InfType::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&InfType::type);
    if (PyType_Ready(&Fun::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&Fun::type);
    if (PyType_Ready(&Model::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (Model::addAttr() < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&Model::type);
    if (PyType_Ready(&SolveIter::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&SolveIter::type);
    if (PyType_Ready(&SolveFuture::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&SolveFuture::type);
    if (PyType_Ready(&SolveResult::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&SolveResult::type);
    if (SolveResult::addAttr() < 0) { throw std::runtime_error("could not initialize gringo module"); };
    if (PyType_Ready(&ControlWrap::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&ControlWrap::type);
    if (PyType_Ready(&ConfigProxy::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&ConfigProxy::type);
    if (PyType_Ready(&SolveControl::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&SolveControl::type);
    if (PyType_Ready(&DomainElement::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&DomainElement::type);
    if (PyType_Ready(&DomainIter::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&DomainIter::type);
    if (PyType_Ready(&DomainProxy::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    Py_INCREF(&DomainProxy::type);
#if PY_MAJOR_VERSION >= 3
    Object m = PyModule_Create(&gringoModule);
#else
    Object m = Py_InitModule3("gringo", gringoMethods, strGrMod);
#endif
    if (!m) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "SupType",       (PyObject*)&SupType::type)       < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "InfType",       (PyObject*)&InfType::type)       < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Fun",           (PyObject*)&Fun::type)           < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Model",         (PyObject*)&Model::type)         < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "SolveFuture",   (PyObject*)&SolveFuture::type)   < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "SolveIter",     (PyObject*)&SolveIter::type)     < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "SolveResult",   (PyObject*)&SolveResult::type)   < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Control",       (PyObject*)&ControlWrap::type)   < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "ConfigProxy",   (PyObject*)&ConfigProxy::type)   < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "SolveControl",  (PyObject*)&SolveControl::type)  < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "DomainElement", (PyObject*)&DomainElement::type) < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "DomainIter",    (PyObject*)&DomainIter::type)    < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Domain",        (PyObject*)&DomainProxy::type)   < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddStringConstant(m, "__version__", GRINGO_VERSION)            < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (InfType::init()                                                         < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Inf", InfType::inf)                              < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (SupType::init()                                                         < 0) { throw std::runtime_error("could not initialize gringo module"); }
    if (PyModule_AddObject(m, "Sup", SupType::sup)                              < 0) { throw std::runtime_error("could not initialize gringo module"); }
    return m.release();
}

// }}}

// {{{ auxiliary functions and objects

PyObject *valToPy(Value v) {
    switch (v.type()) {
        case Value::FUNC: {
            if (*v.name() == "") {
                FWValVec args = v.args();
                Object tuple = PyTuple_New(args.size());
                if (!tuple) { return nullptr; }
                int i = 0;
                for (auto &val : args) {
                    Object pyVal = valToPy(val);
                    if (!pyVal) { return nullptr; }
                    if (PyTuple_SetItem(tuple, i, pyVal.release()) < 0) { return nullptr; }
                    ++i;
                }
                return tuple.release();
            }
        }
        case Value::ID: {
            Object fun(Fun::new_(&Fun::type, nullptr, nullptr));
            if (!fun) { return nullptr; }
            reinterpret_cast<Fun*>(fun.get())->val = v;
            return fun.release();
        }
        case Value::SUP: {
            return Object(SupType::sup, true).release();
        }
        case Value::INF: {
            return Object(InfType::inf, true).release();
        }
        case Value::NUM: {
            return PyInt_FromLong(v.num());
        }
        case Value::STRING: {
            return PyString_FromString((*v.string()).c_str());
        }
        default: { 
            PyErr_SetString(PyExc_RuntimeError, "cannot happen");
            return nullptr;
        }
    }

}

bool pyToVal(Object obj, Value &val) {
    if (obj->ob_type == &SupType::type) {
        val = Value::createSup();
    }
    else if (obj->ob_type == &InfType::type) {
        val = Value::createInf();
    }
    else if (obj->ob_type == &Fun::type) {
        val = reinterpret_cast<Fun*>(obj.get())->val;
    }
    else if (PyTuple_Check(obj)) {
        ValVec vals;
        if (!pyToVals(obj, vals)) { return false; }
        if (vals.size() < 2) {
            PyErr_SetString(PyExc_RuntimeError, "cannot convert to value: tuples need at least two arguments");
        }
        if (!protect([&val, &vals]() { val = Value::createTuple(vals); })) { return false; }
    }
    else if (PyInt_Check(obj)) {
        val = Value::createNum(int(PyInt_AsLong(obj)));
    }
    else if (PyString_Check(obj)) {
        char const *value = PyString_AsString(obj);
        if (!protect([value, &val]() { val = Value::createStr(value); })) { return false; }
    }
    else {
        PyErr_Format(PyExc_RuntimeError, "cannot convert to value: unexpected %s() object", obj->ob_type->tp_name);
        return false;
    }
    return true;
}

template <class T>
PyObject *valsToPy(T const &vals) {
    Object list = PyList_New(vals.size());
    if (!list) { return nullptr; }
    int i = 0;
    for (auto &val : vals) {
        Object pyVal = valToPy(val);
        if (!pyVal) { return nullptr; }
        if (PyList_SetItem(list, i, pyVal.release()) < 0) { return nullptr; }
        ++i;
    }
    return list.release();
}

bool pyToVals(Object obj, ValVec &vals) {
    Object it = PyObject_GetIter(obj);
    if (!it) { return false; }
    while (Object pyVal = PyIter_Next(it)) {
        Value val;
        if (!pyToVal(pyVal, val)) { return false; }
        if (!protect([val, &vals]() { vals.emplace_back(val); })) { return false; }
    }
    if (PyErr_Occurred()) { return false; }
    return true;

}

// }}}

} // namespace

// {{{ definition of PythonImpl

struct PythonInit {
    PythonInit() : selfInit(!Py_IsInitialized()) { 
        if (selfInit) { 
#if PY_MAJOR_VERSION >= 3
            PyImport_AppendInittab("gringo", &initgringo_);
#endif
            Py_Initialize(); 
        }
    }
    ~PythonInit() { 
        if (selfInit) { Py_Finalize(); }
    }
    bool selfInit;
};

struct PythonImpl {
    PythonImpl() {
        if (init.selfInit) {
#if PY_MAJOR_VERSION >= 3
            static wchar_t const *argv[] = {L"clingo", 0};
            PySys_SetArgvEx(1, const_cast<wchar_t**>(argv), 0);
#else
            static char const *argv[] = {"clingo", 0};
            PySys_SetArgvEx(1, const_cast<char**>(argv), 0);
#endif
        }
#if PY_MAJOR_VERSION < 3
        PyObject *sysModules = PyImport_GetModuleDict();
        if (!sysModules) { throw std::runtime_error("could not initialize python interpreter"); }
        Object gringoStr = PyString_FromString("gringo");
        if (!gringoStr) { throw std::runtime_error("could not initialize python interpreter"); }
        int ret = PyDict_Contains(sysModules, gringoStr);
        if (ret == -1) { throw std::runtime_error("could not initialize python interpreter"); }
        if (ret == 0) { initgringo_(); }
#endif
        Object gringoModule = PyImport_ImportModule("gringo");
        if (!gringoModule) { throw std::runtime_error("could not initialize python interpreter"); }
        gringoModule.release(); // Note: stored in a few other places
        Object mainModule = PyImport_ImportModule("__main__");
        if (!mainModule) { throw std::runtime_error("could not initialize python interpreter"); }
        main = PyModule_GetDict(mainModule);
        if (!main) { throw std::runtime_error("could not initialize python interpreter"); }
    }
    bool exec(Location const &loc, FWString code) {
        std::ostringstream oss;
        oss << "<" << loc << ">";
        if (!pyExec((*code).c_str(), oss.str().c_str(), main)) { return false; }
        return true;
    }
    bool callable(PyObject *context, FWString name) {
        if (context) { return PyObject_HasAttrString(context, (*name).c_str()); }
        Object fun = PyMapping_GetItemString(context ? context : main, const_cast<char*>((*name).c_str()));
        PyErr_Clear();
        bool ret = fun && PyCallable_Check(fun);
        return ret;
    }
    bool call(PyObject *context, FWString name, ValVec const &args, ValVec &vals) {
        Object params = PyTuple_New(args.size());
        if (!params) { return false; }
        int i = 0;
        for (auto &val : args) {
            Object pyVal = valToPy(val);
            if (!pyVal) { return false; }
            if (PyTuple_SetItem(params, i, pyVal.release()) < 0) { return false; }
            ++i;
        }
        Object fun = context ? PyObject_GetAttrString(context, (*name).c_str()) : PyMapping_GetItemString(main, const_cast<char*>((*name).c_str()));
        if (!fun) { return false; }
        Object ret = PyObject_Call(fun, params, Py_None);
        if (!ret) { return false; }
        if (PyList_Check(ret)) {
            if (!pyToVals(ret, vals)) { return false; }
        }
        else {
            Value val;
            if (!pyToVal(ret, val)) { return false; }
            vals.emplace_back(val);
        }
        return true;
    }
    bool call(Gringo::Control &ctl) {
        Object fun = PyMapping_GetItemString(main, const_cast<char*>("main"));
        if (!fun) { return false; }
        Object params = PyTuple_New(1);
        if (!params) { return false; }
        Object param(ControlWrap::new_(ctl));
        if (!param) { return false; }
        if (PyTuple_SetItem(params, 0, param.release()) < 0) { return false; }
        Object ret = PyObject_Call(fun, params, Py_None);
        if (!ret) { return false; }
        return true;
    }
    PythonInit init;
    PyObject  *main;
};

// }}}
// {{{ definition of Python

std::unique_ptr<PythonImpl> Python::impl = nullptr;

Python::Python(GringoModule &module) {
    ControlWrap::module = &module;
}
bool Python::exec(Location const &loc, FWString code) {
    if (!impl) { impl = gringo_make_unique<PythonImpl>(); }
    if (!impl->exec(loc, code)) {
        handleError(loc, "parsing failed");
        return false;
    }
    return true;
}
bool Python::callable(Any const &context, FWString name) {
    if (Py_IsInitialized() && !impl) { impl = gringo_make_unique<PythonImpl>(); }
    PyObject * const *pyContext = context.get<PyObject*>();
    return impl && impl->callable(pyContext ? *pyContext : nullptr, name);
}
ValVec Python::call(Any const &context, Location const &loc, FWString name, ValVec const &args) {
    assert(impl);
    ValVec vals;
    PyObject * const *pyContext = context.get<PyObject*>();
    if (!impl->call(pyContext ? *pyContext : nullptr, name, args, vals)) {
        GRINGO_REPORT(W_OPERATION_UNDEFINED)
            << loc << ": info: operation undefined:\n"
            << errorToString()
            ;
        return {};
    }
    return vals;
}
void Python::main(Gringo::Control &ctl) {
    assert(impl);
    if (!impl->call(ctl)) {
        Location loc("<internal>", 1, 1, "<internal>", 1, 1);
        handleError(loc, "error while calling main function");
        return;
    }
}
Python::~Python() = default;

void *Python::initlib(Gringo::GringoModule &module) {
    ControlWrap::module = &module;
    PyObject *ret = nullptr;
    protect([&ret]() { ret = initgringo_(); });
    return ret;
}

// }}}

} // namespace Gringo

#else // WITH_PYTHON

#include "gringo/python.hh"
#include "gringo/value.hh"
#include "gringo/locatable.hh"
#include "gringo/logger.hh"

namespace Gringo {

// {{{ definition of Python

struct PythonImpl { };

std::unique_ptr<PythonImpl> Python::impl = nullptr;

Python::Python(GringoModule &) { }
bool Python::exec(Location const &loc, FWString ) {
    GRINGO_REPORT(E_ERROR)
        << loc << ": error: gringo has been build without python support\n"
        ;
    throw std::runtime_error("grounding stopped because of errors");
}
bool Python::callable(Any const &, FWString ) {
    return false;
}
ValVec Python::call(Any const &, Location const &, FWString , ValVec const &) {
    return {};
}
void Python::main(Control &) { }
Python::~Python() = default;
void *Python::initlib(Gringo::GringoModule &) {
    throw std::runtime_error("gringo lib has been build without python support");
}

// }}}

} // namespace Gringo

#endif // WITH_PYTHON
