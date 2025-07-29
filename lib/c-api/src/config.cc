#include <clingo/config.h>

#include <clasp/cli/clasp_options.h>

#include "clingo/control.h"
#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

#include <algorithm>
#include <map>

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

// TODO:
// - extending the config with custom keys
// - wrap config in custom config
// - the config should live in the control object
// - it should work even if there is not associated solver

class ClingoConfig {
  public:
    //! The type of configuration keys to refer to entries.
    //!
    //! Each configuration key is represented as unsigned integer.
    using KeyType = Clasp::Cli::ClaspCliConfig::KeyType;
    //! Type used for array indexing of configuration entries.
    using IndexType = KeyType;
    //! The available value types of configuration entries.
    enum class ValueFlags : uint8_t {
        none = 0,  //!< The entry cannot have a value.
        get = 1,   //<! The entry can be read.
        set = 2,   //!< The entry can be set.
        value = 4, //!< The entry has a value.
    };
    CLINGO_ENABLE_BITSET_ENUM(ValueFlags, friend);

    // A configuration entry interface to interact with configuration entries.
    //
    // This interface provides methods to check the value type of an entry, get and set values,
    // and retrieve the size of array entries.
    //
    // Array handling is special. Configuration entries are arranged in a tree
    // structure, where each path to a leave can have at most one array entry.
    // If an entry has a parent that is an array, the index is passed to
    // methods that require it.
    class Entry {
      public:
        Entry() = default;
        Entry(Entry &&other) = delete;
        auto operator=(Entry &&other) -> Entry & = delete;
        virtual ~Entry() = default;

        //! Get information about the value of the entry.
        [[nodiscard]] auto value_info(std::optional<IndexType> index) -> ValueFlags { return do_value_type(index); }
        //! Get the value of the entry.
        void get_value(std::optional<IndexType> index, std::string &value) { do_get_value(index, value); }
        //! Set a new value for the entry.
        void set_value(std::optional<KeyType> index, std::string_view value) { do_set_value(index, value); }
        //! Returns the number of elements of an array entry.
        //!
        //! Returns std::nullopt if the entry is not an array.
        auto size_array() -> std::optional<int> { return do_array_size(); }

      private:
        virtual auto do_value_type(std::optional<KeyType> index) -> ValueFlags = 0;
        virtual void do_get_value(std::optional<KeyType> index, std::string &value) = 0;
        virtual void do_set_value(std::optional<KeyType> index, std::string_view value) = 0;
        virtual auto do_array_size() -> std::optional<int> = 0;
    };
    ClingoConfig(Clasp::Cli::ClaspCliConfig *config) : config_{config} {}

    static auto key_root() -> KeyType { return Clasp::Cli::ClaspCliConfig::key_root; }
    static auto key_invalid() -> KeyType { return Clasp::Cli::ClaspCliConfig::key_invalid; }
    static auto index_invalid() -> KeyType { return KeyType(-1); }

    //! Retrieves information about the specified key.
    //!
    //! @param key a handle to a key
    //! @param n_children the number of subkeys for this key
    //! @param array_info the length of array keys or -1 for non-array keys
    //! @param value_info The number of values the key currently has (0 or 1), or -1 if it cannot have values.
    //! @param help A description of the key.
    //! @note All out parameters are optional (i.e., can be null).
    //! @return The number of output values written, or -1 if the key is invalid.
    void key_info(KeyType key, int *n_children, int *array_info, ValueFlags *value_info, std::string *help) const {
        init_(n_children, 0);
        init_(array_info, -1);
        init_(value_info, ValueFlags::none);
        if (help != nullptr) {
            help->clear();
        }
        if (auto clingo_key = is_clingo(key)) {
            clingo_key_info_(*clingo_key, n_children, array_info, value_info, help);
        } else {
            int n_values = 0;
            if (auto ret = config_->getKeyInfo(key, n_children, array_info, help, &n_values); ret == -1) {
                error_("invalid key");
            }
            if (value_info != nullptr) {
                if (n_values < 0) {
                    *value_info = ValueFlags::none;
                } else if (n_values == 0) {
                    *value_info = ValueFlags::get | ValueFlags::set;
                } else {
                    *value_info = ValueFlags::get | ValueFlags::set | ValueFlags::value;
                }
            }
            // NOTE: the root key is special - it is the only clasp key that can contain clingo sub keys
            if (n_children != nullptr && key == key_root() && !nodes_.empty()) {
                *n_children += static_cast<int>(nodes_.front().children().size());
            }
        }
    }

