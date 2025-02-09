#include "config.hh"
#include "util.hh"

#include <iomanip>
#include <sstream>

namespace Clingo::Python {

auto Config::type_() -> clingo_config_type_bitset_t {
    clingo_config_type_bitset_t type = 0;
    handle_error(clingo_config_type(config_, key_, &type));
    return type;
}

auto Config::is_array() -> bool {
    return (type_() & clingo_config_type_array) != 0;
}

auto Config::is_map_() -> bool {
    return (type_() & clingo_config_type_map) != 0;
}

auto Config::is_value() -> bool {
    return (type_() & clingo_config_type_value) != 0;
}

auto Config::has_subkey_(char const *name) -> bool {
    if (is_map_()) {
        auto result = false;
        handle_error(clingo_config_map_has_subkey(config_, key_, name, &result));
        return result;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::has_value() -> bool {
    if (is_value()) {
        bool assigned = false;
        handle_error(clingo_config_value_is_assigned(config_, key_, &assigned));
        return assigned;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::array_at(size_t index) -> Config {
    if (index < array_len()) {
        clingo_id_t subkey = 0;
        handle_error(clingo_config_array_at(config_, key_, index, &subkey));
        return {config_, subkey};
    }
    throw py::index_error{"invalid index"};
}

auto Config::get(char const *name) -> Config {
    if (has_subkey_(name)) {
        clingo_id_t subkey = 0;
        handle_error(clingo_config_map_at(config_, key_, name, &subkey));
        return {config_, subkey};
    }
    throw py::attribute_error{"invalid attribute"};
}

void Config::set_value(pybind11::handle value) {
    if (is_value()) {
        auto val = value.cast<std::string>();
        handle_error(clingo_config_value_set(config_, key_, val.c_str()));
    }
    throw py::attribute_error{"invalid attribute"};
}

void Config::set(char const *name, pybind11::handle value) {
    get(name).set_value(value);
}

auto Config::array_len() -> size_t {
    if (is_array()) {
        size_t size = 0;
        handle_error(clingo_config_array_size(config_, key_, &size));
        return size;
    }
    throw py::attribute_error{"invalid attribute"};
}

auto Config::get_value() -> char const * {
    if (is_value()) {
        char const *value = nullptr;
        handle_error(clingo_config_value_get(config_, key_, &value));
        return value;
    }
    // NOTE: could be written much nicer using std::format
    auto buf = std::array<char, 3 + (2 * sizeof(void *))>{};
    snprintf(buf.data(), buf.size(), "%p", static_cast<void *>(config_));
    static thread_local auto rep = std::string{};
    rep = "<object Config at ";
    rep += buf.data();
    rep += " with key ";
    rep += std::to_string(key_);
    rep += ">";
    return rep.c_str();
}

auto Config::attrs() -> std::vector<char const *> {
    auto res = std::vector<char const *>{};
    if (is_map_()) {
        size_t size = 0;
        handle_error(clingo_config_map_size(config_, key_, &size));
        for (size_t i = 0; i < size; ++i) {
            char const *name = nullptr;
            handle_error(clingo_config_map_subkey_name(config_, key_, i, &name));
            res.emplace_back(name);
        }
    }
    return res;
}

auto Config::desc() -> char const * {
    char const *desc = nullptr;
    clingo_config_description(config_, key_, &desc);
    return desc;
}

namespace {

class fill {
  public:
    fill(size_t n, char c = ' ') : n_{n}, c_{c} {}
    friend auto operator<<(std::ostream &out, fill const &x) -> std::ostream & {
        std::fill_n(std::ostreambuf_iterator<char>(out), 2 * x.n_, x.c_);
        return out;
    }

  private:
    size_t n_;
    char c_;
};

} // namespace

void Config::str_(std::ostringstream &out, size_t first_indent, size_t indent) {
    auto fi = [&, first = true]() mutable { return fill(std::exchange(first, false) ? first_indent : indent); };
    if (is_value()) {
        if (has_value()) {
            out << fi() << std::quoted(str()) << "\n";
        } else {
            out << fi() << "null";
        }
    }
    if (is_map_()) {
        for (auto const *attr : attrs()) {
            out << fi() << attr << ":";
            auto cfg = get(attr);
            if (cfg.is_value()) {
                out << " ";
                cfg.str_(out, 0, indent + strlen(attr) + 2);
            } else {
                out << "\n";
                cfg.str_(out, indent + 2, indent + 2);
            }
        }
    }
    if (is_array()) {
        if (size_t e = array_len(); e > 0) {
            for (size_t i = 0, e = array_len(); i != e; ++i) {
                out << fi() << "- ";
                array_at(i).str_(out, 0, indent + 2);
            }

        } else {
            out << fi() << "[]\n";
        }
    }
}

auto Config::str() -> std::string {
    auto out = std::ostringstream{};
    str_(out, 0, 0);
    return std::move(out).str();
}

} // namespace Clingo::Python
