//! @file clingo.h
//! Single header containing the whole clingo API.
//!
//! @author Roland Kaminski

//! @mainpage Clingo C API
//! This API provides functions to ground and solve logic programs.
//!
//! The documentation is structured into different modules.
//! To get an overview, checkout the [Topics](topics.html) page.
//! To get started, take a look at the documentation of the @ref c_control module.
//!
//! The source code of clingo is available on [github.com/potassco/clingo](https://github.com/potassco/clingo).
//!
//! For information about the syntax and semantics of the clingo language,
//! take a look the [Potassco Guide](https://github.com/potassco/guide/releases/).
//!
//! @note Each module comes with an example highlighting key functionality.
//! The example should be studied along with the module documentation.

#ifndef CLINGO_H
#define CLINGO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/app.h>
#include <clingo/ast.h>
#include <clingo/base.h>
#include <clingo/control.h>
#include <clingo/core.h>
#include <clingo/model.h>
#include <clingo/script.h>
#include <clingo/solve.h>
#include <clingo/symbol.h>

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

//! @defgroup c_model Model Inspection

//! @defgroup c_solving Solving

//! @}

//! @defgroup c_script Scripting Support for Grounding

//! @defgroup c_app Applications on top of Clingo

//! @}

#ifdef __cplusplus
}
#endif

#endif
