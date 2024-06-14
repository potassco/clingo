#pragma once

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! @addtogroup input_print
//! @{

//! @name Enumerations
//! @{

//! Output the given unary operator.
auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

//! Output the given binary operator.
auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;

//! Output the given aggregate function.
auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream &;

//! @}

//! @name Terms
//! @{

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Projection const &projection) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermVariable const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermSymbol const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermAbs const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermUnary const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermBinary const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermTuple const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermFunction const &term) -> std::ostream &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Term const &term) -> std::ostream &;

//! @}

//! @name Theory Terms
//! @{

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermVariable const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermSymbol const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermTuple const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermFunction const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermUnparsed const &term) -> std::ostream &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;

//! @}

//! @name Aggregates
//! @{

//! Output the condititional literal to the given stream.
auto operator<<(std::ostream &out, CondLit const &lit) -> std::ostream &;

//! Output the theory atom element to the given stream.
auto operator<<(std::ostream &out, TheoryElement const &elem) -> std::ostream &;

//! Output the set aggregate element to the given stream.
auto operator<<(std::ostream &out, SetAggregateElement const &elem) -> std::ostream &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, HdLitAggregateElement const &elem) -> std::ostream &;

//! Output the body aggregate element to the given stream.
auto operator<<(std::ostream &out, BdLitAggregateElement const &elem) -> std::ostream &;

//! @}

//! @name Literals
//! @{

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitBool const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitComparison const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitSymbolic const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream &;

//! @}

//! @name Head Literals
//! @{

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitSimple const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitDisjunction const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitAggregate const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitSetAggregate const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLit const &lit) -> std::ostream &;

//! @}

//! @name Body Literals
//! @{

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitSimple const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitConjunction const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitAggregate const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitSetAggregate const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitTheoryAtom const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLit const &lit) -> std::ostream &;

//! @}

//! @name Statement Elements
//! @{

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryRGuardDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream &;

//! Output the optimize tuple to the given stream.
auto operator<<(std::ostream &out, OptimizeTuple const &tuple) -> std::ostream &;

//! Output the optimize element to the given stream.
auto operator<<(std::ostream &out, OptimizeElement const &elem) -> std::ostream &;

//! Output the edge to the given stream.
auto operator<<(std::ostream &out, Edge const &edge) -> std::ostream &;

//! @}

//! @name Statements
//! @{

auto operator<<(std::ostream &out, StmRule const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmTheory const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmOptimize const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmWeakConstraint const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmShow const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmShowSig const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProject const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProjectSig const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmDefined const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmExternal const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmEdge const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmHeuristic const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmScript const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmInclude const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProgram const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmConst const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmComment const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream &;

//! @}

//! Convert the given term into a string.
auto to_string(Term const &term) -> std::string;

//! Convert the given theory term into a string.
auto to_string(TheoryTerm const &term) -> std::string;

//! Convert the given literal into a string.
auto to_string(Lit const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(HdLit const &lit) -> std::string;

//! Convert the given body literal into a string.
auto to_string(BdLit const &lit) -> std::string;

//! Convert the given statement into a string.
auto to_string(Stm const &stm) -> std::string;

//! @}

} // namespace Gringo::Input
