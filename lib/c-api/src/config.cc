#include <clingo/config.h>

#include <clasp/cli/clasp_options.h>

#include "clingo/control.h"
#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

inline auto cpp_cast(clingo_config_t const *config) -> Clasp::Cli::ClaspCliConfig const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Cli::ClaspCliConfig const *>(config);
}

inline auto cpp_cast(clingo_config_t *config) -> Clasp::Cli::ClaspCliConfig * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Cli::ClaspCliConfig *>(config);
}

inline auto c_cast(Clasp::Cli::ClaspCliConfig *config) -> clingo_config_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_config_t *>(config);
}

extern "C" auto clingo_config_root(clingo_config_t const *config, clingo_id_t *key) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || key == nullptr) {
            return clingo_result_invalid;
        }
        *key = Clasp::Cli::ClaspCliConfig::key_root;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_type(clingo_config_t const *config, clingo_id_t key, clingo_config_type_bitset_t *type)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || type == nullptr) {
            return clingo_result_invalid;
        }
        int map_len = 0;
        int arr_len = 0;
        int val_len = 0;
        cpp_cast(config)->getKeyInfo(key, &map_len, &arr_len, nullptr, &val_len);
        *type = 0;
        if (map_len >= 0) {
            *type |= clingo_config_type_map;
        }
        if (arr_len >= 0) {
            *type |= clingo_config_type_array;
        }
        if (val_len >= 0) {
            *type |= clingo_config_type_value;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_description(clingo_config_t const *config, clingo_id_t key, clingo_string_t *description)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || description == nullptr) {
            return clingo_result_invalid;
        }
        thread_local auto val = std::string{};
        if (cpp_cast(config)->getKeyInfo(key, nullptr, nullptr, &val, nullptr) != 1) {
            return clingo_result_invalid;
        }
        description->data = val.c_str();
        description->size = val.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_array_size(clingo_config_t const *config, clingo_id_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        int len = 0;
        cpp_cast(config)->getKeyInfo(key, nullptr, &len, nullptr, nullptr);
        if (len < 0) {
            return clingo_result_invalid;
        }
        *size = static_cast<size_t>(len);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_array_at(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                       clingo_id_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(config)->getArrKey(key, offset);
        if (*subkey == Clasp::Cli::ClaspCliConfig::key_invalid) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        int len = 0;
        cpp_cast(config)->getKeyInfo(key, &len, nullptr, nullptr, nullptr);
        if (len < 0) {
            return clingo_result_invalid;
        }
        *size = static_cast<size_t>(len);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_has_subkey(clingo_config_t const *config, clingo_id_t key, char const *name,
                                             size_t size, bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || (name == nullptr && size > 0) || result == nullptr) {
            return clingo_result_invalid;
        }
        *result =
            cpp_cast(config)->getKey(key, std::string_view{name, size}) != Clasp::Cli::ClaspCliConfig::key_invalid;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                              clingo_string_t *name) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr) {
            return clingo_result_invalid;
        }
        name->data = cpp_cast(config)->getSubkey(key, offset);
        if (name->data == nullptr) {
            return clingo_result_invalid;
        }
        name->size = std::strlen(name->data);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name, size_t size,
                                     clingo_id_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || (name == nullptr && size > 0) || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(config)->getKey(key, std::string_view{name, size});
        if (*subkey == Clasp::Cli::ClaspCliConfig::key_invalid) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_is_assigned(clingo_config_t const *config, clingo_id_t key, bool *assigned)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || assigned == nullptr) {
            return clingo_result_invalid;
        }
        int val_len = 0;
        cpp_cast(config)->getKeyInfo(key, nullptr, nullptr, nullptr, &val_len);
        if (val_len < 0) {
            return clingo_result_invalid;
        }
        *assigned = val_len > 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_get(clingo_config_t const *config, clingo_id_t key, clingo_string_t *value,
                                        bool *has_value) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr) {
            return clingo_result_invalid;
        }
        thread_local auto val = std::string{};
        int res = cpp_cast(config)->getValue(key, val);
        if (res < -1) {
            return clingo_result_invalid;
        }
        if (has_value != nullptr) {
            *has_value = res >= 0;
        }
        if (res >= 0 && value != nullptr) {
            value->data = val.c_str();
            value->size = val.size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value, size_t size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || (value == nullptr && size > 0) ||
            cpp_cast(config)->setValue(key, std::string_view{value, size}) <= 0) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_config(clingo_control_t *control, clingo_config_t **config) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || config == nullptr) {
            return clingo_result_invalid;
        }
        *config = c_cast(&control->slv->clasp_config());
    }
    CLINGO_CATCH;
}
