#pragma once

#include <clingo/core.hh>

#include <clingo/config.h>

namespace Clingo {

//! @addtogroup cpp_config
//! Configuration of search and enumeration algorithms.
//!
//! Entries in a configuration are organized hierarchically. Subentries are
//! either accessed by name for map entries or by offset for array entries.
//! Value entries have a string value that can be inspected or modified.
//!
//! @{

//! Enumeration of configuration types.
enum class ConfigType : clingo_config_type_bitset_t {
    //! The configuration entry is a double value.
    value = clingo_config_type_value,
    //! The configuration entry is a array of configurations.
    array = clingo_config_type_array,
    //! The configuration entry is a map of configurations.
    map = clingo_config_type_map,
};
CLINGO_ENABLE_BITSET_ENUM(ConfigType);

class ConstConfigArray;
class ConfigArray;
class ConstConfigMap;
class ConfigMap;

//! Class modeling an immutable configuration entry.
class ConstConfig {
  public:
    //! Construct from the underlying C API type and a key.
    //!
    //! @param stats the underlying C API type
    //! @param key the key of the configuration entry
    explicit ConstConfig(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}

    //! Cast the configuration to the underlying C API type.
    //!
    //! @param stats the configuration to cast
    //! @reuturn the underlying C API type
    friend auto c_cast(ConstConfig const &stats) -> clingo_config_t const * { return stats.cfg_; }

    //! Get the type of the configuration entry.
    //!
    //! @return the type of the configuration entry
    [[nodiscard]] auto type() const -> ConfigType {
        return static_cast<ConfigType>(Detail::call<clingo_config_type>(cfg_, key_));
    }

    //! Access the configuration entry as an array.
    //!
    //! @return the configuration entry as an array
    [[nodiscard]] auto array() const -> ConstConfigArray;

    //! Get the configuration entry at the given index in the array.
    //!
    //! @param index the index of the configuration entry
    //! @return the configuration entry at the given index
    [[nodiscard]] auto at(size_t index) const -> ConstConfig;

    //! @copydoc ConstConfig::at()
    [[nodiscard]] auto operator[](size_t index) const -> ConstConfig { return at(index); }

    //! Access the configuration entry as a map.
    //!
    //! @return The configuration entry as a map.
    [[nodiscard]] auto map() const -> ConstConfigMap;

    //! Get the configuration entry with the given name in the map.
    //!
    //! @param name the name of the configuration entry in the map
    //! @return the configuration entry with the given name
    [[nodiscard]] auto get(std::string_view name) const -> ConstConfig;

    //! @copydoc ConstConfig::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstConfig { return get(name); }

    //! Get the value of the configuration entry.
    //!
    //! The return might be empty if no value is assigned to the configuration
    //! entry.
    //!
    //! @return the value of the configuration entry
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
        throw std::logic_error{"not a value"};
    }

    //! @copydoc ConstConfig::value()
    [[nodiscard]] auto operator*() const -> std::optional<std::string_view> { return value(); }

    //! Get the description of the configuration entry.
    //!
    //! The description corresponds to the text that appears in the help output
    //! of clingo or derived systems.
    //!
    //! @return the description of the configuration entry
    [[nodiscard]] auto description() const -> std::string_view {
        auto [data, size] = Detail::call<clingo_config_description>(cfg_, key_);
        return {data, size};
    }

    //! Get a string representation of the configuration entry.
    //!
    //! This returns a string in a YAML-like format that can be used to inspect
    //! the configuration entry.
    //!
    //! @return a string representation of the configuration entry
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

//! Class modeling a mutable configuration entry.
class Config : public ConstConfig {
  public:
    //! Construct from the underlying C API type and a key.
    //!
    //! @param stats the underlying C API type
    //! @param key the key of the configuration entry
    explicit Config(clingo_config_t *stats, ProgramId key) : ConstConfig{stats, key} {}

    //! Cast the configuration to the underlying C API type.
    //!
    //! @param stats the configuration to cast
    //! @return the underlying C API type
    friend auto c_cast(Config const &stats) -> clingo_config_t * { return stats.cfg_(); }

    //! @copydoc ConstConfig::array()
    [[nodiscard]] auto array() const -> ConfigArray;
    //! @copydoc ConstConfig::at()
    [[nodiscard]] auto at(size_t index) const -> Config;
    //! @copydoc ConstConfig::at()
    [[nodiscard]] auto operator[](size_t index) const -> Config { return at(index); }
    //! @copydoc ConstConfig::map()
    [[nodiscard]] auto map() const -> ConfigMap;
    //! @copydoc ConstConfig::get()
    [[nodiscard]] auto get(std::string_view name) const -> Config;
    //! @copydoc ConstConfig::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> Config { return get(name); }
    using ConstConfig::value;
    //! Set the value of the configuration entry.
    //!
    //! @param value the new value of the configuration entry
    void value(std::string_view value) const {
        if (intersects(type(), ConfigType::value)) {
            Detail::handle_error(clingo_config_value_set(cfg_(), key_, value.data(), value.size()));
        } else {
            throw std::logic_error{"not a value"};
        }
    }
    //! @copydoc Config::value()
    //! @return the configuration entry itself
    auto operator=(std::string_view value) const -> Config { // NOLINT
        this->value(value);
        return *this;
    }

  private:
    [[nodiscard]] auto cfg_() const -> clingo_config_t * {
        // NOLINTNEXTLINE
        return const_cast<clingo_config_t *>(ConstConfig::cfg_);
    }
};

//! Class modeling an immutable array of configuration entries.
class ConstConfigArray {
  public:
    //! The value type.
    using value_type = ConstConfig;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<ConstConfigArray>;

