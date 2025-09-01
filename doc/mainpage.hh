//! @mainpage Overview
//! This API provides functions to ground and solve logic programs.
//!
//! The documentation is structured into different modules. To get an overview,
//! checkout the [Topics](topics.html) page.
//!
//! The source code of clingo is available on
//! [github.com/potassco/clingo](https://github.com/potassco/clingo).
//!
//! For information about the syntax and semantics of the clingo language, take
//! a look the [Potassco Guide](https://github.com/potassco/guide/releases/).
//!
//! @note Each module in the C API comes with an example highlighting key
//! functionality. The example should be studied along with the module
//! documentation.

//! @defgroup c_api C API
//! API providing a stable interface for applications using Clingo.
//!
//! The API is mainly intended for developing higher level language bindings.
//! @{

//! @defgroup c_core Core Functionality

//! @defgroup c_symbol Symbol Handling

//! @defgroup c_ast Abstract Syntax Trees

//! @defgroup c_control Grounding and Solving
//! @{

//! @defgroup c_base Symbolic Atom Inspection

//! @defgroup c_shared Basic Shared Types

//! @defgroup c_observe Ground Program Inspection

//! @defgroup c_backend Ground Program Extension

//! @defgroup c_model Model Inspection

//! @defgroup c_solve Solving

//! @defgroup c_config Solver Configuration

//! @defgroup c_stats Statistics

//! @defgroup c_propagate Theory Propagation

//! @defgroup c_profile Profile Grounding

//! @}

//! @defgroup c_script Scripting Support for Grounding

//! @defgroup c_app Applications on top of Clingo

//! @defgroup c_theory External Theory Support

//! @}

//! @defgroup cpp_api C++ API
//! API providing a stable interface for applications using Clingo.
//!
//! This API is suitable to develop performant applications on top of Clingo.
//! It should generally be preferred to the low-level C bindings except maybe
//! for language bindings.
//! @{

//! @defgroup cpp_core Core Functionality

//! @defgroup cpp_symbol Symbol Handling

//! @defgroup cpp_ast Abstract Syntax Trees

//! @defgroup cpp_control Grounding and Solving
//! @{

//! @defgroup cpp_base Atom, Term, and Theory Base Inspection

//! @defgroup cpp_observe Ground Program Inspection

//! @defgroup cpp_backend Ground Program Extension

//! @defgroup cpp_solve Solving

//! @defgroup cpp_config Solver Configuration

//! @defgroup cpp_stats Statistics

//! @defgroup cpp_propagate Theory Propagation

//! @}

//! @defgroup cpp_script Scripting Support for Grounding

//! @defgroup cpp_app Applications on top of Clingo

//! @defgroup cpp_theory External Theory Support

//! @}

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
