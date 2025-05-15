#include <clingo/config.h>

#include <clasp/cli/clasp_options.h>

#include "clingo/control.h"
#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

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

struct ConfigPrinter {
  public:
    explicit ConfigPrinter(Clasp::Cli::ClaspCliConfig const *cfg, CppClingo::Util::OutputBuffer *out)
        : cfg_{cfg}, out_{out} {}

    auto str(clingo_id_t key) {
        str_(key, 0, 0);
        if (!out_->view().empty() && out_->view().back() == '\n') {
            out_->pop();
        }
    }

  private:
    void str_(clingo_id_t key, size_t first_indent, size_t indent) {
        int map_keys = 0;
        int arr_len = 0;
        int vals = 0;
        cfg_->getKeyInfo(key, &map_keys, &arr_len, nullptr, &vals);
        auto fi = [&, first = true]() mutable { return fill(std::exchange(first, false) ? first_indent : indent); };
        if (vals >= 0) {
            if (vals > 0) {
                cfg_->getValue(key, val);
                *out_ << fi() << CppClingo::Util::p_quoted(val) << "\n";
            } else {
                *out_ << fi() << "null\n";
            }
        }
        if (map_keys > 0 && arr_len <= 0) {
            for (int i = 0; i < map_keys; ++i) {
                auto name = std::string_view{cfg_->getSubkey(key, i)};
                *out_ << fi() << name << ":";
                auto sub_key = cfg_->getKey(key, name);
                int sub_vals = 0;
                cfg_->getKeyInfo(sub_key, nullptr, nullptr, nullptr, &sub_vals);
                if (sub_vals >= 0) {
                    *out_ << " ";
                    str_(sub_key, 0, indent + name.size() + 2);
                } else {
                    *out_ << "\n";
                    str_(sub_key, indent + 2, indent + 2);
                }
            }
        }
        if (arr_len >= 0) {
            if (int e = arr_len; e > 0) {
                for (int i = 0, e = arr_len; i != e; ++i) {
                    *out_ << fi() << "- ";
                    auto sub_key = cfg_->getArrKey(key, i);
                    str_(sub_key, 0, indent + 2);
                }

            } else {
                *out_ << fi() << "[]\n";
            }
        }
    }

    Clasp::Cli::ClaspCliConfig const *cfg_;
    CppClingo::Util::OutputBuffer *out_;
    std::string val;
};

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
        if (config == nullptr || type == nullptr) {
            return fail_arguments();
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
    -> bool {
    CLINGO_TRY {
        if (config == nullptr || description == nullptr) {
            return fail_arguments();
        }
        thread_local auto val = std::string{};
        if (cpp_cast(config)->getKeyInfo(key, nullptr, nullptr, &val, nullptr) != 1) {
            return fail_arguments();
        }
        description->data = val.c_str();
        description->size = val.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_to_string(clingo_config_t const *config, clingo_id_t key,
                                        clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        ConfigPrinter{cpp_cast(config), cpp_cast(builder)}.str(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_array_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return fail_arguments();
        }
        int len = 0;
        cpp_cast(config)->getKeyInfo(key, nullptr, &len, nullptr, nullptr);
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
        *subkey = cpp_cast(config)->getArrKey(key, offset);
        if (*subkey == Clasp::Cli::ClaspCliConfig::key_invalid) {
            return fail_arguments();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_size(clingo_config_t const *config, clingo_id_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (config == nullptr || size == nullptr) {
            return fail_arguments();
        }
        int len = 0;
        cpp_cast(config)->getKeyInfo(key, &len, nullptr, nullptr, nullptr);
        if (len < 0) {
            return fail_arguments();
        }
        *size = static_cast<size_t>(len);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_has_subkey(clingo_config_t const *config, clingo_id_t key, char const *name,
                                             size_t size, bool *result) -> bool {
    CLINGO_TRY {
        if (config == nullptr || (name == nullptr && size > 0) || result == nullptr) {
            return fail_arguments();
        }
        *result =
            cpp_cast(config)->getKey(key, std::string_view{name, size}) != Clasp::Cli::ClaspCliConfig::key_invalid;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_subkey_name(clingo_config_t const *config, clingo_id_t key, size_t offset,
                                              clingo_string_t *name) -> bool {
    CLINGO_TRY {
        if (config == nullptr || name == nullptr) {
            return fail_arguments();
        }
        name->data = cpp_cast(config)->getSubkey(key, offset);
        if (name->data == nullptr) {
            return fail_arguments();
        }
        name->size = std::strlen(name->data);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_map_at(clingo_config_t const *config, clingo_id_t key, char const *name, size_t size,
                                     clingo_id_t *subkey) -> bool {
    CLINGO_TRY {
        if (config == nullptr || (name == nullptr && size > 0) || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey = cpp_cast(config)->getKey(key, std::string_view{name, size});
        if (*subkey == Clasp::Cli::ClaspCliConfig::key_invalid) {
            return fail_arguments();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_is_assigned(clingo_config_t const *config, clingo_id_t key, bool *assigned)
    -> bool {
    CLINGO_TRY {
        if (config == nullptr || assigned == nullptr) {
            return fail_arguments();
        }
        int val_len = 0;
        cpp_cast(config)->getKeyInfo(key, nullptr, nullptr, nullptr, &val_len);
        if (val_len < 0) {
            return fail_arguments();
        }
        *assigned = val_len > 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_config_value_get(clingo_config_t const *config, clingo_id_t key, clingo_string_t *value,
                                        bool *has_value) -> bool {
    CLINGO_TRY {
        if (config == nullptr) {
            return fail_arguments();
        }
        thread_local auto val = std::string{};
        int res = cpp_cast(config)->getValue(key, val);
        if (res < -1) {
            return fail_arguments();
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
    -> bool {
    CLINGO_TRY {
        if (config == nullptr || (value == nullptr && size > 0) ||
            cpp_cast(config)->setValue(key, std::string_view{value, size}) <= 0) {
            return fail_arguments();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_config(clingo_control_t *control, clingo_config_t **config) -> bool {
    CLINGO_TRY {
        if (control == nullptr || config == nullptr) {
            return fail_arguments();
        }
        *config = c_cast(&control->slv->clasp_config());
    }
    CLINGO_CATCH;
}
