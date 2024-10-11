#include "ast2.hh"
#include "ast_node.hh"

namespace Clingo::AST2 {

namespace py = pybind11;

using TermVariable = Node<clingo_ast_type_term_variable>;
using Term = std::variant<TermVariable>;

template <> struct ast_type_info<Core::Location> {
    static constexpr auto type = Type::location;
    using c_type = clingo_location_t const *;
};

template <> struct ast_type_info<char const *> {
    static constexpr auto type = Type::string;
    using c_type = char const *;
};

template <> struct ast_type_info<bool> {
    static constexpr auto type = Type::number;
    using c_type = int;
};

template <> struct ast_type_info<Term> {
    static constexpr auto type = Type::variant;
    using c_type = clingo_ast_type_t const *;
};

template <> struct ast_type_info<TermVariable> {
    static constexpr auto type = Type::record;
    using c_type = clingo_ast_type_t const *;
    using arguments = std::tuple<Core::Location, char const *, bool>;
    static constexpr auto doc = R"doc(Construct a TermVariable object.

Parameters
----------
lib
    The library object for storing symbols.
location
    The location of the variable.
name
    The name of the variable.
anonymous
    Whether the variable is anonymous.

    Anonymous variables receive a unique name during preprocessing.)doc";
    // NOLINTNEXTLINE
    static constexpr clingo_ast_attribute_e names[] = {clingo_ast_attribute_location, clingo_ast_attribute_name,
                                                       clingo_ast_attribute_anonymous};
    // NOLINTNEXTLINE
    static constexpr char const *strings[] = {"location", "name", "anonymous"};
    // NOLINTNEXTLINE
    static constexpr char const *docs[] = {R"doc(The location of the variable.)doc",
                                           R"doc(The name of the variable.)doc",
                                           R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc"};
    static constexpr auto ast_type = clingo_ast_type_term_variable;
};

static constexpr auto doc_visit = R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc";

static constexpr auto doc_transform = R"doc(Transform the expression.

Parameters
----------
lib
    The library object for storing symbols.
transformer
    The transformer accepting the sub expressions.
)doc";

static constexpr auto doc_update = R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Parameters
----------
lib
    The library object for storing symbols.
)doc";

void register_module(pybind11::module &m) {

    auto ast = m.def_submodule(
        "ast2", R"doc(This module provides functions to work with Abstract Syntax Trees of logic programs.)doc");

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, R"doc(Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)doc");

    auto py_term_variable = py::class_<TermVariable>(ast, "TermVariable", R"doc(A term representing a variable.)doc");

    py_term_variable
        .def(py::init<Core::Library &, Core::Location const &, char const *, bool>(), py::arg("lib"),
             py::arg("location"), py::arg("name"), py::arg("anonymous") = false, TermVariable::init_doc())
        .def("__str__", &TermVariable::to_string)
        .def("__hash__", &TermVariable::hash)
        .def_property_readonly(TermVariable::attr_name<0>(), &TermVariable::get<0>, TermVariable::attr_doc<0>())
        .def_property_readonly(TermVariable::attr_name<1>(), &TermVariable::get<1>, TermVariable::attr_doc<1>())
        .def_property_readonly(TermVariable::attr_name<2>(), &TermVariable::get<2>, TermVariable::attr_doc<2>())
        .def("visit", &TermVariable::visit, py::arg("visitor"), doc_visit)
        .def("transform", &TermVariable::transform, doc_transform)
        .def("update", &TermVariable::update, py::arg("lib"), doc_update);
}

} // namespace Clingo::AST2
