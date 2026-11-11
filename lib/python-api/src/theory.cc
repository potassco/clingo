/*
#include "app.hh"
#include "ast.hh"
#include "control.hh"
#include "solve.hh"
#include "stats.hh"

#include <clingo/theory.h>

namespace PyClingo {

namespace {

using Value = std::variant<Symbol, int, double>;
using AssignmentIterator = py::typing::Iterator<std::pair<Symbol, Value>>;
class TheoryAssignment {
  public:
    TheoryAssignment(clingo_theory_t const &theory, uint32_t thread_id) : theory_{&theory}, thread_id_{thread_id} {
        assert(theory_->assignment_next != nullptr && theory_->assignment_get_value != nullptr);
    }

    auto lookup(Symbol const &symbol) -> std::optional<size_t> {
        bool found = false;
        size_t index = 0;
        handle_error(theory_->lookup_symbol(theory_->self, *c_cast(&symbol), &index, &found));
        return found ? std::optional{index} : std::nullopt;
    }

    auto at(size_t index) -> std::pair<Symbol, Value> {
        clingo_theory_value_t value;
        clingo_symbol_t symbol = 0;
        bool has_value = true;
        handle_error(theory_->assignment_get_value(theory_->self, thread_id_, index, &symbol, &value, &has_value));
        if (!has_value) {
            throw std::out_of_range{"invalid index"};
        }
        switch (static_cast<clingo_theory_value_type_e>(value.type)) {
            case clingo_theory_value_type_int: {
                // NOLINTNEXTLINE
                return {Symbol{symbol, false}, value.int_number};
            }
            case clingo_theory_value_type_double: {
                // NOLINTNEXTLINE
                return {Symbol{symbol, false}, value.double_number};
            }
            case clingo_theory_value_type_symbol: {
                // NOLINTNEXTLINE
                return {Symbol{symbol, false}, Symbol{value.symbol, false}};
            }
            default: {
                throw std::logic_error{"invalid type"};
            }
        }
    }

    auto iter() -> TheoryAssignment * {
        init_ = true;
        has_value_ = true;
        index_ = 0;
        return this;
    }
    auto next() -> std::pair<Symbol, Value> {
        if (has_value_) {
            handle_error(theory_->assignment_next(theory_->self, thread_id_, &init_, &index_, &has_value_));
            if (has_value_) {
                return at(index_);
            }
        }
        throw py::stop_iteration{};
    }

  private:
    clingo_theory_t const *theory_;
    uint32_t thread_id_;
    bool init_ = true;
    bool has_value_ = true;
    size_t index_ = 0;
};

} // namespace

class Theory {
  public:
    Theory(Library const &lib, py::object const &ptr) {
        auto cap = ptr.cast<py::capsule>();
        if (std::strcmp(cap.name(), "clingo_theory_create") != 0) {
            throw std::invalid_argument("clingo_theory_t pointer expected");
        }
        // NOLINTNEXTLINE
        auto create = reinterpret_cast<bool (*)(clingo_lib_t *, clingo_theory_t *)>(cap.get_pointer());
        if (create == nullptr) {
            throw std::invalid_argument{"create function must not be null"};
        }
        handle_error(create(lib, &theory_));
    }

    ~Theory() {
        if (theory_.destroy != nullptr && theory_.self != nullptr) {
            theory_.destroy(theory_.self);
        }
    }

    [[nodiscard]] auto version() const {
        if (theory_.info == nullptr) {
            PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
            throw py::error_already_set();
        }
        int major = 0;
        int minor = 0;
        int revision = 0;
        handle_error(theory_.info(theory_.self, nullptr, &major, &minor, &revision, nullptr));
        return std::make_tuple(major, minor, revision);
    }

    [[nodiscard]] auto name() const -> std::string_view {
        if (theory_.info == nullptr) {
            PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
            throw py::error_already_set();
        }
        clingo_string_t name;
        handle_error(theory_.info(theory_.self, &name, nullptr, nullptr, nullptr, nullptr));
        return {name.data, name.size};
    }

    [[nodiscard]] auto commit_hash() const -> std::string_view {
        if (theory_.info == nullptr) {
            PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
            throw py::error_already_set();
        }
        clingo_string_t hash;
        handle_error(theory_.info(theory_.self, nullptr, nullptr, nullptr, nullptr, &hash));
        return {hash.data, hash.size};
    }

    void register_theory(Control &ctl) const {
        if (theory_.register_theory != nullptr) {
            handle_error(theory_.register_theory(theory_.self, ctl.c_ptr()));
        }
    }

    void prepare(Control &ctl) const {
        if (theory_.prepare != nullptr) {
            handle_error(theory_.prepare(theory_.self, ctl.c_ptr()));
        }
    }

    void register_options(TypeHint<"clingo.app.AppOptions"> const &opts) const {
        if (theory_.register_options != nullptr) {
            handle_error(theory_.register_options(theory_.self, convert_options(opts)));
        }
    }

    void validate_options() const {
        if (theory_.validate_options != nullptr) {
            handle_error(theory_.validate_options(theory_.self));
        }
    }

    void on_model(Model &model) const {
        if (theory_.on_model != nullptr) {
            // NOLINTNEXTLINE
            handle_error(theory_.on_model(theory_.self, const_cast<clingo_model_t *>(model.c_ptr())));
        }
    }

    void on_stats(Stats &step, [[maybe_unused]] Stats &accu) const {
        // NOTE: the accu and steps roots can be obtained from the stats object
        // in C. Both objects contain the same base C pointer.
        if (theory_.on_stats != nullptr) {
            auto *type = py_stats_type_ref();
            if (type == nullptr) {
                throw py::error_already_set();
            }
            clingo_stats_t *stats = nullptr;
            if (!py_stats_get(type, step.ptr(), &stats, nullptr)) {
                throw py::error_already_set();
            }
            handle_error(theory_.on_stats(theory_.self, stats));
        }
    }

    [[nodiscard]] auto value(uint32_t thread_id, Symbol &symbol) const -> std::optional<Value> {
        if (theory_.lookup_symbol != nullptr && theory_.assignment_get_value != nullptr) {
            size_t index = 0;
            bool found = false;
            handle_error(theory_.lookup_symbol(theory_.self, symbol.handle(), &index, &found));
            if (!found) {
                return std::nullopt;
            }
            clingo_theory_value_t value;
            handle_error(theory_.assignment_get_value(theory_.self, thread_id, index, nullptr, &value, &found));
            if (!found) {
                return std::nullopt;
            }
            // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
            switch (static_cast<clingo_theory_value_type_e>(value.type)) {
                case clingo_theory_value_type_int: {
                    return value.int_number;
                }
                case clingo_theory_value_type_double: {
                    return value.double_number;
                }
                case clingo_theory_value_type_symbol: {
                    return Symbol{value.symbol, true};
                }
            }
            // NOLINTEND(cppcoreguidelines-pro-type-union-access)
        }
        return std::nullopt;
    }

    [[nodiscard]] auto has_assignment() const -> bool {
        return theory_.assignment_next != nullptr && theory_.assignment_get_value != nullptr;
    }

    [[nodiscard]] auto assignment(uint32_t thread_id) const -> TheoryAssignment {
        if (!has_assignment()) {
            PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
            throw py::error_already_set();
        }
        return TheoryAssignment{theory_, thread_id};
    }

    using PyStatement = TypeHint<
        "clingo.ast.StatementRule | clingo.ast.StatementTheory | clingo.ast.StatementOptimize | "
        "clingo.ast.StatementWeakConstraint | clingo.ast.StatementShow | clingo.ast.StatementShowNothing | "
        "clingo.ast.StatementShowSignature | clingo.ast.StatementProject | clingo.ast.StatementProjectSignature | "
        "clingo.ast.StatementDefined | clingo.ast.StatementExternal | clingo.ast.StatementEdge | "
        "clingo.ast.StatementHeuristic | clingo.ast.StatementScript | clingo.ast.StatementInclude | "
        "clingo.ast.StatementProgram | clingo.ast.StatementConst | clingo.ast.StatementComment">;

    void rewrite(PyStatement const &stm, std::function<void(PyStatement const &)> fun) const {
        if (theory_.rewrite_ast != nullptr) {
            handle_error(theory_.rewrite_ast(
                theory_.self, AST::convert_stm(stm),
                [](clingo_ast *stm, void *data) -> bool {
                    CLINGO_TRY {
                        auto &fun = *static_cast<std::function<void(py::handle)> *>(data);
                        fun(AST::convert_stm(stm));
                    }
                    CLINGO_CATCH;
                },
                &fun));
        } else {
            fun(stm);
        }
    }
    void rewrite_string(Library &lib, Control &ctl, std::string_view str) const {
        struct Data {
            Data(clingo_lib_t *lib, clingo_theory_t const *thy) : thy{thy} {
                handle_error(clingo_program_new(lib, &prg));
            }
            Data(Data &&other) = delete;
            ~Data() { clingo_program_free(prg); }
            clingo_program_t *prg = nullptr;
            clingo_theory_t const *thy;
        } data{lib, &theory_};
        handle_error(clingo_ast_parse_string(
            lib, str.data(), str.size(), ctl.c_ptr(),
            [](clingo_ast_t *ast, void *data) {
                auto &[prg, theory] = *static_cast<Data *>(data);
                return theory->rewrite_ast(
                    theory->self, ast,
                    [](clingo_ast_t *ast, void *data) {
                        return clingo_program_add(static_cast<clingo_program_t *>(data), ast);
                    },
                    prg);
            },
            &data));
        handle_error(clingo_control_join(ctl.c_ptr(), data.prg));
    }

    void rewrite_files(Library &lib, Control &ctl, std::span<std::string> files) const {
        struct Data {
            Data(clingo_lib_t *lib, clingo_theory_t const *thy) : thy{thy} {
                handle_error(clingo_program_new(lib, &prg));
            }
            Data(Data &&other) = delete;
            ~Data() { clingo_program_free(prg); }
            clingo_program_t *prg = nullptr;
            clingo_theory_t const *thy;
        } data{lib, &theory_};
        auto cfiles = transform_vec(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        handle_error(clingo_ast_parse_files(
            lib, cfiles.data(), cfiles.size(), ctl.c_ptr(),
            [](clingo_ast_t *ast, void *data) {
                auto &[prg, theory] = *static_cast<Data *>(data);
                return theory->rewrite_ast(
                    theory->self, ast,
                    [](clingo_ast_t *ast, void *data) {
                        return clingo_program_add(static_cast<clingo_program_t *>(data), ast);
                    },
                    prg);
            },
            &data));
        handle_error(clingo_control_join(ctl.c_ptr(), data.prg));
    }

  private:
    clingo_theory_t theory_{};
};

void register_theory(pybind11::module &m) {
    using namespace PyClingo;

    auto theory = m.def_submodule("theory", R"(
This module allows for using theories implemented in C from Python.
)"_d);

    py::class_<TheoryAssignment>(theory, "TheoryAssignment", "Assignment of theory values.")
        .def("lookup", &TheoryAssignment::lookup, py::arg("symbol"), R"(
Get the value index of the symbol in the assignment.

Args:
    symbol: The symbol to lookup.
Returns:
    The value or None if unnassigned.
)"_d)
        .def("at", &TheoryAssignment::at, py::arg("index"), R"(
Get the value at the given index in the assignment.

Args:
    index: The index of the value
Returns:
    The value.
)"_d)
        .def("__iter__", &TheoryAssignment::iter, "Return self.")
        .def("__next__", &TheoryAssignment::next, "Get the next symbol value pair.");

    py::class_<Theory>(theory, "Theory", R"(
Object to call functions from a C-library implementing a custom theory.
)"_d)
        .def(py::init<Library const &, py::object>(), py::arg("library"), py::arg("create"), R"(
Construct a theory object.

Args:
    library: Library object to store symbols in.
    create:
        A capsule object holding a function pointer to initialize the theory.
)"_d)
        .def("register", &Theory::register_theory, py::arg("control"), R"(
Register the theory with the given control object.

This function should be called once on the control object before grounding and
solving starts.

Args:
    control: The control object to register the theory with.
)"_d)
        .def("rewrite", &Theory::rewrite, py::arg("statement"), py::arg("callback"), R"(
Rewrite the given statement and pass the result to the callback.

Some theories require rewriting prior to adding a non-ground program to a
control object.

Args:
    statement: The statement to rewrite.
    callback: The callback receiving rewritten statements.
)"_d)
        .def("rewrite_string", &Theory::rewrite_string, py::arg("lib"), py::arg("control"), py::arg("program"), R"(
Rewrite the given program adding it to the control object.

Args:
    lib: The library to store symbols.
    control: The target control..
    program: The program to parse.
)"_d)
        .def("rewrite_files", &Theory::rewrite_files, py::arg("lib"), py::arg("control"), py::arg("files"), R"(
Rewrite the program in the given files and add it to the control.

Args:
    lib: The library to store symbols.
    control: The target control..
    files: The files to parse.
)"_d)
        .def("register_options", &Theory::register_options, py::arg("options"), R"(
Register theory related options.

Args:
    options: The application options.

See also: `clingo.app.App.register_options`
)"_d)
        .def("validate_options", &Theory::validate_options, R"(
Check the registered options.

See also: `clingo.app.App.validate_options`
)"_d)
        .def("prepare", &Theory::prepare, py::arg("control"), R"(
Prepare the theory for solving.

Args:
    control: The control object using for solving.
)"_d)
        .def("on_model", &Theory::on_model, py::arg("model"), R"(
Notify the theory about the given model.

Some theories extend the model here are set their internal assignments. This
function should be called in the on_model callback of a control's solve
function.

Args:
    model: The current model.
)"_d)
        .def("on_stats", &Theory::on_stats, py::arg("step"), py::arg("accu"), R"(
Let the theory update statistics.

Some theories extend the statistics here.

Args:
    step: The per step statistics.
    accu: The accumulated statistics.
)"_d)
        .def("value", &Theory::value, py::arg("thread_id"), py::arg("symbol"), R"(
Get the value of the symbol in the assignment of the given thread.

It depends on the theory when this function can be called. Generally, it can be
called after `clingo.theory.Theory.on_model` while the solver is still holding
its current model.

Args:
    thread_id: The id of the thread to query.
    symbol: The symbol to lookup.

Returns:
    The value or None if unnassigned.
)"_d)
        .def("assignment", &Theory::assignment, py::arg("thread_id"), R"(
Get the symbols and values currently assigned by the theory

It depends on the theory when this function can be called. Generally, it can be
called after `clingo.theory.Theory.on_model` while the solver is still holding
its current model.

Args:
    thread_id: The id of the thread to query.

Returns:
    An interable over symbol value pairs.
)"_d)
        .def_property_readonly("version", &Theory::version, "Get the version of the theory (major, minor, revision).")
        .def_property_readonly("name", &Theory::name, "Get the name of the theory.")
        .def_property_readonly("commit_hash", &Theory::commit_hash, "Get the commit hash of the theory.")
        .def_property_readonly("has_assignment", &Theory::has_assignment,
                               "Check whether the theory supports symbol value assigments.");
}

} // namespace PyClingo
*/