    [[nodiscard]] auto array_at(KeyType key, KeyType index) const -> KeyType {
        if (auto clingo_key = is_clingo(key)) {
            if (auto *entry = nodes_.at(clingo_key->key_id()).entry(); entry == nullptr || !entry->size_array()) {
                error_("key is not an array");
            }
            return Key{clingo_key->key_id(), index}.rep();
        }
        return check_(config_->getArrKey(key, index));
    }

    [[nodiscard]] auto map_at(KeyType key, std::string_view name) const -> KeyType {
        if (auto clingo_key = is_clingo(key)) {
            if (auto subkey = nodes_.front().map_at(name)) {
                return subkey->rep();
            }
            error_("subkey not found");
        }
        if (key == key_root() && !nodes_.empty()) {
            if (auto subkey = nodes_.front().map_at(name)) {
                return subkey->rep();
            }
        }
        return check_(config_->getKey(key, name));
    }

    [[nodiscard]] auto map_nth(KeyType key, uint32_t index) const -> std::string_view {
        if (auto clingo_key = is_clingo(key)) {
            auto const &node = nodes_.at(clingo_key->key_id());
            if (auto name = node.map_nth(index)) {
                return *name;
            }
            error_("index out of bounds");
        }
        if (key == key_root() && !nodes_.empty()) {
            if (auto name = nodes_.front().map_nth(index)) {
                return *name;
            }
        }
        return config_->getSubkey(key, index);
    }

    auto set_value(KeyType key, std::string_view value) -> int { return config_->setValue(key, value); }

    [[nodiscard]] auto get_value(KeyType key) const -> std::string_view {
        thread_local std::string value;
        config_->getValue(key, value);
        return value;
    }

  private:
    static constexpr KeyType Bits = sizeof(KeyType) * 8;
    static constexpr KeyType BitsIndex = 8;
    static constexpr KeyType BitsKeyId = Bits - BitsIndex - 1;

    static constexpr KeyType MaskClingo = KeyType{1} << (Bits - 1);
    static constexpr KeyType MaskKeyId = ((KeyType{1} << BitsKeyId) - 1) << BitsIndex;
    static constexpr KeyType MaskIndex = (KeyType{1} << BitsIndex) - 1;

    struct FromRep {};
    [[maybe_unused]] static constexpr FromRep from_rep{};

    //! Helper to access the components of a configuration key.
    //!
    //! Keys are encoded as follows: Clingo Bit, Key Id Bits, Array Bits
    //! Clingo Bit:
    //! - A clingo key is invalid if this bit is not set.
    //! Key Bits:
    //! - The key id can use all available key id bits.
    //! Array Bits:
    //! - 0: Key is not on a path with an array key
    //! - 1: Key is on a path with an array key that has no index (index_invalid())
    //! - 2: Key is on a path with an array which has index `array bits - 2`
    //! - A key is invalid if all array bits are set.
    class Key {
      public:
        //! Constructs a new Key with the given key_id and optional index.
        //!
        //! Throws if the key cannot represent the given key_id or index.
        explicit Key(KeyType key_id, std::optional<KeyType> index = std::nullopt)
            : rep_{MaskClingo | encode_key_id(key_id) | encode_array(index)} {}

        //! Constructs a Key from a raw representation.
        //!
        //! Throws if the representation does not represent a valid clingo key.
        explicit Key([[maybe_unused]] FromRep tag, KeyType value) : rep_{value} {
            if ((rep_ & MaskIndex) == MaskIndex || (rep_ & MaskClingo) == 0) {
                error_("invalid key");
            }
        }

        //! Get the id of the key.
        [[nodiscard]] auto key_id() const -> KeyType { return (rep_ & MaskKeyId) >> BitsIndex; }
        //! Get the index of the key.
        [[nodiscard]] auto index() const -> std::optional<KeyType> {
            auto res = rep_ & MaskIndex;
            if (res == 0) {
                return std::nullopt;
            }
            if (res == 1) {
                return index_invalid();
            }
            return res - 2;
        }
        //! Transfer the array index of this key to the child key.
        [[nodiscard]] auto subkey(Key child) const -> Key {
            return child.index() ? child : Key{child.key_id(), index()};
        }
        //! Get the raw representation of the key.
        [[nodiscard]] auto rep() const -> KeyType { return rep_; }

