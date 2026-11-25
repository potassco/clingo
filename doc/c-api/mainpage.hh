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
//! @note Each module in the C and C++ API comes with an example highlighting
//! key functionality. The example should be studied along with the module
//! documentation. The C++ examples mirror the C examples.

//! @defgroup c_api C API
//! API providing a stable interface for applications using Clingo.
//!
//! The API is mainly intended for developing higher level language bindings.
//!
//! The functions follow these conventions:
//! - Functions returning whether the call was successful report errors via
//!   clingo_get_error(). They check their arguments and fail with
//!   ::clingo_result_invalid if a required pointer is `NULL`, an array pointer
//!   is `NULL` but its size is not zero, or a value is invalid. Indices out
//!   of range are reported with ::clingo_result_range.
//! - Out parameters marked as optional may be `NULL` and are then not written.
//!   This is mostly the case for functions returning multiple values.
//! - Functions taking ownership of an argument take it also if they fail.
//! - Functions that cannot fail do not check their arguments unless documented
//!   otherwise. Functions freeing or releasing objects accept `NULL` and then
//!   do nothing.
//! @{

//! @defgroup c_core Core Functionality
//! Core types and functions used throughout all modules and version information.
//!
//! For an example, see @ref version.c.

//! @defgroup c_symbol Symbol Handling
//! Working with (evaluated) ground terms and related functions.
//!
//! @note Functions to create symbols are only thread-safe if library flags have been requested accordingly.
//!
//! For an example, see @ref symbol.c.

//! @defgroup c_ast Abstract Syntax Trees
//! Functions and data structures to work with program ASTs.
//!
//! For an example, see @ref ast.c.

//! @defgroup c_control Grounding and Solving
//! Functions to control the grounding and solving process.
//!
//! For an example, see @ref control.c.
//! @{

//! @defgroup c_base Symbolic Atom Inspection
//! Inspection of atoms occurring in ground logic programs.
//!
//! For examples, see @ref base-atoms.c and @ref base-theory.c.

//! @defgroup c_shared Basic Shared Types
//! Enumerations shared by various modules.

//! @defgroup c_observe Ground Program Inspection
//! Functions and data structures to inspect ground programs.
//!
//! For an example, see @ref observe.c.

//! @defgroup c_backend Ground Program Extension
//! Add non-ground program representations (ASTs) to logic programs or extend the ground (aspif) program.
//!
//! For an example about ground logic programs, see @ref backend.c.
//! For an example about non-ground logic programs, see @ref ast.c and the @ref c_ast module.

//! @defgroup c_ground Grounding
//! Interact with a running ground call.
//!
//! ::clingo_ground_handle_t objects can be used for asynchronous grounding.
//! They allow for stopping the grounding process and waiting for its completion.
//!
//! For an example showing how to ground asynchronously, see @ref ground.c.

//! @defgroup c_model Model Inspection
//! Inspection of models and a high-level interface to add constraints during solving.
//!
//! For an example, see @ref model.c.

//! @defgroup c_solve Solving
//! Interact with a running search.
//!
//! ::clingo_solve_handle_t objects can be used for both synchronous and asynchronous search,
//! as well as iteratively receiving models and solve results.
//!
//! For an example showing how to solve asynchronously, see @ref solve.c.

//! @defgroup c_config Solver Configuration
//! Configuration of search and enumeration algorithms.
//!
//! Entries in a configuration are organized hierarchically.
//! Subentries are either accessed by name for map entries or by offset for array entries.
//! Value entries have a string value that can be inspected or modified.
//!
//! For an example, see @ref config.c.

//! @defgroup c_stats Statistics
//! Inspect search and problem stats.
//!
//! For an example, see @ref stats.c.

//! @defgroup c_propagate Theory Propagation
//! Extend the search with propagators for arbitrary theories.
//!
//! For an example, see @ref propagate.c.

//! @defgroup c_profile Profile Grounding
//! Profile the grounding process.
//!
//! For an example, see @ref profile.c.

//! @}

//! @defgroup c_script Scripting Support for Grounding
//! Support for calling external functions during grounding and customizing the main solving loop.

//! @defgroup c_app Applications on top of Clingo
//! Support for building applications on top of clingo.
//!
//! For an example, see @ref app.c.

