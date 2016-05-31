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
#include "gringo/symbol.hh"
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

#ifdef COUNT_ALLOCS
// tp_allocs, tp_frees, tp_maxalloc, tp_prev, tp_next,
#define GRINGO_STRUCT_EXTRA 0, 0, 0, nullptr, nullptr,
#else
#define GRINGO_STRUCT_EXTRA
#endif

namespace Gringo {

using Id_t = uint32_t;

namespace {

// {{{1 workaround for gcc warnings

#if defined(__GNUC__) && !defined(__clang__)
void incRef(PyObject *op) {
    Py_INCREF(op);
}

template <class T>
void incRef(T *object) {
    incRef(reinterpret_cast<PyObject*>(object));
}

#undef Py_INCREF
#define Py_INCREF(op) incRef(op)
#endif

// }}}1

// {{{1 auxiliary functions and objects

struct PyException : std::exception { };

struct Object {
    Object()
    : obj(nullptr) { }
    Object(PyObject *obj, bool inc = false) : obj(obj) {
        if (inc) { Py_XINCREF(obj); }
        if (!obj && PyErr_Occurred()) { throw PyException(); }
    }
    Object(Object const &other) : obj(other.obj) {
        Py_XINCREF(obj);
    }
    Object(Object &&other) : obj(other.obj) {
        other.obj = nullptr;
    }
    bool none() const                      { return obj == Py_None; }
    bool valid() const                     { return obj; }
    PyObject *get() const                  { return obj; }
    PyObject *release()                    { PyObject *ret = obj; obj = nullptr; return ret; }
    PyObject *operator->() const           { return get(); }
    operator bool() const                  { return valid(); }
    operator PyObject*() const             { return get(); }
    Object &operator=(PyObject *other)     { Py_XDECREF(obj); obj = other; return *this; }
    Object &operator=(Object const &other) { Py_XDECREF(obj); obj = other.obj; Py_XINCREF(obj); return *this; }
    ~Object()                              { Py_XDECREF(obj); }
    PyObject *obj;
};

// NOTE: all the functions below can use execptions
//       to remove all the annoying return value checking
//       like in the callback there should be an exception
//       that indicates that a python exception is on the stack
//       this exception should simply be handled in PY_CATCH

template <class T>
void pyToCpp(PyObject *pyVec, std::vector<T> &vec);

void pyToCpp(PyObject *pyBool, bool &x) {
    x = PyObject_IsTrue(pyBool);
    if (PyErr_Occurred()) { throw PyException(); }
}

void pyToCpp(PyObject *pyStr, char const *&x) {
    x = PyString_AsString(pyStr);
    if (!x) { throw PyException(); }
}

template <class T>
void pyToCpp(PyObject *pyNum, T &x, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
    x = PyInt_AsLong(pyNum);
    if (PyErr_Occurred()) { throw PyException(); }
}

template <class T>
void pyToCpp(PyObject *pyNum, T &x, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr) {
    x = PyFloat_AsDouble(pyNum);
    if (PyErr_Occurred()) { throw PyException(); }
}

void pyToCpp(PyObject *obj, Symbol &val);

template <class T, class U>
void pyToCpp(PyObject *pyPair, std::pair<T, U> &x) {
    Object it = PyObject_GetIter(pyPair);
    Object pyVal = PyIter_Next(it);
    if (!pyVal) {
        PyErr_SetString(PyExc_RuntimeError, "pair expected");
        throw PyException();
    }
    pyToCpp(pyVal, x.first);
    pyVal = PyIter_Next(it);
    if (!pyVal) {
        PyErr_SetString(PyExc_RuntimeError, "pair expected");
        throw PyException();
    }
    pyToCpp(pyVal, x.second);
    pyVal = PyIter_Next(it);
    if (pyVal) {
        PyErr_SetString(PyExc_RuntimeError, "pair expected");
        throw PyException();
    }
}

void pyToCpp(PyObject *pyPair, Potassco::WeightLit_t &x) {
    std::pair<Lit_t &, Weight_t &> y{ x.lit, x.weight };
    pyToCpp(pyPair, y);
}

Object pyGetAttr(PyObject *o, char const *attr) {
    return PyMapping_Check(o) && PyMapping_HasKeyString(o, const_cast<char *>(attr))
        ? PyMapping_GetItemString(o, const_cast<char *>(attr))
        : PyObject_GetAttrString(o, attr);
}

struct ASTOwner {
    ASTOwner() = default;
    ASTOwner(ASTOwner &&) = default;
    ASTOwner(ASTOwner const &) = delete;
    ASTOwner &operator=(ASTOwner &&) = default;
    ASTOwner &operator=(ASTOwner const &) = delete;
    clingo_ast root;
    std::vector<clingo_ast> childAST;
    std::vector<ASTOwner> childOwner;
    std::string begin;
    std::string end;
};

struct LocOwner {
    LocOwner(std::string &str, size_t &line, size_t &column)
    : str(str), line(line), column(column) { }
    std::string &str;
    size_t &line;
    size_t &column;
};

void pyToCpp(PyObject *pyStr, std::string &str) {
    char const *cstr = PyString_AsString(pyStr);
    if (!cstr) { throw PyException(); }
    str = cstr;
}

void pyToCpp(PyObject *pyLoc, Gringo::LocOwner &loc) {
    // location.filename
    Object pyName = pyGetAttr(pyLoc, "filename");
    pyToCpp(pyName, loc.str);
    // location.line
    Object pyLine = pyGetAttr(pyLoc, "line");
    pyToCpp(pyLine, loc.line);
    // location.column
    Object pyColumn = pyGetAttr(pyLoc, "column");
    pyToCpp(pyColumn, loc.column);
}

void pyToCpp(PyObject *pyAST, Gringo::ASTOwner &ast) {
    // ast.value
    Object pyTerm = pyGetAttr(pyAST, "term");
    pyToCpp(pyTerm, static_cast<Symbol &>(ast.root.value));
    // ast.location
    Object pyLoc = pyGetAttr(pyAST, "location");
    // ast.location.begin
    Object pyBegin = pyGetAttr(pyLoc, "begin");
    LocOwner begin(ast.begin, ast.root.location.begin_line, ast.root.location.begin_column);
    pyToCpp(pyBegin, begin);
    ast.root.location.begin_file = ast.begin.c_str();
    // ast.location.end
    Object pyEnd = pyGetAttr(pyLoc, "end");
    LocOwner end(ast.end, ast.root.location.end_line, ast.root.location.end_column);
    ast.root.location.end_file = ast.end.c_str();
    // ast.children
    Object pyChildren = pyGetAttr(pyLoc, "children");
    pyToCpp(pyChildren, ast.childOwner);
    for (auto &x : ast.childOwner) {
        ast.childAST.emplace_back(x.root);
    }
}

template <class T>
void pyToCpp(PyObject *pyVec, std::vector<T> &vec) {
    Object it = PyObject_GetIter(pyVec);
    while (Object pyVal = PyIter_Next(it)) {
        T ret;
        pyToCpp(pyVal, ret);
        vec.emplace_back(std::move(ret));
    }
}

template <class T>
T pyToCpp(PyObject *py) {
    T ret;
    pyToCpp(py, ret);
    return ret;
}

PyObject *cppToPy(Symbol val);

template <class T>
Object cppToPy(std::vector<T> const &vals);
template <class T>
Object cppToPy(Potassco::Span<T> const &span);

Object cppToPy(char const *n) { return PyString_FromString(n); }
Object cppToPy(std::string const &s) { return cppToPy(s.c_str()); }
Object cppToPy(bool n) { return PyBool_FromLong(n); }
template <class T>
Object cppToPy(T n, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
    return PyInt_FromLong(n);
}

Object cppToPy(clingo_location const &l) {
    Object dict = PyDict_New();
    auto add = [](char const *n, size_t l, size_t c) -> Object {
        Object loc = PyDict_New();
        Object name = cppToPy(n);
        if (PyDict_SetItemString(loc, "filename", name) < 0) { throw PyException(); }
        Object line = cppToPy(l);
        if (PyDict_SetItemString(loc, "line", line) < 0) { throw PyException(); }
        Object column = cppToPy(c);
        if (PyDict_SetItemString(loc, "filename", column) < 0) { throw PyException(); }
        return loc;
    };
    Object begin = add(l.begin_file, l.begin_line, l.begin_column);
    if (PyDict_SetItemString(dict, "begin", begin) < 0) { throw PyException(); }
    Object end = add(l.end_file, l.end_line, l.end_column);
    if (PyDict_SetItemString(dict, "end", end) < 0) { throw PyException(); }
    return dict;
}

Object cppToPy(clingo_ast const &e) {
    Object dict = PyDict_New();
    Object term = cppToPy(e.value);
    if (PyDict_SetItemString(dict, "term", term) < 0) { throw PyException(); }
    Object children = cppToPy(e.children);
    if (PyDict_SetItemString(dict, "children", children) < 0) { throw PyException(); }
    Object loc = cppToPy(static_cast<clingo_location const &>(e.location));
    if (PyDict_SetItemString(dict, "location", loc) < 0) { throw PyException(); }
    return dict;
}

struct PyUnblock {
    PyUnblock() : state(PyEval_SaveThread()) { }
    ~PyUnblock() { PyEval_RestoreThread(state); }
    PyThreadState *state;
};
template <class T>
auto doUnblocked(T f) -> decltype(f()) {
    PyUnblock b; (void)b;
    return f();
}

struct PyBlock {
    PyBlock() : state(PyGILState_Ensure()) { }
    ~PyBlock() { PyGILState_Release(state); }
    PyGILState_STATE state;
};

Object pyExec(char const *str, char const *filename, PyObject *globals, PyObject *locals = Py_None) {
    if (locals == Py_None) { locals = globals; }
    Object x = Py_CompileString(str, filename, Py_file_input);
#if PY_MAJOR_VERSION >= 3
    return PyEval_EvalCode(x.get(), globals, locals);
#else
    return PyEval_EvalCode((PyCodeObject*)x.get(), globals, locals);
#endif
}

#define PY_TRY try {
#define PY_CATCH(ret) \
} \
catch (std::bad_alloc const &e) { PyErr_SetString(PyExc_MemoryError, e.what()); } \
catch (PyException const &e)    { } \
catch (std::exception const &e) { PyErr_SetString(PyExc_RuntimeError, e.what()); } \
catch (...)                     { PyErr_SetString(PyExc_RuntimeError, "unknown error"); } \
return (ret)

#define PY_HANDLE(func, msg) \
} \
catch (PyException const &e) { handleError(func, msg); throw std::logic_error("cannot happen"); }

#define CHECK_CMP(a, b, op) \
    if ((a)->ob_type != (b)->ob_type) { \
        if ((a)->ob_type != (b)->ob_type) { \
            return cppToPy(false).release(); \
        } \
        else if ((op) == Py_NE && (a)->ob_type != (b)->ob_type) { \
            return cppToPy(true).release(); \
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
        case Py_LT: { return cppToPy(a <  b).release(); }
        case Py_LE: { return cppToPy(a <= b).release(); }
        case Py_EQ: { return cppToPy(a == b).release(); }
        case Py_NE: { return cppToPy(a != b).release(); }
        case Py_GT: { return cppToPy(a >  b).release(); }
        case Py_GE: { return cppToPy(a >= b).release(); }
    }
    return cppToPy(false).release();
}

std::string errorToString() {
    try {
        Object type, value, traceback;
        PyErr_Fetch(&type.obj, &value.obj, &traceback.obj);
        PyErr_NormalizeException(&type.obj, &value.obj, &traceback.obj);
        Object tbModule = PyImport_ImportModule("traceback");
        Object tbDict   = {PyModule_GetDict(tbModule), true};
        Object tbFE     = {PyDict_GetItemString(tbDict, "format_exception"), true};
        Object ret      = PyObject_CallFunctionObjArgs(tbFE, type.get(), value ? value.get() : Py_None, traceback ? traceback.get() : Py_None, nullptr);
        Object it       = PyObject_GetIter(ret);
        std::ostringstream oss;
        while (Object line = PyIter_Next(it)) {
            oss << "  " << pyToCpp<char const *>(line);
        }
        PyErr_Clear();
        return oss.str();
    }
    catch (PyException const &) {
        PyErr_Clear();
        return "error during error handling";
    }
}
void handleError(Location const &loc, char const *msg) {
    std::ostringstream ss;
    ss << loc << ": error: " << msg << ":\n" << errorToString();
    throw GringoError(ss.str().c_str());
}

void handleError(char const *loc, char const *msg) {
    Location l(loc, 1, 1, loc, 1, 1);
    handleError(l, msg);
}

template <class T>
struct ObjectBase {
    PyObject_HEAD
    static PyTypeObject type;

    static constexpr reprfunc tp_repr = nullptr;
    static reprfunc tp_str;
    static constexpr hashfunc tp_hash = nullptr;
    static constexpr destructor tp_dealloc = nullptr;
    static constexpr richcmpfunc tp_richcompare = nullptr;
    static constexpr getiterfunc tp_iter = nullptr;
    static constexpr iternextfunc tp_iternext = nullptr;
    static constexpr initproc tp_init = nullptr;
    static constexpr newfunc tp_new = nullptr;
    static constexpr getattrofunc tp_getattro = nullptr;
    static constexpr setattrofunc tp_setattro = nullptr;
    static constexpr PySequenceMethods *tp_as_sequence = nullptr;
    static constexpr PyMappingMethods *tp_as_mapping = nullptr;
    static constexpr PyGetSetDef *tp_getset = nullptr;
    static PyMethodDef tp_methods[];

    static bool initType(PyObject *module) {
        if (PyType_Ready(&type) < 0) { return false; }
        Py_INCREF(&type);
        if (PyModule_AddObject(module, T::tp_type, (PyObject*)&type) < 0) { return false; }
        return true;
    }

    static T *new_() {
        return new_(&type, nullptr, nullptr);
    }

    static T *new_(PyTypeObject *type, PyObject *, PyObject *) {
        T *self;
        self = reinterpret_cast<T*>(type->tp_alloc(type, 0));
        if (!self) { return nullptr; }
        return self;
    }
};

template <class T>
reprfunc ObjectBase<T>::tp_str = reinterpret_cast<reprfunc>(T::tp_repr);

template <class T>
PyMethodDef ObjectBase<T>::tp_methods[] = {{nullptr, nullptr, 0, nullptr}};

