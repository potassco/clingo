#ifndef CLINGO_STATS_H
#define CLINGO_STATS_H

#include <clingo/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clingo_control clingo_control_t;

//! @example stats.c
//! The example shows how to inspect stats.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./stats 0
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
//! Inspect search and problem stats.
//!
//! For an example, see @ref stats.c.
//! @{

//! Enumeration for entries of the stats.
enum clingo_stats_type_e {
    clingo_stats_type_value = 0, //!< the entry is a (double) value
    clingo_stats_type_array = 1, //!< the entry is an array
    clingo_stats_type_map = 2    //!< the entry is a map
};
//! Corresponding type to ::clingo_stats_type.
typedef int clingo_stats_type_t;

//! Handle for the solver stats.
typedef struct clingo_statistic clingo_stats_t;

//! Well-known key name intended to be used for user-specific step statistics.
CLINGO_VISIBILITY_DEFAULT extern clingo_string_t const clingo_user_stats_step;
//! Well-known key name intended to be used for user-specific accu statistics.
CLINGO_VISIBILITY_DEFAULT extern clingo_string_t const clingo_user_stats_accu;

//! Get the root key of the stats.
//!
//! @param[in] stats the target stats
//! @param[out] key the root key
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_root(clingo_stats_t const *stats, uint64_t *key);

//! Get the type of a key.
//!
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_type(clingo_stats_t const *stats, uint64_t key, clingo_stats_type_t *type);

//! Get a string representation of the statistics.
//!
//! The representation is in a YAML-like format.
//!
//! @param stats the stats
//! @param key the key
//! @param builder the builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_to_string(clingo_stats_t const *stats, uint64_t key,
                                                      clingo_string_builder_t *builder);

//! @name Functions to access arrays
//! @{

//! Get the size of an array entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_array.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_array_size(clingo_stats_t const *stats, uint64_t key, size_t *size);
//! Get the subkey at the given offset of an array entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_array.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] offset the offset in the array
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_array_at(clingo_stats_t const *stats, uint64_t key, size_t offset,
                                                     uint64_t *subkey);
//! Create the subkey at the end of an array entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_array.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] type the type of the new subkey
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_array_push(clingo_stats_t *stats, uint64_t key, clingo_stats_type_t type,
                                                       uint64_t *subkey);
//! @}

//! @name Functions to access maps
//! @{

//! Get the number of subkeys of a map entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_map.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[out] size the resulting number
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_map_size(clingo_stats_t const *stats, uint64_t key, size_t *size);
//! Test if the given map contains a specific subkey.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_map.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] name name of the subkey
//! @param[in] size the size of the name
//! @param[out] subkey resulting subkey if @p result is true
//! @param[out] result true if the map has a subkey with the given name
//! @return whether the call was successful
//! @note @p subkey is optional and can be set to NULL to only check for existence
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_map_has_subkey(clingo_stats_t const *stats, uint64_t key, char const *name,
                                                           size_t size, uint64_t *subkey, bool *result);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_map.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_map_subkey_name(clingo_stats_t const *stats, uint64_t key, size_t offset,
                                                            clingo_string_t *name);
//! Lookup a subkey under the given name.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] name the name to look up the subkey
//! @param[in] size the size of the name
//! @param[out] subkey the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_map_at(clingo_stats_t const *stats, uint64_t key, char const *name,
                                                   size_t size, uint64_t *subkey);
//! Add a subkey with the given name.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_map.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[in] name the name of the new subkey
//! @param[in] size the size of the name
//! @param[in] type the type of the new subkey
//! @param[out] subkey the index of the resulting subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_map_add_subkey(clingo_stats_t *stats, uint64_t key, char const *name,
                                                           size_t size, clingo_stats_type_t type, uint64_t *subkey);
//! @}

//! @name Functions to inspect and change values
//! @{

//! Get the value of the given entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_value.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[out] value the resulting value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_value_get(clingo_stats_t const *stats, uint64_t key, double *value);
//! Set the value of the given entry.
//!
//! @pre The @link clingo_stats_type() type@endlink of the entry must be @ref ::clingo_stats_type_value.
//! @param[in] stats the target stats
//! @param[in] key the key
//! @param[out] value the new value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_stats_value_set(clingo_stats_t *stats, uint64_t key, double value);
//! @}

//! Get the solver stats.
//!
//! @param[in] control the target control object
//! @param[out] stats the resulting stats object
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_stats(clingo_control_t *control, clingo_stats_t const **stats);

//! @}

#ifdef __cplusplus
}
#endif

#endif
