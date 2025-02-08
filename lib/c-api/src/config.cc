#include <clingo/config.h>

#include <clasp/cli/clasp_options.h>

#include "lib.hh"

auto cpp_cast(clingo_config_t const *config) -> Clasp::Cli::ClaspCliConfig const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Cli::ClaspCliConfig const *>(config);
}

auto cpp_cast(clingo_config_t *config) -> Clasp::Cli::ClaspCliConfig * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Cli::ClaspCliConfig *>(config);
}

auto clingo_config_root(clingo_config_t const *config, clingo_id_t *key) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || key == nullptr) {
            return clingo_result_invalid;
        }
        *key = Clasp::Cli::ClaspCliConfig::key_root;
    }
    CLINGO_CATCH;
}

auto clingo_config_type(clingo_config_t const *config, clingo_id_t key, clingo_config_type_bitset_t *type)
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

auto clingo_config_description(clingo_config_t const *config, clingo_id_t key, char const **description)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || description == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(config)->getKeyInfo(key, nullptr, nullptr, description, nullptr);
        if (*description == nullptr) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

auto clingo_config_array_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> clingo_result_t {
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

auto clingo_config_array_at(clingo_config_t const *config, clingo_id_t key, size_t offset, clingo_id_t *subkey)
    -> clingo_result_t {
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

auto clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> clingo_result_t {
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

auto clingo_config_map_has_subkey(clingo_config_t const *config, clingo_id_t key, char const *name, bool *result)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(config)->getKey(key, name) != Clasp::Cli::ClaspCliConfig::key_invalid;
    }
    CLINGO_CATCH;
}

auto clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key, size_t offset, char const **name)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr) {
            return clingo_result_invalid;
        }
        *name = cpp_cast(config)->getSubkey(key, offset);
        if (name == nullptr) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

auto clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name, clingo_id_t *subkey)
    -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(config)->getKey(key, name);
        if (*subkey == Clasp::Cli::ClaspCliConfig::key_invalid) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}

auto clingo_config_value_is_assigned(clingo_config_t const *config, clingo_id_t key, bool *assigned)
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

auto clingo_config_value_get(clingo_config_t const *config, clingo_id_t key, char const **value) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || value == nullptr) {
            return clingo_result_invalid;
        }
        static thread_local auto val = std::string{};
        if (cpp_cast(config)->getValue(key, val) < 0) {
            return clingo_result_invalid;
        }
        *value = val.c_str();
    }
    CLINGO_CATCH;
}

auto clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value) -> clingo_result_t {
    CLINGO_TRY {
        if (config == nullptr || value == nullptr || cpp_cast(config)->setValue(key, value) <= 0) {
            return clingo_result_invalid;
        }
    }
    CLINGO_CATCH;
}