#include "app.hh"
#include "ast.hh"
#include "control.hh"
#include "solve.hh"
#include "stats.hh"
#include "symbol.hh"
#include "util.hh"

#include <clingo/theory.h>

#include <array>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace PyClingo {

namespace {

using PyStatement =
    TypeHint<"clingo.ast.StatementRule | clingo.ast.StatementTheory | clingo.ast.StatementOptimize | "
             "clingo.ast.StatementWeakConstraint | clingo.ast.StatementShow | clingo.ast.StatementShowNothing | "
             "clingo.ast.StatementShowSignature | clingo.ast.StatementProject | clingo.ast.StatementProjectSignature | "
             "clingo.ast.StatementDefined | clingo.ast.StatementExternal | clingo.ast.StatementEdge | "
             "clingo.ast.StatementHeuristic | clingo.ast.StatementScript | clingo.ast.StatementInclude | "
             "clingo.ast.StatementProgram | clingo.ast.StatementConst | clingo.ast.StatementComment">;

using PyTheoryObject = struct {
    PyObject_HEAD clingo_theory_t theory;
};

using PyTheoryAssignmentObject = struct {
    PyObject_HEAD PyObject *owner;
    uint32_t thread_id;
    bool init;
    bool has_value;
    size_t index;
};

struct TheoryModuleState {
    PyObject *symbol_type{nullptr};
    PyObject *theory_type{nullptr};
    PyObject *assignment_type{nullptr};
};

auto get_state(PyObject *module) -> TheoryModuleState * {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "theory module state is unavailable");
        return nullptr;
    }
    return state;
}

