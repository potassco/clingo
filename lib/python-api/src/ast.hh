#pragma once

#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>

#include "core.hh"

namespace Clingo::AST {

namespace py = pybind11;

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
  private:
    clingo_ast_t *ast_; // NOLINT
};

using TermOrProjection = std::variant<Term, Projection>;

class Pool;

using TermOrPool = std::variant<Term, Pool>;

class TermVariable {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermSymbolic {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermAbsolute {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermUnaryOperation {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermBinaryOperation {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermTuple {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class TermFunction {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

class Pool {
  public:
  private:
    clingo_ast_t *ast_; // NOLINT
};

void register_module(pybind11::module &m) {
    auto ast = m.def_submodule("ast", doc(R"(
TODO
)"));

    ast.def("_type_info_json", &clingo_ast_type_info_json, doc(R"(
TODO
)"));

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
}

} // namespace Clingo::AST