template <class T>
PyTypeObject ObjectBase<T>::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    T::tp_name,                                       // tp_name
    sizeof(T),                                        // tp_basicsize
    0,                                                // tp_itemsize
    reinterpret_cast<destructor>(T::tp_dealloc),      // tp_dealloc
    nullptr,                                          // tp_print
    nullptr,                                          // tp_getattr
    nullptr,                                          // tp_setattr
    nullptr,                                          // tp_compare
    reinterpret_cast<reprfunc>(T::tp_repr),           // tp_repr
    nullptr,                                          // tp_as_number
    T::tp_as_sequence,                                // tp_as_sequence
    T::tp_as_mapping,                                 // tp_as_mapping
    reinterpret_cast<hashfunc>(T::tp_hash),           // tp_hash
    nullptr,                                          // tp_call
    reinterpret_cast<reprfunc>(T::tp_str),            // tp_str
    reinterpret_cast<getattrofunc>(T::tp_getattro),   // tp_getattro
    reinterpret_cast<setattrofunc>(T::tp_setattro),   // tp_setattro
    nullptr,                                          // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,         // tp_flags
    T::tp_doc,                                        // tp_doc
    nullptr,                                          // tp_traverse
    nullptr,                                          // tp_clear
    reinterpret_cast<richcmpfunc>(T::tp_richcompare), // tp_richcompare
    0,                                                // tp_weaklistoffset
    reinterpret_cast<getiterfunc>(T::tp_iter),        // tp_iter
    reinterpret_cast<iternextfunc>(T::tp_iternext),   // tp_iternext
    T::tp_methods,                                    // tp_methods
    nullptr,                                          // tp_members
    T::tp_getset,                                     // tp_getset
    nullptr,                                          // tp_base
    nullptr,                                          // tp_dict
    nullptr,                                          // tp_descr_get
    nullptr,                                          // tp_descr_set
    0,                                                // tp_dictoffset
    reinterpret_cast<initproc>(T::tp_init),           // tp_init
    nullptr,                                          // tp_alloc
    reinterpret_cast<newfunc>(T::tp_new),             // tp_new
    nullptr,                                          // tp_free
    nullptr,                                          // tp_is_gc
    nullptr,                                          // tp_bases
    nullptr,                                          // tp_mro
    nullptr,                                          // tp_cache
    nullptr,                                          // tp_subclasses
    nullptr,                                          // tp_weaklist
    nullptr,                                          // tp_del
    0,                                                // tp_version_tag
    GRINGO_STRUCT_EXTRA
};

template <class T>
struct EnumType : ObjectBase<T> {
    unsigned offset;

    static bool initType(PyObject *module) {
        return  ObjectBase<T>::initType(module) && addAttr() >= 0;
    }
    static PyObject *new_(unsigned offset) {
        EnumType *self;
        self = reinterpret_cast<EnumType*>(ObjectBase<T>::type.tp_alloc(&ObjectBase<T>::type, 0));
        if (!self) { return nullptr; }
        self->offset = offset;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *tp_repr(EnumType *self) {
        return PyString_FromString(T::strings[self->offset]);
    }

    template <class U>
    static PyObject *getAttr(U ret) {
        for (unsigned i = 0; i < sizeof(T::values) / sizeof(*T::values); ++i) {
            if (T::values[i] == ret) {
                PyObject *res = PyDict_GetItemString(ObjectBase<T>::type.tp_dict, T::strings[i]);
                Py_XINCREF(res);
                return res;
            }
        }
        return PyErr_Format(PyExc_RuntimeError, "should not happen");
    }

    static int addAttr() {
        for (unsigned i = 0; i < sizeof(T::values) / sizeof(*T::values); ++i) {
            Object elem(new_(i));
            if (!elem) { return -1; }
            if (PyDict_SetItemString(ObjectBase<T>::type.tp_dict, T::strings[i], elem) < 0) { return -1; }
        }
        return 0;
    }

    static long tp_hash(EnumType *self) {
        return static_cast<long>(self->offset);
    }

    static PyObject *tp_richcompare(EnumType *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(self->offset, reinterpret_cast<EnumType*>(b)->offset, op);
    }
};

// }}}1

// {{{1 wrap TheoryTerm

struct TheoryTermType : EnumType<TheoryTermType> {
    static constexpr char const *tp_type = "TheoryTermType";
    static constexpr char const *tp_name = "clingo.TheoryTermType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of theory terms.

TheoryTermType objects cannot be constructed from python. Instead the
following preconstructed objects are available:

TheoryTermType.Function -- a function theor term
TheoryTermType.Number   -- a numeric theory term
TheoryTermType.Symbol   -- a symbolic theory term
TheoryTermType.List     -- a list theory term
TheoryTermType.Tuple    -- a tuple theory term
TheoryTermType.Set      -- a set theory term)";

    static constexpr Gringo::TheoryData::TermType values[] = {
        Gringo::TheoryData::TermType::Function,
        Gringo::TheoryData::TermType::Number,
        Gringo::TheoryData::TermType::Symbol,
        Gringo::TheoryData::TermType::List,
        Gringo::TheoryData::TermType::Tuple,
        Gringo::TheoryData::TermType::Set
    };
    static constexpr const char * strings[] = {
        "Function",
        "Number",
        "Symbol",
        "List",
        "Tuple",
        "Set"
    };
};

constexpr TheoryData::TermType TheoryTermType::values[];
constexpr const char * TheoryTermType::strings[];

struct TheoryTerm : ObjectBase<TheoryTerm> {
    Gringo::TheoryData const *data;
    Id_t value;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "TheoryTerm";
    static constexpr char const *tp_name = "clingo.TheoryTerm";
    static constexpr char const *tp_doc =
R"(TheoryTerm objects represent theory terms.

This are read-only objects, which can be obtained from theory atoms and
elements.)";

    static PyObject *new_(Gringo::TheoryData const *data, Id_t value) {
        TheoryTerm *self = reinterpret_cast<TheoryTerm*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->value = value;
        self->data = data;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *name(TheoryTerm *self, void *) {
        PY_TRY
            return PyString_FromString(self->data->termName(self->value));
        PY_CATCH(nullptr);
    }
    static PyObject *number(TheoryTerm *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->data->termNum(self->value));
        PY_CATCH(nullptr);
    }
    static PyObject *args(TheoryTerm *self, void *) {
        PY_TRY
            Potassco::IdSpan span = self->data->termArgs(self->value);
            Object list = PyList_New(span.size);
            if (!list) { return nullptr; }
            for (size_t i = 0; i < span.size; ++i) {
                Object arg = new_(self->data, *(span.first + i));
                if (!arg) { return nullptr; }
                if (PyList_SetItem(list, i, arg.release()) < 0) { return nullptr; }
            }
            return list.release();
        PY_CATCH(nullptr);
    }
    static PyObject *tp_repr(TheoryTerm *self) {
        PY_TRY
            return PyString_FromString(self->data->termStr(self->value).c_str());
        PY_CATCH(nullptr);
    }
    static PyObject *termType(TheoryTerm *self, void *) {
        PY_TRY
            return TheoryTermType::getAttr(self->data->termType(self->value));
        PY_CATCH(nullptr);
    }
    static long tp_hash(TheoryTerm *self) {
        return self->value;
    }
    static PyObject *tp_richcompare(TheoryTerm *self, PyObject *b, int op) {
        PY_TRY
            CHECK_CMP(OBBASE(self), b, op)
            return doCmp(self->value, reinterpret_cast<TheoryTerm*>(b)->value, op);
        PY_CATCH(nullptr);
    }
};

PyGetSetDef TheoryTerm::tp_getset[] = {
    {(char *)"type", (getter)termType, nullptr, (char *)R"(type -> TheoryTermType

The type of the theory term.)", nullptr},
    {(char *)"name", (getter)name, nullptr, (char *)R"(name -> str

The name of the TheoryTerm\n(for symbols and functions).)", nullptr},
    {(char *)"args", (getter)args, nullptr, (char *)R"(args -> [Term]

The arguments of the TheoryTerm (for functions, tuples, list, and sets).)", nullptr},
    {(char *)"number", (getter)number, nullptr, (char *)R"(number -> integer

The numeric representation of the TheoryTerm (for numbers).)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryElement

struct TheoryElement : ObjectBase<TheoryElement> {
    Gringo::TheoryData const *data;
    Id_t value;
    static constexpr char const *tp_type = "TheoryElement";
    static constexpr char const *tp_name = "clingo.TheoryElement";
    static constexpr char const *tp_doc =
R"(TheoryElement objects represent theory elements which consist of a tuple of
terms and a set of literals.)";
    static PyGetSetDef tp_getset[];
    static PyObject *new_(Gringo::TheoryData const *data, Id_t value) {
        TheoryElement *self;
        self = reinterpret_cast<TheoryElement*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->value = value;
        self->data = data;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *terms(TheoryElement *self, void *) {
        PY_TRY
            Potassco::IdSpan span = self->data->elemTuple(self->value);
            Object list = PyList_New(span.size);
            for (size_t i = 0; i < span.size; ++i) {
                Object arg = TheoryTerm::new_(self->data, *(span.first + i));
                if (PyList_SetItem(list, i, arg.release()) < 0) { return nullptr; }
            }
            return list.release();
        PY_CATCH(nullptr);
    }
    static PyObject *condition(TheoryElement *self, void *) {
        PY_TRY
            Potassco::LitSpan span = self->data->elemCond(self->value);
            Object list = PyList_New(span.size);
            for (size_t i = 0; i < span.size; ++i) {
                Object arg = PyInt_FromLong(*(span.first + i));
                if (PyList_SetItem(list, i, arg.release()) < 0) { return nullptr; }
            }
            return list.release();
        PY_CATCH(nullptr);
    }
    static PyObject *condition_literal(TheoryElement *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->data->elemCondLit(self->value));
        PY_CATCH(nullptr);
    }
    static PyObject *tp_repr(TheoryElement *self) {
        PY_TRY
            return PyString_FromString(self->data->elemStr(self->value).c_str());
        PY_CATCH(nullptr);
    }
    static long tp_hash(TheoryElement *self) {
        return self->value;
    }
    static PyObject *tp_richcompare(TheoryElement *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(self->value, reinterpret_cast<TheoryElement*>(b)->value, op);
    }
};