auto get_state_from_self(PyObject *self) -> TheoryModuleState * {
    auto *mod = PyType_GetModule(Py_TYPE(self));
    if (mod == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "failed to resolve theory module");
        return nullptr;
    }
    return get_state(mod);
}

auto py_theory_cast(PyObject *obj) -> PyTheoryObject * {
    return reinterpret_cast<PyTheoryObject *>(obj);
}

auto py_theory_assignment_cast(PyObject *obj) -> PyTheoryAssignmentObject * {
    return reinterpret_cast<PyTheoryAssignmentObject *>(obj);
}

template <std::integral T> auto parse_num(PyObject *obj, T *out, char const *what) -> bool {
    if constexpr (std::is_unsigned_v<T>) {
        auto i = steal(PyNumber_Index(obj));
        if (i.get() == nullptr) {
            return false;
        }
        auto v = PyLong_AsUnsignedLong(i.get());
        if (PyErr_Occurred() != nullptr) {
            PyErr_Format(PyExc_TypeError, "%s must be a non-negative integer", what);
            return false;
        }
        if (std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    } else {
        auto v = PyLong_AsLong(obj);
        if (PyErr_Occurred() != nullptr) {
            PyErr_Format(PyExc_TypeError, "%s must be an integer", what);
            return false;
        }
        if (std::cmp_less(v, std::numeric_limits<T>::min()) || std::cmp_greater(v, std::numeric_limits<T>::max())) {
            PyErr_Format(PyExc_OverflowError, "%s is out of range", what);
            return false;
        }
        *out = static_cast<T>(v);
    }
    return true;
}

auto parse_symbol(TheoryModuleState *state, PyObject *obj, clingo_symbol_t *sym, char const *what) -> bool {
    if (state == nullptr) {
        return false;
    }
    auto res = py_symbol_check(state->symbol_type, obj, sym);
    if (res < 0) {
        return false;
    }
    if (res == 0) {
        PyErr_Format(PyExc_TypeError, "%s must be a Symbol", what);
        return false;
    }
    return true;
}

auto parse_string_sequence(PyObject *obj, std::vector<std::string> *out) -> bool {
    auto seq = steal(PySequence_Fast(obj, "files must be a sequence of strings"));
    if (seq.get() == nullptr) {
        return false;
    }
    auto n = PySequence_Fast_GET_SIZE(seq.get());
    if (n < 0) {
        return false;
    }

    out->clear();
    out->reserve(static_cast<size_t>(n));

    auto **items = PySequence_Fast_ITEMS(seq.get());
    for (Py_ssize_t i = 0; i < n; ++i) {
        if (PyUnicode_Check(items[i]) == 0) {
            PyErr_SetString(PyExc_TypeError, "files must be a sequence of strings");
            return false;
        }
        Py_ssize_t len = 0;
        const auto *s = PyUnicode_AsUTF8AndSize(items[i], &len);
        if (s == nullptr) {
            return false;
        }
        out->emplace_back(s, static_cast<size_t>(len));
    }
    return true;
}

auto theory_is_ready(PyTheoryObject *o) -> bool {
    if (o->theory.self == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "theory is not initialized");
        return false;
    }
    return true;
}

