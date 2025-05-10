#ifndef CLINGO_SCRIPT_H
#define CLINGO_SCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/control.h>
#include <clingo/core.h>
#include <clingo/symbol.h>

//! @addtogroup c_script
//! Support for calling exteral functions during grounding and customizing the main solving loop.
//!
//! @{

//! Custom scripting language to run functions during grounding.
typedef struct clingo_script {
    //! Evaluate the given source code.
    //! @param[in] code the code to evaluate
    //! @param[in] data user data as given when registering the script
    //! @return whether the function call was successful
    bool (*execute)(char const *code, void *data);
    //! Call the function with the given name and arguments.
    //! @param[in] lib library object
    //! @param[in] name the name of the function
    //! @param[in] arguments the arguments to the function
    //! @param[in] arguments_size the number of arguments
    //! @param[in] symbol_callback callback to return a pool of symbols
    //! @param[in] symbol_callback_data user data for the symbol callback
    //! @param[in] data user data as given when registering the script
    //! @return whether the function call was successful
    bool (*call)(clingo_lib_t *lib, clingo_location_t const *loc, char const *name, clingo_symbol_t const *arguments,
                 size_t arguments_size, clingo_symbol_callback_t symbol_callback, void *symbol_callback_data,
                 void *data);
    //! Check if the given function is callable.
    //! @param[in] name the name of the function
    //! @param[in] arguments the number of arguments
    //! @param[out] result whether the function is callable
    //! @param[in] data user data as given when registering the script
    //! @return whether the function call was successful
    bool (*callable)(char const *name, size_t arguments, bool *result, void *data);
    //! Run the main function.
    //! @param[in] control the control object to pass to the main function
    //! @param[in] data user data as given when registering the script
    //! @return whether the function call was successful
    bool (*main)(clingo_lib_t *lib, clingo_control_t *control, void *data);
    //! Get the name of the script.
    //! @return the name of the script.
    char const *(*name)(void *data);
    //! Get the version of the script.
    //! @return the version of the script.
    char const *(*version)(void *data);
    //! This function is called once when the script is deleted.
    //! @param[in] data user data as given when registering the script
    void (*free)(void *data);
} clingo_script_t;

//! Add a custom scripting language to a control object.
//!
//! @param[in] lib the library object to register the script with
//! @param[in] script struct with functions implementing the language
//! @param[in] data user data to pass to callbacks in the script
//! @return the result code
CLINGO_VISIBILITY_DEFAULT bool clingo_script_register(clingo_lib_t *lib, clingo_script_t const *script, void *data);

//! Get the version of the registered scripting language.
//!
//! @param[in] lib the library object
//! @param[in] name the name of the scripting language
//! @return the version
CLINGO_VISIBILITY_DEFAULT char const *clingo_script_version(clingo_lib_t *lib, char const *name);

//! @}

#ifdef __cplusplus
}
#endif

#endif