    //! Construct from the underlying C API type and a key.
    //!
    //! @param stats the underlying C API type
    //! @param key the key of the configuration entry
    explicit ConstConfigArray(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}

    //! @copydoc ConstConfig::at()
    [[nodiscard]] auto at(size_t index) const -> ConstConfig { return ConstConfig{cfg_, at_(index)}; }
    //! @copydoc ConstConfig::at()
    [[nodiscard]] auto operator[](size_t index) const -> ConstConfig { return at(index); }
    //! Get the size of the array.
    //!
    //! @return the size of the array
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_config_array_size>(cfg_, key_); }
    //! Get an iterator to the beginning of the array.
    //!
    //! @return an iterator to the beginning of the array
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    //! Get an iterator to the end of the array.
    //!
    //! @return an iterator to the end of the array
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
    throw std::logic_error{"not an array"};
}

inline auto ConstConfig::at(size_t index) const -> ConstConfig {
    return array().at(index);
}

//! Class modeling a mutable array of configuration entries.
class ConfigArray : public ConstConfigArray {
  public:
    //! The value type.
    using value_type = Config;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<ConfigArray>;

    //! @copydoc ConstConfigArray::ConstConfigArray()
    explicit ConfigArray(clingo_config_t *stats, ProgramId key) : ConstConfigArray{stats, key} {}

    //! @copydoc ConstConfigArray::at()
    [[nodiscard]] auto at(size_t index) const -> Config { return Config{cfg_(), at_(index)}; }
    //! @copydoc ConstConfigArray::at()
    [[nodiscard]] auto operator[](size_t index) const -> Config { return at(index); }
    //! @copydoc ConstConfigArray::begin()
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    //! @copydoc ConstConfigArray::begin()
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
    throw std::logic_error{"not an array"};
}

inline auto Config::at(size_t index) const -> Config {
    return array().at(index);
}

//! Class modeling an immutable map of configuration entries.
class ConstConfigMap {
  public:
    //! The key type.
    using key_type = std::string_view;
    //! The mapped type.
    using mapped_type = ConstConfig;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<ConstConfigMap>;

    //! Construct from the underlying C API type and a key.
    //!
    //! @param stats the underlying C API type
    //! @param key the key of the configuration entry
    explicit ConstConfigMap(clingo_config_t const *stats, ProgramId key) : cfg_{stats}, key_{key} {}

    //! Get the size of the map.
    //!
    //! @return the size of the map
    [[nodiscard]] auto size() const -> size_t { return Detail::call<clingo_config_map_size>(cfg_, key_); }

    //! Get the name configuration entry pair at the given index in the map.
    //!
    //! @param index the index of the configuration entry
    //! @return the name configuration entry pair at the given index
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, ConstConfig{cfg_, subkey}};
    }

    //! Get the configuration entry with the given name in the map.
    //!
    //! @param name the name of the configuration entry
    //! @return the configuration entry with the given name
    [[nodiscard]] auto get(std::string_view name) const -> ConstConfig { return ConstConfig{cfg_, at_(name)}; }

    //! @copydoc ConstConfigMap::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> ConstConfig { return get(name); }

    //! Check if the map contains a configuration entry with the given name.
    //!
    //! @param name the name of the configuration entry
    //! @return true if the map contains an entry with the given name, false otherwise
    [[nodiscard]] auto contains(std::string_view name) const -> bool {
        return Detail::call<clingo_config_map_has_subkey>(cfg_, key_, name.data(), name.size());
    }

    //! Get an iterator to the beginning of the map.
    //!
    //! @return an iterator to the beginning of the map
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }

    //! Get an iterator to the end of the map.
    //!
    //! @return an iterator to the end of the map
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
    throw std::logic_error{"not a map"};
}

inline auto ConstConfig::get(std::string_view name) const -> ConstConfig {
    return map().get(name);
}

//! Class modeling a mutable map of configuration entries.
class ConfigMap : public ConstConfigMap {
  public:
    //! The key type.
    using key_type = std::string_view;
    //! The mapped type.
    using mapped_type = Config;
    //! The value type.
    using value_type = std::pair<key_type, mapped_type>;
    //! The size type.
    using size_type = std::size_t;
    //! The difference type.
    using difference_type = std::ptrdiff_t;
    //! The reference type.
    using reference = value_type;
    //! The pointer type.
    using pointer = Detail::ArrowProxy<value_type>;
    //! The iterator type.
    using iterator = Detail::RandomAccessIterator<ConfigMap>;

    //! @copydoc ConstConfigMap::ConstConfigMap()
    explicit ConfigMap(clingo_config_t *stats, ProgramId key) : ConstConfigMap{stats, key} {}

    //! @copydoc ConstConfigMap::at()
    [[nodiscard]] auto at(size_t index) const -> value_type {
        auto [name, subkey] = at_(index);
        return {name, Config{cfg_(), subkey}};
    }
    //! @copydoc ConstConfigMap::get()
    [[nodiscard]] auto get(std::string_view name) const -> Config { return Config{cfg_(), at_(name)}; }
    //! @copydoc ConstConfigMap::get()
    [[nodiscard]] auto operator[](std::string_view name) const -> Config { return get(name); }
    //! @copydoc ConstConfigMap::begin()
    [[nodiscard]] auto begin() const -> iterator { return iterator{*this, 0}; }
    //! @copydoc ConstConfigMap::end()
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
    throw std::logic_error{"not a map"};
}

inline auto Config::get(std::string_view name) const -> Config {
    return map().get(name);
}

//! @}

} // namespace Clingo
