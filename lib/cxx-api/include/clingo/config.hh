#pragma once

#include <clingo/core.hh>

#include <clingo/config.h>

#include <variant>

namespace Clingo {

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
    explicit ConstConfig(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}
    friend auto c_cast(ConstConfig const &stats) -> clingo_config_t const * { return stats.cfg_; }

    [[nodiscard]] auto type() const -> ConfigType {
        return static_cast<ConfigType>(Detail::call<clingo_config_type>(cfg_, key_));
    }

    [[nodiscard]] auto array() const -> ConstConfigArray;
    [[nodiscard]] auto at(size_t index) const -> ConstConfig;
    [[nodiscard]] auto operator[](size_t index) const -> ConstConfig { return at(index); }
    [[nodiscard]] auto map() const -> ConstConfigMap;
    [[nodiscard]] auto get(std::string_view name) const -> ConstConfig;
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstConfig { return get(name); }
    [[nodiscard]] auto value() const -> std::optional<std::string_view> {
        if (intersects(type(), ConfigType::value)) {
            clingo_string_t value;
            bool assigned = false;
            Detail::handle_error(clingo_config_value_get(cfg_, key_, &value, &assigned));
            if (assigned) {
                return std::string_view{value.data, value.size};
            }
            return std::nullopt;
        }
        throw std::bad_variant_access();
    }
    [[nodiscard]] auto operator*() const -> std::optional<std::string_view> { return value(); }
    [[nodiscard]] auto description() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_config_description>(cfg_, key_);
        return {data, size};
    }
    [[nodiscard]] auto to_string() const -> std::string {
        auto bld = StringBuilder{};
        Detail::handle_error(clingo_config_to_string(cfg_, key_, c_cast(bld)));
        return std::string{bld.str()};
    }

  private:
    friend class Config;

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

class Config : public ConstConfig {
  public:
    explicit Config(clingo_config_t *stats, ProgramId key) : ConstConfig{stats, key} {}
    friend auto c_cast(Config const &stats) -> clingo_config_t * { return stats.cfg_(); }

    [[nodiscard]] auto array() const -> ConfigArray;
    [[nodiscard]] auto at(size_t index) const -> Config;
    [[nodiscard]] auto operator[](size_t index) const -> Config { return at(index); }
    [[nodiscard]] auto map() const -> ConfigMap;
    [[nodiscard]] auto get(std::string_view name) const -> Config;
    [[nodiscard]] auto operator[](std::string_view name) const -> Config { return get(name); }
    using ConstConfig::value;
    void value(std::string_view value) const {
        if (intersects(type(), ConfigType::value)) {
            Detail::handle_error(clingo_config_value_set(cfg_(), key_, value.data(), value.size()));
        } else {
            throw std::bad_variant_access{};
        }
    }
    auto operator=(std::string_view value) -> Config & {
        this->value(value);
        return *this;
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

    explicit ConstConfigArray(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}

    [[nodiscard]] auto at(size_t index) const -> ConstConfig { return ConstConfig{cfg_, at_(index)}; }
    [[nodiscard]] auto operator[](size_t index) const -> ConstConfig { return at(index); }
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_config_array_size>(cfg_, key_); }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class ConfigArray;
    [[nodiscard]] auto at_(size_t index) const -> clingo_id_t {
        return Detail::call<clingo_config_array_at>(cfg_, key_, index);
    }

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

inline auto ConstConfig::array() const -> ConstConfigArray {
    if (intersects(type(), ConfigType::array)) {
        return ConstConfigArray{cfg_, key_};
    }
    throw std::bad_variant_access{};
}

inline auto ConstConfig::at(size_t index) const -> ConstConfig {
    return array().at(index);
}

class ConfigArray : public ConstConfigArray {
  public:
    using value_type = Config;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type;
    using pointer = Detail::ArrowProxy<value_type>;
    using iterator = Detail::RandomAccessIterator<ConfigArray>;

    explicit ConfigArray(clingo_config_t *stats, ProgramId key) : ConstConfigArray{stats, key} {}

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
    if (intersects(type(), ConfigType::array)) {
        return ConfigArray{cfg_(), key_};
    }
    throw std::bad_variant_access{};
}

inline auto Config::at(size_t index) const -> Config {
    return array().at(index);
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

    explicit ConstConfigMap(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}

    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_config_map_size>(cfg_, key_); }
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstConfig{cfg_, subkey}};
    }
    [[nodiscard]] auto get(std::string_view name) const -> ConstConfig { return ConstConfig{cfg_, at_(name)}; }
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstConfig { return get(name); }
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        return Detail::call<clingo_config_map_has_subkey>(cfg_, key_, name.data(), name.size());
    }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    friend class ConfigMap;

    [[nodiscard]] auto at_(std::string_view name) const -> clingo_id_t {
        return Detail::call<clingo_config_map_at>(cfg_, key_, name.data(), name.size());
    }

    [[nodiscard]] auto at_(size_t index) const -> std::pair<std::string_view, clingo_id_t> {
        auto [data, size] = Detail::call<clingo_config_map_subkey_name>(cfg_, key_, index);
        auto str = std::string_view{data, size};
        return {str, at_(str)};
    }

    clingo_config_t const *cfg_;
    clingo_id_t key_;
};

inline auto ConstConfig::map() const -> ConstConfigMap {
    if (intersects(type(), ConfigType::map)) {
        return ConstConfigMap{cfg_, key_};
    }
    throw std::bad_variant_access{};
}

inline auto ConstConfig::get(std::string_view name) const -> ConstConfig {
    return map().get(name);
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

    explicit ConfigMap(clingo_config_t *stats, ProgramId key) : ConstConfigMap{stats, key} {}

    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, Config{cfg_(), subkey}};
    }
    [[nodiscard]] auto get(std::string_view name) const -> Config { return Config{cfg_(), at_(name)}; }
    [[nodiscard]] auto operator[](std::string_view name) const -> Config { return get(name); }
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    [[nodiscard]] auto end() const -> iterator { return iterator{*this, size()}; }

  private:
    [[nodiscard]] auto cfg_() const -> clingo_config_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_config_t *>(ConstConfigMap::cfg_);
    }
};

inline auto Config::map() const -> ConfigMap {
    if (intersects(type(), ConfigType::map)) {
        return ConfigMap{cfg_(), key_};
    }
    throw std::bad_variant_access{};
}

inline auto Config::get(std::string_view name) const -> Config {
    return map().get(name);
}

} // namespace Clingo
