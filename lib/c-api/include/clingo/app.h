#ifndef CLINGO_APP_H
#define CLINGO_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>

//! @addtogroup c_app
//! Support for building applications on top of clingo.
//!
//! @{

//! Run a clingo application with the given library and arguments.
//!
//! @param[in] lib the library object
//! @param[in] arguments the command line arguments
//! @param[in] size the number of command line arguments
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_main(clingo_lib_t *lib, char const *const *arguments, size_t size);

//! @}

#ifdef __cplusplus
}
#endif

#endif