auto py_value_to_object(clingo_theory_value_t const &value) -> PyObject * {
    switch (static_cast<clingo_theory_value_type_e>(value.type)) {
        case clingo_theory_value_type_int: {
            return PyLong_FromLong(value.int_number);
        }
        case clingo_theory_value_type_double: {
            return PyFloat_FromDouble(value.double_number);
        }
        case clingo_theory_value_type_symbol: {
            try {
                auto sym = py::cast(Symbol{value.symbol, false});
                return sym.release().ptr();
            } catch (py::error_already_set &e) {
                e.restore();
                return nullptr;
            }
        }
    }
    PyErr_SetString(PyExc_RuntimeError, "invalid theory value type");
    return nullptr;
}

auto py_symbol_value_pair(clingo_symbol_t symbol, clingo_theory_value_t const &value) -> PyObject * {
    try {
        auto sym = py::cast(Symbol{symbol, false});
        auto *sym_obj = sym.release().ptr();
        if (sym_obj == nullptr) {
            return nullptr;
        }
        auto *val_obj = py_value_to_object(value);
        if (val_obj == nullptr) {
            Py_DECREF(sym_obj);
            return nullptr;
        }
        auto *ret = PyTuple_New(2);
        if (ret == nullptr) {
            Py_DECREF(sym_obj);
            Py_DECREF(val_obj);
            return nullptr;
        }
        PyTuple_SET_ITEM(ret, 0, sym_obj);
        PyTuple_SET_ITEM(ret, 1, val_obj);
        return ret;
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }
}