        //! Compare two keys for equality.
        [[nodiscard, maybe_unused]] friend auto operator==(Key const &a, Key const &b) -> bool {
            return a.rep_ == b.rep_;
        }
        //! Compare two keys.
        [[nodiscard, maybe_unused]] friend auto operator<=>(Key const &a, Key const &b) -> std::strong_ordering {
            return a.rep_ <=> b.rep_;
        }

      private:
        static auto encode_array(std::optional<KeyType> index) -> KeyType {
            KeyType rep = 0;
            if (index) {
                rep = 1;
                if (*index != index_invalid()) {
                    if (*index + 2 >= MaskIndex) {
                        error_("index size exceeded");
                    }
                    rep = ((*index + 2) & MaskIndex);
                }
            }
            return rep;
        }
        static auto encode_key_id(KeyType key_id) -> KeyType {
            if (key_id > (MaskKeyId >> BitsIndex)) {
                error_("key size exceeded");
            }
            return key_id << BitsIndex;
        }
        KeyType rep_;
    };

    //! A concrete node in the configuration tree.
    class Node {
      public:
        //! Constructs a new Node with the given description and optional updater.
        explicit Node(std::string_view description, std::unique_ptr<Entry> updater)
            : updater_{std::move(updater)}, description_{description} {}

        //! The entry associated with this node.
        [[nodiscard]] auto entry() const -> Entry * { return updater_.get(); }
        //! The description of this node.
        [[nodiscard]] auto description() const -> std::string_view { return description_; }
        //! Theh children of this node, mapping names to keys.
        [[nodiscard]] auto children() -> std::map<std::string, Key, std::less<>> & { return children_; }
        //! Theh children of this node, mapping names to keys.
        [[nodiscard]] auto children() const -> std::map<std::string, Key, std::less<>> const & { return children_; }
        [[nodiscard]] auto map_at(std::string_view name) const -> std::optional<Key> {
            auto it = children_.find(name);
            return it != children_.end() ? std::make_optional(it->second) : std::nullopt;
        }
        [[nodiscard]] auto map_nth(KeyType index) const -> std::optional<std::string_view> {
            auto it = children_.begin();
            for (; index > 0 && it != children_.end(); --index, ++it) {
            }
            return it != children_.end() ? std::make_optional<std::string_view>(it->first) : std::nullopt;
        }

      private:
        std::unique_ptr<Entry> updater_;
        std::map<std::string, Key, std::less<>> children_;
        std::string description_;
    };

    static auto check_(KeyType key) -> KeyType {
        if (key == key_invalid()) {
            error_("invalid key");
        }
        return key;
    }

    //! Helper to emit error messages for configuration issues.
    template <class... T> static void error_(T const &...args) {
        std::ostringstream oss;
        oss << "configuration error: ";
        (oss << ... << args); // NOLINT
        throw std::runtime_error{oss.str()};
    }

    //! Helper to initialize a pointer with a default value if it is not null.
    template <class P> static void init_(P *val, P def) {
        if (val != nullptr) {
            *val = def;
        }
    }

    //! Get information about a clingo configuration key.
    void clingo_key_info_(Key key, int *n_children, int *array_info, ValueFlags *value_info, std::string *help) const {
        auto const &node = nodes_.at(key.key_id());
        if (n_children != nullptr) {
            *n_children = static_cast<int>(node.children().size());
        }
        auto *entry = node.entry();
        if (array_info != nullptr && entry != nullptr) {
            *array_info = entry->size_array().value_or(-1);
        }
        if (help != nullptr) {
            auto desc = node.description();
            help->assign(desc.begin(), desc.end());
        }
        if (value_info != nullptr && entry != nullptr) {
            *value_info = entry->value_info(key.index());
        }
    }

