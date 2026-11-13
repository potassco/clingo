#define PY_SSIZE_T_CLEAN

#include "app.hh"

#include "control.hh"
#include "core.hh"
#include "solve.hh"
#include "util.hh"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace PyClingo {

namespace {

struct AppModuleState {
    PyObject *flag_type{nullptr};
    PyObject *options_type{nullptr};
    PyObject *app_type{nullptr};
};

using PyFlagObject = struct {
    PyObject_HEAD bool value;
};

using PyOptionsObject = struct {
    PyObject_HEAD clingo_options_t *opts;
    PyObject *keep;
};

using PyAppObject = struct {
    PyObject_HEAD PyObject *program_name;
    PyObject *version;
    PyObject *commit_hash;
    PyObject *keep;
    AppModuleState *state;
};

struct PrinterData {
    clingo_default_model_printer_t printer;
    void *data;
    bool valid;
};

constexpr auto printer_capsule_name = "clingo.app.default_printer";

// module state

auto get_state(PyObject *module) -> AppModuleState * {
    auto *state = static_cast<AppModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "app module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> AppModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve app module");
        return nullptr;
    }
    return get_state(mod);
}

auto py_flag_cast(PyObject *obj) -> PyFlagObject * {
    return reinterpret_cast<PyFlagObject *>(obj);
}

auto py_options_cast(PyObject *obj) -> PyOptionsObject * {
    return reinterpret_cast<PyOptionsObject *>(obj);
}

auto py_app_cast(PyObject *obj) -> PyAppObject * {
    return reinterpret_cast<PyAppObject *>(obj);
}

// helpers

auto app_str(PyObject *str) noexcept -> clingo_string_t {
    Py_ssize_t size = 0;
    auto const *data = PyUnicode_AsUTF8AndSize(str, &size);
    if (data == nullptr) {
        PyErr_Clear();
        fprintf(stderr, "panic: invalid application string\n");
        std::abort();
    }
    return clingo_string_t{data, static_cast<size_t>(size)};
}

auto app_program_name(PyAppObject const *app) noexcept -> clingo_string_t {
    if (app != nullptr && app->program_name != nullptr) {
        return app_str(app->program_name);
    }
    return clingo_string_t{CLINGO_EXECUTABLE, std::strlen(CLINGO_EXECUTABLE)};
}

// NOTE: these two are the only places creating pybind11 wrapped objects.
auto new_control(clingo_control_t *ctl) noexcept -> PyObject * {
    try {
        py::object obj = Control::cast(ctl, true);
        return obj.release().ptr();
    } catch (py::error_already_set &e) {
        e.restore();
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
    }
    return nullptr;
}

auto new_model(clingo_model_t const *model) noexcept -> PyObject * {
    try {
        py::object obj = py::cast(Model{model});
        return obj.release().ptr();
    } catch (py::error_already_set &e) {
        e.restore();
    } catch (std::exception const &e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
    }
    return nullptr;
}

void report_value_error() noexcept {
    PyObject *type = nullptr;
    PyObject *value = nullptr;
    PyObject *tb = nullptr;
    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);
    if (type != nullptr && PyErr_GivenExceptionMatches(type, PyExc_ValueError) != 0) {
        auto msg = steal(value != nullptr ? PyObject_Str(value) : nullptr);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tb);
        Py_ssize_t size = 0;
        char const *str = msg.get() != nullptr ? PyUnicode_AsUTF8AndSize(msg.get(), &size) : nullptr;
        if (str != nullptr) {
            clingo_set_error(clingo_result_invalid, str, static_cast<size_t>(size));
            return;
        }
    } else {
        PyErr_Restore(type, value, tb);
    }
    if (PyErr_Occurred() == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "validate_options failed");
    }
    store_python_error("validate_options failed");
}

// Flag

auto py_flag_init(PyObject *self, PyObject *args, PyObject *kwargs) -> int {
    static constexpr auto kwlist = std::array<char const *, 2>{"value", nullptr};
    int value = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|p:Flag", cast_kw(kwlist), &value) == 0) {
        return -1;
    }
    py_flag_cast(self)->value = value != 0;
    return 0;
}

auto py_flag_get_value(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    return PyBool_FromLong(py_flag_cast(self)->value ? 1 : 0);
}

