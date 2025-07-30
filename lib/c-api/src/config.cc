#include <clingo/config.h>

#include <clasp/cli/clasp_options.h>

#include "clingo/control.h"
#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

inline auto cpp_cast(clingo_config_t const *config) -> Control::ClingoConfig const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Control::ClingoConfig const *>(config);
}

inline auto cpp_cast(clingo_config_t *config) -> Control::ClingoConfig * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Control::ClingoConfig *>(config);
}

inline auto c_cast(Control::ClingoConfig *config) -> clingo_config_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_config_t *>(config);
}

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_config_root(clingo_config_t const *config, clingo_id_t *key) -> bool {
    CLINGO_TRY {
        if (config == nullptr || key == nullptr) {
            return fail_arguments();
        }
        *key = Clasp::Cli::ClaspCliConfig::key_root;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_type(clingo_config_t const *config, clingo_id_t key, clingo_config_type_bitset_t *type)
    -> bool {
    CLINGO_TRY {
        using Flags = CppClingo::Control::ClingoConfig::ValueFlags;
        if (config == nullptr || type == nullptr) {
            return fail_arguments();
        }
        int map_len = 0;
        int arr_len = 0;
        auto val_info = Flags::none;
        cpp_cast(config)->key_info(key, &map_len, &arr_len, &val_info);
        *type = 0;
        if (map_len > 0) {
            *type |= clingo_config_type_map;
        }
        if (arr_len >= 0) {
            *type |= clingo_config_type_array;
        }
        // TODO: extend
        if (intersects(val_info, Flags::get | Flags::set)) {
            *type |= clingo_config_type_value;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_description(clingo_config_t const *config, clingo_id_t key, clingo_string_t *description)
    -> bool {
    CLINGO_TRY {
        if (config == nullptr || description == nullptr) {
            return fail_arguments();
        }
        auto str = cpp_cast(config)->description(key);
        description->data = str.data();
        description->size = str.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_to_string(clingo_config_t const *config, clingo_id_t key,
                                        clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (config == nullptr || builder == nullptr) {
            return fail_arguments();
        }
        cpp_cast(config)->str(*cpp_cast(builder), key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_array_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return fail_arguments();
        }
        int len = 0;
        cpp_cast(config)->key_info(key, nullptr, &len, nullptr);
        if (len < 0) {
            return fail_arguments();
        }
        *size = static_cast<size_t>(len);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_array_at(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                       clingo_id_t *subkey) -> bool {
    CLINGO_TRY {
        if (config == nullptr || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey = cpp_cast(config)->array_at(key, static_cast<CppClingo::Control::ClingoConfig::IndexType>(offset));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return fail_arguments();
        }
        int len = 0;
        cpp_cast(config)->key_info(key, &len, nullptr, nullptr);
        *size = static_cast<size_t>(len);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                              clingo_string_t *name) -> bool {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr) {
            return fail_arguments();
        }
        auto str = cpp_cast(config)->map_nth(key, offset);
        name->data = str.data();
        name->size = str.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name, size_t size,
                                     clingo_id_t *subkey, bool *has_subkey) -> bool {
    CLINGO_TRY {
        if (config == nullptr || (name == nullptr && size > 0)) {
            return fail_arguments();
        }
        auto res = cpp_cast(config)->map_at(key, std::string_view{name, size});
        if (has_subkey != nullptr) {
            *has_subkey = res.has_value();
        }
        if (subkey != nullptr && res) {
            *subkey = *res;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_get(clingo_config_t const *config, clingo_id_t key, clingo_string_t *value,
                                        bool *has_value) -> bool {
    CLINGO_TRY {
        if (config == nullptr) {
            return fail_arguments();
        }
        auto str = cpp_cast(config)->get_value(key);
        if (has_value != nullptr) {
            *has_value = str.has_value();
        }
        if (value != nullptr) {
            value->data = str ? str->data() : nullptr;
            value->size = str ? str->size() : 0;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_set(clingo_config_t *config, clingo_id_t key, char const *value, size_t size)
    -> bool {
    CLINGO_TRY {
        if (config == nullptr || (value == nullptr && size > 0)) {
            return fail_arguments();
        }
        cpp_cast(config)->set_value(key, std::string_view{value, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_config(clingo_control_t *control, clingo_config_t **config) -> bool {
    CLINGO_TRY {
        if (control == nullptr || config == nullptr) {
            return fail_arguments();
        }
        *config = c_cast(&control->slv->config());
    }
    CLINGO_CATCH;
}