auto py_theory_assignment_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    auto *a = py_theory_assignment_cast(self);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(a->owner);
    return 0;
}

auto py_theory_assignment_clear(PyObject *self) -> int {
    auto *a = py_theory_assignment_cast(self);
    Py_CLEAR(a->owner);
    return 0;
}

void py_theory_assignment_dealloc(PyObject *self) {
    PyObject_GC_UnTrack(self);
    py_theory_assignment_clear(self);
    Py_TYPE(self)->tp_free(self);
}

auto py_theory_assignment_new(PyObject *type, PyObject *owner, uint32_t thread_id) -> PyObject * {
    auto *to = reinterpret_cast<PyTypeObject *>(type);
    auto *obj = to->tp_alloc(to, 0);
    if (obj == nullptr) {
        return nullptr;
    }
    auto *a = py_theory_assignment_cast(obj);
    a->owner = owner;
    a->thread_id = thread_id;
    a->init = true;
    a->has_value = true;
    a->index = 0;
    Py_XINCREF(owner);
    return obj;
}

auto py_theory_assignment_lookup(PyObject *self, PyObject *arg) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }
    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }
    auto *o = py_theory_cast(a->owner);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    clingo_symbol_t sym = 0;
    if (!parse_symbol(state, arg, &sym, "symbol")) {
        return nullptr;
    }

    if (o->theory.lookup_symbol == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "lookup_symbol not implemented");
        return nullptr;
    }

    size_t index = 0;
    bool found = false;
    if (!o->theory.lookup_symbol(o->theory.self, sym, &index, &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }
    return PyLong_FromSize_t(index);
}

auto py_theory_assignment_at(PyObject *self, PyObject *arg) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }
    auto *o = py_theory_cast(a->owner);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    size_t index = 0;
    if (!parse_num(arg, &index, "index")) {
        return nullptr;
    }

    if (o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment_get_value not implemented");
        return nullptr;
    }

    clingo_symbol_t symbol = 0;
    clingo_theory_value_t value{};
    bool has_value = true;
    if (!o->theory.assignment_get_value(o->theory.self, a->thread_id, index, &symbol, &value, &has_value)) {
        return py_handle_error();
    }
    if (!has_value) {
        PyErr_SetString(PyExc_IndexError, "invalid index");
        return nullptr;
    }
    return py_symbol_value_pair(symbol, value);
}

auto py_theory_assignment_iter(PyObject *self) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    a->init = true;
    a->has_value = true;
    a->index = 0;
    Py_INCREF(self);
    return self;
}

auto py_theory_assignment_next(PyObject *self) -> PyObject * {
    auto *a = py_theory_assignment_cast(self);
    if (a->owner == nullptr) {
        PyErr_SetString(PyExc_RuntimeError, "assignment is invalid");
        return nullptr;
    }
    auto *o = py_theory_cast(a->owner);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.assignment_next == nullptr || o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment not implemented");
        return nullptr;
    }

    if (a->has_value) {
        if (!o->theory.assignment_next(o->theory.self, a->thread_id, &a->init, &a->index, &a->has_value)) {
            return py_handle_error();
        }
        if (a->has_value) {
            clingo_symbol_t symbol = 0;
            clingo_theory_value_t value{};
            bool has_value = true;
            if (!o->theory.assignment_get_value(o->theory.self, a->thread_id, a->index, &symbol, &value, &has_value)) {
                return py_handle_error();
            }
            if (!has_value) {
                PyErr_SetString(PyExc_StopIteration, "");
                return nullptr;
            }
            return py_symbol_value_pair(symbol, value);
        }
    }

    PyErr_SetNone(PyExc_StopIteration);
    return nullptr;
}

auto py_theory_init(PyObject *self, PyObject *args, PyObject *kwargs) -> int {
    static constexpr auto kwlist = std::array<char const *, 3>{"library", "create", nullptr};
    PyObject *lib_obj = nullptr;
    PyObject *create_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:Theory", cast_kw(kwlist), &lib_obj, &create_obj) == 0) {
        return -1;
    }

    auto *o = py_theory_cast(self);
    std::memset(&o->theory, 0, sizeof(o->theory));

    if (!PyCapsule_CheckExact(create_obj)) {
        PyErr_SetString(PyExc_TypeError, "create must be a capsule");
        return -1;
    }
    const auto *cap_name = PyCapsule_GetName(create_obj);
    if (cap_name == nullptr) {
        return -1;
    }
    if (std::strcmp(cap_name, "clingo_theory_create") != 0) {
        PyErr_SetString(PyExc_ValueError, "clingo_theory_t pointer expected");
        return -1;
    }
    auto *ptr = PyCapsule_GetPointer(create_obj, "clingo_theory_create");
    if (ptr == nullptr) {
        return -1;
    }

    auto create = reinterpret_cast<bool (*)(clingo_lib_t *, clingo_theory_t *)>(ptr);
    if (create == nullptr) {
        PyErr_SetString(PyExc_ValueError, "create function must not be null");
        return -1;
    }

    try {
        auto const *lib = py::reinterpret_borrow<py::object>(lib_obj).cast<Library const *>();
        if (!create(*lib, &o->theory)) {
            py_handle_error();
            return -1;
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return -1;
    }

    return 0;
}

auto py_theory_traverse(PyObject *self, visitproc visit, void *arg) -> int {
    Py_VISIT(Py_TYPE(self));
    return 0;
}

