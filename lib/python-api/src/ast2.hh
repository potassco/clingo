#pragma once

#include <clingo.h>

#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {

namespace py = pybind11;

namespace Experiment {

template <clingo_ast_type_e T> class Node;
using TermVariable = Node<clingo_ast_type_term_variable>;
using TermSymbolic = Node<clingo_ast_type_term_symbolic>;
using Term = std::variant<TermVariable, TermSymbolic>;
using RightGuard = Node<clingo_ast_type_right_guard>;

template <class T> struct cpp_attribute_trait;

enum class Type : uint8_t { record, variant, location, string, number, array };

template <> struct cpp_attribute_trait<Core::Location> {
    static constexpr auto type = Type::location;
    using c_type = clingo_location_t const *;
};

template <> struct cpp_attribute_trait<char const *> {
    static constexpr auto type = Type::string;
    using c_type = char const *;
};

template <> struct cpp_attribute_trait<bool> {
    static constexpr auto type = Type::number;
    using c_type = int;
};

template <> struct cpp_attribute_trait<Term> {
    static constexpr auto type = Type::variant;
    using c_type = clingo_ast_type_t const *;
};

template <> struct cpp_attribute_trait<TermVariable> {
    static constexpr auto type = Type::record;
    using c_type = clingo_ast_type_t const *;
    using arguments = std::tuple<Core::Location, char const *>;
    static constexpr auto ast_type = clingo_ast_type_term_variable;
};

template <> struct cpp_attribute_trait<TermSymbolic> {
    static constexpr auto type = Type::record;
    using c_type = clingo_ast_type_t const *;
    using arguments = std::tuple<Core::Location, Symbol::Symbol>;
    static constexpr auto ast_type = clingo_ast_type_term_symbolic;
};

template <> struct cpp_attribute_trait<RightGuard> {
    static constexpr auto type = Type::record;
    using c_type = clingo_ast_type_t const *;
    static constexpr auto ast_type = clingo_ast_type_term_variable;
};

template <clingo_ast_type_e T> class Node {
  public:
    // Note: for pybind
    Node() = default;

    explicit Node(clingo_ast_t *ast) : ast_{ast} {}

    Node(Node const &x) {
        if (!clingo_ast_copy(x.ast_, &ast_)) {
            throw std::runtime_error("could not copy ast");
        }
    }

    // a generic version is implementable by mapping the c++ arguments to their c counter parts
    // arrays make this slightly more complicated because they have to be expanded into two arguments
    template <class... U> Node(Core::Library &lib, U... args);

    Node(Node &&x) noexcept { std::swap(ast_, x.ast_); }

    ~Node() { clingo_ast_free(ast_); }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    auto operator=(Node const &x) -> Node & {
        if (ast_ != x.ast_) {
            clingo_ast_free(ast_);
            ast_ = nullptr;
            if (!clingo_ast_copy(x.ast_, &ast_)) {
                throw std::runtime_error("could not copy ast");
            }
        }
        return *this;
    }

    auto operator=(Node &&x) noexcept -> Node & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(Node const &a, Node const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<=>(Node const &a, Node const &b) -> std::strong_ordering {
        return clingo_ast_compare(a.ast_, b.ast_) <=> 0;
    }

    auto to_string() -> std::string {
        size_t len = 0;
        if (!clingo_ast_to_string_size(ast_, &len)) {
            throw std::runtime_error("could convert to string");
        }
        std::string str;
        str.resize(len);
        if (!clingo_ast_to_string(ast_, str.data(), len)) {
            throw std::runtime_error("could convert to string");
        }
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    template <clingo_ast_attribute_e name, class U>
        requires(cpp_attribute_trait<U>::type == Type::location)
    auto get() -> U {
        clingo_location_t const *ret = nullptr;
        if (!clingo_ast_attribute_get_location(ast_, name, &ret)) {
            throw std::runtime_error("could not get location attribute");
        }
        return Core::Location{ret};
    }

    template <clingo_ast_attribute_e name, class U>
        requires(cpp_attribute_trait<U>::type == Type::string)
    auto get() -> U {
        char const *ret = nullptr;
        if (!clingo_ast_attribute_get_string(ast_, name, &ret)) {
            throw std::runtime_error("could not get string attribute");
        }
        return ret;
    }

    template <clingo_ast_attribute_e name, class U>
        requires(cpp_attribute_trait<U>::type == Type::number)
    auto get() -> U {
        int ret = 0;
        if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret)) {
            throw std::runtime_error("could not get number attribute");
        }
        return static_cast<U>(ret);
    }

    template <class U> static constexpr auto is_node_v = false;

    template <clingo_ast_type_e U> static constexpr auto is_node_v<Node<U>> = true;

    template <clingo_ast_attribute_e name, class U>
        requires(cpp_attribute_trait<U>::type == Type::record)
    auto get() -> U {
        clingo_ast_t *ast = nullptr;
        if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
            throw std::runtime_error("could not get ast attribute");
        }
        return U{ast};
    }

