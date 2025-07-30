#pragma once

#include <clingo/control/config.hh>

#include <algorithm>

namespace CppClingo::Control {

void ClingoConfig::key_info(KeyType key, int *n_children, int *array_info, ValueFlags *value_info) const {
    init_(n_children, 0);
    init_(array_info, -1);
    init_(value_info, ValueFlags::none);
    if (auto clingo_key = is_clingo(key)) {
        nodes_.at(clingo_key->key_id()).info(*clingo_key, n_children, array_info, value_info);
    } else {
        int n_values = 0;
        if (auto ret = config_->getKeyInfo(key, n_children, array_info, nullptr, &n_values); ret == -1) {
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
        // NOTE: the root key can have clingo subkeys
        if (n_children != nullptr && key == key_root() && !nodes_.empty()) {
            *n_children += nodes_.front().entries();
        }
    }
}

auto ClingoConfig::description(KeyType key) const -> std::string_view {
    if (auto clingo_key = is_clingo(key)) {
        return nodes_.at(clingo_key->key_id()).description();
    }
    buf_.clear();
    if (config_->getKeyInfo(key, nullptr, nullptr, &buf_, nullptr) < 0) {
        error_("cannot get description");
    }
    return buf_;
}

[[nodiscard]] auto ClingoConfig::array_at(KeyType key, KeyType index) const -> KeyType {
    if (auto clingo_key = is_clingo(key)) {
        return nodes_.at(clingo_key->key_id()).array_at(*clingo_key, index).rep();
    }
    return check_(config_->getArrKey(key, index));
}

[[nodiscard]] auto ClingoConfig::map_at(KeyType key, std::string_view name) const -> KeyType {
    if (auto clingo_key = is_clingo(key)) {
        if (auto subkey = nodes_.at(clingo_key->key_id()).map_at(*clingo_key, name)) {
            return subkey->rep();
        }
        error_("subkey not found");
    }
    if (key == key_root() && !nodes_.empty()) {
        if (auto subkey = nodes_.front().map_at(Key{0}, name)) {
            return subkey->rep();
        }
    }
    return check_(config_->getKey(key, name));
}

[[nodiscard]] auto ClingoConfig::map_nth(KeyType key, uint32_t index) const -> std::string_view {
    if (auto clingo_key = is_clingo(key)) {
        if (auto name = nodes_.at(clingo_key->key_id()).map_nth(index)) {
            return *name;
        }
        error_("index out of bounds");
    }
    if (key == key_root() && !nodes_.empty()) {
        if (auto name = nodes_.front().map_nth(index)) {
            return *name;
        }
    }
    auto ret = config_->getSubkey(key, index);
    if (ret.empty()) {
        error_("index out of bounds");
    }
    return ret;
}

[[nodiscard]] auto ClingoConfig::get_value(KeyType key) const -> std::string_view {
    buf_.clear();
    if (auto clingo_key = is_clingo(key)) {
        nodes_.at(clingo_key->key_id()).get_value(clingo_key->index(), buf_);
    } else {
        if (config_->getValue(key, buf_) < 0) {
            error_("cannot get value");
        }
    }
    return buf_;
}

void ClingoConfig::set_value(KeyType key, std::string_view value) {
    if (auto clingo_key = is_clingo(key)) {
        nodes_.at(clingo_key->key_id()).set_value(clingo_key->index(), value);
    } else {
        if (config_->setValue(key, value) <= 0) {
            error_("cannot set value");
        }
    }
}

void ClingoConfig::add_entry(KeyType key, std::string_view name, std::string_view description,
                             std::unique_ptr<Entry> entry) {
    if (nodes_.empty()) {
        nodes_.emplace_back("the root key", nullptr);
    }
    auto clingo_key = Key{key == key_root() ? 0 : key};
    auto dot = name.rfind('.');
    if (dot == std::string_view::npos) {
        dot = name.size();
        clingo_key = parse_path_(clingo_key, name.substr(0, dot));
        name = name.substr(dot + 1);
    }

    auto index = parse_name_(name);
    auto &node = nodes_.at(clingo_key.key_id());
    if (index && !entry) {
        error_("array key without entry");
    }
    node.add_subkey(name, clingo_key, Key{static_cast<KeyType>(nodes_.size()), index});
    nodes_.emplace_back(description, std::move(entry));
}

auto ClingoConfig::str(KeyType key) const -> std::string_view {
    out_.reset();
    str_(key, 0, 0);
    return out_.view();
}

[[nodiscard]] auto ClingoConfig::Key::key_id() const -> KeyType {
    return (rep_ & MaskKeyId) >> BitsIndex;
}

[[nodiscard]] auto ClingoConfig::Key::index() const -> std::optional<KeyType> {
    auto res = rep_ & MaskIndex;
    if (res == 0) {
        return std::nullopt;
    }
    if (res == 1) {
        return index_invalid();
    }
    return res - 2;
}

[[nodiscard]] auto ClingoConfig::Key::subkey(Key child) const -> Key {
    if (index() && child.index()) {
        error_("multiple indices");
    }
    return index() ? Key{child.key_id(), index()} : child;
}

[[nodiscard]] auto ClingoConfig::Key::rep() const -> KeyType {
    return rep_;
}

auto ClingoConfig::Key::encode_array(std::optional<KeyType> index) -> KeyType {
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

auto ClingoConfig::Key::encode_key_id(KeyType key_id) -> KeyType {
    if (key_id >= (MaskKeyId >> BitsIndex)) {
        error_("key size exceeded");
    }
    return key_id << BitsIndex;
}

[[nodiscard]] auto ClingoConfig::Node::description() const -> std::string_view {
    return description_;
}

[[nodiscard]] auto ClingoConfig::Node::entries() const -> int {
    return static_cast<int>(subkeys_.size());
}

[[nodiscard]] auto ClingoConfig::Node::map_at(Key key, std::string_view name) const -> std::optional<Key> {
    auto it = subkeys_.find(name);
    if (it != subkeys_.end()) {
        return key.subkey(it->second);
    }
    return std::nullopt;
}

[[nodiscard]] auto ClingoConfig::Node::map_nth(KeyType index) const -> std::optional<std::string_view> {
    auto it = subkeys_.begin();
    for (; index > 0 && it != subkeys_.end(); --index, ++it) {
    }
    return it != subkeys_.end() ? std::make_optional<std::string_view>(it->first) : std::nullopt;
}

[[nodiscard]] auto ClingoConfig::Node::array_at(Key key, KeyType index) const -> Key {
    if (entry_ == nullptr || !entry_->size_array()) {
        error_("not an array");
    }
    if (index == index_invalid()) {
        error_("invalid index");
    }
    return Key{key.key_id(), index};
}

void ClingoConfig::Node::get_value(std::optional<KeyType> index, std::string &value) const {
    if (entry_ == nullptr) {
        error_("not a value");
    }
    if (index == index_invalid()) {
        index = std::nullopt;
    }
    auto flags = entry_->value_info(index);
    if (!intersects(flags, ValueFlags::value)) {
        error_("cannot get value");
    }
    entry_->get_value(index, value);
}

void ClingoConfig::Node::set_value(std::optional<KeyType> index, std::string_view value) {
    if (entry_ == nullptr) {
        error_("not a value");
    }
    if (index == index_invalid()) {
        index = std::nullopt;
    }
    auto flags = entry_->value_info(index);
    if (!intersects(flags, ValueFlags::set)) {
        error_("cannot set value");
    }
    entry_->set_value(index, value);
}

void ClingoConfig::Node::info(Key key, int *n_children, int *array_info, ValueFlags *value_info) const {
    if (n_children != nullptr) {
        *n_children = static_cast<int>(subkeys_.size());
    }
    if (array_info != nullptr && entry_ != nullptr) {
        *array_info = entry_->size_array().value_or(-1);
    }
    if (value_info != nullptr && entry_ != nullptr) {
        *value_info = entry_->value_info(key.index());
    }
}

void ClingoConfig::Node::add_subkey(std::string_view name, Key parent, Key child) {
    if (parent.index() && child.index()) {
        error_("multiple indices");
    }
    if (child.index()) {
        child = Key{child.key_id(), index_invalid()};
    }
    subkeys_.emplace(name, child);
}

auto ClingoConfig::parse_name_(std::string_view &name) -> std::optional<IndexType> {
    auto res = std::optional<IndexType>{};
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
    if (std::isalpha(static_cast<unsigned char>(name.front())) == 0 && name.front() != '_') {
        error_("invalid key");
    }
    if (!std::ranges::all_of(name,
                             [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; })) {
        error_("invalid key");
    }
    return res;
}

auto ClingoConfig::parse_path_(Key key, std::string_view path) -> Key {
    for (size_t start = 0; start < path.size();) {
        auto &cur = nodes_.at(key.key_id());

        size_t dot = path.find('.', start);
        auto last = dot == std::string_view::npos;
        if (last) {
            dot = path.size();
        }
        auto name = path.substr(start, dot - start);
        auto index = parse_name_(name);
        if (auto subkey = cur.map_at(key, name)) {
            key = *subkey;
        } else {
            error_("key not found");
        }
        if (index) {
            if (!key.index()) {
                error_("unexpected index");
            }
            key = Key{key.key_id(), index};
        }
        start = dot + 1;
    }
    return key;
}

[[nodiscard]] auto ClingoConfig::is_clasp(KeyType key) -> bool {
    return (key & MaskClingo) == 0;
}

[[nodiscard]] auto ClingoConfig::is_clingo(KeyType key) -> std::optional<Key> {
    return !is_clasp(key) ? std::make_optional<Key>(key) : std::nullopt;
}

void ClingoConfig::str_(KeyType key, size_t first_indent, size_t indent) const {
    int map_keys = 0;
    int arr_len = 0;
    auto val_info = ValueFlags::none;
    key_info(key, &map_keys, &arr_len, &val_info);
    auto fi = [&, first = true]() mutable { return Util::fill(std::exchange(first, false) ? first_indent : indent); };
    if (intersects(val_info, ValueFlags::get)) {
        if (intersects(val_info, ValueFlags::value)) {
            out_ << fi() << CppClingo::Util::p_quoted(get_value(key)) << "\n";
        } else {
            out_ << fi() << "null\n";
        }
    }
    if (map_keys > 0 && arr_len <= 0) {
        for (int i = 0; i < map_keys; ++i) {
            auto name = std::string_view{map_nth(key, i)};
            out_ << fi() << name << ":";
            auto sub_key = map_at(key, name);
            auto sub_info = ValueFlags::none;
            key_info(sub_key, nullptr, nullptr, &sub_info);
            if (intersects(sub_info, ValueFlags::get)) {
                out_ << " ";
                str_(sub_key, 0, indent + name.size() + 2);
            } else {
                out_ << "\n";
                str_(sub_key, indent + 2, indent + 2);
            }
        }
    }
    if (arr_len >= 0) {
        if (int e = arr_len; e > 0) {
            for (int i = 0, e = arr_len; i != e; ++i) {
                out_ << fi() << "- ";
                auto sub_key = array_at(key, i);
                str_(sub_key, 0, indent + 2);
            }

        } else {
            out_ << fi() << "[]\n";
        }
    }
}

} // namespace CppClingo::Control