auto py_theory_clear([[maybe_unused]] PyObject *self) -> int {
    return 0;
}

void py_theory_dealloc(PyObject *self) {
    PyObject_GC_UnTrack(self);
    auto *o = py_theory_cast(self);
    if (o->theory.destroy != nullptr && o->theory.self != nullptr) {
        o->theory.destroy(o->theory.self);
    }
    std::memset(&o->theory, 0, sizeof(o->theory));
    py_theory_clear(self);
    Py_TYPE(self)->tp_free(self);
}

auto py_theory_register(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.register_theory == nullptr) {
        Py_RETURN_NONE;
    }

    try {
        auto *ctl = py::reinterpret_borrow<py::object>(arg).cast<Control *>();
        if (!o->theory.register_theory(o->theory.self, ctl->c_ptr())) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

struct RewriteData {
    PyObject *callback;
};

auto rewrite_ast_cb(clingo_ast_t *ast, void *data) -> bool {
    auto *d = static_cast<RewriteData *>(data);
    try {
        auto py_stm = AST::convert_stm(ast);
        auto ret = steal(PyObject_CallFunctionObjArgs(d->callback, py_stm.ptr(), nullptr));
        if (ret.get() == nullptr) {
            return store_python_error("rewrite callback failed");
        }
        return true;
    } catch (py::error_already_set &e) {
        e.restore();
        return store_python_error("rewrite callback failed");
    }
}

auto py_theory_rewrite(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    PyObject *stm_obj = nullptr;
    PyObject *callback = nullptr;
    if (PyArg_ParseTuple(args, "OO:rewrite", &stm_obj, &callback) == 0) {
        return nullptr;
    }
    if (PyCallable_Check(callback) == 0) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return nullptr;
    }

    if (o->theory.rewrite_ast == nullptr) {
        auto ret = steal(PyObject_CallFunctionObjArgs(callback, stm_obj, nullptr));
        if (ret.get() == nullptr) {
            return nullptr;
        }
        Py_RETURN_NONE;
    }

    RewriteData data{callback};
    try {
        auto stm = py::reinterpret_borrow<py::object>(stm_obj).cast<PyStatement>();
        if (!o->theory.rewrite_ast(o->theory.self, AST::convert_stm(stm), rewrite_ast_cb, &data)) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

struct ParseData {
    clingo_program_t *prg;
    clingo_theory_t const *theory;
};

auto parse_rewrite_cb(clingo_ast_t *ast, void *data) -> bool {
    auto *d = static_cast<ParseData *>(data);
    if (d->theory->rewrite_ast == nullptr) {
        return clingo_program_add(d->prg, ast);
    }
    return d->theory->rewrite_ast(
        d->theory->self, ast,
        [](clingo_ast_t *rewritten, void *dst) -> bool {
            return clingo_program_add(static_cast<clingo_program_t *>(dst), rewritten);
        },
        d->prg);
}

auto py_theory_rewrite_string(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    static constexpr auto kwlist = std::array<char const *, 4>{"lib", "control", "program", nullptr};
    PyObject *lib_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    char const *text = nullptr;
    Py_ssize_t n = 0;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOs#:rewrite_string", cast_kw(kwlist), &lib_obj, &ctl_obj, &text,
                                    &n) == 0) {
        return nullptr;
    }

    try {
        auto &lib = py::reinterpret_borrow<py::object>(lib_obj).cast<Library &>();
        auto &ctl = py::reinterpret_borrow<py::object>(ctl_obj).cast<Control &>();

        clingo_program_t *prg = nullptr;
        if (!clingo_program_new(lib, &prg)) {
            return py_handle_error();
        }
        auto guard = Guard<clingo_program_t *, clingo_program_free>(prg);

        ParseData data{prg, &o->theory};
        if (!clingo_ast_parse_string(lib, text, static_cast<size_t>(n), ctl.c_ptr(), parse_rewrite_cb, &data)) {
            return py_handle_error();
        }
        if (!clingo_control_join(ctl.c_ptr(), prg)) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_theory_rewrite_files(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    static constexpr auto kwlist = std::array<char const *, 4>{"lib", "control", "files", nullptr};
    PyObject *lib_obj = nullptr;
    PyObject *ctl_obj = nullptr;
    PyObject *files_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO:rewrite_files", cast_kw(kwlist), &lib_obj, &ctl_obj,
                                    &files_obj) == 0) {
        return nullptr;
    }

    std::vector<std::string> files;
    if (!parse_string_sequence(files_obj, &files)) {
        return nullptr;
    }

    try {
        auto &lib = py::reinterpret_borrow<py::object>(lib_obj).cast<Library &>();
        auto &ctl = py::reinterpret_borrow<py::object>(ctl_obj).cast<Control &>();

        clingo_program_t *prg = nullptr;
        if (!clingo_program_new(lib, &prg)) {
            return py_handle_error();
        }
        auto guard = Guard<clingo_program_t *, clingo_program_free>(prg);

        auto cfiles = transform_vec(files, [](auto const &x) { return clingo_string_t{x.data(), x.size()}; });
        ParseData data{prg, &o->theory};
        if (!clingo_ast_parse_files(lib, cfiles.data(), cfiles.size(), ctl.c_ptr(), parse_rewrite_cb, &data)) {
            return py_handle_error();
        }
        if (!clingo_control_join(ctl.c_ptr(), prg)) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_theory_register_options(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.register_options == nullptr) {
        Py_RETURN_NONE;
    }

    try {
        auto opts = py::reinterpret_borrow<py::object>(arg).cast<TypeHint<"clingo.app.AppOptions">>();
        if (!o->theory.register_options(o->theory.self, convert_options(opts))) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_theory_validate_options(PyObject *self, [[maybe_unused]] PyObject *args) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.validate_options != nullptr) {
        if (!o->theory.validate_options(o->theory.self)) {
            return py_handle_error();
        }
    }
    Py_RETURN_NONE;
}

auto py_theory_prepare(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.prepare == nullptr) {
        Py_RETURN_NONE;
    }

    try {
        auto *ctl = py::reinterpret_borrow<py::object>(arg).cast<Control *>();
        if (!o->theory.prepare(o->theory.self, ctl->c_ptr())) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_theory_on_model(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.on_model == nullptr) {
        Py_RETURN_NONE;
    }

    try {
        auto model = py::reinterpret_borrow<py::object>(arg).cast<Model &>();
        if (!o->theory.on_model(o->theory.self, const_cast<clingo_model_t *>(model.c_ptr()))) {
            return py_handle_error();
        }
    } catch (py::error_already_set &e) {
        e.restore();
        return nullptr;
    }

    Py_RETURN_NONE;
}

auto py_theory_on_stats(PyObject *self, PyObject *args) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.on_stats == nullptr) {
        Py_RETURN_NONE;
    }

    PyObject *step = nullptr;
    PyObject *accu = nullptr;
    if (PyArg_ParseTuple(args, "OO:on_stats", &step, &accu) == 0) {
        return nullptr;
    }
    (void)accu;

    auto *type = py_stats_type_ref();
    if (type == nullptr) {
        return nullptr;
    }

    clingo_stats_t *stats = nullptr;
    if (!py_stats_get(type, step, &stats, nullptr)) {
        return nullptr;
    }

    if (!o->theory.on_stats(o->theory.self, stats)) {
        return py_handle_error();
    }
    Py_RETURN_NONE;
}

