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

auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream & {
    print_quoted(out, sym.value);
    return out;
}

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
