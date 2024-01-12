#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"
#include "symbol.hh"

namespace Clingo::AST {

namespace py = pybind11;

using Clingo::Symbol::Symbol;

struct Position {
    char const *file;
    size_t line;
    size_t column;
};
enum class UnaryOperator {
    Minus = 0,
    Negation = 1,
};

enum class BinaryOperator {
    And = 0,
    Division = 1,
    Minus = 2,
    Modulo = 3,
    Multiplication = 4,
    Or = 5,
    Plus = 6,
    Power = 7,
    Xor = 8,
};

class TermVariable;

class TermSymbolic;

class TermAbsolute;

class TermUnaryOperation;

class TermBinaryOperation;

class TermTuple;

class TermFunction;

class TermVariable;

using Term = std::variant<TermVariable, TermSymbolic, TermAbsolute, TermUnaryOperation, TermBinaryOperation, TermTuple,
                          TermFunction>;

class Projection {
  public:
    auto location() -> clingo_location_t;
    static auto acquire(clingo_ast_t *ast) -> Projection { return {ast}; }
    ~Projection() { clingo_ast_free(ast_); }

  private:
    Projection(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};
using TermOrProjection = std::variant<Term, Projection>;

class Pool;

using TermOrPool = std::variant<Term, Pool>;

class TermVariable {
  public:
    auto location() -> clingo_location_t;
    auto name() -> char const *;
    static auto acquire(clingo_ast_t *ast) -> TermVariable { return {ast}; }
    ~TermVariable() { clingo_ast_free(ast_); }