//! @defgroup c_theory External Theory Support
//! Interface for external theory plugins.
//!
//! This module defines a well-specified C interface that must be implemented
//! by external theory plugins. The interface is designed to allow efficient
//! interoperation between the core solver and external theory implementations.
//! It facilitates integration across language barriers (for example, bridging
//! C++ and Python) and across modular boundaries within the system.
//!
//! Among its responsibilities, the theory interface includes: Providing
//! essential theory callbacks (e.g., rewriting ASTs, model handling, preparing
//! theory data, and statistics collection). Defining the lifecycle of a theory
//! object, including optional cleanup via a destroy callback.
//!
//! For an example, see @ref theory.c.

//! @}

//! @defgroup cpp_api C++ API
//! API providing a stable interface for applications using Clingo.
//!
//! This API is suitable to develop performant applications on top of Clingo.
//! It should generally be preferred to the low-level C bindings except maybe
//! for language bindings.
//! @{

//! @defgroup cpp_core Core Functionality
//! Core types and functions used throughout all modules and version information.
//!
//! For an example, see @ref version.cc.

//! @defgroup cpp_symbol Symbol Handling
//! Working with (evaluated) ground terms and related functions.
//!
//! For an example, see @ref symbol.cc.
//!
//! @note Functions to create symbols are only thread-safe if library flags
//! have been requested accordingly.

//! @defgroup cpp_ast Abstract Syntax Trees
//! Functions and data structures to work with program ASTs.
//!
//! For an example, see @ref ast.cc.

//! @defgroup cpp_control Grounding and Solving
//! Functions to control the grounding and solving process.
//!
//! For an example, see @ref control.cc.
//! @{

//! @defgroup cpp_base Atom, Term, and Theory Base Inspection
//! Inspection of atoms occurring in ground logic programs.
//!
//! For examples, see @ref base-atoms.cc and @ref base-theory.cc.

//! @defgroup cpp_observe Ground Program Inspection
//! Functions and data structures to inspect ground programs.
//!
//! For an example, see @ref observe.cc.

//! @defgroup cpp_backend Ground Program Extension
//! Extend the logic program with ground statements (in aspif-like format).
//!
//! For an example, see @ref backend.cc.

//! @defgroup cpp_ground Grounding
//! Grounding logic programs.
//!
//! For an example, see @ref ground.cc.

//! @defgroup cpp_profile Profile Grounding
//! Functions to profile the grounding process.
//!
//! For an example, see @ref profile.cc.

//! @defgroup cpp_solve Solving
//! Intercept models and interact with a running search.
//!
//! For examples, see @ref model.cc and @ref solve.cc.

//! @defgroup cpp_config Solver Configuration
//! Configuration of search and enumeration algorithms.
//!
//! For an example, see @ref config.cc.
//!
//! Entries in a configuration are organized hierarchically. Subentries are
//! either accessed by name for map entries or by offset for array entries.
//! Value entries have a string value that can be inspected or modified.

//! @defgroup cpp_stats Statistics
//! Inspect search and problem stats.
//!
//! For an example, see @ref stats.cc.

//! @defgroup cpp_propagate Theory Propagation
//! Extend the search with propagators for arbitrary theories.
//!
//! For an example, see @ref propagate.cc.

//! @}

//! @defgroup cpp_script Scripting Support for Grounding
//! Support for external functions solving customizations.
//!
//! This module provides an interface to implement custom scripts that can be
//! used to provide external functions callable during grounding and to
//! customize the main application flow.

//! @defgroup cpp_app Applications on top of Clingo
//! Support for building applications on top of clingo.
//!
//! For an example, see @ref app.cc.

//! @defgroup cpp_theory External Theory Support
//! Interface for external theory plugins.
//!
//! This module defines a well-specified C interface that must be implemented
//! by external theory plugins. The interface is designed to allow efficient
//! interoperation between the core solver and external theory implementations.
//! It facilitates integration across language barriers (for example, bridging
//! C++ and Python) and across modular boundaries within the system.
//!
//! Among its responsibilities, the theory interface includes: Providing
//! essential theory callbacks (e.g., rewriting ASTs, model handling, preparing
//! theory data, and statistics collection). Defining the lifecycle of a theory
//! object, including optional cleanup via a destroy callback.
//!
//! For an example, see @ref theory.cc.

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
