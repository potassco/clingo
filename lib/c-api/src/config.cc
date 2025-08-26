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

class ConfigEntry {
  public:
    constexpr ConfigEntry(clingo_config_entry_t const *entry) noexcept
        : entry_{entry != nullptr ? *entry : clingo_config_entry_t{}} {}
    ConfigEntry(ConfigEntry const &other) = delete;
    ConfigEntry(ConfigEntry &&other) noexcept : entry_{other.entry_} { other.entry_ = clingo_config_entry_t{}; }
    auto operator=(ConfigEntry const &other) -> auto & = delete;
    auto operator=(ConfigEntry &&other) noexcept -> auto & {
        if (this != &other) {
            if (entry_.free != nullptr) {
                entry_.free(nullptr);
            }
            entry_ = other.entry_;
            other.entry_ = clingo_config_entry_t{};
        }
        return *this;
    }
    ~ConfigEntry() {
        if (entry_.free != nullptr) {
            entry_.free(nullptr);
        }
    }

    auto operator->() -> clingo_config_entry_t const * { return &entry_; }

  private:
    clingo_config_entry_t entry_;
};

class ConfigEntryAdapter : public CppClingo::Control::ClingoConfig::Entry {
  public:
    ConfigEntryAdapter(ConfigEntry c_entry, void *data) : c_entry_{std::move(c_entry)}, data_{data} {}

  private:
    using ValueFlags = CppClingo::Control::ClingoConfig::ValueFlags;
    using KeyType = CppClingo::Control::ClingoConfig::KeyType;

    auto do_value_type() -> ValueFlags override {
        auto ret = ValueFlags::none;
        if (c_entry_->get != nullptr) {
            ret |= ValueFlags::get;
        }
        if (c_entry_->set != nullptr) {
            ret |= ValueFlags::set;
        }
        return ret;
    }

    auto do_get_value(std::optional<CppClingo::Control::ClingoConfig::KeyType> index, std::string &value)
        -> bool override {
        if (c_entry_->get == nullptr) {
            return false;
        }
        auto cstr = clingo_string_t{};
        bool has_value = false;
        size_t idx = index.value_or(0);
        handle_error(c_entry_->get(index ? &idx : nullptr, data_, &cstr, &has_value));
        value.clear();
        if (has_value) {
            value.assign(cstr.data, cstr.size);
        }
        return has_value;
    }

    void do_set_value(std::optional<CppClingo::Control::ClingoConfig::KeyType> index, std::string_view val) override {
        if (c_entry_->set == nullptr) {
            throw std::runtime_error("set_value not implemented");
        }
        size_t idx = index.value_or(0);
        handle_error(c_entry_->set(index ? &idx : nullptr, val.data(), val.size(), data_));
    }

    auto do_array_size() -> std::optional<int> override {
        if (c_entry_->size == nullptr) {
            return std::nullopt;
        }
        size_t sz = 0;
        bool has_size = false;
        handle_error(c_entry_->size(data_, &sz, &has_size));
        return has_size ? std::make_optional(static_cast<int>(sz)) : std::nullopt;
    }

    ConfigEntry c_entry_;
    void *data_;
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

extern "C" auto clingo_config_add(clingo_config_t *config, clingo_id_t parent, char const *name, size_t name_size,
                                  char const *description, size_t description_size, clingo_config_entry_t const *entry,
                                  void *data) -> bool {
    auto c_entry = ConfigEntry{entry};
    CLINGO_TRY {
        if (config == nullptr || name == nullptr) {
            return fail_arguments();
        }
        auto cpp_entry = std::make_unique<ConfigEntryAdapter>(std::move(c_entry), data);
        auto name_sv = std::string_view{name, name_size};
        auto desc_sv = std::string_view{description, description_size};
        cpp_cast(config)->add_entry(parent, name_sv, desc_sv, std::move(cpp_entry));
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
