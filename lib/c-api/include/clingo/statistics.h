#ifndef CLINGO_STATISTICS_H
#define CLINGO_STATISTICS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>

typedef struct clingo_control clingo_control_t;

//! @example statistics.c
//! The example shows how to inspect statistics.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./statistics 0
//! Model: a
//! Model: b
//! problem:
//!   lp:
//!     atoms:
//!       2
//!     atoms_aux:
//!       0
//!     ...
//! solving:
//!   ...
//! summary:
//!   ...
//! accu:
//!   times:
//!     ...
//!   models:
//!     ...
//!   solving:
//!     ...
//! ~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_stats
//! Inspect search and problem statistics.
//!
//! For an example, see @ref statistics.c.
//! @{

//! Enumeration for entries of the statistics.
enum clingo_statistics_type_e {
    clingo_statistics_type_empty = 0, //!< the entry is invalid (has neither of the types below)
    clingo_statistics_type_value = 1, //!< the entry is a (double) value
    clingo_statistics_type_array = 2, //!< the entry is an array
    clingo_statistics_type_map = 3    //!< the entry is a map
};
//! Corresponding type to ::clingo_statistics_type.
typedef int clingo_statistics_type_t;

//! Handle for the solver statistics.
typedef struct clingo_statistic clingo_statistics_t;

//! Get the root key of the statistics.
//!
//! @param[in] statistics the target statistics
//! @param[out] key the root key
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_root(clingo_statistics_t const *statistics, uint64_t *key);

//! Get the type of a key.
//!
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_type(clingo_statistics_t const *statistics, uint64_t key,
                                                                 clingo_statistics_type_t *type);

//! @name Functions to access arrays
//! @{

//! Get the size of an array entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_array.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_array_size(clingo_statistics_t const *statistics,
                                                                       uint64_t key, size_t *size);
//! Get the subkey at the given offset of an array entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_array.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] offset the offset in the array
//! @param[out] subkey the resulting subkey
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_array_at(clingo_statistics_t const *statistics,
                                                                     uint64_t key, size_t offset, uint64_t *subkey);
//! Create the subkey at the end of an array entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_array.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] type the type of the new subkey
//! @param[out] subkey the resulting subkey
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_array_push(clingo_statistics_t *statistics, uint64_t key,
                                                                       clingo_statistics_type_t type, uint64_t *subkey);
//! @}

//! @name Functions to access maps
//! @{

//! Get the number of subkeys of a map entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] size the resulting number
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_map_size(clingo_statistics_t const *statistics,
                                                                     uint64_t key, size_t *size);
//! Test if the given map contains a specific subkey.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] name name of the subkey
//! @param[out] result true if the map has a subkey with the given name
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_map_has_subkey(clingo_statistics_t const *statistics,
                                                                           uint64_t key, char const *name,
                                                                           bool *result);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_map_subkey_name(clingo_statistics_t const *statistics,
                                                                            uint64_t key, size_t offset,
                                                                            char const **name);
//! Lookup a subkey under the given name.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] name the name to look up the subkey
//! @param[out] subkey the resulting subkey
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_map_at(clingo_statistics_t const *statistics, uint64_t key,
                                                                   char const *name, uint64_t *subkey);
//! Add a subkey with the given name.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_map.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[in] name the name of the new subkey
//! @param[in] type the type of the new subkey
//! @param[out] subkey the index of the resulting subkey
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_map_add_subkey(clingo_statistics_t *statistics,
                                                                           uint64_t key, char const *name,
                                                                           clingo_statistics_type_t type,
                                                                           uint64_t *subkey);
//! @}

//! @name Functions to inspect and change values
//! @{

//! Get the value of the given entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_value.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] value the resulting value
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_value_get(clingo_statistics_t const *statistics,
                                                                      uint64_t key, double *value);
//! Set the value of the given entry.
//!
//! @pre The @link clingo_statistics_type() type@endlink of the entry must be @ref ::clingo_statistics_type_value.
//! @param[in] statistics the target statistics
//! @param[in] key the key
//! @param[out] value the new value
//! @return the result code
CLINGO_VISIBILITY_DEFAULT clingo_result_t clingo_statistics_value_set(clingo_statistics_t *statistics, uint64_t key,
                                                                      double value);
//! @}

//! @}

#ifdef __cplusplus
}
#endif

#endif
