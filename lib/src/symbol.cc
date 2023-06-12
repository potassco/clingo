#include <util/print.hh>

#include <symbol.hh>

auto operator<<(std::ostream &out, Constant op) -> std::ostream & {
    switch (op) {
        case Constant::supremum: {
            out << "#sup";
            break;
        }
        case Constant::infimum: {
            out << "#inf";
            break;
        }
    }
    return out;
}

auto QuotedString::hash() const -> size_t { return std::hash<std::string>{}(value); }

auto operator==(QuotedString const &a, QuotedString const &b) -> bool { return a.value == b.value; }

auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream & {
    print_quoted(out, sym.value);
    return out;
}

auto Function::hash() const -> size_t {
    size_t hash = std::hash<std::string>{}(name);
    for (auto const &value : args) {
        hash = hash_combine(hash, std::hash<Symbol>{}(value));
    }
    return hash;
}

auto operator==(Function const &a, Function const &b) -> bool { return value_equal(a.name, b.name, a.args, b.args); }

auto operator<<(std::ostream &out, Function const &sym) -> std::ostream & {
    out << sym.name;
    if (!sym.args.empty() || sym.name.empty()) {
        bool comma = sym.name.empty() && sym.args.size() == 1;
        out << "(" << p_range(sym.args) << (comma ? "," : "") << ")";
    }
    return out;
}

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
    std::visit([&](auto &&value) { out << value; }, sym);
    return out;
}