    //! Parses a configuration key name, extracting an optional index if present.
    //!
    //! Accepts key names in the form "key" or "key[index]". If an index is
    //! present, it is parsed and returned (where an empty index represents an
    //! invalid index). The input name is modified to remove any index portion.
    //!
    //! Throws if the key is malformed.
    //!
    //! @param name[inout] reference to the key name
    //! @return an optional index value
    static auto parse_name_(std::string_view &name) -> std::optional<IndexType> {
        auto res = std::optional<KeyType>{};
        if (name.ends_with(']')) {
            auto start = name.find_last_of('[');
            if (start == std::string_view::npos) {
                error_("missing '['");
            }
            auto index_str = name.substr(start + 1, name.size() - start - 2);
            if (index_str.empty()) {
                res = index_invalid();
            } else {
                res = 0;
                auto [ptr, code] = std::from_chars(index_str.data(), index_str.data() + index_str.size(), *res);
                if (code != std::errc{} && ptr != index_str.data() + index_str.size()) {
                    error_("invalid index");
                }
                if (res.value() + 2 >= MaskIndex) {
                    error_("index out of bounds");
                }
            }
            name = name.substr(0, start);
        }
        if (name.empty()) {
            error_("empty key");
        }
        return res;
    }

    //! Resolves a clingo configuration key from a hierarchical path.
    //!
    //! Traverses the configuration tree starting from the given key and
    //! following the path, which may contain dot-separated segments and
    //! optional indices (e.g., "foo.bar[2].baz"). Returns a ClingoKey with the
    //! resolved key id and index.
    //!
    //! The current implementation is limited to at most one index per key
    //! segment.
    //!
    //! @param key The starting ClingoKey for resolution.
    //! @param path The dot-separated configuration path to resolve.
    //! @return The resolved ClingoKey.
    //! @throws std::runtime_error if the path is invalid or resolution fails.
    auto parse_path_(Key key, std::string_view path) -> Key {
        auto key_index = key.index();
        for (size_t start = 0; start < path.size();) {
            auto &cur = nodes_.at(key.key_id());

            size_t dot = path.find('.', start);
            auto last = dot == std::string_view::npos;
            if (last) {
                dot = path.size();
            }
            auto name = path.substr(start, dot - start);
            auto index = parse_name_(name);
            auto it = cur.children().find(name);
            if (it == cur.children().end()) {
                error_("key not found");
            }
            key = it->second;
            if (auto def = key.index()) {
                if (key_index.has_value()) {
                    error_("multiple indices");
                }
                key_index = index.value_or(*def);
            } else if (index) {
                error_("unexpected indices");
            }
            start = dot + 1;
        }

        return Key{key.key_id(), key_index};
    }

    //!  Adds a new clingo configuration key to the configuration tree.
    //!
    //!  Validates the key name format and inserts the key as a child of the
    //!  given parent key. The key name may include an optional index in the form
    //!  "key[index]" to add an array key. The index of such a key is used as
    //!  default index when the key is accessed without an index.
    //!
    //!  If an updater is provided, it will be associated with the key to handle
    //!  changes to the key's value.
    //!
    //!  @param key Parent clingo key under which the new key is added.
    //!  @param name Name of the key, possibly with an index.
    //!  @param description Description string for the key.
    //!  @param updater Optional unique pointer to a ConfigUpdater for the key.
    void add_clingo_key_(Key key, std::string_view name, std::string_view description,
                         std::unique_ptr<Entry> updater = nullptr) {
        auto dot = name.rfind('.');
        if (dot == std::string_view::npos) {
            dot = name.size();
            key = parse_path_(key, name.substr(0, dot));
            name = name.substr(dot + 1);
        }

        auto index = parse_name_(name);
        if (std::isalpha(static_cast<unsigned char>(name.front())) == 0 && name.front() != '_') {
            error_("invalid key");
        }
        if (!std::ranges::all_of(name,
                                 [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; })) {
            error_("invalid key");
        }

        auto &entry = nodes_.at(key.key_id());
        if (entry.children().contains(name)) {
            error_("key exists");
        }

        auto sub_key = Key{static_cast<KeyType>(nodes_.size()), index};
        entry.children().emplace(name, sub_key);
        nodes_.emplace_back(description, std::move(updater));
    }

    // Checks if this key is a clasp key.
    //
    // Returns true if the upper bit is not set.
    [[nodiscard]] static auto is_clasp(KeyType key) -> bool { return (key & MaskClingo) == 0; }

    // Checks if this key is a clingo key.
    //
    // Returns true if the upper bit is set and the key is not the invalid key.
    [[nodiscard]] static auto is_clingo(KeyType key) -> std::optional<Key> {
        return !is_clasp(key) ? std::make_optional<Key>(key) : std::nullopt;
    }

    Clasp::Cli::ClaspCliConfig *config_;
    std::vector<Node> nodes_;
};

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
        auto str = cpp_cast(config)->getSubkey(key, offset);
        name->data = str.data();
        name->size = str.size();
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