  private:
    TermVariable(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermSymbolic {
  public:
    auto location() -> clingo_location_t;
    auto symbol() -> Symbol;
    static auto acquire(clingo_ast_t *ast) -> TermSymbolic { return {ast}; }
    ~TermSymbolic() { clingo_ast_free(ast_); }

  private:
    TermSymbolic(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermAbsolute {
  public:
    auto location() -> clingo_location_t;
    static auto acquire(clingo_ast_t *ast) -> TermAbsolute { return {ast}; }
    ~TermAbsolute() { clingo_ast_free(ast_); }

  private:
    TermAbsolute(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermUnaryOperation {
  public:
    auto location() -> clingo_location_t;
    auto operator_type() -> UnaryOperator;
    auto right() -> Term;
    static auto acquire(clingo_ast_t *ast) -> TermUnaryOperation { return {ast}; }
    ~TermUnaryOperation() { clingo_ast_free(ast_); }

  private:
    TermUnaryOperation(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermBinaryOperation {
  public:
    auto location() -> clingo_location_t;
    auto left() -> Term;
    auto operator_type() -> BinaryOperator;
    auto right() -> Term;
    static auto acquire(clingo_ast_t *ast) -> TermBinaryOperation { return {ast}; }
    ~TermBinaryOperation() { clingo_ast_free(ast_); }

  private:
    TermBinaryOperation(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermTuple {
  public:
    auto location() -> clingo_location_t;
    static auto acquire(clingo_ast_t *ast) -> TermTuple { return {ast}; }
    ~TermTuple() { clingo_ast_free(ast_); }

  private:
    TermTuple(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class TermFunction {
  public:
    auto location() -> clingo_location_t;
    auto name() -> char const *;
    auto external() -> bool;
    static auto acquire(clingo_ast_t *ast) -> TermFunction { return {ast}; }
    ~TermFunction() { clingo_ast_free(ast_); }

  private:
    TermFunction(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

class Pool {
  public:
    static auto acquire(clingo_ast_t *ast) -> Pool { return {ast}; }
    ~Pool() { clingo_ast_free(ast_); }

  private:
    Pool(clingo_ast_t *ast) : ast_{ast} {}
    clingo_ast_t *ast_;
};

auto Projection::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermVariable::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermVariable::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}
auto TermSymbolic::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermSymbolic::symbol() -> Symbol {
    clingo_symbol_t ret;
    if (!clingo_ast_attribute_get_symbol(ast_, clingo_ast_attribute_symbol, &ret)) {
        throw std::runtime_error("could not get symbol attribute");
    }
    return Symbol::acquire(ret);
}
auto TermAbsolute::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermUnaryOperation::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermUnaryOperation::operator_type() -> UnaryOperator {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<UnaryOperator>(ret);
}
auto TermUnaryOperation::right() -> Term {
    clingo_ast_t *ret;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ret)) {
        throw std::runtime_error("could not get ast attribute");
    }
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ret, &type)) {
        clingo_ast_free(ret);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ret);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ret);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ret);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ret);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ret);
        }
    }
    throw std::runtime_error("unexpected ast type");
}
auto TermBinaryOperation::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermBinaryOperation::left() -> Term {
    clingo_ast_t *ret;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_left, &ret)) {
        throw std::runtime_error("could not get ast attribute");
    }
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ret, &type)) {
        clingo_ast_free(ret);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ret);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ret);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ret);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ret);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ret);
        }
    }
    throw std::runtime_error("unexpected ast type");
}
auto TermBinaryOperation::operator_type() -> BinaryOperator {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_operator_type, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return static_cast<BinaryOperator>(ret);
}
auto TermBinaryOperation::right() -> Term {
    clingo_ast_t *ret;
    if (!clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_right, &ret)) {
        throw std::runtime_error("could not get ast attribute");
    }
    clingo_ast_type_t type;
    if (!clingo_ast_get_type(ret, &type)) {
        clingo_ast_free(ret);
        throw std::runtime_error("could not get type");
    }
    switch (type) {
        case clingo_ast_type_term_variable: {
            return TermVariable::acquire(ret);
        }
        case clingo_ast_type_term_symbolic: {
            return TermSymbolic::acquire(ret);
        }
        case clingo_ast_type_term_absolute: {
            return TermAbsolute::acquire(ret);
        }
        case clingo_ast_type_term_unary_operation: {
            return TermUnaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_binary_operation: {
            return TermBinaryOperation::acquire(ret);
        }
        case clingo_ast_type_term_tuple: {
            return TermTuple::acquire(ret);
        }
        case clingo_ast_type_term_function: {
            return TermFunction::acquire(ret);
        }
    }
    throw std::runtime_error("unexpected ast type");
}
auto TermTuple::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermFunction::location() -> clingo_location_t {
    clingo_location_t ret;
    if (!clingo_ast_attribute_get_location(ast_, clingo_ast_attribute_location, &ret)) {
        throw std::runtime_error("could not get location attribute");
    }
    return ret;
}
auto TermFunction::name() -> char const * {
    char const *ret;
    if (!clingo_ast_attribute_get_string(ast_, clingo_ast_attribute_name, &ret)) {
        throw std::runtime_error("could not get string attribute");
    }
    return ret;
}
auto TermFunction::external() -> bool {
    int ret;
    if (!clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_external, &ret)) {
        throw std::runtime_error("could not get number attribute");
    }
    return ret != 0;
}

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule("ast", doc(R"(
TODO
)"));

    ast.def("_type_info_json", &clingo_ast_type_info_json, doc(R"(
TODO
)"));

    py::class_<Position>(ast, "Position", R"(Position tracking object.)")
        .def_readonly("file", &Position::file)
        .def_readonly("line", &Position::line)
        .def_readonly("column", &Position::column);

    py::class_<clingo_location_t>(ast, "Location", R"(Location tracking object.)")
        .def_property_readonly("begin",
                               [](clingo_location_t const &loc) {
                                   return Position{loc.begin_file, loc.begin_line, loc.begin_column};
                               })
        .def_property_readonly("end", [](clingo_location_t const &loc) {
            return Position{loc.end_file, loc.end_line, loc.end_column};
        });
    py::enum_<UnaryOperator>(ast, "UnaryOperator", R"(Available unary operators.)")
        .value("Minus", UnaryOperator::Minus, R"(Operator `-`.)")
        .value("Negation", UnaryOperator::Negation, R"(Operator `~`.)");

    py::enum_<BinaryOperator>(ast, "BinaryOperator", R"(Available binary operators.)")
        .value("And", BinaryOperator::And, R"(Operator `&`.)")
        .value("Division", BinaryOperator::Division, R"(Operator `/`.)")
        .value("Minus", BinaryOperator::Minus, R"(Operator `-`.)")
        .value("Modulo", BinaryOperator::Modulo, R"(Operator `%`.)")
        .value("Multiplication", BinaryOperator::Multiplication, R"(Operator `*`.)")
        .value("Or", BinaryOperator::Or, R"(Operator `|`.)")
        .value("Plus", BinaryOperator::Plus, R"(Operator `+`.)")
        .value("Power", BinaryOperator::Power, R"(Operator `**`.)")
        .value("Xor", BinaryOperator::Xor, R"(Operator `^`.)");

    py::class_<Projection>(ast, "Projection", R"(TODO.)").def_property_readonly("location", &Projection::location);

    py::class_<TermVariable>(ast, "TermVariable", R"(TODO.)")
        .def_property_readonly("location", &TermVariable::location)
        .def_property_readonly("name", &TermVariable::name);

    py::class_<TermSymbolic>(ast, "TermSymbolic", R"(TODO.)")
        .def_property_readonly("location", &TermSymbolic::location)
        .def_property_readonly("symbol", &TermSymbolic::symbol);

    py::class_<TermAbsolute>(ast, "TermAbsolute", R"(TODO.)")
        .def_property_readonly("location", &TermAbsolute::location);

    py::class_<TermUnaryOperation>(ast, "TermUnaryOperation", R"(TODO.)")
        .def_property_readonly("location", &TermUnaryOperation::location)
        .def_property_readonly("operator_type", &TermUnaryOperation::operator_type)
        .def_property_readonly("right", &TermUnaryOperation::right);

    py::class_<TermBinaryOperation>(ast, "TermBinaryOperation", R"(TODO.)")
        .def_property_readonly("location", &TermBinaryOperation::location)
        .def_property_readonly("left", &TermBinaryOperation::left)
        .def_property_readonly("operator_type", &TermBinaryOperation::operator_type)
        .def_property_readonly("right", &TermBinaryOperation::right);

    py::class_<TermTuple>(ast, "TermTuple", R"(TODO.)").def_property_readonly("location", &TermTuple::location);

    py::class_<TermFunction>(ast, "TermFunction", R"(TODO.)")
        .def_property_readonly("location", &TermFunction::location)
        .def_property_readonly("name", &TermFunction::name)
        .def_property_readonly("external", &TermFunction::external);

    py::class_<Pool>(ast, "Pool", R"(TODO.)");
}

} // namespace Clingo::AST
