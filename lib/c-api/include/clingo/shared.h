#ifndef CLINGO_SHARED_H
#define CLINGO_SHARED_H

//! @addtogroup c_shared
//! Enumerations shared by various modules.
//! @{

#ifdef __cplusplus
extern "C" {
#endif

//! Enumeration of different heuristic modifiers.
enum clingo_heuristic_type_e {
    clingo_heuristic_type_level = 0,  //!< set the level of an atom
    clingo_heuristic_type_sign = 1,   //!< configure which sign to chose for an atom
    clingo_heuristic_type_factor = 2, //!< modify VSIDS factor of an atom
    clingo_heuristic_type_init = 3,   //!< modify the initial VSIDS score of an atom
    clingo_heuristic_type_true = 4,   //!< set the level of an atom and choose a positive sign
    clingo_heuristic_type_false = 5   //!< set the level of an atom and choose a negative sign
};
//! Corresponding type to ::clingo_heuristic_type_e.
typedef int clingo_heuristic_type_t;

//! Enumeration of different external statements.
enum clingo_external_type_e {
    clingo_external_type_free = 0,    //!< allow an external to be assigned freely
    clingo_external_type_true = 1,    //!< assign an external to true
    clingo_external_type_false = 2,   //!< assign an external to false
    clingo_external_type_release = 3, //!< no longer treat an atom as external
};
//! Corresponding type to ::clingo_external_type_e.
typedef int clingo_external_type_t;

//! @}

#ifdef __cplusplus
}
#endif

#endif