    template <clingo_ast_attribute_e name, class U>
        requires(cpp_attribute_trait<U>::type == Type::variant)
    auto get() -> U {
        clingo_ast_t *ast = nullptr;
        if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast)) {
            throw std::runtime_error("could not get ast attribute");
        }
        clingo_ast_type_t type = 0;
        if (!clingo_ast_get_type(ast, &type)) {
            clingo_ast_free(ast);
            throw std::runtime_error("could not get type");
        }
        return make_<U, 0>(type, ast);
    }

    // TODO: get for
    // - array

    // TODO: relatively staightforward to implement
    // needs a template list for the get calls
    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs);

    // TODO: relatively staightforward to implement
    // needs a template list for the get calls
    auto transform(Core::Library &lib, py::handle transform, py::args const &args,
                   py::kwargs const &kwargs) -> std::optional<Node>;

    // TODO: same as visit
    // needs a template list for the get calls
    auto update(Core::Library &lib, py::kwargs const &kwargs) -> Node;

    friend auto c_cast(Node const &x) -> clingo_ast_t *;

  private:
    template <class U, size_t i> static auto make_(clingo_ast_type_t type, clingo_ast_t *ast) -> U {
        if constexpr (i < std::variant_size_v<U>) {
            using A = std::variant_alternative_t<i, U>;
            if (cpp_attribute_trait<A>::ast_type == type) {
                return A{ast};
            }
            return make_<U, i + 1>(type, ast);
        } else {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    };

    clingo_ast_t *ast_ = nullptr;
};

template <clingo_ast_type_e T> inline auto c_cast(Node<T> const &x) -> clingo_ast_t * { return x.ast_; }

inline void test() {
    auto lib = Core::Library(false, false, nullptr, 0);
    auto x = TermVariable();
    auto loc = x.get<clingo_ast_attribute_location, Core::Location>();
    auto const *name = x.get<clingo_ast_attribute_name, char const *>();
    auto guard = x.get<clingo_ast_attribute_left, RightGuard>();
    auto variable = x.get<clingo_ast_attribute_left, Term>();

    auto y = TermVariable(lib, loc, "test");
    static_cast<void>(loc);
    static_cast<void>(name);
    static_cast<void>(guard);
}

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule(
        "ast", R"doc(This module provides functions to work with Abstract Syntax Trees of logic programs.)doc");

    ast.def("_type_info_yaml", &clingo_ast_type_info_yaml, R"doc(Return a yaml description of the AST.

This can be used to auto-generate most of the binding.)doc");

    auto py_term_variable = py::class_<TermVariable>(ast, "TermVariable", R"doc(A term representing a variable.)doc");

    py_term_variable
        .def(py::init<Core::Library &, Core::Location const &, char const *, bool>(), py::arg("lib"),
             py::arg("location"), py::arg("name"), py::arg("anonymous") = false, R"doc(Construct a TermVariable object.

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

    Anonymous variables receive a unique name during preprocessing.)doc")
        .def("__str__", &TermVariable::to_string)
        .def("__hash__", &TermVariable::hash)
        .def_property_readonly("location", &TermVariable::get<clingo_ast_attribute_location, Clingo::Core::Location>,
                               R"doc(The location of the variable.)doc")
        .def_property_readonly("name", &TermVariable::get<clingo_ast_attribute_name, char const *>,
                               R"doc(The name of the variable.)doc")
        .def_property_readonly("anonymous", &TermVariable::get<clingo_ast_attribute_anonymous, bool>,
                               R"doc(Whether the variable is anonymous.
Anonymous variables receive a unique name during preprocessing.)doc")
        .def("visit", &TermVariable::visit, py::arg("visitor"), R"doc(Visit the children of the expression.

Parameters
----------
visitor
    The visitor accepting the sub expressions.
)doc")
        .def("transform", &TermVariable::transform, R"doc(Transform the expression.

Parameters
----------
lib
    The library object for storing symbols.
transformer
    The transformer accepting the sub expressions.
)doc")
        .def("update", &TermVariable::update, py::arg("lib"), R"doc(Update the expression.

Accepts keyword arguments with attributes to update.

Parameters
----------
lib
    The library object for storing symbols.
)doc");
}

} // namespace Experiment

} // namespace Clingo::AST
