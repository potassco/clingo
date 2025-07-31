#pragma once

#include <clingo/util/enum.hh>
#include <clingo/util/print.hh>

#include <clasp/cli/clasp_options.h>

#include <map>
#include <sstream>

namespace CppClingo::Control {

//! This class provides a hierarchical configuration interface for clingo.
//!
//! It wraps a ClaspCliConfig object and allows managing configuration entries
//! as a tree of keys, supporting both map and array entries. The class
//! provides methods to query, retrieve, and set configuration values, as well
//! as to add custom entries. Keys are encoded to distinguish between clingo
//! and clasp entries, and array/map navigation is supported via dedicated
//! methods.
//!
//! Each instance should be used from a single thread. Methods throw on invalid
//! operations or errors.
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
        none = 0, //!< The entry cannot have a value.
        get = 1,  //<! The entry can be read.
        set = 2,  //!< The entry can be set.
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
        using ValueFlags = ClingoConfig::ValueFlags;
        using KeyType = ClingoConfig::KeyType;

        Entry() = default;
        Entry(Entry &&other) = delete;
        auto operator=(Entry &&other) -> Entry & = delete;
        virtual ~Entry() = default;

        //! Get information about the value of the entry.
        [[nodiscard]] auto value_info(std::optional<IndexType> index) -> ValueFlags { return do_value_type(index); }
        //! Get the value of the entry.
        auto get_value(std::optional<IndexType> index, std::string &value) -> bool {
            return do_get_value(index, value);
        }
        //! Set a new value for the entry.
        void set_value(std::optional<KeyType> index, std::string_view value) { do_set_value(index, value); }
        //! Returns the number of elements of an array entry.
        //!
        //! Returns std::nullopt if the entry is not an array.
        auto size_array() -> std::optional<int> { return do_array_size(); }

