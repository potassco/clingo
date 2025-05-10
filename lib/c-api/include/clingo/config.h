#ifndef CLINGO_CONFIG_H
#define CLINGO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <clingo/core.h>

typedef struct clingo_control clingo_control_t;

//! @example config.c
//! The example shows how to configure the solver.
//!
//! ## Output ##
//!
//! ~~~~~~~~
//! ./config
//! Model: a
//! Model: b
//! ~~~~~~~~
//!
//! ## Code ##

//! @addtogroup c_config
//! Configuration of search and enumeration algorithms.
//!
//! Entries in a configuration are organized hierarchically.
//! Subentries are either accessed by name for map entries or by offset for array entries.
//! Value entries have a string value that can be inspected or modified.
//!
//! For an example, see @ref config.c.
//!
//! @{

//! Enumeration for entries of the configuration.
enum clingo_config_type_e {
    clingo_config_type_value = 1, //!< the entry is a (string) value
    clingo_config_type_array = 2, //!< the entry is an array
    clingo_config_type_map = 4    //!< the entry is a map
};
//! Bitset for values of type ::clingo_config_type_e.
typedef unsigned clingo_config_type_bitset_t;

//! Handle for to the solver configuration.
typedef struct clingo_config clingo_config_t;

//! Get the root key of the configuration.
//!
//! @param[in] config the target configuration
//! @param[out] key the root key
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_root(clingo_config_t const *config, clingo_id_t *key);

//! Get the type of a key.
//!
//! @note The type is bitset, an entry can have multiple (but at least one) type.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_type(clingo_config_t const *config, clingo_id_t key,
                                                  clingo_config_type_bitset_t *type);
//! Get the description of an entry.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] description the description
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_description(clingo_config_t const *config, clingo_id_t key,
                                                         clingo_string_t *description);

//! @name Functions to access arrays
//! @{

//! Get the size of an array entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_array.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] size the resulting size
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_array_size(clingo_config_t const *config, clingo_id_t key, size_t *size);
//! Get the subkey at the given offset of an array entry.
//!
//! @note Some array entries, like fore example the solver configuration, can be accessed past there actual size to add
//! subentries.
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_array.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] offset the offset in the array
//! @param[out] subkey the resulting subkey
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_array_at(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                                      clingo_id_t *subkey);
//! @}

//! @name Functions to access maps
//! @{

//! Get the number of subkeys of a map entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_map.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] size the resulting number
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size);
//! Query whether the map has a key.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] name the name to look up the subkey
//! @param[in] name the size of the name
//! @param[out] result whether the key is in the map
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_has_subkey(clingo_config_t const *config, clingo_id_t key,
                                                            char const *name, size_t size, bool *result);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_map.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key,
                                                             size_t offset, clingo_string_t *name);
//! Lookup a subkey under the given name.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_map.
//! @note Multiple levels can be looked up by concatenating keys with a period.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] name the name to look up the subkey
//! @param[in] size the size of the name
//! @param[out] subkey the resulting subkey
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name,
                                                    size_t size, clingo_id_t *subkey);
//! @}

//! @name Functions to access values
//! @{

//! Get the string value of the given entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_value.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] value the resulting string value
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_value_get(clingo_config_t const *config, clingo_id_t key,
                                                       clingo_string_t *value, bool *has_value);
//! Set the value of an entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_value.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] value the value to set
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value,
                                                       size_t size);

//! Get the string representation of the given theory element.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] builder the builder
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_to_string(clingo_config_t const *config, clingo_id_t key,
                                                       clingo_string_builder_t *builder);

//! @}

//! Get the configuration object.
//!
//! @param[in] control the target
//! @param[out] config the configuration
//! @return wether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_config(clingo_control_t *control, clingo_config_t **config);

//! @}

#ifdef __cplusplus
}
#endif

#endif
