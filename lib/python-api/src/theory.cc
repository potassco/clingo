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
        handle_error(theory_.info(theory_.self, nullptr, &major, &minor, &revision));
        return std::make_tuple(major, minor, revision);
    }

    [[nodiscard]] auto name() const -> std::string_view {
        if (theory_.info == nullptr) {
            PyErr_SetString(PyExc_NotImplementedError, "info not implemented");
            throw py::error_already_set();
        }
        clingo_string_t name;
        handle_error(theory_.info(theory_.self, &name, nullptr, nullptr, nullptr));
        return {name.data, name.size};
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

    void configure(std::string_view key, std::string_view value) const {
        if (theory_.configure != nullptr) {
            handle_error(theory_.configure(theory_.self, key.data(), key.size(), value.data(), value.size()));
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
            handle_error(theory_.on_stats(theory_.self, step.c_ptr()));
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
        .def("configure", &Theory::configure, py::arg("name"), py::arg("value"), R"(
Configure the theory using its name/value interface.

It depends on the theory which keys are supported and when this function can be
called.

Args:
    key: The name of the option.
    value: The value of the option.
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
called after `on_model` while the solver is still holding its current model.

Args:
    thread_id: The id of the thread to query.
    symbol: The symbol to lookup.

Returns:
    The value or None if unnassigned.
)"_d)
        .def("assignment", &Theory::assignment, py::arg("thread_id"), R"(
Get the symbols and values currently assigned by the theory

It depends on the theory when this function can be called. Generally, it can be
called after `on_model` while the solver is still holding its current model.

Args:
    thread_id: The id of the thread to query.

Returns:
    An interable over symbol value pairs.
)"_d)
        .def_property_readonly("version", &Theory::version, "Get the version of the theory (major, minor, revision).")
        .def_property_readonly("name", &Theory::name, "Get the name of the theory.")
        .def_property_readonly("has_assignment", &Theory::has_assignment,
                               "Check whether the theory supports symbol value assigments.");
}

} // namespace PyClingo
