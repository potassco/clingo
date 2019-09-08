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

// NOTE: the python header has a linker pragma to link with python_d.lib
//       when _DEBUG is set which is not part of official python releases
#if defined(_MSC_VER) && defined(_DEBUG) && !defined(CLINGO_UNDEF__DEBUG)
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include "pyclingo.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <memory>
#include <forward_list>
#ifdef _MSC_VER
#pragma warning (disable : 4800) // forcing value to bool 'true' or 'false'
#endif
#if PY_MAJOR_VERSION >= 3
#define PyString_FromString PyUnicode_FromString
#if PY_MINOR_VERSION >= 3
#define PyString_AsString PyUnicode_AsUTF8
#else
#define PyString_AsString _PyUnicode_AsString
#endif
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_Check PyUnicode_Check
#define OBBASE(x) (&(x)->ob_base)
#else
#define OBBASE(x) x
#define Py_hash_t long
#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

#if defined(__clang__) || defined(__GNUC__)
#define CLINGO_ATTRIBUTE_UNUSED __attribute__ ((unused))
#else
#define CLINGO_ATTRIBUTE_UNUSED
#endif
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

// directly translate a c++ into a python exception
void handle_cxx_error () {
    try { throw; }
    catch (std::bad_alloc const &e) { PyErr_SetString(PyExc_MemoryError, e.what()); }
    catch (PyException const &)     { }
    catch (std::exception const &e) { PyErr_SetString(PyExc_RuntimeError, e.what()); }
    catch (...)                     { PyErr_SetString(PyExc_RuntimeError, "unknown error"); }
}

#define PY_TRY try
#define PY_CATCH(ret) catch (...) { handle_cxx_error(); } return ret

struct Iter;
struct Reference;
template <class T>
struct SharedObject;
using Object = SharedObject<PyObject>;

template <class T>
struct ObjectProtocoll {
    // {{{2 object protocol
    bool callable();
    bool callable(char const *name);
    template <class... Args>
    Object call(char const *name, Args &&... args);
    template <class... Args>
    Object operator()(Args &&... args);
    ssize_t size();
    bool empty() { return size() == 0; }
    Object getItem(Reference o);
    Object getItem(char const *key);
    Object getItem(int key);
    void setItem(char const *key, Reference val);
    void setItem(Reference key, Reference val);
    Object getAttr(char const *key);
    void setAttr(char const *key, Reference val);
    void setAttr(Reference key, Reference val);
    bool hasAttr(char const *key);
    bool hasAttr(Reference key);
    Object repr();
    Object str();
    bool isTrue();
    bool isInstance(Reference type);
    bool isInstance(PyTypeObject &type);
    Object richCompare(Reference other, int op);
    Iter iter();
    friend bool operator==(Reference a, Reference b);
    //friend bool operator!=(Reference a, Reference b) {
    //    auto ret = PyObject_RichCompareBool(a, b, Py_NE);
    //    if (ret < 0) { throw PyException(); }
    //    return ret;
    //}

    // }}}2
    bool is_none() const;
    bool is_sequence() const;
    bool is_number() const;
    bool is_str() const;
    bool valid() const;

private:
    PyObject *toPy_() const;
};

struct Reference : ObjectProtocoll<Reference> {
    Reference() : obj(nullptr) { }
    template <class T>
    Reference(T const &x) : obj{x.toPy()} { }
    Reference(std::nullptr_t) : Reference{} { }
    Reference(PyObject *obj) : obj(obj) {
        if (!obj && PyErr_Occurred()) { throw PyException(); }
    }
    PyObject *release() {
        Py_XINCREF(obj);
        PyObject *ret = obj;
        obj = nullptr;
        return ret;
    }
    PyObject *toPy() const { return obj; }
    PyObject *obj;
};


template <class T>
struct SharedObject : ObjectProtocoll<SharedObject<T>>{
    SharedObject() : obj(nullptr) { }
    SharedObject(std::nullptr_t) : Object() { }
    template <class U>
    SharedObject(U const &x) : obj{x.toPy()} { Py_XINCREF(obj); }
    SharedObject(T *obj) : obj(obj) {
        if (!obj && PyErr_Occurred()) { throw PyException(); }
    }
    SharedObject(SharedObject const &other) : obj(other.toPy()) {
        Py_XINCREF(obj);
    }
    SharedObject(SharedObject &&other) : obj(nullptr) {
        std::swap(other.obj, obj);
    }
    T *operator->() { return obj; }
    T *operator->() const { return obj; }
    PyObject *toPy() const                             { return reinterpret_cast<PyObject*>(obj); }
    PyObject *release()                                { PyObject *ret = toPy(); obj = nullptr; return ret; }
    void clear()                                       { Py_CLEAR(obj); }
    SharedObject &operator=(SharedObject const &other) { Py_XDECREF(obj); obj = other.obj; Py_XINCREF(obj); return *this; }
    SharedObject &operator=(SharedObject &&other)      { std::swap(obj, other.obj); return *this; }
    ~SharedObject()                                    { Py_XDECREF(obj); }
    T *obj;
};

using Object = SharedObject<PyObject>;

struct Iter : Object {
    Iter(Object iter)
    : Object(iter) { }
    Object next() {
        return {PyIter_Next(toPy())};
    }
};

Object None() { Py_RETURN_NONE; }

template <class T>
PyObject *ObjectProtocoll<T>::toPy_() const { return static_cast<T const *>(this)->toPy(); }

template <class T>
bool ObjectProtocoll<T>::callable() {
    return PyCallable_Check(toPy_());
}
template <class T>
bool ObjectProtocoll<T>::callable(char const *name) {
    return hasAttr(name) && getAttr(name).callable();
}
template <class T>
template <class... Args>
Object ObjectProtocoll<T>::call(char const *name, Args &&... args) {
    return PyObject_CallMethodObjArgs(toPy_(), Object{PyString_FromString(name)}.toPy(), Reference(args).toPy()..., nullptr);
}
template <class T>
template <class... Args>
Object ObjectProtocoll<T>::operator()(Args &&... args) {
    return PyObject_CallFunctionObjArgs(toPy_(), Reference(args).toPy()..., nullptr);
}
template <class T>
ssize_t ObjectProtocoll<T>::size() {
    auto ret = PyObject_Size(toPy_());
    if (PyErr_Occurred()) { throw PyException(); }
    return ret;
}
template <class T>
Object ObjectProtocoll<T>::getItem(Reference o) {
    return PyObject_GetItem(toPy_(), o.toPy());
}
template <class T>
Object ObjectProtocoll<T>::getItem(char const *key) {
    return getItem(Object{PyString_FromString(key)});
}
template <class T>
Object ObjectProtocoll<T>::getItem(int key) {
    return getItem(Object{PyLong_FromLong(key)});
}
template <class T>
void ObjectProtocoll<T>::setItem(char const *key, Reference val) {
    return setItem(Object{PyString_FromString(key)}, val);
}
template <class T>
void ObjectProtocoll<T>::setItem(Reference key, Reference val) {
    if (PyObject_SetItem(toPy_(), key.toPy(), val.toPy()) < 0) {
        throw PyException();
    }
}
template <class T>
Object ObjectProtocoll<T>::getAttr(char const *key) {
    return PyObject_GetAttrString(toPy_(), key);
}
template <class T>
void ObjectProtocoll<T>::setAttr(char const *key, Reference val) {
    if (PyObject_SetAttrString(toPy_(), key, val.toPy()) < 0) {
        throw PyException();
    }
}
template <class T>
void ObjectProtocoll<T>::setAttr(Reference key, Reference val) {
    if (PyObject_SetAttr(toPy_(), key, val.toPy()) < 0) {
        throw PyException();
    }
}
template <class T>
bool ObjectProtocoll<T>::hasAttr(char const *key) {
    int ret = PyObject_HasAttrString(toPy_(), key);
    if (ret < 0) { throw PyException(); }
    return ret;
}
template <class T>
bool ObjectProtocoll<T>::hasAttr(Reference key) {
    int ret = PyObject_HasAttr(toPy_(), key);
    if (ret < 0) { throw PyException(); }
    return ret;
}
template <class T>
Object ObjectProtocoll<T>::repr() { return PyObject_Repr(toPy_()); }
template <class T>
Object ObjectProtocoll<T>::str() { return PyObject_Str(toPy_()); }
template <class T>
bool ObjectProtocoll<T>::isTrue() {
    auto ret = PyObject_IsTrue(toPy_());
    if (PyErr_Occurred()) { throw PyException(); }
    return ret;
}
template <class T>
bool ObjectProtocoll<T>::isInstance(Reference type) {
    auto inst = PyObject_IsInstance(toPy_(), type);
    if (PyErr_Occurred()) { throw PyException(); }
    return inst;
}
template <class T>
bool ObjectProtocoll<T>::isInstance(PyTypeObject &type) {
    auto inst = PyObject_IsInstance(toPy_(), reinterpret_cast<PyObject*>(&type));
    if (PyErr_Occurred()) { throw PyException(); }
    return inst;
}
template <class T>
Iter ObjectProtocoll<T>::iter() {
    return {PyObject_GetIter(toPy_())};
}
template <class T>
Object ObjectProtocoll<T>::richCompare(Reference other, int op) {
    return PyObject_RichCompare(toPy_(), other.toPy(), op);
}
bool operator==(Reference a, Reference b) {
    auto ret = PyObject_RichCompareBool(a.toPy(), b.toPy(), Py_EQ);
    if (ret < 0) { throw PyException(); }
    return ret;
}
bool operator!=(Object a, Object b) {
    auto ret = PyObject_RichCompareBool(a.toPy(), b.toPy(), Py_NE);
    if (ret < 0) { throw PyException(); }
    return ret;
}

// }}}2
template <class T>
bool ObjectProtocoll<T>::is_none() const { return toPy_() == Py_None; }
template <class T>
bool ObjectProtocoll<T>::is_sequence() const { return PySequence_Check(toPy_()); }
template <class T>
bool ObjectProtocoll<T>::is_number() const { return PyNumber_Check(toPy_()); }
template <class T>
bool ObjectProtocoll<T>::is_str() const { return PyString_Check(toPy_()); }
template <class T>
bool ObjectProtocoll<T>::valid() const { return toPy_(); }

template <class T>
struct ParsePtr {
    ParsePtr(T &x) : x(x) { }
    T *get() { return &x; }
    T &x;
};

template <>
struct ParsePtr<Object> {
    ParsePtr(Object &x) : x(x) { x = nullptr; }
    PyObject **get() { return &x.obj; }
    ~ParsePtr() { Py_XINCREF(x.obj); }
    Object &x;
};

template <>
struct ParsePtr<Reference> {
    ParsePtr(Reference &x) : x(x) { }
    PyObject **get() { return &x.obj; }
    Reference &x;
};

template <class... T>
void ParseTupleAndKeywords(Reference pyargs, Reference pykwds, char const *fmt, char const * const* kwds, T &...x) {
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), fmt, const_cast<char**>(kwds), ParsePtr<T>(x).get()...)) { throw PyException(); }
}

template <class... T>
void ParseTuple(Reference pyargs, char const *fmt, T &...x) {
    if (!PyArg_ParseTuple(pyargs.toPy(), fmt, ParsePtr<T>(x).get()...)) { throw PyException(); }
}

template <Object (*f)(Reference, Reference)>
struct ToFunctionBinary {
    static PyObject *value(PyObject *, PyObject *params, PyObject *keywords) {
        PY_TRY { return f(params, keywords).release(); }
        PY_CATCH(nullptr);
    };
};

template <Object (*f)(Reference)>
struct ToFunctionUnary {
    static PyObject *value(PyObject *, PyObject *params) {
        PY_TRY { return f(params).release(); }
        PY_CATCH(nullptr);
    };
};

template <Object (*f)()>
struct ToFunctionNullary {
    static PyObject *value(PyObject *, PyObject *) {
        PY_TRY { return f().release(); }
        PY_CATCH(nullptr);
    };
};

template <Object (*f)(Reference, Reference)>
constexpr PyCFunction to_function() { return reinterpret_cast<PyCFunction>(ToFunctionBinary<f>::value); }

template <Object (*f)(Reference)>
constexpr PyCFunction to_function() { return reinterpret_cast<PyCFunction>(ToFunctionUnary<f>::value); }

template <Object (*f)()>
constexpr PyCFunction to_function() { return reinterpret_cast<PyCFunction>(ToFunctionNullary<f>::value); }

struct Tuple : Object {
    template <class... Args>
    Tuple(Args &&... args)
    : Object{PyTuple_Pack(sizeof...(args), args.toPy()...)} { }
};

struct List : Object {
    List(std::nullptr_t)
    : Object{} { }
    List(Object x)
    : Object(x) { }
    List(size_t size = 0)
    : Object{PyList_New(size)} { }
    void setItem(size_t i, Object x) {
        if (PyList_SetItem(toPy(), i, x.release()) < 0) { throw PyException(); }
    }
    void append(Reference x) {
        if (PyList_Append(toPy(), x.toPy()) < 0) { throw PyException(); }
    }
    void sort() {
        if (PyList_Sort(toPy()) < 0) { throw PyException(); }
    }
};

struct Dict : Object {
    Dict() : Object{PyDict_New()} {}
    Dict(Object dict) : Object{dict} {}
    List keys() { return {PyDict_Keys(obj)}; }
    List values() { return {PyDict_Values(obj)}; }
    List items() { return {PyDict_Items(obj)}; }
    void delItem(Reference name) {
        if (PyDict_DelItem(obj, name.toPy()) < 0) { throw PyException(); }
    }
    Py_ssize_t length() {
        auto ret = PyDict_Size(obj);
        if (ret == -1) { throw PyException(); }
        return ret;
    }
    bool contains(Reference key) {
        auto ret = PyDict_Contains(obj, key.toPy());
        if (ret == -1) { throw PyException(); }
        return ret;
    }
};

template <class... Args>
Object call(Object (&f)(Reference, Reference), Args&&... args) {
    return f(Tuple{std::forward<Args>(args)...}, Dict{});
}

template <class T>
class ValuePointer {
public:
    ValuePointer(T value) : value_(value) { }
    T &operator*() { return value_; }
    T *operator->() { return &value_; }
private:
    T value_;
};

class IterIterator : std::iterator<std::forward_iterator_tag, Object, ptrdiff_t, ValuePointer<Object>, Object> {
public:
    IterIterator() = delete;
    IterIterator(IterIterator const &) = default;
    IterIterator(Iter it, Object current)
    : it_(it)
    , current_(current) { }
    IterIterator& operator++() { current_ = it_.next(); return *this; }
    IterIterator operator++(int) {
        IterIterator t(*this);
        ++*this;
        return t;
    }
    reference operator*() { return current_; }
    pointer operator->() { return pointer(**this); }

    friend bool operator==(IterIterator a, IterIterator b) { return a.current_.toPy() == b.current_.toPy(); }
    friend bool operator!=(IterIterator a, IterIterator b) { return !(a == b); }
    //friend void swap(IterIterator a, IterIterator b) {
    //    std::swap(a.it_, b.it_);
    //    std::swap(a.current_, b.current_);
    //}
private:
    Iter it_;
    Object current_;
};

IterIterator begin(Iter it) { return {it, it.next()}; }
IterIterator end(Iter it) { return {it, nullptr}; }

// NOTE: all the functions below can use execptions
//       to remove all the annoying return value checking
//       like in the callback there should be an exception
//       that indicates that a python exception is on the stack
//       this exception should simply be handled in PY_CATCH

struct symbol_wrapper {
    clingo_symbol_t symbol;
};
using symbol_vector = std::vector<symbol_wrapper>;

bool pyIsSymbol(Reference obj);

template <class T>
void pyToCpp(Reference pyVec, std::vector<T> &vec);

void pyToCpp(Reference pyBool, bool &x) {
    x = pyBool.isTrue();
}

void pyToCpp(Reference pyObj, std::string &x) {
    Object pyStr = pyObj.str();
    auto ret = PyString_AsString(pyStr.toPy());
    if (!ret) { throw PyException(); }
    x.assign(ret);
}

void pyToCpp(Reference ref, Object &obj);

void pyToNum(Reference pyNum, long &x) { x = PyLong_AsLong(pyNum.toPy()); }
void pyToNum(Reference pyNum, unsigned long &x) { x = PyLong_AsUnsignedLong(pyNum.toPy()); }
CLINGO_ATTRIBUTE_UNUSED void pyToNum(Reference pyNum, long long &x) { x = PyLong_AsLongLong(pyNum.toPy()); }
CLINGO_ATTRIBUTE_UNUSED void pyToNum(Reference pyNum, unsigned long long &x) { x = PyLong_AsUnsignedLongLong(pyNum.toPy()); }

template <bool Signed, bool FitsLong> struct NumType;
template <> struct NumType<true, true> { using Type = long; };
template <> struct NumType<false, true> { using Type = unsigned long; };
template <> struct NumType<true, false> { using Type = long long; };
template <> struct NumType<false, false> { using Type = unsigned long long; };

template <class T>
void pyToCpp(Reference pyNum, T &x, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) {
    typename NumType<std::is_signed<T>::value, (sizeof(T) <= sizeof(long))>::Type y;
    pyToNum(pyNum, y);
    x = static_cast<T>(y);
    if (PyErr_Occurred()) { throw PyException(); }
}

template <class T>
void pyToCpp(Reference pyNum, T &x, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr) {
    x = PyFloat_AsDouble(pyNum.toPy());
    if (PyErr_Occurred()) { throw PyException(); }
}

void pyToCpp(Reference obj, symbol_wrapper &val);

template <class T, class U>
void pyToCpp(Reference pyPair, std::pair<T, U> &x) {
    auto it = pyPair.iter();
    Object pyVal = it.next();
    if (!pyVal.valid()) { throw std::runtime_error("pair expected"); }
    pyToCpp(pyVal, x.first);
    pyVal = it.next();
    if (!pyVal.valid()) { throw std::runtime_error("pair expected"); }
    pyToCpp(pyVal, x.second);
    pyVal = it.next();
    if (pyVal.valid()) { throw std::runtime_error("pair expected"); }
}

struct symbolic_literal_t {
    clingo_symbol_t symbol;
    bool positive;
};

void pyToCpp(Reference obj, symbolic_literal_t &val) {
    std::pair<symbol_wrapper &, bool &> y{ reinterpret_cast<symbol_wrapper&>(val.symbol), val.positive };
    pyToCpp(obj, y);
}

void pyToCpp(Reference pyPair, clingo_weighted_literal_t &x) {
    std::pair<clingo_literal_t &, clingo_weight_t &> y{ x.literal, x.weight };
    pyToCpp(pyPair, y);
}

template <class T>
void pyToCpp(Reference pyVec, std::vector<T> &vec) {
    for (auto x : pyVec.iter()) {
        T ret;
        pyToCpp(x, ret);
        vec.emplace_back(std::move(ret));
    }
}

void pyToCpp(Reference ref, Object &obj) {
    obj = ref;
}

template <class T>
T pyToCpp(Reference py) {
    T ret;
    pyToCpp(py, ret);
    return ret;
}

std::ostream &operator<<(std::ostream &out, Reference o) {
    return out << pyToCpp<std::string>(o.str());
}

struct PrintWrapper {
    Object list;
    char const *pre;
    char const *sep;
    char const *post;
    bool empty;
    friend std::ostream &operator<<(std::ostream &out, PrintWrapper x) {
        auto it = x.list.iter();
        Object o = it.next();
        if (o.valid()) {
            out << x.pre;
            out << o;
            while ((o = it.next()).valid()) { out << x.sep << o; }
            out << x.post;
        }
        else if (x.empty) {
            out << x.pre;
            out << x.post;
        }
        return out;
    }
};

PrintWrapper printList(Reference list, char const *pre, char const *sep, char const *post, bool empty) {
    return {list, pre, sep, post, empty};
}
PrintWrapper printBody(Reference list, char const *pre = " : ") {
    return printList(list, list.empty() ? "" : pre, "; ", ".", true);
}

template <class T>
Object cppRngToPy(T begin, T end);
Object cppToPy(symbol_wrapper val);

template <class T>
Object cppToPy(std::vector<T> const &vals);
template <class T>
Object cppToPy(std::initializer_list<T> l);
template <class T>
Object cppToPy(T const *arr, size_t size);
template <class T, class U>
Object cppToPy(std::pair<T, U> const &pair);
Object cppToPy(char const *n) { return PyString_FromString(n); }
Object cppToPy(bool n) { return PyBool_FromLong(n); }
Object cppToPy(int n) { return PyLong_FromLong(n); }
Object cppToPy(unsigned n) { return PyLong_FromUnsignedLong(n); }
Object cppToPy(long n) { return PyLong_FromLong(n); }
Object cppToPy(unsigned long n) { return PyLong_FromUnsignedLong(n); }
CLINGO_ATTRIBUTE_UNUSED Object cppToPy(long long n) { return PyLong_FromLongLong(n); }
CLINGO_ATTRIBUTE_UNUSED Object cppToPy(unsigned long long n) { return PyLong_FromUnsignedLongLong(n); }

template <class T>
Object cppToPy(T n, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr) {
    return PyFloat_FromDouble(n);
}

Object cppToPy(clingo_weighted_literal_t lit) {
    return Tuple(cppToPy(lit.literal), cppToPy(lit.weight));
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
    return PyEval_EvalCode(x.toPy(), globals, locals);
#else
    return PyEval_EvalCode((PyCodeObject*)x.toPy(), globals, locals);
#endif
}

template <class T>
Object doCmp(T const &a, T const &b, int op) {
    switch (op) {
        case Py_LT: { return cppToPy(a <  b); }
        case Py_LE: { return cppToPy(a <= b); }
        case Py_EQ: { return cppToPy(a == b); }
        case Py_NE: { return cppToPy(a != b); }
        case Py_GT: { return cppToPy(a >  b); }
        case Py_GE: { return cppToPy(a >= b); }
    }
    Py_RETURN_FALSE;
}

std::ostream &operator<<(std::ostream &out, clingo_location_t loc) {
    out << loc.begin_file << ":" << loc.begin_line << ":" << loc.begin_column;
    if (strcmp(loc.begin_file, loc.end_file) != 0) {
        out << "-" << loc.end_file << ":" << loc.end_line << ":" << loc.end_column;
    }
    else if (loc.begin_line != loc.end_line) {
        out << "-" << loc.end_line << ":" << loc.end_column;
    }
    else if (loc.begin_column != loc.end_column) {
        out << "-" << loc.end_column;
    }
    return out;
}

std::string errorToString() {
    try {
        Object type, value, traceback;
        PyErr_Fetch(&type.obj, &value.obj, &traceback.obj);
        PyErr_NormalizeException(&type.obj, &value.obj, &traceback.obj);
        Object tbModule  = PyImport_ImportModule("traceback");
        Reference tbDict = PyModule_GetDict(tbModule.toPy());
        Reference tbFE   = PyDict_GetItemString(tbDict.toPy(), "format_exception");
        Object ret       = PyObject_CallFunctionObjArgs(tbFE.toPy(), type.toPy(), value.valid() ? value.toPy() : Py_None, traceback.valid() ? traceback.toPy() : Py_None, nullptr);
        std::ostringstream oss;
        for (auto line : ret.iter()) {
            oss << "  " << line.str();
        }
        PyErr_Clear();
        return oss.str();
    }
    catch (PyException const &) {
        PyErr_Clear();
        return "error during error handling";
    }
}

// translates a clingo api error into a c++ exception
void handle_c_error(bool ret, std::exception_ptr *exc = nullptr) {
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

// rethrows the current exception and translates it into a python exception
void handle_cxx_error_(std::ostringstream &ss) {
    clingo_error_t code = clingo_error_unknown;
    try { throw; }
    catch (PyException const &) {
        code = clingo_error_runtime;
        ss << errorToString();
    }
    catch (std::runtime_error const &e) {
        code = clingo_error_runtime;
        ss << e.what();
    }
    catch (std::logic_error const &e) {
        code = clingo_error_logic;
        ss << e.what();
    }
    catch (std::bad_alloc const &e) {
        code = clingo_error_bad_alloc;
        ss << e.what();
    }
    catch (std::exception const &e) {
        ss << e.what();
    }
    catch (...) {
        ss << "no message";
    }
    clingo_set_error(code, ss.str().c_str());
}

void handle_cxx_error(clingo_location loc, char const *msg) {
    try {
        std::ostringstream ss;
        ss << loc << ": error: " << msg << ":\n";
        handle_cxx_error_(ss);
    }
    catch (...) {
        clingo_set_error(clingo_error_bad_alloc, "bad alloc during exception handling");
    }
}
void handle_cxx_error(char const *loc, char const *msg) {
    try {
        std::ostringstream ss;
        ss << loc << ": error: " << msg << ":\n";
        handle_cxx_error_(ss);
    }
    catch (...) {
        clingo_set_error(clingo_error_bad_alloc, "bad alloc during exception handling");
    }
}

struct TraverseError : std::exception {
    TraverseError(int ret) : ret{ret} { }
    int ret;
};

struct Traverse {
    void operator()(Reference ref) const {
        if (ref.valid()) {
            auto ret = visit(ref.toPy(), arg);
            if (ret != 0) { throw TraverseError(ret); }
        }
    }
    visitproc visit;
    void *arg;
};

namespace PythonDetail {

// macros

#define CHECK_EXPRESSION(E) decltype(static_cast<void>(E))

template <bool A>
struct Available {
    static constexpr bool available = A;
};

#define WRAP_FUNCTION(F) \
template <class B, class Enable = void> \
struct Get_##F : Available<false> { \
    static constexpr std::nullptr_t value = nullptr; \
}; \
template <class B> \
struct Get_##F<B, CHECK_EXPRESSION(&B::F)> : Available<true>

#define BEGIN_PROTOCOL(F) \
template <class B, class Enable = void> \
struct Get_##F { \
    static constexpr std::nullptr_t value = nullptr; \
    static constexpr bool has_protocol = false; \
}; \
template <class B> \
struct Get_##F<B, CHECK_EXPRESSION(&B::F)> { \
    static constexpr bool has_protocol = true;

#define NEXT_PROTOCOL(G,F) \
}; \
template <class B, class Enable = void> \
struct Get_##F { \
    static constexpr std::nullptr_t value = nullptr; \
    static constexpr bool has_protocol = Get_##G<B>::has_protocol; \
}; \
template <class B> \
struct Get_##F<B, CHECK_EXPRESSION(&B::F)> { \
    static constexpr bool has_protocol = true;

#define END_PROTOCOL(G, F, T) \
}; \
template <class B, class Enable = void> \
struct Get_##F { \
    static constexpr T* value = nullptr; \
}; \
template <class B> \
struct Get_##F<B, typename std::enable_if<Get_##G<B>::has_protocol>::type> { \
    static T value[]; \
}; \
template <class B> \
T Get_##F<B, typename std::enable_if<Get_##G<B>::has_protocol>::type>::value[] =

// gc protocol

WRAP_FUNCTION(tp_traverse) {
    static int value(PyObject *pySelf, visitproc visit, void *arg) {
        PY_TRY {
            auto self = reinterpret_cast<B*>(pySelf);
            Traverse t{visit, arg};
            self->tp_traverse(t);
            return 0;
        }
        catch (TraverseError const &e) { return e.ret; }
        PY_CATCH(1);
    };
};

WRAP_FUNCTION(tp_clear) {
    static int value(PyObject *pySelf) {
        PY_TRY {
            auto self = reinterpret_cast<B*>(pySelf);
            self->tp_clear();
            return 0;
        }
        PY_CATCH(1);
    };
};

// object protocol

WRAP_FUNCTION(tp_dealloc) {
    static void value(PyObject *self) {
        if (PythonDetail::Get_tp_clear<B>::available) {
            PyObject_GC_UnTrack(self);
        }
        reinterpret_cast<B*>(self)->tp_dealloc();
        B::type.tp_free(self);
    };
};

WRAP_FUNCTION(tp_repr) {
    static PyObject *value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->tp_repr().release(); }
        PY_CATCH(nullptr);
    };
};

WRAP_FUNCTION(tp_new) {
    static PyObject *value(PyTypeObject *type, PyObject *, PyObject *) {
        PY_TRY { return B::tp_new(type).release(); }
        PY_CATCH(nullptr);
    };
};

WRAP_FUNCTION(tp_init) {
    static int value(PyObject *self, PyObject *args, PyObject *kwargs) {
        PY_TRY { reinterpret_cast<B*>(self)->tp_init(args, kwargs); return 0; }
        PY_CATCH(-1);
    };
};

template <class B, class Enable = void>
struct Get_tp_str : Get_tp_repr<B, void> { };

template <class B>
struct Get_tp_str<B, CHECK_EXPRESSION(&B::tp_str)> {
    static PyObject *value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->tp_str().release(); }
        PY_CATCH(nullptr);
    };
};

template <typename D, int P, int S>
struct PyHash {
    Py_hash_t operator()(D x) const { return static_cast<Py_hash_t>(x); }
};

template <typename D>
struct PyHash<D, 4, 8> {
    Py_hash_t operator()(D x) const {
        return static_cast<Py_hash_t>((x >> 32) ^ x);
    }
};

WRAP_FUNCTION(tp_hash) {
    static Py_hash_t value(PyObject *self) {
        PY_TRY { return PyHash<size_t, sizeof(Py_hash_t), sizeof(size_t)>()(reinterpret_cast<B*>(self)->tp_hash()); }
        PY_CATCH(-1);
    };
};

WRAP_FUNCTION(tp_richcompare) {
    static PyObject *value(PyObject *pySelf, PyObject *pyB, int op) {
        PY_TRY {
            auto self = reinterpret_cast<B*>(pySelf);
            Reference b{pyB};
            return b.isInstance(self->type)
                ? self->tp_richcompare(*reinterpret_cast<B*>(pyB), op).release()
                : (Py_INCREF(Py_NotImplemented), Py_NotImplemented);
        }
        PY_CATCH(nullptr);
    };
};

WRAP_FUNCTION(tp_iter) {
    static PyObject *value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->tp_iter().release(); }
        PY_CATCH(nullptr);
    };
};

WRAP_FUNCTION(tp_getattro) {
    static PyObject *value(PyObject *self, PyObject *name) {
        PY_TRY { return reinterpret_cast<B*>(self)->tp_getattro(Reference{name}).release(); }
        PY_CATCH(nullptr);
    };
};

WRAP_FUNCTION(tp_setattro) {
    static int value(PyObject *self, PyObject *name, PyObject *value) {
        PY_TRY { return (reinterpret_cast<B*>(self)->tp_setattro(Reference{name}, Reference{value}), 0); }
        PY_CATCH(-1);
    };
};

WRAP_FUNCTION(tp_iternext) {
    static PyObject *value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->tp_iternext().release(); }
        PY_CATCH(nullptr);
    };
};

// mapping protocol

BEGIN_PROTOCOL(mp_length)
    static Py_ssize_t value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->mp_length(); }
        PY_CATCH(-1);
    }
NEXT_PROTOCOL(mp_length, mp_subscript)
    static PyObject *value(PyObject *self, PyObject *name) {
        PY_TRY { return reinterpret_cast<B*>(self)->mp_subscript(Reference{name}).release(); }
        PY_CATCH(nullptr);
    }
NEXT_PROTOCOL(mp_subscript, mp_ass_subscript)
    static int value(PyObject *self, PyObject *name, PyObject *value) {
        PY_TRY { return (reinterpret_cast<B*>(self)->mp_ass_subscript(Reference{name}, Reference{value}), 0); }
        PY_CATCH(-1);
    }
END_PROTOCOL(mp_ass_subscript, tp_as_mapping, PyMappingMethods) {{
    Get_mp_length<B>::value,
    Get_mp_subscript<B>::value,
    Get_mp_ass_subscript<B>::value,
}};

// sequence protocol

BEGIN_PROTOCOL(sq_length)
    static Py_ssize_t value(PyObject *self) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_length(); }
        PY_CATCH(-1);
    };
NEXT_PROTOCOL(sq_length, sq_concat)
    static PyObject *value(PyObject *self, PyObject *other) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_concat(Reference{other}).release(); }
        PY_CATCH(nullptr);
    };
NEXT_PROTOCOL(sq_concat, sq_repeat)
    static PyObject *value(PyObject *self, Py_ssize_t count) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_repeat(count).release(); }
        PY_CATCH(nullptr);
    };
NEXT_PROTOCOL(sq_repeat, sq_item)
    static PyObject *value(PyObject *self, Py_ssize_t index) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_item(index).release(); }
        PY_CATCH(nullptr);
    };
NEXT_PROTOCOL(sq_item, sq_slice)
    static PyObject *value(PyObject *self, Py_ssize_t left, Py_ssize_t right) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_slice(left, right).release(); }
        PY_CATCH(nullptr);
    };
NEXT_PROTOCOL(sq_slice, sq_ass_item)
    static int value(PyObject *self, Py_ssize_t index, PyObject *value) {
        PY_TRY { return (reinterpret_cast<B*>(self)->sq_ass_item(index, Reference{value}), 0); }
        PY_CATCH(-1);
    };
NEXT_PROTOCOL(sq_ass_item, sq_ass_slice)
    static int value(PyObject *self, Py_ssize_t left, Py_ssize_t right, PyObject *value) {
        PY_TRY { return (reinterpret_cast<B*>(self)->sq_ass_slice(left, right, Reference{value}), 0); }
        PY_CATCH(-1);
    };
NEXT_PROTOCOL(sq_ass_slice, sq_contains)
    static int value(PyObject *self, PyObject *value) {
        PY_TRY { return reinterpret_cast<B*>(self)->sq_contains(Reference{value}); }
        PY_CATCH(-1);
    };
NEXT_PROTOCOL(sq_contains, sq_inplace_concat)
    static PyObject *value(PyObject *self, PyObject *other) {
        PY_TRY { reinterpret_cast<B*>(self)->sq_inplace_concat(Reference{other}); Py_XINCREF(self); return self; }
        PY_CATCH(nullptr);
    };
NEXT_PROTOCOL(sq_inplace_concat, sq_inplace_repeat)
    static PyObject *value(PyObject *self, Py_ssize_t count) {
        PY_TRY { reinterpret_cast<B*>(self)->sq_inplace_repeat(count); Py_XINCREF(self); return self; }
        PY_CATCH(nullptr);
    };
END_PROTOCOL(sq_inplace_repeat, tp_as_sequence, PySequenceMethods) {{
    Get_sq_length<B>::value,
    Get_sq_concat<B>::value,
    Get_sq_repeat<B>::value,
    Get_sq_item<B>::value,
    Get_sq_slice<B>::value,
    Get_sq_ass_item<B>::value,
    Get_sq_ass_slice<B>::value,
    Get_sq_contains<B>::value,
    Get_sq_inplace_concat<B>::value,
    Get_sq_inplace_repeat<B>::value,
}};

} // namespace PythonDetail

template <class T>
struct ObjectBase : ObjectProtocoll<T> {
    PyObject_HEAD
    static PyTypeObject type;

    static constexpr PyGetSetDef *tp_getset = nullptr;
    static PyMethodDef tp_methods[];

    static bool initType(Reference module) {
        if (PyType_Ready(&type) < 0) { return false; }
        Py_INCREF(&type);
        if (PyModule_AddObject(module.toPy(), T::tp_type, (PyObject*)&type) < 0) { return false; }
        return true;
    }

    static SharedObject<T> new_() {
        return new_(&type);
    }

    static SharedObject<T> new_(PyTypeObject *type) {
        T *self;
        self = reinterpret_cast<T*>(type->tp_alloc(type, 0));
        if (!self) { throw PyException(); }
        return self;
    }
    PyObject *toPy() const { return reinterpret_cast<PyObject*>(const_cast<ObjectBase*>(this)); }

protected:
    template <Object (T::*f)()>
    static getter to_getter() { return to_getter_<f>; }
    template <void (T::*f)(Reference)>
    static setter to_setter() { return to_setter_<f>; }
    template <Object (T::*f)()>
    static PyCFunction to_function() { return to_function_<Object, f>; }
    template <Reference (T::*f)()>
    static PyCFunction to_function() { return to_function_<Reference, f>; }
    template <Object (T::*f)(Reference)>
    static PyCFunction to_function() { return to_function_<Object, f>; }
    template <Object (T::*f)(Reference, Reference)>
    static PyCFunction to_function() {
        auto x = to_function_<Object, f>;
        return reinterpret_cast<PyCFunction>(x);
    }

private:
    template <Object (T::*f)()>
    static PyObject *to_getter_(PyObject *o, void *) {
        PY_TRY { return (reinterpret_cast<T*>(o)->*f)().release(); }
        PY_CATCH(nullptr);
    }
    template <void (T::*f)(Reference)>
    static int to_setter_(PyObject *self, PyObject *value, void *) {
        PY_TRY { return ((reinterpret_cast<T*>(self)->*f)(Reference{value}), 0); }
        PY_CATCH(-1);
    }
    template <class R, R (T::*f)()>
    static PyObject *to_function_(PyObject *self, PyObject *) {
        PY_TRY { return (reinterpret_cast<T*>(self)->*f)().release(); }
        PY_CATCH(nullptr);
    }
    template <class R, R (T::*f)(Reference)>
    static PyObject *to_function_(PyObject *self, PyObject *params) {
        PY_TRY { return (reinterpret_cast<T*>(self)->*f)(params).release(); }
        PY_CATCH(nullptr);
    }
    template <class R, R (T::*f)(Reference, Reference)>
    static PyObject *to_function_(PyObject *self, PyObject *params, PyObject *keywords) {
        PY_TRY { return (reinterpret_cast<T*>(self)->*f)(params, keywords).release(); }
        PY_CATCH(nullptr);
    }
};

template <class T>
PyMethodDef ObjectBase<T>::tp_methods[] = {{nullptr, nullptr, 0, nullptr}};

template <class T>
PyTypeObject ObjectBase<T>::type = {
    PyVarObject_HEAD_INIT(nullptr, 0)
    T::tp_name,                                 // tp_name
    sizeof(T),                                  // tp_basicsize
    0,                                          // tp_itemsize
    PythonDetail::Get_tp_dealloc<T>::value,     // tp_dealloc
    0,                                          // tp_print
    nullptr,                                    // tp_getattr
    nullptr,                                    // tp_setattr
    nullptr,                                    // tp_compare
    PythonDetail::Get_tp_repr<T>::value,        // tp_repr
    nullptr,                                    // tp_as_number
    PythonDetail::Get_tp_as_sequence<T>::value, // tp_as_sequence
    PythonDetail::Get_tp_as_mapping<T>::value,  // tp_as_mapping
    PythonDetail::Get_tp_hash<T>::value,        // tp_hash
    nullptr,                                    // tp_call
    PythonDetail::Get_tp_str<T>::value,         // tp_str
    PythonDetail::Get_tp_getattro<T>::value,    // tp_getattro
    PythonDetail::Get_tp_setattro<T>::value,    // tp_setattro
    nullptr,                                    // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    (PythonDetail::Get_tp_traverse<T>::available ? Py_TPFLAGS_HAVE_GC : 0UL),
                                                // tp_flags
    T::tp_doc,                                  // tp_doc
    PythonDetail::Get_tp_traverse<T>::value,    // tp_traverse
    PythonDetail::Get_tp_clear<T>::value,       // tp_clear
    PythonDetail::Get_tp_richcompare<T>::value, // tp_richcompare
    0,                                          // tp_weaklistoffset
    PythonDetail::Get_tp_iter<T>::value,        // tp_iter
    PythonDetail::Get_tp_iternext<T>::value,    // tp_iternext
    T::tp_methods,                              // tp_methods
    nullptr,                                    // tp_members
    T::tp_getset,                               // tp_getset
    nullptr,                                    // tp_base
    nullptr,                                    // tp_dict
    nullptr,                                    // tp_descr_get
    nullptr,                                    // tp_descr_set
    0,                                          // tp_dictoffset
    PythonDetail::Get_tp_init<T>::value,        // tp_init
    nullptr,                                    // tp_alloc
    PythonDetail::Get_tp_new<T>::value,         // tp_new
    nullptr,                                    // tp_free
    nullptr,                                    // tp_is_gc
    nullptr,                                    // tp_bases
    nullptr,                                    // tp_mro
    nullptr,                                    // tp_cache
    nullptr,                                    // tp_subclasses
    nullptr,                                    // tp_weaklist
    nullptr,                                    // tp_del
#ifdef Py_TPFLAGS_HAVE_VERSION_TAG
    0,                                          // tp_version_tag
#endif
#ifdef Py_TPFLAGS_HAVE_FINALIZE
    nullptr,                                    // tp_finalize
#endif
#ifdef COUNT_ALLOCS
    0,                                          // tp_allocs
    0,                                          // tp_frees
    0,                                          // tp_maxalloc
    nullptr,                                    // tp_prev
    nullptr,                                    // tp_next
#endif
};

template <class T>
struct EnumType : ObjectBase<T> {
    unsigned offset;

    static bool initType(Reference module) {
        return ObjectBase<T>::initType(module) && addAttr() >= 0;
    }
    static PyObject *new_(unsigned offset) {
        EnumType *self;
        self = reinterpret_cast<EnumType*>(ObjectBase<T>::type.tp_alloc(&ObjectBase<T>::type, 0));
        if (!self) { return nullptr; }
        self->offset = offset;
        return reinterpret_cast<PyObject*>(self);
    }

    Object tp_repr() {
        return PyString_FromString(T::strings[offset]);
    }

    template <class U>
    static Object getAttr(U ret) {
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
            if (!elem.valid()) { return -1; }
            if (PyDict_SetItemString(ObjectBase<T>::type.tp_dict, T::strings[i], elem.toPy()) < 0) { return -1; }
        }
        return 0;
    }

    size_t tp_hash() {
        return static_cast<size_t>(offset);
    }

    Object tp_richcompare(EnumType b, int op) {
        return doCmp(offset, b.offset, op);
    }
};
template <class T>
auto enumValue(Reference self) -> decltype(std::declval<T*>()->values[0]) {
    if (!self.isInstance(T::type)) {
        throw std::runtime_error("not an enumeration object");
    }
    auto *p = reinterpret_cast<T*>(self.toPy());
    return p->values[p->offset];
}


// }}}1

// {{{1 wrap TheoryTerm

struct TheoryTermType : EnumType<TheoryTermType> {
    static constexpr char const *tp_type = "TheoryTermType";
    static constexpr char const *tp_name = "clingo.TheoryTermType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of theory terms.

`TheoryTermType` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed objects are available:

Attributes
----------
Function : TheoryTermType
    For a function theory terms.
Number : TheoryTermType
    For numeric theory terms.
Symbol : TheoryTermType
    For symbolic theory terms.
List : TheoryTermType
    For list theory terms.
Tuple : TheoryTermType
    For tuple theory terms.
Set : TheoryTermType
    For set theory terms.
)";

    static constexpr clingo_theory_term_type const values[] = {
        clingo_theory_term_type_function,
        clingo_theory_term_type_number,
        clingo_theory_term_type_symbol,
        clingo_theory_term_type_list,
        clingo_theory_term_type_tuple,
        clingo_theory_term_type_set
    };
    static constexpr const char * const strings[] = {
        "Function",
        "Number",
        "Symbol",
        "List",
        "Tuple",
        "Set"
    };
};

constexpr clingo_theory_term_type const TheoryTermType::values[];
constexpr const char * const TheoryTermType::strings[];

struct TheoryTerm : ObjectBase<TheoryTerm> {
    clingo_theory_atoms_t const *atoms;
    clingo_id_t value;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "TheoryTerm";
    static constexpr char const *tp_name = "clingo.TheoryTerm";
    static constexpr char const *tp_doc =
R"(`TheoryTerm` objects represent theory terms.

Theory terms have a readable string representation, implement Python's rich
comparison operators, and can be used as dictionary keys.
)";

    static Object construct(clingo_theory_atoms_t const *atoms, clingo_id_t value) {
        auto self = new_();
        self->value = value;
        self->atoms = atoms;
        return self;
    }
    Object name() {
        char const *ret;
        handle_c_error(clingo_theory_atoms_term_name(atoms, value, &ret));
        return cppToPy(ret);
    }
    Object number() {
        int ret;
        handle_c_error(clingo_theory_atoms_term_number(atoms, value, &ret));
        return cppToPy(ret);
    }
    Object args() {
        clingo_id_t const *args;
        size_t size;
        handle_c_error(clingo_theory_atoms_term_arguments(atoms, value, &args, &size));
        List list;
        for (size_t i = 0; i < size; ++i, ++args) {
            list.append(construct(atoms, *args));
        }
        return list;
    }
    Object tp_repr() {
        std::vector<char> ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_term_to_string_size(atoms, value, &size));
        ret.resize(size);
        handle_c_error(clingo_theory_atoms_term_to_string(atoms, value, ret.data(), size));
        return cppToPy(ret.data());
    }
    Object termType() {
        clingo_theory_term_type_t ret;
        handle_c_error(clingo_theory_atoms_term_type(atoms, value, &ret));
        return TheoryTermType::getAttr(ret);
    }
    size_t tp_hash() {
        return value;
    }
    Object tp_richcompare(TheoryTerm &b, int op) {
        return doCmp(value, b.value, op);
    }
};

PyGetSetDef TheoryTerm::tp_getset[] = {
    {(char *)"type", to_getter<&TheoryTerm::termType>(), nullptr, (char *)R"(type: TheoryTermType

The type of the theory term.)", nullptr},
    {(char *)"name", to_getter<&TheoryTerm::name>(), nullptr, (char *)R"(name: str

The name of the term (for symbols and functions).)", nullptr},
    {(char *)"arguments", to_getter<&TheoryTerm::args>(), nullptr, (char *)R"(arguments: List[Symbol]

The arguments of the term (for functions, tuples, list, and sets).
)", nullptr},
    {(char *)"number", to_getter<&TheoryTerm::number>(), nullptr, (char *)R"(number: int

The numeric representation of the term (for numbers).
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryElement

struct TheoryElement : ObjectBase<TheoryElement> {
    clingo_theory_atoms_t const *atoms;
    clingo_id_t value;
    static constexpr char const *tp_type = "TheoryElement";
    static constexpr char const *tp_name = "clingo.TheoryElement";
    static constexpr char const *tp_doc =
R"(Class to represent theory elements.

Theory elements have a readable string representation, implement Python's rich
comparison operators, and can be used as dictionary keys.
)";
    static PyGetSetDef tp_getset[];
    static Object construct(clingo_theory_atoms const *atoms, clingo_id_t value) {
        auto self = new_();
        self->value = value;
        self->atoms = atoms;
        return self;
    }
    Object terms() {
        clingo_id_t const *ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_element_tuple(atoms, value, &ret, &size));
        List list;
        for (size_t i = 0; i < size; ++i, ++ret) {
            list.append(TheoryTerm::construct(atoms, *ret));
        }
        return list;
    }
    Object condition() {
        clingo_literal_t const *ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_element_condition(atoms, value, &ret, &size));
        List list;
        for (size_t i = 0; i < size; ++i, ++ret) {
            list.append(cppToPy(*ret));
        }
        return list;
    }
    Object condition_id() {
        clingo_literal_t ret;
        handle_c_error(clingo_theory_atoms_element_condition_id(atoms, value, &ret));
        return cppToPy(ret);
    }
    Object tp_repr() {
        std::vector<char> ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_element_to_string_size(atoms, value, &size));
        ret.resize(size);
        handle_c_error(clingo_theory_atoms_element_to_string(atoms, value, ret.data(), size));
        return cppToPy(ret.data());
    }
    size_t tp_hash() {
        return value;
    }
    Object tp_richcompare(TheoryElement &b, int op) {
        return doCmp(value, b.value, op);
    }
};

PyGetSetDef TheoryElement::tp_getset[] = {
    {(char *)"terms", to_getter<&TheoryElement::terms>(), nullptr, (char *)R"(terms: List[TheoryTerm]

The tuple of the element.
)", nullptr},
    {(char *)"condition", to_getter<&TheoryElement::condition>(), nullptr, (char *)R"(condition: List[TheoryTerm]

The condition of the element.
)", nullptr},
    {(char *)"condition_id", to_getter<&TheoryElement::condition_id>(), nullptr, (char *)R"(condition_id: int

Each condition has an id. This id can be passed to
`PropagateInit.solver_literal` to obtain a solver literal equivalent to the
condition.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryAtom

struct TheoryAtom : ObjectBase<TheoryAtom> {
    clingo_theory_atoms_t const *atoms;
    clingo_id_t value;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "TheoryAtom";
    static constexpr char const *tp_name = "clingo.TheoryAtom";
    static constexpr char const *tp_doc = R"(Class to represent theory atoms.

Theory atoms have a readable string representation, implement Python's rich
comparison operators, and can be used as dictionary keys.
)";
    static Object construct(clingo_theory_atoms_t const *atoms, clingo_id_t value) {
        auto self = new_();
        self->value = value;
        self->atoms = atoms;
        return self;
    }
    Object elements() {
        clingo_id_t const *ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_atom_elements(atoms, value, &ret, &size));
        List list;
        for (size_t i = 0; i < size; ++i, ++ret) {
            list.append(TheoryElement::construct(atoms, *ret));
        }
        return list;
    }
    Object term() {
        clingo_id_t ret;
        handle_c_error(clingo_theory_atoms_atom_term(atoms, value, &ret));
        return TheoryTerm::construct(atoms, ret);
    }
    Object literal() {
        clingo_literal_t ret;
        handle_c_error(clingo_theory_atoms_atom_literal(atoms, value, &ret));
        return cppToPy(ret);
    }
    Object guard() {
        bool hasGuard;
        handle_c_error(clingo_theory_atoms_atom_has_guard(atoms, value, &hasGuard));
        if (!hasGuard) { return None(); }
        char const *conn;
        clingo_id_t term;
        handle_c_error(clingo_theory_atoms_atom_guard(atoms, value, &conn, &term));
        return Tuple(cppToPy(conn), TheoryTerm::construct(atoms, term));
    }
    Object tp_repr() {
        std::vector<char> ret;
        size_t size;
        handle_c_error(clingo_theory_atoms_atom_to_string_size(atoms, value, &size));
        ret.resize(size);
        handle_c_error(clingo_theory_atoms_atom_to_string(atoms, value, ret.data(), size));
        return cppToPy(ret.data());
    }
    size_t tp_hash() {
        return value;
    }
    Object tp_richcompare(TheoryAtom &b, int op) {
        return doCmp(value, b.value, op);
    }
};

PyGetSetDef TheoryAtom::tp_getset[] = {
    {(char *)"elements", to_getter<&TheoryAtom::elements>(), nullptr, (char *)R"(elements: List[TheoryElement]

The elements of the atom.)", nullptr},
    {(char *)"term", to_getter<&TheoryAtom::term>(), nullptr, (char *)R"(term: TheoryTerm

The term of the atom.
)", nullptr},
    {(char *)"guard", to_getter<&TheoryAtom::guard>(), nullptr, (char *)R"(guard: Tuple[str, TheoryTerm]

The guard of the atom or None if the atom has no guard.
)", nullptr},
    {(char *)"literal", to_getter<&TheoryAtom::literal>(), nullptr, (char *)R"(literal: int

The program literal associated with the atom.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap TheoryAtomIter

struct TheoryAtomIter : ObjectBase<TheoryAtomIter> {
    clingo_theory_atoms_t const *atoms;
    clingo_id_t offset;

    static constexpr char const *tp_type = "TheoryAtomIter";
    static constexpr char const *tp_name = "clingo.TheoryAtomIter";
    static constexpr char const *tp_doc =
R"(Implements `Iterator[SymbolicAtom]`.)";
    static Object construct(clingo_theory_atoms_t const *atoms, clingo_id_t offset) {
        auto self = new_();
        self->atoms = atoms;
        self->offset = offset;
        return self;
    }
    Reference tp_iter() { return *this; }
    Object tp_iternext() {
        size_t size;
        handle_c_error(clingo_theory_atoms_size(atoms, &size));
        if (offset < size) {
            Object next = TheoryAtom::construct(atoms, offset);
            ++offset;
            return next;
        } else {
            PyErr_SetNone(PyExc_StopIteration);
            return nullptr;
        }
    }
};

// {{{1 wrap Symbol

struct SymbolType : EnumType<SymbolType> {
    static constexpr char const *tp_type = "SymbolType";
    static constexpr char const *tp_name = "clingo.SymbolType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of symbols.

`SymbolType` objects have a readable string representation, implement Python's
rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed objects are available:

Attributes
----------
Number : SymbolType
    A numeric symbol, e.g., `1`.
String : SymbolType
    A string symbol, e.g., `"a"`.
Function : SymbolType
    A function symbol, e.g., `c`, `(1, "a")`, or `f(1,"a")`.
Infimum : SymbolType
    The `#inf` symbol.
Supremum : SymbolType
    The `#sup` symbol
)";

    static constexpr enum clingo_symbol_type const values[] = {
        clingo_symbol_type_number,
        clingo_symbol_type_string,
        clingo_symbol_type_function,
        clingo_symbol_type_infimum,
        clingo_symbol_type_supremum
    };
    static constexpr const char * const strings[] = { "Number", "String", "Function", "Infimum", "Supremum" };
};

constexpr enum clingo_symbol_type const SymbolType::values[];
constexpr const char * const SymbolType::strings[];

struct Symbol : ObjectBase<Symbol> {
    clingo_symbol_t val;
    static PyObject *inf;
    static PyObject *sup;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "Symbol";
    static constexpr char const *tp_name = "clingo.Symbol";
    static constexpr char const *tp_doc =
R"(Represents a gringo symbol.

This includes numbers, strings, functions (including constants with
`len(arguments) == 0` and tuples with `len(name) == 0`), `#inf` and `#sup`.

Symbol objects implemente Python's rich comparison operators and are ordered
like in gringo. They can also be used as keys in dictionaries. Their string
representation corresponds to their gringo representation.

Notes
-----
Note that this class does not have a constructor. Instead there are the
functions `Number`, `String`, and `Function` to construct symbol objects or the
preconstructed symbols `Infimum` and `Supremum`.)";

    static bool initType(Reference module) {
        if (!ObjectBase<Symbol>::initType(module)) { return false; }
        inf = type.tp_alloc(&type, 0);
        if (!inf) { return false; }
        clingo_symbol_create_infimum(&reinterpret_cast<Symbol*>(inf)->val);
        if (PyModule_AddObject(module.toPy(), "Infimum", inf) < 0) { return false; }
        sup = type.tp_alloc(&type, 0);
        clingo_symbol_create_supremum(&reinterpret_cast<Symbol*>(sup)->val);
        if (!sup) { return false; }
        if (PyModule_AddObject(module.toPy(), "Supremum", sup) < 0) { return false; }
        return true;
    }

    static Object construct(clingo_symbol_t value) {
        auto type = clingo_symbol_type(value);
        if (type == clingo_symbol_type_infimum) {
            Py_INCREF(inf);
            return inf;
        }
        else if (type == clingo_symbol_type_supremum) {
            Py_INCREF(sup);
            return sup;
        }
        else {
            auto self = new_();
            self->val = value;
            return self;
        }
    }

    static Object construct(char const *name, Reference params, Reference pyPos) {
        auto sign = !pyToCpp<bool>(pyPos);
        if (strcmp(name, "") == 0 && sign) {
            PyErr_SetString(PyExc_RuntimeError, "tuples must not have signs");
            throw PyException();
        }
        clingo_symbol_t ret;
        if (!params.is_none()) {
            std::vector<symbol_wrapper> syms;
            pyToCpp(params, syms);
            handle_c_error(clingo_symbol_create_function(name, reinterpret_cast<clingo_symbol_t*>(syms.data()), syms.size(), !sign, &ret));
        }
        else {
            handle_c_error(clingo_symbol_create_id(name, !sign, &ret));
        }
        return construct(ret);
    }
    static Object new_function(Reference args, Reference kwds) {
        static char const *kwlist[] = {"name", "arguments", "positive", nullptr};
        char const *name;
        Reference pyPos = Py_True;
        Reference params = Py_None;
        ParseTupleAndKeywords(args, kwds, "s|OO", kwlist, name, params, pyPos);
        return construct(name, params, pyPos);
    }
    static Object new_tuple(Reference arg) {
        return construct("", arg, Py_True);
    }
    static Object new_number(Reference arg) {
        auto num = pyToCpp<int>(arg);
        clingo_symbol_t ret;
        clingo_symbol_create_number(num, &ret);
        return construct(ret);
    }
    static Object new_symbol(Reference arg) {
        return construct(pyToCpp<clingo_symbol_t>(arg));
    }
    static Object new_string(Reference arg) {
        auto str = pyToCpp<std::string>(arg);
        clingo_symbol_t ret;
        handle_c_error(clingo_symbol_create_string(str.c_str(), &ret));
        return construct(ret);
    }
    Object name() {
        if (clingo_symbol_type(val) == clingo_symbol_type_function) {
            char const *ret;
            handle_c_error(clingo_symbol_name(val, &ret));
            return cppToPy(ret);
        }
        else { return None(); }
    }

    Object string() {
        if (clingo_symbol_type(val) == clingo_symbol_type_string) {
            char const *ret;
            handle_c_error(clingo_symbol_string(val, &ret));
            return cppToPy(ret);
        }
        else { return None(); }
    }

    Object negative() {
        if (clingo_symbol_type(val) == clingo_symbol_type_function) {
            bool ret;
            handle_c_error(clingo_symbol_is_negative(val, &ret));
            return cppToPy(ret);
        }
        else { return None(); }
    }

    Object positive() {
        if (clingo_symbol_type(val) == clingo_symbol_type_function) {
            bool ret;
            handle_c_error(clingo_symbol_is_negative(val, &ret));
            return cppToPy(!ret);
        }
        else { return None(); }
    }

    Object num() {
        if (clingo_symbol_type(val) == clingo_symbol_type_number) {
            int ret;
            handle_c_error(clingo_symbol_number(val, &ret));
            return cppToPy(ret);
        }
        else { return None(); }
    }

    Object args() {
        if (clingo_symbol_type(val) == clingo_symbol_type_function) {
            clingo_symbol_t const *ret;
            size_t size;
            handle_c_error(clingo_symbol_arguments(val, &ret, &size));
            return cppRngToPy(reinterpret_cast<symbol_wrapper const*>(ret), reinterpret_cast<symbol_wrapper const*>(ret) + size);
        }
        else { return None(); }
    }

    Object type_() {
        return SymbolType::getAttr(clingo_symbol_type(val));
    }

    Object match(Reference pyargs, Reference pykwds) {
        char const *name, *pyName;
        int pyArity;
        size_t arity;
        char const *kwlist[] = {"name", "arity", nullptr};
        ParseTupleAndKeywords(pyargs, pykwds, "si", kwlist, pyName, pyArity);
        clingo_symbol_t const *args;
        if (clingo_symbol_type(val) != clingo_symbol_type_function) { Py_RETURN_FALSE; }
        handle_c_error(clingo_symbol_name(val, &name));
        if (strcmp(name, pyName) != 0) { Py_RETURN_FALSE; }
        handle_c_error(clingo_symbol_arguments(val, &args, &arity));
        if (static_cast<int>(arity) != pyArity) { Py_RETURN_FALSE; }
        Py_RETURN_TRUE;
    }

    Object tp_repr() {
        std::vector<char> ret;
        size_t size;
        handle_c_error(clingo_symbol_to_string_size(val, &size));
        ret.resize(size);
        handle_c_error(clingo_symbol_to_string(val, ret.data(), size));
        return cppToPy(ret.data());
    }

    size_t tp_hash() {
        return clingo_symbol_hash(val);
    }

    Object tp_richcompare(Symbol &b, int op) {
        switch (op) {
            case Py_LT: { return cppToPy( clingo_symbol_is_less_than(val, b.val)); }
            case Py_LE: { return cppToPy(!clingo_symbol_is_less_than(b.val, val)); }
            case Py_EQ: { return cppToPy( clingo_symbol_is_equal_to (val, b.val)); }
            case Py_NE: { return cppToPy(!clingo_symbol_is_equal_to (val, b.val)); }
            case Py_GT: { return cppToPy( clingo_symbol_is_less_than(b.val, val)); }
            case Py_GE: { return cppToPy(!clingo_symbol_is_less_than(val, b.val)); }
        }
        return None();
    }

    Object to_c() {
        return cppToPy(val);
    }
};

PyMethodDef Symbol::tp_methods[] = {
    {"match", to_function<&Symbol::match>(), METH_KEYWORDS | METH_VARARGS,
R"(match(self, name: str, arity: int) -> bool

Check if this is a function symbol with the given signature.

Parameters
----------
name : str
    The name of the function.

arity : int
    The arity of the function.

Returns
-------
bool
    Whether the function matches.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef Symbol::tp_getset[] = {
    {(char *)"name", to_getter<&Symbol::name>(), nullptr, (char *)R"(name: str

The name of a function.
)", nullptr},
    {(char *)"string", to_getter<&Symbol::string>(), nullptr, (char *)R"(string: str

The value of a string.)", nullptr},
    {(char *)"number", to_getter<&Symbol::num>(), nullptr, (char *)R"(number: int

The value of a number.)", nullptr},
    {(char *)"arguments", to_getter<&Symbol::args>(), nullptr, (char *)R"(arguments: List[Symbol]

The arguments of a function.)", nullptr},
    {(char *)"negative", to_getter<&Symbol::negative>(), nullptr, (char *)R"(negative: bool

The inverted sign of a function.)", nullptr},
    {(char *)"positive", to_getter<&Symbol::positive>(), nullptr, (char *)R"(positive: bool

The sign of a function.)", nullptr},
    {(char *)"type", to_getter<&Symbol::type_>(), nullptr, (char *)R"(type: SymbolType

The type of the symbol.)", nullptr},
    {(char *)"_to_c", to_getter<&Symbol::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the underlying C `clingo_symbol_t`.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyObject *Symbol::inf = nullptr;
PyObject *Symbol::sup = nullptr;

// {{{1 wrap SolveResult

struct SolveResult : ObjectBase<SolveResult> {
    clingo_solve_result_bitset_t result;
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "SolveResult";
    static constexpr char const *tp_name = "clingo.SolveResult";
    static constexpr char const *tp_doc =
R"(Captures the result of a solve call.

`SolveResult` objects cannot be constructed from Python. Instead they are
returned by the solve methods of the Control object.)";
    static Object construct(clingo_solve_result_bitset_t result) {
        auto self = new_();
        self->result = result;
        return self;
    }
    Object satisfiable() {
        if (result & clingo_solve_result_satisfiable) { Py_RETURN_TRUE; }
        if (result & clingo_solve_result_unsatisfiable) { Py_RETURN_FALSE; }
        Py_RETURN_NONE;
    }
    Object unsatisfiable() {
        if (result & clingo_solve_result_satisfiable) { Py_RETURN_FALSE; }
        if (result & clingo_solve_result_unsatisfiable) { Py_RETURN_TRUE; }
        Py_RETURN_NONE;
    }
    Object unknown() {
        if (result & clingo_solve_result_satisfiable) { Py_RETURN_FALSE; }
        if (result & clingo_solve_result_unsatisfiable) { Py_RETURN_FALSE; }
        Py_RETURN_TRUE;
    }
    Object exhausted() {
        return cppToPy(static_cast<bool>(result & clingo_solve_result_exhausted)).release();
    }
    Object interrupted() {
        return cppToPy(static_cast<bool>(result & clingo_solve_result_interrupted)).release();
    }
    Object tp_repr() {
        if (result & clingo_solve_result_satisfiable) { return cppToPy("SAT"); }
        if (result & clingo_solve_result_unsatisfiable) { return cppToPy("UNSAT"); }
        return PyString_FromString("UNKNOWN");
    }
};

PyGetSetDef SolveResult::tp_getset[] = {
    {(char *)"satisfiable", to_getter<&SolveResult::satisfiable>(), nullptr,
(char *)R"(satisfiable: Optional[bool]

True if the problem is satisfiable, False if the problem is unsatisfiable, or
None if the satisfiablity is not known.
)", nullptr},
    {(char *)"unsatisfiable", to_getter<&SolveResult::unsatisfiable>(), nullptr,
(char *)R"(unsatisfiable: Optional[bool]

True if the problem is unsatisfiable, false if the problem is satisfiable, or
`None` if the satisfiablity is not known.
)", nullptr},
    {(char *)"unknown", to_getter<&SolveResult::unknown>(), nullptr,
(char *)R"(unknown: bool

True if the satisfiablity is not known.

This is equivalent to satisfiable is None.
)", nullptr},
    {(char *)"exhausted", to_getter<&SolveResult::exhausted>(), nullptr,
(char *)R"(exhausted: bool

True if the search space was exhausted.
)", nullptr},
    {(char *)"interrupted", to_getter<&SolveResult::interrupted>(), nullptr,
(char *)R"(interruped: bool

True if the search was interrupted.)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap SymbolicAtom

struct SymbolicAtom : public ObjectBase<SymbolicAtom> {
    clingo_symbolic_atoms_t const *atoms;
    clingo_symbolic_atom_iterator_t range;

    static constexpr char const *tp_type = "SymbolicAtom";
    static constexpr char const *tp_name = "clingo.SymbolicAtom";
    static constexpr char const *tp_doc = R"(Captures a symbolic atom and provides properties to inspect its state.)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static Object construct(clingo_symbolic_atoms_t const *atoms, clingo_symbolic_atom_iterator_t range) {
        auto self = new_();
        self->atoms = atoms;
        self->range = range;
        return self;
    }
    Object symbol() {
        clingo_symbol_t ret;
        handle_c_error(clingo_symbolic_atoms_symbol(atoms, range, &ret));
        return Symbol::construct(ret);
    }
    Object literal() {
        clingo_literal_t ret;
        handle_c_error(clingo_symbolic_atoms_literal(atoms, range, &ret));
        return cppToPy(ret);
    }
    Object is_fact() {
        bool ret;
        handle_c_error(clingo_symbolic_atoms_is_fact(atoms, range, &ret));
        return cppToPy(ret);
    }
    Object is_external() {
        bool ret;
        handle_c_error(clingo_symbolic_atoms_is_external(atoms, range, &ret));
        return cppToPy(ret);
    }
    Object match(Reference pyargs, Reference pykwds) {
        Object sym{symbol()};
        return reinterpret_cast<SharedObject<Symbol> &>(sym)->match(pyargs, pykwds);
    }
};

PyMethodDef SymbolicAtom::tp_methods[] = {
    {"match", to_function<&SymbolicAtom::match>(), METH_KEYWORDS | METH_VARARGS,
R"(match(self, name: str, arity: int) -> bool

Check if the atom matches the given signature.

Parameters
----------
name : str
    The name of the function.

arity : int
    The arity of the function.

Returns
-------
bool
    Whether the function matches.

See Also
--------
Symbol.match
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef SymbolicAtom::tp_getset[] = {
    {(char *)"symbol", to_getter<&SymbolicAtom::symbol>(), nullptr, (char *)R"(symbol: Symbol

The representation of the atom in form of a symbol.
)", nullptr},
    {(char *)"literal", to_getter<&SymbolicAtom::literal>(), nullptr, (char *)R"(literal: int

The program literal associated with the atom.
)", nullptr},
    {(char *)"is_fact", to_getter<&SymbolicAtom::is_fact>(), nullptr, (char *)R"(is_fact: bool

Whether the atom is a fact.
)", nullptr},
    {(char *)"is_external", to_getter<&SymbolicAtom::is_external>(), nullptr, (char *)R"(is_external: bool

Whether the atom is an external atom.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr},
};

// {{{1 wrap SymbolicAtomIter

struct SymbolicAtomIter : ObjectBase<SymbolicAtomIter> {
    clingo_symbolic_atoms_t const *atoms;
    clingo_symbolic_atom_iterator_t range;

    static constexpr char const *tp_type = "SymbolicAtomIter";
    static constexpr char const *tp_name = "clingo.SymbolicAtomIter";
    static constexpr char const *tp_doc = R"(Implements `Iterator[SymbolicAtom]`.

See Also
--------
SymbolicAtoms
)";

    static Object construct(clingo_symbolic_atoms_t const *atoms, clingo_symbolic_atom_iterator_t range) {
        auto self = new_();
        self->atoms = atoms;
        self->range = range;
        return self;
    }
    Reference tp_iter() { return *this; }
    Object tp_iternext() {
        auto current = range;
        bool valid;
        handle_c_error(clingo_symbolic_atoms_is_valid(atoms, current, &valid));
        if (valid) {
            handle_c_error(clingo_symbolic_atoms_next(atoms, current, &range));
            return SymbolicAtom::construct(atoms, current);
        }
        else {
            PyErr_SetNone(PyExc_StopIteration);
            return nullptr;
        }
    }
};

// {{{1 wrap SymbolicAtoms

struct SymbolicAtoms : ObjectBase<SymbolicAtoms> {
    clingo_symbolic_atoms_t const *atoms;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "SymbolicAtoms";
    static constexpr char const *tp_name = "clingo.SymbolicAtoms";
    static constexpr char const *tp_doc =
R"(This class provides read-only access to the atom base of the grounder.

It implements `Sequence[SymbolicAtom]` and `Mapping[Symbol,SymbolicAtom]`.

Examples
--------

    >>> import clingo
    >>> prg = clingo.Control()
    >>> prg.add('base', [], '''\
    ... p(1).
    ... { p(3) }.
    ... #external p(1..3).
    ...
    ... q(X) :- p(X).
    ... ''')
    >>> prg.ground([("base", [])])
    >>> len(prg.symbolic_atoms)
    6
    >>> prg.symbolic_atoms[clingo.Function("p", [2])] is not None
    True
    >>> prg.symbolic_atoms[clingo.Function("p", [4])] is None
    True
    >>> prg.symbolic_atoms.signatures
    [('p', 1L, True), ('q', 1L, True)]
    >>> [(x.symbol, x.is_fact, x.is_external)
    ...  for x in prg.symbolic_atoms.by_signature("p", 1)]
    [(p(1), True, False), (p(3), False, False), (p(2), False, True)]
)";

    static Object construct(clingo_symbolic_atoms_t const *atoms) {
        auto self = new_();
        self->atoms = atoms;
        return self;
    }

    Py_ssize_t mp_length() {
        size_t size;
        handle_c_error(clingo_symbolic_atoms_size(atoms, &size));
        return size;
    }

    Object tp_iter() {
        clingo_symbolic_atom_iterator_t ret;
        handle_c_error(clingo_symbolic_atoms_begin(atoms, nullptr, &ret));
        return SymbolicAtomIter::construct(atoms, ret);
    }

    Object mp_subscript(Reference key) {
        symbol_wrapper atom;
        pyToCpp(key, atom);
        clingo_symbolic_atom_iterator_t range;
        handle_c_error(clingo_symbolic_atoms_find(atoms, atom.symbol, &range));
        bool valid;
        handle_c_error(clingo_symbolic_atoms_is_valid(atoms, range, &valid));
        if (valid) { return SymbolicAtom::construct(atoms, range); }
        else       { Py_RETURN_NONE; }
    }

    bool sq_contains(Reference key) {
        symbol_wrapper atom;
        pyToCpp(key, atom);
        clingo_symbolic_atom_iterator_t range;
        handle_c_error(clingo_symbolic_atoms_find(atoms, atom.symbol, &range));
        bool valid;
        handle_c_error(clingo_symbolic_atoms_is_valid(atoms, range, &valid));
        return valid;
    }

    Object by_signature(Reference pyargs, Reference pykwds) {
        char const *name;
        int arity;
        PyObject *pos = Py_True;
        char const *kwlist[] = {"name", "arity", "positive", nullptr};
        ParseTupleAndKeywords(pyargs, pykwds, "si|O", kwlist, name, arity, pos);
        clingo_symbolic_atom_iterator_t ret;
        clingo_signature_t sig;
        handle_c_error(clingo_signature_create(name, arity, pyToCpp<bool>(pos), &sig));
        handle_c_error(clingo_symbolic_atoms_begin(atoms, &sig, &ret));
        return SymbolicAtomIter::construct(atoms, ret);
    }

    Object signatures() {
        size_t size;
        handle_c_error(clingo_symbolic_atoms_signatures_size(atoms, &size));
        std::vector<clingo_signature_t> ret(size);
        handle_c_error(clingo_symbolic_atoms_signatures(atoms, ret.data(), size));
        List pyRet;
        for (auto &sig : ret) {
            pyRet.append(Tuple{
                cppToPy(clingo_signature_name(sig)),
                cppToPy(clingo_signature_arity(sig)),
                cppToPy(clingo_signature_is_positive(sig))
            });
        }
        return pyRet;
    }

    Object to_c() {
        return PyLong_FromVoidPtr(const_cast<clingo_symbolic_atoms_t*>(atoms));
    }
};

PyMethodDef SymbolicAtoms::tp_methods[] = {
    {"by_signature", to_function<&SymbolicAtoms::by_signature>(), METH_KEYWORDS | METH_VARARGS,
R"(by_signature(self, name: str, arity: int, positive: bool=True) -> Iterator[Symbol]

Return an iterator over the symbolic atoms with the given signature.

Arguments
---------
name : str
    The name of the signature.
arity : int
    The arity of the signature.
positive : bool=True
    The sign of the signature.

Returns
-------
Iterator[Symbol]
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef SymbolicAtoms::tp_getset[] = {
    {(char *)"signatures", to_getter<&SymbolicAtoms::signatures>(), nullptr, (char *)
R"(signatures: List[Tuple[str,int,bool]]

The list of predicate signatures occurring in the program.

The Boolean indicates the sign of the signature.
)", nullptr},
    {(char *)"_to_c", to_getter<&SymbolicAtoms::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_symbolic_atoms_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap SolveControl

clingo_literal_t pyToAtom(Reference x, clingo_symbolic_atoms_t const *atoms) {
    if (PyNumber_Check(x.toPy())) {
        auto ret = pyToCpp<clingo_literal_t>(x);
        return ret;
    }
    else {
        clingo_symbol_t symbol = pyToCpp<symbol_wrapper>(x).symbol;
        clingo_symbolic_atom_iterator_t it;
        handle_c_error(clingo_symbolic_atoms_find(atoms, symbol, &it));
        bool valid;
        handle_c_error(clingo_symbolic_atoms_is_valid(atoms, it, &valid));
        if (valid) {
            clingo_literal_t lit;
            handle_c_error(clingo_symbolic_atoms_literal(atoms, it, &lit));
            return lit;
        }
    }
    return 0;
}

std::vector<clingo_literal_t> pyToLits(Reference pyLits, clingo_symbolic_atoms_t const *atoms, bool invert, bool disjunctive) {
    std::vector<clingo_literal_t> lits;
    for (auto x : pyLits.iter()) {
        if (PyNumber_Check(x.toPy())) {
            auto lit = pyToCpp<clingo_literal_t>(x);
            if (invert) { lit = -lit; }
            lits.emplace_back(lit);
        }
        else {
            symbolic_literal_t sym = pyToCpp<symbolic_literal_t>(x);
            if (invert) { sym.positive = !sym.positive; }
            clingo_symbolic_atom_iterator_t it;
            handle_c_error(clingo_symbolic_atoms_find(atoms, sym.symbol, &it));
            bool valid;
            handle_c_error(clingo_symbolic_atoms_is_valid(atoms, it, &valid));
            if (valid) {
                clingo_literal_t lit;
                handle_c_error(clingo_symbolic_atoms_literal(atoms, it, &lit));
                if (!sym.positive) { lit = -lit; }
                lits.emplace_back(lit);
            }
            else if (sym.positive != disjunctive) {
                lits.emplace_back(1);
                lits.emplace_back(-1);
            }
        }
    }
    return lits;
}

struct SolveControl : ObjectBase<SolveControl> {
    clingo_solve_control_t *ctl;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "SolveControl";
    static constexpr char const *tp_name = "clingo.SolveControl";
    static constexpr char const *tp_doc =
R"(Object that allows for controlling a running search.

`SolveControl` objects cannot be constructed from Python. Instead they are
available via `Model.context`.)";

    static Object construct(clingo_solve_control_t *ctl) {
        auto self = new_();
        self->ctl = ctl;
        return self;
    }

    Object getClause(Reference pyLits, bool invert) {
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_solve_control_symbolic_atoms(ctl, &atoms));
        auto lits = pyToLits(pyLits, atoms, invert, true);
        handle_c_error(clingo_solve_control_add_clause(ctl, lits.data(), lits.size()));
        Py_RETURN_NONE;
    }

    Object symbolicAtoms() {
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_solve_control_symbolic_atoms(ctl, &atoms));
        return SymbolicAtoms::construct(atoms);
    }

    Object add_clause(Reference pyLits) {
        return getClause(pyLits, false);
    }

    Object add_nogood(Reference pyLits) {
        return getClause(pyLits, true);
    }

    Object to_c() {
        return PyLong_FromVoidPtr(ctl);
    }
};

PyMethodDef SolveControl::tp_methods[] = {
    // add_clause
    {"add_clause", to_function<&SolveControl::add_clause>(), METH_O,
R"(add_clause(self, literals: List[Union[Tuple[Symbol,bool],int]]) -> None

Add a clause that applies to the current solving step during the search.

Parameters
----------
literals : List[Union[Tuple[Symbol,bool],int]]
    List of literals either represented as pairs of symbolic atoms and Booleans
    or as program literals.

Notes
-----
This function can only be called in a model callback or while iterating when
using a `SolveHandle`.
)"},
    // add_nogood
    {"add_nogood", to_function<&SolveControl::add_nogood>(), METH_O,
R"(add_nogood(self, literals: List[Union[Tuple[Symbol,bool],int]]) -> None

Equivalent to `SolveControl.add_clause` with the literals inverted.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef SolveControl::tp_getset[] = {
    {(char *)"symbolic_atoms", to_getter<&SolveControl::symbolicAtoms>(), nullptr, (char *)R"(symbolic_atoms: SymbolicAtoms

`SymbolicAtoms` object to inspect the symbolic atoms.
)", nullptr},
    {(char *)"_to_c", to_getter<&SolveControl::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_solve_control_t`
struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Model

struct ModelType : EnumType<ModelType> {
    static constexpr char const *tp_type = "ModelType";
    static constexpr char const *tp_name = "clingo.ModelType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of models.

`ModelType` objects have a readable string representation, implement Python's
rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
StableModel : ModelType
    The model captures a stable model.
BraveConsequences : ModelType
    The model stores the set of brave consequences.
CautiousConsequences : ModelType
    The model stores the set of cautious consequences.
)";

    static constexpr enum clingo_model_type const values[] = {
        clingo_model_type_stable_model,
        clingo_model_type_brave_consequences,
        clingo_model_type_cautious_consequences
    };
    static constexpr const char * const strings[] = { "StableModel", "BraveConsequences", "CautiousConsequences" };
};

constexpr enum clingo_model_type const ModelType::values[];
constexpr const char * const ModelType::strings[];

struct Model : ObjectBase<Model> {
    clingo_model_t const *model;
    clingo_model_t *mut_model;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "Model";
    static constexpr char const *tp_name = "clingo.Model";
    static constexpr char const *tp_doc =
R"(Provides access to a model during a solve call and provides a `SolveContext`
object to provided limited support to influence the running search.

Notes
-----
The string representation of a model object is similar to the output of models
by clingo using the default output.

`Model` objects cannot be constructed from Python. Instead they are obained
during solving (see `Control.solve`). Furthermore, the lifetime of a model
object is limited to the scope of the callback it was passed to or until the
search for the next model is started. They must not be stored for later use.

Examples
--------
The following example shows how to store atoms in a model for usage after
solving:

    >>> import clingo
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], "{a;b}.")
    >>> ctl.ground([("base", [])])
    >>> ctl.configuration.solve.models="0"
    >>> models = []
    >>> with ctl.solve(yield_=True) as handle:
    ...     for model in handle:
    ...         models.append(model.symbols(atoms=True))
    ...
    >>> sorted(models)
    [[], [a], [a, b], [b]]
)";

    static Object construct(clingo_model_t *model) {
        auto self = new_();
        self->mut_model = model;
        self->model = model;
        return self;
    }
    static Object construct(clingo_model_t const *model) {
        auto self = new_();
        self->mut_model = nullptr;
        self->model = model;
        return self;
    }
    Object contains(Reference arg) {
        symbol_wrapper val;
        pyToCpp(arg, val);
        bool ret;
        handle_c_error(clingo_model_contains(model, val.symbol, &ret));
        return cppToPy(ret);
    }
    Object is_true(Reference arg) {
        clingo_literal_t val;
        pyToCpp(arg, val);
        bool ret;
        handle_c_error(clingo_model_is_true(model, val, &ret));
        return cppToPy(ret);
    }
    Object atoms(Reference pyargs, Reference pykwds) {
        clingo_show_type_bitset_t atomset = 0;
        static char const *kwlist[] = {"atoms", "terms", "shown", "csp", "complement", nullptr};
        Reference pyAtoms = Py_False, pyTerms = Py_False, pyShown = Py_False, pyCSP = Py_False, pyComp = Py_False;
        ParseTupleAndKeywords(pyargs, pykwds, "|OOOOO", const_cast<char**>(kwlist), pyAtoms, pyTerms, pyShown, pyCSP, pyComp);
        if (pyToCpp<bool>(pyAtoms)) { atomset |= clingo_show_type_atoms; }
        if (pyToCpp<bool>(pyTerms)) { atomset |= clingo_show_type_terms; }
        if (pyToCpp<bool>(pyShown)) { atomset |= clingo_show_type_shown; }
        if (pyToCpp<bool>(pyCSP))   { atomset |= clingo_show_type_csp; }
        if (pyToCpp<bool>(pyComp))  { atomset |= clingo_show_type_complement; }
        size_t size;
        handle_c_error(clingo_model_symbols_size(model, atomset, &size));
        std::vector<symbol_wrapper> ret(size);
        auto fst = reinterpret_cast<clingo_symbol_t*>(ret.data());
        handle_c_error(clingo_model_symbols(model, atomset, fst, size));
        return cppToPy(ret);
    }
    Object cost() {
        size_t size;
        handle_c_error(clingo_model_cost_size(model, &size));
        std::vector<int64_t> ret(size);
        handle_c_error(clingo_model_cost(model, ret.data(), size));
        return cppToPy(ret);
    }
    Object thread_id() {
        clingo_id_t id;
        handle_c_error(clingo_model_thread_id(model, &id));
        return cppToPy(id);
    }
    Object optimality_proven() {
        bool ret;
        handle_c_error(clingo_model_optimality_proven(model, &ret));
        return cppToPy(ret);
    }
    Object number() {
        uint64_t ret;
        handle_c_error(clingo_model_number(model, &ret));
        return cppToPy(ret);
    }
    Object model_type() {
        clingo_model_type_t ret;
        handle_c_error(clingo_model_type(model, &ret));
        return ModelType::getAttr(ret);
    }
    Object tp_repr() {
        std::vector<char> buf;
        auto printSymbol = [&buf](std::ostream &out, clingo_symbol_t val) {
            size_t size;
            handle_c_error(clingo_symbol_to_string_size(val, &size));
            buf.resize(size);
            handle_c_error(clingo_symbol_to_string(val, buf.data(), size));
            out << buf.data();
        };
        auto printAtom = [printSymbol](std::ostream &out, clingo_symbol_t val) {
            if (clingo_symbol_type_function == clingo_symbol_type(val)) {
                char const *name;
                clingo_symbol_t const *args;
                size_t size;
                handle_c_error(clingo_symbol_name(val, &name));
                handle_c_error(clingo_symbol_arguments(val, &args, &size));
                if (size == 2 && strcmp(name, "$") == 0) {
                    printSymbol(out, args[0]);
                    out << "=";
                    printSymbol(out, args[1]);
                }
                else { printSymbol(out, val); }
            }
            else { printSymbol(out, val); }
        };
        std::ostringstream oss;
        size_t size;
        handle_c_error(clingo_model_symbols_size(model, clingo_show_type_shown, &size));
        std::vector<clingo_symbol_t> ret(size);
        handle_c_error(clingo_model_symbols(model, clingo_show_type_shown, ret.data(), size));
        bool comma = false;
        for (auto &&x : ret) {
            if (comma) { oss << " "; }
            else       { comma = true; }
            printAtom(oss, x);
        }
        return cppToPy(oss.str().c_str());
    }
    Object getContext() {
        clingo_solve_control_t *ctl;
        handle_c_error(clingo_model_context(model, &ctl));
        return SolveControl::construct(ctl);
    }
    Object extend(Reference symbols) {
        auto syms = pyToCpp<std::vector<symbol_wrapper>>(symbols);
        if (!mut_model) {
            throw std::runtime_error("models can only be extended from on_model callback");
        }
        handle_c_error(clingo_model_extend(mut_model, reinterpret_cast<clingo_symbol_t const *>(syms.data()), syms.size()));
        return None();
    }
    Object to_c() {
        return PyLong_FromVoidPtr(const_cast<clingo_model_t*>(model));
    }
};

PyGetSetDef Model::tp_getset[] = {
    {(char *)"thread_id", to_getter<&Model::thread_id>(), nullptr, (char*)R"(thread_id: int

The id of the thread which found the model.
)", nullptr},
    {(char *)"context", to_getter<&Model::getContext>(), nullptr, (char*)R"(
context: SolveControl

Object that allows for controlling the running search.
)", nullptr},
    {(char *)"cost", to_getter<&Model::cost>(), nullptr,
(char *)R"(cost: List[int]

Return the list of integer cost values of the model.

The return values correspond to clasp's cost output.
)", nullptr},
    {(char *)"optimality_proven", to_getter<&Model::optimality_proven>(), nullptr, (char*)R"(optimality_proven: bool

Whether the optimality of the model has been proven.
)", nullptr},
    {(char *)"number", to_getter<&Model::number>(), nullptr, (char*)R"(number: int

The running number of the model.
)", nullptr},
    {(char *)"type", to_getter<&Model::model_type>(), nullptr, (char*)R"(type: ModelType

The type of the model.
)", nullptr},
    {(char *)"_to_c", to_getter<&Model::to_c>(), nullptr, (char *)R"(_to_c: int
An int representing the pointer to the underlying C `clingo_model_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

PyMethodDef Model::tp_methods[] = {
    {"symbols", to_function<&Model::atoms>(), METH_VARARGS | METH_KEYWORDS,
R"(symbols(self, atoms: bool=False, terms: bool=False, shown: bool=False, csp: bool=False, complement: bool=False) -> List[Symbol]

Return the list of atoms, terms, or CSP assignments in the model.

Parameters
----------
atoms : bool=False
    Select all atoms in the model (independent of `#show` statements).
terms : bool=False
    Select all terms displayed with `#show` statements in the model.
shown : bool
    Select all atoms and terms as outputted by clingo.
csp : bool
    Select all csp assignments (independent of `#show` statements).
complement : bool
    Return the complement of the answer set w.r.t. to the atoms known to the
    grounder. (Does not affect csp assignments.)

Returns
-------
List[Symbol]
    The selected symbols.

Notes
-----
Atoms are represented using functions (`Symbol` objects), and CSP assignments
are represented using functions with name `"$"` where the first argument is the
name of the CSP variable and the second its value.
)"},
    {"contains", to_function<&Model::contains>(), METH_O,
R"(contains(self, atom: Symbol) -> bool

Efficiently check if an atom is contained in the model.

Parameters
----------
atom : Symbol
    The atom to lookup.

Returns
-------
bool
    Whether the given atom is contained in the model.

Notes
-----
The atom must be represented using a function symbol.
)"},
    {"is_true", to_function<&Model::is_true>(), METH_O,
R"(is_true(self, literal: int) -> bool

Check if the given program literal is true.

Parameters
----------
literal : int
    The given program literal.

Returns
-------
bool
    Whether the given program literal is true.
)"},
    {"extend", to_function<&Model::extend>(), METH_O,
R"(extend(self, symbols: List[Symbol]) -> None

Extend a model with the given symbols.

Parameters
----------
symbols : List[Symbol]
    The symbols to add to the model.

Returns
-------
None

Notes
-----
This only has an effect if there is an underlying clingo application, which
will print the added symbols.
)"},
    {nullptr, nullptr, 0, nullptr}

};

// {{{1 wrap SolveHandle

Object getUserStatistics(clingo_statistics_t *stats, uint64_t key);
struct SolveHandle : ObjectBase<SolveHandle> {
    clingo_solve_handle_t *handle;
    Object on_model;
    Object on_finish;
    Object on_statistics;

    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "SolveHandle";
    static constexpr char const *tp_name = "clingo.SolveHandle";
    static constexpr char const *tp_doc =
R"(Handle for solve calls.

`SolveHandle` objects cannot be created from Python. Instead they are returned
by `Control.solve`. They can be used to control solving, like, retrieving
models or cancelling a search.

See Also
--------
Control.solve

Notes
-----
A `SolveHandle` is a context manager and must be used with Python's `with`
statement.

Blocking functions in this object release the GIL. They are not thread-safe
though.
)";

    static SharedObject<SolveHandle> construct() {
        auto ret = new_();
        ret->handle = nullptr;
        new (&ret->on_model) Object{};
        new (&ret->on_finish) Object{};
        new (&ret->on_statistics) Object{};
        return ret;
    }

    void tp_traverse(Traverse const &visit) {
        visit(on_model);
        visit(on_finish);
        visit(on_statistics);
    }

    void tp_clear() {
        if (handle) {
            auto tmp = handle;
            handle = nullptr;
            try         { doUnblocked([tmp](){ handle_c_error(clingo_solve_handle_close(tmp)); }); }
            catch (...) { }
        }
        on_model.clear();
        on_finish.clear();
        on_statistics.clear();
    }

    void tp_dealloc() {
        tp_clear();
    }

    clingo_solve_event_callback_t notify(clingo_solve_event_callback_t event, Reference mh, Reference sh, Reference fh) {
        on_model      = !mh.is_none() ? mh : Reference{};
        on_statistics = !sh.is_none() ? sh : Reference{};
        on_finish     = !fh.is_none() ? fh : Reference{};
        return on_model.valid() || on_finish.valid() || on_statistics.valid() ? event : nullptr;
    }

    Reference tp_iter() { return *this; }

    Object tp_iternext() {
        if (clingo_model_t const *m = doUnblocked([this]() {
            clingo_model_t const *ret;
            handle_c_error(clingo_solve_handle_resume(handle));
            handle_c_error(clingo_solve_handle_model(handle, &ret));
            return ret;
        })) {
            return Model::construct(m);
        } else {
            PyErr_SetNone(PyExc_StopIteration);
            return nullptr;
        }
    }

    Object enter() { return Reference{*this}; }

    Object exit() {
        std::exception_ptr except;
        if (handle) {
            try         { doUnblocked([this](){ handle_c_error(clingo_solve_handle_close(handle)); }); }
            catch (...) { except = std::current_exception(); }
            handle = nullptr;
        }
        on_model = nullptr;
        on_finish = nullptr;
        on_statistics = nullptr;
        if (except) { std::rethrow_exception(except); }
        Py_RETURN_FALSE;
    }

    Object get() {
        return SolveResult::construct(doUnblocked([this]() {
            clingo_solve_result_bitset_t result;
            handle_c_error(clingo_solve_handle_get(handle, &result));
            return result;
        }));
    }

    Object user_statistics_(clingo_statistics_t *stats) {
        uint64_t root;
        handle_c_error(clingo_statistics_root(stats, &root));
        return getUserStatistics(stats, root);
    }

    Object wait(Reference args) {
        Reference timeout = Py_None;
        ParseTuple(args, "|O", timeout);
        auto time = timeout.is_none() ? -1 : pyToCpp<double>(timeout);
        return cppToPy(doUnblocked([this, time](){
            bool ret;
            clingo_solve_handle_wait(handle, time, &ret);
            return ret;
        }));
    }

    Object resume() {
        doUnblocked([this](){
            handle_c_error(clingo_solve_handle_resume(handle));
        });
        Py_RETURN_NONE;
    }

    Object cancel() {
        doUnblocked([this](){ handle_c_error(clingo_solve_handle_cancel(handle)); });
        Py_RETURN_NONE;
    }

    static bool on_event(clingo_solve_event_type_t type, void *event, void *data, bool *goon) {
        auto &handle = *static_cast<SolveHandle*>(data);
        switch (type) {
            case clingo_solve_event_type_model: {
                if (handle.on_model.valid()) {
                    PyBlock block;
                    try {
                        auto pyModel = Model::construct(static_cast<clingo_model_t*>(event));
                        Object ret = handle.on_model(pyModel);
                        *goon = ret.is_none() || pyToCpp<bool>(ret);
                    }
                    catch (...) {
                        handle_cxx_error("<on_model>", "error in model callback");
                        return false;
                    }
                }
                return true;
            }
            case clingo_solve_event_type_statistics: {
                if (handle.on_statistics.valid()) {
                    PyBlock block;
                    try {
                        auto stats = static_cast<clingo_statistics_t **>(event);
                        handle.on_statistics(handle.user_statistics_(stats[0]), handle.user_statistics_(stats[1]));
                    }
                    catch (...) {
                        handle_cxx_error("<on_finish>", "error in finish callback");
                        return false;
                    }
                }
                return true;
            }
            case clingo_solve_event_type_finish: {
                if (handle.on_finish.valid()) {
                    PyBlock block;
                    try {
                        auto ret = SolveResult::construct(*static_cast<clingo_solve_result_bitset_t*>(event));
                        handle.on_finish(ret);
                    }
                    catch (...) {
                        handle_cxx_error("<on_finish>", "error in finish callback");
                        return false;
                    }
                }
                return true;
            }
        }
        return true;
    }

    Object to_c() {
        return PyLong_FromVoidPtr(handle);
    }
};

#define CLINGO_PY_NEXT "__next__"

PyMethodDef SolveHandle::tp_methods[] = {
    {"get", to_function<&SolveHandle::get>(), METH_NOARGS,
R"(get(self) -> SolveResult

Get the result of a solve call.

If the search is not completed yet, the function blocks until the result is
ready.

Returns
-------
SolveResult
)"},
    {"wait", to_function<&SolveHandle::wait>(),  METH_VARARGS,
R"(wait(self, timeout: Optional[float]=None) -> Optional[bool]

Wait for solve call to finish with an optional timeout.

Parameters
----------
timeout : Optional[float]=None
    If a timeout is given, the function blocks for at most timeout seconds.

Returns
-------
Optional[bool]
    If a timout is given, returns a Boolean indicating whether the search has
    finished. Otherwise, the function blocks until the search is finished and
    returns nothing.
)"},
    {"cancel", to_function<&SolveHandle::cancel>(), METH_NOARGS,
R"(cancel(self) -> None

Cancel the running search.

Returns
-------
None

See Also
--------
Control.interrupt
)"},
    {"resume", to_function<&SolveHandle::resume>(), METH_NOARGS,
R"(resume(self) -> None

Discards the last model and starts searching for the next one.

Notes
-----
If the search has been started asynchronously, this function also starts the
search in the background. A model that was not yet retrieved by calling `)"
CLINGO_PY_NEXT R"(` is not discared.)"},
    {"__enter__", to_function<&SolveHandle::enter>(), METH_NOARGS,
R"(__enter__(self) -> SolveHandle

Returns self.)"},
    {"__exit__", to_function<&SolveHandle::exit>(), METH_VARARGS,
R"(__exit__(self, type : Optional[Type[BaseException]], value : Optional[BaseException], traceback : Optional[TracebackType]) -> bool

Follows Python's __exit__ conventions. Does not suppress exceptions.

Stops the current search. It is necessary to call this method after each
search.)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef SolveHandle::tp_getset[] = {
    {(char *)"_to_c", to_getter<&SolveHandle::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_solve_handle_t`
struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};
// {{{1 wrap Configuration

struct Configuration : ObjectBase<Configuration> {
    clingo_configuration_t *conf;
    clingo_id_t key;
    static PyGetSetDef tp_getset[];

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

To list the subgroups of an option group, use the `Configuration.keys` member.
Array option groups, like solver, have a non-negative length and can be
iterated. Furthermore, there are meta options having key `configuration`.
Assigning a meta option sets a number of related options.  To get further
information about an option or option group `<opt>`, use property
`__desc_<opt>` to retrieve a description.

Notes
-----
When integers are assigned to options, they are automatically converted to
strings. The value of an option is always a string.

Examples
--------
The following example shows how to modify the configuration to enumerate all
models:

    >>> import clingo
    >>> prg = clingo.Control()
    >>> prg.configuration.solve.__desc_models
    'Compute at most %A models (0 for all)\n'
    >>> prg.configuration.solve.models = 0
    >>> prg.add("base", [], "{a;b}.")
    >>> prg.ground([("base", [])])
    >>> prg.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer:
    Answer: a
    Answer: b
    Answer: a b
    SAT
)";

    static Object construct(unsigned key, clingo_configuration_t *conf) {
        auto self = new_();
        self->conf = conf;
        self->key  = key;
        return self;
    }

    Object keys() {
        clingo_configuration_type_bitset_t type;
        handle_c_error(clingo_configuration_type(conf, key, &type));
        List list;
        if (type & clingo_configuration_type_map) {
            size_t size;
            handle_c_error(clingo_configuration_map_size(conf, key, &size));
            for (size_t i = 0; i < size; ++i) {
                char const *name;
                handle_c_error(clingo_configuration_map_subkey_name(conf, key, i, &name));
                list.append(cppToPy(name));
            }
        }
        return list;
    }

    Object tp_getattro(Reference name) {
        auto current_ = pyToCpp<std::string>(name);
        char const *current = current_.c_str();
        bool desc = strncmp("__desc_", current, 7) == 0;
        if (desc) { current += 7; }
        clingo_configuration_type_bitset_t type;
        handle_c_error(clingo_configuration_type(conf, key, &type));
        if (type & clingo_configuration_type_map) {
            bool haskey;
            handle_c_error(clingo_configuration_map_has_subkey(conf, key, current, &haskey));
            if (haskey) {
                clingo_id_t subkey;
                handle_c_error(clingo_configuration_map_at(conf, key, current, &subkey));
                if (desc) {
                    char const *ret;
                    handle_c_error(clingo_configuration_description(conf, subkey, &ret));
                    return cppToPy(ret);
                }
                else {
                    handle_c_error(clingo_configuration_type(conf, subkey, &type));
                    if (type & clingo_configuration_type_value) {
                        bool assigned;
                        handle_c_error(clingo_configuration_value_is_assigned(conf, subkey, &assigned));
                        if (!assigned) { Py_RETURN_NONE; }
                        size_t size;
                        handle_c_error(clingo_configuration_value_get_size(conf, subkey, &size));
                        std::vector<char> ret(size);
                        handle_c_error(clingo_configuration_value_get(conf, subkey, ret.data(), size));
                        return cppToPy(ret.data());
                    }
                    else { return construct(subkey, conf); }
                }
            }
        }
        return PyObject_GenericGetAttr(toPy(), name.toPy());
    }

    void tp_setattro(Reference name, Reference pyValue) {
        auto current = pyToCpp<std::string>(name);
        clingo_id_t subkey;
        handle_c_error(clingo_configuration_map_at(conf, key, current.c_str(), &subkey));
        handle_c_error(clingo_configuration_value_set(conf, subkey, pyToCpp<std::string>(pyValue).c_str()));
    }

    Py_ssize_t sq_length() {
        clingo_configuration_type_bitset_t type;
        handle_c_error(clingo_configuration_type(conf, key, &type));
        size_t size = 0;
        if (type & clingo_configuration_type_array) {
            handle_c_error(clingo_configuration_array_size(conf, key, &size));
        }
        return size;
    }

    Object sq_item(Py_ssize_t index) {
        if (index < 0 || index >= sq_length()) {
            PyErr_Format(PyExc_IndexError, "invalid index");
            return nullptr;
        }
        clingo_id_t subkey;
        handle_c_error(clingo_configuration_array_at(conf, key, index, &subkey));
        return construct(subkey, conf);
    }

    Object to_c() {
        return PyLong_FromVoidPtr(conf);
    }
};

PyGetSetDef Configuration::tp_getset[] = {
    // keys
    {(char *)"keys", to_getter<&Configuration::keys>(), nullptr, (char *)R"(keys: Optional[List[str]]

The list of names of sub-option groups or options.

The list is `None` if the current object is not an option group.
)", nullptr},
    {(char *)"_to_c", to_getter<&Configuration::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_configuration_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Assignment

struct Assignment : ObjectBase<Assignment> {
    clingo_assignment_t const *assign;
    static constexpr char const *tp_type = "Assignment";
    static constexpr char const *tp_name = "clingo.Assignment";
    static constexpr char const *tp_doc = R"(Object to inspect the (parital) assignment of an associated solver.

Assigns truth values to solver literals.  Each solver literal is either true,
false, or undefined, represented by the Python constants `True`, `False`, or
`None`, respectively.)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static Object construct(clingo_assignment_t const *assign) {
        auto self = new_();
        self->assign = assign;
        return self;
    }

    Object hasConflict() {
        return cppToPy(clingo_assignment_has_conflict(assign));
    }

    Object decisionLevel() {
        return cppToPy(clingo_assignment_decision_level(assign));
    }

    Object rootLevel() {
        return cppToPy(clingo_assignment_root_level(assign));
    }

    Object hasLit(Reference lit) {
        return cppToPy(clingo_assignment_has_literal(assign, pyToCpp<clingo_literal_t>(lit)));
    }

    Object level(Reference lit) {
        uint32_t ret;
        handle_c_error(clingo_assignment_level(assign, pyToCpp<clingo_literal_t>(lit), &ret));
        return cppToPy(ret);
    }

    Object decision(Reference level) {
        clingo_literal_t ret;
        handle_c_error(clingo_assignment_decision(assign, pyToCpp<uint32_t>(level), &ret));
        return cppToPy(ret);
    }

    Object isFixed(Reference lit) {
        bool ret;
        handle_c_error(clingo_assignment_is_fixed(assign, pyToCpp<clingo_literal_t>(lit), &ret));
        return cppToPy(ret);
    }

    Object truthValue(Reference lit) {
        clingo_truth_value_t ret;
        handle_c_error(clingo_assignment_truth_value(assign, pyToCpp<clingo_literal_t>(lit), &ret));
        if (ret == clingo_truth_value_true)  { Py_RETURN_TRUE; }
        if (ret == clingo_truth_value_false) { Py_RETURN_FALSE; }
        Py_RETURN_NONE;
    }

    Object isTrue(Reference lit) {
        bool ret;
        handle_c_error(clingo_assignment_is_true(assign, pyToCpp<clingo_literal_t>(lit), &ret));
        return cppToPy(ret);
    }

    Object isFalse(Reference lit) {
        bool ret;
        handle_c_error(clingo_assignment_is_false(assign, pyToCpp<clingo_literal_t>(lit), &ret));
        return cppToPy(ret);
    }

    Object size() {
        return cppToPy(clingo_assignment_size(assign));
    }

    Object max_size() {
        return cppToPy(clingo_assignment_max_size(assign));
    }

    Object isTotal() {
        return cppToPy(clingo_assignment_is_total(assign));
    }

    Object to_c() {
        return PyLong_FromVoidPtr(const_cast<clingo_assignment_t*>(assign));
    }
};

PyMethodDef Assignment::tp_methods[] = {
    {"has_literal", to_function<&Assignment::hasLit>(), METH_O, R"(has_literal(self, literal : int) -> bool

Determine if the given literal is valid in this solver.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
bool
)"},
    {"value", to_function<&Assignment::truthValue>(), METH_O, R"(value(self, literal) -> Optional[bool]

Get the truth value of the given literal or `None` if it has none.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
Optional[bool]
)"},
    {"level", to_function<&Assignment::level>(), METH_O, R"(level(self, literal: int) -> int

The decision level of the given literal.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
int

Notes
-----
Note that the returned value is only meaningful if the literal is assigned -
i.e., `value(lit) is not None`.
)"},
    {"is_fixed", to_function<&Assignment::isFixed>(), METH_O, R"(is_fixed(self, literal: int) -> bool

Determine if the literal is assigned on the top level.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
bool
)"},
    {"is_true", to_function<&Assignment::isTrue>(), METH_O, R"(is_true(self, literal: int) -> bool

Determine if the literal is true.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
bool
)"},
    {"is_false", to_function<&Assignment::isFalse>(), METH_O, R"(is_false(self, literal: int) -> bool

Determine if the literal is false.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
bool
)"},
    {"decision", to_function<&Assignment::decision>(), METH_O, R"(decision(self, level: int) -> int

Return the decision literal of the given level.

Parameters
----------
literal : int
    The solver literal.

Returns
-------
int
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef Assignment::tp_getset[] = {
    {(char *)"has_conflict", to_getter<&Assignment::hasConflict>(), nullptr, (char *)R"(has_conflict: bool

True if the assignment is conflicting.)", nullptr},
    {(char *)"decision_level", to_getter<&Assignment::decisionLevel>(), nullptr, (char *)R"(decision_level: int

The current decision level.)", nullptr},
    {(char *)"root_level", to_getter<&Assignment::rootLevel>(), nullptr, (char *)R"(root_level: int

The current root level.)", nullptr},
    {(char *)"size", to_getter<&Assignment::size>(), nullptr, (char *)R"(size: int

The number of assigned literals.)", nullptr},
    {(char *)"max_size", to_getter<&Assignment::max_size>(), nullptr, (char *)R"(max_size: int

The maximum size of the assignment (if all literals are assigned).)", nullptr},
    {(char *)"is_total", to_getter<&Assignment::isTotal>(), nullptr, (char *)R"(is_total: bool

Whether the assignment is total.)", nullptr},
    {(char *)"_to_c", to_getter<&Assignment::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_assignment_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap PropagateInit

struct PropagatorCheckMode : EnumType<PropagatorCheckMode> {
    using Type = clingo_propagator_check_mode;
    static constexpr char const *tp_type = "PropagatorCheckMode";
    static constexpr char const *tp_name = "clingo.PropagatorCheckMode";
    static constexpr char const *tp_doc =
R"(Enumeration of supported check modes for propagators.

`PropagatorCheckMode` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Off : PropagatorCheckMode
    Do not call `Propagator.check` at all.
Total : PropagatorCheckMode
    Call `Propagator.check` on total assignments.
Fixpoint : PropagatorCheckMode
    Call `Propagator.check` on propagation fixpoints.
)";

    static constexpr Type const values[] = {
        clingo_propagator_check_mode_none,
        clingo_propagator_check_mode_total,
        clingo_propagator_check_mode_fixpoint,
    };
    static constexpr const char * const strings[] = {
        "Off",
        "Total",
        "Fixpoint",
    };
};

constexpr PropagatorCheckMode::Type const PropagatorCheckMode::values[];
constexpr const char * const PropagatorCheckMode::strings[];

struct PropagateInit : ObjectBase<PropagateInit> {
    clingo_propagate_init_t *init;
    static constexpr char const *tp_type = "PropagateInit";
    static constexpr char const *tp_name = "clingo.PropagateInit";
    static constexpr char const *tp_doc =
R"(Object that is used to initialize a propagator before each solving step.

See Also
--------
Control.register_propagator
)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    using ObjectBase<PropagateInit>::new_;
    static Object construct(clingo_propagate_init_t *init) {
        auto self = new_();
        self->init = init;
        return self;
    }

    Object theoryIter() {
        clingo_theory_atoms_t const *atoms;
        handle_c_error(clingo_propagate_init_theory_atoms(init, &atoms));
        return TheoryAtomIter::construct(atoms, 0);
    }

    Object symbolicAtoms() {
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_propagate_init_symbolic_atoms(init, &atoms));
        return SymbolicAtoms::construct(atoms);
    }

    Object assignment() {
        return Assignment::construct(clingo_propagate_init_assignment(init));
    }

    Object numThreads() {
        return cppToPy(clingo_propagate_init_number_of_threads(init));
    }

    Object mapLit(Reference lit) {
        clingo_literal_t ret;
        handle_c_error(clingo_propagate_init_solver_literal(init, pyToCpp<clingo_literal_t>(lit), &ret));
        return cppToPy(ret);
    }

    Object addWatch(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"literal", "thread_id", nullptr};
        Reference lit, thread_id = Py_None;
        ParseTupleAndKeywords(pyargs, pykwds, "O|O", kwlist, lit, thread_id);
        if (!thread_id.is_none()) {
            handle_c_error(clingo_propagate_init_add_watch_to_thread(init, pyToCpp<clingo_literal_t>(lit), pyToCpp<uint32_t>(thread_id)));
        }
        else {
            handle_c_error(clingo_propagate_init_add_watch(init, pyToCpp<clingo_literal_t>(lit)));
        }
        Py_RETURN_NONE;
    }

    Object addLiteral() {
        clingo_literal_t ret;
        handle_c_error(clingo_propagate_init_add_literal(init, &ret));
        return cppToPy(ret);
    }

    Object addClause(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"clause", nullptr};
        Reference pyClause;
        ParseTupleAndKeywords(pyargs, pykwds, "O", kwlist, pyClause);
        auto clause = pyToCpp<std::vector<clingo_literal_t>>(pyClause);
        bool ret;
        handle_c_error(clingo_propagate_init_add_clause(init, clause.data(), clause.size(), &ret));
        return cppToPy(ret);
    }

    Object getCheckMode() {
        return PropagatorCheckMode::getAttr(clingo_propagate_init_get_check_mode(init));
    }

    void setCheckMode(Reference value) {
        clingo_propagate_init_set_check_mode(init, enumValue<PropagatorCheckMode>(value));
    }

    Object to_c() {
        return PyLong_FromVoidPtr(init);
    }
};

PyMethodDef PropagateInit::tp_methods[] = {
    {"add_watch", to_function<&PropagateInit::addWatch>(), METH_KEYWORDS | METH_VARARGS, R"(add_watch(self, literal: int, thread_id: Optional[int]=None) -> None

Add a watch for the solver literal in the given phase.


Parameters
----------
literal : int
    The solver literal to watch.
thread_id : Optional[int]
    The id of the thread to watch the literal. If the is `None` then all active
    threads will watch the literal.

Returns
-------
None
)"},
    {"solver_literal", to_function<&PropagateInit::mapLit>(), METH_O, R"(solver_literal(self, literal: int) -> int

Maps the given program literal or condition id to its solver literal.

Parameters
----------
literal : int
    A program literal or condition id.

Returns
-------
int
    A solver literal.
)"},
    {"add_literal", to_function<&PropagateInit::addLiteral>(), METH_NOARGS, R"(add_literal(self) -> int

Statically adds a literal to the solver.

Returns
-------
int
    Returns the added literal.
)"},
    {"add_clause", to_function<&PropagateInit::addClause>(), METH_KEYWORDS | METH_VARARGS, R"(add_clause(self, clause: List[int]) -> bool

Statically adds the given clause to the problem.

Parameters
----------
clause : List[int]
    The clause over solver literals to add.

Returns
-------
bool
    The return value is always true and should be ignored.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef PropagateInit::tp_getset[] = {
    {(char *)"symbolic_atoms", to_getter<&PropagateInit::symbolicAtoms>(), nullptr, (char *)R"(symbolic_atoms: SymbolicAtoms

The symbolic atoms captured by a `SymbolicAtoms` object.
)", nullptr},
    {(char *)"theory_atoms", to_getter<&PropagateInit::theoryIter>(), nullptr, (char *)R"(theory_atom: TheoryAtomIter

A `TheoryAtomIter` object to iterate over all theory atoms.
)", nullptr},
    {(char *)"number_of_threads", to_getter<&PropagateInit::numThreads>(), nullptr, (char *) R"(number_of_threads: int

The number of solver threads used in the corresponding solve call.
)", nullptr},
    {(char *)"check_mode", to_getter<&PropagateInit::getCheckMode>(), to_setter<&PropagateInit::setCheckMode>(), (char *) R"(check_mode: PropagatorCheckMode

`PropagatorCheckMode` controlling when to call `Propagator.check`.)", nullptr},
    {(char *)"assignment", to_getter<&PropagateInit::assignment>(), nullptr, (char *)R"(assignment: Assignment

`Assignment` object capturing the top level assignment.
)", nullptr},
    {(char *)"_to_c", to_getter<&PropagateInit::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_propagate_init_t`
struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap PropagateControl

struct PropagateControl : ObjectBase<PropagateControl> {
    clingo_propagate_control_t* ctl;
    static constexpr char const *tp_type = "PropagateControl";
    static constexpr char const *tp_name = "clingo.PropagateControl";
    static constexpr char const *tp_doc = R"(This object can be used to add clauses and to propagate them.

See Also
--------
Control.register_propagator
)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    using ObjectBase<PropagateControl>::new_;
    static Object construct(clingo_propagate_control_t *ctl) {
        auto self = new_();
        self->ctl = ctl;
        return self;
    }

    Object id() {
        return cppToPy(clingo_propagate_control_thread_id(ctl));
    }

    Object addClauseOrNogood(Reference pyargs, Reference pykwds, bool invert) {
        static char const *kwlist[] = {"clause", "tag", "lock", nullptr};
        Reference clause;
        Reference pyTag = Py_False;
        Reference pyLock = Py_False;
        ParseTupleAndKeywords(pyargs, pykwds, "O|OO", kwlist, clause, pyTag, pyLock);
        auto lits = pyToCpp<std::vector<clingo_literal_t>>(clause);
        if (invert) {
            for (auto &lit : lits) { lit = -lit; }
        }
        clingo_clause_type_t type = 0;
        if (pyToCpp<bool>(pyTag))  { type |= clingo_clause_type_volatile; }
        if (pyToCpp<bool>(pyLock)) { type |= clingo_clause_type_static; }
        return cppToPy(doUnblocked([this, &lits, type](){
            bool ret;
            handle_c_error(clingo_propagate_control_add_clause(ctl, lits.data(), lits.size(), type, &ret));
            return ret;
        }));
    }

    Object addNogood(Reference pyargs, Reference pykwds) {
        return addClauseOrNogood(pyargs, pykwds, true);
    }

    Object addClause(Reference pyargs, Reference pykwds) {
        return addClauseOrNogood(pyargs, pykwds, false);
    }

    Object propagate() {
        return cppToPy(doUnblocked([this](){
            bool ret;
            handle_c_error(clingo_propagate_control_propagate(ctl, &ret));
            return ret;
        }));
    }

    Object add_literal() {
        clingo_literal_t ret;
        handle_c_error(clingo_propagate_control_add_literal(ctl, &ret));
        return cppToPy(ret);
    }

    Object add_watch(Reference pyLit) {
        handle_c_error(clingo_propagate_control_add_watch(ctl, pyToCpp<clingo_literal_t>(pyLit)));
        return None();
    }

    Object remove_watch(Reference pyLit) {
        clingo_propagate_control_remove_watch(ctl, pyToCpp<clingo_literal_t>(pyLit));
        return None();
    }

    Object has_watch(Reference pyLit) {
        return cppToPy(clingo_propagate_control_has_watch(ctl, pyToCpp<clingo_literal_t>(pyLit)));
    }

    Object assignment() {
        return Assignment::construct(clingo_propagate_control_assignment(ctl));
    }

    Object to_c() {
        return PyLong_FromVoidPtr(ctl);
    }
};

PyMethodDef PropagateControl::tp_methods[] = {
    {"add_literal", to_function<&PropagateControl::add_literal>(), METH_NOARGS, R"(add_literal(self) -> int

Adds a new positive volatile literal to the underlying solver thread.

The literal is only valid within the current solving step and solver thread.
All volatile literals and clauses involving a volatile literal are deleted
after the current search.

Returns
-------
int
    The added solver literal.
)"},
    {"add_watch", to_function<&PropagateControl::add_watch>(), METH_O, R"(add_watch(self, literal: int) -> None
Add a watch for the solver literal in the given phase.

Parameters
----------
literal : int
    The target solver literal.

Returns
-------
None

Notes
-----
Unlike `PropagateInit.add_watch` this does not add a watch to all solver
threads but just the current one.
)"},
    {"has_watch", to_function<&PropagateControl::has_watch>(), METH_O, R"(has_watch(self, literal: int) -> bool
Check whether a literal is watched in the current solver thread.

Parameters
----------
literal : int
    The target solver literal.

Returns
-------
bool
    Whether the literal is watched.
)"},
    {"remove_watch", to_function<&PropagateControl::remove_watch>(), METH_O, R"(remove_watch(self, literal: int) -> None
Removes the watch (if any) for the given solver literal.

Parameters
----------
literal : int
    The target solver literal.

Returns
-------
None
)"},
    {"add_clause", to_function<&PropagateControl::addClause>(), METH_KEYWORDS | METH_VARARGS, R"(add_clause(self, clause: List[int], tag: bool=False, lock: bool=False) -> bool

Add the given clause to the solver.

Parameters
----------
clause : List[int]
    List of solver literals forming the clause.
tag : bool=False
    If true, the clause applies only in the current solving step.
lock : bool=False
    If true, exclude clause from the solver's regular clause deletion policy.

Returns
-------
bool
    This method returns false if the current propagation must be stopped.
)"},
    {"add_nogood", to_function<&PropagateControl::addNogood>(), METH_KEYWORDS | METH_VARARGS, R"(add_nogood(self, clause: List[int], tag: bool=False, lock: bool=False) -> bool

Equivalent to `self.add_clause([-lit for lit in clause], tag, lock)`.
)"},
    {"propagate", to_function<&PropagateControl::propagate>(), METH_NOARGS, R"(propagate(self) -> bool

Propagate literals implied by added clauses.

Returns
-------
bool
    This method returns false if the current propagation must be stopped.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef PropagateControl::tp_getset[] = {
    {(char *)"thread_id", to_getter<&PropagateControl::id>(), nullptr, (char *)R"(thread_id: int

The numeric id of the current solver thread.
)", nullptr},
    {(char *)"assignment", to_getter<&PropagateControl::assignment>(), nullptr, (char *)R"(assignment: Assignment

`Assignment` object capturing the partial assignment of the current solver thread.
)", nullptr},
    {(char *)"_to_c", to_getter<&PropagateControl::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C
`clingo_propagate_control_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Propagator

static bool propagator_init(clingo_propagate_init_t *init, void *prop) {
    PyBlock block;
    try {
        Object i = PropagateInit::construct(init);
        Object n = PyString_FromString("init");
        Object ret = PyObject_CallMethodObjArgs(static_cast<PyObject*>(prop), n.toPy(), i.toPy(), nullptr);
        return true;
    }
    catch (...) {
        handle_cxx_error("Propagator::init", "error during initialization");
        return false;
    }
}

static bool propagator_propagate(clingo_propagate_control_t *control, clingo_literal_t const *changes, size_t size, void *prop) {
    PyBlock block;
    try {
        Object c = PropagateControl::construct(control);
        Object l = cppRngToPy(changes, changes + size);
        Object n = PyString_FromString("propagate");
        Object ret = PyObject_CallMethodObjArgs(static_cast<PyObject*>(prop), n.toPy(), c.toPy(), l.toPy(), nullptr);
        return true;
    }
    catch (...) {
        handle_cxx_error("Propagator::propagate", "error during propagation");
        return false;
    }
}

bool propagator_undo(clingo_propagate_control_t const *control, clingo_literal_t const *changes, size_t size, void *prop) {
    PyBlock block;
    try {
        Object i = cppToPy(clingo_propagate_control_thread_id(control));
        Object a = Assignment::construct(clingo_propagate_control_assignment(control));
        Object l = cppRngToPy(changes, changes + size);
        Object n = PyString_FromString("undo");
        Object ret = PyObject_CallMethodObjArgs(static_cast<PyObject*>(prop), n.toPy(), i.toPy(), a.toPy(), l.toPy(), nullptr);
        return true;
    }
    catch (...) {
        handle_cxx_error("Propagator::undo", "error during undo");
        return false;
    }
}

bool propagator_check(clingo_propagate_control_t *control, void *prop) {
    PyBlock block;
    try {
        Object c = PropagateControl::construct(control);
        Object n = PyString_FromString("check");
        Object ret = PyObject_CallMethodObjArgs(static_cast<PyObject*>(prop), n.toPy(), c.toPy(), nullptr);
        return true;
    }
    catch (...) {
        handle_cxx_error("Propagator::check", "error during check");
        return false;
    }
}

static bool propagator_decide(clingo_id_t solverId, clingo_assignment_t const *assign, clingo_literal_t vsids, void *heu, clingo_literal_t *decision) {
    PyBlock block;
    try {
        Object a = Assignment::construct(assign);
        Object s = PyLong_FromLong(solverId);
        Object l = PyLong_FromLong(vsids);
        Object n = PyString_FromString("decide");
        Object ret = PyObject_CallMethodObjArgs(static_cast<PyObject*>(heu), n.toPy(), s.toPy(), a.toPy(), l.toPy(), nullptr);
        *decision = pyToCpp<clingo_literal_t>(ret);
        return true;
    }
    catch (...) {
        handle_cxx_error("Propagator::decide", "error during decide");
        return false;
    }
}

// {{{1 wrap observer

struct TruthValue : EnumType<TruthValue> {
    using Type = clingo_external_type;
    static constexpr char const *tp_type = "TruthValue";
    static constexpr char const *tp_name = "clingo.TruthValue";
    static constexpr char const *tp_doc =
R"(Enumeration of the different truth values.

`TruthValue` objects have a readable string representation, implement Python's
rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
True_ : TruthValue
    Represents truth value true.
False_ : TruthValue
    Represents truth value true.
Free : TruthValue
    Represents absence of a truth value.
Release : TruthValue
    Indicates that an atom is to be released.
)";

    static constexpr Type const values[] = {
        clingo_external_type_true,
        clingo_external_type_false,
        clingo_external_type_free,
        clingo_external_type_release
    };
    static constexpr const char * const strings[] = {
        "True_",
        "False_",
        "Free",
        "Release",
    };
};

constexpr TruthValue::Type const TruthValue::values[];
constexpr const char * const TruthValue::strings[];

struct HeuristicType : EnumType<HeuristicType> {
    using Type = clingo_heuristic_type;
    static constexpr char const *tp_type = "HeuristicType";
    static constexpr char const *tp_name = "clingo.HeuristicType";
    static constexpr char const *tp_doc =
R"(Enumeration of the different heuristic types.

`HeuristicType` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes  are available:

Attributes
----------
Level : HeuristicType
    Heuristic modification to set the level of an atom.
Sign : HeuristicType
    Heuristic modification to set the sign of an atom.
Factor : HeuristicType
    Heuristic modification to set the decaying factor of an atom.
Init : HeuristicType
    Heuristic modification to set the inital score of an atom.
True_ : HeuristicType
    Heuristic modification to make an atom true.
False_ : HeuristicType
    Heuristic modification to make an atom false.
)";

    static constexpr Type const values[] =          {
        clingo_heuristic_type_level,
        clingo_heuristic_type_sign,
        clingo_heuristic_type_factor,
        clingo_heuristic_type_init,
        clingo_heuristic_type_true,
        clingo_heuristic_type_false,
    };
    static constexpr const char * const strings[] = {
        "Level",
        "Sign",
        "Factor",
        "Init",
        "True_",
        "False_",
    };
};

constexpr HeuristicType::Type const HeuristicType::values[];
constexpr const char * const HeuristicType::strings[];

template <class... T>
bool observer_call(char const *loc, char const *msg, void *data, char const *fun, T&&... args) {
    try {
        Reference obs{reinterpret_cast<PyObject*>(data)};
        if (obs.hasAttr(fun)) { obs.call(fun, std::forward<T>(args)...); }
        return true;
    }
    catch(...) {
        handle_cxx_error(loc, msg);
        return false;
    }
}

bool observer_init_program(bool incremental, void *data) {
    PyBlock b;
    return observer_call("Observer::init_program", "error in init_program", data, "init_program", cppToPy(incremental));
}
bool observer_begin_step(void *data) {
    PyBlock b;
    return observer_call("Observer::begin_step", "error in begin_step", data, "begin_step");
}
bool observer_end_step(void *data) {
    PyBlock b;
    return observer_call("Observer::end_step", "error in end_step", data, "end_step");
}
bool observer_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_literal_t const *body, size_t body_size, void *data) {
    PyBlock b;
    return observer_call("Observer::rule", "error in rule", data, "rule", cppToPy(choice), cppRngToPy(head, head + head_size), cppRngToPy(body, body + body_size));
}
bool observer_weight_rule(bool choice, clingo_atom_t const *head, size_t head_size, clingo_weight_t lower_bound, clingo_weighted_literal_t const *body, size_t body_size, void *data) {
    PyBlock b;
    return observer_call("Observer::weight_rule", "error in weight_rule", data, "weight_rule", cppToPy(choice), cppRngToPy(head, head + head_size), cppToPy(lower_bound), cppRngToPy(body, body + body_size));
}
bool observer_minimize(clingo_weight_t priority, clingo_weighted_literal_t const* literals, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::minimize", "error in minimize", data, "minimize", cppToPy(priority), cppRngToPy(literals, literals + size));
}
bool observer_project(clingo_atom_t const *atoms, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::project", "error in project", data, "project", cppRngToPy(atoms, atoms + size));
}
bool observer_output_atom(clingo_symbol_t symbol, clingo_atom_t atom, void *data) {
    PyBlock b;
    return observer_call("Observer::output_atom", "error in output_atom", data, "output_atom", cppToPy(symbol_wrapper{symbol}), cppToPy(atom));
}
bool observer_output_term(clingo_symbol_t symbol, clingo_literal_t const *condition, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::output_term", "error in output_term", data, "output_term", cppToPy(symbol_wrapper{symbol}), cppRngToPy(condition, condition + size));
}
bool observer_output_csp(clingo_symbol_t symbol, int value, clingo_literal_t const *condition, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::output_csp", "error in output_csp", data, "output_csp", cppToPy(symbol_wrapper{symbol}), cppToPy(value), cppRngToPy(condition, condition + size));
}
bool observer_external(clingo_atom_t atom, clingo_external_type_t type, void *data) {
    PyBlock b;
    return observer_call("Observer::external", "error in external", data, "external", cppToPy(atom), TruthValue::getAttr(type));
}
bool observer_assume(clingo_literal_t const *literals, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::assume", "error in assume", data, "assume", cppRngToPy(literals, literals + size));
}
bool observer_heuristic(clingo_atom_t atom, clingo_heuristic_type_t type, int bias, unsigned priority, clingo_literal_t const *condition, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::heuristic", "error in heuristic", data, "heuristic", cppToPy(atom), HeuristicType::getAttr(type), cppToPy(bias), cppToPy(priority), cppRngToPy(condition, condition + size));
}
bool observer_acyc_edge(int node_u, int node_v, clingo_literal_t const *condition, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::acyc_edge", "error in acyc_edge", data, "acyc_edge", cppToPy(node_u), cppToPy(node_v), cppRngToPy(condition, condition + size));
}
bool observer_theory_term_number(clingo_id_t term_id, int number, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_term_number", "error in theory_term_number", data, "theory_term_number", cppToPy(term_id), cppToPy(number));
}
bool observer_theory_term_string(clingo_id_t term_id, char const *name, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_term_string", "error in theory_term_string", data, "theory_term_string", cppToPy(term_id), cppToPy(name));
}
bool observer_theory_term_compound(clingo_id_t term_id, int name_id_or_type, clingo_id_t const *arguments, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_term_compound", "error in theory_term_compound", data, "theory_term_compound", cppToPy(term_id), cppToPy(name_id_or_type), cppRngToPy(arguments, arguments + size));
}
bool observer_theory_element(clingo_id_t element_id, clingo_id_t const *terms, size_t terms_size, clingo_literal_t const *condition, size_t condition_size, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_element", "error in theory_element", data, "theory_element", cppToPy(element_id), cppRngToPy(terms, terms + terms_size), cppRngToPy(condition, condition + condition_size));
}
bool observer_theory_atom(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_atom", "error in theory_atom", data, "theory_atom", cppToPy(atom_id_or_zero), cppToPy(term_id), cppRngToPy(elements, elements + size));
}
bool observer_theory_atom_with_guard(clingo_id_t atom_id_or_zero, clingo_id_t term_id, clingo_id_t const *elements, size_t size, clingo_id_t operator_id, clingo_id_t right_hand_side_id, void *data) {
    PyBlock b;
    return observer_call("Observer::theory_atom_with_guard", "error in theory_atom_with_guard", data, "theory_atom_with_guard", cppToPy(atom_id_or_zero), cppToPy(term_id), cppRngToPy(elements, elements + size), cppToPy(operator_id), cppToPy(right_hand_side_id));
}

// {{{1 wrap wrap Backend

struct Backend : ObjectBase<Backend> {
    clingo_backend_t *backend;

    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "Backend";
    static constexpr char const *tp_name = "clingo.Backend";
    static constexpr char const *tp_doc =
    R"(Backend object providing a low level interface to extend a logic program.

This class allows for adding statements in ASPIF format.

See Also
--------
Control.backend

Notes
-----
The `Backend` is a context manager and must be used with Python's `with`
statement.

Statements added with the backend are added directly to the solver. For
example, the grounding component will not be aware if facts were added to a
program via the backend. The only exception are atoms added with
`Backend.add_atom`, which will subsequently be used to instantiate rules.
Furthermore, the `Control.cleanup` method can be used to transfer information
about facts back to the grounder.

Examples
--------
The following example shows how to add a fact to a program and the effect of
the `Control.cleanup` function:

    >>> import clingo
    >>> ctl = clingo.Control()
    >>> sym_a = clingo.Function("a")
    >>> with ctl.backend() as backend:
    ...     atm_a = backend.add_atom(sym_a)
    ...     backend.add_rule([atm_a])
    ...
    >>> ctl.symbolic_atoms[sym_a].is_fact
    False
    >>> ctl.cleanup()
    >>> ctl.symbolic_atoms[sym_a].is_fact
    True
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: a
    SAT
)";

    static Object construct(clingo_backend_t *backend) {
        if (!backend) {
            PyErr_Format(PyExc_RuntimeError, "backend not available");
            throw PyException();
        }
        auto self = new_();
        self->backend = backend;
        return self;
    }

    Object enter() {
        handle_c_error(clingo_backend_begin(backend));
        return Reference{*this};
    }

    Object exit() {
        handle_c_error(clingo_backend_end(backend));
        Py_RETURN_FALSE;
    }

    Object addAtom(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"symbol", nullptr};
        Reference symbol;
        ParseTupleAndKeywords(pyargs, pykwds, "|O", kwlist, symbol);
        clingo_symbol_t *symp;
        clingo_symbol_t sym;
        if (!symbol.valid()) { symp = nullptr; }
        else {
            sym = pyToCpp<symbol_wrapper>(symbol).symbol;
            symp = &sym;
        }
        clingo_atom_t atom;
        handle_c_error(clingo_backend_add_atom(backend, symp, &atom));
        return cppToPy(atom);
    }

    Object addRule(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"head", "body", "choice", nullptr};
        Reference pyHead;
        Reference pyBody = Py_None;
        Reference pyChoice = Py_False;
        ParseTupleAndKeywords(pyargs, pykwds, "O|OO", kwlist, pyHead, pyBody, pyChoice);
        std::vector<clingo_atom_t> head;
        pyToCpp(pyHead, head);
        std::vector<clingo_literal_t> body;
        if (!pyBody.is_none()) { pyToCpp(pyBody, body); }
        bool choice = pyChoice.isTrue();
        handle_c_error(clingo_backend_rule(backend, choice, head.data(), head.size(), body.data(), body.size()));
        Py_RETURN_NONE;
    }

    Object addExternal(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"head", "value", nullptr};
        Reference pyAtom;
        Reference pyValue = nullptr;
        ParseTupleAndKeywords(pyargs, pykwds, "O|O", kwlist, pyAtom, pyValue);
        clingo_atom_t atom;
        pyToCpp(pyAtom, atom);
        clingo_external_type_t value = pyValue.valid() ? enumValue<TruthValue>(pyValue) : clingo_external_type_false;
        handle_c_error(clingo_backend_external(backend, atom, value));
        Py_RETURN_NONE;
    }

    Object addWeightRule(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"head", "lower", "body", "choice", nullptr};
        Reference pyHead;
        Reference pyLower;
        Reference pyBody;
        Reference pyChoice = Py_False;
        ParseTupleAndKeywords(pyargs, pykwds, "OOO|O", kwlist, pyHead, pyLower, pyBody, pyChoice);
        auto head = pyToCpp<std::vector<clingo_atom_t>>(pyHead);
        auto lower = pyToCpp<clingo_weight_t>(pyLower);
        auto body = pyToCpp<std::vector<clingo_weighted_literal_t>>(pyBody);
        auto choice = pyToCpp<bool>(pyChoice);
        handle_c_error(clingo_backend_weight_rule(backend, choice, head.data(), head.size(), lower, body.data(), body.size()));
        Py_RETURN_NONE;
    }

    Object addMinimize(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"priority", "literals", nullptr};
        Reference pyPriority;
        Reference pyBody;
        ParseTupleAndKeywords(pyargs, pykwds, "OO", kwlist, pyPriority, pyBody);
        auto priority = pyToCpp<clingo_weight_t>(pyPriority);
        auto body = pyToCpp<std::vector<clingo_weighted_literal_t>>(pyBody);
        handle_c_error(clingo_backend_minimize(backend, priority, body.data(), body.size()));
        Py_RETURN_NONE;
    }

    Object addProject(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"atoms", nullptr};
        Reference pyAtoms;
        ParseTupleAndKeywords(pyargs, pykwds, "O", kwlist, pyAtoms);
        auto atoms = pyToCpp<std::vector<clingo_atom_t>>(pyAtoms);
        handle_c_error(clingo_backend_project(backend, atoms.data(), atoms.size()));
        Py_RETURN_NONE;
    }

    Object addAssume(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"literals", nullptr};
        Reference pyLiterals = nullptr;
        ParseTupleAndKeywords(pyargs, pykwds, "O", kwlist, pyLiterals);
        auto literals = pyToCpp<std::vector<clingo_literal_t>>(pyLiterals);
        handle_c_error(clingo_backend_assume(backend, literals.data(), literals.size()));
        Py_RETURN_NONE;
    }

    Object addHeuristic(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"atom", "type", "bias", "priority", "condition", nullptr};
        Reference pyAtom, pyType, pyBias, pyPriority, pyCondition;
        ParseTupleAndKeywords(pyargs, pykwds, "OOOOO", kwlist, pyAtom, pyType, pyBias, pyPriority, pyCondition);
        auto atom = pyToCpp<clingo_atom_t>(pyAtom);
        auto type = enumValue<HeuristicType>(pyType);
        auto bias = pyToCpp<int>(pyBias);
        auto priority = pyToCpp<unsigned>(pyPriority);
        auto condition = pyToCpp<std::vector<clingo_literal_t>>(pyCondition);
        handle_c_error(clingo_backend_heuristic(backend, atom, type, bias, priority, condition.data(), condition.size()));
        Py_RETURN_NONE;
    }

    Object addAcycEdge(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"node_u", "node_v", "condition", nullptr};
        Reference pyU, pyV, pyCondition;
        ParseTupleAndKeywords(pyargs, pykwds, "OOO", kwlist, pyU, pyV, pyCondition);
        auto u = pyToCpp<int>(pyU);
        auto v = pyToCpp<int>(pyV);
        auto condition = pyToCpp<std::vector<clingo_literal_t>>(pyCondition);
        handle_c_error(clingo_backend_acyc_edge(backend, u, v, condition.data(), condition.size()));
        Py_RETURN_NONE;
    }

    Object to_c() {
        return PyLong_FromVoidPtr(backend);
    }
};

PyMethodDef Backend::tp_methods[] = {
    {"__enter__", to_function<&Backend::enter>(), METH_NOARGS,
R"(__enter__(self) -> Backend

Initialize the backend.

Returns
-------
Backend
    Returns the backend itself.

Notes
-----
Must be called before using the backend.
)"},
    {"__exit__", to_function<&Backend::exit>(), METH_VARARGS,
R"(__exit__(self, type : Optional[Type[BaseException]], value : Optional[BaseException], traceback : Optional[TracebackType]) -> bool

Finalize the backend.

Notes
-----
Follows Python's __exit__ conventions. Does not suppress exceptions.
)"},
    // add_atom
    {"add_atom", to_function<&Backend::addAtom>(), METH_VARARGS | METH_KEYWORDS,
R"(add_atom(self, symbol : Optional[Symbol]=None) -> int

Return a fresh program atom or the atom associated with the given symbol.

If the given symbol does not exist in the atom base, it is added first. Such
atoms will be used in subequents calls to ground for instantiation.

Parameters
----------
symbol : Optional[Symbol]=None
    The symbol associated with the atom.

Returns
-------
int
    The program atom representing the atom.
)"},
    // add_external
    {"add_external", to_function<&Backend::addExternal>(), METH_VARARGS | METH_KEYWORDS,
R"(add_external(self, atom : int, value : TruthValue=TruthValue.False_) -> None

Mark a program atom as external optionally fixing its truth value.

Parameters
----------
atom : int
    The program atom to mark as external.
value : TruthValue=TruthValue.False_
    Optional truth value.

Returns
-------
None

Notes
-----
Can also be used to release an external atom using `TruthValue.Release`.
)"},
    // add_rule
    {"add_rule", to_function<&Backend::addRule>(), METH_VARARGS | METH_KEYWORDS,
R"(add_rule(self, head: List[int], body: List[int]=[], choice: bool=False) -> None

Add a disjuntive or choice rule to the program.

Parameters
----------
head : List[int]
    The program atoms forming the rule head.
body : List[int]=[]
    The program literals forming the rule body.
choice : bool=False
    Whether to add a disjunctive or choice rule.

Returns
-------
None

Notes
-----
Integrity constraints and normal rules can be added by using an empty or
singleton head list, respectively.
)"},
    // add_weight_rule
    {"add_weight_rule", to_function<&Backend::addWeightRule>(), METH_VARARGS | METH_KEYWORDS,
R"(add_weight_rule(self, head: List[int], lower: int, body: List[Tuple[int,int]], choice: bool=False) -> None

Add a disjuntive or choice rule with one weight constraint with a lower bound
in the body to the program.

Parameters
----------
head : List[int]
    The program atoms forming the rule head.
lower : int
    The lower bound.
body : List[Tuple[int,int]]
    The pairs of program literals and weights forming the elements of the
    weight constraint.
choice : bool=False
    Whether to add a disjunctive or choice rule.

Returns
-------
None
)"},
    // add_minimize
    {"add_minimize", to_function<&Backend::addMinimize>(), METH_VARARGS | METH_KEYWORDS,
R"(add_minimize(self, priority: int, literals: List[Tuple[int,int]]) -> None

Add a minimize constraint to the program.

Parameters
----------
priority : int
    Integer for the priority.
literals : List[Tuple[int,int]]
    List of pairs of program literals and weights.

Returns
-------
None
)"},
    // add_project
    {"add_project", to_function<&Backend::addProject>(), METH_VARARGS | METH_KEYWORDS,
R"(add_project(self, atoms: List[int]) -> None

Add a project statement to the program.

Parameters
----------
atoms : List[int]
    List of program atoms to project on.

Returns
-------
None
)"},
    // add_assume
    {"add_assume", to_function<&Backend::addAssume>(), METH_VARARGS | METH_KEYWORDS,
R"(add_assume(self, literals: List[int]) -> None

Add assumptions to the program.

Parameters
----------
literals : List[int]
    The list of literals to assume true.

Returns
-------
None
)"},
    // add_heuristic
    {"add_heuristic", to_function<&Backend::addHeuristic>(), METH_VARARGS | METH_KEYWORDS,
R"(add_heuristic(self, atom: int, type: HeuristicType, bias: int, priority: int, condition: List[int]) -> None

Add a heuristic directive to the program.

Parameters
----------
atom : int
    Program atom to heuristically modify.
type : HeuristicType
    The type of modification.
bias : int
    A signed integer.
priority : int
    An unsigned integer.
condition : List[int]
    List of program literals.

Returns
-------
None
)"},
    // add_acyc_edge
    {"add_acyc_edge", to_function<&Backend::addAcycEdge>(), METH_VARARGS | METH_KEYWORDS,
R"(add_acyc_edge(self, node_u: int, node_v: int, condition: List[int]) -> None

Add an edge directive to the program.

Parameters
----------
node_u : int
    The start node represented as an unsigned integer.
node_v : int
    The end node represented as an unsigned integer.
condition : List[int]
    List of program literals.

Returns
-------
None
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef Backend::tp_getset[] = {
    {(char *)"_to_c", to_getter<&Backend::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_backend_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap AST

// {{{2 Py

// {{{3 macros

#define CREATE1(N,a1) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1, nullptr}; \
    PyObject* vals[] = { nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "O", const_cast<char**>(kwlist), &vals[0])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}
#define CREATE2(N,a1,a2) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1,#a2, nullptr}; \
    PyObject* vals[] = { nullptr, nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "OO", const_cast<char**>(kwlist), &vals[0], &vals[1])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}
#define CREATE3(N,a1,a2,a3) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1, #a2, #a3, nullptr}; \
    PyObject* vals[] = { nullptr, nullptr, nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "OOO", const_cast<char**>(kwlist), &vals[0], &vals[1], &vals[2])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}
#define CREATE4(N,a1,a2,a3,a4) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1, #a2, #a3, #a4, nullptr}; \
    PyObject* vals[] = { nullptr, nullptr, nullptr, nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "OOOO", const_cast<char**>(kwlist), &vals[0], &vals[1], &vals[2], &vals[3])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}
#define CREATE5(N,a1,a2,a3,a4,a5) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1, #a2, #a3, #a4, #a5, nullptr}; \
    PyObject* vals[] = { nullptr, nullptr, nullptr, nullptr, nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "OOOOO", const_cast<char**>(kwlist), &vals[0], &vals[1], &vals[2], &vals[3], &vals[4])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}
#define CREATE6(N,a1,a2,a3,a4,a5,a6) \
Object create ## N(Reference pyargs, Reference pykwds) { \
    static char const *kwlist[] = {#a1, #a2, #a3, #a4, #a5, #a6, nullptr}; \
    PyObject* vals[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }; \
    if (!PyArg_ParseTupleAndKeywords(pyargs.toPy(), pykwds.toPy(), "OOOOOO", const_cast<char**>(kwlist), &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5])) { return nullptr; } \
    return AST::construct(ASTType::N, kwlist, vals); \
}

// {{{3 enums

struct AggregateFunction : EnumType<AggregateFunction> {
    static constexpr char const *tp_type = "AggregateFunction";
    static constexpr char const *tp_name = "clingo.ast.AggregateFunction";
    static constexpr char const *tp_doc =
R"(Enumeration of aggegate functions.

`AggregateFunction` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Count : AggregateFunction
    The `#count` function.
Sum : AggregateFunction
    The `#sum` function.
SumPlus : AggregateFunction
    The `#sum+` function.
Min : AggregateFunction
    The `#min` function.
Max : AggregateFunction
    The `#max` function.
)";

    static constexpr clingo_ast_aggregate_function_t const values[] = {
        clingo_ast_aggregate_function_count,
        clingo_ast_aggregate_function_sum,
        clingo_ast_aggregate_function_sump,
        clingo_ast_aggregate_function_min,
        clingo_ast_aggregate_function_max
    };
    static constexpr const char * const strings[] = {
        "Count",
        "Sum",
        "SumPlus",
        "Min",
        "Max",
    };
    Object tp_repr() {
        switch (static_cast<enum clingo_ast_aggregate_function>(values[offset])) {
            case clingo_ast_aggregate_function_count: { return PyString_FromString("#count"); }
            case clingo_ast_aggregate_function_sum:   { return PyString_FromString("#sum"); }
            case clingo_ast_aggregate_function_sump:  { return PyString_FromString("#sum+"); }
            case clingo_ast_aggregate_function_min:   { return PyString_FromString("#min"); }
            case clingo_ast_aggregate_function_max:   { return PyString_FromString("#max"); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_aggregate_function_t const AggregateFunction::values[];
constexpr const char * const AggregateFunction::strings[];

struct ComparisonOperator : EnumType<ComparisonOperator> {
    static constexpr char const *tp_type = "ComparisonOperator";
    static constexpr char const *tp_name = "clingo.ast.ComparisonOperator";
    static constexpr char const *tp_doc =
R"(Enumeration of comparison operators.

`ComparisonOperator` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
GreaterThan : ComparisonOperator
    The `>` operator.
LessThan : ComparisonOperator
    The `<` operator.
LessEqual : ComparisonOperator
    The `<=` operator.
GreaterEqual : ComparisonOperator
    The `>=` operator.
NotEqual : ComparisonOperator
    The `!=` operator.
Equal : ComparisonOperator
    The `=` operator
)";

    static constexpr clingo_ast_comparison_operator_t const values[] = {
        clingo_ast_comparison_operator_greater_than,
        clingo_ast_comparison_operator_less_than,
        clingo_ast_comparison_operator_less_equal,
        clingo_ast_comparison_operator_greater_equal,
        clingo_ast_comparison_operator_not_equal,
        clingo_ast_comparison_operator_equal
    };
    static constexpr const char * const strings[] = {
        "GreaterThan",
        "LessThan",
        "LessEqual",
        "GreaterEqual",
        "NotEqual",
        "Equal"
    };
    Object tp_repr() {
        switch (offset) {
            case 0: { return PyString_FromString(">"); }
            case 1: { return PyString_FromString("<"); }
            case 2: { return PyString_FromString("<="); }
            case 3: { return PyString_FromString(">="); }
            case 4: { return PyString_FromString("!="); }
            case 5: { return PyString_FromString("="); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_comparison_operator_t const ComparisonOperator::values[];
constexpr const char * const ComparisonOperator::strings[];

struct ASTType : EnumType<ASTType> {
    enum T {
        Id,
        Variable, Symbol, UnaryOperation, BinaryOperation, Interval, Function, Pool,
        CSPProduct, CSPSum, CSPGuard,
        BooleanConstant, SymbolicAtom, Comparison, CSPLiteral,
        AggregateGuard, ConditionalLiteral, Aggregate, BodyAggregateElement, BodyAggregate, HeadAggregateElement, HeadAggregate, Disjunction, DisjointElement, Disjoint,
        TheorySequence, TheoryFunction, TheoryUnparsedTermElement, TheoryUnparsedTerm, TheoryGuard, TheoryAtomElement, TheoryAtom,
        Literal,
        TheoryOperatorDefinition, TheoryTermDefinition, TheoryGuardDefinition, TheoryAtomDefinition, TheoryDefinition,
        Rule, Definition, ShowSignature, ShowTerm, Minimize, Script, Program, External, Edge, Heuristic, ProjectAtom, ProjectSignature, Defined
    };
    static constexpr char const *tp_type = "ASTType";
    static constexpr char const *tp_name = "clingo.ast.ASTType";
    static constexpr char const *tp_doc =
R"(Enumeration of ast node types.)";

    static constexpr T const values[] = {
        Id,
        Variable, Symbol, UnaryOperation, BinaryOperation, Interval, Function, Pool,
        CSPProduct, CSPSum, CSPGuard,
        BooleanConstant, SymbolicAtom, Comparison, CSPLiteral,
        AggregateGuard, ConditionalLiteral, Aggregate, BodyAggregateElement, BodyAggregate, HeadAggregateElement, HeadAggregate, Disjunction, DisjointElement, Disjoint,
        TheorySequence, TheoryFunction, TheoryUnparsedTermElement, TheoryUnparsedTerm, TheoryGuard, TheoryAtomElement, TheoryAtom,
        Literal,
        TheoryOperatorDefinition, TheoryTermDefinition, TheoryGuardDefinition, TheoryAtomDefinition, TheoryDefinition,
        Rule, Definition, ShowSignature, ShowTerm, Minimize, Script, Program, External, Edge, Heuristic, ProjectAtom, ProjectSignature, Defined
    };
    static constexpr const char * const strings[] = {
        "Id",
        "Variable", "Symbol", "UnaryOperation", "BinaryOperation", "Interval", "Function", "Pool",
        "CSPProduct", "CSPSum", "CSPGuard",
        "BooleanConstant", "SymbolicAtom", "Comparison", "CSPLiteral",
        "AggregateGuard", "ConditionalLiteral", "Aggregate", "BodyAggregateElement", "BodyAggregate", "HeadAggregateElement", "HeadAggregate", "Disjunction", "DisjointElement", "Disjoint",
        "TheorySequence", "TheoryFunction", "TheoryUnparsedTermElement", "TheoryUnparsedTerm", "TheoryGuard", "TheoryAtomElement", "TheoryAtom",
        "Literal",
        "TheoryOperatorDefinition", "TheoryTermDefinition", "TheoryGuardDefinition", "TheoryAtomDefinition", "TheoryDefinition",
        "Rule", "Definition", "ShowSignature", "ShowTerm", "Minimize", "Script", "Program", "External", "Edge", "Heuristic", "ProjectAtom", "ProjectSignature", "Defined"
    };
};

constexpr ASTType::T const ASTType::values[];
constexpr const char * const ASTType::strings[];

struct Sign : EnumType<Sign> {
    static constexpr char const *tp_type = "Sign";
    static constexpr char const *tp_name = "clingo.ast.Sign";
    static constexpr char const *tp_doc =
R"(Enumeration of signs for literals.

`Sign` objects have a readable string representation, implement Python's rich
comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
NoSign : Sign
    For positive literals.
Negation : Sign
    For negative literals (with prefix `not`).
DoubleNegation : Sign
    For double negated literals (with prefix `not not`)
)";

    static constexpr clingo_ast_sign_t const values[] = {
        clingo_ast_sign_none,
        clingo_ast_sign_negation,
        clingo_ast_sign_double_negation
    };
    static constexpr const char * const strings[] = {
        "NoSign",
        "Negation",
        "DoubleNegation"
    };
    Object tp_repr() {
        switch (offset) {
            case 0: { return PyString_FromString(""); }
            case 1: { return PyString_FromString("not "); }
            case 2: { return PyString_FromString("not not "); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_sign_t const Sign::values[];
constexpr const char * const Sign::strings[];

struct UnaryOperator : EnumType<UnaryOperator> {
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "UnaryOperator";
    static constexpr char const *tp_name = "clingo.ast.UnaryOperator";
    static constexpr char const *tp_doc =
R"(Enumeration of signs for literals.

`UnaryOperator` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Negation : UnaryOperator
    For bitwise negation.
Minus : UnaryOperator
    For unary minus and classical negation.
Absolute : UnaryOperator
    For taking the absolute value.
)";

    static constexpr clingo_ast_unary_operator_t const values[] = {
        clingo_ast_unary_operator_absolute,
        clingo_ast_unary_operator_minus,
        clingo_ast_unary_operator_negation,
    };
    static constexpr const char * const strings[] = {
        "Absolute",
        "Minus",
        "Negation",
    };
    Object leftHandSide() {
        switch (static_cast<enum clingo_ast_unary_operator>(values[offset])) {
            case clingo_ast_unary_operator_absolute: { return PyString_FromString("|"); }
            case clingo_ast_unary_operator_minus:    { return PyString_FromString("-"); }
            case clingo_ast_unary_operator_negation: { return PyString_FromString("~"); }
        }
        return PyString_FromString("");
    }
    Object rightHandSide() {
        return values[offset] == clingo_ast_unary_operator_absolute
            ? PyString_FromString("|")
            : PyString_FromString("");
    }
};

PyGetSetDef UnaryOperator::tp_getset[] = {
    {(char*)"left_hand_side", to_getter<&UnaryOperator::leftHandSide>(), nullptr, (char*)R"(left_hand_side: str

Left-hand side representation of the operator.
)", nullptr},
    {(char*)"right_hand_side", to_getter<&UnaryOperator::rightHandSide>(), nullptr, (char*)R"(right_hand_side: str

Right-hand side representation of the operator.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

constexpr clingo_ast_unary_operator_t const UnaryOperator::values[];
constexpr const char * const UnaryOperator::strings[];

struct BinaryOperator : EnumType<BinaryOperator> {
    static constexpr char const *tp_type = "BinaryOperator";
    static constexpr char const *tp_name = "clingo.ast.BinaryOperator";
    static constexpr char const *tp_doc =
R"(Enumeration of binary operators.

`BinaryOperator` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the
following preconstructed class attributes are available:

Attributes
----------
XOr : BinaryOperator
    For bitwise exclusive or.
Or : BinaryOperator
    For bitwise or.
And : BinaryOperator
    For bitwise and.
Plus : BinaryOperator
    For arithmetic addition.
Minus : BinaryOperator
    For arithmetic subtraction.
Multiplication : BinaryOperator
    For arithmetic multipilcation.
Division : BinaryOperator
    For arithmetic division.
Modulo : BinaryOperator
    For arithmetic modulo.
Power : BinaryOperator
    For arithmetic exponentiation.
)";
    static constexpr clingo_ast_binary_operator_t const values[] = {
        clingo_ast_binary_operator_xor,
        clingo_ast_binary_operator_or,
        clingo_ast_binary_operator_and,
        clingo_ast_binary_operator_plus,
        clingo_ast_binary_operator_minus,
        clingo_ast_binary_operator_multiplication,
        clingo_ast_binary_operator_division,
        clingo_ast_binary_operator_modulo,
        clingo_ast_binary_operator_power,
    };
    static constexpr const char * const strings[] = {
        "XOr",
        "Or",
        "And",
        "Plus",
        "Minus",
        "Multiplication",
        "Division",
        "Modulo",
        "Power",
    };
    Object tp_repr() {
        switch (offset) {
            case 0: { return PyString_FromString("^"); }
            case 1: { return PyString_FromString("?"); }
            case 2: { return PyString_FromString("&"); }
            case 3: { return PyString_FromString("+"); }
            case 4: { return PyString_FromString("-"); }
            case 5: { return PyString_FromString("*"); }
            case 6: { return PyString_FromString("/"); }
            case 7: { return PyString_FromString("\\"); }
            case 8: { return PyString_FromString("**"); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_binary_operator_t const BinaryOperator::values[];
constexpr const char * const BinaryOperator::strings[];

struct TheorySequenceType : EnumType<TheorySequenceType> {
    enum T { Set, Tuple, List };
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "TheorySequenceType";
    static constexpr char const *tp_name = "clingo.ast.TheorySequenceType";
    static constexpr char const *tp_doc =
R"(Enumeration of theory term sequence types.

`TheorySequenceType` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Tuple : TheorySequenceType
    For sequences enclosed in parenthesis.
List : TheorySequenceType
    For sequences enclosed in brackets.
Set : TheorySequenceType
    For sequences enclosed in braces.
)";

    static constexpr T const values[] = {
        Set,
        Tuple,
        List,
    };
    static constexpr const char * const strings[] = {
        "Set",
        "Tuple",
        "List",
    };
    Object leftHandSide() {
        switch (values[offset]) {
            case Set:   { return PyString_FromString("{"); }
            case Tuple: { return PyString_FromString("("); }
            case List:  { return PyString_FromString("["); }
        }
        return PyString_FromString("");
    }
    Object rightHandSide() {
        switch (values[offset]) {
            case Set:   { return PyString_FromString("}"); }
            case Tuple: { return PyString_FromString(")"); }
            case List:  { return PyString_FromString("]"); }
        }
        return PyString_FromString("");
    }
};

PyGetSetDef TheorySequenceType::tp_getset[] = {
    {(char*)"left_hand_side", to_getter<&TheorySequenceType::leftHandSide>(), nullptr, (char*)R"(left_hand_side: str

Left-hand side representation of the sequence.
)", nullptr},
    {(char*)"right_hand_side", to_getter<&TheorySequenceType::rightHandSide>(), nullptr, (char*)R"(right_hand_side: str

Right-hand side representation of the sequence.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

constexpr TheorySequenceType::T const TheorySequenceType::values[];
constexpr const char * const TheorySequenceType::strings[];

struct TheoryOperatorType : EnumType<TheoryOperatorType> {
    static constexpr char const *tp_type = "TheoryOperatorType";
    static constexpr char const *tp_name = "clingo.ast.TheoryOperatorType";
    static constexpr char const *tp_doc =
R"(Enumeration of operator types.

`TheoryOperatorType` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Unary : TheoryOperatorType
    For unary operators.
BinaryLeft : TheoryOperatorType
    For binary left associative operators.
BinaryRight : TheoryOperatorType
    For binary right associative operator.
)";

    static constexpr clingo_ast_theory_operator_type_t const values[] = {
        clingo_ast_theory_operator_type_unary,
        clingo_ast_theory_operator_type_binary_left,
        clingo_ast_theory_operator_type_binary_right
    };
    static constexpr const char * const strings[] = {
        "Unary",
        "BinaryLeft",
        "BinaryRight"
    };
    Object tp_repr() {
        switch (offset) {
            case 0: { return PyString_FromString("unary"); }
            case 1: { return PyString_FromString("binary, left"); }
            case 2: { return PyString_FromString("binary, right"); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_theory_operator_type_t const TheoryOperatorType::values[];
constexpr const char * const TheoryOperatorType::strings[];

struct TheoryAtomType : EnumType<TheoryAtomType> {
    static constexpr char const *tp_type = "TheoryAtomType";
    static constexpr char const *tp_name = "clingo.ast.TheoryAtomType";
    static constexpr char const *tp_doc =
R"(Enumeration of theory atom types.

`TheoryAtomType` objects have a readable string representation, implement
Python's rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Any : TheoryAtomType
    For atoms that can occur anywhere.
Body : TheoryAtomType
    For atoms that can only occur in rule bodies.
Head : TheoryAtomType
    For atoms that can only occur in rule heads.
Directive : TheoryAtomType
    For atoms that can only occur in facts.
)";

    static constexpr clingo_ast_theory_atom_definition_type_t const values[] = {
        clingo_ast_theory_atom_definition_type_any,
        clingo_ast_theory_atom_definition_type_body,
        clingo_ast_theory_atom_definition_type_head,
        clingo_ast_theory_atom_definition_type_directive
    };
    static constexpr const char * const strings[] = {
        "Any",
        "Body",
        "Head",
        "Directive"
    };
    Object tp_repr() {
        switch (static_cast<enum clingo_ast_theory_atom_definition_type>(values[offset])) {
            case clingo_ast_theory_atom_definition_type_any:       { return PyString_FromString("any"); }
            case clingo_ast_theory_atom_definition_type_body:      { return PyString_FromString("body"); }
            case clingo_ast_theory_atom_definition_type_head:      { return PyString_FromString("head"); }
            case clingo_ast_theory_atom_definition_type_directive: { return PyString_FromString("directive"); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr clingo_ast_theory_atom_definition_type_t const TheoryAtomType::values[];
constexpr const char * const TheoryAtomType::strings[];

struct ScriptType : EnumType<ScriptType> {
    enum T { Python, Lua };
    static constexpr char const *tp_type = "ScriptType";
    static constexpr char const *tp_name = "clingo.ast.ScriptType";
    static constexpr char const *tp_doc =
R"(Enumeration of theory atom types.

`ScriptType` objects have a readable string representation, implement Python's
rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
Python : ScriptType
    For Python code.
Lua : ScriptType
    For Lua code.
)";

    static constexpr T const values[] = {
        Python,
        Lua
    };
    static constexpr const char * const strings[] = {
        "Python",
        "Lua",
    };
    Object tp_repr() {
        switch (values[offset]) {
            case Python: { return PyString_FromString("python"); }
            case Lua:    { return PyString_FromString("lua"); }
        }
        throw std::logic_error("cannot happen");
    }
};

constexpr ScriptType::T const ScriptType::values[];
constexpr const char * const ScriptType::strings[];

// }}}3

struct AST : ObjectBase<AST> {
    ASTType::T type_;
    Dict fields_;
    List children;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];
    static constexpr char const *tp_type = "AST";
    static constexpr char const *tp_name = "clingo.ast.AST";
    static constexpr char const *tp_doc = R"(AST(type: ASTType, **arguments: Mapping[str,Any]) -> AST

Represents a node in the abstract syntax tree.

AST nodes implement Python's rich comparison operators and are ordered
structurally ignoring the location. They can also be used as dictionary keys.
Their string representation corresponds to their gringo representation.

Parameters
----------
type : ASTType
    The type of the onde.
arguments : Mapping[str,Any]
    Additionally, the functions takes an arbitrary number of keyword arguments.
    These should contain the required fields of the node but can also be set
    later.

Notes
-----
It is also possible to create AST nodes using one of the functions provided in
this module. The parameters of the functions correspond to the nonterminals as
given in the [grammar](.) above.
)";
    static Object tp_new(PyTypeObject *type) {
        auto self = new_(type);
        new (&self->fields_) Dict();
        new (&self->children) List(nullptr);
        return self;
    }
    void tp_init(Reference args, Reference kwargs) {
        Reference pyType;
        ParseTuple(args, "O", pyType);
        type_ = enumValue<ASTType>(pyType);
        if (kwargs.valid()) {
            for (auto item : Dict{Reference{kwargs}}.items().iter()) {
                fields_.setItem(item.getItem(0), item.getItem(1));
            }
        }
    }
    static Object construct(ASTType::T t) {
        auto self = new_();
        new (&self->fields_) Dict();
        new (&self->children) List(nullptr);
        self->type_ = t;
        return self;
    }
    static Object construct(ASTType::T type, char const **kwlist, PyObject **vals) {
        Object ret = construct(type);
        auto jt = vals;
        for (auto it = kwlist; *it; ++it) {
            ret.setAttr(*it, *jt++);
        }
        return ret;
    }
    Object childKeys_() {
        auto ret = [](std::initializer_list<char const *> l) { return cppToPy(l); };
        switch (type_) {
            case ASTType::Id:                        { return ret({ }); }
            case ASTType::Variable:                  { return ret({ }); }
            case ASTType::Symbol:                    { return ret({ }); }
            case ASTType::UnaryOperation:            { return ret({ "argument" }); }
            case ASTType::BinaryOperation:           { return ret({ "left", "right" }); }
            case ASTType::Interval:                  { return ret({ "left", "right" }); }
            case ASTType::Function:                  { return ret({ "arguments" }); }
            case ASTType::Pool:                      { return ret({ "arguments" }); }
            case ASTType::CSPProduct:                { return ret({ "coefficient", "variable" }); }
            case ASTType::CSPSum:                    { return ret({ "terms" }); }
            case ASTType::CSPGuard:                  { return ret({ "term" }); }
            case ASTType::BooleanConstant:           { return ret({ }); }
            case ASTType::SymbolicAtom:              { return ret({ "term" }); }
            case ASTType::Comparison:                { return ret({ "left", "right" }); }
            case ASTType::CSPLiteral:                { return ret({ "term", "guards" }); }
            case ASTType::AggregateGuard:            { return ret({ "term" }); }
            case ASTType::ConditionalLiteral:        { return ret({ "literal", "condition" }); }
            case ASTType::Aggregate:                 { return ret({ "left_guard", "elements", "right_guard" }); }
            case ASTType::BodyAggregateElement:      { return ret({ "tuple", "condition" }); }
            case ASTType::BodyAggregate:             { return ret({ "left_guard", "elements", "right_guard" }); }
            case ASTType::HeadAggregateElement:      { return ret({ "tuple", "condition" }); }
            case ASTType::HeadAggregate:             { return ret({ "left_guard", "elements", "right_guard" }); }
            case ASTType::Disjunction:               { return ret({ "elements" }); }
            case ASTType::DisjointElement:           { return ret({ "tuple", "term", "condition" }); }
            case ASTType::Disjoint:                  { return ret({ "elements" }); }
            case ASTType::TheorySequence:            { return ret({ "terms" }); }
            case ASTType::TheoryFunction:            { return ret({ "arguments" }); }
            case ASTType::TheoryUnparsedTermElement: { return ret({ "term" }); }
            case ASTType::TheoryUnparsedTerm:        { return ret({ "elements" }); }
            case ASTType::TheoryGuard:               { return ret({ "term" }); }
            case ASTType::TheoryAtomElement:         { return ret({ "tuple", "condition" }); }
            case ASTType::TheoryAtom:                { return ret({ "term", "elements", "guard" }); }
            case ASTType::Literal:                   { return ret({ "atom" }); }
            case ASTType::TheoryOperatorDefinition:  { return ret({ }); }
            case ASTType::TheoryTermDefinition:      { return ret({ "operators" }); }
            case ASTType::TheoryGuardDefinition:     { return ret({ }); }
            case ASTType::TheoryAtomDefinition:      { return ret({ "guard" }); }
            case ASTType::TheoryDefinition:          { return ret({ "terms", "atoms" }); }
            case ASTType::Rule:                      { return ret({ "head", "body" }); }
            case ASTType::Definition:                { return ret({ "value" }); }
            case ASTType::ShowSignature:             { return ret({ }); }
            case ASTType::Defined:                   { return ret({ }); }
            case ASTType::ShowTerm:                  { return ret({ "term", "body" }); }
            case ASTType::Minimize:                  { return ret({ "weight", "priority", "tuple", "body" }); }
            case ASTType::Script:                    { return ret({ }); }
            case ASTType::Program:                   { return ret({ "parameters" }); }
            case ASTType::External:                  { return ret({ "atom", "body", "external_type" }); }
            case ASTType::Edge:                      { return ret({ "u", "v", "body" }); }
            case ASTType::Heuristic:                 { return ret({ "atom", "body", "bias", "priority", "modifier" }); }
            case ASTType::ProjectAtom:               { return ret({ "atom", "body" }); }
            case ASTType::ProjectSignature:          { return ret({ }); }
        }
        throw std::logic_error("cannot happen");
    }
    Object childKeys() {
        if (!children.valid()) { children = childKeys_(); }
        return children;
    }
    Object getType() {
        return ASTType::getAttr(type_);
    }
    void setType(Reference value) {
        type_ = enumValue<ASTType>(value);
    }
    void tp_setattro(Reference name, Reference value) {
        children = nullptr;
        if (PyObject_GenericSetAttr(toPy(), name.toPy(), value.toPy()) < 0) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
                PyErr_Clear();
                fields_.setItem(name, value);
            }
            else { throw PyException(); }
        }
    }
    Object tp_getattro(Reference name) {
        auto ret = PyDict_GetItem(fields_.toPy(), name.toPy());
        Py_XINCREF(ret);
        return ret
            ? ret
            : PyObject_GenericGetAttr(reinterpret_cast<PyObject*>(this), name.toPy());
    }

    void tp_traverse(Traverse const &visit) {
        visit(fields_);
        visit(children);
    }

    void tp_clear() {
        fields_.clear();
        children.clear();
    }

    void tp_dealloc() {
        tp_clear();
    }

    Object tp_repr() {
        std::ostringstream out;
        switch (type_) {
            // {{{3 term
            case ASTType::Id: { return fields_.getItem("id").str(); }
            case ASTType::Variable: { return fields_.getItem("name").str(); }
            case ASTType::Symbol:   { return fields_.getItem("symbol").str(); }
            case ASTType::UnaryOperation: {
                Object unop = fields_.getItem("operator");
                out << unop.getAttr("left_hand_side") << fields_.getItem("argument") << unop.getAttr("right_hand_side");
                break;
            }
            case ASTType::BinaryOperation: {
                out << "(" << fields_.getItem("left") << fields_.getItem("operator") << fields_.getItem("right") << ")";
                break;
            }
            case ASTType::Interval: {
                out << "(" << fields_.getItem("left") << ".." << fields_.getItem("right") << ")";
                break;
            }
            case ASTType::Function: {
                Object name = fields_.getItem("name"), args = fields_.getItem("arguments");
                bool tc = name.size() == 0 && args.size() == 1;
                bool ey = name.size() == 0 && args.empty();
                out << (fields_.getItem("external").isTrue() ? "@" : "") << name << printList(args, "(", ",", tc ? ",)" : ")", ey);
                break;
            }
            case ASTType::Pool: {
                Object args = fields_.getItem("arguments");
                if (args.empty()) { out << "(1/0)"; }
                else              { out << printList(args, "(", ";", ")", true); }
                break;
            }
            case ASTType::CSPProduct: {
                auto var = fields_.getItem("variable");
                auto coe = fields_.getItem("coefficient");
                if (!var.is_none()) { out << coe << "$*" << "$" << var; }
                else             { out << coe; }
                break;
            }
            case ASTType::CSPSum: {
                auto terms = fields_.getItem("terms");
                if (terms.empty()) { out << "0"; }
                else               { out << printList(terms, "", "$+", "", false); }
                break;
            }
            // {{{3 literal
            case ASTType::Literal: {
                out << fields_.getItem("sign") << fields_.getItem("atom");
                break;
            }
            case ASTType::BooleanConstant: {
                out << (fields_.getItem("value").isTrue() ? "#true" : "#false");
                break;
            }
            case ASTType::SymbolicAtom: {
                out << fields_.getItem("term");
                break;
            }
            case ASTType::Comparison: {
                out << fields_.getItem("left") << fields_.getItem("comparison") << fields_.getItem("right");
                break;
            }
            case ASTType::CSPGuard: {
                out << "$" << fields_.getItem("comparison") << fields_.getItem("term");
                break;
            }
            case ASTType::CSPLiteral: {
                out << fields_.getItem("term") << printList(fields_.getItem("guards"), "", "", "", false);
                break;
            }
            // {{{3 aggregate
            case ASTType::AggregateGuard: {
                out << "AggregateGuard(" << fields_.getItem("comparison") << ", " << fields_.getItem("term") << ")";
                break;
            }
            case ASTType::ConditionalLiteral: {
                out << fields_.getItem("literal") << printList(fields_.getItem("condition"), " : ", ", ", "", true);
                break;
            }
            case ASTType::Aggregate: {
                auto left = fields_.getItem("left_guard"), right = fields_.getItem("right_guard");
                if (!left.is_none()) { out << left.getAttr("term") << " " << left.getAttr("comparison") << " "; }
                out << "{ " << printList(fields_.getItem("elements"), "", "; ", "", false) << " }";
                if (!right.is_none()) { out << " " << right.getAttr("comparison") << " " << right.getAttr("term"); }
                break;
            }
            case ASTType::BodyAggregateElement: {
                out << printList(fields_.getItem("tuple"), "", ",", "", false) << " : " << printList(fields_.getItem("condition"), "", ", ", "", false);
                break;
            }
            case ASTType::BodyAggregate: {
                auto left = fields_.getItem("left_guard"), right = fields_.getItem("right_guard");
                if (!left.is_none()) { out << left.getAttr("term") << " " << left.getAttr("comparison") << " "; }
                out << fields_.getItem("function") << " { " << printList(fields_.getItem("elements"), "", "; ", "", false) << " }";
                if (!right.is_none()) { out << " " << right.getAttr("comparison") << " " << right.getAttr("term"); }
                break;
            }
            case ASTType::HeadAggregateElement: {
                out << printList(fields_.getItem("tuple"), "", ",", "", false) << " : " << fields_.getItem("condition");
                break;
            }
            case ASTType::HeadAggregate: {
                auto left = fields_.getItem("left_guard"), right = fields_.getItem("right_guard");
                if (!left.is_none()) { out << left.getAttr("term") << " " << left.getAttr("comparison") << " "; }
                out << fields_.getItem("function") << " { " << printList(fields_.getItem("elements"), "", "; ", "", false) << " }";
                if (!right.is_none()) { out << " " << right.getAttr("comparison") << " " << right.getAttr("term"); }
                break;
            }
            case ASTType::Disjunction: {
                out << printList(fields_.getItem("elements"), "", "; ", "", false);
                break;
            }
            case ASTType::DisjointElement: {
                out << printList(fields_.getItem("tuple"), "", ",", "", false) << " : " << fields_.getItem("term") << " : " << printList(fields_.getItem("condition"), "", ",", "", false);
                break;
            }
            case ASTType::Disjoint: {
                out << "#disjoint { " << printList(fields_.getItem("elements"), "", "; ", "", false) << " }";
                break;
            }
            // {{{3 theory atom
            case ASTType::TheorySequence: {
                auto type = fields_.getItem("sequence_type"), terms = fields_.getItem("terms");
                bool tc = terms.size() == 1 && type == TheorySequenceType::getAttr(TheorySequenceType::Tuple);
                out << type.getAttr("left_hand_side") << printList(terms, "", ",", "", true) << (tc ? "," : "") << type.getAttr("right_hand_side");
                break;
            }
            case ASTType::TheoryFunction: {
                auto args = fields_.getItem("arguments");
                out << fields_.getItem("name") << printList(args, "(", ",", ")", !args.empty());
                break;
            }
            case ASTType::TheoryUnparsedTermElement: {
                out << printList(fields_.getItem("operators"), "", " ", " ", false) << fields_.getItem("term");
                break;
            }
            case ASTType::TheoryUnparsedTerm: {
                auto elems = fields_.getItem("elements");
                bool pp = elems.size() != 1 || !elems.getItem(0).getAttr("operators").empty();
                out << (pp ? "(" : "") << printList(elems, "", " ", "", false) << (pp ? ")" : "");
                break;
            }
            case ASTType::TheoryGuard: {
                out << fields_.getItem("operator_name") << " " << fields_.getItem("term");
                break;
            }
            case ASTType::TheoryAtomElement: {
                out << printList(fields_.getItem("tuple"), "", ",", "", false) << " : " << printList(fields_.getItem("condition"), "", ",", "", false);
                break;
            }
            case ASTType::TheoryAtom: {
                auto guard = fields_.getItem("guard");
                out << "&" << fields_.getItem("term") << " { " << printList(fields_.getItem("elements"), "", "; ", "", false) << " }";
                if (!guard.is_none()) { out << " " << guard; }
                break;
            }
            // {{{3 theory definition
            case ASTType::TheoryOperatorDefinition: {
                out << fields_.getItem("name") << " : " << fields_.getItem("priority") << ", " << fields_.getItem("operator_type");
                break;
            }
            case ASTType::TheoryTermDefinition: {
                out << fields_.getItem("name") << " {\n" << printList(fields_.getItem("operators"), "  ", ";\n", "\n", true) << "}";
                break;
            }
            case ASTType::TheoryGuardDefinition: {
                out << "{ " << printList(fields_.getItem("operators"), "", ", ", "", false) << " }, " << fields_.getItem("term");
                break;
            }
            case ASTType::TheoryAtomDefinition: {
                auto guard = fields_.getItem("guard");
                out << "&" << fields_.getItem("name") << "/" << fields_.getItem("arity") << " : " << fields_.getItem("elements");
                if (!guard.is_none()) { out << ", " << guard; }
                out << ", " << fields_.getItem("atom_type");
                break;
            }
            case ASTType::TheoryDefinition: {
                out << "#theory " << fields_.getItem("name") << " {\n";
                bool comma = false;
                for (auto y : fields_.getItem("terms").iter()) {
                    if (comma) { out << ";\n"; }
                    else       { comma = true; }
                    out << "  " << y.getAttr("name") << " {\n" << printList(y.getAttr("operators"), "    ", ";\n", "\n", true) << "  }";
                }
                for (auto y : fields_.getItem("atoms").iter()) {
                    if (comma) { out << ";\n"; }
                    else       { comma = true; }
                    out << "  " << y;
                }
                if (comma) { out << "\n"; }
                out << "}.";
                break;
            }
            // {{{3 statement
            case ASTType::Rule: {
                out << fields_.getItem("head") << printBody(fields_.getItem("body"), " :- ");
                break;
            }
            case ASTType::Definition: {
                out << "#const " << fields_.getItem("name") << " = " << fields_.getItem("value") << ".";
                if (!fields_.getItem("is_default").isTrue()) { out << " [override]"; }
                break;
            }
            case ASTType::ShowSignature: {
                out << "#show " << (fields_.getItem("csp").isTrue() ? "$" : "") << (fields_.getItem("positive").isTrue() ? "" : "-") << fields_.getItem("name") << "/" << fields_.getItem("arity") << ".";
                break;
            }
            case ASTType::Defined: {
                out << "#defined " << (fields_.getItem("positive").isTrue() ? "" : "-") << fields_.getItem("name") << "/" << fields_.getItem("arity") << ".";
                break;
            }
            case ASTType::ShowTerm: {
                out << "#show " << (fields_.getItem("csp").isTrue() ? "$" : "") << fields_.getItem("term") << printBody(fields_.getItem("body"));
                break;
            }
            case ASTType::Minimize: {
                out << printBody(fields_.getItem("body"), ":~ ") << " [" << fields_.getItem("weight") << "@" << fields_.getItem("priority") << printList(fields_.getItem("tuple"), ",", ",", "", false) << "]";
                break;
            }
            case ASTType::Script: {
                auto s = pyToCpp<std::string>(fields_.getItem("code"));
                if (!s.empty() && s.back() == '\n') {
                    s.back() = '.';
                }
                out << s;
                break;
            }
            case ASTType::Program: {
                out << "#program " << fields_.getItem("name") << printList(fields_.getItem("parameters"), "(", ",", ")", false) << ".";
                break;
            }
            case ASTType::External: {
                out << "#external " << fields_.getItem("atom") << printBody(fields_.getItem("body")) << " [" << fields_.getItem("external_type") << "]";
                break;
            }
            case ASTType::Edge: {
                out << "#edge (" << fields_.getItem("u") << "," << fields_.getItem("v") << ")" << printBody(fields_.getItem("body"));
                break;
            }
            case ASTType::Heuristic: {
                out << "#heuristic " << fields_.getItem("atom") << printBody(fields_.getItem("body")) << " [" << fields_.getItem("bias")<< "@" << fields_.getItem("priority") << "," << fields_.getItem("modifier") << "]";
                break;
            }
            case ASTType::ProjectAtom: {
                out << "#project " << fields_.getItem("atom") << printBody(fields_.getItem("body"));
                break;
            }
            case ASTType::ProjectSignature: {
                out << "#project " << (fields_.getItem("positive").isTrue() ? "" : "-") << fields_.getItem("name") << "/" << fields_.getItem("arity") << ".";
                break;
            }
            // }}}3
        }
        return cppToPy(out.str().c_str());
    }

    List toList() {
        List k;
        Object loc = PyString_FromString("location");
        for (auto x : keys().iter()) {
            if (x != loc) { k.append(x); }
        }
        k.sort();
        List ret;
        ret.append(ASTType::getAttr(type_));
        for (auto x : k.iter()) {
            ret.append(Tuple{x, mp_subscript(x)});
        }
        return ret;
    }

    Object tp_richcompare(AST &b, int op) {
        return toList().richCompare(b.toList(), op);
    }

    Py_ssize_t mp_length() { return fields_.length(); }
    Object mp_subscript(Reference name) { return fields_.getItem(name); }
    void mp_ass_subscript(Reference name, Reference value) {
        if (value.valid()) { fields_.setItem(name, value); }
        else               { fields_.delItem(name); }
    }
    Object keys() { return fields_.keys(); }
    Object values() { return fields_.values(); }
    Object items() { return fields_.items(); }
    bool sq_contains(Reference value) { return fields_.contains(value); }
    Object tp_iter() { return fields_.iter(); }
};

PyMethodDef AST::tp_methods[] = {
    {"keys", to_function<&AST::keys>(), METH_NOARGS,
R"(keys(self) -> List[str]

The list of keys of the AST node.

Returns
-------
List[str]
)"},
    {"values", to_function<&AST::values>(), METH_NOARGS,
R"(values(self) -> List[AST]

The list of values of the AST node.

Returns
-------
List[AST]
)"},
    {"items", to_function<&AST::items>(), METH_NOARGS,
R"(items(self) -> List[Tuple[str,AST]]

The list of items of the AST node.

Returns
-------
List[Tuple[str,AST]]
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef AST::tp_getset[] = {
    {(char*)"child_keys", to_getter<&AST::childKeys>(), nullptr, (char*)R"(child_keys: List[str]

List of names of all AST child nodes.
)", nullptr},
    {(char*)"type", to_getter<&AST::getType>(), to_setter<&AST::setType>(), (char*)R"(type: ASTType

The type of the node.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{3 terms

CREATE2(Id, location, id)
CREATE2(Variable, location, name)
CREATE2(Symbol, location, symbol)
CREATE3(UnaryOperation, location, operator, argument)
CREATE4(BinaryOperation, location, operator, left, right)
CREATE3(Interval, location, left, right)
CREATE4(Function, location, name, arguments, external)
CREATE2(Pool, location, arguments)
CREATE3(CSPProduct, location, coefficient, variable)
CREATE2(CSPSum, location, terms)

// {{{3 literals

CREATE1(BooleanConstant, value)
CREATE1(SymbolicAtom, term)
CREATE3(Comparison, comparison, left, right)
CREATE2(CSPGuard, comparison, term)
CREATE3(CSPLiteral, location, term, guards)
CREATE3(Literal, location, sign, atom)

// {{{3 aggregates

CREATE2(AggregateGuard, comparison, term)
CREATE3(ConditionalLiteral, location, literal, condition)
CREATE4(Aggregate, location, left_guard, elements, right_guard)
CREATE2(BodyAggregateElement, tuple, condition)
CREATE5(BodyAggregate, location, left_guard, function, elements, right_guard)
CREATE2(HeadAggregateElement, tuple, condition)
CREATE5(HeadAggregate, location, left_guard, function, elements, right_guard)
CREATE2(Disjunction, location, elements)
CREATE4(DisjointElement, location, tuple, term, condition)
CREATE2(Disjoint, location, elements)

// {{{3 theory atom

CREATE3(TheorySequence, location, sequence_type, terms)
CREATE3(TheoryFunction, location, name, arguments)
CREATE2(TheoryUnparsedTermElement, operators, term)
CREATE2(TheoryUnparsedTerm, location, elements)
CREATE2(TheoryGuard, operator_name, term)
CREATE2(TheoryAtomElement, tuple, condition)
CREATE4(TheoryAtom, location, term, elements, guard)

// {{{3 theory definitions

CREATE4(TheoryOperatorDefinition, location, name, priority, operator_type)
CREATE3(TheoryTermDefinition, location, name, operators)
CREATE2(TheoryGuardDefinition, operators, term)
CREATE6(TheoryAtomDefinition, location, atom_type, name, arity, elements, guard)

// {{{3 statements

CREATE3(Rule, location, head, body)
CREATE4(Definition, location, name, value, is_default)
CREATE5(ShowSignature, location, name, arity, positive, csp)
CREATE4(Defined, location, name, arity, positive)
CREATE4(ShowTerm, location, term, body, csp)
CREATE5(Minimize, location, weight, priority, tuple, body)
CREATE3(Script, location, script_type, code)
CREATE3(Program, location, name, parameters)
CREATE4(External, location, atom, body, external_type)
CREATE4(Edge, location, u, v, body)
CREATE6(Heuristic, location, atom, body, bias, priority, modifier)
CREATE3(ProjectAtom, location, atom, body)
CREATE4(ProjectSignature, location, name, arity, positive)
CREATE4(TheoryDefinition, location, name, terms, atoms)

// }}}3

// {{{2 C -> Py

// {{{3 location

Object cppToPy(clingo_location_t const &l) {
    Object dict = PyDict_New();
    auto add = [](char const *n, size_t l, size_t c) -> Object {
        Object loc = PyDict_New();
        Object name = cppToPy(n);
        if (PyDict_SetItemString(loc.toPy(), "filename", name.toPy()) < 0) { throw PyException(); }
        Object line = cppToPy(l);
        if (PyDict_SetItemString(loc.toPy(), "line", line.toPy()) < 0) { throw PyException(); }
        Object column = cppToPy(c);
        if (PyDict_SetItemString(loc.toPy(), "column", column.toPy()) < 0) { throw PyException(); }
        return loc;
    };
    Object begin = add(l.begin_file, l.begin_line, l.begin_column);
    if (PyDict_SetItemString(dict.toPy(), "begin", begin.toPy()) < 0) { throw PyException(); }
    Object end = add(l.end_file, l.end_line, l.end_column);
    if (PyDict_SetItemString(dict.toPy(), "end", end.toPy()) < 0) { throw PyException(); }
    return dict;
}

// {{{3 terms

Object cppToPy(clingo_ast_id_t const &id) {
    return call(createId, cppToPy(id.location), cppToPy(id.id));
}

Object cppToPy(clingo_ast_term_t const &term) {
    switch (static_cast<enum clingo_ast_term_type>(term.type)) {
        case clingo_ast_term_type_symbol: {
            return call(createSymbol, cppToPy(term.location), cppToPy(symbol_wrapper{term.symbol}));
        }
        case clingo_ast_term_type_variable: {
            return call(createVariable, cppToPy(term.location), cppToPy(term.variable));
        }
        case clingo_ast_term_type_unary_operation: {
            auto &op = *term.unary_operation;
            return call(createUnaryOperation, cppToPy(term.location), UnaryOperator::getAttr(op.unary_operator), cppToPy(op.argument));
        }
        case clingo_ast_term_type_binary_operation: {
            auto &op = *term.binary_operation;
            return call(createBinaryOperation, cppToPy(term.location), BinaryOperator::getAttr(op.binary_operator), cppToPy(op.left), cppToPy(op.right));
        }
        case clingo_ast_term_type_interval: {
            auto &x = *term.interval;
            return call(createInterval, cppToPy(term.location), cppToPy(x.left), cppToPy(x.right));
        }
        case clingo_ast_term_type_external_function:
        case clingo_ast_term_type_function: {
            auto &x = *term.function;
            return call(createFunction, cppToPy(term.location), cppToPy(x.name), cppToPy(x.arguments, x.size), cppToPy(term.type == clingo_ast_term_type_external_function));
        }
        case clingo_ast_term_type_pool: {
            auto &x = *term.pool;
            return call(createPool, cppToPy(term.location), cppToPy(x.arguments, x.size));
        }
    }
    throw std::logic_error("cannot happen");
}

Object cppToPy(clingo_ast_term_t const *term) {
    return term ? cppToPy(*term) : None();
}

// csp

Object cppToPy(clingo_ast_csp_product_term const &term) {
    return call(createCSPProduct, cppToPy(term.location), cppToPy(term.coefficient), cppToPy(term.variable));
}

Object cppToPy(clingo_ast_csp_sum_term_t const &term) {
    return call(createCSPSum, cppToPy(term.location), cppToPy(term.terms, term.size));
}

// theory

Object cppToPy(clingo_ast_theory_term_t const &term);
Object cppToPy(clingo_ast_theory_unparsed_term_element_t const &term) {
    return call(createTheoryUnparsedTermElement, cppToPy(term.operators, term.size), cppToPy(term.term));
}

Object cppToPy(clingo_ast_theory_term_t const &term) {
    switch (static_cast<enum clingo_ast_theory_term_type>(term.type)) {
        case clingo_ast_theory_term_type_symbol: {
            return call(createSymbol, cppToPy(term.location), cppToPy(symbol_wrapper{term.symbol}));
        }
        case clingo_ast_theory_term_type_variable: {
            return call(createVariable, cppToPy(term.location), cppToPy(term.variable));
        }
        case clingo_ast_theory_term_type_list: {
            auto &x = *term.list;
            return call(createTheorySequence, cppToPy(term.location), TheorySequenceType::getAttr(TheorySequenceType::List), cppToPy(x.terms, x.size));
        }
        case clingo_ast_theory_term_type_set: {
            auto &x = *term.list;
            return call(createTheorySequence, cppToPy(term.location), TheorySequenceType::getAttr(TheorySequenceType::Set), cppToPy(x.terms, x.size));
        }
        case clingo_ast_theory_term_type_tuple: {
            auto &x = *term.list;
            return call(createTheorySequence, cppToPy(term.location), TheorySequenceType::getAttr(TheorySequenceType::Tuple), cppToPy(x.terms, x.size));
        }
        case clingo_ast_theory_term_type_function: {
            auto &x = *term.function;
            return call(createTheoryFunction, cppToPy(term.location), cppToPy(x.name), cppToPy(x.arguments, x.size));
        }
        case clingo_ast_theory_term_type_unparsed_term: {
            auto &x = *term.unparsed_term;
            return call(createTheoryUnparsedTerm, cppToPy(term.location), cppToPy(x.elements, x.size));
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{3 literal

Object cppToPy(clingo_ast_csp_guard_t const &guard) {
    return call(createCSPGuard, ComparisonOperator::getAttr(guard.comparison), cppToPy(guard.term));
}

Object cppToPy(clingo_ast_literal_t const &lit) {
    switch (static_cast<enum clingo_ast_literal_type>(lit.type)) {
        case clingo_ast_literal_type_boolean: {
            return call(createLiteral, cppToPy(lit.location), Sign::getAttr(lit.sign), call(createBooleanConstant, cppToPy(lit.boolean)));
        }
        case clingo_ast_literal_type_symbolic: {
            return call(createLiteral, cppToPy(lit.location), Sign::getAttr(lit.sign), call(createSymbolicAtom, cppToPy(*lit.symbol)));
        }
        case clingo_ast_literal_type_comparison: {
            auto &c = *lit.comparison;
            return call(createLiteral, cppToPy(lit.location), Sign::getAttr(lit.sign), call(createComparison, ComparisonOperator::getAttr(c.comparison), cppToPy(c.left), cppToPy(c.right)));
        }
        case clingo_ast_literal_type_csp: {
            auto &c = *lit.csp_literal;
            return call(createCSPLiteral, cppToPy(lit.location), cppToPy(c.term), cppToPy(c.guards, c.size));
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{3 aggregates

Object cppToPy(clingo_ast_aggregate_guard_t const *guard) {
    return guard
        ? Object{call(createAggregateGuard, ComparisonOperator::getAttr(guard->comparison), cppToPy(guard->term))}
        : None();
}

Object cppToPy(clingo_ast_conditional_literal_t const &lit) {
    clingo_location_t loc = lit.literal.location;
    if (lit.size > 0) {
        loc.end_file   = lit.condition[lit.size-1].location.end_file;
        loc.end_line   = lit.condition[lit.size-1].location.end_line;
        loc.end_column = lit.condition[lit.size-1].location.end_column;
    }
    return call(createConditionalLiteral, cppToPy(loc), cppToPy(lit.literal), cppToPy(lit.condition, lit.size));
}

Object cppToPy(clingo_location_t loc, clingo_ast_aggregate_t const &aggr) {
    return call(createAggregate, cppToPy(loc), cppToPy(aggr.left_guard), cppToPy(aggr.elements, aggr.size), cppToPy(aggr.right_guard));
}

// theory atom

Object cppToPy(clingo_ast_theory_guard_t const *guard) {
    return guard
        ? Object{call(createTheoryGuard, cppToPy(guard->operator_name), cppToPy(guard->term))}
        : None();
}

Object cppToPy(clingo_ast_theory_atom_element_t const &elem) {
    return call(createTheoryAtomElement, cppToPy(elem.tuple, elem.tuple_size), cppToPy(elem.condition, elem.condition_size));
}

Object cppToPy(clingo_location_t loc, clingo_ast_theory_atom_t const &atom) {
    return call(createTheoryAtom, cppToPy(loc), cppToPy(atom.term), cppToPy(atom.elements, atom.size), cppToPy(atom.guard));
}

// disjoint

Object cppToPy(clingo_ast_disjoint_element_t const &elem) {
    return call(createDisjointElement, cppToPy(elem.location), cppToPy(elem.tuple, elem.tuple_size), cppToPy(elem.term), cppToPy(elem.condition, elem.condition_size));
}

// head aggregates

Object cppToPy(clingo_ast_head_aggregate_element_t const &elem) {
    return call(createHeadAggregateElement, cppToPy(elem.tuple, elem.tuple_size), cppToPy(elem.conditional_literal));
}

// body aggregates

Object cppToPy(clingo_ast_body_aggregate_element_t const &elem) {
    return call(createBodyAggregateElement, cppToPy(elem.tuple, elem.tuple_size), cppToPy(elem.condition, elem.condition_size));
}

// {{{3 head literal

Object cppToPy(clingo_ast_head_literal_t const &head) {
    switch (static_cast<enum clingo_ast_head_literal_type>(head.type)) {
        case clingo_ast_head_literal_type_literal: {
            return cppToPy(*head.literal);
        }
        case clingo_ast_head_literal_type_disjunction: {
            auto &d = *head.disjunction;
            return call(createDisjunction, cppToPy(head.location), cppToPy(d.elements, d.size));
        }
        case clingo_ast_head_literal_type_aggregate: {
            return cppToPy(head.location, *head.aggregate);
        }
        case clingo_ast_head_literal_type_head_aggregate: {
            auto &a = *head.head_aggregate;
            return call(createHeadAggregate, cppToPy(head.location), cppToPy(a.left_guard), AggregateFunction::getAttr(a.function), cppToPy(a.elements, a.size), cppToPy(a.right_guard));
        }
        case clingo_ast_head_literal_type_theory_atom: {
            return cppToPy(head.location, *head.theory_atom);
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{3 body literal

Object cppToPy(clingo_ast_body_literal_t const &body) {
    switch (static_cast<enum clingo_ast_body_literal_type>(body.type)) {
        case clingo_ast_body_literal_type_literal: {
            assert(body.sign == clingo_ast_sign_none);
            return cppToPy(*body.literal);
        }
        case clingo_ast_body_literal_type_conditional: {
            assert(body.sign == clingo_ast_sign_none);
            return cppToPy(*body.conditional);
        }
        case clingo_ast_body_literal_type_aggregate: {
            return call(createLiteral, cppToPy(body.location), Sign::getAttr(body.sign), cppToPy(body.location, *body.aggregate));
        }
        case clingo_ast_body_literal_type_body_aggregate: {
            auto &a = *body.body_aggregate;
            return call(createLiteral, cppToPy(body.location), Sign::getAttr(body.sign), call(createBodyAggregate, cppToPy(body.location), cppToPy(a.left_guard), AggregateFunction::getAttr(a.function), cppToPy(a.elements, a.size), cppToPy(a.right_guard)));
        }
        case clingo_ast_body_literal_type_theory_atom: {
            return call(createLiteral, cppToPy(body.location), Sign::getAttr(body.sign), cppToPy(body.location, *body.theory_atom));
        }
        case clingo_ast_body_literal_type_disjoint: {
            auto &d = *body.disjoint;
            return call(createLiteral, cppToPy(body.location), Sign::getAttr(body.sign), call(createDisjoint, cppToPy(body.location), cppToPy(d.elements, d.size)));
        }
    }
    throw std::logic_error("cannot happen");
}

// {{{3 statement

Object cppToPy(clingo_ast_theory_operator_definition_t const &def) {
    return call(createTheoryOperatorDefinition, cppToPy(def.location), cppToPy(def.name), cppToPy(def.priority), TheoryOperatorType::getAttr(def.type));
}

Object cppToPy(clingo_ast_theory_guard_definition_t const *def) {
    return def
        ? Object{call(createTheoryGuardDefinition, cppToPy(def->operators, def->size), cppToPy(def->term))}
        : None();
}

Object cppToPy(clingo_ast_theory_term_definition_t const &def) {
    return call(createTheoryTermDefinition, cppToPy(def.location), cppToPy(def.name), cppToPy(def.operators, def.size));
}

Object cppToPy(clingo_ast_theory_atom_definition_t const &def) {
    return call(createTheoryAtomDefinition, cppToPy(def.location), TheoryAtomType::getAttr(def.type), cppToPy(def.name), cppToPy(def.arity), cppToPy(def.elements), cppToPy(def.guard));
}

Object cppToPy(clingo_ast_statement_t const &stm) {
    switch (static_cast<enum clingo_ast_statement_type>(stm.type)) {
        case clingo_ast_statement_type_rule: {
            return call(createRule, cppToPy(stm.location), cppToPy(stm.rule->head), cppToPy(stm.rule->body, stm.rule->size));
        }
        case clingo_ast_statement_type_const: {
            return call(createDefinition, cppToPy(stm.location), cppToPy(stm.definition->name), cppToPy(stm.definition->value), cppToPy(stm.definition->is_default));
        }
        case clingo_ast_statement_type_show_signature: {
            auto sig = stm.show_signature->signature;
            return call(createShowSignature, cppToPy(stm.location), cppToPy(clingo_signature_name(sig)), cppToPy(clingo_signature_arity(sig)), cppToPy(clingo_signature_is_positive(sig)), cppToPy(stm.show_signature->csp));
        }
        case clingo_ast_statement_type_defined: {
            auto sig = stm.defined->signature;
            return call(createDefined, cppToPy(stm.location), cppToPy(clingo_signature_name(sig)), cppToPy(clingo_signature_arity(sig)), cppToPy(clingo_signature_is_positive(sig)));
        }
        case clingo_ast_statement_type_show_term: {
            return call(createShowTerm, cppToPy(stm.location), cppToPy(stm.show_term->term), cppToPy(stm.show_term->body, stm.show_term->size), cppToPy(stm.show_term->csp));
        }
        case clingo_ast_statement_type_minimize: {
            auto &min = *stm.minimize;
            return call(createMinimize, cppToPy(stm.location), cppToPy(min.weight), cppToPy(min.priority), cppToPy(min.tuple, min.tuple_size), cppToPy(min.body, min.body_size));
        }
        case clingo_ast_statement_type_script: {
            return call(createScript, cppToPy(stm.location), ScriptType::getAttr(stm.script->type), cppToPy(stm.script->code));
        }
        case clingo_ast_statement_type_program: {
            return call(createProgram, cppToPy(stm.location), cppToPy(stm.program->name), cppToPy(stm.program->parameters, stm.program->size));
        }
        case clingo_ast_statement_type_external: {
            return call(createExternal, cppToPy(stm.location), call(createSymbolicAtom, cppToPy(stm.external->atom)), cppToPy(stm.external->body, stm.external->size), cppToPy(stm.external->type));
        }
        case clingo_ast_statement_type_edge: {
            return call(createEdge, cppToPy(stm.location), cppToPy(stm.edge->u), cppToPy(stm.edge->v), cppToPy(stm.edge->body, stm.edge->size));
        }
        case clingo_ast_statement_type_heuristic: {
            auto &heu = *stm.heuristic;
            return call(createHeuristic, cppToPy(stm.location), call(createSymbolicAtom, cppToPy(heu.atom)), cppToPy(heu.body, heu.size), cppToPy(heu.bias), cppToPy(heu.priority), cppToPy(heu.modifier));
        }
        case clingo_ast_statement_type_project_atom: {
            return call(createProjectAtom, cppToPy(stm.location), call(createSymbolicAtom, cppToPy(stm.project_atom->atom)), cppToPy(stm.project_atom->body, stm.project_atom->size));
        }
        case clingo_ast_statement_type_project_atom_signature: {
            auto sig = stm.project_signature;
            return call(createProjectSignature, cppToPy(stm.location), cppToPy(clingo_signature_name(sig)), cppToPy(clingo_signature_arity(sig)), cppToPy(clingo_signature_is_positive(sig)));
        }
        case clingo_ast_statement_type_theory_definition: {
            auto &def = *stm.theory_definition;
            return call(createTheoryDefinition, cppToPy(stm.location), cppToPy(def.name), cppToPy(def.terms, def.terms_size), cppToPy(def.atoms, def.atoms_size));
        }
    }
    throw std::logic_error("cannot happen");
}

// TODO: consider exposing the logger to python...
Object parseProgram(Reference args, Reference kwds) {
    static char const *kwlist[] = {"program", "callback", nullptr};
    Reference str, cb;
    ParseTupleAndKeywords(args, kwds, "OO", kwlist, str, cb);
    using Data = std::pair<Object, std::exception_ptr>;
    Data data{cb, std::exception_ptr()};
    handle_c_error(clingo_parse_program(pyToCpp<std::string>(str).c_str(), [](clingo_ast_statement_t const *stm, void *d) -> bool {
        auto &data = *static_cast<Data*>(d);
        try {
            data.first(cppToPy(*stm));
            return true;
        }
        catch (...) {
            data.second = std::current_exception();
            return false;
        }
    }, &data, nullptr, nullptr, 20), &data.second);
    return None();
}

// }}}3

// {{{2 Py -> C

struct ASTToC {
    clingo_location_t convLocation(Reference x) {
        clingo_location_t ret;
        Object begin = x.getItem("begin");
        Object end   = x.getItem("end");
        ret.begin_file   = convString(begin.getItem("filename"));
        ret.begin_line   = pyToCpp<size_t>(begin.getItem("line"));
        ret.begin_column = pyToCpp<size_t>(begin.getItem("column"));
        ret.end_file     = convString(end.getItem("filename"));
        ret.end_line     = pyToCpp<size_t>(end.getItem("line"));
        ret.end_column   = pyToCpp<size_t>(end.getItem("column"));
        return ret;
    }

    char const *convString(Reference x) {
        char const *ret;
        handle_c_error(clingo_add_string(pyToCpp<std::string>(x).c_str(), &ret));
        return ret;
    }

    // {{{3 term

    clingo_ast_id_t convId(Reference x) {
        return {convLocation(x.getAttr("location")), convString(x.getAttr("id"))};
    }

    clingo_ast_term_t convTerm(Reference x) {
        clingo_ast_term_t ret;
        ret.location = convLocation(x.getAttr("location"));
        switch (enumValue<ASTType>(x.getAttr("type"))) {
            case ASTType::Variable: {
                ret.type     = clingo_ast_term_type_variable;
                ret.variable = convString(x.getAttr("name"));
                return ret;
            }
            case ASTType::Symbol: {
                ret.type   = clingo_ast_term_type_symbol;
                ret.symbol = pyToCpp<symbol_wrapper>(x.getAttr("symbol")).symbol;
                return ret;
            }
            case ASTType::UnaryOperation: {
                auto unary_operation = create_<clingo_ast_unary_operation_t>();
                unary_operation->unary_operator = enumValue<UnaryOperator>(x.getAttr("operator"));
                unary_operation->argument       = convTerm(x.getAttr("argument"));
                ret.type            = clingo_ast_term_type_unary_operation;
                ret.unary_operation = unary_operation;
                return ret;
            }
            case ASTType::BinaryOperation: {
                auto binary_operation = create_<clingo_ast_binary_operation_t>();
                binary_operation->binary_operator = enumValue<BinaryOperator>(x.getAttr("operator"));
                binary_operation->left            = convTerm(x.getAttr("left"));
                binary_operation->right           = convTerm(x.getAttr("right"));
                ret.type             = clingo_ast_term_type_binary_operation;
                ret.binary_operation = binary_operation;
                return ret;
            }
            case ASTType::Interval: {
                auto interval = create_<clingo_ast_interval_t>();
                interval->left  = convTerm(x.getAttr("left"));
                interval->right = convTerm(x.getAttr("right"));
                ret.type     = clingo_ast_term_type_interval;
                ret.interval = interval;
                return ret;
            }
            case ASTType::Function: {
                auto function = create_<clingo_ast_function_t>();
                auto args = x.getAttr("arguments");
                function->name      = convString(x.getAttr("name"));
                function->arguments = convTermVec(args);
                function->size      = args.size();
                ret.type     = x.getAttr("external").isTrue() ? clingo_ast_term_type_external_function : clingo_ast_term_type_function;
                ret.function = function;
                return ret;
            }
            case ASTType::Pool: {
                auto pool = create_<clingo_ast_pool_t>();
                auto args = x.getAttr("arguments");
                pool->arguments = convTermVec(args);
                pool->size      = args.size();
                ret.type = clingo_ast_term_type_pool;
                ret.pool = pool;
                return ret;
            }
            default: {
                throw std::runtime_error("term expected");
            }
        }
    }

    clingo_ast_term_t *convTermVec(Reference x) {
        return createArray_(x, &ASTToC::convTerm);
    }

    clingo_ast_term_t *convTermOpt(Reference x) {
        return !x.is_none() ? create_(convTerm(x)) : nullptr;
    }

    clingo_ast_csp_product_term_t convCSPProduct(Reference x) {
        clingo_ast_csp_product_term_t ret;
        ret.location    = convLocation(x.getAttr("location"));
        ret.variable    = convTermOpt(x.getAttr("variable"));
        ret.coefficient = convTerm(x.getAttr("coefficient"));
        return ret;
    }

    clingo_ast_csp_sum_term_t convCSPAdd(Reference x) {
        clingo_ast_csp_sum_term_t ret;
        auto terms = x.getAttr("terms");
        ret.location = convLocation(x.getAttr("location"));
        ret.terms    = createArray_(terms, &ASTToC::convCSPProduct);
        ret.size     = terms.size();
        return ret;
    }

    clingo_ast_theory_unparsed_term_element_t convTheoryUnparsedTermElement(Reference x) {
        auto ops= x.getAttr("operators");
        clingo_ast_theory_unparsed_term_element_t ret;
        ret.term      = convTheoryTerm(x.getAttr("term"));
        ret.operators = createArray_(ops, &ASTToC::convString);
        ret.size      = ops.size();
        return ret;
    }

    clingo_ast_theory_term_t convTheoryTerm(Reference x) {
        clingo_ast_theory_term_t ret;
        ret.location = convLocation(x.getAttr("location"));
        switch (enumValue<ASTType>(x.getAttr("type"))) {
            case ASTType::Variable: {
                ret.type     = clingo_ast_theory_term_type_variable;
                ret.variable = convString(x.getAttr("name"));
                return ret;
            }
            case ASTType::Symbol: {
                ret.type   = clingo_ast_theory_term_type_symbol;
                ret.symbol = pyToCpp<symbol_wrapper>(x.getAttr("symbol")).symbol;
                return ret;
            }
            case ASTType::TheorySequence: {
                auto sequence = create_<clingo_ast_theory_term_array_t>();
                auto terms = x.getAttr("terms");
                sequence->terms = convTheoryTermVec(terms);
                sequence->size  = terms.size();
                switch (enumValue<TheorySequenceType>(x.getAttr("sequence_type"))) {
                    case TheorySequenceType::Set:   { ret.type = clingo_ast_theory_term_type_set; break; }
                    case TheorySequenceType::List:  { ret.type = clingo_ast_theory_term_type_list; break; }
                    case TheorySequenceType::Tuple: { ret.type = clingo_ast_theory_term_type_tuple; break; }
                }
                ret.set = sequence;
                return ret;
            }
            case ASTType::TheoryFunction: {
                auto function = create_<clingo_ast_theory_function_t>();
                auto args = x.getAttr("arguments");
                function->name      = convString(x.getAttr("name"));
                function->arguments = convTheoryTermVec(args);
                function->size      = args.size();
                ret.type     = clingo_ast_theory_term_type_function;
                ret.function = function;
                return ret;
            }
            case ASTType::TheoryUnparsedTerm: {
                auto unparsed_term = create_<clingo_ast_theory_unparsed_term>();
                auto elems = x.getAttr("elements");
                unparsed_term->elements = createArray_(elems, &ASTToC::convTheoryUnparsedTermElement);
                unparsed_term->size     = elems.size();
                ret.type          = clingo_ast_theory_term_type_unparsed_term;
                ret.unparsed_term = unparsed_term;
                return ret;

            }
            default: {
                throw std::runtime_error("theory term expected");
            }
        }
        return ret;
    }
    clingo_ast_theory_term_t *convTheoryTermVec(Reference x) {
        return createArray_(x, &ASTToC::convTheoryTerm);
    }

    // {{{3 literal

    clingo_ast_csp_guard_t convCSPGuard(Reference x) {
        clingo_ast_csp_guard_t ret;
        ret.comparison = enumValue<ComparisonOperator>(x.getAttr("comparison"));
        ret.term       = convCSPAdd(x.getAttr("term"));
        return ret;
    }

    clingo_ast_term_t convSymbolicAtom(Reference x) {
        return convTerm(x.getAttr("term"));
    }
    clingo_ast_literal_t convLiteral(Reference x) {
        clingo_ast_literal_t ret;
        ret.location = convLocation(x.getAttr("location"));
        if (enumValue<ASTType>(x.getAttr("type")) == ASTType::CSPLiteral) {
            auto csp = create_<clingo_ast_csp_literal_t>();
            auto guards = x.getAttr("guards");
            csp->term   = convCSPAdd(x.getAttr("term"));
            csp->guards = createArray_(guards, &ASTToC::convCSPGuard);
            csp->size   = guards.size();
            ret.sign        = clingo_ast_sign_none;
            ret.type        = clingo_ast_literal_type_csp;
            ret.csp_literal = csp;
            return ret;
        }
        auto atom = x.getAttr("atom");
        ret.sign = enumValue<Sign>(x.getAttr("sign"));
        switch (enumValue<ASTType>(atom.getAttr("type"))) {
            case ASTType::BooleanConstant: {
                ret.type     = clingo_ast_literal_type_boolean;
                ret.boolean  = pyToCpp<bool>(atom.getAttr("value"));
                return ret;
            }
            case ASTType::SymbolicAtom: {
                ret.type   = clingo_ast_literal_type_symbolic;
                ret.symbol = create_<clingo_ast_term_t>(convSymbolicAtom(atom));
                return ret;
            }
            case ASTType::Comparison: {
                auto comparison = create_<clingo_ast_comparison_t>();
                comparison->comparison = enumValue<ComparisonOperator>(atom.getAttr("comparison"));
                comparison->left       = convTerm(atom.getAttr("left"));
                comparison->right      = convTerm(atom.getAttr("right"));
                ret.type       = clingo_ast_literal_type_comparison;
                ret.comparison = comparison;
                return ret;
            }
            case ASTType::CSPLiteral: {
            }
            default: {
                throw std::runtime_error("literal expected");
            }
        }
    }
    clingo_ast_literal_t *convLiteralVec(Reference x) {
        return createArray_(x, &ASTToC::convLiteral);
    }

    // {{{3 aggregates

    clingo_ast_aggregate_guard_t *convAggregateGuardOpt(Reference x) {
        return !x.is_none()
            ? create_<clingo_ast_aggregate_guard_t>({enumValue<ComparisonOperator>(x.getAttr("comparison")), convTerm(x.getAttr("term"))})
            : nullptr;
    }

    clingo_ast_conditional_literal_t convConditionalLiteral(Reference x) {
        clingo_ast_conditional_literal_t ret;
        auto cond = x.getAttr("condition");
        ret.literal   = convLiteral(x.getAttr("literal"));
        ret.condition = convLiteralVec(cond);
        ret.size      = cond.size();
        return ret;
    }

    clingo_ast_theory_guard_t *convTheoryGuardOpt(Reference x) {
        return !x.is_none()
            ? create_<clingo_ast_theory_guard_t>({convString(x.getAttr("operator_name")), convTheoryTerm(x.getAttr("term"))})
            : nullptr;
    }

    clingo_ast_theory_atom_element_t convTheoryAtomElement(Reference x) {
        clingo_ast_theory_atom_element_t ret;
        auto tuple = x.getAttr("tuple"), cond = x.getAttr("condition");
        ret.tuple          = convTheoryTermVec(tuple);
        ret.tuple_size     = tuple.size();
        ret.condition      = convLiteralVec(cond);
        ret.condition_size = cond.size();
        return ret;
    }

    clingo_ast_body_aggregate_element_t convBodyAggregateElement(Reference x) {
        clingo_ast_body_aggregate_element_t ret;
        auto tuple = x.getAttr("tuple"), cond = x.getAttr("condition");
        ret.tuple          = convTermVec(tuple);
        ret.tuple_size     = tuple.size();
        ret.condition      = convLiteralVec(cond);
        ret.condition_size = cond.size();
        return ret;
    }

    clingo_ast_head_aggregate_element_t convHeadAggregateElement(Reference x) {
        clingo_ast_head_aggregate_element_t ret;
        auto tuple = x.getAttr("tuple");
        ret.tuple               = convTermVec(tuple);
        ret.tuple_size          = tuple.size();
        ret.conditional_literal = convConditionalLiteral(x.getAttr("condition"));
        return ret;
    }

    clingo_ast_aggregate_t convAggregate(Reference x) {
        clingo_ast_aggregate_t ret;
        auto elems = x.getAttr("elements");
        ret.left_guard  = convAggregateGuardOpt(x.getAttr("left_guard"));
        ret.right_guard = convAggregateGuardOpt(x.getAttr("right_guard"));
        ret.size        = elems.size();
        ret.elements    = createArray_(elems, &ASTToC::convConditionalLiteral);
        return ret;
    }

    clingo_ast_theory_atom_t convTheoryAtom(Reference x) {
        clingo_ast_theory_atom_t ret;
        auto elems = x.getAttr("elements");
        ret.term     = convTerm(x.getAttr("term"));
        ret.guard    = convTheoryGuardOpt(x.getAttr("guard"));
        ret.elements = createArray_(elems, &ASTToC::convTheoryAtomElement);
        ret.size     = elems.size();
        return ret;
    }

    clingo_ast_disjoint_element_t convDisjointElement(Reference x) {
        clingo_ast_disjoint_element_t ret;
        auto tuple = x.getAttr("tuple"), cond = x.getAttr("condition");
        ret.location       = convLocation(x.getAttr("location"));
        ret.tuple          = convTermVec(tuple);
        ret.tuple_size     = tuple.size();
        ret.term           = convCSPAdd(x.getAttr("term"));
        ret.condition      = convLiteralVec(cond);
        ret.condition_size = cond.size();
        return ret;
    }

    // {{{3 head literal

    clingo_ast_head_literal_t convHeadLiteral(Object x) {
        clingo_ast_head_literal_t ret;
        ret.location = convLocation(x.getAttr("location"));
        switch (enumValue<ASTType>(x.getAttr("type"))) {
            case ASTType::CSPLiteral:
            case ASTType::Literal: {
                ret.type    = clingo_ast_head_literal_type_literal;
                ret.literal = create_<clingo_ast_literal_t>(convLiteral(x));
                return ret;
            }
            case ASTType::Aggregate: {
                ret.type      = clingo_ast_head_literal_type_aggregate;
                ret.aggregate = create_<clingo_ast_aggregate_t>(convAggregate(x));
                return ret;
            }
            case ASTType::HeadAggregate: {
                auto head_aggregate = create_<clingo_ast_head_aggregate_t>();
                auto elems = x.getAttr("elements");
                head_aggregate->left_guard  = convAggregateGuardOpt(x.getAttr("left_guard"));
                head_aggregate->right_guard = convAggregateGuardOpt(x.getAttr("right_guard"));
                head_aggregate->function    = enumValue<AggregateFunction>(x.getAttr("function"));
                head_aggregate->size        = elems.size();
                head_aggregate->elements    = createArray_(elems, &ASTToC::convHeadAggregateElement);
                ret.type           = clingo_ast_head_literal_type_head_aggregate;
                ret.head_aggregate = head_aggregate;
                return ret;
            }
            case ASTType::Disjunction: {
                auto disjunction = create_<clingo_ast_disjunction_t>();
                auto elems = x.getAttr("elements");
                disjunction->size     = elems.size();
                disjunction->elements = createArray_(elems, &ASTToC::convConditionalLiteral);
                ret.type        = clingo_ast_head_literal_type_disjunction;
                ret.disjunction = disjunction;
                return ret;
            }
            case ASTType::TheoryAtom: {
                ret.type        = clingo_ast_head_literal_type_theory_atom;
                ret.theory_atom = create_<clingo_ast_theory_atom_t>(convTheoryAtom(x));
                return ret;
            }
            default: {
                throw std::runtime_error("head literal expected");
            }
        }
        return ret;
    }

    // {{{3 body literal

    clingo_ast_body_literal_t convBodyLiteral(Object x) {
        clingo_ast_body_literal_t ret;
        ret.location = convLocation(x.getAttr("location"));
        if (enumValue<ASTType>(x.getAttr("type")) == ASTType::ConditionalLiteral) {
            ret.sign        = clingo_ast_sign_none;
            ret.type        = clingo_ast_body_literal_type_conditional;
            ret.conditional = create_<clingo_ast_conditional_literal_t>(convConditionalLiteral(x));
            return ret;
        }
        else if (enumValue<ASTType>(x.getAttr("type")) == ASTType::CSPLiteral) {
            ret.sign    = clingo_ast_sign_none;
            ret.type    = clingo_ast_body_literal_type_literal;
            ret.literal = create_<clingo_ast_literal_t>(convLiteral(x));
            return ret;
        }
        auto atom = x.getAttr("atom");
        switch (enumValue<ASTType>(atom.getAttr("type"))) {
            case ASTType::Aggregate: {
                ret.sign      = enumValue<Sign>(x.getAttr("sign"));
                ret.type      = clingo_ast_body_literal_type_aggregate;
                ret.aggregate = create_<clingo_ast_aggregate_t>(convAggregate(atom));
                return ret;
            }
            case ASTType::BodyAggregate: {
                auto body_aggregate = create_<clingo_ast_body_aggregate_t>();
                auto elems = atom.getAttr("elements");
                body_aggregate->left_guard  = convAggregateGuardOpt(atom.getAttr("left_guard"));
                body_aggregate->right_guard = convAggregateGuardOpt(atom.getAttr("right_guard"));
                body_aggregate->function    = enumValue<AggregateFunction>(atom.getAttr("function"));
                body_aggregate->size        = elems.size();
                body_aggregate->elements    = createArray_(elems, &ASTToC::convBodyAggregateElement);
                ret.sign           = enumValue<Sign>(x.getAttr("sign"));
                ret.type           = clingo_ast_body_literal_type_body_aggregate;
                ret.body_aggregate = body_aggregate;
                return ret;
            }
            case ASTType::TheoryAtom: {
                ret.sign        = enumValue<Sign>(x.getAttr("sign"));
                ret.type        = clingo_ast_body_literal_type_theory_atom;
                ret.theory_atom = create_<clingo_ast_theory_atom_t>(convTheoryAtom(atom));
                return ret;
            }
            case ASTType::Disjoint: {
                auto disjoint = create_<clingo_ast_disjoint_t>();
                auto elems = atom.getAttr("elements");
                disjoint->size     = elems.size();
                disjoint->elements = createArray_(elems, &ASTToC::convDisjointElement);
                ret.sign     = enumValue<Sign>(x.getAttr("sign"));
                ret.type     = clingo_ast_body_literal_type_disjoint;
                ret.disjoint = disjoint;
                return ret;
            }
            default: {
                ret.sign    = clingo_ast_sign_none;
                ret.type    = clingo_ast_body_literal_type_literal;
                ret.literal = create_<clingo_ast_literal_t>(convLiteral(x));
                return ret;
            }
        }
    }
    clingo_ast_body_literal_t *convBodyLiteralVec(Object x) {
        return createArray_(x, &ASTToC::convBodyLiteral);
    }

    // {{{3 theory definitions

    clingo_ast_theory_operator_definition_t convTheoryOperatorDefinition(Reference x) {
        clingo_ast_theory_operator_definition_t ret;
        ret.type     = enumValue<TheoryOperatorType>(x.getAttr("operator_type"));
        ret.priority = pyToCpp<unsigned>(x.getAttr("priority"));
        ret.location = convLocation(x.getAttr("location"));
        ret.name     = convString(x.getAttr("name"));
        return ret;
    }

    clingo_ast_theory_term_definition_t convTheoryTermDefinition(Reference x) {
        clingo_ast_theory_term_definition_t ret;
        auto ops = x.getAttr("operators");
        ret.name      = convString(x.getAttr("name"));
        ret.location  = convLocation(x.getAttr("location"));
        ret.operators = createArray_(ops, &ASTToC::convTheoryOperatorDefinition);
        ret.size      = ops.size();
        return ret;
    }

    clingo_ast_theory_guard_definition_t *convTheoryGuardDefinitionOpt(Reference x) {
        if (x.is_none()) { return nullptr; }
        auto ret = create_<clingo_ast_theory_guard_definition_t>();
        auto ops = x.getAttr("operators");
        ret->term      = convString(x.getAttr("term"));
        ret->operators = createArray_(ops, &ASTToC::convString);
        ret->size      = ops.size();
        return ret;
    }

    clingo_ast_theory_atom_definition_t convTheoryAtomDefinition(Reference x) {
        clingo_ast_theory_atom_definition_t ret;
        auto guard = x.getAttr("guard");
        ret.name     = convString(x.getAttr("name"));
        ret.arity    = pyToCpp<unsigned>(x.getAttr("arity"));
        ret.location = convLocation(x.getAttr("location"));
        ret.type     = enumValue<TheoryAtomType>(x.getAttr("atom_type"));
        ret.elements = convString(x.getAttr("elements"));
        ret.guard    = convTheoryGuardDefinitionOpt(x.getAttr("guard"));
        return ret;
    }

    // {{{3 statement

    clingo_ast_statement_t convStatement(Reference x) {
        clingo_ast_statement_t ret;
        ret.location = convLocation(x.getAttr("location"));
        switch (enumValue<ASTType>(x.getAttr("type"))) {
            case ASTType::Rule: {
                auto *rule = create_<clingo_ast_rule_t>();
                auto body = x.getAttr("body");
                rule->head = convHeadLiteral(x.getAttr("head"));
                rule->size = body.size();
                rule->body = convBodyLiteralVec(body);
                ret.type = clingo_ast_statement_type_rule;
                ret.rule = rule;
                return ret;
            }
            case ASTType::Definition: {
                auto *definition = create_<clingo_ast_definition_t>();
                definition->is_default = pyToCpp<bool>(x.getAttr("is_default"));
                definition->name       = convString(x.getAttr("name"));
                definition->value      = convTerm(x.getAttr("value"));
                ret.type       = clingo_ast_statement_type_const;
                ret.definition = definition;
                return ret;
            }
            case ASTType::ShowSignature: {
                auto *show_signature = create_<clingo_ast_show_signature_t>();
                show_signature->csp = pyToCpp<bool>(x.getAttr("csp"));
                handle_c_error(clingo_signature_create(convString(x.getAttr("name")), pyToCpp<unsigned>(x.getAttr("arity")), pyToCpp<bool>(x.getAttr("positive")), &show_signature->signature));
                ret.type           = clingo_ast_statement_type_show_signature;
                ret.show_signature = show_signature;
                return ret;
            }
            case ASTType::Defined: {
                auto *defined = create_<clingo_ast_defined_t>();
                handle_c_error(clingo_signature_create(convString(x.getAttr("name")), pyToCpp<unsigned>(x.getAttr("arity")), pyToCpp<bool>(x.getAttr("positive")), &defined->signature));
                ret.type  = clingo_ast_statement_type_defined;
                ret.defined = defined;
                return ret;
            }
            case ASTType::ShowTerm: {
                auto *show_term = create_<clingo_ast_show_term_t>();
                auto body = x.getAttr("body");
                show_term->csp  = pyToCpp<bool>(x.getAttr("csp"));
                show_term->term = convTerm(x.getAttr("term"));
                show_term->body = convBodyLiteralVec(body);
                show_term->size = body.size();
                ret.type      = clingo_ast_statement_type_show_term;
                ret.show_term = show_term;
                return ret;
            }
            case ASTType::Minimize: {
                auto *minimize = create_<clingo_ast_minimize_t>();
                auto tuple = x.getAttr("tuple"), body = x.getAttr("body");
                minimize->weight     = convTerm(x.getAttr("weight"));
                minimize->priority   = convTerm(x.getAttr("priority"));
                minimize->tuple      = convTermVec(tuple);
                minimize->tuple_size = tuple.size();
                minimize->body       = convBodyLiteralVec(body);
                minimize->body_size  = body.size();
                ret.type     = clingo_ast_statement_type_minimize;
                ret.minimize = minimize;
                return ret;
            }
            case ASTType::Script: {
                auto *script = create_<clingo_ast_script_t>();
                script->type = enumValue<ScriptType>(x.getAttr("script_type"));
                script->code = convString(x.getAttr("code"));
                ret.type   = clingo_ast_statement_type_script;
                ret.script = script;
                return ret;
            }
            case ASTType::Program: {
                auto *program = create_<clingo_ast_program_t>();
                auto params = x.getAttr("parameters");
                program->name       = convString(x.getAttr("name"));
                program->parameters = createArray_(params, &ASTToC::convId);
                program->size       = params.size();
                ret.type    = clingo_ast_statement_type_program;
                ret.program = program;
                return ret;
            }
            case ASTType::External: {
                auto *external = create_<clingo_ast_external_t>();
                auto body = x.getAttr("body");
                external->atom = convSymbolicAtom(x.getAttr("atom"));
                external->body = convBodyLiteralVec(body);
                external->size = body.size();
                external->type = convTerm(x.getAttr("external_type"));
                ret.type     = clingo_ast_statement_type_external;
                ret.external = external;
                return ret;
            }
            case ASTType::Edge: {
                auto *edge = create_<clingo_ast_edge_t>();
                auto body = x.getAttr("body");
                edge->u    = convTerm(x.getAttr("u"));
                edge->v    = convTerm(x.getAttr("v"));
                edge->body = convBodyLiteralVec(body);
                edge->size = body.size();
                ret.type = clingo_ast_statement_type_edge;
                ret.edge = edge;
                return ret;
            }
            case ASTType::Heuristic: {
                auto *heuristic = create_<clingo_ast_heuristic_t>();
                auto body = x.getAttr("body");
                heuristic->atom     = convSymbolicAtom(x.getAttr("atom"));
                heuristic->bias     = convTerm(x.getAttr("bias"));
                heuristic->priority = convTerm(x.getAttr("priority"));
                heuristic->modifier = convTerm(x.getAttr("modifier"));
                heuristic->body     = convBodyLiteralVec(body);
                heuristic->size     = body.size();
                ret.type      = clingo_ast_statement_type_heuristic;
                ret.heuristic = heuristic;
                return ret;
            }
            case ASTType::ProjectAtom: {
                auto *project = create_<clingo_ast_project_t>();
                auto body = x.getAttr("body");
                project->atom = convSymbolicAtom(x.getAttr("atom"));
                project->body = convBodyLiteralVec(body);
                project->size = body.size();
                ret.type         = clingo_ast_statement_type_project_atom;
                ret.project_atom = project;
                return ret;
            }
            case ASTType::ProjectSignature: {
                ret.type = clingo_ast_statement_type_project_atom_signature;
                handle_c_error(clingo_signature_create(convString(x.getAttr("name")), pyToCpp<unsigned>(x.getAttr("arity")), pyToCpp<bool>(x.getAttr("positive")), &ret.project_signature));
                return ret;
            }
            case ASTType::TheoryDefinition: {
                auto *theory_definition = create_<clingo_ast_theory_definition_t>();
                auto terms = x.getAttr("terms"), atoms = x.getAttr("atoms");
                theory_definition->name       = convString(x.getAttr("name"));
                theory_definition->terms      = createArray_(terms, &ASTToC::convTheoryTermDefinition);
                theory_definition->terms_size = terms.size();
                theory_definition->atoms      = createArray_(atoms, &ASTToC::convTheoryAtomDefinition);
                theory_definition->atoms_size = atoms.size();
                ret.type              = clingo_ast_statement_type_theory_definition;
                ret.theory_definition = theory_definition;
                return ret;
            }
            default: {
                throw std::runtime_error("statement expected");
            }
        }
    }

    // {{{3 aux

    template <class T>
    T *create_() {
        data_.emplace_back(operator new(sizeof(T)));
        return reinterpret_cast<T*>(data_.back());
    }
    template <class T>
    T *create_(T x) {
        auto *r = create_<T>();
        *r = x;
        return r;
    }
    template <class T>
    T *createArray_(size_t size) {
        arrdata_.emplace_back(operator new[](sizeof(T) * size));
        return reinterpret_cast<T*>(arrdata_.back());
    }
    template <class F>
    auto createArray_(Reference vec, F f) -> decltype((this->*f)(std::declval<Object>()))* {
        using U = decltype((this->*f)(std::declval<Object>()));
        auto r = createArray_<U>(vec.size()), jt = r;
        for (auto x : vec.iter()) { *jt++ = (this->*f)(x); }
        return r;
    }

    ~ASTToC() noexcept {
        for (auto &x : data_) { operator delete(x); }
        for (auto &x : arrdata_) { operator delete[](x); }
        data_.clear();
        arrdata_.clear();
    }

    std::vector<void *> data_;
    std::vector<void *> arrdata_;

    // }}}3
};

// }}}2

// {{{1 wrap ProgramBuilder

struct ProgramBuilder : ObjectBase<ProgramBuilder> {
    clingo_program_builder_t *builder;
    bool locked;
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static constexpr char const *tp_type = "ProgramBuilder";
    static constexpr char const *tp_name = "clingo.ProgramBuilder";
    static constexpr char const *tp_doc =
R"(Object to build non-ground programs.

See Also
--------
Control.builder, parse_program

Notes
-----
A `ProgramBuilder` is a context manager and must be used with Python's `with`
statement.

Examples
--------
The following example parses a program from a string and passes the resulting
`AST` to the builder:

    >>> import clingo
    >>> ctl = clingo.Control()
    >>> prg = "a."
    >>> with ctl.builder() as bld:
    ...    clingo.parse_program(prg, lambda stm: bld.add(stm))
    ...
    >>> ctl.ground([("base", [])])
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: a
    SAT
)";

    static Object construct(clingo_program_builder_t *builder) {
        auto self = new_();
        self->builder = builder;
        self->locked  = true;
        return self;
    }
    Object add(Reference pyStm) {
        if (locked) { throw std::runtime_error("__enter__ has not been called"); }
        ASTToC toc;
        auto stm = toc.convStatement(pyStm);
        handle_c_error(clingo_program_builder_add(builder, &stm));
        return None();
    }
    Reference enter() {
        if (!locked) { throw std::runtime_error("__enter__ already called"); }
        locked = false;
        handle_c_error(clingo_program_builder_begin(builder));
        return *this;
    }
    Object exit() {
        if (locked) { throw std::runtime_error("__enter__ has not been called"); }
        locked = true;
        handle_c_error(clingo_program_builder_end(builder));
        return cppToPy(false);
    }

    Object to_c() {
        return PyLong_FromVoidPtr(builder);
    }
};

PyMethodDef ProgramBuilder::tp_methods[] = {
    {"__enter__", to_function<&ProgramBuilder::enter>(), METH_NOARGS,
R"(__enter__(self) -> ProgramBuilder

Begin building a program.

Must be called before adding statements.)"},
    {"add", to_function<&ProgramBuilder::add>(), METH_O,
R"(add(self, statement: ast.AST) -> None

Adds a statement in form of an `ast.AST` node to the program.

Parameters
----------
statement: ast.AST
    The statement to add.

Returns
-------
None
)"},
    {"__exit__", to_function<&ProgramBuilder::exit>(), METH_VARARGS,
R"(__exit__(self, type : Optional[Type[BaseException]], value : Optional[BaseException], traceback : Optional[TracebackType]) -> bool

Finish building a program.

Follows Python's __exit__ conventions. Does not suppress exceptions.
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef ProgramBuilder::tp_getset[] = {
    {(char *)"_to_c", to_getter<&ProgramBuilder::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_program_builder_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap MessageCode

struct MessageCode : EnumType<MessageCode> {
    static constexpr char const *tp_type = "MessageCode";
    static constexpr char const *tp_name = "clingo.MessageCode";
    static constexpr char const *tp_doc =
R"(Enumeration of the different types of messages.

`MessageCode` objects have a readable string representation, implement Python's
rich comparison operators, and can be used as dictionary keys.

Furthermore, they cannot be constructed from Python. Instead the following
preconstructed class attributes are available:

Attributes
----------
OperationUndefined : MessageCode
    Inform about an undefined arithmetic operation or unsupported weight of an
    aggregate.
RuntimeError : MessageCode
    To report multiple errors; a corresponding runtime error is raised later.
AtomUndefined : MessageCode
    Informs about an undefined atom in program.
FileIncluded : MessageCode
    Indicates that the same file was included multiple times.
VariableUnbounded : MessageCode
    Informs about a CSP variable with an unbounded domain.
GlobalVariable : MessageCode
    Informs about a global variable in a tuple of an aggregate element.
Other : MessageCode
    Reports other kinds of messages.
)";
    static constexpr clingo_warning const values[] = {
        clingo_warning_operation_undefined,
        clingo_warning_runtime_error,
        clingo_warning_atom_undefined,
        clingo_warning_file_included,
        clingo_warning_variable_unbounded,
        clingo_warning_global_variable,
        clingo_warning_other,
    };
    static constexpr const char * const strings[] = {
        "OperationUndefined",
        "RuntimeError",
        "AtomUndefined",
        "FileIncluded",
        "VariableUnbounded",
        "GlobalVariable",
        "Other"
    };
};

constexpr clingo_warning const MessageCode::values[];
constexpr const char * const MessageCode::strings[];

// {{{1 wrap Statistics

Object getStatistics(clingo_statistics_t const *stats, uint64_t key) {
    clingo_statistics_type_t type;
    handle_c_error(clingo_statistics_type(stats, key, &type));
    switch (type) {
        case clingo_statistics_type_value: {
            double val;
            handle_c_error(clingo_statistics_value_get(stats, key, &val));
            return cppToPy(val);
        }
        case clingo_statistics_type_array: {
            size_t e;
            handle_c_error(clingo_statistics_array_size(stats, key, &e));
            List list;
            for (size_t i = 0; i != e; ++i) {
                uint64_t subkey;
                handle_c_error(clingo_statistics_array_at(stats, key, i, &subkey));
                list.append(getStatistics(stats, subkey));
            }
            return list;
        }
        case clingo_statistics_type_map: {
            size_t e;
            handle_c_error(clingo_statistics_map_size(stats, key, &e));
            Dict dict;
            for (size_t i = 0; i != e; ++i) {
                char const *name;
                uint64_t subkey;
                handle_c_error(clingo_statistics_map_subkey_name(stats, key, i, &name));
                handle_c_error(clingo_statistics_map_at(stats, key, name, &subkey));
                dict.setItem(name, getStatistics(stats, subkey));
            }
            return dict;
        }
        default: {
            throw std::logic_error("cannot happen");
        }
    }
}

void setUserStatistics(clingo_statistics_t *stats, uint64_t key, clingo_statistics_type_t type, Reference value, bool update);
clingo_statistics_type_t getUserStatisticsType(Reference value) {
    // there is an extra check for strings
    // because recursively iterating over them causes stack overflows
    if (value.is_str())                             { throw std::runtime_error("unexpected string"); }
    else if (value.is_number() || value.callable()) { return clingo_statistics_type_value; }
    else if (value.callable("items"))               { return clingo_statistics_type_map; }
    else                                            { return clingo_statistics_type_array; }
}

struct StatisticsArray : ObjectBase<StatisticsArray> {
    clingo_statistics_t *stats;
    uint64_t key;

    static PyGetSetDef tp_getset[];
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "StatisticsArray";
    static constexpr char const *tp_name = "clingo.StatisticsArray";
    static constexpr char const *tp_doc =
    R"(Object to modify statistics stored in an array.

This class implements `Sequence[Union[StatisticsArray,StatisticsMap,float]]`
but only supports inplace concatenation and does not support deletion.

See Also
--------
Control.solve

Notes
-----
The `StatisticsArray.update` function provides convenient means to initialize
and modify a statistics array.
)";

    static SharedObject<StatisticsArray> construct(clingo_statistics_t *stats, int64_t key) {
        auto self = new_();
        self->stats = stats;
        self->key = key;
        return self;
    }
    Py_ssize_t sq_length() {
        size_t size;
        handle_c_error(clingo_statistics_array_size(stats, key, &size));
        return size;
    }
    Object sq_item(Py_ssize_t index) {
        uint64_t subkey;
        handle_c_error(clingo_statistics_array_at(stats, key, index, &subkey));
        return getUserStatistics(stats, subkey);
    };
    void sq_ass_item(Py_ssize_t index, Reference value) {
        if (!value.valid()) { throw std::runtime_error("item deletion is not supported"); }
        uint64_t subkey;
        clingo_statistics_type_t type;
        handle_c_error(clingo_statistics_array_at(stats, key, index, &subkey));
        handle_c_error(clingo_statistics_type(stats, subkey, &type));
        setUserStatistics(stats, subkey, type, value, true);
    };
    Object append(Reference value) {
        uint64_t subkey;
        clingo_statistics_type_t type = getUserStatisticsType(value);
        handle_c_error(clingo_statistics_array_push(stats, key, type, &subkey));
        setUserStatistics(stats, subkey, type, value, false);
        return None();
    }
    Object extend(Reference value) {
        sq_inplace_concat(value);
        return None();
    }
    Object update(Reference value) {
        size_t size = sq_length(), i = 0;
        for (auto x : value.iter()) {
            if (i < size) { sq_ass_item(i, x); }
            else          { append(x); }
            ++i;
        }
        return None();
    }
    void sq_inplace_concat(Reference other) {
        for (auto x : other.iter()) { append(x); }
    };
    Object to_c() {
        return PyLong_FromVoidPtr(stats);
    }
};

PyMethodDef StatisticsArray::tp_methods[] = {
    // append
    {"append", to_function<&StatisticsArray::append>(), METH_O,
R"(append(self, value: Any) -> None

Append a value.

Parameters
----------
value: Any
    A nested structure composed of floats, sequences, and mappings.

Returns
-------
None
)"},
    // extend
    {"extend", to_function<&StatisticsArray::extend>(), METH_O,
R"(extend(self, values: Sequence[Any]) -> None

Extend the statistics array with the given values.

Paremeters
----------
values: Sequence[Any]
    A sequence of nested structures composed of floats, sequences, and
    mappings.

Returns
-------
None

See Also
-----
append
)"},
    // update
    {"update", to_function<&StatisticsArray::update>(), METH_O,
R"(update(self, values: Sequence[Any]) -> None

Update a statistics array.

Parameters
----------
values: Sequence[Any]
    A sequence of nested structures composed of floats, callable, sequences,
    and mappings. A callable can be used to update an existing value, it
    receives the previous numeric value (or None if absent) as argument and
    must return an updated numeric value.

Returns
-------
None
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef StatisticsArray::tp_getset[] = {
    {(char *)"_to_c", to_getter<&StatisticsArray::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_statistics_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

struct StatisticsMap : ObjectBase<StatisticsMap> {
    clingo_statistics_t *stats;
    uint64_t key;

    static PyGetSetDef tp_getset[];
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "StatisticsMap";
    static constexpr char const *tp_name = "clingo.StatisticsMap";
    static constexpr char const *tp_doc =
    R"(Object to capture statistics stored in a map.

This class implements `Mapping[str,Union[StatisticsArray,StatisticsMap,float]]`
but does not support item deletion.

See Also
--------
Control.solve

Notes
-----
The `StatisticsMap.update` function provides convenient means to initialize
and modify a statistics map.
)";

    static SharedObject<StatisticsMap> construct(clingo_statistics_t *stats, int64_t key) {
        auto self = new_();
        self->stats = stats;
        self->key = key;
        return self;
    }
    Py_ssize_t mp_length() {
        size_t size;
        handle_c_error(clingo_statistics_array_size(stats, key, &size));
        return size;
    }
    Object mp_subscript(Reference name) {
        uint64_t subkey;
        handle_c_error(clingo_statistics_map_at(stats, key, pyToCpp<std::string>(name).c_str(), &subkey));
        return getUserStatistics(stats, subkey);
    };
    void mp_ass_subscript(Reference pyName, Reference value) {
        if (!value.valid()) { throw std::runtime_error("item deletion is not supported"); }
        std::string name = pyToCpp<std::string>(pyName);
        uint64_t subkey;
        bool has_subkey;
        clingo_statistics_type_t type;
        handle_c_error(clingo_statistics_map_has_subkey(stats, key, name.c_str(), &has_subkey));
        if (has_subkey) {
            handle_c_error(clingo_statistics_map_at(stats, key, name.c_str(), &subkey));
            handle_c_error(clingo_statistics_type(stats, subkey, &type));
        }
        else {
            type = getUserStatisticsType(value);
            handle_c_error(clingo_statistics_map_add_subkey(stats, key, name.c_str(), type, &subkey));
        }
        setUserStatistics(stats, subkey, type, value, has_subkey);
    };
    bool sq_contains(Reference name) {
        bool ret;
        handle_c_error(clingo_statistics_map_has_subkey(stats, key, pyToCpp<std::string>(name).c_str(), &ret));
        return ret;
    }
    Object update(Reference value) {
        for (auto x : value.call("items").iter()) {
            auto pair = pyToCpp<std::pair<Object, Object>>(x);
            mp_ass_subscript(pair.first, pair.second);
        }
        return None();
    }
    Object tp_iter() {
        return keys().iter();
    }
    Object keys() {
        List ret;
        for (Py_ssize_t i = 0, e = mp_length(); i != e; ++i) {
            char const *name;
            clingo_statistics_map_subkey_name(stats, key, i, &name);
            ret.append(cppToPy(name));
        }
        return ret;
    }
    Object values() {
        List ret;
        for (Py_ssize_t i = 0, e = mp_length(); i != e; ++i) {
            char const *name;
            uint64_t subkey;
            clingo_statistics_map_subkey_name(stats, key, i, &name);
            clingo_statistics_map_at(stats, key, name, &subkey);
            ret.append(getUserStatistics(stats, subkey));
        }
        return ret;
    }
    Object items() {
        List ret;
        auto jt = begin(values().iter());
        for (auto key : keys().iter()) {
            ret.append(Tuple(key, *jt++));
        }
        return ret;
    }
    Object to_c() {
        return PyLong_FromVoidPtr(stats);
    }

};

PyMethodDef StatisticsMap::tp_methods[] = {
    // keys
    {"keys", to_function<&StatisticsMap::keys>(), METH_NOARGS,
R"(keys(self) -> List[str]

Return the keys of the map.

Returns
-------
List[str]
    The keys of the map.
)"},
    // values
    {"values", to_function<&StatisticsMap::values>(), METH_NOARGS,
R"(values(self) -> List[Union[StatisticsArray,StatisticsMap,float]]

Return the values of the map.

Returns
-------
List[Union[StatisticsArray,StatisticsMap,float]]
    The values of the map.
)"},
    // items
    {"items", to_function<&StatisticsMap::items>(), METH_NOARGS,
R"(items(self) -> List[Tuple[str, Union[StatisticsArray,StatisticsMap,float]]]

Return the items of the map.

Returns
-------
List[Tuple[str, Union[StatisticsArray,StatisticsMap,float]]]
    The items of the map.
)"},
    // update
    {"update", to_function<&StatisticsMap::update>(), METH_O,
R"(update(self, values: Mappping[str,Any]) -> None

Update the map with the given values.

Parameters
----------
values: Mapping[Any]
    A mapping of nested structures composed of floats, callable, sequences,
    and mappings. A callable can be used to update an existing value, it
    receives the previous numeric value (or None if absent) as argument and
    must return an updated numeric value.

Returns
-------
None
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef StatisticsMap::tp_getset[] = {
    {(char *)"_to_c", to_getter<&StatisticsMap::to_c>(), nullptr, (char *)R"(_to_c: int

An int representing the pointer to the underlying C `clingo_statistics_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

void setUserStatistics(clingo_statistics_t *stats, uint64_t key, clingo_statistics_type_t type, Reference value, bool update) {
    switch (type) {
        case clingo_statistics_type_value: {
            Object old = None();
            if (update && value.callable()) {
                double y;
                handle_c_error(clingo_statistics_value_get(stats, key, &y));
                old = cppToPy(y);
            }
            double x = pyToCpp<double>(value.callable() ? Reference{value(old)} : value);
            handle_c_error(clingo_statistics_value_set(stats, key, x));
            break;
        }
        case clingo_statistics_type_map: {
            StatisticsMap::construct(stats, key)->update(value);
            break;
        }
        case clingo_statistics_type_array: {
            StatisticsArray::construct(stats, key)->update(value);
            break;
        }
    }
}

Object getUserStatistics(clingo_statistics_t *stats, uint64_t key) {
    clingo_statistics_type_t type;
    handle_c_error(clingo_statistics_type(stats, key, &type));
    switch (type) {
        case clingo_statistics_type_array: {
            return StatisticsArray::construct(stats, key);
        }
        case clingo_statistics_type_map: {
            return StatisticsMap::construct(stats, key);
        }
        default: {
            assert(type == clingo_statistics_type_value);
            double value;
            handle_c_error(clingo_statistics_value_get(stats, key, &value));
            return cppToPy(value);
        }
    }
}

// {{{1 wrap Control

void pycall(Reference fun, clingo_symbol_t const *arguments, size_t arguments_size, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) {
    Object tuple = PyTuple_New(arguments_size);
    int i = 0;
    for (auto it = arguments, ie = it + arguments_size; it != ie; ++it, ++i) {
        PyTuple_SET_ITEM(tuple.toPy(), i, Symbol::construct(*it).release());
    }
    Object ret = PyObject_Call(fun.toPy(), tuple.toPy(), nullptr);
    auto add = [&](Reference sym) {
        symbol_wrapper val;
        pyToCpp(sym, val);
        handle_c_error(symbol_callback(&val.symbol, 1, symbol_callback_data));
    };
    if (pyIsSymbol(ret)) { add(ret); }
    else {
        for (auto &&x : ret.iter()) { add(x); }
    }
}

static void logger_callback(clingo_warning_t code, char const *message, void *data) {
    try {
        PyBlock block;
        Object pyMsg = cppToPy(message);
        Object pyCode = MessageCode::getAttr(code);
        Object ret = PyObject_CallFunctionObjArgs(static_cast<PyObject*>(data), pyCode.toPy(), pyMsg.toPy(), nullptr);
    }
    catch (...) {
        handle_cxx_error("<logger>", "error during message logging going to terminate");
        std::cerr << clingo_error_message() << std::endl;
        std::terminate();
    }
}

struct ControlWrap : ObjectBase<ControlWrap> {
    using Objects = std::vector<Object>;
    clingo_control_t *ctl;
    clingo_control_t *freeCtl;
    PyObject         *stats;
    PyObject         *logger;
    Objects           objects;
    bool              blocked;

    static PyGetSetDef tp_getset[];
    static PyMethodDef tp_methods[];

    static constexpr char const *tp_type = "Control";
    static constexpr char const *tp_name = "clingo.Control";
    static constexpr char const *tp_doc =
    R"(Control(arguments: List[str]=[], logger: Callback[[MessageCode,str],None]=None, message_limit: int=20) -> Control

Control object for the grounding/solving process.

Parameters
----------
arguments : List[str]
    Arguments to the grounder and solver.
logger : Callback[[MessageCode,str],None]=None
    Function to intercept messages normally printed to standard error.
message_limit : int
    The maximum number of messages passed to the logger.

Notes
-----
Note that only gringo options (without `--text`) and clasp's search options are
supported. Furthermore, a `Control` object is blocked while a search call is
active; you must not call any member function during search.
)";
    struct Block {
        Block(bool &blocked, char const *function) : blocked_(blocked) {
            if (blocked) {
                PyErr_Format(PyExc_RuntimeError, "Control.%s must not be called during solve call", function);
                throw PyException();
            }
        }
        ~Block() {
            blocked_ = false;
        }
        bool &blocked_;
    };
    #define CHECK_BLOCKED(function) auto block_ = Block(blocked, function);
    static Object construct(clingo_control_t *ctl) {
        auto self = new_();
        self->ctl = ctl;
        self->freeCtl = nullptr;
        self->stats   = nullptr;
        self->logger  = nullptr;
        self->blocked = false;
        new (&self->objects) Objects();
        return self;
    }
    static Object tp_new(PyTypeObject *type) {
        auto self = new_(type);
        self->ctl     = nullptr;
        self->freeCtl = nullptr;
        self->stats   = nullptr;
        self->logger  = nullptr;
        self->blocked = false;
        new (&self->objects) Objects();
        return self;
    }
    void tp_traverse(Traverse const &visit) {
        visit(stats);
        visit(logger);
        for (auto &object : objects) { visit(object); }
    }
    void tp_clear() {
        Py_CLEAR(stats);
        Py_CLEAR(logger);
        // NOTE: tp_dealloc might be called from an objects deconstructor while tp_clear is running
        Objects{}.swap(objects);
    }
    void tp_dealloc() {
        tp_clear();
        if (freeCtl) { clingo_control_free(freeCtl); }
        ctl = freeCtl = nullptr;
        objects.~Objects();
    }
    void tp_init(Reference pyargs, Reference pykwds) {
        static char const *kwlist[] = {"arguments", "logger", "message_limit", nullptr};
        Reference params  = Py_None, pyLogger = Py_None;
        int message_limit = 20;
        ParseTupleAndKeywords(pyargs, pykwds, "|OOi", kwlist, params, pyLogger, message_limit);
        std::forward_list<std::string> strs;
        std::vector<char const *> args;
        if (!params.is_none()) {
            for (Object pyVal : Reference{params}.iter()) {
                strs.emplace_front(pyToCpp<std::string>(pyVal));
                args.emplace_back(strs.front().c_str());
            }
        }
        if (!pyLogger.is_none()) {
            logger = pyLogger.release();
        }
        handle_c_error(clingo_control_new(args.data(), args.size(), logger ? logger_callback : nullptr, logger, message_limit, &freeCtl));
        ctl = freeCtl;
    }
    Object add(Reference args) {
        CHECK_BLOCKED("add");
        char  *name;
        Reference pyParams;
        char  *part;
        ParseTuple(args, "sOs", name, pyParams, part);
        std::forward_list<std::string> strs;
        std::vector<char const *> params;
        for (auto pyVal : Reference{pyParams}.iter()) {
            strs.emplace_front(pyToCpp<std::string>(pyVal));
            params.emplace_back(strs.front().c_str());
        }
        handle_c_error(clingo_control_add(ctl, name, params.data(), params.size(), part));
        Py_RETURN_NONE;
    }
    Object load(Reference args) {
        CHECK_BLOCKED("load");
        char *filename;
        ParseTuple(args, "s", filename);
        handle_c_error(clingo_control_load(ctl, filename));
        Py_RETURN_NONE;
    }
    static bool on_context(clingo_location_t const *location, char const *name, clingo_symbol_t const *arguments, size_t arguments_size, void *data, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data) {
        PyBlock block;
        try {
            Object fun = PyObject_GetAttrString(static_cast<PyObject*>(data), name);
            pycall(fun.toPy(), arguments, arguments_size, symbol_callback, symbol_callback_data);
            return true;
        }
        catch (...) {
            handle_cxx_error(*location, "error in context");
            return false;
        }
    }
    Object ground(Reference args, Reference kwds) {
        CHECK_BLOCKED("ground");
        static char const *kwlist[] = {"parts", "context", nullptr};
        Reference pyParts = Py_None;
        Reference pyContext = Py_None;
        ParseTupleAndKeywords(args, kwds, "O|O", kwlist, pyParts, pyContext);
        std::vector<std::pair<std::string, symbol_vector>> cpp_parts;
        std::vector<clingo_part_t> parts;
        pyToCpp(pyParts, cpp_parts);
        for (auto &&cpp_part : cpp_parts) {
            parts.emplace_back(clingo_part_t{cpp_part.first.c_str(), reinterpret_cast<clingo_symbol_t*>(cpp_part.second.data()), cpp_part.second.size()});
        }
        doUnblocked([&](){ handle_c_error(clingo_control_ground(ctl, parts.data(), parts.size(), pyContext.is_none() ? nullptr : on_context, pyContext.is_none() ? nullptr : pyContext.toPy())); });
        Py_RETURN_NONE;
    }
    Object getConst(Reference args) {
        CHECK_BLOCKED("get_const");
        char *name;
        ParseTuple(args, "s", name);
        bool has;
        handle_c_error(clingo_control_has_const(ctl, name, &has));
        if (!has) { Py_RETURN_NONE; }
        clingo_symbol_t val;
        handle_c_error(clingo_control_get_const(ctl, name, &val));
        return Symbol::construct(val);
    }
    Object solve(Reference args, Reference kwds) {
        CHECK_BLOCKED("solve");
        Py_XDECREF(stats);
        stats = nullptr;
        static char const *kwlist[] = {"assumptions", "on_model", "on_statistics", "on_finish", "yield_", "async", "async_", nullptr};
        Reference pyAss = Py_None, pyM = Py_None, pyS = Py_None, pyF = Py_None, pyYield = Py_False, pyAsync = Py_False, pyAsync_ = Py_False;
        ParseTupleAndKeywords(args, kwds, "|OOOOOOO", kwlist, pyAss, pyM, pyS, pyF, pyYield, pyAsync, pyAsync_);
        std::vector<clingo_literal_t> ass;
        if (!pyAss.is_none()) {
            clingo_symbolic_atoms_t const *atoms;
            handle_c_error(clingo_control_symbolic_atoms(ctl, &atoms));
            ass = pyToLits(pyAss, atoms, false, false);
        }
        bool async = pyAsync.isTrue() || pyAsync_.isTrue();
        clingo_solve_mode_bitset_t mode = 0;
        if (pyYield.isTrue()) { mode |= clingo_solve_mode_yield; }
        if (async) { mode |= clingo_solve_mode_async; }
        auto handle = SolveHandle::construct();
        auto notify = handle->notify(&SolveHandle::on_event, pyM, pyS, pyF);
        doUnblocked([&](){ handle_c_error(clingo_control_solve(ctl, mode, ass.data(), ass.size(), notify, handle.obj, &handle->handle)); });
        if (!pyYield.isTrue() && !async) { return handle->get(); }
        else { return handle; }
    }
    Object cleanup() {
        CHECK_BLOCKED("cleanup");
        handle_c_error(clingo_control_cleanup(ctl));
        Py_RETURN_NONE;
    }
    Object assign_external(Reference args) {
        CHECK_BLOCKED("assign_external");
        Reference pyExt, pyVal;
        ParseTuple(args, "OO", pyExt, pyVal);
        clingo_external_type_t val;
        if (pyVal == Py_True)       { val = clingo_external_type_true; }
        else if (pyVal == Py_False) { val = clingo_external_type_false; }
        else if (pyVal == Py_None)  { val = clingo_external_type_free; }
        else {
            PyErr_Format(PyExc_RuntimeError, "unexpected %s() object as second argumet", pyVal.toPy()->ob_type->tp_name);
            return nullptr;
        }
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_control_symbolic_atoms(ctl, &atoms));
        auto ext = pyToAtom(pyExt, atoms);
        handle_c_error(clingo_control_assign_external(ctl, ext, val));
        Py_RETURN_NONE;
    }
    Object release_external(Reference args) {
        CHECK_BLOCKED("release_external");
        Reference pyExt;
        ParseTuple(args, "O", pyExt);
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_control_symbolic_atoms(ctl, &atoms));
        auto ext = pyToAtom(pyExt, atoms);
        handle_c_error(clingo_control_assign_external(ctl, ext, clingo_external_type_release));
        Py_RETURN_NONE;
    }
    Object isConflicting() {
        return cppToPy(clingo_control_is_conflicting(ctl));
    }
    Object getStats() {
        CHECK_BLOCKED("statistics");
        if (!stats) {
            clingo_statistics_t const *s;
            handle_c_error(clingo_control_statistics(ctl, &s));
            uint64_t root;
            handle_c_error(clingo_statistics_root(s, &root));
            stats = getStatistics(s, root).release();
        }
        Py_XINCREF(stats);
        return stats;
    }
    void set_use_enumeration_assumption(Reference pyEnable) {
        CHECK_BLOCKED("use_enumeration_assumption");
        int enable = pyEnable.isTrue();
        handle_c_error(clingo_control_use_enumeration_assumption(ctl, enable));
    }
    Object conf() {
        CHECK_BLOCKED("configuration");
        clingo_configuration_t *conf;
        handle_c_error(clingo_control_configuration(ctl, &conf));
        clingo_id_t root;
        handle_c_error(clingo_configuration_root(conf, &root));
        return Configuration::construct(root, conf).release();
    }
    Object symbolicAtoms() {
        CHECK_BLOCKED("symbolic_atoms");
        clingo_symbolic_atoms_t const *atoms;
        handle_c_error(clingo_control_symbolic_atoms(ctl, &atoms));
        return SymbolicAtoms::construct(atoms);
    }
    Object theoryIter() {
        CHECK_BLOCKED("theory_atoms");
        clingo_theory_atoms_t const *atoms;
        handle_c_error(clingo_control_theory_atoms(ctl, &atoms));
        return TheoryAtomIter::construct(atoms, 0);
    }
    bool has_method(Reference tp, char const *name) {
        return PyObject_HasAttrString(tp.toPy(), name);
    }
    Object registerPropagator(Reference tp) {
        CHECK_BLOCKED("register_propagator");
        clingo_propagator_t propagator = {
            has_method(tp, "init")      ? propagator_init      : nullptr,
            has_method(tp, "propagate") ? propagator_propagate : nullptr,
            has_method(tp, "undo")      ? propagator_undo      : nullptr,
            has_method(tp, "check")     ? propagator_check     : nullptr,
            has_method(tp, "decide")    ? propagator_decide    : nullptr,
        };
        objects.emplace_back(tp);
        handle_c_error(clingo_control_register_propagator(ctl, &propagator, tp.toPy(), false));
        Py_RETURN_NONE;
    }
    Object registerObserver(Reference args, Reference kwds) {
        CHECK_BLOCKED("register_observer");
        static char const *kwlist[] = {"observer", "replace", nullptr};
        Reference obs, rep = Py_False;
        ParseTupleAndKeywords(args, kwds, "O|O", kwlist, obs, rep);
        static clingo_ground_program_observer_t observer = {
            observer_init_program,
            observer_begin_step,
            observer_end_step,
            observer_rule,
            observer_weight_rule,
            observer_minimize,
            observer_project,
            observer_output_atom,
            observer_output_term,
            observer_output_csp,
            observer_external,
            observer_assume,
            observer_heuristic,
            observer_acyc_edge,
            observer_theory_term_number,
            observer_theory_term_string,
            observer_theory_term_compound,
            observer_theory_element,
            observer_theory_atom,
            observer_theory_atom_with_guard
        };

        objects.emplace_back(obs);
        handle_c_error(clingo_control_register_observer(ctl, &observer, rep.isTrue(), obs.toPy()));
        return None();
    }
    Object interrupt() {
        clingo_control_interrupt(ctl);
        Py_RETURN_NONE;
    }
    Object backend() {
        clingo_backend_t *backend;
        handle_c_error(clingo_control_backend(ctl, &backend));
        return Backend::construct(backend);
    }
    Object builder() {
        clingo_program_builder_t *builder;
        handle_c_error(clingo_control_program_builder(ctl, &builder));
        return ProgramBuilder::construct(builder);
    }
    Object to_c() {
        return PyLong_FromVoidPtr(ctl);
    }
};

PyMethodDef ControlWrap::tp_methods[] = {
    // builder
    {"builder", to_function<&ControlWrap::builder>(), METH_NOARGS,
R"(builder(self) -> ProgramBuilder

Return a builder to construct a non-ground logic programs.

Returns
-------
ProgramBuilder

See Also
--------
ProgramBuilder
)"},
    // ground
    {"ground", to_function<&ControlWrap::ground>(), METH_KEYWORDS | METH_VARARGS,
R"(ground(self, parts: List[Tuple[str,List[Symbol]]], context: Any=None) -> None

Ground the given list of program parts specified by tuples of names and arguments.

Parameters
----------
parts : List[Tuple[str,List[Symbol]]]
    List of tuples of program names and program arguments to ground.
context : Any=None
    A context object whose methods are called during grounding using the
    `@`-syntax (if omitted methods, from the main module are used).

Notes
-----
Note that parts of a logic program without an explicit `#program` specification
are by default put into a program called `base` without arguments.

Examples
--------

    >>> import clingo
    >>> ctl = clingo.Control()
    >>> ctl.add("p", ["t"], "q(t).")
    >>> parts = []
    >>> parts.append(("p", [1]))
    >>> parts.append(("p", [2]))
    >>> ctl.ground(parts)
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: q(1) q(2)
    SAT
)"},
    // get_const
    {"get_const", to_function<&ControlWrap::getConst>(), METH_VARARGS,
R"(get_const(self, name: str) -> Optional[Symbol]

Return the symbol for a constant definition of form: `#const name = symbol.`

Parameters
----------
name : str
    The name of the constant to retrieve.

Returns
-------
Optional[Symbol]
    The function returns `None` if no matching constant definition exists.
)"},
    // add
    {"add", to_function<&ControlWrap::add>(), METH_VARARGS,
R"(add(self, name: str, parameters: List[str], program: str) -> None

Extend the logic program with the given non-ground logic program in string form.

Parameters
----------
name : str
    The name of program block to add.
parameters : List[str]
    The parameters of the program block to add.
program : str
    The non-ground program in string form.

Returns
-------
None

See Also
--------
Control.ground
)"},
    // load
    {"load", to_function<&ControlWrap::load>(), METH_VARARGS,
R"(load(self, path: str) -> None

Extend the logic program with a (non-ground) logic program in a file.

Parameters
----------
path : str
    The path of the file to load.

Returns
-------
None
)"},
    // solve
    {"solve", to_function<&ControlWrap::solve>(), METH_KEYWORDS | METH_VARARGS,
R"(solve(self, assumptions: List[Union[Tuple[Symbol,bool],int]]=[], on_model: Callback[[Model],Optional[bool]]=None, on_statistics : Callback[[StatisticsMap,StatisticsMap],None]=None, on_finish: Callback[[SolveResult],None]=None, yield_: bool=False, async_: bool=False) -> Union[SolveHandle,SolveResult]

Starts a search.

Parameters
----------
assumptions : List[Union[Tuple[Symbol,bool],int]]=[]
    List of (atom, boolean) tuples or program literals that serve
    as assumptions for the solve call, e.g., solving under
    assumptions `[(Function("a"), True)]` only admits answer sets
    that contain atom `a`.
on_model : Callback[[Model],Optional[bool]]=None
    Optional callback for intercepting models.
    A `Model` object is passed to the callback.
    The search can be interruped from the model callback by
    returning False.
on_statistics : Callback[[StatisticsMap,StatisticsMap],None]=None
    Optional callback to update statistics.
    The step and accumulated statistics are passed as arguments.
on_finish : Callback[[SolveResult],None]=None
    Optional callback called once search has finished.
    A `SolveResult` also indicating whether the solve call has been intrrupted
    is passed to the callback.
yield_ : bool=False
    The resulting `SolveHandle` is iterable yielding `Model` objects.
async_ : bool=False
    The solve call and the method `SolveHandle.resume` of the returned handle
    are non-blocking.

Returns
-------
Union[SolveHandle,SolveResult]
    The return value depends on the parameters. If either `yield_` or `async_`
    is true, then a handle is returned. Otherwise, a `SolveResult` is returned.

Notes
-----
If neither `yield_` nor `async_` is set, the function returns a SolveResult right
away.

Note that in gringo or in clingo with lparse or text output enabled this
function just grounds and returns a SolveResult where `SolveResult.unknown`
is true.

If this function is used in embedded Python code, you might want to start
clingo using the `--outf=3` option to disable all output from clingo.

Note that asynchronous solving is only available in clingo with thread support
enabled. Furthermore, the on_model and on_finish callbacks are called from
another thread. To ensure that the methods can be called, make sure to not use
any functions that block Python's GIL indefinitely.

This function as well as blocking functions on the `SolveHandle` release the GIL
but are not thread-safe.

Examples
--------

The following example shows how to intercept models with a callback:

    >>> import clingo
    >>> ctl = clingo.Control("0")
    >>> ctl.add("p", [], "1 { a; b } 1.")
    >>> ctl.ground([("p", [])])
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: a
    Answer: b
    SAT

The following example shows how to yield models:

    >>> import clingo
    >>> ctl = clingo.Control("0")
    >>> ctl.add("p", [], "1 { a; b } 1.")
    >>> ctl.ground([("p", [])])
    >>> with ctl.solve(yield_=True) as handle:
    ...     for m in handle: print("Answer: {}".format(m))
    ...     handle.get()
    ...
    Answer: a
    Answer: b
    SAT

The following example shows how to solve asynchronously:

    >>> import clingo
    >>> ctl = clingo.Control("0")
    >>> ctl.add("p", [], "1 { a; b } 1.")
    >>> ctl.ground([("p", [])])
    >>> with ctl.solve(on_model=lambda m: print("Answer: {}".format(m)), async_=True) as handle:
    ...     while not handle.wait(0): pass
    ...     handle.get()
    ...
    Answer: a
    Answer: b
    SAT
)"},
    // cleanup
    {"cleanup", to_function<&ControlWrap::cleanup>(), METH_NOARGS,
R"(cleanup(self) -> None

Cleanup the domain used for grounding by incorporating information from the
solver.

This function cleans up the domain used for grounding.  This is done by first
simplifying the current program representation (falsifying released external
atoms).  Afterwards, the top-level implications are used to either remove atoms
from the domain or mark them as facts.

Returns
-------
None

Notes
-----
Any atoms falsified are completely removed from the logic program. Hence, a
definition for such an atom in a successive step introduces a fresh atom.
)"},
    // assign_external
    {"assign_external", to_function<&ControlWrap::assign_external>(), METH_VARARGS,
R"(assign_external(self, external: Union[Symbol,int], truth: Optional[bool]) -> None

Assign a truth value to an external atom.

Parameters
----------
external : Union[Symbol,int]
    A symbol or program literal representing the external atom.
truth : Optional[bool]
    A Boolean fixes the external to the respective truth value; and None leaves
    its truth value open.

Returns
-------
None

See Also
--------
Control.release_external, SolveControl.symbolic_atoms, SymbolicAtom.is_external

Notes
-----
The truth value of an external atom can be changed before each solve call. An
atom is treated as external if it has been declared using an `#external`
directive, and has not been released by calling release_external() or defined
in a logic program with some rule. If the given atom is not external, then the
function has no effect.

For convenience, the truth assigned to atoms over negative program literals is
inverted.
)"},
    // release_external
    {"release_external", to_function<&ControlWrap::release_external>(), METH_VARARGS,
R"(release_external(self, symbol: Union[Symbol,int]) -> None

Release an external atom represented by the given symbol or program literal.

This function causes the corresponding atom to become permanently false if
there is no definition for the atom in the program. Otherwise, the function has
no effect.

Parameters
----------
symbol : Union[Symbol,int]
    The symbolic atom or program atom to release.

Returns
-------
None

Notes
-----
If the program literal is negative, the corresponding atom is released.

Examples
--------
The following example shows the effect of assigning and releasing and external
atom.

    >>> import clingo
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], "a. #external b.")
    >>> ctl.ground([("base", [])])
    >>> ctl.assign_external(clingo.Function("b"), True)
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: b a
    SAT
    >>> ctl.release_external(clingo.Function("b"))
    >>> ctl.solve(on_model=lambda m: print("Answer: {}".format(m)))
    Answer: a
    SAT
)"},
    {"register_observer", to_function<&ControlWrap::registerObserver>(), METH_VARARGS | METH_KEYWORDS,
R"(register_observer(self, observer: Observer, replace: bool=False) -> None

Registers the given observer to inspect the produced grounding.

Parameters
----------
observer : Observer
    The observer to register. See below for a description of the requirede
    interface.
replace : bool=False
    If set to true, the output is just passed to the observer and nolonger to
    the underlying solver (or any previously registered observers).

Returns
-------
None

Notes
-----
An observer should be a class of the form below. Not all functions have to be
implemented and can be omitted if not needed.

```python
class Observer:
    def init_program(self, incremental: bool) -> None:
        """
        Called once in the beginning.

        Parameters
        ----------
        incremental : bool
            Whether the program is incremental. If the incremental flag is
            true, there can be multiple calls to `Control.solve`.

        Returns
        -------
        None
        """

    def begin_step(self) -> None:
        """
        Marks the beginning of a block of directives passed to the solver.

        Returns
        -------
        None
        """

    def rule(self, choice: bool, head: List[int], body: List[int]) -> None:
        """
        Observe rules passed to the solver.

        Parameters
        ----------
        choice : bool
            Determines if the head is a choice or a disjunction.
        head : List[int]
            List of program atoms forming the rule head.
        body : List[int]
            List of program literals forming the rule body.

        Returns
        -------
        None
        """

    def weight_rule(self, choice: bool, head: List[int], lower_bound: int,
                    body: List[Tuple[int,int]]) -> None:
        """
        Observe rules with one weight constraint in the body passed to the
        solver.

        Parameters
        ----------
        choice : bool
            Determines if the head is a choice or a disjunction.
        head : List[int]
            List of program atoms forming the head of the rule.
        lower_bound:
            The lower bound of the weight constraint in the rule body.
        body : List[Tuple[int,int]]
            List of weighted literals (pairs of literal and weight) forming the
            elements of the weight constraint.

        Returns
        -------
        None
        """

    def minimize(self, priority: int, literals: List[Tuple[int,int]]) -> None:
        """
        Observe minimize directives (or weak constraints) passed to the
        solver.

        Parameters
        ----------
        priority : int
            The priority of the directive.
        literals : List[Tuple[int,int]]
            List of weighted literals whose sum to minimize (pairs of literal
            and weight).

        Returns
        -------
        None
        """

    def project(self, atoms: List[int]) -> None:
        """
        Observe projection directives passed to the solver.

        Parameters
        ----------
        atoms : List[int]
            The program atoms to project on.

        Returns
        -------
        None
        """

    def output_atom(self, symbol: Symbol, atom: int) -> None:
        """
        Observe shown atoms passed to the solver.  Facts do not have an
        associated program atom. The value of the atom is set to zero.

        Parameters
        ----------
        symbol : Symbolic
            The symbolic representation of the atom.
        atom : int
            The associated program atom (0 for facts).

        Returns
        -------
        None
        """

    def output_term(self, symbol: Symbol, condition: List[int]) -> None:
        """
        Observe shown terms passed to the solver.

        Parameters
        ----------
        symbol : Symbol
            The symbolic representation of the term.
        condition : List[int]
            List of program literals forming the condition when to show the
            term.

        Returns
        -------
        None
        """

    def output_csp(self, symbol: Symbol, value: int,
                   condition: List[int]) -> None:
        """
        Observe shown csp variables passed to the solver.

        Parameters
        ----------
        symbol : Symbol
            The symbolic representation of the variable.
        value : int
            The integer value of the variable.
        condition : List[int]
            List of program literals forming the condition when to show the
            variable with its value.

        Returns
        -------
        None
        """

    def external(self, atom: int, value: TruthValue) -> None:
        """
        Observe external statements passed to the solver.

        Parameters
        ----------
        atom : int
            The external atom in form of a program literal.
        value : TruthValue
            The truth value of the external statement.

        Returns
        -------
        None
        """

    def assume(self, literals: List[int]) -> None:
        """
        Observe assumption directives passed to the solver.

        Parameters
        ----------
        literals : List[int]
            The program literals to assume (positive literals are true and
            negative literals false for the next solve call).

        Returns
        -------
        None
        """

    def heuristic(self, atom: int, type: HeuristicType, bias: int,
                  priority: int, condition: List[int]) -> None:
        """
        Observe heuristic directives passed to the solver.

        Parameters
        ----------
        atom : int
            The program atom heuristically modified.
        type : HeuristicType
            The type of the modification.
        bias : int
            A signed integer.
        priority : int
            An unsigned integer.
        condition : List[int]
            List of program literals.

        Returns
        -------
        None
        """

    def acyc_edge(self, node_u: int, node_v: int,
                  condition: List[int]) -> None:
        """
        Observe edge directives passed to the solver.

        Parameters
        ----------
        node_u : int
            The start vertex of the edge (in form of an integer).
        node_v : int
            Тhe end vertex of the edge (in form of an integer).
        condition : List[int]
            The list of program literals forming th condition under which to
            add the edge.

        Returns
        -------
        None
        """

    def theory_term_number(self, term_id: int, number: int) -> None:
        """
        Observe numeric theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        number : int
            The value of the term.

        Returns
        -------
        None
        """

    def theory_term_string(self, term_id : int, name : str) -> None:
        """
        Observe string theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        name : str
            The string value of the term.

        Returns
        -------
        None
        """

    def theory_term_compound(self, term_id: int, name_id_or_type: int,
                             arguments: List[int]) -> None:
        """
        Observe compound theory terms.

        Parameters
        ----------
        term_id : int
            The id of the term.
        name_id_or_type : int
            The name or type of the term where
            - if it is -1, then it is a tuple
            - if it is -2, then it is a set
            - if it is -3, then it is a list
            - otherwise, it is a function and name_id_or_type refers to the id
            of the name (in form of a string term)
        arguments : List[int]
            The arguments of the term in form of a list of term ids.

        Returns
        -------
        None
        """

    def theory_element(self, element_id: int, terms: List[int],
                       condition: List[int]) -> None:
        """
        Observe theory elements.

        Parameters
        ----------
        element_id : int
            The id of the element.
        terms : List[int]
            The term tuple of the element in form of a list of term ids.
        condition : List[int]
            The list of program literals forming the condition.

        Returns
        -------
        None
        """

    def theory_atom(self, atom_id_or_zero: int, term_id: int,
                    elements: List[int]) -> None:
        """
        Observe theory atoms without guard.

        Parameters
        ----------
        atom_id_or_zero : int
            The id of the atom or zero for directives.
        term_id : int
            The term associated with the atom.
        elements : List[int]
            The elements of the atom in form of a list of element ids.

        Returns
        -------
        None
        """

    def theory_atom_with_guard(self, atom_id_or_zero: int, term_id: int,
                               elements: List[int], operator_id: int,
                               right_hand_side_id: int) -> None:
        """
        Observe theory atoms with guard.

        Parameters
        ----------
        atom_id_or_zero : int
            The id of the atom or zero for directives.
        term_id : int
            The term associated with the atom.
        elements : List[int]
            The elements of the atom in form of a list of element ids.
        operator_id : int
            The id of the operator (a string term).
        right_hand_side_id : int
            The id of the term on the right hand side of the atom.

        Returns
        -------
        None
        """

    def end_step(self) -> None:
        """
        Marks the end of a block of directives passed to the solver.

        This function is called right before solving starts.

        Returns
        -------
        None
        """
```
)"},
    {"register_propagator", to_function<&ControlWrap::registerPropagator>(), METH_O,
R"(register_propagator(self, propagator: Propagator) -> None

Registers the given propagator with all solvers.

Parameters
----------
propagator : Propagator
    The propagator to register.

Returns
-------
None

Notes
-----
Each symbolic or theory atom is uniquely associated with a positive program
atom in form of a positive integer. Program literals additionally have a sign
to represent default negation. Furthermore, there are non-zero integer solver
literals. There is a surjective mapping from program atoms to solver literals.

All methods called during propagation use solver literals whereas
`SymbolicAtom.literal` and `TheoryAtom.literal` return program literals. The
function `PropagateInit.solver_literal` can be used to map program literals or
condition ids to solver literals.

A propagator should be a class of the form below. Not all functions have to be
implemented and can be omitted if not needed.

```python
class Propagator:
    def init(self, init: PropagateInit) -> None:
        """
        This function is called once before each solving step.

        It is used to map relevant program literals to solver literals, add
        watches for solver literals, and initialize the data structures used
        during propagation.

        Parameters
        ----------
        init : PropagateInit
            Object to initialize the propagator.

        Returns
        -------
        None

        Notes
        -----
        This is the last point to access theory atoms.  Once the search has
        started, they are no longer accessible.
        """

    def propagate(self, control: PropagateControl, changes: List[int]) -> None:
        """
        Can be used to propagate solver literals given a partial assignment.

        Parameters
        ----------
        control : PropagateControl
            Object to control propagation.
        changes : List[int]
            List of watched solver literals assigned to true.

        Returns
        -------
        None

        Notes
        -----
        Called during propagation with a non-empty list of watched solver
        literals that have been assigned to true since the last call to either
        propagate, undo, (or the start of the search) - the change set. Only
        watched solver literals are contained in the change set. Each literal
        in the change set is true w.r.t. the current Assignment.
        `PropagateControl.add_clause` can be used to add clauses. If a clause
        is unit resulting, it can be propagated using
        `PropagateControl.propagate`. If either of the two methods returns
        False, the propagate function must return immediately.

            c = ...
            if not control.add_clause(c) or not control.propagate(c):
                return

        Note that this function can be called from different solving threads.
        Each thread has its own assignment and id, which can be obtained using
        `PropagateControl.id`.
        """

    def undo(self, thread_id: int, assignment: Assignment,
             changes: List[int]) -> None:
        """
        Called whenever a solver with the given id undos assignments to watched
        solver literals.

        Parameters
        ----------
        thread_id : int
            The solver thread id.
        assignment : Assignment
            Object for inspecting the partial assignment of the solver.
        changes : List[int]
            The list of watched solver literals whose assignment is undone.

        Returns
        -------
        None

        Notes
        -----
        This function is meant to update assignment dependent state in a
        propagator but not to modify the current state of the solver.
        """

    def check(self, control: PropagateControl) -> None:
        """
        This function is similar to propagate but is called without a change
        set on propagation fixpoints.

        When exactly this function is called, can be configured using the @ref
        PropagateInit.check_mode property.

        Parameters
        ----------
        control : PropagateControl
            Object to control propagation.

        Returns
        -------
        None

        Notes
        -----
        This function is called even if no watches have been added.
        """

    def decide(self, thread_id: int, assignment: Assignment, fallback: int) -> int:
        """
        This function allows a propagator to implement domain-specific
        heuristics.

        It is called whenever propagation reaches a fixed point.

        Parameters
        ----------
        thread_id : int
            The solver thread id.
        assignment : Assignment
            Object for inspecting the partial assignment of the solver.
        fallback : int
            The literal choosen by the solver's heuristic.

        Returns
        -------
        int
            Тhe next solver literal to make true.

        Notes
        -----
        This function should return a free solver literal that is to be
        assigned true. In case multiple propagators are registered, this
        function can return 0 to let a propagator registered later make a
        decision. If all propagators return 0, then the fallback literal is
        used.
        """
```
        )"},
    {"interrupt", to_function<&ControlWrap::interrupt>(), METH_NOARGS,
R"(interrupt(self) -> None

Interrupt the active solve call.

Returns
-------
None

Notes
-----
This function is thread-safe and can be called from a signal handler. If no
search is active, the subsequent call to `Control.solve` is interrupted. The
result of the `Control.solve` method can be used to query if the search was
interrupted.
)"},
    {"backend", to_function<&ControlWrap::backend>(), METH_NOARGS,
R"(backend(self) -> Backend

Returns a `Backend` object providing a low level interface to extend a logic
program.

Returns
-------
Backend
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef ControlWrap::tp_getset[] = {
    {(char*)"configuration", to_getter<&ControlWrap::conf>(), nullptr, (char*)R"(configuration: Configuration
`Configuration` object to change the configuration.
)", nullptr},
    {(char*)"symbolic_atoms", to_getter<&ControlWrap::symbolicAtoms>(), nullptr, (char*)R"(symbolic_atoms: SymbolicAtoms

`SymbolicAtoms` object to inspect the symbolic atoms.
)", nullptr},
    {(char*)"use_enumeration_assumption", nullptr, to_setter<&ControlWrap::set_use_enumeration_assumption>(),
(char*)R"(set_use_enumeration_assumption: bool

Whether do discard or keep learnt information from enumeration modes.

If the enumeration assumption is enabled, then all information learnt from
clasp's various enumeration modes is removed after a solve call. This includes
enumeration of cautious or brave consequences, enumeration of answer sets with
or without projection, or finding optimal models; as well as clauses added with
`SolveControl.add_clause`.

Notes
-----
Initially the enumeration assumption is enabled.

In general, the enumeration assumption should be enabled whenever there are
multiple calls to solve. Otherwise, the behavior of the solver will be
unpredictable because there are no guarantees which information exactly is
kept. There might be small speed benefits when disabling the enumeration
assumption for single shot solving.
)", nullptr},
    {(char*)"is_conflicting", to_getter<&ControlWrap::isConflicting>(), nullptr, (char*)R"(is_conflicting: bool

Whether the internal program representation is conflicting.

If this (read-only) property is true, solve calls return immediately with an
unsatisfiable solve result.

Notes
-----
Conflicts first have to be detected, e.g., initial unit propagation results in
an empty clause, or later if an empty clause is resolved during solving. Hence,
the property might be false even if the problem is unsatisfiable.
)", nullptr},
    {(char*)"statistics", to_getter<&ControlWrap::getStats>(), nullptr,
(char*)R"(statistics: dict

A `dict` containing solve statistics of the last solve call.

Notes
-----
The statistics correspond to the `--stats` output of clingo. The detail of the
statistics depends on what level is requested on the command line.

This property is only available in clingo.

Examples
--------
The following example shows how to dump the solving statistics in json format:

    >>> import json
    >>> import clingo
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], "{a}.")
    >>> ctl.ground([("base", [])])
    >>> ctl.solve()
    SAT
    >>> print(json.dumps(ctl.statistics['solving'], sort_keys=True, indent=4,
    ... separators=(',', ': ')))
    {
        "solvers": {
            "choices": 1.0,
            "conflicts": 0.0,
            "conflicts_analyzed": 0.0,
            "restarts": 0.0,
            "restarts_last": 0.0
        }
    }
)", nullptr},
    {(char *)"theory_atoms", to_getter<&ControlWrap::theoryIter>(), nullptr, (char *)R"(theory_atoms: TheoryAtomIter

A `TheoryAtomIter` object, which can be used to iterate over the theory atoms.
)", nullptr},
    {(char *)"_to_c", to_getter<&ControlWrap::to_c>(), nullptr, (char *)R"(_to_c: int

An `int` representing the pointer to the underlying C `clingo_control_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap ApplicationOptions

struct Flag : ObjectBase<Flag> {
    bool flag;
    static constexpr char const *tp_type = "Flag";
    static constexpr char const *tp_name = "clingo.Flag";
    static constexpr char const *tp_doc = R"(Flag(value: bool=False) -> Flag

Helper object to parse command-line flags.

Parameters
----------
value : bool=False
    The initial value of the flag.
)";
    static PyGetSetDef tp_getset[];
    static Object tp_new(PyTypeObject *type) {
        auto self = new_(type);
        self->flag = false;
        return self;
    }
    void tp_init(Reference args, Reference kwargs) {
        Reference pyValue = Py_False;
        static char const *kwlist[] = {"value", nullptr};
        ParseTupleAndKeywords(args, kwargs, "|O", kwlist, pyValue);
        flag = pyValue.isTrue();
    }
    Object get_value() {
        return cppToPy(flag);
    }
    void set_value(Reference value) {
        flag = value.isTrue();
    }
};

PyGetSetDef Flag::tp_getset[] = {
    {(char*)"value", to_getter<&Flag::get_value>(), to_setter<&Flag::set_value>(), (char*)R"(flag: bool

The value of the flag.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

struct ApplicationOptions : ObjectBase<ApplicationOptions> {
    clingo_options_t *options;
    std::vector<Object> *refs;
    static constexpr char const *tp_type = "ApplicationOptions";
    static constexpr char const *tp_name = "clingo.ApplicationOptions";
    static constexpr char const *tp_doc = R"(Object to add custom options to a clingo based application.)";
    static PyMethodDef tp_methods[];
    static PyGetSetDef tp_getset[];

    static Object construct(clingo_options_t *options, std::vector<Object> &refs) {
        auto self = new_();
        self->options = options;
        self->refs = &refs;
        return self;
    }

    static bool parse_option(char const *value, void *data) {
        try {
            Reference pyParse = static_cast<PyObject*>(data);
            Object params = PyTuple_New(1);
            if (PyTuple_SetItem(params.toPy(), 0, cppToPy(value).release()) < 0) { throw PyException(); }
            if (!Object{PyObject_Call(pyParse.toPy(), params.toPy(), nullptr)}.isTrue()) { throw std::runtime_error("parsing option failed"); }
            return true;
        }
        catch (...) {
            handle_cxx_error("<application>", "error during parse option");
            return false;
        }
    }

    Object add(Reference args, Reference kwds) {
        static char const *kwlist[] = {"group", "option", "description", "parser", "multi", "argument", nullptr};
        char const *group = nullptr, *option = nullptr, *description = nullptr, *argument = nullptr;
        Reference pyParser, pyMulti = Py_False;

        ParseTupleAndKeywords(args, kwds, "sssO|Os", kwlist, group, option, description, pyParser, pyMulti, argument);
        refs->emplace_back(pyParser);

        clingo_options_add(options, group, option, description, &parse_option, pyParser.toPy(), pyMulti.isTrue(), argument);
        return None();

    }
    Object add_flag(Reference args, Reference kwds) {
        static char const *kwlist[] = {"group", "option", "description", "flag", nullptr};
        char const *group = nullptr, *option = nullptr, *description = nullptr;
        Reference pyFlag;

        ParseTupleAndKeywords(args, kwds, "sssO!", kwlist, group, option, description, Flag::type, pyFlag);
        refs->emplace_back(pyFlag);

        handle_c_error(clingo_options_add_flag(options, group, option, description, &reinterpret_cast<Flag*>(pyFlag.toPy())->flag));
        return None();
    }

    Object to_c() {
        return PyLong_FromVoidPtr(options);
    }
};

PyMethodDef ApplicationOptions::tp_methods[] = {
    {"add", to_function<&ApplicationOptions::add>(), METH_VARARGS | METH_KEYWORDS, R"(add_flag(self, group: str, option: str, description: str, parser: Callback[[str], bool], multi: bool=False, argument: str=None) -> None

Add an option that is processed with a custom parser.

Parameters
----------
group : str
    Options are grouped into sections as given by this string.
option : str
    Parameter option specifies the name(s) of the option. For example,
    `"ping,p"` adds the short option `-p` and its long form `--ping`. It is
    also possible to associate an option with a help level by adding `"@l"` to
    the option specification. Options with a level greater than zero are only
    shown if the argument to help is greater or equal to `l`.
description : str
    The description of the option shown in the help output.
parser : Callback[[str], bool]
    An option parser is a function that takes a string as input and returns
    true or false depending on whether the option was parsed successively.
multi : bool=False
    Whether the option can appear multiple times on the command-line.
argument : str=None
    Otional string to change the value name in the generated help.

Returns
-------
None

Raises
------
RuntimeError
    An error is raised if an option with the same name already exists.

Notes
-----
The parser also has to take care of storing the semantic value of the option
somewhere.
)"},
    {"add_flag", to_function<&ApplicationOptions::add_flag>(), METH_VARARGS | METH_KEYWORDS, R"(add_flag(self, group: str, option: str, description: str, target: Flag) -> None

Add an option that is a simple flag.

This function is similar to `ApplicationOptions.add` but simpler because
it only supports flags, which do not have values. Note that the target
parameter must be of type Flag, which is set to true if the flag is passed on
the command line.

Parameters
----------
group : str
    Options are grouped into sections as given by this string.
option : str
    Same as for `ApplicationOptions.add`.
description : str
    The description of the option shown in the help output.
target : Flag
    The object that receives the value.

Returns
-------
None
)"},
    {nullptr, nullptr, 0, nullptr}
};

PyGetSetDef ApplicationOptions::tp_getset[] = {
    {(char *)"_to_c", to_getter<&ApplicationOptions::to_c>(), nullptr, (char *)R"(_to_c: int
An int representing the pointer to the underlying C `clingo_options_t` struct.
)", nullptr},
    {nullptr, nullptr, nullptr, nullptr, nullptr}
};

// {{{1 wrap Application

using AppData = std::pair<Reference&, std::vector<Object>>;
char const *g_app_program_name(void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        Object name = pyApp.first.getAttr("program_name");
        char const *s;
        handle_c_error(clingo_add_string(pyToCpp<std::string>(name).c_str(), &s));
        return s;
    }
    catch (...) {
        handle_cxx_error("<application>", "error when getting program name");
        std::cerr << clingo_error_message() << std::endl;
        std::terminate();
    }
}

char const *g_app_version(void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        Object name = pyApp.first.getAttr("version");
        char const *s;
        handle_c_error(clingo_add_string(pyToCpp<std::string>(name).c_str(), &s));
        return s;
    }
    catch (...) {
        handle_cxx_error("<application>", "error when getting version");
        std::cerr << clingo_error_message() << std::endl;
        std::terminate();
    }
}

unsigned g_app_message_limit(void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        Object limit = pyApp.first.getAttr("message_limit");
        return pyToCpp<unsigned>(limit);
    }
    catch (...) {
        handle_cxx_error("<application>", "error when getting message limit");
        std::cerr << clingo_error_message() << std::endl;
        std::terminate();
    }
}

bool g_app_main(clingo_control_t *control, char const *const * files, size_t size, void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        pyApp.first.call("main", ControlWrap::construct(control), cppToPy(files, size));
        return true;
    }
    catch (...) {
        handle_cxx_error("<application>", "error during main");
        return false;
    }
}

void g_app_logger(clingo_warning_t code, char const *message, void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        pyApp.first.call("logger", MessageCode::getAttr(code), cppToPy(message));
    }
    catch (...) {
        handle_cxx_error("<application>", "error when calling message logger");
        std::cerr << clingo_error_message() << std::endl;
        std::terminate();
    }
}

bool g_app_register_options(clingo_options_t *options, void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        pyApp.first.call("register_options", ApplicationOptions::construct(options, pyApp.second));
        return true;
    }
    catch (...) {
        handle_cxx_error("<application>", "error during register options");
        return false;
    }
}

PyObject* call_printer(PyObject *data) {
    PY_TRY {
        auto d = static_cast<std::pair<clingo_default_model_printer_t, void*>*>(PyCapsule_GetPointer(data, nullptr));
        if (!d) { return nullptr; }
        handle_c_error(d->first(d->second));
        Py_RETURN_NONE;
    }
    PY_CATCH(nullptr);
}

PyMethodDef call_printer_def = {
    "clingo.default_model_printer",
    reinterpret_cast<PyCFunction>(call_printer),
    METH_NOARGS,
    nullptr
};

bool g_app_model_printer(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data, void *data) {
    PyBlock block;
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        std::pair<clingo_default_model_printer_t, void*> pd{printer, printer_data};
        Object ptr = PyCapsule_New(&pd, nullptr, nullptr);
        Object pyP = PyCFunction_New(&call_printer_def, ptr.toPy());
        pyApp.first.call("print_model", Model::construct(model), pyP);
        return true;
    }
    catch (...) {
        handle_cxx_error("<application>", "error during model printing");
        return false;
    }
}

bool g_app_validate_options(void *data) {
    try {
        AppData &pyApp = *static_cast<AppData*>(data);
        pyApp.first.call("validate_options");
        return true;
    }
    catch (...) {
        handle_cxx_error("<application>", "error when validating options");
        std::cerr << clingo_error_message() << std::endl;
        // NOTE: to avoid getting stack traces for such kind of errors
        //       could be avoided with a special kind of error that is
        //       reported differently than a runtime error
        std::terminate();
    }
}

// {{{1 wrap module functions

Object parseTerm(Reference args, Reference kwds) {
    static char const *kwlist[] = {"string", "logger", "message_limit", nullptr};
    char const *str;
    Reference logger = Py_None;
    int message_limit = 20;
    ParseTupleAndKeywords(args, kwds, "s|Oi", kwlist, str, logger, message_limit);
    clingo_symbol_t sym;
    handle_c_error(clingo_parse_term(str, !logger.is_none() ? logger_callback : nullptr, logger.toPy(), message_limit, &sym));
    return Symbol::construct(sym);
}

Object clingoMain(Reference args, Reference kwds) {
    Reference pyApp;
    Reference pyArgs;
    static char const *kwlist[] = {"application", "arguments", nullptr};
    ParseTupleAndKeywords(args, kwds, "O|O", kwlist, pyApp, pyArgs);
    std::vector<std::string> sArgs;
    std::vector<char const*> cArgs;
    if (pyArgs.valid()) { pyToCpp(pyArgs, sArgs); }
    for (auto &s : sArgs) { cArgs.emplace_back(s.c_str()); }
    clingo_application_t app {
        pyApp.hasAttr("program_name") ? g_app_program_name : nullptr,
        pyApp.hasAttr("version") ? g_app_version : nullptr,
        pyApp.hasAttr("message_limit") ? g_app_message_limit : nullptr,
        g_app_main,
        pyApp.hasAttr("logger") ? g_app_logger : nullptr,
        pyApp.hasAttr("print_model") ? g_app_model_printer : nullptr,
        pyApp.hasAttr("register_options") ? g_app_register_options : nullptr,
        pyApp.hasAttr("validate_options") ? g_app_validate_options : nullptr,
    };
    AppData data{pyApp, {}};
    return PyLong_FromLong(clingo_main(&app, cArgs.data(), cArgs.size(), &data));
}

Object clingoErrorMessage() {
    char const *msg = clingo_error_message();
    return msg ? cppToPy(msg) : None();
}

Object clingoErrorCode() {
    return cppToPy(static_cast<int>(clingo_error_code()));
}

// {{{1 gringo module

static PyMethodDef clingoASTModuleMethods[] = {
    {"Id", to_function<createId>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Variable", to_function<createVariable>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Symbol", to_function<createSymbol>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UnaryOperation", to_function<createUnaryOperation>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"BinaryOperation", to_function<createBinaryOperation>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Interval", to_function<createInterval>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Function", to_function<createFunction>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Pool", to_function<createPool>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"CSPProduct", to_function<createCSPProduct>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"CSPSum", to_function<createCSPSum>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"CSPGuard", to_function<createCSPGuard>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"BooleanConstant", to_function<createBooleanConstant>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SymbolicAtom", to_function<createSymbolicAtom>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Comparison", to_function<createComparison>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"CSPLiteral", to_function<createCSPLiteral>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"AggregateGuard", to_function<createAggregateGuard>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ConditionalLiteral", to_function<createConditionalLiteral>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Aggregate", to_function<createAggregate>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"BodyAggregateElement", to_function<createBodyAggregateElement>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"BodyAggregate", to_function<createBodyAggregate>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"HeadAggregateElement", to_function<createHeadAggregateElement>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"HeadAggregate", to_function<createHeadAggregate>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Disjunction", to_function<createDisjunction>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"DisjointElement", to_function<createDisjointElement>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Disjoint", to_function<createDisjoint>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryFunction", to_function<createTheoryFunction>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheorySequence", to_function<createTheorySequence>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryUnparsedTermElement", to_function<createTheoryUnparsedTermElement>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryUnparsedTerm", to_function<createTheoryUnparsedTerm>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryGuard", to_function<createTheoryGuard>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryAtomElement", to_function<createTheoryAtomElement>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryAtom", to_function<createTheoryAtom>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Literal", to_function<createLiteral>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryOperatorDefinition", to_function<createTheoryOperatorDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryTermDefinition", to_function<createTheoryTermDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryGuardDefinition", to_function<createTheoryGuardDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryAtomDefinition", to_function<createTheoryAtomDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"TheoryDefinition", to_function<createTheoryDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Rule", to_function<createRule>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Definition", to_function<createDefinition>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ShowSignature", to_function<createShowSignature>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Defined", to_function<createDefined>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ShowTerm", to_function<createShowTerm>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Minimize", to_function<createMinimize>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Script", to_function<createScript>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Program", to_function<createProgram>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"External", to_function<createExternal>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Edge", to_function<createEdge>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Heuristic", to_function<createHeuristic>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ProjectAtom", to_function<createProjectAtom>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ProjectSignature", to_function<createProjectSignature>(), METH_VARARGS | METH_KEYWORDS, nullptr},
    {nullptr, nullptr, 0, nullptr}
};
static char const *clingoASTModuleDoc = "The clingo.ast-" CLINGO_VERSION " module." R"(

The grammar below defines valid ASTs. For each upper case identifier there is a
matching function in the module. Arguments follow in parenthesis: each having a
type given on the right-hand side of the colon. The symbols `?`, `*`, and `+`
are used to denote optional arguments (`None` encodes abscence), list
arguments, and non-empty list arguments.

```
# Terms

term = Symbol
        ( location : Location
        , symbol   : clingo.Symbol
        )
     | Variable
        ( location : Location
        , name     : str
        )
     | UnaryOperation
        ( location : Location
        , operator : UnaryOperator
        , argument : term
        )
     | BinaryOperation
        ( location : Location
        , operator : BinaryOperator
        , left     : term
        , right    : term
        )
     | Interval
        ( location : Location
        , left     : term
        , right    : term
        )
     | Function
        ( location  : Location
        , name      : str
        , arguments : term*
        , external  : bool
        )
     | Pool
        ( location  : Location
        , arguments : term*
        )

csp_term = CSPSum
            ( location : Location
            , terms    : CSPProduct
                          ( location    : Location
                          , coefficient : term
                          , variable    : term?
                          )*
            )

theory_term = Symbol
               ( location : Location
               , symbol   : clingo.Symbol
               )
            | Variable
               ( location : Location
               , name     : str
               )
            | TheoryTermSequence
               ( location : Location
               , sequence_type : TheorySequenceType
               , terms         : theory_term*
               )
            | TheoryFunction
               ( location  : Location
               , name      : str
               , arguments : theory_term*
               )
            | TheoryUnparsedTerm
               ( location : Location
               , elements : TheoryUnparsedTermElement
                             ( operators : str*
                             , term      : theory_term
                             )+
               )

# Literals

symbolic_atom = SymbolicAtom
                 ( term : term
                 )

literal = Literal
           ( location : Location
           , sign     : Sign
           , atom     : Comparison
                         ( comparison : ComparisonOperator
                         , left       : term
                         , right      : term
                         )
                      | BooleanConstant
                         ( value : bool
                         )
                      | symbolic_atom
           )

        | CSPLiteral
           ( location : Location
           , term     : csp_term
           , guards   : CSPGuard
                         ( comparison : ComparisonOperator
                         , term       : csp_term
                         )+
           )

# Head and Body Literals

aggregate_guard = AggregateGuard
                   ( comparison : ComparisonOperator
                   , term       : term
                   )

conditional_literal = ConditionalLiteral
                       ( location  : Location
                       , literal   : Literal
                       , condition : Literal*
                       )

aggregate = Aggregate
             ( location    : Location
             , left_guard  : aggregate_guard?
             , elements    : conditional_literal*
             , right_guard : aggregate_guard?
             )

theory_atom = TheoryAtom
               ( location : Location
               , term     : term
               , elements : TheoryAtomElement
                             ( tuple     : theory_term*
                             , condition : literal*
                             )*
               , guard    : TheoryGuard
                             ( operator_name : str
                             , term          : theory_term
                             )?
               )

body_atom = aggregate
          | BodyAggregate
             ( location    : Location
             , left_guard  : aggregate_guard?
             , function    : AggregateFunction
             , elements    : BodyAggregateElement
                              ( tuple     : term*
                              , condition : literal*
                              )*
             , right_guard : aggregate_guard?
             )
          | Disjoint
             ( location : Location
             , elements : DisjointElement
                           ( location  : Location
                           , tuple     : term*
                           , term      : csp_term
                           , condition : literal*
                           )*
             )
          | theory_atom

body_literal = literal
             | conditional_literal
             | Literal
                ( location : Location
                , sign     : Sign
                , atom     : body_atom
                )

head = literal
     | aggregate
     | HeadAggregate
        ( location    : Location
        , left_guard  : aggregate_guard?
        , function    : AggregateFunction
        , elements    : HeadAggregateElement
                         ( tuple     : term*
                         , condition : conditional_literal
                         )*
        , right_guard : aggregate_guard?
        )
     | Disjunction
        ( location : Location
        , elements : conditional_literal*
        )
     | theory_atom

# Theory Definitions

theory = TheoryDefinition
          ( location : Location
          , name     : str
          , terms    : TheoryTermDefinition
                        ( location  : Location
                        , name      : str
                        , operators : TheoryOperatorDefinition
                                       ( location      : Location
                                       , name          : str
                                       , priority      : int
                                       , operator_type : TheoryOperatorType
                                       )*
                        )
          , atoms    : TheoryAtomDefinition
                        ( location  : Location
                        , atom_type : TheoryAtomType
                        , name      : str
                        , arity     : int
                        , elements  : str*
                        , guard     : TheoryGuardDefinition
                                       ( operators : str*
                                       , term      : str
                                       )?
                        )
          )

# Statements

statement = Rule
             ( location : Location
             , head     : head
             , body     : body_literal*
             )
          | Definition
             ( location   : Location
             , name       : str
             , value      : term
             , is_default : bool
             )
          | ShowSignature
             ( location   : Location
             , name       : str
             , arity      : int
             , sign       : bool
             , csp        : bool
             )
          | Defined
             ( location   : Location
             , name       : str
             , arity      : int
             , sign       : bool
             )
          | ShowTerm
             ( location : Location
             , term     : term
             , body     : body_literal*
             , csp      : bool
             )
          | Minimize
             ( location : Location
             , weight   : term
             , priority : term
             , tuple    : term*
             , body     : body_literal*
             )
          | Script
             ( location    : Location
             , script_type : ScriptType
             , code        : str
             )
          | Program
             ( location   : Location
             , name       : str
             , parameters : Id
                             ( location : Location
                             , id       : str
                             )*
             )
          | External
             ( location : Location
             , atom     : symbolic_atom
             , body     : body_literal*
             , type     : term
             )
          | Edge
             ( location : Location
             , u        : term
             , v        : term
             , body     : body_literal*
             )
          | Heuristic
             ( location : Location
             , atom     : symbolic_atom
             , body     : body_literal*
             , bias     : term
             , priority : term
             , modifier : term
             )
          | ProjectAtom
             ( location : Location
             , atom     : symbolic_atom
             , body     : body_literal*
             )
          | ProjectSignature
             ( location   : Location
             , name       : str
             , arity      : int
             , positive   : bool
             )
```
)";

static PyMethodDef clingoModuleMethods[] = {
    {"parse_term", to_function<parseTerm>(), METH_VARARGS | METH_KEYWORDS,
R"(parse_term(string: str, logger: Callback[[MessageCode,str],None]=None, message_limit: int=20) -> Symbol

Parse the given string using gringo's term parser for ground terms.

The function also evaluates arithmetic functions.

Parameters
----------
string : str
    The string to be parsed.
logger : Callback[[MessageCode,str],None] = None
    Function to intercept messages normally printed to standard error.
message_limit : int = 20
    Maximum number of messages passed to the logger.

Returns
-------
Symbol

Examples
--------

    >>> import clingo
    >>> clingo.parse_term('p(1+2)')
    p(3)
)"},
    {"clingo_main", to_function<clingoMain>(), METH_VARARGS | METH_KEYWORDS,
R"(clingo_main(application: Application, files: List[str]=[]) -> int

Runs the given application using clingo's default output and signal handling.

The application can overwrite clingo's default behaviour by registering
additional options and overriding its default main function.

Parameters
----------
application : Application
    The Application object (see notes).
files : List[str]
    The files to pass to the main function of the application.

Returns
-------
int
    The exit code of the application.
Notes
-----
The application object must implement a main function and additionally can
override the other functions.

    class Application(object):
        """
        Interface that has to be implemented to customize clingo.

        Attributes
        ----------
        program_name: str = 'clingo'
            Optional program name to be used in the help output.

        message_limit: int = 20
            Maximum number of messages passed to the logger.
        """

        def main(self, control: Control, files: List[str]) -> None:
            """
            Function to replace clingo's default main function.

            Parameters
            ----------
            control : Control
                The main control object.
            files : List[str]
                The files passed to clingo_main.

            Returns
            -------
            None
            """

        def register_options(self, options: ApplicationOptions) -> None:
            """
            Function to register custom options.

            Parameters
            ----------
            options : ApplicationOptions
                Object to register additional options

            Returns
            -------
            None
            """

        def validate_options(self) -> bool:
            """
            Function to validate custom options.

            This function should return false or throw an exception if option
            validation fails.

            Returns
            -------
            bool
            """

        def logger(self, code: MessageCode, message: str) -> None:
            """
            Function to intercept messages normally printed to standard error.

            By default, messages are printed to stdandard error.

            Parameters
            ----------
            code : MessageCode
                The message code.
            message : str
                The message string.

            Returns
            -------
            None

            Notes
            -----
            This function should not raise exceptions.
            """

Examples
--------

The following example reproduces the default clingo application:

    import sys
    import clingo

    class Application:
        def __init__(self, name):
            self.program_name = name

        def main(self, ctl, files):
            if len(files) > 0:
                for f in files:
                    ctl.load(f)
            else:
                ctl.load("-")
            ctl.ground([("base", [])])
            ctl.solve()

    clingo.clingo_main(Application(sys.argv[0]), sys.argv[1:])
)"},
    {"parse_program", to_function<parseProgram>(), METH_VARARGS | METH_KEYWORDS,
R"(parse_program(program: str, callback: Callable[[ast.AST], None]) -> None

Parse the given program and return an abstract syntax tree for each statement
via a callback.

Parameters
----------
program : str
    String representation of the program.
callback : Callable[[ast.AST], None]
    Callback taking an ast as argument.

Returns
-------
None

See Also
--------
ProgramBuilder
)"},
    {"Function", to_function<Symbol::new_function>(), METH_VARARGS | METH_KEYWORDS, R"(Function(name: str, arguments: List[Symbol]=[], positive: bool=True) -> Symbol

Construct a function symbol.

This includes constants and tuples. Constants have an empty argument list and
tuples have an empty name. Functions can represent classically negated atoms.
Argument `positive` has to be set to false to represent such atoms.

Parameters
----------
name : str
    The name of the function (empty for tuples).
arguments: List[Symbol] = []
    The arguments in form of a list of symbols.
positive: bool = True
    The sign of the function (tuples must not have signs).

Returns
-------
Symbol
)"},
    {"Tuple", to_function<Symbol::new_tuple>(), METH_O, R"(Tuple(arguments: List[Symbol]) -> Symbol

A shortcut for `Function("", arguments)`.

Parameters
----------
arguments: List[Symbol]
    The arguments in form of a list of symbols.

Returns
-------
Symbol

See Also
--------
clingo.Function
)"},
    {"Number", to_function<Symbol::new_number>(), METH_O, R"(Number(number: int) -> Symbol

Construct a numeric symbol given a number.

Parameters
----------
number : int
    The given number.

Returns
-------
Symbol
)"},
    {"String", to_function<Symbol::new_string>(), METH_O, R"(String(string: str) -> Symbol

Construct a string symbol given a string.

Parameters
----------
string : str
    The given string.

Returns
-------
Symbol
)"},
    {"_Symbol", to_function<Symbol::new_symbol>(), METH_O, R"(_Symbol(value: int) -> Symbol

Construct a symbol from its numeric C representation.

Parameters
----------
value : int
    The internal value of the symbol.

Returns
-------
Symbol
)"},
    {"_error_message", to_function<clingoErrorMessage>(), METH_NOARGS, R"(_error_message() -> str

Get the internal error message.

Returns
-------
str
)"},
    {"_error_code", to_function<clingoErrorCode>(), METH_NOARGS, R"(_error_code() -> int

Get the internal error code.

Returns
-------
int
)"},
    {nullptr, nullptr, 0, nullptr}
};
static char const *clingoModuleDoc =
"The clingo-" CLINGO_VERSION R"( module.

This module provides functions and classes to control the grounding and solving
process.

If the clingo application is build with Python support, clingo will also be
able to execute Python code embedded in logic programs.  Functions defined in a
Python script block are callable during the instantiation process using
`@`-syntax.  The default grounding/solving process can be customized if a main
function is provided.

Note that gringo's precomputed terms (terms without variables and interpreted
functions), called symbols in the following, are wrapped in the Symbol class.
Furthermore, strings, numbers, and tuples can be passed wherever a symbol is
expected - they are automatically converted into a Symbol object.  Functions
called during the grounding process from the logic program must either return a
symbol or a sequence of symbols.  If a sequence is returned, the corresponding
`@`-term is successively substituted by the values in the sequence.

## Examples

The first example shows how to use the clingo module from Python.

    >>> import clingo
    >>> class Context:
    ...     def id(self, x):
    ...         return x
    ...     def seq(self, x, y):
    ...         return [x, y]
    ...
    >>> def on_model(m):
    ...     print (m)
    ...
    >>> ctl = clingo.Control()
    >>> ctl.add("base", [], """\
    ... p(@id(10)).
    ... q(@seq(1,2)).
    ... """)
    >>> ctl.ground([("base", [])], context=Context())
    >>> ctl.solve(on_model=on_model)
    p(10) q(1) q(2)
    SAT

The second example shows how to use Python code from clingo.

    #script (python)

    import clingo

    class Context:
        def id(x):
            return x

        def seq(x, y):
            return [x, y]

    def main(prg):
        prg.ground([("base", [])], context=Context())
        prg.solve()

    #end.

    p(@id(10)).
    q(@seq(1,2)).
)";

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef clingoModule = {
    PyModuleDef_HEAD_INIT,
    "clingo",
    clingoModuleDoc,
    -1,
    clingoModuleMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

static struct PyModuleDef clingoASTModule = {
    PyModuleDef_HEAD_INIT,
    "clingo.ast",
    clingoASTModuleDoc,
    -1,
    clingoASTModuleMethods,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};
#endif

PyObject *initclingoast_() {
    PY_TRY {
#if PY_MAJOR_VERSION >= 3
        Object m = PyModule_Create(&clingoASTModule);
        Reference{PySys_GetObject("modules")}.setItem(clingoASTModule.m_name, m);
#else
        Object m = Py_InitModule3("clingo.ast", clingoASTModuleMethods, clingoASTModuleDoc);
#endif
        if (!m.valid() ||
            !ComparisonOperator::initType(m) || !Sign::initType(m)               || !AST::initType(m)   ||
            !ASTType::initType(m)            || !UnaryOperator::initType(m)      || !BinaryOperator::initType(m)     ||
            !AggregateFunction::initType(m)  || !TheorySequenceType::initType(m) || !TheoryOperatorType::initType(m) ||
            !TheoryAtomType::initType(m)     || !ScriptType::initType(m)         ||
            false) { return nullptr; }
        return m.release();
    }
    PY_CATCH(nullptr);
}

PyObject *initclingo_() {
    PY_TRY {
        if (!PyEval_ThreadsInitialized()) { PyEval_InitThreads(); }
#if PY_MAJOR_VERSION >= 3
        Object m = PyModule_Create(&clingoModule);
#else
        Object m = Py_InitModule3("clingo", clingoModuleMethods, clingoModuleDoc);
#endif
        if (!m.valid() ||
            !SolveResult::initType(m)         || !TheoryTermType::initType(m)   || !PropagateControl::initType(m) ||
            !TheoryElement::initType(m)       || !TheoryAtom::initType(m)       || !TheoryAtomIter::initType(m)   ||
            !Model::initType(m)               || !ModelType::initType(m)        || !SolveHandle::initType(m)      ||
            !ControlWrap::initType(m)         || !Configuration::initType(m)    || !SolveControl::initType(m)     ||
            !SymbolicAtom::initType(m)        || !SymbolicAtomIter::initType(m) || !SymbolicAtoms::initType(m)    ||
            !TheoryTerm::initType(m)          || !PropagateInit::initType(m)    || !Assignment::initType(m)       ||
            !SymbolType::initType(m)          || !Symbol::initType(m)           || !Backend::initType(m)          ||
            !ProgramBuilder::initType(m)      || !HeuristicType::initType(m)    || !TruthValue::initType(m)       ||
            !PropagatorCheckMode::initType(m) || !MessageCode::initType(m)      || !Flag::initType(m)             ||
            !ApplicationOptions::initType(m)  || !StatisticsArray::initType(m)  || !StatisticsMap::initType(m)    ||
            PyModule_AddStringConstant(m.toPy(), "__version__", CLINGO_VERSION) < 0 ||
            false) { return nullptr; }
        Reference a{initclingoast_()};
        Py_XINCREF(a.toPy());
        if (PyModule_AddObject(m.toPy(), "ast", a.toPy()) < 0) { return nullptr; }
        return m.release();
    }
    PY_CATCH(nullptr);
}

// {{{1 auxiliary functions and objects

bool pyIsInt(Reference x) {
    if (PyLong_Check(x.toPy())) { return true; }
#if PY_MAJOR_VERSION < 3
    if (PyInt_Check(x.toPy())) { return true; }
#endif
    return false;
}

bool pyIsSymbol(Reference obj) {
    if (obj.isInstance(Symbol::type)) { return true; }
    if (PyTuple_Check(obj.toPy()))    { return true; }
    if (pyIsInt(obj))                 { return true; }
    if (PyString_Check(obj.toPy()))   { return true; }
    return false;
}

void pyToCpp(Reference obj, symbol_wrapper &val) {
    if (obj.isInstance(Symbol::type))    { val.symbol = reinterpret_cast<Symbol*>(obj.toPy())->val; }
    else if (PyTuple_Check(obj.toPy()))  {
        auto vec = pyToCpp<symbol_vector>(obj);
        handle_c_error(clingo_symbol_create_function("", reinterpret_cast<clingo_symbol_t*>(vec.data()), vec.size(), true, &val.symbol));
    }
    else if (pyIsInt(obj))               { clingo_symbol_create_number(pyToCpp<int>(obj), &val.symbol); }
    else if (PyString_Check(obj.toPy())) { handle_c_error(clingo_symbol_create_string(pyToCpp<std::string>(obj).c_str(), &val.symbol)); }
    else {
        PyErr_Format(PyExc_RuntimeError, "cannot convert to value: unexpected %s() object", obj.toPy()->ob_type->tp_name);
        throw PyException();
    }
}

Object cppToPy(symbol_wrapper val) {
    return Symbol::construct(val.symbol);
}

template <class T>
Object cppRngToPy(T begin, T end) {
    List list{static_cast<size_t>(std::distance(begin, end))};
    int i = 0;
    for (auto it = begin; it != end; ++it, ++i) {
        list.setItem(i, cppToPy(*it));
    }
    return list;
}

template <class T>
Object cppToPy(std::vector<T> const &vals) {
    return cppRngToPy(vals.begin(), vals.end());
}

template <class T>
Object cppToPy(std::initializer_list<T> l) {
    return cppRngToPy(l.begin(), l.end());
}

template <class T>
Object cppToPy(T const *arr, size_t size) {
    return cppRngToPy(arr, arr + size);
}

template <class T, class U>
Object cppToPy(std::pair<T, U> const &pair) {
    return Tuple(cppToPy(pair.first), cppToPy(pair.second));
}

// {{{1 definition of PythonImpl

class PythonImpl {
public:
    PythonImpl() : selfInit(!Py_IsInitialized()) {
        if (selfInit) {
#if PY_MAJOR_VERSION >= 3
            PyImport_AppendInittab("clingo", &initclingo_);
#else
            PyImport_AppendInittab("clingo", []() { initclingo_(); });
#endif
            Py_Initialize();
#if PY_MAJOR_VERSION >= 3
            static wchar_t const *argv[] = {L"clingo", 0};
            PySys_SetArgvEx(1, const_cast<wchar_t**>(argv), 0);
#else
            static char const *argv[] = {"clingo", 0};
            PySys_SetArgvEx(1, const_cast<char**>(argv), 0);
#endif
            List path{Reference{PySys_GetObject(const_cast<char*>("path"))}};
            path.append(cppToPy("."));
        }
        Object clingoModule = PyImport_ImportModule("clingo");
        Object mainModule = PyImport_ImportModule("__main__");
        main = PyModule_GetDict(mainModule.toPy());
        if (!main) { throw PyException(); }
    }
    ~PythonImpl() {
        if (selfInit) { Py_Finalize(); }
    }
    void exec(clingo_location_t loc, const char *code) {
        std::ostringstream oss;
        oss << "<" << loc << ">";
        pyExec(code, oss.str().c_str(), main);
    }
    bool callable(char const *name) {
        PyBlock block;
        if (!PyMapping_HasKeyString(main, const_cast<char *>(name))) { return false; }
        Object fun = PyMapping_GetItemString(main, const_cast<char *>(name));
        return PyCallable_Check(fun.toPy());
    }
    void call(char const *name, clingo_symbol_t const *arguments, size_t size, clingo_symbol_callback_t symbol_callback, void *data) {
        PyBlock block;
        Object fun = PyMapping_GetItemString(main, const_cast<char*>(name));
        pycall(fun, arguments, size, symbol_callback, data);
    }
    void call(clingo_control_t *ctl) {
        Object fun = PyMapping_GetItemString(main, const_cast<char*>("main"));
        Object params = PyTuple_New(1);
        Object param(ControlWrap::construct(ctl));
        if (PyTuple_SetItem(params.toPy(), 0, param.release()) < 0) { throw PyException(); }
        Object ret = PyObject_Call(fun.toPy(), params.toPy(), nullptr);
    }
private:
    bool      selfInit;
    PyObject *main;
};

struct PythonScript {
    static bool execute(clingo_location_t const *loc, char const *code, void *) {
        try {
            if (!impl) { impl.reset(new PythonImpl()); }
            impl->exec(*loc, code);
            return true;
        }
        catch (...) {
            handle_cxx_error(*loc, "error executing python code");
            return false;
        }
    }
    static bool call(clingo_location_t const *loc, char const *name, clingo_symbol_t const *arguments, size_t size, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data, void *) {
        try {
            if (!impl) { impl.reset(new PythonImpl()); }
            impl->call(name, arguments, size, symbol_callback, symbol_callback_data);
            return true;
        }
        catch (...) {
            PyBlock block;
            handle_cxx_error(*loc, "error calling python function");
            return false;
        }
    }
    static bool callable(char const * name, bool *ret, void *) {
        try {
            *ret = impl && impl->callable(name);
            return true;
        }
        catch (...) {
            PyBlock block;
            handle_cxx_error("<python>", "error cecking if function is callable");
            return false;
        }
    }
    static bool main(clingo_control_t *ctl, void *) {
        try {
            if (!impl) { impl.reset(new PythonImpl()); }
            impl->call(ctl);
            return true;
        }
        catch (...) {
            handle_cxx_error("<python>", "error calling main function");
            return false;
        }
    }
    static void free(void *) { }

    static std::unique_ptr<PythonImpl> impl;
};

std::unique_ptr<PythonImpl> PythonScript::impl = nullptr;

// }}}1

} // namespace

// {{{1 definition of Python

extern "C" void *clingo_init_python_() {
    PY_TRY { return initclingo_(); }
    PY_CATCH(nullptr);
}

extern "C" bool clingo_register_python_() {
    try {
        clingo_script_t_ script = {
            PythonScript::execute,
            PythonScript::call,
            PythonScript::callable,
            PythonScript::main,
            PythonScript::free,
            PY_VERSION
        };
        return clingo_register_script_(clingo_ast_script_type_python, &script, nullptr);
    }
    catch (...) {
        handle_cxx_error("python", "error registering python script");
        return false;
    }
}

// }}}1
