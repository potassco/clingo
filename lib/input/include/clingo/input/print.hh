#pragma once

#include <clingo/input/statement.hh>

namespace CppClingo::Input {

//! @addtogroup input_print
//! @{

//! @name Enumerations
//! @{

//! Output the given unary operator.
auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;
//! Output the given unary operator.
auto operator<<(Util::OutputBuffer &out, UnaryOperator op) -> Util::OutputBuffer &;

//! Output the given binary operator.
auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;
//! Output the given binary operator.
auto operator<<(Util::OutputBuffer &out, BinaryOperator op) -> Util::OutputBuffer &;

//! @}

//! @name Terms
//! @{

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Projection const &projection) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, Projection const &projection) -> Util::OutputBuffer &;

//! Output an f-string format spec to the given stream.
auto operator<<(std::ostream &out, FormatSpec const &spec) -> std::ostream &;
//! Output an f-string format spec to the given stream.
auto operator<<(Util::OutputBuffer &out, FormatSpec const &spec) -> Util::OutputBuffer &;

//! Output an f-string format field to the given stream.
auto operator<<(std::ostream &out, FormatFieldExpression const &field) -> std::ostream &;
//! Output an f-string format field to the given stream.
auto operator<<(Util::OutputBuffer &out, FormatFieldExpression const &field) -> Util::OutputBuffer &;

//! Output an f-string format string field to the given stream.
auto operator<<(std::ostream &out, FormatFieldLiteral const &field) -> std::ostream &;
//! Output an f-string format string field to the given stream.
auto operator<<(Util::OutputBuffer &out, FormatFieldLiteral const &field) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermFormatString const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermFormatString const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermVariable const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermVariable const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermSymbol const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermSymbol const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermAbs const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermAbs const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermUnary const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermUnary const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermBinary const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermBinary const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermTuple const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermTuple const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, TermFunction const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, TermFunction const &term) -> Util::OutputBuffer &;

//! Output the term to the given stream.
auto operator<<(std::ostream &out, Term const &term) -> std::ostream &;
//! Output the term to the given stream.
auto operator<<(Util::OutputBuffer &out, Term const &term) -> Util::OutputBuffer &;

//! @}

//! @name Theory Terms
//! @{

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermVariable const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermVariable const &term) -> Util::OutputBuffer &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermSymbol const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermSymbol const &term) -> Util::OutputBuffer &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermTuple const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermTuple const &term) -> Util::OutputBuffer &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermFunction const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermFunction const &term) -> Util::OutputBuffer &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTermUnparsed const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermUnparsed const &term) -> Util::OutputBuffer &;

//! Output the theory term to the given stream.
auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream &;
//! Output the theory term to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTerm const &term) -> Util::OutputBuffer &;

//! @}

//! @name Aggregates
//! @{

//! Output the conditional literal to the given stream.
auto operator<<(std::ostream &out, CondLit const &lit) -> std::ostream &;
//! Output the conditional literal to the given stream.
auto operator<<(Util::OutputBuffer &out, CondLit const &lit) -> Util::OutputBuffer &;

//! Output the theory atom element to the given stream.
auto operator<<(std::ostream &out, TheoryElement const &elem) -> std::ostream &;
//! Output the theory atom element to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryElement const &elem) -> Util::OutputBuffer &;

//! Output the set aggregate element to the given stream.
auto operator<<(std::ostream &out, SetAggregateElement const &elem) -> std::ostream &;
//! Output the set aggregate element to the given stream.
auto operator<<(Util::OutputBuffer &out, SetAggregateElement const &elem) -> Util::OutputBuffer &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, HdLitAggregateElement const &elem) -> std::ostream &;
//! Output the head aggregate element to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLitAggregateElement const &elem) -> Util::OutputBuffer &;

//! Output the body aggregate element to the given stream.
auto operator<<(std::ostream &out, BdLitAggregateElement const &elem) -> std::ostream &;
//! Output the body aggregate element to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitAggregateElement const &elem) -> Util::OutputBuffer &;

//! @}