auto py_theory_value(PyObject *self, PyObject *args, PyObject *kwargs) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    static constexpr auto kwlist = std::array<char const *, 3>{"thread_id", "symbol", nullptr};
    PyObject *thread_id_obj = nullptr;
    PyObject *symbol_obj = nullptr;
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "OO:value", cast_kw(kwlist), &thread_id_obj, &symbol_obj) == 0) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    uint32_t thread_id = 0;
    if (!parse_num(thread_id_obj, &thread_id, "thread_id")) {
        return nullptr;
    }

    clingo_symbol_t symbol = 0;
    if (!parse_symbol(state, symbol_obj, &symbol, "symbol")) {
        return nullptr;
    }

    if (o->theory.lookup_symbol == nullptr || o->theory.assignment_get_value == nullptr) {
        Py_RETURN_NONE;
    }

    size_t index = 0;
    bool found = false;
    if (!o->theory.lookup_symbol(o->theory.self, symbol, &index, &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }

    clingo_theory_value_t value{};
    if (!o->theory.assignment_get_value(o->theory.self, thread_id, index, nullptr, &value, &found)) {
        return py_handle_error();
    }
    if (!found) {
        Py_RETURN_NONE;
    }

    return py_value_to_object(value);
}

auto py_theory_assignment(PyObject *self, PyObject *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }

    auto *state = get_state_from_self(self);
    if (state == nullptr) {
        return nullptr;
    }

    uint32_t thread_id = 0;
    if (!parse_num(arg, &thread_id, "thread_id")) {
        return nullptr;
    }

    if (o->theory.assignment_next == nullptr || o->theory.assignment_get_value == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "assignment not implemented");
        return nullptr;
    }

    return py_theory_assignment_new(state->assignment_type, self, thread_id);
}

auto py_theory_get_version(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    int major = 0;
    int minor = 0;
    int revision = 0;
    if (!o->theory.info(o->theory.self, nullptr, &major, &minor, &revision, nullptr)) {
        return py_handle_error();
    }
    return Py_BuildValue("(iii)", major, minor, revision);
}