PyGetSetDef TheoryElement::tp_getset[] = {
    {(char *)"terms", (getter)terms, nullptr, (char *)R"(terms -> [TheoryTerm]

The tuple of the element.)", nullptr},
    {(char *)"condition", (getter)condition, nullptr, (char *)R"(condition -> [Term]

The conditon of the element.)", nullptr},
    {(char *)"condition_literal", (getter)condition_literal, nullptr, (char *)R"(condition_literal -> int

The single program literal associated with the condition.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryAtom

struct TheoryAtom : ObjectBase<TheoryAtom> {
    Gringo::TheoryData const *data;
    Id_t value;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "TheoryAtom";
    static constexpr char const *tp_name = "clingo.TheoryAtom";
    static constexpr char const *tp_doc =
R"(TheoryAtom objects represent theory atoms.)";
    static PyObject *new_(Gringo::TheoryData const *data, Id_t value) {
        TheoryAtom *self;
        self = reinterpret_cast<TheoryAtom*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->value = value;
        self->data = data;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *elements(TheoryAtom *self, void *) {
        PY_TRY
            Potassco::IdSpan span = self->data->atomElems(self->value);
            Object list = PyList_New(span.size);
            for (size_t i = 0; i < span.size; ++i) {
                Object arg = TheoryElement::new_(self->data, *(span.first + i));
                if (PyList_SetItem(list, i, arg.release()) < 0) { return nullptr; }
            }
            return list.release();
        PY_CATCH(nullptr);
    }
    static PyObject *term(TheoryTerm *self, void *) {
        PY_TRY
            return TheoryTerm::new_(self->data, self->data->atomTerm(self->value));
        PY_CATCH(nullptr);
    }
    static PyObject *literal(TheoryTerm *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->data->atomLit(self->value));
        PY_CATCH(nullptr);
    }
    static PyObject *guard(TheoryTerm *self, void *) {
        PY_TRY
            if (!self->data->atomHasGuard(self->value)) { Py_RETURN_NONE; }
            std::pair<char const *, Id_t> guard = self->data->atomGuard(self->value);
            Object tuple = PyTuple_New(2);
            Object op = PyString_FromString(guard.first);
            if (PyTuple_SetItem(tuple, 0, op.release()) < 0) { return nullptr; }
            Object term = TheoryTerm::new_(self->data, guard.second);
            if (PyTuple_SetItem(tuple, 1, term.release()) < 0) { return nullptr; }
            return tuple.release();
        PY_CATCH(nullptr);
    }
    static PyObject *tp_repr(TheoryAtom *self) {
        PY_TRY
            return PyString_FromString(self->data->atomStr(self->value).c_str());
        PY_CATCH(nullptr);
    }
    static long tp_hash(TheoryAtom *self) {
        return self->value;
    }
    static PyObject *tp_richcompare(TheoryAtom *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        return doCmp(self->value, reinterpret_cast<TheoryAtom*>(b)->value, op);
    }
};

PyGetSetDef TheoryAtom::tp_getset[] = {
    {(char *)"elements", (getter)elements, nullptr, (char *)R"(elements -> [TheoryElement]

The theory elements of the theory atom.)", nullptr},
    {(char *)"term", (getter)term, nullptr, (char *)R"(term -> TheoryTerm

The term of the theory atom.)", nullptr},
    {(char *)"guard", (getter)guard, nullptr, (char *)R"(guard -> (str, TheoryTerm)

The guard of the theory atom or None if the atom has no guard.)", nullptr},
    {(char *)"literal", (getter)literal, nullptr, (char *)R"(literal -> int

The program literal associated with the theory atom.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryAtomIter

struct TheoryAtomIter : ObjectBase<TheoryAtomIter> {
    Gringo::TheoryData const *data;
    Id_t offset;
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "TheoryAtomIter";
    static constexpr char const *tp_name = "clingo.TheoryAtomIter";
    static constexpr char const *tp_doc =
R"(Object to iterate over all theory atoms.)";
    static PyObject *new_(Gringo::TheoryData const *data, Id_t offset) {
        TheoryAtomIter *self;
        self = reinterpret_cast<TheoryAtomIter*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->data = data;
        self->offset = offset;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject* tp_iter(PyObject *self) {
        Py_INCREF(self);
        return self;
    }
    static PyObject* get(TheoryAtomIter *self) {
        return TheoryAtom::new_(self->data, self->offset);
    }
    static PyObject* tp_iternext(TheoryAtomIter *self) {
        PY_TRY
            if (self->offset < self->data->numAtoms()) {
                Object next = get(self);
                ++self->offset;
                return next.release();
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        PY_CATCH(nullptr);
    }
    static PyObject *enter(TheoryAtomIter *self) {
        Py_INCREF(self);
        return (PyObject*)self;
    }
    static PyObject *exit(TheoryAtomIter *, PyObject *) {
        return cppToPy(false).release();
    }
};

PyMethodDef TheoryAtomIter::tp_methods[] = {
    {"__enter__", (PyCFunction)enter, METH_NOARGS,
R"(__enter__(self) -> TheoryAtomIter

Returns self.)"},
    {"get", (PyCFunction)get, METH_NOARGS,
R"(get(self) -> TheoryAtom)"},
    {"__exit__", (PyCFunction)exit, METH_VARARGS,
R"(__exit__(self, type, value, traceback) -> bool

Follows python __exit__ conventions. Does not suppress exceptions.)"},
    {nullptr, nullptr, 0, nullptr}
};

// {{{1 wrap Term

struct TermType : EnumType<TermType> {
    enum Type { Number, String, Function, Inf, Sup };
    static constexpr char const *tp_type = "TermType";
    static constexpr char const *tp_name = "clingo.TermType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of theory terms.

TermType objects cannot be constructed from python. Instead the following
preconstructed objects are available:

TermType.Number   -- a numeric term - e.g., 1
TermType.String   -- a string term - e.g., "a"
TermType.Function -- a numeric theory term - e.g., c, (1, "a"), or f(1,"a")
TermType.Inf      -- a tuple theory term - i.e., #inf
TermType.Sup      -- a set theory term - i.e., #sup)";

    static constexpr Type values[] =          {  Number,   String,   Function,   Inf,   Sup };
    static constexpr const char * strings[] = { "Number", "String", "Function", "Inf", "Sup" };
};

constexpr TermType::Type TermType::values[];
constexpr const char * TermType::strings[];

struct Term : ObjectBase<Term> {
    Symbol val;
    static PyObject *inf;
    static PyObject *sup;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "Term";
    static constexpr char const *tp_name = "clingo.Term";
    static constexpr char const *tp_doc =
R"(Represents a gringo term.

This includes numbers, strings, functions (including constants with len(args)
== 0 and tuples with len(name) == 0), #inf and #sup. Term objects are ordered
like in gringo and their string representation corresponds to their gringo
representation.

Note that this class does not have a constructor. Instead there are the
functions number(), string(), and function() to construct term objects or the
preconstructed terms Inf and Sup.)";

    static bool initType(PyObject *module) {
        if (!ObjectBase<Term>::initType(module)) { return false; }
        inf = type.tp_alloc(&type, 0);
        if (!inf) { return false; }
        reinterpret_cast<Term*>(inf)->val = Symbol::createInf();
        if (PyModule_AddObject(module, "Inf", inf) < 0) { return false; }
        sup = type.tp_alloc(&type, 0);
        reinterpret_cast<Term*>(sup)->val = Symbol::createSup();
        if (!sup) { return false; }
        if (PyModule_AddObject(module, "Sup", sup) < 0) { return false; }
        return true;
    }

    static PyObject *new_(Symbol value) {
        if (value.type() == SymbolType::Inf) {
            Py_INCREF(inf);
            return inf;
        }
        else if (value.type() == SymbolType::Sup) {
            Py_INCREF(sup);
            return sup;
        }
        else {
            Term *self = reinterpret_cast<Term*>(type.tp_alloc(&type, 0));
            if (!self) { return nullptr; }
            new (&self->val) Symbol(value);
            return reinterpret_cast<PyObject*>(self);
        }
    }

    static PyObject *new_function_(char const *name, PyObject *params, PyObject *pySign) {
        PY_TRY
            auto sign = pyToCpp<bool>(pySign);
            if (strcmp(name, "") == 0 && sign) {
                PyErr_SetString(PyExc_RuntimeError, "tuples must not have signs");
                return nullptr;
            }
            if (params) {
                SymVec vals;
                pyToCpp(params, vals);
                return new_(Symbol::createFun(name, Potassco::toSpan(vals), sign));
            }
            else {
                return new_(Symbol::createId(name, sign));
            }
        PY_CATCH(nullptr);
    }
    static PyObject *new_function(PyObject *, PyObject *args, PyObject *kwds) {
        PY_TRY
            static char const *kwlist[] = {"name", "args", "sign", nullptr};
            char const *name;
            PyObject *pySign = Py_False;
            PyObject *params = nullptr;
            if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|OO", const_cast<char**>(kwlist), &name, &params, &pySign)) { return nullptr; }
            return new_function_(name, params, pySign);
        PY_CATCH(nullptr);
    }
    static PyObject *new_tuple(PyObject *, PyObject *arg) {
        return new_function_("", arg, Py_False);
    }
    static PyObject *new_number(PyObject *, PyObject *arg) {
        PY_TRY
            auto num = pyToCpp<int>(arg);
            return new_(Symbol::createNum(num));
        PY_CATCH(nullptr);
    }
    static PyObject *new_string(PyObject *, PyObject *arg) {
        PY_TRY
            char const *str = pyToCpp<char const *>(arg);
            return new_(Symbol::createStr(str));
        PY_CATCH(nullptr);
    }
    static PyObject *name(Term *self, void *) {
        PY_TRY
            if (self->val.type() == SymbolType::Fun) {
                return PyString_FromString(self->val.name().c_str());
            }
            else {
                Py_RETURN_NONE;
            }
        PY_CATCH(nullptr);
    }

    static PyObject *string(Term *self, void *) {
        PY_TRY
            if (self->val.type() == SymbolType::Str) {
                return PyString_FromString(self->val.string().c_str());
            }
            else {
                Py_RETURN_NONE;
            }
        PY_CATCH(nullptr);
    }

    static PyObject *sign(Term *self, void *) {
        PY_TRY
            if (self->val.type() == SymbolType::Fun) {
                return PyBool_FromLong(self->val.sign());
            }
            else {
                Py_RETURN_NONE;
            }
        PY_CATCH(nullptr);
    }

    static PyObject *num(Term *self, void *) {
        PY_TRY
            if (self->val.type() == SymbolType::Num) {
                return PyInt_FromLong(self->val.num());
            }
            else {
                Py_RETURN_NONE;
            }
        PY_CATCH(nullptr);
    }

    static PyObject *args(Term *self, void *) {
        PY_TRY
            if (self->val.type() == SymbolType::Fun) {
                return cppToPy(self->val.args()).release();
            }
            else {
                Py_RETURN_NONE;
            }
        PY_CATCH(nullptr);
    }

    static PyObject *type_(Term *self, void *) {
        PY_TRY
            switch (self->val.type()) {
                case SymbolType::Str:     { return TermType::getAttr(TermType::String); }
                case SymbolType::Num:     { return TermType::getAttr(TermType::Number); }
                case SymbolType::Inf:     { return TermType::getAttr(TermType::Inf); }
                case SymbolType::Sup:     { return TermType::getAttr(TermType::Sup); }
                case SymbolType::Fun:     { return TermType::getAttr(TermType::Function); }
                case SymbolType::Special: { throw std::logic_error("must not happen"); }
            }
        PY_CATCH(nullptr);
    }
    static PyObject *tp_repr(Term *self) {
        PY_TRY
            std::ostringstream oss;
            oss << self->val;
            return PyString_FromString(oss.str().c_str());
        PY_CATCH(nullptr);
    }

    static long tp_hash(Term *self) {
        return self->val.hash();
    }

    static PyObject *tp_richcompare(Term *self, PyObject *b, int op) {
        CHECK_CMP(OBBASE(self), b, op)
        // Note: should not throw
        return doCmp(self->val, reinterpret_cast<Term*>(b)->val, op);
    }
};

PyGetSetDef Term::tp_getset[] = {
    {(char *)"name", (getter)name, nullptr, (char *)"The name of a function.", nullptr},
    {(char *)"string", (getter)string, nullptr, (char *)"The value of a string.", nullptr},
    {(char *)"number", (getter)num, nullptr, (char *)"The value of a number.", nullptr},
    {(char *)"args", (getter)args, nullptr, (char *)"The arguments of a function.", nullptr},
    {(char *)"sign", (getter)sign, nullptr, (char *)"The sign of a function.", nullptr},
    {(char *)"type", (getter)type_, nullptr, (char *)"The type of the term.", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyObject *Term::inf = nullptr;
PyObject *Term::sup = nullptr;

// {{{1 wrap SolveResult

struct SolveResult : ObjectBase<SolveResult> {
    Gringo::SolveResult result;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "SolveResult";
    static constexpr char const *tp_name = "clingo.SolveResult";
    static constexpr char const *tp_doc =
R"(Captures the result of a solve call.

SolveResult objects cannot be constructed from python. Instead they
are returned by the solve methods of the Control object.)";
    static PyObject *new_(Gringo::SolveResult result) {
        SolveResult *self;
        self = reinterpret_cast<SolveResult*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->result = result;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *satisfiable(SolveResult *self, void *) {
        switch (self->result.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { Py_RETURN_TRUE; }
            case Gringo::SolveResult::Unsatisfiable: { Py_RETURN_FALSE; }
            case Gringo::SolveResult::Unknown:       { Py_RETURN_NONE; }
        }
        Py_RETURN_NONE;
    }
    static PyObject *unsatisfiable(SolveResult *self, void *) {
        switch (self->result.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { Py_RETURN_FALSE; }
            case Gringo::SolveResult::Unsatisfiable: { Py_RETURN_TRUE; }
            case Gringo::SolveResult::Unknown:       { Py_RETURN_NONE; }
        }
        Py_RETURN_NONE;
    }
    static PyObject *unknown(SolveResult *self, void *) {
        switch (self->result.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { Py_RETURN_FALSE; }
            case Gringo::SolveResult::Unsatisfiable: { Py_RETURN_FALSE; }
            case Gringo::SolveResult::Unknown:       { Py_RETURN_TRUE; }
        }
        Py_RETURN_NONE;
    }
    static PyObject *exhausted(SolveResult *self, void *) {
        return cppToPy(self->result.exhausted()).release();
    }
    static PyObject *interrupted(SolveResult *self, void *) {
        return cppToPy(self->result.interrupted()).release();
    }
    static PyObject *tp_repr(SolveResult *self, PyObject *) {
        switch (self->result.satisfiable()) {
            case Gringo::SolveResult::Satisfiable:   { return PyString_FromString("SAT"); }
            case Gringo::SolveResult::Unsatisfiable: { return PyString_FromString("UNSAT"); }
            case Gringo::SolveResult::Unknown:       { return PyString_FromString("UNKNOWN"); }
        }
        return PyString_FromString("UNKNOWN");
    }
};

PyGetSetDef SolveResult::tp_getset[] = {
    {(char *)"satisfiable", (getter)satisfiable, nullptr,
(char *)R"(True if the problem is satisfiable, False if the problem is
unsatisfiable, or None if the satisfiablity is not known.)", nullptr},
    {(char *)"unsatisfiable", (getter)unsatisfiable, nullptr,
(char *)R"(True if the problem is unsatisfiable, False if the problem is
satisfiable, or None if the satisfiablity is not known.

This is equivalent to None if satisfiable is None else not satisfiable.)", nullptr},
    {(char *)"unknown", (getter)unknown, nullptr,
(char *)R"(True if the satisfiablity is not known.

This is equivalent to satisfiable is None.)", nullptr},
    {(char *)"exhausted", (getter)exhausted, nullptr,
(char *)R"(True if the search space was exhausted.)", nullptr},
    {(char *)"interrupted", (getter)interrupted, nullptr,
(char *)R"(True if the search was interrupted.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Statistics

PyObject *getStatistics(Statistics const *stats, char const *prefix) {
    PY_TRY
        Statistics::Quantity ret = stats->getStat(prefix);
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
                char const *keys = stats->getKeys(prefix);
                if (!keys) { return PyErr_Format(PyExc_RuntimeError, "error zero keys string: %s", prefix); }
                if (strcmp(keys, "__len") == 0) {
                    std::string lenPrefix;
                    lenPrefix += prefix;
                    lenPrefix += "__len";
                    int len = (int)(double)stats->getStat(lenPrefix.c_str());
                    Object list = PyList_New(len);
                    for (int i = 0; i < len; ++i) {
                        Object objPrefix = PyString_FromFormat("%s%d.", prefix, i);
                        auto subPrefix = pyToCpp<char const *>(objPrefix);
                        Object subStats = getStatistics(stats, subPrefix);
                        if (PyList_SetItem(list, i, subStats.release()) < 0) { return nullptr; }
                    }
                    return list.release();
                }
                else {
                    Object dict = PyDict_New();
                    for (char const *it = keys; *it; it+= strlen(it) + 1) {
                        int len = strlen(it);
                        Object key = PyString_FromStringAndSize(it, len - (it[len-1] == '.'));
                        Object objPrefix = PyString_FromFormat("%s%s", prefix, it);
                        auto subPrefix = pyToCpp<char const *>(objPrefix);
                        Object subStats = getStatistics(stats, subPrefix);
                        if (PyDict_SetItem(dict, key, subStats) < 0) { return nullptr; }
                    }
                    return dict.release();
                }
            }
        }
        return PyErr_Format(PyExc_RuntimeError, "error unhandled prefix: %s", prefix);
    PY_CATCH(nullptr);
}

// {{{1 wrap SolveControl

struct SolveControl : ObjectBase<SolveControl> {
    Gringo::Model const *model;
    static PyMethodDef tp_methods[];
    static constexpr char const *tp_type = "SolveControl";
    static constexpr char const *tp_name = "clingo.SolveControl";
    static constexpr char const *tp_doc =
R"(Object that allows for controlling a running search.

Note that SolveControl objects cannot be constructed from python.  Instead
they are available as properties of Model objects.)";

    static PyObject *new_(Gringo::Model const &model) {
        SolveControl *self;
        self = reinterpret_cast<SolveControl*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->model = &model;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *getClause(SolveControl *self, PyObject *pyLits, bool invert) {
        PY_TRY
            auto lits = pyToCpp<Gringo::Model::LitVec>(pyLits);
            if (invert) {
                for (auto &lit : lits) { lit.second = !lit.second; }
            }
            self->model->addClause(lits);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }

    static PyObject *add_clause(SolveControl *self, PyObject *pyLits) {
        return getClause(self, pyLits, false);
    }

    static PyObject *add_nogood(SolveControl *self, PyObject *pyLits) {
        return getClause(self, pyLits, true);
    }
};

PyMethodDef SolveControl::tp_methods[] = {
    // add_clause
    {"add_clause", (PyCFunction)add_clause, METH_O,
R"(add_clause(self, lits) -> None

Add a clause that applies to the current solving step during the search.

Arguments:
lits -- list of literals represented as pairs of atoms and Booleans

Note that this function can only be called in the model callback (or while
iterating when using a SolveIter).)"},
    // add_nogood
    {"add_nogood", (PyCFunction)add_nogood, METH_O,
R"(add_nogood(self, lits) -> None

Equivalent to add_clause with the literals inverted.

Arguments:
lits -- list of pairs of Booleans and atoms representing the nogood)"},
    {nullptr, nullptr, 0, nullptr}
};

// {{{1 wrap Model

struct Model : ObjectBase<Model> {
    Gringo::Model const *model;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "Model";
    static constexpr char const *tp_name = "clingo.Model";
    static constexpr char const *tp_doc =
R"(Provides access to a model during a solve call.

The string representation of a model object is similar to the output of models
by clingo using the default output.

Note that model objects cannot be constructed from python.  Instead they are
passed as argument to a model callback (see Control.solve() and
Control.solve_async()).  Furthermore, the lifetime of a model object is limited
to the scope of the callback. They must not be stored for later use in other
places like - e.g., the main function.)";

    static PyObject *new_(Gringo::Model const &model) {
        Model *self;
        self = reinterpret_cast<Model*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->model = &model;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject *contains(Model *self, PyObject *arg) {
        PY_TRY
            Symbol val;
            pyToCpp(arg, val);
            return cppToPy(self->model->contains(val)).release();
        PY_CATCH(nullptr);
    }
    static PyObject *atoms(Model *self, PyObject *pyargs, PyObject *pykwds) {
        PY_TRY
            unsigned atomset = 0;
            static char const *kwlist[] = {"atoms", "terms", "shown", "csp", "comp", nullptr};
            PyObject *pyAtoms = Py_False, *pyTerms = Py_False, *pyShown = Py_False, *pyCSP = Py_False, *pyComp = Py_False;
            if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "|OOOOO", const_cast<char**>(kwlist), &pyAtoms, &pyTerms, &pyShown, &pyCSP, &pyComp)) { return nullptr; }
            if (pyToCpp<bool>(pyAtoms)) { atomset |= clingo_show_type_atoms; }
            if (pyToCpp<bool>(pyTerms)) { atomset |= clingo_show_type_terms; }
            if (pyToCpp<bool>(pyShown)) { atomset |= clingo_show_type_shown; }
            if (pyToCpp<bool>(pyCSP))   { atomset |= clingo_show_type_csp; }
            if (pyToCpp<bool>(pyComp))  { atomset |= clingo_show_type_comp; }
            return cppToPy(self->model->atoms(atomset)).release();
        PY_CATCH(nullptr);
    }
    static PyObject *optimization(Model *self, void *) {
        PY_TRY
            return cppToPy(self->model->optimization()).release();
        PY_CATCH(nullptr);
    }
    static PyObject *tp_repr(Model *self, PyObject *) {
        PY_TRY
            auto printAtom = [](std::ostream &out, Symbol val) {
                auto sig = val.sig();
                if (val.type() == SymbolType::Fun && sig.name() == "$" && sig.arity() == 2) {
                    auto args = val.args().first;
                    out << args[0] << "=" << args[1];
                }
                else { out << val; }
            };
            std::ostringstream oss;
            print_comma(oss, self->model->atoms(clingo_show_type_shown), " ", printAtom);
            return cppToPy(oss.str()).release();
        PY_CATCH(nullptr);
    }
    static PyObject *getContext(Model *self, void *) {
        return SolveControl::new_(*self->model);
    }
};

PyGetSetDef Model::tp_getset[] = {
    {(char *)"context", (getter)getContext, nullptr, (char*)"SolveControl object that allows for controlling the running search.", nullptr},
    {(char *)"optimization", (getter)optimization, nullptr,
(char *)R"(Return the list of integer optimization values of the model.

The return values correspond to clasp's optimization output.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyMethodDef Model::tp_methods[] = {
    {"atoms", (PyCFunction)atoms, METH_VARARGS | METH_KEYWORDS,
R"(atoms(self, atoms, terms, shown, csp, comp)
        -> list of terms

Return the list of atoms, terms, or CSP assignments in the model.

Keyword Arguments:
atoms -- select all atoms in the model (independent of #show statements)
         (Default: False)
terms -- select all terms displayed with #show statements in the model
         (Default: False)
shown -- select all atoms and terms as outputted by clingo
         (Default: False)
csp   -- select all csp assignments (independent of #show statements)
         (Default: False)
comp  -- return the complement of the answer set w.r.t. to the Herbrand
         base accumulated so far (does not affect csp assignments)
         (Default: False)

Note that atoms are represented using functions (Term objects), and that CSP
assignments are represented using functions with name "$" where the first
argument is the name of the CSP variable and the second its value.)"},
    {"contains", (PyCFunction)contains, METH_O,
R"(contains(self, a) -> bool

Check if an atom a is contained in the model.

The atom must be represented using a function term.)"},
    {nullptr, nullptr, 0, nullptr}

};

// {{{1 wrap SolveFuture

struct SolveFuture : ObjectBase<SolveFuture> {
    Gringo::SolveFuture *future;
    PyObject *mh;
    PyObject *fh;

    static PyMethodDef tp_methods[];
    static constexpr char const *tp_type = "SolveFuture";
    static constexpr char const *tp_name = "clingo.SolveFuture";
    static constexpr char const *tp_doc =
R"(Handle for asynchronous solve calls.

SolveFuture objects cannot be created from python. Instead they are returned by
Control.solve_async, which performs a search in the background.  A SolveFuture
object can be used to wait for such a background search or cancel it.

Functions in this object release the GIL. They are not thread-safe though.

See Control.solve_async for an example.)";

    static PyObject *new_(Gringo::SolveFuture &future, PyObject *mh, PyObject *fh) {
        SolveFuture *self;
        self = reinterpret_cast<SolveFuture*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->future = &future;
        self->mh = mh;
        self->fh = fh;
        Py_XINCREF(self->mh);
        Py_XINCREF(self->fh);
        return reinterpret_cast<PyObject*>(self);
    }

    static void tp_dealloc(SolveFuture *self) {
        Py_XDECREF(self->mh);
        Py_XDECREF(self->fh);
        type.tp_free(self);
    }

    static PyObject *get(SolveFuture *self, PyObject *) {
        PY_TRY
            return SolveResult::new_(doUnblocked([self]() { return self->future->get(); }));
        PY_CATCH(nullptr);
    }

    static PyObject *wait(SolveFuture *self, PyObject *args) {
        PY_TRY
            PyObject *timeout = nullptr;
            if (!PyArg_ParseTuple(args, "|O", &timeout)) { return nullptr; }
            if (!timeout) {
                doUnblocked([self](){ self->future->wait(); });
                Py_RETURN_NONE;
            }
            else {
                auto time = pyToCpp<double>(timeout);
                return cppToPy(doUnblocked([self, time](){ return self->future->wait(time); })).release();
            }
        PY_CATCH(nullptr);
    }

    static PyObject *cancel(SolveFuture *self, PyObject *) {
        PY_TRY
            doUnblocked([self](){ self->future->cancel(); });
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
};

PyMethodDef SolveFuture::tp_methods[] = {
    {"get", (PyCFunction)get, METH_NOARGS,
R"(get(self) -> SolveResult

Get the result of an solve_async call.

If the search is not completed yet, the function blocks until the result is
ready.)"},
    {"wait", (PyCFunction)wait,  METH_VARARGS,
R"(wait(self, timeout) -> None or bool

Wait for solve_async call to finish with an optional timeout.

If a timeout is given, the function waits at most timeout seconds and returns a
Boolean indicating whether the search has finished. Otherwise, the function
blocks until the search is finished and returns nothing.

Arguments:
timeout -- optional timeout in seconds
           (permits floating point values))"},
    {"cancel", (PyCFunction)cancel, METH_NOARGS,
R"(cancel(self) -> None

Cancel the running search.

See Control.interrupt() for a thread-safe alternative.)"},
    {nullptr, nullptr, 0, nullptr}
};

// {{{1 wrap SolveIter

struct SolveIter : ObjectBase<SolveIter> {
    Gringo::SolveIter *solve_iter;
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "SolveIter";
    static constexpr char const *tp_name = "clingo.SolveIter";
    static constexpr char const *tp_doc =
R"(Object to conveniently iterate over all models.

During solving the GIL is released. The functions in this object are not
thread-safe though.)";

    static PyObject *new_(Gringo::SolveIter &iter) {
        SolveIter *self;
        self = reinterpret_cast<SolveIter*>(type.tp_alloc(&type, 0));
        if (!self) { return nullptr; }
        self->solve_iter = &iter;
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject* tp_iter(PyObject *self) {
        Py_INCREF(self);
        return self;
    }
    static PyObject* get(SolveIter *self) {
        PY_TRY
            return SolveResult::new_(doUnblocked([self]() { return self->solve_iter->get(); }));
        PY_CATCH(nullptr);
    }
    static PyObject* tp_iternext(SolveIter *self) {
        PY_TRY
            if (Gringo::Model const *m = doUnblocked([self]() { return self->solve_iter->next(); })) {
                return Model::new_(*m);
            } else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        PY_CATCH(nullptr);
    }
    static PyObject *enter(SolveIter *self) {
        Py_INCREF(self);
        return (PyObject*)self;
    }
    static PyObject *exit(SolveIter *self, PyObject *) {
        PY_TRY
            doUnblocked([self]() { return self->solve_iter->close(); });
            return cppToPy(false).release();
        PY_CATCH(nullptr);
    }
};

PyMethodDef SolveIter::tp_methods[] = {
    {"__enter__",      (PyCFunction)enter,      METH_NOARGS,
R"(__enter__(self) -> SolveIter

Returns self.)"},
    {"get",            (PyCFunction)get,        METH_NOARGS,
R"(get(self) -> SolveResult

Return the result of the search.

Note that this function might start a search for the next model and then return
a result accordingly. The function might be called after iteration to check if
the search has been interrupted.)"},
    {"__exit__",       (PyCFunction)exit,       METH_VARARGS,
R"(__exit__(self, type, value, traceback) -> bool

Follows python __exit__ conventions. Does not suppress exceptions.

Stops the current search. It is necessary to call this method after each search.)"},
    {nullptr, nullptr, 0, nullptr}
};

// {{{1 wrap Configuration

struct Configuration : ObjectBase<Configuration> {
    unsigned key;
    int nSubkeys;
    int arrLen;
    int nValues;
    char const* help;
    Gringo::ConfigProxy *proxy;
    static PyGetSetDef tp_getset[];
    static PySequenceMethods tp_as_sequence[];

    static constexpr char const *tp_type = "Configuration";
    static constexpr char const *tp_name = "clingo.Configuration";
    static constexpr char const *tp_doc =
R"(Allows for changing the configuration of the underlying solver.

Options are organized hierarchically. To change and inspect an option use:

  config.group.subgroup.option = "value"
  value = config.group.subgroup.option

There are also arrays of option groups that can be accessed using integer
indices:

  config.group.subgroup[0].option = "value1"
  config.group.subgroup[1].option = "value2"

To list the subgroups of an option group, use the keys member. Array option
groups, like solver, have a non-negative length and can be iterated.
Furthermore, there are meta options having key "configuration". Assigning a
meta option sets a number of related options.  To get further information about
an option or option group <opt>, use property __desc_<opt> to retrieve a
description.

Example:

#script (python)
import clingo

def main(prg):
    prg.conf.solve.models = 0
    prg.ground([("base", [])])
    prg.solve()

#end.

{a; c}.

Expected Answer Sets:

{ {}, {a}, {c}, {a,c} })";

    static PyObject *new_(unsigned key, Gringo::ConfigProxy &proxy) {
        PY_TRY
            Object ret(type.tp_alloc(&type, 0));
            if (!ret) { return nullptr; }
            Configuration *self = reinterpret_cast<Configuration*>(ret.get());
            self->proxy = &proxy;
            self->key   = key;
            self->proxy->getKeyInfo(self->key, &self->nSubkeys, &self->arrLen, &self->help, &self->nValues);
            return ret.release();
        PY_CATCH(nullptr);
    }

    static PyObject *keys(Configuration *self, void *) {
        PY_TRY
            if (self->nSubkeys < 0) { Py_RETURN_NONE; }
            else {
                Object list = PyList_New(self->nSubkeys);
                for (int i = 0; i < self->nSubkeys; ++i) {
                    char const *key = self->proxy->getSubKeyName(self->key, i);
                    Object pyString = PyString_FromString(key);
                    if (PyList_SetItem(list, i, pyString.release()) < 0) { return nullptr; }
                }
                return list.release();
            }
        PY_CATCH(nullptr);
    }

    static PyObject *tp_getattro(Configuration *self, PyObject *name) {
        PY_TRY
            auto current = pyToCpp<char const *>(name);
            bool desc = strncmp("__desc_", current, 7) == 0;
            if (desc) { current += 7; }
            unsigned key;
            if (self->proxy->hasSubKey(self->key, current, &key)) {
                Object subKey(new_(key, *self->proxy));
                Configuration *sub = reinterpret_cast<Configuration*>(subKey.get());
                if (desc) { return PyString_FromString(sub->help); }
                else if (sub->nValues < 0) { return subKey.release(); }
                else {
                    std::string value;
                    if (!sub->proxy->getKeyValue(sub->key, value)) { Py_RETURN_NONE; }
                    return PyString_FromString(value.c_str());
                }
            }
            return PyObject_GenericGetAttr(reinterpret_cast<PyObject*>(self), name);
        PY_CATCH(nullptr);
    }

    static int tp_setattro(Configuration *self, PyObject *name, PyObject *pyValue) {
        PY_TRY
            char const *current = PyString_AsString(name);
            if (!current) { return -1; }
            unsigned key;
            if (self->proxy->hasSubKey(self->key, current, &key)) {
                Object pyStr(PyObject_Str(pyValue));
                if (!pyStr) { return -1; }
                char const *value = PyString_AsString(pyStr);
                if (!value) { return -1; }
                self->proxy->setKeyValue(key, value);
                return 0;
            }
            return PyObject_GenericSetAttr(reinterpret_cast<PyObject*>(self), name, pyValue);
        PY_CATCH(-1);
    }

    static Py_ssize_t length(Configuration *self) {
        return self->arrLen;
    }

    static PyObject* item(Configuration *self, Py_ssize_t index) {
        PY_TRY
            if (index < 0 || index >= self->arrLen) {
                PyErr_Format(PyExc_IndexError, "invalid index");
                return nullptr;
            }
            return new_(self->proxy->getArrKey(self->key, index), *self->proxy);
        PY_CATCH(nullptr);
    }
};

PyGetSetDef Configuration::tp_getset[] = {
    // keys
    {(char *)"keys", (getter)keys, nullptr,
(char *)R"(The list of names of sub-option groups or options.

The list is None if the current object is not an option group.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PySequenceMethods Configuration::tp_as_sequence[] = {{
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
}};

// {{{1 wrap SymbolicAtom

struct SymbolicAtom : public ObjectBase<SymbolicAtom> {
    Gringo::SymbolicAtoms *atoms;
    Gringo::SymbolicAtomIter range;

    static constexpr char const *tp_type = "SymbolicAtom";
    static constexpr char const *tp_name = "clingo.SymbolicAtom";
    static constexpr char const *tp_doc = "Captures a symbolic atom and provides properties to inspect its state.";
    static PyGetSetDef tp_getset[];

    static PyObject *new_(Gringo::SymbolicAtoms &atoms, Gringo::SymbolicAtomIter range) {
        Object ret(type.tp_alloc(&type, 0));
        SymbolicAtom *self = reinterpret_cast<SymbolicAtom*>(ret.get());
        self->atoms = &atoms;
        self->range = range;
        return ret.release();
    }
    static PyObject *symbol(SymbolicAtom *self, void *) {
        PY_TRY
            return Term::new_(self->atoms->atom(self->range));
        PY_CATCH(nullptr);
    }
    static PyObject *literal(SymbolicAtom *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->atoms->literal(self->range));
        PY_CATCH(nullptr);
    }
    static PyObject *is_fact(SymbolicAtom *self, void *) {
        PY_TRY
            return cppToPy(self->atoms->fact(self->range)).release();
        PY_CATCH(nullptr);
    }
    static PyObject *is_external(SymbolicAtom *self, void *) {
        PY_TRY
            return cppToPy(self->atoms->external(self->range)).release();
        PY_CATCH(nullptr);
    }
};

PyGetSetDef SymbolicAtom::tp_getset[] = {
    {(char *)"symbol", (getter)symbol, nullptr, (char *)R"(The representation of the atom in form of a term (Term object).)", nullptr},
    {(char *)"literal", (getter)literal, nullptr, (char *)R"(The program literal associated with the atom.)", nullptr},
    {(char *)"is_fact", (getter)is_fact, nullptr, (char *)R"(Wheather the atom is a is_fact.)", nullptr},
    {(char *)"is_external", (getter)is_external, nullptr, (char *)R"(Wheather the atom is an external atom.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr},
};

// {{{1 wrap SymbolicAtomIter

struct SymbolicAtomIter : ObjectBase<SymbolicAtomIter> {
    SymbolicAtoms *atoms;
    Gringo::SymbolicAtomIter range;

    static constexpr char const *tp_type = "SymbolicAtomIter";
    static constexpr char const *tp_name = "clingo.SymbolicAtomIter";
    static constexpr char const *tp_doc = "Class to iterate over symbolic atoms.";

    static PyObject *new_(Gringo::SymbolicAtoms &atoms, Gringo::SymbolicAtomIter range) {
        Object ret(type.tp_alloc(&type, 0));
        SymbolicAtomIter *self = reinterpret_cast<SymbolicAtomIter*>(ret.get());
        self->atoms = &atoms;
        self->range = range;
        return ret.release();
    }
    static PyObject* tp_iter(SymbolicAtomIter *self) {
        Py_XINCREF(self);
        return reinterpret_cast<PyObject*>(self);
    }
    static PyObject* tp_iternext(SymbolicAtomIter *self) {
        PY_TRY
            Gringo::SymbolicAtomIter current = self->range;
            if (self->atoms->valid(current)) {
                self->range = self->atoms->next(current);
                return SymbolicAtom::new_(*self->atoms, current);
            }
            else {
                PyErr_SetNone(PyExc_StopIteration);
                return nullptr;
            }
        PY_CATCH(nullptr);
    }
};

// {{{1 wrap SymbolicAtoms

struct SymbolicAtoms : ObjectBase<SymbolicAtoms> {
    Gringo::SymbolicAtoms *atoms;
    static PyMethodDef tp_methods[];
    static PyMappingMethods tp_as_mapping[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "Domain";
    static constexpr char const *tp_name = "clingo.Domain";
    static constexpr char const *tp_doc =
R"(This class provides read-only access to the symbolic atoms of the grounder
(the Herbrand base).

Example:

p(1).
{ p(3) }.
#external p(1..3).

q(X) :- p(X).

#script (python)

import clingo

def main(prg):
    prg.ground([("base", [])])
    print "universe:", len(prg.symbolic_atoms)
    for x in prg.symbolic_atoms:
        print x.atom, x.is_fact, x.is_external
    print "p(2) is in domain:", prg.symbolic_atoms[clingo.function("p", [3])] is not None
    print "p(4) is in domain:", prg.symbolic_atoms[clingo.function("p", [6])] is not None
    print "domain of p/1:"
    for x in prg.symbolic_atoms.by_signature(("p", 1)):
        print x.atom, x.is_fact, x.is_external
    print "signatures:", prg.symbolic_atoms.signatures

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
signatures: [('p', 1), ('q', 1)])";

    static PyObject *new_(Gringo::SymbolicAtoms &atoms) {
        Object ret(type.tp_alloc(&type, 0));
        if (!ret) { return nullptr; }
        SymbolicAtoms *self = reinterpret_cast<SymbolicAtoms*>(ret.get());
        self->atoms = &atoms;
        return ret.release();
    }

    static Py_ssize_t length(SymbolicAtoms *self) {
        return self->atoms->length();
    }

    static PyObject* tp_iter(SymbolicAtoms *self) {
        PY_TRY
            return SymbolicAtomIter::new_(*self->atoms, self->atoms->begin());
        PY_CATCH(nullptr);
    }

    static PyObject* subscript(SymbolicAtoms *self, PyObject *key) {
        PY_TRY
            Gringo::Symbol atom;
            pyToCpp(key, atom);
            Gringo::SymbolicAtomIter range = self->atoms->lookup(atom);
            if (self->atoms->valid(range)) { return SymbolicAtom::new_(*self->atoms, range); }
            else                           { Py_RETURN_NONE; }
        PY_CATCH(nullptr);
    }

    static PyObject* by_signature(SymbolicAtoms *self, PyObject *pyargs) {
        PY_TRY
            char const *name;
            int arity;
            if (!PyArg_ParseTuple(pyargs, "si", &name, &arity)) { return nullptr; }
            Gringo::SymbolicAtomIter range = self->atoms->begin(Sig(name, arity, false));
            return SymbolicAtomIter::new_(*self->atoms, range);
        PY_CATCH(nullptr);
    }

    static PyObject* signatures(SymbolicAtoms *self, void *) {
        PY_TRY
            auto ret = self->atoms->signatures();
            Object pyRet = PyList_New(ret.size());
            int i = 0;
            for (auto &sig : ret) {
                Object pySig = Py_BuildValue("(si)", sig.name().c_str(), (int)sig.arity());
                if (PyList_SetItem(pyRet, i, pySig.release()) < 0) { return nullptr; }
                ++i;
            }
            return pyRet.release();
        PY_CATCH(nullptr);
    }
};

PyMethodDef SymbolicAtoms::tp_methods[] = {
    {"by_signature", (PyCFunction)by_signature, METH_VARARGS,
R"(by_signature(self, name, arity) -> SymbolicAtomIter

Return an iterator over the symbolic atoms with the given signature.

Arguments:
name  -- the name of the signature
arity -- the arity of the signature
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyMappingMethods SymbolicAtoms::tp_as_mapping[] = {{
    (lenfunc)length,
    (binaryfunc)subscript,
    nullptr,
}};

PyGetSetDef SymbolicAtoms::tp_getset[] = {
    {(char *)"signatures", (getter)signatures, nullptr, (char *)
R"(The list of predicate signatures (pairs of names and arities) occurring in
the program.)"
    , nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap PropagateInit

struct PropagateInit : ObjectBase<PropagateInit> {
    Gringo::PropagateInit *init;
    static constexpr char const *tp_type = "PropagateInit";
    static constexpr char const *tp_name = "clingo.PropagateInit";
    static constexpr char const *tp_doc = R"(
Object that is used to initialize a propagator before each solving step.

Each symbolic or theory atom is uniquely associated with a positive program
atom in form of a positive integer.  Program literals additionally have a sign
to represent default negation.  Furthermore, there are non-zero integer solver
literals.  There is a surjective mapping from program atoms to solver literals.

All methods called during propagation use solver literals whereas
SymbolicAtom.literal() and TheoryAtom.literal() return program literals.  The
function PropagateInit.solver_literal() can be used to map program literals to
solver literals.)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    using ObjectBase<PropagateInit>::new_;
    static PyObject *construct(Gringo::PropagateInit &init) {
        PropagateInit *self = new_();
        self->init = &init;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *theoryIter(PropagateInit *self, void *) {
        return TheoryAtomIter::new_(&self->init->theory(), 0);
    }

    static PyObject *symbolicAtoms(PropagateInit *self, void *) {
        return SymbolicAtoms::new_(self->init->getDomain());
    }

    static PyObject *numThreads(PropagateInit *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->init->threads());
        PY_CATCH(nullptr);
    }

    static PyObject *mapLit(PropagateInit *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            Lit_t r = 0;
            r = self->init->mapLit(l);
            return PyInt_FromLong(r);
        PY_CATCH(nullptr);
    }

    static PyObject *addWatch(PropagateInit *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            self->init->addWatch(l);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
};

PyMethodDef PropagateInit::tp_methods[] = {
    {"add_watch", (PyCFunction)addWatch, METH_O, R"(add_watch(self, lit) -> None

Add a watch for the solver literal in the given phase.)"},
    {"solver_literal", (PyCFunction)mapLit, METH_O, R"(solver_literal(self, lit) -> int

Map the given program literal to its solver literal.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef PropagateInit::tp_getset[] = {
    {(char *)"symbolic_atoms", (getter)symbolicAtoms, nullptr, (char *)R"(The symbolic atoms captured by a SymbolicAtoms object.)", nullptr},
    {(char *)"theory_atoms", (getter)theoryIter, nullptr, (char *)R"(A TheoryAtomIter object to iterate over all theory atoms.)", nullptr},
    {(char *)"num_threads", (getter)numThreads, nullptr, (char *) R"(The number of solver threads used in the corresponding solve call.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Assignment

struct Assignment : ObjectBase<Assignment> {
    Potassco::AbstractAssignment const *assign;
    static constexpr char const *tp_type = "Assignment";
    static constexpr char const *tp_name = "clingo.Assignment";
    static constexpr char const *tp_doc = R"(Object to inspect the (parital) assignment of an associated solver.

Assigns truth values to solver literals.  Each solver literal is either true,
false, or undefined, represented by the python constants True, False, or None,
respectively.)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static PyObject *construct(Potassco::AbstractAssignment const &assign) {
        Assignment *self = new_();
        self->assign = &assign;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *hasConflict(Assignment *self) {
        PY_TRY
            return cppToPy(self->assign->hasConflict()).release();
        PY_CATCH(nullptr);
    }

    static PyObject *decisionLevel(Assignment *self, void *) {
        PY_TRY
            return PyInt_FromLong(self->assign->level());
        PY_CATCH(nullptr);
    }

    static PyObject *hasLit(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            return cppToPy(self->assign->hasLit(l)).release();
        PY_CATCH(nullptr);
    }

    static PyObject *level(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            return PyInt_FromLong(self->assign->level(l));
        PY_CATCH(nullptr);
    }

    static PyObject *decision(Assignment *self, PyObject *level) {
        PY_TRY
            auto l = pyToCpp<uint32_t>(level);
            return PyInt_FromLong(self->assign->decision(l));
        PY_CATCH(nullptr);
    }

    static PyObject *isFixed(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            return cppToPy(self->assign->isFixed(l)).release();
        PY_CATCH(nullptr);
    }

    static PyObject *truthValue(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            Potassco::Value_t val = self->assign->value(l);
            if (val == Potassco::Value_t::False){ return cppToPy(false).release(); }
            if (val == Potassco::Value_t::True) { return cppToPy(true).release(); }
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }

    static PyObject *isTrue(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            return cppToPy(self->assign->isTrue(l)).release();
        PY_CATCH(nullptr);
    }

    static PyObject *isFalse(Assignment *self, PyObject *lit) {
        PY_TRY
            auto l = pyToCpp<Lit_t>(lit);
            return cppToPy(self->assign->isFalse(l)).release();
        PY_CATCH(nullptr);
    }
};

PyMethodDef Assignment::tp_methods[] = {
    {"has_lit", (PyCFunction)hasLit, METH_O, R"(has_lit(self, lit) -> bool

Determine if the literal is valid in this solver.)"},
    {"value", (PyCFunction)truthValue, METH_O, R"(value(self, lit) -> bool or None

The truth value of the given literal or None if it has none.)"},
    {"level", (PyCFunction)level, METH_O, R"(level(self, lit) -> int

The decision level of the given literal.

Note that the returned value is only meaningful if the literal is assigned -
i.e., value(lit) is not None.)"},
    {"is_fixed", (PyCFunction)isFixed, METH_O, R"(is_fixed(self, lit) -> bool

Determine if the literal is assigned on the top level.)"},
    {"is_true", (PyCFunction)isTrue, METH_O, R"(is_true(self, lit) -> bool

Determine if the literal is true.)"},
    {"is_false", (PyCFunction)isFalse, METH_O, R"(is_false(self, lit) -> bool

Determine if the literal is false.)"},
    {"decision", (PyCFunction)decision, METH_O, R"(decision(self, level) -> int

    Return the decision literal of the given level.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef Assignment::tp_getset[] = {
    {(char *)"has_conflict", (getter)hasConflict, nullptr, (char *)R"(True if the current assignment is conflicting.)", nullptr},
    {(char *)"decision_level", (getter)decisionLevel, nullptr, (char *)R"(The current decision level.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap PropagateControl

struct PropagateControl : ObjectBase<PropagateControl> {
    Potassco::AbstractSolver* ctl;
    static constexpr char const *tp_type = "PropagateControl";
    static constexpr char const *tp_name = "clingo.PropagateControl";
    static constexpr char const *tp_doc = "This object can be used to add clauses and propagate literals.";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    using ObjectBase<PropagateControl>::new_;
    static PyObject *construct(Potassco::AbstractSolver &ctl) {
        PropagateControl *self = new_();
        self->ctl = &ctl;
        return reinterpret_cast<PyObject*>(self);
    }

    static PyObject *id(PropagateControl *self, void *) {
        return PyInt_FromLong(self->ctl->id());
    }

    static PyObject *addClauseOrNogood(PropagateControl *self, PyObject *pyargs, PyObject *pykwds, bool invert) {
        PY_TRY
            static char const *kwlist[] = {"clause", "tag", "lock", nullptr};
            PyObject *clause;
            PyObject *pyTag = Py_False;
            PyObject *pyLock = Py_False;
            if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "O|OO", const_cast<char**>(kwlist), &clause, &pyTag, &pyLock)) { return nullptr; }
            auto lits = pyToCpp<std::vector<Lit_t>>(clause);
            if (invert) {
                for (auto &lit : lits) { lit = -lit; }
            }
            unsigned type = 0;
            if (pyToCpp<bool>(pyTag))  { type |= Potassco::Clause_t::Volatile; }
            if (pyToCpp<bool>(pyLock)) { type |= Potassco::Clause_t::Static; }
            return cppToPy(doUnblocked([self, &lits, type](){ return self->ctl->addClause(Potassco::toSpan(lits), static_cast<Potassco::Clause_t>(type)); })).release();
        PY_CATCH(nullptr);
    }

    static PyObject *addNogood(PropagateControl *self, PyObject *pyargs, PyObject *pykwds) {
        return addClauseOrNogood(self, pyargs, pykwds, true);
    }

    static PyObject *addClause(PropagateControl *self, PyObject *pyargs, PyObject *pykwds) {
        return addClauseOrNogood(self, pyargs, pykwds, false);
    }

    static PyObject *propagate(PropagateControl *self) {
        PY_TRY
            return cppToPy(doUnblocked([self](){ return self->ctl->propagate(); })).release();
        PY_CATCH(nullptr);
    }

    static PyObject *assignment(PropagateControl *self, void *) {
        return Assignment::construct(self->ctl->assignment());
    }
};

PyMethodDef PropagateControl::tp_methods[] = {
    {"add_clause", (PyCFunction)addClause, METH_KEYWORDS | METH_VARARGS, R"(add_clause(self, clause, tag, lock) -> bool

Add the given clause to the solver.

This method returns False if the current propagation must be stopped.

Arguments:
clause -- sequence of solver literals

Keyword Arguments:
tag  -- clause applies only in the current solving step
        (Default: False)
lock -- exclude clause from the solver's regular clause deletion policy
        (Default: False))"},
    {"add_nogood", (PyCFunction)addNogood, METH_KEYWORDS | METH_VARARGS, R"(add_nogood(self, clause, tag, lock) -> bool
Equivalent to self.add_clause([-lit for lit in clause], tag, lock).)"},
    {"propagate", (PyCFunction)propagate, METH_NOARGS, R"(propagate(self) -> bool

Propagate implied literals.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef PropagateControl::tp_getset[] = {
    {(char *)"thread_id", (getter)id, nullptr, (char *)R"(The numeric id of the current solver thread.)", nullptr},
    {(char *)"assignment", (getter)assignment, nullptr, (char *)R"(The partial assignment of the current solver thread.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Propagator

class Propagator : public Gringo::Propagator {
public:
    Propagator(PyObject *tp) : tp_(tp, true) {}
    void init(Gringo::PropagateInit &init) override {
        PyBlock block;
        PY_TRY
            Object i = PropagateInit::construct(init);
            Object n = PyString_FromString("init");
            Object ret = PyObject_CallMethodObjArgs(tp_, n.get(), i.get(), nullptr);
        PY_HANDLE("Propagator::init", "error during initialization")
    }
    void propagate(Potassco::AbstractSolver &solver, Potassco::LitSpan const &changes) override {
        PyBlock block;
        PY_TRY
            if (!PyObject_HasAttrString(tp_, "propagate")) { return; }
            Object c = PropagateControl::construct(solver);
            Object l = cppToPy(changes);
            Object n = PyString_FromString("propagate");
            Object ret = PyObject_CallMethodObjArgs(tp_, n, c.get(), l.get(), nullptr);
        PY_HANDLE("Propagator::propagate", "error during propagation")
    }
    void undo(Potassco::AbstractSolver const &solver, Potassco::LitSpan const &undo) override {
        PyBlock block;
        PY_TRY
            if (!PyObject_HasAttrString(tp_, "undo")) { return; }
            Object i = PyInt_FromLong(solver.id());
            Object a = Assignment::construct(solver.assignment());
            Object l = cppToPy(undo);
            Object n = PyString_FromString("undo");
            Object ret = PyObject_CallMethodObjArgs(tp_, n, i.get(), a.get(), l.get(), nullptr);
        PY_HANDLE("Propagator::undo", "error during undo")
    }
    void check(Potassco::AbstractSolver &solver) override {
        PyBlock block;
        PY_TRY
            if (!PyObject_HasAttrString(tp_, "check")) { return; }
            Object c = PropagateControl::construct(solver);
            Object n = PyString_FromString("check");
            Object ret = PyObject_CallMethodObjArgs(tp_, n, c.get(), nullptr);
        PY_HANDLE("Propagator::check", "error during check")
    }
    ~Propagator() noexcept = default;
private:
    Object tp_;
};

// {{{1 wrap wrap Backend

struct Backend : ObjectBase<Backend> {
    Gringo::Control *ctl;
    Gringo::Backend *backend;

    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "Backend";
    static constexpr char const *tp_name = "clingo.Backend";
    static constexpr char const *tp_doc =
    R"(Backend object providing a low level interface to extend a logic program."

This class provides an interface that allows for adding statements in ASPIF
format.)";

    static PyObject *new_(Gringo::Control &ctl) {
        PY_TRY
            auto *backend = ctl.backend();
            if (!backend) {
                PyErr_Format(PyExc_RuntimeError, "backend not available");
                return nullptr;
            }
            Object ret(type.tp_alloc(&type, 0));
            Backend *self = reinterpret_cast<Backend*>(ret.get());
            self->ctl = &ctl;
            self->backend = backend;
            return ret.release();
        PY_CATCH(nullptr);
    }

    static PyObject *addAtom(Backend *self) {
        PY_TRY
            return PyInt_FromLong(self->ctl->addProgramAtom());
        PY_CATCH(nullptr);
    }

    static PyObject *addRule(Backend *self, PyObject *pyargs, PyObject *pykwds) {
        PY_TRY
            static char const *kwlist[] = {"head", "body", "choice", nullptr};
            PyObject *pyHead = nullptr;
            PyObject *pyBody = nullptr;
            PyObject *pyChoice = Py_False;
            if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "O|OO", const_cast<char**>(kwlist), &pyHead, &pyBody, &pyChoice)) { return nullptr; }
            Gringo::Backend::AtomVec head;
            pyToCpp(pyHead, head);
            Gringo::Backend::LitVec body;
            if (pyBody) { pyToCpp(pyBody, body); }
            bool choice = pyToCpp<bool>(pyChoice);
            self->backend->printHead(choice, head);
            self->backend->printNormalBody(body);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }

    static PyObject *addWeightRule(Backend *self, PyObject *pyargs, PyObject *pykwds) {
        PY_TRY
            static char const *kwlist[] = {"head", "lower", "body", "choice", nullptr};
            PyObject *pyHead = nullptr;
            PyObject *pyLower = nullptr;
            PyObject *pyBody = nullptr;
            PyObject *pyChoice = Py_False;
            if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "OOO|O", const_cast<char**>(kwlist), &pyHead, &pyLower, &pyBody, &pyChoice)) { return nullptr; }
            auto head = pyToCpp<Gringo::Backend::AtomVec>(pyHead);
            auto lower = pyToCpp<Potassco::Weight_t>(pyLower);
            auto body = pyToCpp<Gringo::Backend::LitWeightVec>(pyBody);
            auto choice = pyToCpp<bool>(pyChoice);
            self->backend->printHead(choice, head);
            self->backend->printWeightBody(lower, body);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
};

PyMethodDef Backend::tp_methods[] = {
    // add_atom
    {"add_atom", (PyCFunction)addAtom, METH_NOARGS,
R"(add_atom(self) -> Int

Return a fresh program atom.)"},
    // add_rule
    {"add_rule", (PyCFunction)addRule, METH_VARARGS | METH_KEYWORDS,
R"(add_rule(self, head, body, choice) -> None

Add a disjuntive or choice rule to the program.

Arguments:
head -- list of program atoms

Keyword Arguments:
body   -- list of program literals (Default: [])
choice -- whether to add a disjunctive or choice rule (Default: False)

Integrity constraints and normal rules can be added by using an empty or
singleton head list, respectively.)"},
    // add_weight_rule
    {"add_weight_rule", (PyCFunction)addWeightRule, METH_VARARGS | METH_KEYWORDS,
R"(add_weight_rule(self, head, lower, body, choice) -> None
Add a disjuntive or choice rule with one weight constraint with a lower bound
in the body to the program.

Arguments:
head  -- list of program atoms
lower -- integer for the lower bound
body  -- list of pairs of program literals and weights

Keyword Arguments:
choice -- whether to add a disjunctive or choice rule (Default: False)
)"},
    {nullptr, nullptr, 0, nullptr}
};

// {{{1 wrap Control

void pycall(PyObject *fun, SymSpan args, SymVec &vals) {
    Object params = PyTuple_New(args.size);
    int i = 0;
    for (auto &val : args) {
        Object pyVal = Term::new_(val);
        if (PyTuple_SetItem(params, i, pyVal.release()) < 0) { throw PyException(); }
        ++i;
    }
    Object ret = PyObject_Call(fun, params, Py_None);
    if (PyList_Check(ret)) { pyToCpp(ret, vals); }
    else { vals.emplace_back(pyToCpp<Symbol>(ret)); }
}

class PyContext : public Context {
public:
    PyContext(Logger &log)
    : log(log)
    , ctx(nullptr) { }
    bool callable(String name) const override {
        return ctx && PyObject_HasAttrString(ctx, name.c_str());
    }
    SymVec call(Location const &loc, String name, SymSpan args) override {
        assert(ctx);
        try {
            Object fun = PyObject_GetAttrString(ctx, name.c_str());
            SymVec ret;
            pycall(fun, args, ret);
            return ret;
        }
        catch (PyException const &) {
            GRINGO_REPORT(log, clingo_warning_operation_undefined)
                << loc << ": info: operation undefined:\n"
                << errorToString()
                ;
            return {};
        }
    }
    operator bool() const { return ctx; }
    virtual ~PyContext() noexcept = default;

    Logger &log;
    PyObject *ctx;
};

struct ControlWrap : ObjectBase<ControlWrap> {
    using Propagators = std::vector<std::unique_ptr<Propagator>>;
    Gringo::Control *ctl;
    Gringo::Control *freeCtl;
    PyObject        *stats;
    Propagators      propagators_;

    static PyGetSetDef tp_getset[];
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "Control";
    static constexpr char const *tp_name = "clingo.Control";
    static constexpr char const *tp_doc =
    R"(Control(args) -> Control

Control object for the grounding/solving process.

Arguments:
args -- optional arguments to the grounder and solver (default: []).

Note that only gringo options (without --text) and clasp's search options are
supported. Furthermore, a Control object is blocked while a search call is
active; you must not call any member function during search.)";

    static void checkBlocked(ControlWrap *self, char const *function) {
        if (self->ctl->blocked()) {
            PyErr_Format(PyExc_RuntimeError, "Control.%s must not be called during solve call", function);
            throw PyException();
        }
    }
    static PyObject *new_(Gringo::Control &ctl) {
        PyObject *self = tp_new(&type, nullptr, nullptr);
        if (!self) { return nullptr; }
        reinterpret_cast<ControlWrap*>(self)->ctl = &ctl;
        return self;
    }
    static Gringo::GringoModule *module;
    static PyObject *tp_new(PyTypeObject *type, PyObject *, PyObject *) {
        ControlWrap *self;
        self = reinterpret_cast<ControlWrap*>(type->tp_alloc(type, 0));
        if (!self) { return nullptr; }
        self->ctl     = nullptr;
        self->freeCtl = nullptr;
        self->stats   = nullptr;
        new (&self->propagators_) Propagators();
        return reinterpret_cast<PyObject*>(self);
    }
    static void tp_dealloc(ControlWrap *self) {
        if (self->freeCtl) { delete self->freeCtl; }
        self->ctl = self->freeCtl = nullptr;
        Py_XDECREF(self->stats);
        self->propagators_.~Propagators();
        type.tp_free(self);
    }
    static int tp_init(ControlWrap *self, PyObject *pyargs, PyObject *pykwds) {
        PY_TRY
            static char const *kwlist[] = {"args", nullptr};
            PyObject *params = nullptr;
            if (!PyArg_ParseTupleAndKeywords(pyargs, pykwds, "|O", const_cast<char**>(kwlist), &params)) { return -1; }
            std::vector<char const *> args;
            args.emplace_back("clingo");
            if (params) {
                Object it = PyObject_GetIter(params);
                if (!it) { return -1; }
                while (Object pyVal = PyIter_Next(it)) {
                    char const *x = PyString_AsString(pyVal);
                    if (!x) { return -1; }
                    args.emplace_back(x);
                }
                if (PyErr_Occurred()) { return -1; }
            }
            args.emplace_back(nullptr);
            self->ctl = self->freeCtl = module->newControl(args.size(), args.data(), nullptr, 20);
            return 0;
        PY_CATCH(-1);
    }
    static PyObject *add(ControlWrap *self, PyObject *args) {
        PY_TRY
            checkBlocked(self, "add");
            char     *name;
            PyObject *pyParams;
            char     *part;
            if (!PyArg_ParseTuple(args, "sOs", &name, &pyParams, &part)) { return nullptr; }
            FWStringVec params;
            Object it = PyObject_GetIter(pyParams);
            while (Object pyVal = PyIter_Next(it)) {
                auto val = pyToCpp<char const *>(pyVal);
                params.emplace_back(val);
            }
            self->ctl->add(name, params, part);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *load(ControlWrap *self, PyObject *args) {
        PY_TRY
            checkBlocked(self, "load");
            char *filename;
            if (!PyArg_ParseTuple(args, "s", &filename)) { return nullptr; }
            if (!filename) { return nullptr; }
            self->ctl->load(filename);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *ground(ControlWrap *self, PyObject *args, PyObject *kwds) {
        PY_TRY
            checkBlocked(self, "ground");
            Gringo::Control::GroundVec parts;
            static char const *kwlist[] = {"parts", "context", nullptr};
            PyObject *pyParts;
            PyContext context(self->ctl->logger());
            if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", const_cast<char**>(kwlist), &pyParts, &context.ctx)) { return nullptr; }
            Object it = PyObject_GetIter(pyParts);
            while (Object pyVal = PyIter_Next(it)) {
                Object jt = PyObject_GetIter(pyVal);
                Object pyName = PyIter_Next(jt);
                if (!pyName) { return PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
                Object pyArgs = PyIter_Next(jt);
                if (!pyArgs) { return PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
                Object pyNext = PyIter_Next(jt);
                if (pyNext) { return PyErr_Format(PyExc_RuntimeError, "tuple of name and arguments expected"); }
                auto name = pyToCpp<char const *>(pyName);
                auto args = pyToCpp<SymVec>(pyArgs);
                parts.emplace_back(name, args);
            }
            // anchor
            self->ctl->ground(parts, context ? &context : nullptr);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *getConst(ControlWrap *self, PyObject *args) {
        PY_TRY
            checkBlocked(self, "get_const");
            char *name;
            if (!PyArg_ParseTuple(args, "s", &name)) { return nullptr; }
            Symbol val;
            val = self->ctl->getConst(name);
            if (val.type() == SymbolType::Special) { Py_RETURN_NONE; }
            else { return Term::new_(val); }
        PY_CATCH(nullptr);
    }
    static bool on_model(Gringo::Model const &m, PyObject *mh) {
        PY_TRY
            Object model(Model::new_(m));
            Object ret = PyObject_CallFunction(mh, const_cast<char*>("O"), model.get());
            if (ret == Py_None) { return true; }
            else                { return pyToCpp<bool>(ret); }
        PY_HANDLE("<on_model>", "error in model callback");

    }
    static void on_finish(Gringo::SolveResult ret, PyObject *fh) {
        PY_TRY
            Object pyRet = SolveResult::new_(ret);
            Object fhRet = PyObject_CallFunction(fh, const_cast<char*>("O"), pyRet.get());
        PY_HANDLE("<on_finish>", "error in finish callback");
    }
    static bool getAssumptions(PyObject *pyAss, Gringo::Control::Assumptions &ass) {
        PY_TRY
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
                    ass.emplace_back(pyToCpp<Symbol>(pyAtom), pyToCpp<bool>(pyBool));
                }
                if (PyErr_Occurred()) { return false; }
            }
            return true;
        PY_CATCH(false);
    }
    static PyObject *solve_async(ControlWrap *self, PyObject *args, PyObject *kwds) {
        PY_TRY
            checkBlocked(self, "solve_async");
            Py_XDECREF(self->stats);
            self->stats = nullptr;
            static char const *kwlist[] = {"on_model", "on_finish", "assumptions", nullptr};
            PyObject *pyAss = nullptr, *mh = Py_None, *fh = Py_None;
            if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOO", const_cast<char **>(kwlist), &mh, &fh, &pyAss)) { return nullptr; }
            Gringo::Control::Assumptions ass;
            if (!getAssumptions(pyAss, ass)) { return nullptr; }
            Gringo::SolveFuture *future = self->ctl->solveAsync(
                mh == Py_None ? Control::ModelHandler(nullptr) : [mh](Gringo::Model const &m) -> bool { PyBlock b; (void)b; return on_model(m, mh); },
                fh == Py_None ? Control::FinishHandler(nullptr) : [fh](Gringo::SolveResult ret) -> void { PyBlock b; (void)b; on_finish(ret, fh); },
                std::move(ass)
            );
            return SolveFuture::new_(*future, mh, fh);
        PY_CATCH(nullptr);
    }
    static PyObject *solve_iter(ControlWrap *self, PyObject *args, PyObject *kwds) {
        PY_TRY
            checkBlocked(self, "solve_iter");
            Py_XDECREF(self->stats);
            self->stats = nullptr;
            PyObject *pyAss = nullptr;
            static char const *kwlist[] = {"assumptions", nullptr};
            if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", const_cast<char **>(kwlist), &pyAss)) { return nullptr; }
            Gringo::Control::Assumptions ass;
            if (!getAssumptions(pyAss, ass)) { return nullptr; }
            return SolveIter::new_(*self->ctl->solveIter(std::move(ass)));
        PY_CATCH(nullptr);
    }
    static PyObject *solve(ControlWrap *self, PyObject *args, PyObject *kwds) {
        PY_TRY
            checkBlocked(self, "solve");
            Py_XDECREF(self->stats);
            self->stats = nullptr;
            static char const *kwlist[] = {"on_model", "assumptions", nullptr};
            PyObject *mh = Py_None;
            PyObject *pyAss = nullptr;
            if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", const_cast<char **>(kwlist), &mh, &pyAss)) { return nullptr; }
            Gringo::Control::Assumptions ass;
            if (!getAssumptions(pyAss, ass)) { return nullptr; }
            Gringo::SolveResult ret = doUnblocked([self, mh, &ass]() {
                return self->ctl->solve(
                    mh == Py_None ? Control::ModelHandler(nullptr) : [mh](Gringo::Model const &m) { PyBlock block; return on_model(m, Object(mh, true)); },
                    std::move(ass)); });
            return SolveResult::new_(ret);
        PY_CATCH(nullptr);
    }
    static PyObject *cleanup(ControlWrap *self) {
        PY_TRY
            checkBlocked(self, "cleanup");
            self->ctl->cleanupDomains();
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *assign_external(ControlWrap *self, PyObject *args) {
        PY_TRY
            checkBlocked(self, "assign_external");
            PyObject *pyExt, *pyVal;
            if (!PyArg_ParseTuple(args, "OO", &pyExt, &pyVal)) { return nullptr; }
            Potassco::Value_t val;
            if (pyVal == Py_True)       { val = Potassco::Value_t::True; }
            else if (pyVal == Py_False) { val = Potassco::Value_t::False; }
            else if (pyVal == Py_None)  { val = Potassco::Value_t::Free; }
            else {
                PyErr_Format(PyExc_RuntimeError, "unexpected %s() object as second argumet", pyVal->ob_type->tp_name);
                return nullptr;
            }
            auto ext = pyToCpp<Symbol>(pyExt);
            self->ctl->assignExternal(ext, val);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *release_external(ControlWrap *self, PyObject *args) {
        PY_TRY
            checkBlocked(self, "release_external");
            PyObject *pyExt;
            if (!PyArg_ParseTuple(args, "O", &pyExt)) { return nullptr; }
            auto ext = pyToCpp<Symbol>(pyExt);
            self->ctl->assignExternal(ext, Potassco::Value_t::Release);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *getStats(ControlWrap *self, void *) {
        PY_TRY
            checkBlocked(self, "stats");
            if (!self->stats) {
                Statistics *stats = self->ctl->getStats();
                self->stats = getStatistics(stats, "");
            }
            Py_XINCREF(self->stats);
            return self->stats;
        PY_CATCH(nullptr);
    }
    static int set_use_enum_assumption(ControlWrap *self, PyObject *pyEnable, void *) {
        PY_TRY
            checkBlocked(self, "use_enum_assumption");
            int enable = PyObject_IsTrue(pyEnable);
            if (enable < 0) { return -1; }
            self->ctl->useEnumAssumption(enable);
            return 0;
        PY_CATCH(-1);
    }
    static PyObject *get_use_enum_assumption(ControlWrap *self, void *) {
        PY_TRY
            return PyBool_FromLong(self->ctl->useEnumAssumption());
        PY_CATCH(nullptr);
    }
    static PyObject *conf(ControlWrap *self, void *) {
        PY_TRY
            Gringo::ConfigProxy &proxy = self->ctl->getConf();
            return Configuration::new_(proxy.getRootKey(), proxy);
        PY_CATCH(nullptr);
    }
    static PyObject *symbolicAtoms(ControlWrap *self, void *) {
        return SymbolicAtoms::new_(self->ctl->getDomain());
    }
    static PyObject *theoryIter(ControlWrap *self, void *) {
        PY_TRY
            checkBlocked(self, "theory_atoms");
            return TheoryAtomIter::new_(&self->ctl->theory(), 0);
        PY_CATCH(nullptr);
    }
    static PyObject *registerPropagator(ControlWrap *self, PyObject *tp) {
        PY_TRY
            self->propagators_.emplace_back(gringo_make_unique<Propagator>(tp));
            self->ctl->registerPropagator(*self->propagators_.back(), false);
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *interrupt(ControlWrap *self) {
        PY_TRY
            self->ctl->interrupt();
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
    static PyObject *backend(ControlWrap *self, void *) {
        return Backend::new_(*self->ctl);
    }
    static PyObject *parse(ControlWrap *self, PyObject *pyStr) {
        PY_TRY
            const char *str = pyToCpp<char const *>(pyStr);
            Object list = PyList_New(0);
            self->ctl->parse(str, [list](clingo_ast const &x){
                Object ast = cppToPy(x);
                if (!ast) { throw PyException(); }
                if (PyList_Append(list, ast) < 0) { throw PyException(); }
            });
            return list.release();
        PY_CATCH(nullptr);
    }
    static PyObject *addAST(ControlWrap *self, PyObject *pyAST) {
        PY_TRY
            self->ctl->add([pyAST](std::function<void (clingo_ast const &)> f) {
                Object it = PyObject_GetIter(pyAST);
                while (Object pyVal = PyIter_Next(it)) {
                    ASTOwner ast;
                    pyToCpp(pyAST, ast);
                    f(ast.root);
                }
            });
            Py_RETURN_NONE;
        PY_CATCH(nullptr);
    }
};

Gringo::GringoModule *ControlWrap::module  = nullptr;

PyMethodDef ControlWrap::tp_methods[] = {
    // add_ast
    {"add_ast", (PyCFunction)addAST, METH_O,
R"(add_ast(self, [clingo_ast]) -> None

Add the given list of abstract syntax trees to the program.

The given ASTs must have the same format as in the output of parse().  The
input must not necessarily be a dictionary.  Any object that either implements
the dictionary protocoll or has the required attributes suffices.)"},
    // parse
    {"parse", (PyCFunction)parse, METH_O,
R"(parse(self, prg) -> [clingo_ast]

Parse the program given as string to obtain list of ASTs representing the
statements in the program.

clingo_ast is dictionary of form
  { "value"    : Term
  , "location" : { "begin": Loc, "end": Loc }
  , "children" : [clingo_ast]
  }
and Loc is a dictionary of form
  { "filename" : str
  , "line"     : int
  , "column"   : int
  }.)"},
    // ground
    {"ground", (PyCFunction)ground, METH_KEYWORDS | METH_VARARGS,
R"(ground(self, parts, context) -> None

Ground the given list of program parts specified by tuples of names and arguments.

Keyword Arguments:
parts   -- list of tuples of program names and program arguments to ground
context -- context object whose methods are called during grounding using
           the @-syntax (if ommitted methods from the main module are used)

Note that parts of a logic program without an explicit #program specification
are by default put into a program called base without arguments.

Example:

#script (python)
import clingo

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
    {"get_const", (PyCFunction)getConst, METH_VARARGS,
R"(get_const(self, name) -> Term

Return the term for a constant definition of form: #const name = term.)"},
    // add
    {"add", (PyCFunction)add, METH_VARARGS,
R"(add(self, name, params, program) -> None

Extend the logic program with the given non-ground logic program in string form.

Arguments:
name    -- name of program block to add
params  -- parameters of program block
program -- non-ground program as string

Example:

#script (python)
import clingo

def main(prg):
    prg.add("p", ["t"], "q(t).")
    prg.ground([("p", [2])])
    prg.solve()

#end.

Expected Answer Set:
q(2))"},
    // load
    {"load", (PyCFunction)load, METH_VARARGS,
R"(load(self, path) -> None

Extend the logic program with a (non-ground) logic program in a file.

Arguments:
path -- path to program)"},
    // solve_async
    {"solve_async", (PyCFunction)solve_async, METH_KEYWORDS | METH_VARARGS,
R"(solve_async(self, on_model, on_finish, assumptions) -> SolveFuture

Start a search process in the background and return a SolveFuture object.

Keyword Arguments:
on_model    -- optional callback for intercepting models
               a Model object is passed to the callback
on_finish   -- optional callback called once search has finished
               a SolveResult and a Boolean indicating whether the solve call
               has been canceled is passed to the callback
assumptions -- list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(function("a"),
               True)] only admits answer sets that contain atom a

Note that this function is only available in clingo with thread support
enabled. Both the on_model and the on_finish callbacks are called from another
thread.  To ensure that the methods can be called, make sure to not use any
functions that block the GIL indefinitely. Furthermore, you might want to start
clingo using the --outf=3 option to disable all output from clingo.

Example:

#script (python)
import clingo

def on_model(model):
    print model

def on_finish(res, canceled):
    print res, canceled

def main(prg):
    prg.ground([("base", [])])
    f = prg.solve_async(on_model, on_finish)
    f.wait()

#end.

q.

Expected Output:
q
SAT False)"},
    // solve_iter
    {"solve_iter", (PyCFunction)solve_iter, METH_KEYWORDS | METH_VARARGS,
R"(solve_iter(self, assumptions) -> SolveIter

Return a SolveIter object, which can be used to iterate over models.

Keyword Arguments:
assumptions -- a list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(function("a"),
               True)] only admits answer sets that contain atom a

Example:

#script (python)
import clingo

def main(prg):
    prg.add("p", "{a;b;c}.")
    prg.ground([("p", [])])
    with prg.solve_iter() as it:
        for m in it: print m

#end.)"},
    // solve
    {"solve", (PyCFunction)solve, METH_KEYWORDS | METH_VARARGS,
R"(solve(self, on_model, assumptions) -> SolveResult

Start a search process and return a SolveResult.

Keyword Arguments:
on_model    -- optional callback for intercepting models
               a Model object is passed to the callback
assumptions -- a list of (atom, boolean) tuples that serve as assumptions for
               the solve call, e.g. - solving under assumptions [(function("a"),
               True)] only admits answer sets that contain atom a

Note that in gringo or in clingo with lparse or text output enabled this
function just grounds and returns a SolveResult where
SolveResult.satisfiability() is None. Furthermore, you might want to start
clingo using the --outf=3 option to disable all output from clingo.

This function releases the GIL but it is not thread-safe.

Take a look at Control.solve_async for an example on how to use the model
callback.)"},
    // cleanup
    {"cleanup", (PyCFunction)cleanup, METH_NOARGS,
R"(cleanup(self) -> None

Cleanup the domain used for grounding by incorporating information from the
solver.

This function cleans up the domain used for grounding.  This is done by first
simplifying the current program representation (falsifying released external
atoms).  Afterwards, the top-level implications are used to either remove atoms
from the domain or mark them as facts.

Note that any atoms falsified are completely removed from the logic program.
Hence, a definition for such an atom in a successive step introduces a fresh atom.)"},
    // assign_external
    {"assign_external", (PyCFunction)assign_external, METH_VARARGS,
R"(assign_external(self, external, truth) -> None

Assign a truth value to an external atom (represented as a term).

It is possible to assign a Boolean or None.  A Boolean fixes the external to the
respective truth value; and None leaves its truth value open.

The truth value of an external atom can be changed before each solve call. An
atom is treated as external if it has been declared using an #external
directive, and has not been forgotten by calling release_external() or defined
in a logic program with some rule. If the given atom is not external, then the
function has no effect.

Arguments:
external -- term representing the external atom
truth    -- bool or None indicating the truth value

To determine whether an atom a is external, inspect the symbolic_atoms using
SolveControl.symbolic_atoms[a].is_external. See release_external() for an
example.)"},
    // release_external
    {"release_external", (PyCFunction)release_external, METH_VARARGS,
R"(release_external(self, term) -> None

Release an external atom represented by the given term.

This function causes the corresponding atom to become permanently false if
there is no definition for the atom in the program. Otherwise, the function has
no effect.

Example:

#script (python)
from clingo import function

def main(prg):
    prg.ground([("base", [])])
    prg.assign_external(function("b"), True)
    prg.solve()
    prg.release_external(function("b"))
    prg.solve()

#end.

a.
#external b.

Expected Answer Sets:
a b
a)"},
    {"register_propagator", (PyCFunction)registerPropagator, METH_O,
R"(register_propagator(self, propagator) -> None

Registers the given propagator with all solvers.

Arguments:
propagator -- the propagator to register

A propagator should be a class of the form below. Not all functions have to be
implemented and can be ommited if not needed.

class Propagator(object)
    init(self, init) -> None
        This function is called once before each solving step.  It is used to
        map relevant program literals to solver literals, add watches for
        solver literals, and initialize the data structures used during
        propagation.

        Arguments:
        init -- PropagateInit object

        Note that this is the last point to access theory atoms.  Once the
        search has started, they are no longer accessible.

    propagate(self, control, changes) -> None
        Can be used to propagate solver literals given a partial assignment.

        Arguments:
        control -- PropagateControl object
        changes -- list of watched solver literals assigned to true

        Usage:
        Called during propagation with a non-empty list of watched solver
        literals that have been assigned to true since the last call to either
        propagate, undo, (or the start of the search) - the change set.  Only
        watched solver literals are contained in the change set.  Each literal
        in the change set is true w.r.t. the current Assignment.
        PropagateControl.add_clause can be used to add clauses.  If a clause is
        unit resulting, it can be propagated using
        PropagateControl.propagate().  If either of the two methods returns
        False, the propagate function must return immediately.

          c = ...
          if not control.add_clause(c) or not control.propagate(c):
              return

        Note that this function can be called from different solving threads.
        Each thread has its own assignment and id, which can be obtained using
        PropagateControl.id().

    undo(self, thread_id, assign, changes) -> None
        Called whenever a solver with the given id undos assignments to watched
        solver literals.

        Arguments:
        thread_id -- the solver thread id
        changes   -- list of watched solver literals whose assignment is undone

        This function is meant to update assignment dependend state in a
        propagator.

    check(self, control) -> None
        This function is similar to propagate but is only called on total
        assignments without a change set.

        Arguments:
        control -- PropagateControl object

        This function is called even if no watches have been added.)"},
    {"interrupt", (PyCFunction)interrupt, METH_NOARGS,
R"(interrupt(self) -> None

Interrupt the active solve call.

This function is thread-safe and can be called from a signal handler.  If no
search is active the subsequent call to solve(), solve_async(), or solve_iter()
is interrupted.  The SolveResult of the above solving methods can be used to
query if the search was interrupted.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef ControlWrap::tp_getset[] = {
    {(char*)"conf", (getter)conf, nullptr, (char*)"Configuration object to change the configuration.", nullptr},
    {(char*)"symbolic_atoms", (getter)symbolicAtoms, nullptr, (char*)"symbolicAtoms object to inspect the symbolic atoms.", nullptr},
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

Contains the statistics of the last solve(), solve_async(), or solve_iter()
call. The statistics correspond to the --stats output of clingo.  The detail of
the statistics depends on what level is requested on the command line.
Furthermore, you might want to start clingo using the --outf=3 option to
disable all output from clingo.

Note that this (read-only) property is only available in clingo.

Example:
import json
json.dumps(prg.stats, sort_keys=True, indent=4, separators=(',', ': ')))", nullptr},
    {(char *)"theory_atoms", (getter)theoryIter, nullptr, (char *)R"(A TheoryAtomIter object, which can be used to iterate over the theory atoms.)", nullptr},
    {(char *)"backend", (getter)backend, nullptr, (char *)R"(A Backend object providing a low level interface to extend a logic program.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap module functions

static PyObject *parseTerm(PyObject *, PyObject *objString) {
    PY_TRY
        char const *current = PyString_AsString(objString);
        Symbol value = ControlWrap::module->parseValue(current, nullptr, 20);
        if (value.type() == SymbolType::Special) { Py_RETURN_NONE; }
        else { return Term::new_(value); }
    PY_CATCH(nullptr);
}

// {{{1 gringo module

static PyMethodDef clingoMethods[] = {
    {"parse_term", (PyCFunction)parseTerm, METH_O,
R"(parse_term(s) -> term

Parse the given string using gringo's term parser for ground terms. The
function also evaluates arithmetic functions.

Example:

clingo.parse_term('p(1+2)') == clingo.function("p", [3])
)"},
    {"function", (PyCFunction)Term::new_function, METH_VARARGS | METH_KEYWORDS, R"(function(name, args, sign) -> Term

Construct a function term.

Arguments:
name -- the name of the function (empty for tuples)

Keyword Arguments:
args -- the arguments in form of a list of terms
sign -- the sign of the function (tuples must not have signs)

This includes constants and tuples. Constants have an empty argument list and
tuples have an empty name.)"},
    {"tuple_", (PyCFunction)Term::new_tuple, METH_O, R"(tuple_(args) -> Term

Shortcut for function("", args).
)"},
    {"number", (PyCFunction)Term::new_number, METH_O, R"(number(num) -> Term

Construct a numeric term given a number.)"},
    {"string", (PyCFunction)Term::new_string, METH_O, R"(string(s) -> Term

Construct a string term given a string.)"},
    {nullptr, nullptr, 0, nullptr}
};
static char const *strGrMod =
"The clingo-" GRINGO_VERSION R"( module.

This module provides functions and classes to work with ground terms and to
control the instantiation process.  In clingo builts, additional functions to
control and inspect the solving process are available.

Functions defined in a python script block are callable during the
instantiation process using @-syntax. The default grounding/solving process can
be customized if a main function is provided.

Note that gringo terms are wrapped in the Term class.  Furthermore, strings,
numbers, and tuples can be passed whereever a term is expected - they are
automatically converted into a Term object.  Functions called during the
grounding process from the logic program must either return a term or a
sequence of terms.  If a sequence is returned, the corresponding @-term is
successively substituted by the values in the sequence.

Static Objects:

__version__ -- version of the clingo module ()" GRINGO_VERSION  R"()
Inf         -- represents an #inf term
Sup         -- represents a #sup term

Functions:

function()   -- create a function term
number()     -- creat a number term
parse_term() -- parse ground terms
string()     -- create a string term
tuple_()     -- create a tuple term (shortcut)

Classes:

Assignment       -- partial assignment of truth values to solver literals
Backend          -- extend the logic program
Configuration    -- modify/inspect the solver configuration
Control          -- controls the grounding/solving process
Model            -- provides access to a model during solve call
PropagateControl -- controls running search in a custom propagator
PropagateInit    -- object to initialize custom propagators
SolveControl     -- controls running search in a model handler
SolveFuture      -- handle for asynchronous solve calls
SolveIter        -- handle to iterate over models
SolveResult      -- result of a solve call
SymbolicAtom     -- captures information about a symbolic atom
SymbolicAtomIter -- iterate over symbolic atoms
SymbolicAtoms    -- inspection of symbolic atoms
Term             -- captures ground terms
TermType         -- the type of a ground term
TheoryAtom       -- captures theory atoms
TheoryAtomIter   -- iterate over theory atoms
TheoryElement    -- captures theory elements
TheoryTerm       -- captures theory terms
TheoryTermType   -- the type of a theory term

Example:

#script (python)
import clingo
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
static struct PyModuleDef clingoModule = {
    PyModuleDef_HEAD_INIT,
    "clingo",
    strGrMod,
    -1,
    clingoMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};
#endif

PyObject *initclingo_() {
    if (!PyEval_ThreadsInitialized()) { PyEval_InitThreads(); }
#if PY_MAJOR_VERSION >= 3
    Object m = PyModule_Create(&clingoModule);
#else
    Object m = Py_InitModule3("clingo", clingoMethods, strGrMod);
#endif
    if (!m ||
        !SolveResult::initType(m)   || !TheoryTermType::initType(m)   || !PropagateControl::initType(m) ||
        !TheoryElement::initType(m) || !TheoryAtom::initType(m)       || !TheoryAtomIter::initType(m)   ||
        !Model::initType(m)         || !SolveIter::initType(m)        || !SolveFuture::initType(m)      ||
        !ControlWrap::initType(m)   || !Configuration::initType(m)    || !SolveControl::initType(m)     ||
        !SymbolicAtom::initType(m)  || !SymbolicAtomIter::initType(m) || !SymbolicAtoms::initType(m)    ||
        !TheoryTerm::initType(m)    || !PropagateInit::initType(m)    || !Assignment::initType(m)       ||
        !TermType::initType(m)      || !Term::initType(m)             || !Backend::initType(m)          ||
        PyModule_AddStringConstant(m, "__version__", GRINGO_VERSION) < 0 ||
        false) { return nullptr; }
    return m.release();
}

// }}}1

// {{{1 auxiliary functions and objects

void pyToCpp(PyObject *obj, Symbol &val) {
    if (obj->ob_type == &Term::type) { val = reinterpret_cast<Term*>(obj)->val; }
    else if (PyTuple_Check(obj))     { val = Symbol::createTuple(Potassco::toSpan(pyToCpp<SymVec>(obj))); }
    else if (PyInt_Check(obj))       { val = Symbol::createNum(pyToCpp<int>(obj)); }
    else if (PyString_Check(obj))    { val = Symbol::createStr(pyToCpp<char const *>(obj)); }
    else {
        PyErr_Format(PyExc_RuntimeError, "cannot convert to value: unexpected %s() object", obj->ob_type->tp_name);
        throw PyException();
    }
}

PyObject *cppToPy(Symbol val) {
    return Term::new_(val);
}

template <class T>
Object cppRngToPy(T begin, T end) {
    Object list = PyList_New(std::distance(begin, end));
    int i = 0;
    for (auto it = begin; it != end; ++it) {
        Object pyVal = cppToPy(*it);
        if (PyList_SetItem(list, i, pyVal.release()) < 0) { throw PyException(); }
        ++i;
    }
    return list.release();
}

template <class T>
Object cppToPy(std::vector<T> const &vals) {
    return cppRngToPy(vals.begin(), vals.end());
}

template <class T>
Object cppToPy(Potassco::Span<T> const &span) {
    return cppRngToPy(span.first, span.first + span.size);
}

// }}}1

} // namespace

// {{{1 definition of PythonImpl

struct PythonInit {
    PythonInit() : selfInit(!Py_IsInitialized()) {
        if (selfInit) {
#if PY_MAJOR_VERSION >= 3
            PyImport_AppendInittab("clingo", &initclingo_);
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
        PY_TRY
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
            Object sysModules = {PyImport_GetModuleDict(), true};
            Object clingoStr = PyString_FromString("clingo");
            int ret = PyDict_Contains(sysModules, clingoStr);
            if (ret == -1) { throw PyException(); }
            if (ret == 0 && !initclingo_()) { throw PyException(); }
#endif
            Object clingoModule = PyImport_ImportModule("clingo");
            Object mainModule = PyImport_ImportModule("__main__");
            main = PyModule_GetDict(mainModule);
            if (!main) { throw PyException(); }
        PY_HANDLE("<internal>", "could not initialize python interpreter");
    }
    void exec(Location const &loc, String code) {
        std::ostringstream oss;
        oss << "<" << loc << ">";
        pyExec(code.c_str(), oss.str().c_str(), main);
    }
    bool callable(String name) {
        if (!PyMapping_HasKeyString(main, const_cast<char *>(name.c_str()))) { return false; }
        Object fun = PyMapping_GetItemString(main, const_cast<char *>(name.c_str()));
        return PyCallable_Check(fun);
    }
    void call(String name, SymSpan args, SymVec &vals) {
        Object fun = PyMapping_GetItemString(main, const_cast<char*>(name.c_str()));
        pycall(fun, args, vals);
    }
    void call(Gringo::Control &ctl) {
        Object fun = PyMapping_GetItemString(main, const_cast<char*>("main"));
        Object params = PyTuple_New(1);
        Object param(ControlWrap::new_(ctl));
        if (PyTuple_SetItem(params, 0, param.release()) < 0) { throw PyException(); }
        Object ret = PyObject_Call(fun, params, Py_None);
    }
    PythonInit init;
    PyObject  *main;
};

// {{{1 definition of Python

std::unique_ptr<PythonImpl> Python::impl = nullptr;

Python::Python(GringoModule &module) {
    ControlWrap::module = &module;
}
bool Python::exec(Location const &loc, String code) {
    if (!impl) { impl = gringo_make_unique<PythonImpl>(); }
    PY_TRY
        impl->exec(loc, code);
        return true;
    PY_HANDLE(loc, "parsing failed");
}
bool Python::callable(String name) {
    if (Py_IsInitialized() && !impl) { impl = gringo_make_unique<PythonImpl>(); }
    try {
        return impl && impl->callable(name);
    }
    catch (PyException const &e) {
        PyErr_Clear();
        return false;
    }
}
SymVec Python::call(Location const &loc, String name, SymSpan args, Logger &log) {
    assert(impl);
    try {
        SymVec vals;
        impl->call(name, args, vals);
        return vals;
    }
    catch (PyException const &) {
        GRINGO_REPORT(log, clingo_warning_operation_undefined)
            << loc << ": info: operation undefined:\n"
            << errorToString()
            ;
        return {};
    }
}
void Python::main(Gringo::Control &ctl) {
    assert(impl);
    PY_TRY
        impl->call(ctl);
    PY_HANDLE("<internal>", "error while calling main function")
}
Python::~Python() = default;

void *Python::initlib(Gringo::GringoModule &module) {
    PY_TRY
        ControlWrap::module = &module;
        PyObject *ret = initclingo_();
        return ret;
    PY_CATCH(nullptr);
}

// }}}1

} // namespace Gringo

#else // WITH_PYTHON

#include "gringo/python.hh"
#include "gringo/symbol.hh"
#include "gringo/locatable.hh"
#include "gringo/logger.hh"

namespace Gringo {

// {{{1 definition of Python

struct PythonImpl { };

std::unique_ptr<PythonImpl> Python::impl = nullptr;

Python::Python(GringoModule &) { }
bool Python::exec(Location const &loc, String ) {
    std::stringstream ss;
    ss << loc << ": error: clingo has been build without python support\n";
    throw GringoError(ss.str());
}
bool Python::callable(String) {
    return false;
}
SymVec Python::call(Location const &, String , SymSpan, Logger &) {
    return {};
}
void Python::main(Control &) { }
Python::~Python() = default;
void *Python::initlib(Gringo::GringoModule &) {
    throw std::runtime_error("clingo lib has been build without python support");
}

// }}}1

} // namespace Gringo

#endif // WITH_PYTHON
