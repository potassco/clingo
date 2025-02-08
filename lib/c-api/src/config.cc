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
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(offset);
        *subkey = 0;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        *size = 0;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_map_has_subkey(clingo_config_t const *config, clingo_id_t key, char const *name, bool *result)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(name);
        *result = false;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key, size_t offset, char const **name)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(offset);
        *name = nullptr;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name, clingo_id_t *subkey)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(name);
        *subkey = 0;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_value_is_assigned(clingo_config_t const *config, clingo_id_t key, bool *assigned)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        *assigned = false;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_value_get(clingo_config_t const *config, clingo_id_t key, char const **value, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(value);
        *size = 0;
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}

auto clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(config);
        static_cast<void>(key);
        static_cast<void>(value);
        throw std::logic_error("implement me!!!");
    }
    CLINGO_CATCH;
}