auto py_flag_set_value(PyObject *self, PyObject *value, [[maybe_unused]] void *arg) -> int {
    if (value == nullptr) {
        PyErr_SetString(PyExc_TypeError, "cannot delete value");
        return -1;
    }
    auto res = PyObject_IsTrue(value);
    if (res < 0) {
        return -1;
    }
    py_flag_cast(self)->value = res != 0;
    return 0;
}

auto py_flag_getset = std::array{
    PyGetSetDef{"value", py_flag_get_value, py_flag_set_value, PyDoc_STR("Get/set the value of the flag."), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_flag_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Boolean flag with value management."))},
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(PyType_GenericNew)},
    PyType_Slot{Py_tp_init, reinterpret_cast<void *>(py_flag_init)},
    PyType_Slot{Py_tp_getset, py_flag_getset.data()},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto py_flag_spec = PyType_Spec{
    "clingo.app.Flag", sizeof(PyFlagObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, py_flag_slots.data(),
};

// AppOptions

auto py_options_new(PyObject *type, clingo_options_t *opts, PyObject *keep) -> PyObject * {
    auto *obj = PyType_GenericAlloc(reinterpret_cast<PyTypeObject *>(type), 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *o = py_options_cast(obj);
    o->opts = opts;
    o->keep = keep;
    Py_INCREF(keep);
    return obj;
}

auto py_options_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *o = py_options_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(o->keep);
    return 0;
}

auto py_options_clear(PyObject *self) -> int {
    auto *o = py_options_cast(self);
    Py_CLEAR(o->keep);
    return 0;
}

auto py_options_check(PyObject *self) -> PyOptionsObject * {
    auto *o = py_options_cast(self);
    if (o->opts == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "options are only valid during register_options");
        return nullptr;
    }
    return o;
}

auto options_parser_cb(char const *value, size_t size, void *data) -> bool {
    auto str = steal(PyUnicode_FromStringAndSize(value, static_cast<Py_ssize_t>(size)));
    if (str.get() == nullptr) {
        return store_python_error("decoding option value failed");
    }
    auto ret = steal(PyObject_CallOneArg(static_cast<PyObject *>(data), str.get()));
    if (ret.get() == nullptr) {
        return store_python_error("option parser failed");
    }
    return true;
}

auto py_options_add(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_options_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    static constexpr auto kwlist =
        std::array<char const *, 7>{"group", "option", "description", "parser", "multi", "argument", nullptr};
    char const *group = nullptr;
    char const *option = nullptr;
    char const *description = nullptr;
    char const *argument = nullptr;
    Py_ssize_t group_n = 0;
    Py_ssize_t option_n = 0;
    Py_ssize_t description_n = 0;
    Py_ssize_t argument_n = 0;
    PyObject *parser = nullptr;
    int multi = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#s#s#O|pz#:add", cast_kw(kwlist), &group, &group_n, &option,
                                    &option_n, &description, &description_n, &parser, &multi, &argument,
                                    &argument_n) == 0) {
        return nullptr;
    }
    if (PyCallable_Check(parser) == 0) {
        PyErr_SetString(PyExc_TypeError, "parser must be callable");
        return nullptr;
    }
    if (PyList_Append(o->keep, parser) < 0) {
        return nullptr;
    }
    if (!clingo_options_add(o->opts, group, static_cast<size_t>(group_n), option, static_cast<size_t>(option_n),
                            description, static_cast<size_t>(description_n), options_parser_cb,
                            static_cast<void *>(parser), multi != 0, argument,
                            argument != nullptr ? static_cast<size_t>(argument_n) : 0)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_options_add_flag(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_options_check(self);
    if (o == nullptr) {
        return nullptr;
    }
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    static constexpr auto kwlist = std::array<char const *, 5>{"group", "option", "description", "flag", nullptr};
    char const *group = nullptr;
    char const *option = nullptr;
    char const *description = nullptr;
    Py_ssize_t group_n = 0;
    Py_ssize_t option_n = 0;
    Py_ssize_t description_n = 0;
    PyObject *flag = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#s#s#O:add_flag", cast_kw(kwlist), &group, &group_n, &option,
                                    &option_n, &description, &description_n, &flag) == 0) {
        return nullptr;
    }
    if (PyObject_TypeCheck(flag, reinterpret_cast<PyTypeObject *>(state->flag_type)) == 0) {
        PyErr_SetString(PyExc_TypeError, "flag must be a Flag");
        return nullptr;
    }
    if (PyList_Append(o->keep, flag) < 0) {
        return nullptr;
    }
    if (!clingo_options_add_flag(o->opts, group, static_cast<size_t>(group_n), option, static_cast<size_t>(option_n),
                                 description, static_cast<size_t>(description_n), &py_flag_cast(flag)->value)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_options_set_default_value(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_options_check(self);
    if (o == nullptr) {
        return nullptr;
    }

    static constexpr auto kwlist = std::array<char const *, 3>{"option", "value", nullptr};
    char const *option = nullptr;
    char const *value = nullptr;
    Py_ssize_t option_n = 0;
    Py_ssize_t value_n = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "s#s#:set_default_value", cast_kw(kwlist), &option, &option_n, &value,
                                    &value_n) == 0) {
        return nullptr;
    }
    if (!clingo_options_set_default_value(o->opts, option, static_cast<size_t>(option_n), value,
                                          static_cast<size_t>(value_n))) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_options_methods = std::array{
    PyMethodDef{
        "add",
        py_c_function_cast(py_options_add),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Add an option with a custom parser."),
    },
    PyMethodDef{
        "add_flag",
        py_c_function_cast(py_options_add_flag),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Add a Boolean flag option."),
    },
    PyMethodDef{
        "set_default_value",
        py_c_function_cast(py_options_set_default_value),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Set the default value of an existing clingo option."),
    },
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_options_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Manager for application options and their definitions."))},
    PyType_Slot{Py_tp_methods, py_options_methods.data()},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_options_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_options_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto py_options_spec = PyType_Spec{
    "clingo.app.AppOptions",
    sizeof(PyOptionsObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_options_slots.data(),
};

// App

auto set_opt_str_ok(PyObject *value, char const *what) -> bool {
    if (value == Py_None) {
        return true;
    }
    if (PyUnicode_Check(value) == 0) {
        PyErr_Format(PyExc_TypeError, "%s must be a str or None", what);
        return false;
    }
    return PyUnicode_AsUTF8AndSize(value, nullptr) != nullptr;
}

void set_opt_str(PyObject **slot, PyObject *value) {
    auto *old = *slot;
    if (value == Py_None) {
        value = nullptr;
    }
    Py_XINCREF(value);
    *slot = value;
    Py_XDECREF(old);
}

auto py_app_new(PyTypeObject *type, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs) -> PyObject * {
    auto *obj = PyType_GenericAlloc(type, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = py_app_cast(obj);
    a->keep = PyList_New(0);
    if (a->keep == nullptr) {
        Py_DECREF(obj);
        return nullptr;
    }
    return obj;
}

auto py_app_init(PyObject *self, PyObject *args, PyObject *kwargs) -> int {
    static constexpr auto kwlist = std::array<char const *, 4>{"program_name", "version", "commit_hash", nullptr};
    PyObject *name = Py_None;
    PyObject *version = Py_None;
    PyObject *hash = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO:App", cast_kw(kwlist), &name, &version, &hash) == 0) {
        return -1;
    }
    if (!set_opt_str_ok(name, "program_name") || !set_opt_str_ok(version, "version") ||
        !set_opt_str_ok(hash, "commit_hash")) {
        return -1;
    }
    auto *a = py_app_cast(self);
    set_opt_str(&a->program_name, name);
    set_opt_str(&a->version, version);
    set_opt_str(&a->commit_hash, hash);
    return 0;
}

auto py_app_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *a = py_app_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(a->program_name);
    Py_VISIT(a->version);
    Py_VISIT(a->commit_hash);
    Py_VISIT(a->keep);
    return 0;
}

auto py_app_clear(PyObject *self) -> int {
    auto *a = py_app_cast(self);
    Py_CLEAR(a->program_name);
    Py_CLEAR(a->version);
    Py_CLEAR(a->commit_hash);
    Py_CLEAR(a->keep);
    return 0;
}

auto py_app_noop([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    Py_RETURN_NONE;
}

auto py_app_noop_kw([[maybe_unused]] PyObject *self, [[maybe_unused]] PyObject *args, [[maybe_unused]] PyObject *kwargs)
    -> PyObject * {
    Py_RETURN_NONE;
}

auto py_app_methods = std::array{
    PyMethodDef{
        "register_options",
        py_app_noop,
        METH_O,
        PyDoc_STR("Register command-line options."),
    },
    PyMethodDef{
        "validate_options",
        py_app_noop,
        METH_NOARGS,
        PyDoc_STR("Validate options; raise ValueError."),
    },
    PyMethodDef{
        "print_model",
        py_c_function_cast(py_app_noop_kw),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Print the given model in a custom format."),
    },
    PyMethodDef{
        "main",
        py_c_function_cast(py_app_noop_kw),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Run the main execution flow of the application."),
    },
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

// Callbacks passed to clingo. The data pointer is the App instance.

void app_get_program_name_cb(void *data, clingo_string_t *res) {
    *res = app_str(py_app_cast(static_cast<PyObject *>(data))->program_name);
}

void app_get_version_cb(void *data, clingo_string_t *res) {
    *res = app_str(py_app_cast(static_cast<PyObject *>(data))->version);
}

void app_get_commit_hash_cb(void *data, clingo_string_t *res) {
    *res = app_str(py_app_cast(static_cast<PyObject *>(data))->commit_hash);
}

auto app_main_cb(clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) -> bool {
    auto *app = static_cast<PyObject *>(data);
    auto pyctl = steal(new_control(ctl));
    if (pyctl.get() == nullptr) {
        return store_python_error("creating control failed");
    }
    auto pyfiles = steal(PyList_New(static_cast<Py_ssize_t>(size)));
    if (pyfiles.get() == nullptr) {
        return store_python_error("creating file list failed");
    }
    for (size_t i = 0; i < size; ++i) {
        auto *str = PyUnicode_FromStringAndSize(files[i].data, static_cast<Py_ssize_t>(files[i].size));
        if (str == nullptr) {
            return store_python_error("decoding file name failed");
        }
        // NOTE: steals the reference to str
        if (PyList_SetItem(pyfiles.get(), static_cast<Py_ssize_t>(i), str) < 0) {
            return store_python_error("creating file list failed");
        }
    }
    auto ret = steal(PyObject_CallMethod(app, "main", "OO", pyctl.get(), pyfiles.get()));
    if (ret.get() == nullptr) {
        return store_python_error("main failed");
    }
    return true;
}

auto py_printer_call(PyObject *self, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *pd = static_cast<PrinterData *>(PyCapsule_GetPointer(self, printer_capsule_name));
    if (pd == nullptr) {
        return nullptr;
    }
    if (!pd->valid) {
        PyErr_SetString(PyExc_RuntimeError, "default printer is only valid during print_model");
        return nullptr;
    }
    if (!pd->printer(pd->data)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

void printer_capsule_free(PyObject *cap) {
    PyMem_Free(PyCapsule_GetPointer(cap, printer_capsule_name));
}

auto py_printer_def =
    PyMethodDef{"default_printer", py_printer_call, METH_NOARGS, PyDoc_STR("Print the model in the default format.")};

auto app_print_model_cb(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
                        void *data) -> bool {
    auto *app = static_cast<PyObject *>(data);
    auto gil = GILGuard{};
    // NOTE: Python seems to directly write large buffers
    fflush(stdout);

    auto pymodel = steal(new_model(model));
    if (pymodel.get() == nullptr) {
        return store_python_error("creating model failed");
    }

    auto *pd = static_cast<PrinterData *>(PyMem_Malloc(sizeof(PrinterData)));
    if (pd == nullptr) {
        PyErr_NoMemory();
        return store_python_error("creating printer failed");
    }
    *pd = PrinterData{printer, printer_data, true};
    auto cap = steal(PyCapsule_New(pd, printer_capsule_name, printer_capsule_free));
    if (cap.get() == nullptr) {
        PyMem_Free(pd);
        return store_python_error("creating printer failed");
    }
    auto fun = steal(PyCFunction_New(&py_printer_def, cap.get()));
    if (fun.get() == nullptr) {
        return store_python_error("creating printer failed");
    }

    auto ret = steal(PyObject_CallMethod(app, "print_model", "OO", pymodel.get(), fun.get()));
    // NOTE: the printer must not be used once the model is gone
    pd->valid = false;
    if (ret.get() == nullptr) {
        return store_python_error("print_model failed");
    }

    auto out = borrow(PySys_GetObject("stdout"));
    if (out.get() != nullptr && out.get() != Py_None) {
        auto res = steal(PyObject_CallMethod(out.get(), "flush", nullptr));
        if (res.get() == nullptr) {
            return store_python_error("flushing stdout failed");
        }
    }
    return true;
}

auto app_register_options_cb(clingo_options_t *options, void *data) -> bool {
    auto *app = static_cast<PyObject *>(data);
    auto *a = py_app_cast(app);
    auto opts = steal(py_options_new(a->state->options_type, options, a->keep));
    if (opts.get() == nullptr) {
        return store_python_error("creating options failed");
    }
    auto ret = steal(PyObject_CallMethod(app, "register_options", "O", opts.get()));
    // NOTE: the options object must not be used after this point
    py_options_cast(opts.get())->opts = nullptr;
    if (ret.get() == nullptr) {
        return store_python_error("register_options failed");
    }
    return true;
}

auto app_validate_options_cb(void *data) -> bool {
    auto *app = static_cast<PyObject *>(data);
    auto ret = steal(PyObject_CallMethod(app, "validate_options", nullptr));
    if (ret.get() != nullptr) {
        return true;
    }
    report_value_error();
    return false;
}

// Check if the given method has been overridden in a subclass.
auto has_override(PyObject *self, PyObject *base_type, char const *name) -> int {
    auto cur = steal(PyObject_GetAttrString(reinterpret_cast<PyObject *>(Py_TYPE(self)), name));
    if (cur.get() == nullptr) {
        return -1;
    }
    auto base = steal(PyObject_GetAttrString(base_type, name));
    if (base.get() == nullptr) {
        return -1;
    }
    return cur.get() != base.get() ? 1 : 0;
}

auto app_prepare(PyObject *self, AppModuleState *state, clingo_application_t *out) -> bool {
    auto has_main = has_override(self, state->app_type, "main");
    auto has_print_model = has_override(self, state->app_type, "print_model");
    auto has_register_options = has_override(self, state->app_type, "register_options");
    auto has_validate_options = has_override(self, state->app_type, "validate_options");
    if (has_main < 0 || has_print_model < 0 || has_register_options < 0 || has_validate_options < 0) {
        return false;
    }
    auto *app = py_app_cast(self);
    app->state = state;
    *out = clingo_application_t{
        app->program_name != nullptr ? app_get_program_name_cb : nullptr,
        app->version != nullptr ? app_get_version_cb : nullptr,
        app->commit_hash != nullptr ? app_get_commit_hash_cb : nullptr,
        has_main != 0 ? app_main_cb : nullptr,
        has_print_model != 0 ? app_print_model_cb : nullptr,
        has_register_options != 0 ? app_register_options_cb : nullptr,
        has_validate_options != 0 ? app_validate_options_cb : nullptr,
    };
    return true;
}

auto py_app_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Interface to implement a custom clingo-based application."))},
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(py_app_new)},
    PyType_Slot{Py_tp_init, reinterpret_cast<void *>(py_app_init)},
    PyType_Slot{Py_tp_methods, py_app_methods.data()},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_app_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_app_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(free_object)},
    PyType_Slot{0, nullptr},
};

auto py_app_spec = PyType_Spec{
    "clingo.app.App",    sizeof(PyAppObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,
    py_app_slots.data(),
};

// module functions

auto run_main(AppModuleState *state, PyObject *lib_obj, PyObject *args_obj, PyObject *app_obj) -> PyObject * {
    auto *lib = py_cast<Library>(lib_obj);
    if (lib == nullptr) {
        return nullptr;
    }

    PyGuard seq;
    PyBuffer<clingo_string_t> cargs;
    if (!parse_string_sequence(args_obj, seq, cargs, "arguments must be a sequence of strings")) {
        return nullptr;
    }

    PyObject *app = nullptr;
    if (app_obj != nullptr && app_obj != Py_None) {
        if (PyObject_TypeCheck(app_obj, reinterpret_cast<PyTypeObject *>(state->app_type)) == 0) {
            PyErr_SetString(PyExc_TypeError, "app must be an App");
            return nullptr;
        }
        app = app_obj;
    }

    auto capp = clingo_application_t{};
    if (app != nullptr && !app_prepare(app, state, &capp)) {
        return nullptr;
    }

    int code = 0;
    auto ok = clingo_main(*lib, cargs.data(), cargs.size(), app != nullptr ? &capp : nullptr, static_cast<void *>(app),
                          &code);
    // NOTE: Clasp's main is noexcept, it will report whether an exception was
    // caught via the exit code. Whenever an exception is rethrown from within
    // the internal C++ API, the clingo error will be cleared and *only* be
    // reported by clasp. In this case, no clingo error is set but the exit
    // code indicates an error.
    report_main_error(ok, code, app_program_name(app != nullptr ? py_app_cast(app) : nullptr));
    return PyLong_FromLong(code);
}

auto py_clingo_main(PyObject *module, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }
    static constexpr auto kwlist = std::array<char const *, 4>{"lib", "arguments", "app", nullptr};
    PyObject *lib = nullptr;
    PyObject *arguments = nullptr;
    PyObject *app = Py_None;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO|O:clingo_main", cast_kw(kwlist), &lib, &arguments, &app) == 0) {
        return nullptr;
    }
    return run_main(state, lib, arguments, app);
}

auto py_pyclingo(PyObject *module, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto *state = get_state(module);
    if (state == nullptr) {
        return nullptr;
    }

    auto sys = steal(PyImport_ImportModule("sys"));
    auto core = steal(PyImport_ImportModule("clingo.core"));
    auto script = steal(PyImport_ImportModule("clingo.script"));
    if (sys.get() == nullptr || core.get() == nullptr || script.get() == nullptr) {
        return nullptr;
    }

    auto library = steal(PyObject_GetAttrString(core.get(), "Library"));
    auto enable_python = steal(PyObject_GetAttrString(script.get(), "enable_python"));
    auto argv = steal(PyObject_GetAttrString(sys.get(), "argv"));
    if (library.get() == nullptr || enable_python.get() == nullptr || argv.get() == nullptr) {
        return nullptr;
    }

    auto args = steal(PySequence_GetSlice(argv.get(), 1, PY_SSIZE_T_MAX));
    if (args.get() == nullptr) {
        return nullptr;
    }
    auto lib = steal(PyObject_CallNoArgs(library.get()));
    if (lib.get() == nullptr) {
        return nullptr;
    }
    auto ret = steal(PyObject_CallOneArg(enable_python.get(), lib.get()));
    if (ret.get() == nullptr) {
        return nullptr;
    }
    return run_main(state, lib.get(), args.get(), nullptr);
}

auto app_module_methods = std::array{
    PyMethodDef{
        "clingo_main",
        py_c_function_cast(py_clingo_main),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR("Entry point for running a clingo application; returns the exit code."),
    },
    PyMethodDef{
        "_pyclingo",
        py_pyclingo,
        METH_NOARGS,
        PyDoc_STR("Entry point of the clingo executable."),
    },
    PyMethodDef{
        nullptr,
        nullptr,
        0,
        nullptr,
    },
};

auto app_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<AppModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->flag_type);
    Py_VISIT(state->options_type);
    Py_VISIT(state->app_type);
    return 0;
}

