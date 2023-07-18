#include <util/print.hh>

#include <symbol.hh>

namespace Gringo {

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

auto operator==(QuotedString const &a, QuotedString const &b) -> bool { return a.value == b.value; }

auto operator<<(std::ostream &out, QuotedString const &sym) -> std::ostream & {
    Util::print_quoted(out, sym.value);
    return out;
}

auto operator==(Function const &a, Function const &b) -> bool {
    return Util::value_equal(a.has_sign, b.has_sign, a.name, b.name, a.args, b.args);
}

auto operator<<(std::ostream &out, Function const &sym) -> std::ostream & {
    if (sym.has_sign) {
        out << "-";
    }
    out << sym.name;
    if (!sym.args.empty() || sym.name.empty()) {
        bool comma = sym.name.empty() && sym.args.size() == 1;
        out << "(" << Util::p_range(sym.args) << (comma ? "," : "") << ")";
    }
    return out;
}

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
    std::visit([&](auto &&value) { out << value; }, sym);
    return out;
}

namespace {

auto has_sign(Function const &fun) -> bool { return fun.has_sign; }

auto has_sign(int num) -> bool { return num < 0; }

auto has_sign(QuotedString str) -> bool {
    static_cast<void>(str);
    return false;
}

auto has_sign(Constant cst) -> bool {
    static_cast<void>(cst);
    return false;
}

} // namespace

auto has_sign(Symbol const &sym) -> bool {
    return std::visit([](auto &&symbol) { return has_sign(symbol); }, sym);
}

auto Util::value_hasher<QuotedString>::operator()(QuotedString const &x) const -> size_t {
    return Util::value_hash(x.value);
}

auto Util::value_hasher<Function>::operator()(Function const &x) const -> size_t {
    return Util::value_hash(x.has_sign, x.name, x.args);
}

} // namespace Gringo
