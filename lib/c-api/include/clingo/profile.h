#ifndef CLINGO_PROFILE_H
#define CLINGO_PROFILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>

typedef struct clingo_control clingo_control_t;

//! @example profile.c
//! The example shows how to inspect profiling data.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./profile
//! TODO
//! ~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_profile
//! Profile the grounding process.
//!
//! For an example, see @ref profile.c.
//! @{

//! Enumeration for entries of the stats.
enum clingo_profile_type_e {
    clingo_profile_type_step = 0, //!< indicate per step values
    clingo_profile_type_accu = 1, //!< indicate accumulated values
};
//! Corresponding type to ::clingo_profile_type_e.
typedef int clingo_profile_type_t;

//! Per node performance statistics gathered while grounding a logic program.
typedef struct clingo_profile_data {
    //! The number of matches produced by the instantiator.
    uint64_t matches;
    //! The number of instances produced by the instantiator.
    uint64_t instances;
    //! The time in nanoseconds spent instantiating.
    uint64_t time_instantiate;
    //! The time in nanoseconds spent propagating.
    uint64_t time_propagate;
} clingo_profile_data_t;

//! Visitor for profiling data.
typedef struct clingo_profile_visitor {
    //! Visit an internal node in the profile tree.
    //!
    //! The key corresponds to the string representation of expressions in a
    //! logic program.
    //!
    //! @param[in] key the key of the node
    //! @param[in] key_size the size of the key
    //! @param[in] depth the depth of the node in the tree
    //! @param[in] nested whether time values are included in parent nodes
    //! @param[in] data the user data of the visitor
    //! @return whether the call was successful
    bool (*internal)(size_t depth, char const *key, size_t key_size, bool nested, void *data);
    //! Visit leaf nodes in the profile tree.
    //!
    //! The key corresponds to a fixed set of values.
    //!
    //! @param[in] values the values of the node
    //! @param[in] type the type of the node
    //! @param[in] data the user data of the visitor
    //! @return whether the call was successful
    bool (*leaf)(size_t depth, clingo_profile_data_t *values, clingo_profile_type_t type, void *data);

} clingo_profile_visitor_t;

//! Visit the profiling data of a control object.
//!
//! @param[in] control the target control object
//! @param[in] visit the visitor to call for each entry
//! @param[in] data user data for the visitor
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_profile(clingo_control_t const *control,
                                                      clingo_profile_visitor_t const *visit, void *data);
//! @}

#ifdef __cplusplus
}
#endif

#endif
