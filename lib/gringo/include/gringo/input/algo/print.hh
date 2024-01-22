#pragma once

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! @defgroup input_print Print
//! Functions to output expressions.
//!
//! @ingroup input_algo
//!
//! @{

//! @name Enumerations
//! @{

//! Output the given unary operator.
auto operator<<(std::ostream &out, UnaryOperator op) -> std::ostream &;

//! Output the given binary operator.
auto operator<<(std::ostream &out, BinaryOperator op) -> std::ostream &;

//! Output the given relation.
auto operator<<(std::ostream &out, Relation op) -> std::ostream &;

//! Output the given sign.
auto operator<<(std::ostream &out, Sign sign) -> std::ostream &;

//! Output the given aggregate function.
auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream &;

//! @}

//! @name Locations
//! @{

//! Output the position to the given stream.
auto operator<<(std::ostream &out, Position const &pos) -> std::ostream &;

//! Output the location to the given stream.
auto operator<<(std::ostream &out, Location const &loc) -> std::ostream &;

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
auto operator<<(std::ostream &out, ConditionalLiteral const &lit) -> std::ostream &;

//! Output the theory atom element to the given stream.
auto operator<<(std::ostream &out, TheoryElement const &elem) -> std::ostream &;

//! Output the set aggregate element to the given stream.
auto operator<<(std::ostream &out, SetAggregateElement const &elem) -> std::ostream &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, HeadAggregate::Element const &elem) -> std::ostream &;

//! Output the head aggregate element to the given stream.
auto operator<<(std::ostream &out, BodyAggregate::Element const &elem) -> std::ostream &;

//! @}

//! @name Literals
//! @{

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LiteralBoolean const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LiteralRelation const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, LiteralSymbolic const &lit) -> std::ostream &;

//! Output the literal to the given stream.
auto operator<<(std::ostream &out, Literal const &lit) -> std::ostream &;

//! @}

//! @name Head Literals
//! @{

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, SimpleHeadLiteral const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, Disjunction const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HeadAggregate const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HeadSetAggregate const &lit) -> std::ostream &;

//! Output the head literal to the given stream.
auto operator<<(std::ostream &out, HeadLiteral const &lit) -> std::ostream &;

//! @}

//! @name Body Literals
//! @{

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, SimpleBodyLiteral const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, Conjunction const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BodyAggregate const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BodySetAggregate const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BodyTheoryAtom const &lit) -> std::ostream &;

//! Output the body literal to the given stream.
auto operator<<(std::ostream &out, BodyLiteral const &lit) -> std::ostream &;

//! @}

//! @name Statements
//! @{

auto operator<<(std::ostream &out, Rule const &stm) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryRGuardDefinition const &def) -> std::ostream &;

//! Output the definition to the given stream.
auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, TheoryDefinition const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementOptimize const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementWeakConstraint const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementShow const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementShowSig const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementProject const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementProjectSig const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementDefined const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementExternal const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementEdge const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementHeuristic const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementScript const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementInclude const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementProgram const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, StatementConst const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, Comment const &stm) -> std::ostream &;

//! Output the statement to the given stream.
auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream &;

//! @}

//! Convert the given term into a string.
auto to_string(Term const &term) -> std::string;

//! Convert the given term into a string.
auto to_string(TheoryTerm const &term) -> std::string;

//! Convert the given literal into a string.
auto to_string(Literal const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(HeadLiteral const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(BodyLiteral const &lit) -> std::string;

//! Convert the given head literal into a string.
auto to_string(Statement const &stm) -> std::string;

//! @}

} // namespace Gringo::Input
