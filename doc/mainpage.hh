//! @defgroup API Internal C++ API
//! This is the internal Clingo API, use at your own risk.
//!
//! The interface might change across minor releases without further notice.
//! @{

//! @defgroup util Utility
//! Library for utility functionality.
//! @{

//! @defgroup util_traits Type Traits
//! Type traits used throughout the library.
//! @{
//! @}

//! @defgroup util_math Math Functions
//! Additional (checked) math functions.
//! @{
//! @}

//! @defgroup util_algorithm Generic Algorithms
//! Generic algorithms used throughout the library.
//! @{
//! @}

//! @defgroup util_hash Hash Functions
//! Generic functions for equality comparison and hash computation.
//! @{
//! @}

//! @defgroup util_print Printing
//! Generic functions for printing.
//! @{
//! @}

//! @defgroup util_enum Helpers for enumurations
//! Currently, just provides a macro to create bitsets.
//! @{
//! @}

//! @defgroup util_optional Optional Values
//! Data structures and algorithms around optional values.
//! @{
//! @}

//! @defgroup util_immutable Immutable Values
//! Immutable values and arrays with reference counting.
//! @{
//! @}

//! @defgroup util_container Generic Containers
//! Generic containers used throughout the library.
//! @{
//! @}

//! @defgroup util_record Records
//! Helpers to declare records with keyword arguments.
//! @{
//! @}

//! @defgroup util_debug Debugging Functions
//! Helper functions for debugging.
//! @{
//! @}

//! @}

//! @defgroup core Core
//! Library for core functionality.
//! @{

//! @defgroup core_number Numbers
//! Data structures and functions to represent arbitrary precision integers.
//! @{
//! @}

//! @defgroup core_symbol Symbols
//! Data structures and functions to represent symbols.
//! @{
//! @}

//! @defgroup core_location Source Locations
//! Data structures and functions to track source locations.
//! @{
//! @}

//! @defgroup core_logger Logging
//! Functions and classes for logging.
//! @{
//! @}

//! @defgroup core_output Output
//! Interfaces to output logic programs.
//! @{
//! @}

//! @}

//! @defgroup input Input
//! Library for representing and rewriting logic programs.
//! @{

//! @defgroup input_language Language
//! Data structures and functions to capture the clingo language.
//! @{

//! @defgroup input_term Terms
//! Data structures and functions to represent terms.
//! @{
//! @}

//! @defgroup input_literal Literals
//! Data structures and functions to represent simple literals.
//! @{
//! @}

//! @defgroup input_theory Theory Terms and Atoms
//! Data structures and functions to represent theory terms and atoms.
//! @{
//! @}

//! @defgroup input_aggregate Aggregates
//! Common data structures and functions for head and body aggregates.
//! @{
//! @}

//! @defgroup input_head Head Literals
//! Data structures and functions to represent head literals.
//! @{
//! @}

//! @defgroup input_body Body Literals
//! Data structures and functions to represent body literals.
//! @{
//! @}

//! @defgroup input_statement Statements
//! Data structures and functions to represent statements.
//! @{
//! @}

//! @defgroup input_program Programs
//! Data structures and functions to represent and rewrite programs.
//! @{
//! @}

//! @}

//! @defgroup input_algo Algorithms
//! Algorithms for the input language.
//! @{

//! @defgroup input_visit_variables Visit Variables
//! Functions to visit variables in expressions.
//! @{
//! @}

//! @defgroup input_analyze Analyze
//! Functions to analyze expressions.
//! @{
//! @}

//! @defgroup input_print Print
//! Functions to output expressions.
//! @{
//! @}

//! @defgroup input_parse Parse
//! Functions to parse the input language.
//! @{
//! @}

//! @defgroup input_check Check
//! Additional syntax checks.
//! @{
//! @}

//! @defgroup input_evaluate Evaluate
//! Functions to evaluate expressions.
//! @{
//! @}

//! @defgroup input_rewrite Rewrite
//! Functions to rewrite expressions.
//! @{
//! @}

//! @}

//! @}

//! @defgroup ground Grounding
//! Library for grounding statements.
//! @{

//! @defgroup ground_script Scripts
//! Interfaces to run scripts.
//! @{
//! @}

//! @defgroup ground_base Atom Bases
//! Data structures and functions to represent bases for atoms, aggregate atoms, and similar.
//! @{
//! @}

//! @defgroup ground_matcher Matchers
//! Data structures and functions to match symbols and expressions.
//! @{
//! @}

//! @defgroup ground_instantiator Instantiators
//! Data structures and functions to compute joins.
//! @{
//! @}

//! @defgroup ground_language Language
//! Data structures and functions to represent groundable expressions.
//! @{

//! @defgroup ground_term Terms
//! Data structures and functions to ground terms.
//! @{
//! @}

//! @defgroup ground_literal Literals
//! Data structures and functions to ground literals.
//! @{
//! @}

//! @defgroup ground_hdcondlit Head Conditional Literals
//! Data structures and functions to ground head conditional literals.
//! @{
//! @}

//! @defgroup ground_bdcondlit Body Conditional Literals
//! Data structures and functions to ground body conditional literals.
//! @{
//! @}

//! @defgroup ground_hdaggr Head Aggregates
//! Data structures and functions to ground head aggregates.
//! @{
//! @}

//! @defgroup ground_bdaggr Body Aggregates
//! Data structures and functions to ground body aggregates.
//! @{
//! @}

//! @defgroup ground_assignaggr Assignment Aggregates
//! Data structures and functions to ground assignment aggregates.
//! @{
//! @}

//! @defgroup ground_theory Theory Atoms
//! Data structures and functions to ground theory atoms.
//! @{
//! @}

//! @defgroup ground_stm Statements
//! Data structures and functions to ground statements.
//! @{
//! @}

//! @}

//! @}

//! @defgroup output Output
//! Library for outputting grounded statements.
//! @{
//! @}

//! @defgroup control Control
//! Library combining input, ground, and output.
//! @{
//! @}

//! @}

//! @defgroup cpp_api C++ API
//! This is the stable C++ API for applications using Clingo.
//! @{

//! @defgroup cpp_app Applications on top of Clingo

//! @}