auto py_theory_get_name(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    clingo_string_t name;
    if (!o->theory.info(o->theory.self, &name, nullptr, nullptr, nullptr, nullptr)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(name.data, static_cast<Py_ssize_t>(name.size));
}

auto py_theory_get_commit_hash(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    if (o->theory.info == nullptr) {
        PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
        return nullptr;
    }

    clingo_string_t hash;
    if (!o->theory.info(o->theory.self, nullptr, nullptr, nullptr, nullptr, &hash)) {
        return py_handle_error();
    }
    return PyUnicode_FromStringAndSize(hash.data, static_cast<Py_ssize_t>(hash.size));
}

auto py_theory_get_has_assignment(PyObject *self, [[maybe_unused]] void *arg) -> PyObject * {
    auto *o = py_theory_cast(self);
    if (!theory_is_ready(o)) {
        return nullptr;
    }
    auto ok = o->theory.assignment_next != nullptr && o->theory.assignment_get_value != nullptr;
    if (ok) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

auto py_theory_assignment_methods = std::array{
    PyMethodDef{"lookup", py_theory_assignment_lookup, METH_O, PyDoc_STR("")},
    PyMethodDef{"at", py_theory_assignment_at, METH_O, PyDoc_STR("")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_theory_assignment_slots = std::array{
    PyType_Slot{Py_tp_doc, PyDoc_STR(const_cast<char *>("Assignment of theory values."))},
    PyType_Slot{Py_tp_methods, py_theory_assignment_methods.data()},
    PyType_Slot{Py_tp_iter, reinterpret_cast<void *>(py_theory_assignment_iter)},
    PyType_Slot{Py_tp_iternext, reinterpret_cast<void *>(py_theory_assignment_next)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_theory_assignment_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_theory_assignment_clear)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_theory_assignment_dealloc)},
    PyType_Slot{0, nullptr},
};

auto py_theory_assignment_spec = PyType_Spec{
    "clingo.theory.TheoryAssignment",
    sizeof(PyTheoryAssignmentObject),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    py_theory_assignment_slots.data(),
};

auto py_theory_methods = std::array{
    PyMethodDef{"register", py_theory_register, METH_O, PyDoc_STR("")},
    PyMethodDef{"rewrite", py_theory_rewrite, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{
        "rewrite_string",
        py_c_function_cast(py_theory_rewrite_string),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{
        "rewrite_files",
        py_c_function_cast(py_theory_rewrite_files),
        METH_VARARGS | METH_KEYWORDS,
        PyDoc_STR(""),
    },
    PyMethodDef{"register_options", py_theory_register_options, METH_O, PyDoc_STR("")},
    PyMethodDef{"validate_options", py_theory_validate_options, METH_NOARGS, PyDoc_STR("")},
    PyMethodDef{"prepare", py_theory_prepare, METH_O, PyDoc_STR("")},
    PyMethodDef{"on_model", py_theory_on_model, METH_O, PyDoc_STR("")},
    PyMethodDef{"on_stats", py_theory_on_stats, METH_VARARGS, PyDoc_STR("")},
    PyMethodDef{"value", py_c_function_cast(py_theory_value), METH_VARARGS | METH_KEYWORDS, PyDoc_STR("")},
    PyMethodDef{"assignment", py_theory_assignment, METH_O, PyDoc_STR("")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

auto py_theory_getset = std::array{
    PyGetSetDef{"version", py_theory_get_version, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"name", py_theory_get_name, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"commit_hash", py_theory_get_commit_hash, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{"has_assignment", py_theory_get_has_assignment, nullptr, PyDoc_STR(""), nullptr},
    PyGetSetDef{nullptr, nullptr, nullptr, nullptr, nullptr},
};

auto py_theory_slots = std::array{
    PyType_Slot{
        Py_tp_doc,
        PyDoc_STR(const_cast<char *>("Object to call functions from a C-library implementing a custom theory.")),
    },
    PyType_Slot{Py_tp_init, reinterpret_cast<void *>(py_theory_init)},
    PyType_Slot{Py_tp_new, reinterpret_cast<void *>(PyType_GenericNew)},
    PyType_Slot{Py_tp_dealloc, reinterpret_cast<void *>(py_theory_dealloc)},
    PyType_Slot{Py_tp_traverse, reinterpret_cast<void *>(py_theory_traverse)},
    PyType_Slot{Py_tp_clear, reinterpret_cast<void *>(py_theory_clear)},
    PyType_Slot{Py_tp_methods, py_theory_methods.data()},
    PyType_Slot{Py_tp_getset, py_theory_getset.data()},
    PyType_Slot{0, nullptr},
};

auto py_theory_spec = PyType_Spec{
    "clingo.theory.Theory", sizeof(PyTheoryObject), 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    py_theory_slots.data(),
};

auto theory_module_traverse(PyObject *module, visitproc visit, void *arg) -> int {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_VISIT(state->symbol_type);
    Py_VISIT(state->theory_type);
    Py_VISIT(state->assignment_type);
    return 0;
}

auto theory_module_clear(PyObject *module) -> int {
    auto *state = static_cast<TheoryModuleState *>(PyModule_GetState(module));
    if (state == nullptr) {
        return 0;
    }
    Py_CLEAR(state->symbol_type);
    Py_CLEAR(state->theory_type);
    Py_CLEAR(state->assignment_type);
    return 0;
}

void theory_module_free(void *module) {
    theory_module_clear(static_cast<PyObject *>(module));
}

PyModuleDef theory_def = {
    PyModuleDef_HEAD_INIT,
    "clingo.theory",
    PyDoc_STR("This module allows for using theories implemented in C from Python."),
    static_cast<Py_ssize_t>(sizeof(TheoryModuleState)),
    nullptr,
    nullptr,
    theory_module_traverse,
    theory_module_clear,
    theory_module_free,
};

} // namespace

void register_theory(pybind11::module &m) {
    auto theory_mod = PyGuard{nullptr};
    if (!add_submodule(m.ptr(), &theory_def, &theory_mod.get())) {
        throw py::error_already_set();
    }

    auto *state = get_state(theory_mod.get());
    if (state == nullptr) {
        throw py::error_already_set();
    }

    state->symbol_type = py_symbol_type_ref();
    if (state->symbol_type == nullptr) {
        throw py::error_already_set();
    }
    Py_INCREF(state->symbol_type);

    if (!add_type(theory_mod.get(), "TheoryAssignment", &py_theory_assignment_spec, &state->assignment_type) ||
        !add_type(theory_mod.get(), "Theory", &py_theory_spec, &state->theory_type)) {
        throw py::error_already_set();
    }
}

} // namespace PyClingo
