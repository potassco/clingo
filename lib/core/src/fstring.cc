#include <clingo/core/fstring.hh>
#include <clingo/util/type_traits.hh>

namespace CppClingo {

namespace {

auto is_alpha_or_underscore(char c) -> bool {
    return ('a' <= c && c <= 'z') || c == '_';
};

auto is_digit(char c) -> bool {
    return '0' <= c && c <= '9';
};

auto is_ascii(char c) -> bool {
    return c >= 0x20 && c <= 0x7E; // NOLINT
};

auto match_pred(std::string_view &sv, auto pred) {
    auto ib = sv.begin(); // NOLINT
    auto it = std::ranges::find_if_not(sv, pred);
    sv.remove_prefix(std::distance(ib, it));
    return std::string_view{ib, it};
};

auto match_char(std::string_view &sv, char c) -> bool {
    if (sv.starts_with(c)) {
        sv.remove_prefix(1);
        return true;
    }
    return false;
};

template <class T> auto match_num(std::string_view &sv) -> std::optional<T> {
    if (sv.empty()) {
        return std::nullopt;
    }
    if (is_digit(sv.front())) {
        auto ret = T{};
        auto const *ib = sv.data();
        auto const *ie = std::next(ib, std::ssize(sv));
        auto [it, ec] = std::from_chars(ib, ie, ret);
        if (ec != std::errc{}) {
            return std::nullopt;
        }
        sv.remove_prefix(std::distance(ib, it));
        return ret;
    }
    return std::nullopt;
};

auto is_align(char c) -> std::optional<FormatSpec::Align> {
    if (c == '<') {
        return FormatSpec::Align::left;
    }
    if (c == '>') {
        return FormatSpec::Align::right;
    }
    if (c == '=') {
        return FormatSpec::Align::number;
    }
    if (c == '^') {
        return FormatSpec::Align::center;
    }
    return std::nullopt;
};

auto to_string(FormatSpec::Align x) -> std::string_view {
    switch (x) {
        case FormatSpec::Align::left: {
            return "<";
        }
        case FormatSpec::Align::right: {
            return ">";
        }
        case FormatSpec::Align::center: {
            return "^";
        }
        case FormatSpec::Align::number: {
            return "=";
        }
        default: {
            return "";
        }
    }
}

auto to_string(FormatSpec::Type x) -> std::string_view {
    switch (x) {
        case FormatSpec::Type::character: {
            return "c";
        }
        case FormatSpec::Type::binary: {
            return "b";
        }
        case FormatSpec::Type::octal: {
            return "o";
        }
        case FormatSpec::Type::decimal: {
            return "d";
        }
        case FormatSpec::Type::hex_lower: {
            return "x";
        }
        case FormatSpec::Type::hex_upper: {
            return "X";
        }
        case FormatSpec::Type::locale: {
            return "n";
        }
        default: {
            return "";
        }
    }
}

auto to_string(FormatSpec::Sign x) -> std::string_view {
    switch (x) {
        case FormatSpec::Sign::plus: {
            return "+";
        }
        case FormatSpec::Sign::space: {
            return " ";
        }
        default: {
            return "";
        }
    }
}

auto to_string(FormatSpec::Grouping x) -> std::string_view {
    switch (x) {
        case FormatSpec::Grouping::comma: {
            return ",";
        }
        case FormatSpec::Grouping::underscore: {
            return "_";
        }
        default: {
            return "";
        }
    }
}

auto to_string(FormatSpec::Conversion x) -> std::string_view {
    switch (x) {
        case FormatSpec::Conversion::repr: {
            return "!r";
        }
        default: {
            return "";
        }
    }
}

template <class T> auto print_spec(T &out, FormatSpec const &spec) -> T & {
    for (auto const &x : spec.accessors) {
        std::visit(
            [&out]<class U>(U const &acc) {
                if constexpr (Util::is_among_v<U, size_t>) {
                    out << "[" << acc << "]";
                } else {
                    static_assert(Util::is_among_v<U, SharedString>);
                    out << "." << *acc;
                }
            },
            x);
    }
    out << to_string(spec.conversion);
    if (spec.fill || spec.align != FormatSpec::Align::none || spec.sign != FormatSpec::Sign::minus ||
        spec.alternate_form || spec.width > 0 || spec.grouping != FormatSpec::Grouping::none ||
        spec.type != FormatSpec::Type::string) {
        out << ":";
        if (spec.fill) {
            out << *spec.fill;
        }
        out << to_string(spec.align) << to_string(spec.sign);
        if (spec.alternate_form) {
            out << "#";
        }
        if (spec.width > 0) {
            out << std::to_string(spec.width);
        }
        out << to_string(spec.grouping) << to_string(spec.type);
    }
    return out;
}

} // namespace

auto FormatSpec::build(SymbolStore &store, std::string_view str) -> std::optional<FormatSpec> {
    auto ret = FormatSpec{};

    // parse accessors
    while (true) {
        if (match_char(str, '.')) {
            auto id = match_pred(str, is_alpha_or_underscore);
            if (id.empty()) {
                return std::nullopt;
            }
            ret.accessors.emplace_back(store.string(id));
        } else if (match_char(str, '[')) {
            if (auto n = match_num<size_t>(str)) {
                ret.accessors.emplace_back(*n);
            } else {
                return std::nullopt;
            }
            if (!match_char(str, ']')) {
                return std::nullopt;
            }
        } else {
            break;
        }
    }
    // parse conversion
    if (match_char(str, '!')) {
        if (match_char(str, 's')) {
            ret.conversion = Conversion::str;
        } else if (match_char(str, 'r')) {
            ret.conversion = Conversion::repr;
        } else {
            return std::nullopt;
        }
    }
    // spec
    if (match_char(str, ':')) {
        // align
        if (!str.empty()) {
            if (auto a = str.size() > 1 ? is_align(str[1]) : std::nullopt) {
                if (!is_ascii(str[0])) {
                    return std::nullopt;
                }
                ret.fill = str[0];
                ret.align = *a;
                str.remove_prefix(2);
            } else if (auto a = is_align(str.front())) {
                ret.align = *a;
                str.remove_prefix(1);
            }
        }
        // sign
        if (match_char(str, '+')) {
            ret.sign = Sign::plus;
        } else if (match_char(str, '-')) {
            ret.sign = Sign::minus;
        } else if (match_char(str, ' ')) {
            ret.sign = Sign::space;
        }
        // alternate form
        if (match_char(str, '#')) {
            ret.alternate_form = true;
        }
        // width
        if (!str.empty() && is_digit(str.front())) {
            if (auto num = match_num<uint32_t>(str)) {
                ret.width = *num;
            } else {
                return std::nullopt;
            }
        }
        // grouping
        if (match_char(str, ',')) {
            ret.grouping = Grouping::comma;
        } else if (match_char(str, '_')) {
            ret.grouping = Grouping::underscore;
        }
        // type
        if (match_char(str, 'b')) {
            ret.type = Type::binary;
        } else if (match_char(str, 'c')) {
            ret.type = Type::character;
        } else if (match_char(str, 'd')) {
            ret.type = Type::decimal;
        } else if (match_char(str, 'o')) {
            ret.type = Type::octal;
        } else if (match_char(str, 'x')) {
            ret.type = Type::hex_lower;
        } else if (match_char(str, 'X')) {
            ret.type = Type::hex_upper;
        } else if (match_char(str, 'n')) {
            ret.type = Type::locale;
        } else if (match_char(str, 's')) {
            ret.type = Type::string;
        }
    }
    if (!str.empty()) {
        return std::nullopt;
    }
    return ret;
}

auto operator<<(std::ostream &out, FormatSpec const &spec) -> std::ostream & {
    return print_spec(out, spec);
}

auto operator<<(Util::OutputBuffer &out, FormatSpec const &spec) -> Util::OutputBuffer & {
    return print_spec(out, spec);
}
} // namespace CppClingo