      private:
        virtual auto do_value_type([[maybe_unused]] std::optional<KeyType> index) -> ValueFlags {
            return ValueFlags::none;
        }
        virtual auto do_get_value([[maybe_unused]] std::optional<KeyType> index, [[maybe_unused]] std::string &value)
            -> bool {
            error_("cannot get value");
            return false;
        }
        virtual void do_set_value([[maybe_unused]] std::optional<KeyType> index,
                                  [[maybe_unused]] std::string_view value) {
            error_("cannot set value");
        }
        virtual auto do_array_size() -> std::optional<int> { return std::nullopt; }
    };

    //! Constructor for ClingoConfig.
    //!
    //! @param config a reference to the Clasp CLI configuration object
    ClingoConfig(Clasp::Cli::ClaspCliConfig &config) : config_{&config} {}

    //! The root key of the configuration.
    static auto key_root() -> KeyType { return Clasp::Cli::ClaspCliConfig::key_root; }

    //! Retrieves information about the specified key.
    //!
    //! @note All out parameters are optional (i.e., can be null).
    //!
    //! @param key a handle to a key
    //! @param n_children the number of subkeys for this key
    //! @param array_info the length of array keys or -1 for non-array keys
    //! @param value_info whether the key has a value, can be read, and can be set
    //! @param help a description of the key
    void key_info(KeyType key, int *n_children, int *array_info, ValueFlags *value_info) const;

    //! Get the description of a configuration entry.
    //!
    //! @param key key of the configuration entry
    //! @return a string view of the description
    auto description(KeyType key) const -> std::string_view;

    //! Get the key for an element at a specific index in an array entry.
    //!
    //! Returns the key corresponding to the element at the given index in the
    //! array configuration entry specified by `key`. Throws if the entry is
    //! not an array or the index is invalid.
    //!
    //! @param key key of the array entry
    //! @param index index of the array element
    //! @return key for the array element at the given index
    [[nodiscard]] auto array_at(KeyType key, KeyType index) const -> KeyType;

    //! Get the key for a named subkey in a map entry.
    //!
    //! Returns the key corresponding to the subkey with the given path in the
    //! map configuration entry specified by `key`. Throws if the subkey does
    //! not exist.
    //!
    //! @param key key of the map entry
    //! @param path path of the subkey
    //! @return key for the named subkey
    [[nodiscard]] auto map_at(KeyType key, std::string_view path) const -> std::optional<KeyType>;

    //! Get the name of the nth subkey in a map entry.
    //!
    //! Returns the name of the subkey at the given index in the map
    //! configuration entry specified by `key`. Throws if the index is out of
    //! bounds.
    //!
    //! @param key key of the map entry
    //! @param index index of the subkey
    //! @return name of the nth subkey
    [[nodiscard]] auto map_nth(KeyType key, KeyType index) const -> std::string_view;

    //! Get the value of a configuration entry.
    //!
    //! Returns the value associated with the given configuration key.
    //! Throws if the key does not have a value or cannot be accessed.
    //!
    //! @param key key of the configuration entry
    //! @return value of the configuration entry
    [[nodiscard]] auto get_value(KeyType key) const -> std::optional<std::string_view>;

    //! Set the value of a configuration entry.
    //!
    //! Sets the value associated with the given configuration key.
    //! Throws if the key cannot be set or the value is invalid.
    //!
    //! @param key key of the configuration entry
    //! @param value value to set
    void set_value(KeyType key, std::string_view value);

    //!  Adds a new clingo configuration entry to the configuration tree.
    //!
    //!  Validates the key name format and inserts the key as a child of the
    //!  given parent key. The key name may include an optional index in the form
    //!  "key[index]" to add an array key. The index of such a key is used as
    //!  default index when the key is accessed without an index.
    //!
    //!  If an updater is provided, it will be associated with the key to handle
    //!  changes to the key's value.
    //!
    //!  @param key parent clingo key under which the new key is added
    //!  @param name name of the key
    //!  @param description description string for the key
    //!  @param entry optional entry to handle arrays and values
    void add_entry(KeyType key, std::string_view name, std::string_view description,
                   std::unique_ptr<Entry> entry = nullptr);

    //! Get the string representation of a configuration tree.
    //!
    //! @param out the output buffer for the string representation
    //! @param key the key to start from
    void str(Util::OutputBuffer &out, KeyType key) const;

    //! Get the underlying clasp configuration object.
    [[nodiscard]] auto clasp() const -> Clasp::Cli::ClaspCliConfig & { return *config_; }

  private:
    static constexpr KeyType Bits = sizeof(KeyType) * 8;
    static constexpr KeyType BitsIndex = 8;
    static constexpr KeyType BitsKeyId = Bits - BitsIndex - 1;

    static constexpr KeyType MaskClingo = KeyType{1} << (Bits - 1);
    static constexpr KeyType MaskKeyId = ((KeyType{1} << BitsKeyId) - 1) << BitsIndex;
    static constexpr KeyType MaskIndex = (KeyType{1} << BitsIndex) - 1;

    //! An invalid index for array keys.
    //!
    //! This index is used to mark that an array key is not associated with a
    //! specific index.
    static auto index_invalid() -> KeyType { return KeyType(-1); }
    //! An invalid key for configuration entries.
    //!
    //! This key is used internally for error handling.
    static auto key_invalid() -> KeyType { return Clasp::Cli::ClaspCliConfig::key_invalid; }

    struct FromRep {};
    [[maybe_unused]] static constexpr FromRep from_rep{};

    //! Checks if the given key is valid and returns it.
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
        [[nodiscard]] auto key_id() const -> KeyType;
        //! Get the index of the key.
        [[nodiscard]] auto index() const -> std::optional<KeyType>;
        //! Transfer the array index of this key to the child key.
        [[nodiscard]] auto subkey(Key child) const -> Key;
        //! Get the raw representation of the key.
        [[nodiscard]] auto rep() const -> KeyType;

        //! Compare two keys for equality.
        [[nodiscard, maybe_unused]] friend auto operator==(Key const &a, Key const &b) -> bool = default;
        //! Compare two keys.
        [[nodiscard, maybe_unused]] friend auto operator<=>(Key const &a, Key const &b)
            -> std::strong_ordering = default;

      private:
        static auto encode_array(std::optional<KeyType> index) -> KeyType;
        static auto encode_key_id(KeyType key_id) -> KeyType;

        KeyType rep_;
    };

    //! A concrete node in the configuration tree.
    class Node {
      public:
        //! Constructs a new Node with the given description and optional entry.
        explicit Node(std::string_view description, std::unique_ptr<Entry> entry)
            : entry_{std::move(entry)}, description_{description} {}

        //! The description of this node.
        [[nodiscard]] auto description() const -> std::string_view;
        //! Get the number of entries of tihs node.
        [[nodiscard]] auto entries() const -> int;

        //! Get the key for a subkey with the given name.
        [[nodiscard]] auto map_at(Key key, std::string_view name) const -> std::optional<Key>;
        //! Get the name of the nth subkey in this node.
        [[nodiscard]] auto map_nth(KeyType index) const -> std::optional<std::string_view>;
        //! Get the key for an array entry at the given index.
        [[nodiscard]] auto array_at(Key key, KeyType index) const -> Key;
        //! Get the value of this key.
        auto get_value(std::optional<KeyType> index, std::string &value) const -> bool;
        //! Set the value of this key.
        void set_value(std::optional<KeyType> index, std::string_view value);
        //! Get information about this key.
        void info(Key key, int *n_children, int *array_info, ValueFlags *value_info) const;
        //! Adds a subkey to this node.
        void add_subkey(std::string_view name, Key parent, Key child);

      private:
        std::unique_ptr<Entry> entry_;
        std::map<std::string, Key, std::less<>> subkeys_;
        std::string description_;
    };

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
    static auto parse_name_(std::string_view &name) -> std::optional<IndexType>;

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
    //! @param key the starting ClingoKey for resolution
    //! @param path the dot-separated configuration path to resolve
    //! @return the resolved ClingoKey
    //! @throws std::runtime_error if the path is invalid
    auto parse_path_(Key key, std::string_view path) const -> std::optional<Key>;
    // Checks if this key is a clasp key.
    //
    // Returns true if the upper bit is not set.
    [[nodiscard]] static auto is_clasp(KeyType key) -> bool;

    // Checks if this key is a clingo key.
    //
    // Returns the key if the upper bit is set and thows an exception for the
    // invalid key.
    [[nodiscard]] static auto is_clingo(KeyType key) -> std::optional<Key>;

    // Get the string representation of the configuration tree under the given key.
    void str_(Util::OutputBuffer &out, KeyType key, size_t first_indent, size_t indent) const;

    Clasp::Cli::ClaspCliConfig *config_;
    std::string mutable buf_;
    std::vector<Node> nodes_;
};

} // namespace CppClingo::Control