//! @name Literals
//! @{

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitBool const &lit) -> std::ostream &;
//! Output the literal to the given stream.
auto operator<<(Util::OutputBuffer &out, LitBool const &lit) -> Util::OutputBuffer &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitComparison const &lit) -> std::ostream &;
//! Output the literal to the given stream.
auto operator<<(Util::OutputBuffer &out, LitComparison const &lit) -> Util::OutputBuffer &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LitSymbolic const &lit) -> std::ostream &;
//! Output the literal to the given stream.
auto operator<<(Util::OutputBuffer &out, LitSymbolic const &lit) -> Util::OutputBuffer &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, Lit const &lit) -> std::ostream &;
//! Output the literal to the given stream.
auto operator<<(Util::OutputBuffer &out, Lit const &lit) -> Util::OutputBuffer &;

//! @}

//! @name Head Literals
//! @{

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitSimple const &lit) -> std::ostream &;
//! Output the head literal to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLitSimple const &lit) -> Util::OutputBuffer &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitDisjunction const &lit) -> std::ostream &;
//! Output the head literal to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLitDisjunction const &lit) -> Util::OutputBuffer &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitAggregate const &lit) -> std::ostream &;
//! Output the head literal to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLitAggregate const &lit) -> Util::OutputBuffer &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLitSetAggregate const &lit) -> std::ostream &;
//! Output the head literal to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLitSetAggregate const &lit) -> Util::OutputBuffer &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HdLit const &lit) -> std::ostream &;
//! Output the head literal to the given stream.
auto operator<<(Util::OutputBuffer &out, HdLit const &lit) -> Util::OutputBuffer &;

//! @}

//! @name Body Literals
//! @{

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitSimple const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitSimple const &lit) -> Util::OutputBuffer &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitConjunction const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitConjunction const &lit) -> Util::OutputBuffer &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitAggregate const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitAggregate const &lit) -> Util::OutputBuffer &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitSetAggregate const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitSetAggregate const &lit) -> Util::OutputBuffer &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLitTheoryAtom const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLitTheoryAtom const &lit) -> Util::OutputBuffer &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BdLit const &lit) -> std::ostream &;
//! Output the body literal to the given stream.
auto operator<<(Util::OutputBuffer &out, BdLit const &lit) -> Util::OutputBuffer &;

//! @}

//! @name Statement Elements
//! @{

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream &;
//! Output the definition to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryOpDefinition const &def) -> Util::OutputBuffer &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream &;
//! Output the definition to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryTermDefinition const &def) -> Util::OutputBuffer &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryRGuardDefinition const &def) -> std::ostream &;
//! Output the definition to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryRGuardDefinition const &def) -> Util::OutputBuffer &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream &;
//! Output the definition to the given stream.
auto operator<<(Util::OutputBuffer &out, TheoryAtomDefinition const &def) -> Util::OutputBuffer &;

//! Output the optimize tuple to the given stream.
auto operator<<(std::ostream &out, OptimizeTuple const &tuple) -> std::ostream &;
//! Output the optimize tuple to the given stream.
auto operator<<(Util::OutputBuffer &out, OptimizeTuple const &tuple) -> Util::OutputBuffer &;

//! Output the optimize element to the given stream.
auto operator<<(std::ostream &out, OptimizeElement const &elem) -> std::ostream &;
//! Output the optimize element to the given stream.
auto operator<<(Util::OutputBuffer &out, OptimizeElement const &elem) -> Util::OutputBuffer &;

//! Output the edge to the given stream.
auto operator<<(std::ostream &out, Edge const &edge) -> std::ostream &;
//! Output the edge to the given stream.
auto operator<<(Util::OutputBuffer &out, Edge const &edge) -> Util::OutputBuffer &;

//! @}

//! @name Statements
//! @{

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmRule const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmRule const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmTheory const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmTheory const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmOptimize const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmOptimize const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmWeakConstraint const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmWeakConstraint const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmShow const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmShow const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmShowSig const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmShowSig const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProject const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmProject const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProjectSig const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmProjectSig const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmDefined const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmDefined const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmExternal const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmExternal const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmEdge const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmEdge const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmHeuristic const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmHeuristic const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmScript const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmScript const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmInclude const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmInclude const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmProgram const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmProgram const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmConst const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmConst const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StmComment const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, StmComment const &stm) -> Util::OutputBuffer &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream &;
//! Output the statement to the given stream.
auto operator<<(Util::OutputBuffer &out, Stm const &stm) -> Util::OutputBuffer &;

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

} // namespace CppClingo::Input
