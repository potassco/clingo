#pragma once

#include <clingo/core.hh>

#include <clingo/config.h>

namespace Clingo {

// TODO: a to string would be nice

enum class ConfigType : clingo_config_type_bitset_t {
    value = clingo_config_type_value,
    array = clingo_config_type_array,
    map = clingo_config_type_map,
};
CLINGO_ENABLE_BITSET_ENUM(ConfigType);

class ConstConfigArray;
class ConfigArray;
class ConstConfigMap;
class ConfigMap;

class ConstConfig {
  public:
    explicit ConstConfig(clingo_config_t const *stats, Id key) : cfg_{stats}, key_{key} {}
    friend auto c_cast(ConstConfig const &stats) -> clingo_config_t const * { return stats.cfg_; }

    [[nodiscard]] auto type() const -> ConfigType {
        clingo_config_type_bitset_t type = 0;
        Detail::handle_error(clingo_config_type(cfg_, key_, &type));
        return static_cast<ConfigType>(type);
    }

    [[nodiscard]] auto array() const -> ConstConfigArray;
    [[nodiscard]] auto map() const -> ConstConfigMap;
    [[nodiscard]] auto value() const -> std::optional<std::string_view> {
        if (intersects(type(), ConfigType::value)) {
            // TODO: combine interface
            bool assigned = false;
            Detail::handle_error(clingo_config_value_is_assigned(cfg_, key_, &assigned));
            if (!assigned) {
                // TODO: need string view interface
                char const *value = nullptr;
                Detail::handle_error(clingo_config_value_get(cfg_, key_, &value));
                return value;
            }
            return std::nullopt;
        }
        throw std::bad_variant_access();
    }
    [[nodiscard]] auto description() const -> std::string_view {
        // TODO: need string view interface/value might be absent?
        char const *ret = nullptr;
        clingo_config_description(cfg_, key_, &ret);
        return ret;
    }

  private:
    friend class Config;

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

class Config : public ConstConfig {
  public:
    explicit Config(clingo_config_t *stats, Id key) : ConstConfig{stats, key} {}
    friend auto c_cast(Config const &stats) -> clingo_config_t * { return stats.cfg_(); }

    [[nodiscard]] auto array() const -> ConfigArray;
    [[nodiscard]] auto map() const -> ConfigMap;
    void value(std::string_view value) const {
        if (intersects(type(), ConfigType::value)) {
            // TODO: need string view interface
            Detail::handle_error(clingo_config_value_set(cfg_(), key_, std::string{value}.c_str()));
        } else {
            throw std::bad_variant_access{};
        }
    }

  private:
    [[nodiscard]] auto cfg_() const -> clingo_config_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_config_t *>(ConstConfig::cfg_);
    }
};

class ConstConfigArray {
  public:
    using value_type = ConstConfig;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConstConfigArray>;

    explicit ConstConfigArray(clingo_config_t const *stats, Id key) : cfg_{stats}, key_{key} {}

    [[nodiscard]] auto at(size_t index) const -> ConstConfig { return ConstConfig{cfg_, at_(index)}; }
    [[nodiscard]] auto operator[](size_t index) const -> ConstConfig { return at(index); }
    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        Detail::handle_error(clingo_config_array_size(cfg_, key_, &size));
        return size;
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class ConfigArray;
    [[nodiscard]] auto at_(size_t index) const -> clingo_id_t {
        clingo_id_t subkey = 0;
        Detail::handle_error(clingo_config_array_at(cfg_, key_, index, &subkey));
        return subkey;
    }

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

[[nodiscard]] inline auto ConstConfig::array() const -> ConstConfigArray {
    if (intersects(type(), ConfigType::array)) {
        return ConstConfigArray{cfg_, key_};
    }
    throw std::bad_variant_access{};
}

class ConfigArray : public ConstConfigArray {
  public:
    using value_type = Config;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConfigArray>;

    explicit ConfigArray(clingo_config_t *stats, Id key) : ConstConfigArray{stats, key} {}

    [[nodiscard]] auto at(size_t index) const -> Config { return Config{cfg_(), at_(index)}; }
    [[nodiscard]] auto operator[](size_t index) const -> Config { return at(index); }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto cfg_() const -> clingo_config_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_config_t *>(ConstConfigArray::cfg_);
    }
};

[[nodiscard]] inline auto Config::array() const -> ConfigArray {
    if (type() == ConfigType::array) {
        return ConfigArray{cfg_(), key_};
    }
    throw std::bad_variant_access{};
}

class ConstConfigMap {
  public:
    using key_type = std::string_view;
    using mapped_type = ConstConfig;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConstConfigMap>;

    explicit ConstConfigMap(clingo_config_t const *stats, Id key) : cfg_{stats}, key_{key} {}

    [[nodiscard]] auto size() const -> size_t {
        size_t size = 0;
        Detail::handle_error(clingo_config_map_size(cfg_, key_, &size));
        return size;
    }
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstConfig{cfg_, subkey}};
    }
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstConfig { return ConstConfig{cfg_, at_(name)}; }
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        auto res = false;
        // TODO: need string_view interface
        Detail::handle_error(clingo_config_map_has_subkey(cfg_, key_, std::string{name}.c_str(), &res));
        // Detail::handle_error(clingo_config_map_has_subkey(cfg_, key_, name.data(), name.size(), &res));
        return res;
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class ConfigMap;

    [[nodiscard]] auto at_(std::string_view name) const -> clingo_id_t {
        clingo_id_t subkey = 0;
        // TODO: need string_view interface
        Detail::handle_error(clingo_config_map_at(cfg_, key_, std::string{name}.c_str(), &subkey));
        // Detail::handle_error(clingo_config_map_at(cfg_, key_, name.data(), name.size(), &subkey));
        return subkey;
    }

    [[nodiscard]] auto at_(size_t index) const -> std::pair<std::string_view, clingo_id_t> {
        // TODO: need string_view interface
        // clingo_string_t name;
        char const *name = nullptr;
        Detail::handle_error(clingo_config_map_subkey_name(cfg_, key_, index, &name));
        // auto str = std::string_view{name.data, name.size};
        auto str = std::string_view{name};
        return {str, at_(str)};
    }

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

[[nodiscard]] inline auto ConstConfig::map() const -> ConstConfigMap {
    if (intersects(type(), ConfigType::map)) {
        return ConstConfigMap{cfg_, key_};
    }
    throw std::bad_variant_access{};
}

class ConfigMap : public ConstConfigMap {
  public:
    using key_type = std::string_view;
    using mapped_type = Config;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConfigMap>;

    explicit ConfigMap(clingo_config_t *stats, Id key) : ConstConfigMap{stats, key} {}

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, Config{cfg_(), subkey}};
    }
    [[nodiscard]] auto operator[](std::string_view name) const -> Config { return Config{cfg_(), at_(name)}; }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto cfg_() const -> clingo_config_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_config_t *>(ConstConfigMap::cfg_);
    }
};

[[nodiscard]] inline auto Config::map() const -> ConfigMap {
    if (intersects(type(), ConfigType::map)) {
        return ConfigMap{cfg_(), key_};
    }
    throw std::bad_variant_access{};
}

} // namespace Clingo
