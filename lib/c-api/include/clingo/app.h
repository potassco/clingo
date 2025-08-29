#ifndef CLINGO_APP_H
#define CLINGO_APP_H

#include <clingo/control.h>
#include <clingo/core.h>
#include <clingo/solve.h>

#ifdef __cplusplus
extern "C" {
#endif

//! @example app.c
//! The example shows how to extend the clingo application.
//!
//! It behaves like a normal clingo but adds one option to override the default program part to ground.
//! ## Example calls ##
//!
//! ~~~~~~~~~~~~
//! $ cat example.lp
//! b.
//! #program test.
//! t.
//!
//! $ ./application --program test example.lp
//! example version 1.0.0
//! Reading from example.lp
//! Solving...
//! Answer: 1
//! t
//! SATISFIABLE
//!
//! Models       : 1+
//! Calls        : 1
//! Time         : 0.004s (Solving: 0.00s 1st Model: 0.00s Unsat: 0.00s)
//! CPU Time     : 0.004s
//! ~~~~~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_app
//! Support for building applications on top of clingo.
//!
//! @{

//! Object to add command-line options.
typedef struct clingo_options clingo_options_t;

//! Callback to customize clingo main function.
//!
//! @param[in] control corresponding control object
//! @param[in] files files passed via command line arguments
//! @param[in] size number of files
//! @param[in] data user data for the callback
//! @return whether the call was successful
typedef bool (*clingo_main_function_t)(clingo_control_t *control, clingo_string_t const *files, size_t size,
                                       void *data);

//! Callback to print a model in default format.
//!
//! @param[in] data user data for the callback
//! @return whether the call was successful
typedef bool (*clingo_default_model_printer_t)(void *data);

//! Callback to customize model printing.
//!
//! @param[in] model the model
//! @param[in] printer the default model printer
//! @param[in] printer_data user data for the printer
//! @param[in] data user data for the callback
//! @return whether the call was successful
typedef bool (*clingo_model_printer_t)(clingo_model_t const *model, clingo_default_model_printer_t printer,
                                       void *printer_data, void *data);

//! This struct contains a set of functions to customize the clingo application.
typedef struct clingo_application {
    void (*program_name)(void *data, clingo_string_t *string);       //!< callback to obtain program name
    void (*version)(void *data, clingo_string_t *string);            //!< callback to obtain version information
    clingo_main_function_t main;                                     //!< callback to override clingo's main function
    clingo_model_printer_t print_model;                              //!< callback to override default model printing
    bool (*register_options)(clingo_options_t *options, void *data); //!< callback to register options
    bool (*validate_options)(void *data);                            //!< callback validate options
} clingo_application_t;

//! Callback to parse the value of a command-line option.
//!
//! If an options fails to parse, the function should set
//! a clingo_result_invalid error and return false.
//!
//! @param[in] value the value to parse
//! @param[in] size the size of the value
//! @param[in] data the user data of the callback
//! @return whether the call was successful
typedef bool (*clingo_option_parser_t)(char const *value, size_t size, void *data);

//! Add an option that is processed with a custom parser.
//!
//! Note that the parser also has to take care of storing the semantic value of
//! the option somewhere.
//!
//! Parameter option specifies the name(s) of the option. For example,
//! "-p,ping" adds the short option "-p" and its long form "--ping". It is also
//! possible to associate an option with a help level by prepending "@l" to the
//! option specification. Options with a level greater than zero are only shown
//! if the argument to help is greater or equal to l.
//!
//! @param[in] options object to register the option with
//! @param[in] group options are grouped into sections as given by this string
//! @param[in] group_size the size of the group
//! @param[in] option specifies the command line option
//! @param[in] option_size teh size of the option
//! @param[in] description the description of the option
//! @param[in] description_size the size of the description
//! @param[in] parser callback to parse the value of the option
//! @param[in] data user data for the callback
//! @param[in] multi whether the option can appear multiple times on the command-line
//! @param[in] argument optional string (NULL) to change the value name in the generated help output
//! @param[in] argument_size the size of the argument
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_options_add(clingo_options_t *options, char const *group, size_t group_size,
                                                  char const *option, size_t option_size, char const *description,
                                                  size_t description_size, clingo_option_parser_t parser, void *data,
                                                  bool multi, char const *argument, size_t argument_size);
//! Add an option that is a simple flag.
//!
//! This function is similar to @ref clingo_options_add() but simpler because it only supports flags, which do not have
//! values. If a flag is passed via the command-line, the parameter target is set to true.
//!
//! @param[in] options object to register the option with
//! @param[in] group options are grouped into sections as given by this string
//! @param[in] group_size the size of the group
//! @param[in] option specifies the command line option
//! @param[in] option_size teh size of the option
//! @param[in] description the description of the option
//! @param[in] description_size the size of the description
//! @param[in] target boolean set to true if the flag is given on the command-line
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_options_add_flag(clingo_options_t *options, char const *group, size_t group_size,
                                                       char const *option, size_t option_size, char const *description,
                                                       size_t description_size, bool *target);

//! Run an application with the given library and arguments.
//!
//! Note that the application can be set to NULL to start clingo.
//!
//! Returns an exit code instead of a result code.
//!
//! @param[in] lib the library object
//! @param[in] arguments the command line arguments
//! @param[in] size the number of command line arguments
//! @param[in] app struct with callbacks to override default clingo functionality
//! @param[in] data user data for callbacks in app
//! @param[out] code the exit code
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_main(clingo_lib_t *lib, clingo_string_t const *arguments, size_t size,
                                           clingo_application_t const *app, void *data, int *code);

//! @}

#ifdef __cplusplus
}
#endif

#endif
