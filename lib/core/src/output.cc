#include <clingo/core/output.hh>

namespace Clingo {

auto Clingo::OutputTheory::sym(SymbolStore &store, Symbol sym) -> size_t {
    switch (sym.type()) {
        case SymbolType::inf: {
            return str(*store.string("#inf"));
        }
        case SymbolType::sup: {
            return str(*store.string("#sup"));
        }
        case SymbolType::string: {
            static thread_local auto buf = Util::OutputBuffer{};
            buf.reset();
            buf << Util::p_quoted(sym.str().view());
            return str(*store.string(buf.view()));
        }
        case SymbolType::function: {
            auto args = std::vector<size_t>{};
            args.reserve(sym.args().size());
            for (auto const &arg : sym.args()) {
                args.emplace_back(this->sym(store, arg));
            }
            auto ret = fun(sym.name(), args);
            if (sym.has_sign()) {
                ret = fun(*store.string("-"), {&ret, 1});
            }
            return ret;
        }
        case SymbolType::tuple: {
            auto args = std::vector<size_t>{};
            args.reserve(sym.args().size());
            for (auto const &arg : sym.args()) {
                args.emplace_back(this->sym(store, arg));
            }
            return tup(TheoryTermTupleType::tuple, args);
        }
        case SymbolType::number: {
            return num(sym.num());
        }
    }
    Util::unreachable();
}

} // namespace Clingo