auto app_module_clear(PyObject *module) -> int {
    auto *state = static_cast<AppModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->flag_type);
    Py_CLEAR(state->options_type);
    Py_CLEAR(state->app_type);
    return 0;
}

void app_module_free(void *module) {
    app_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef app_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.app",
    PyDoc_STR("Module to create custom clingo-based applications."),
    static_cast<Py_ssize_t>(sizeof(AppModuleState)),
    app_module_methods.data(),
    nullptr,
    app_module_traverse,
    app_module_clear,
    app_module_free,
};

} // namespace

auto py_app_options_type_ref() -> PyObject * {
    auto mod = steal(PyImport_ImportModule("clingo.app"));
    if (mod.get() == nullptr) {
        return nullptr;
    }
    auto *state = reinterpret_cast<AppModuleState *>(PyModule_GetState(mod.get()));
    if (state == nullptr) {
        return nullptr;
    }
    if (state->options_type == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "clingo.app.AppOptions type not initialized");
        return nullptr;
    }
    return state->options_type;
}

auto py_app_options_check(PyObject *type, PyObject *obj) -> clingo_options_t * {
    if (PyObject_TypeCheck(obj, reinterpret_cast<PyTypeObject *>(type)) == 0) {
        PyErr_SetString(PyExc_TypeError, "object is not of type AppOptions");
        return nullptr;
    }
    auto *o = py_options_check(obj);
    if (o == nullptr) {
        return nullptr;
    }
    return o->opts;
}

void register_app(pybind11::module &m) {
    auto app_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &app_def, &app_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(app_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    if (!add_type(app_mod.get(), "Flag", &py_flag_spec, &state->flag_type) ||
        !add_type(app_mod.get(), "AppOptions", &py_options_spec, &state->options_type) ||
        !add_type(app_mod.get(), "App", &py_app_spec, &state->app_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
