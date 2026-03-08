#ifndef CLINGO_CONFIG_H
#define CLINGO_CONFIG_H

#include <clingo/core.h>

#ifdef __cplusplus
extern "C" {
#endif

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

//! Callback interface for custom configuration entries.
//!
//! This struct allows users to define custom configuration entries by
//! providing function pointers for querying and manipulating the entry. The
//! `data` pointer is passed to each callback and can be used to store
//! user-defined state.
//!
//! On each configuration path, there can be at most one array entry. If
//! present, the array index is passed to all entries following an array entry
//! in the path. It is up to the entry implementation to interpret an absent
//! index (i.e., when `index` is NULL).
//!
//! All callbacks are optional; if a callback is NULL, the corresponding
//! operation is not supported for this entry.
typedef struct clingo_config_entry {
    //! Get the value of this entry as a string.
    //!
    //! @param[in] index an optional array index (can be NULL)
    //! @param[in] data user data pointer
    //! @param[out] value output string value
    //! @param[out] has_value set to true if a value is available, false otherwise
    //! @return true on success, false on error
    bool (*get)(size_t const *index, void *data, clingo_string_t *value, bool *has_value);

    //! Set the value of this entry from a string.
    //!
    //! @param[in] index an optional array index (can be NULL)
    //! @param[in] value input string value
    //! @param[in] size length of the value string
    //! @param[in] data user data pointer
    //! @return true on success, false on error
    bool (*set)(size_t const *index, char const *value, size_t size, void *data);

    //! Get the size of this entry if it is an array.
    //!
    //! @param[in] data user data pointer
    //! @param[out] size number of elements in the array (or zero)
    //! @param[out] has_size true if the entry is an array, false otherwise
    //! @return true on success, false on error
    bool (*size)(void *data, size_t *size, bool *has_size);

    //! Free the user data associated with this entry.
    //!
    //! @param[in] data user data pointer
    void (*free)(void *data);
} clingo_config_entry_t;

//! Add a custom configuration entry under a parent key.
//!
//! The entry is defined by the provided callbacks and user data. The API takes
//! ownership of the `data` pointer and will call the `free` callback (if non-NULL)
//! when the entry is removed or the configuration is destroyed.
//!
//! @param[in] config the configuration object
//! @param[in] parent the parent key under which to add the entry
//! @param[in] name the name of the new entry (not null-terminated)
//! @param[in] name_size the length of the name string
//! @param[in] description the description of the entry (not null-terminated)
//! @param[in] description_size the length of the description string
//! @param[in] entry pointer to the entry callbacks struct
//! @param[in] data user data pointer for the entry callbacks
//! @return true on success, false on error
CLINGO_VISIBILITY_DEFAULT bool clingo_config_add(clingo_config_t *config, clingo_id_t parent, char const *name,
                                                 size_t name_size, char const *description, size_t description_size,
                                                 clingo_config_entry_t const *entry, void *data);

//! Get the root key of the configuration.
//!
//! @param[in] config the target configuration
//! @param[out] key the root key
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_root(clingo_config_t const *config, clingo_id_t *key);

//! Get the type of a key.
//!
//! @note The type is bitset, an entry can have multiple (but at least one) type.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] type the resulting type
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_type(clingo_config_t const *config, clingo_id_t key,
                                                  clingo_config_type_bitset_t *type);
//! Get the description of an entry.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] description the description
//! @return whether the call was successful
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
//! @return whether the call was successful
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
//! @return whether the call was successful
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
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size);
//! Get the name associated with the offset-th subkey.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_map.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] offset the offset of the name
//! @param[out] name the resulting name
//! @return whether the call was successful
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
//! @param[out] has_subkey whether the map has the subkey
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name,
                                                    size_t size, clingo_id_t *subkey, bool *has_subkey);
//! @}

//! @name Functions to access values
//! @{

//! Get the string value of the given entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_value.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[out] value the resulting string value
//! @param[out] has_value whether the config entry has a value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_value_get(clingo_config_t const *config, clingo_id_t key,
                                                       clingo_string_t *value, bool *has_value);
//! Set the value of an entry.
//!
//! @pre The @link clingo_config_type() type@endlink of the entry must be @ref ::clingo_config_type_value.
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] value the value to set
//! @param[in] size the size of the value
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value,
                                                       size_t size);

//! Get the string representation of the given configuration element.
//!
//! @param[in] config the target configuration
//! @param[in] key the key
//! @param[in] builder the builder
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_config_to_string(clingo_config_t const *config, clingo_id_t key,
                                                       clingo_string_builder_t *builder);

//! @}

//! Get the configuration object.
//!
//! @param[in] control the target
//! @param[out] config the configuration
//! @return whether the call was successful
CLINGO_VISIBILITY_DEFAULT bool clingo_control_config(clingo_control_t *control, clingo_config_t **config);

//! @}

#ifdef __cplusplus
}
#endif

#endif
