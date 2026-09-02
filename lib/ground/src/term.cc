#include <clingo/ground/term.hh>

#include <clingo/util/optional.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>
#include <ranges>
#include <typeindex>
#include <utility>

namespace CppClingo::Ground {

namespace {

auto eval_args(EvalContext const &ctx, UTermVec const &args, std::vector<Symbol> &res) -> bool {
    res.clear();
    for (auto const &arg : args) {
        if (auto sym = arg->eval(ctx); sym) {
            res.emplace_back(*sym);
        } else {
            return false;
        }
    }
    return true;
}

auto match_args(EvalContext const &ctx, UTermVec const &term_args, SymbolSpan sym_args) -> bool {
    if (term_args.size() != sym_args.size()) {
        return false;
    }
    auto it = sym_args.begin();
    for (auto const &arg : term_args) {
        if (!arg->match(ctx, *it++)) {
            return false;
        }
    }
    return true;
}

auto rename_args(UTermVec const &args, SymbolStore &store, RenameMode mode, size_t *vars) -> UTermVec {
    auto res = UTermVec{};
    res.reserve(args.size());
    for (auto const &arg : args) {
        if (auto rep = arg->rename(store, mode, nullptr, vars); rep != nullptr) {
            res.emplace_back(std::move(rep));
        }
    }
    res.shrink_to_fit();
    return res;
}

} // namespace

// TermProjection

auto TermProjection::do_score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const
    -> double {
    return 0;
}

auto TermProjection::do_match([[maybe_unused]] EvalContext const &ctx, [[maybe_unused]] Symbol sym) const -> bool {
    return true;
}

auto TermProjection::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    // Note: this is a sentinel symbol intended for text output
    return ctx.store().fun_ref(ctx.store().string_ref("*"), {}, false);
}

auto TermProjection::do_rename([[maybe_unused]] SymbolStore &store, RenameMode mode,
                               [[maybe_unused]] String const *name, size_t *vars) const -> UTerm {
    assert(name == nullptr);
    if (mode == RenameMode::drop_projection) {
        return nullptr;
    }
    if (mode == RenameMode::rename_projection && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermProjection>();
}

auto TermProjection::do_rename([[maybe_unused]] Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermProjection>();
}

void TermProjection::do_vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] bool provide) const {
}

void TermProjection::do_print(std::ostream &out) const {
    out << "*";
}

auto TermProjection::do_copy() const -> UTerm {
    return std::make_unique<TermProjection>();
}

auto TermProjection::do_hash() const -> size_t {
    return Util::value_hash_record<TermProjection>();
}

auto TermProjection::do_equal_to([[maybe_unused]] Term const &other) const -> bool {
    return dynamic_cast<TermProjection const *>(&other) != nullptr;
}

auto TermProjection::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermProjection const *>(&other); x != nullptr) {
        return 0 <=> 0;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermSymbol

auto TermSymbol::do_match([[maybe_unused]] EvalContext const &ctx, Symbol sym) const -> bool {
    return sym == *sym_;
}

auto TermSymbol::do_score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const
    -> double {
    return 0;
}

auto TermSymbol::do_eval([[maybe_unused]] EvalContext const &ctx) const -> std::optional<Symbol> {
    return *sym_;
}

auto TermSymbol::do_rename([[maybe_unused]] SymbolStore &store, [[maybe_unused]] RenameMode mode, String const *name,
                           [[maybe_unused]] size_t *vars) const -> UTerm {
    if (name != nullptr && sym_->type() == SymbolType::function) {
        return std::make_unique<TermSymbol>(store.fun_ref(*name, sym_->args(), sym_->has_classical_sign()));
    }
    if (mode == RenameMode::rename_vars && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermSymbol>(*sym_);
}

auto TermSymbol::do_rename([[maybe_unused]] Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermSymbol>(*sym_);
}

void TermSymbol::do_vars([[maybe_unused]] VariableSet &vars, [[maybe_unused]] bool provide) const {
}

void TermSymbol::do_print(std::ostream &out) const {
    out << *sym_;
}

auto TermSymbol::do_copy() const -> UTerm {
    return std::make_unique<TermSymbol>(*sym_);
}

auto TermSymbol::do_hash() const -> size_t {
    return Util::value_hash_record<TermSymbol>(*sym_);
}

auto TermSymbol::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermSymbol const *>(&other);
    return x != nullptr && sym_ == x->sym_;
}

auto TermSymbol::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermSymbol const *>(&other); x != nullptr) {
        return sym_ <=> x->sym_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermFormatString

auto TermFormatString::do_score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const
    -> double {
    // All variables in a format string must be bound.
    return 0.0;
}

auto TermFormatString::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    return eval(ctx) == sym;
}

namespace {

auto align(Util::OutputBuffer &out, std::string_view str, FormatSpec const &spec,
           FormatSpec::Align def = FormatSpec::Align::left) {
    auto fill = [&](size_t padding) {
        std::ranges::fill_n(std::back_inserter(out), static_cast<ptrdiff_t>(padding), spec.fill.value_or(' '));
    };
    if (str.size() < spec.width) {
        auto padding = spec.width - str.size();
        switch (spec.align == FormatSpec::Align::none ? def : spec.align) {
            case FormatSpec::Align::center: {
                if (str.size() < spec.width) {
                    auto left = padding / 2;
                    auto right = padding - left;
                    fill(left);
                    out << str;
                    fill(right);
                }
                break;
            }
            case FormatSpec::Align::right: {
                fill(padding);
                out << str;
                break;
            }
            default: {
                out << str;
                fill(padding);
                break;
            }
        }
    } else {
        out << str;
    }
}

auto insert_sep(Util::OutputBuffer &tmp, size_t start, char sep, FormatSpec::Type type) {
    auto digits = tmp.size() - start;
    int width =
        type == FormatSpec::Type::binary || type == FormatSpec::Type::hex_lower || type == FormatSpec::Type::hex_upper
            ? 4
            : 3;
    if (std::cmp_less_equal(digits, width)) {
        return;
    }
    auto seps = (digits - 1) / static_cast<size_t>(width);
    tmp.reserve(static_cast<ptrdiff_t>(seps));

    auto span = tmp.span();
    auto jt = span.rbegin();
    int group = -1;
    for (char c : std::views::reverse(span.subspan(start, digits))) {
        if (++group == width) {
            *jt++ = sep;
            group = 0;
        }
        *jt++ = c;
    }
}

auto insert_prefix(Util::OutputBuffer &buf, size_t &start, std::string_view prefix) {
    size_t digits = buf.size() - start;
    buf.reserve(std::ssize(prefix));
    auto span = buf.span();
    auto num = span.subspan(start, digits);
    std::ranges::copy_backward(num, buf.span().end());
    std::ranges::copy(prefix, num.begin());
    start += prefix.size();
}

auto append_chr(Util::OutputBuffer &buf, Number const &num) -> bool {
    // NOLINTBEGIN(readability-magic-numbers)
    auto cp = static_cast<uint32_t>(num.as_int().value_or(-1));
    if (num > 0x10FFFF || (num >= 0xD800 && num <= 0xDFFF)) {
        buf.append('?');
        return false;
    }
    if (cp < 0x80) {
        buf.append(static_cast<char>(cp));
    } else if (cp < 0x800) {
        buf.append(static_cast<char>(0xC0 | (cp >> 6)));
        buf.append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        buf.append(static_cast<char>(0xE0 | (cp >> 12)));
        buf.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        buf.append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        buf.append(static_cast<char>(0xF0 | (cp >> 18)));
        buf.append(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        buf.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        buf.append(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return true;
    // NOLINTEND(readability-magic-numbers)
}

auto append_num(Util::OutputBuffer &buf, Number const &num, FormatSpec const &spec) -> void {
    if (spec.sign == FormatSpec::Sign::space && num >= 0) {
        buf.append(' ');
    }
    if (spec.sign == FormatSpec::Sign::plus && num >= 0) {
        buf.append('+');
    }
    switch (spec.type) {
        case FormatSpec::Type::binary: {
            append(buf, num, 2);
            break;
        }
        case FormatSpec::Type::octal: {
            append(buf, num, 8); // NOLINT
            break;
        }
        case FormatSpec::Type::hex_lower: {
            append(buf, num, 16); // NOLINT
            auto span = buf.span();
            std::ranges::transform(span, span.begin(), [](char c) {
                return static_cast<char>(static_cast<unsigned char>(std::tolower(c)));
            });
            break;
        }
        case FormatSpec::Type::hex_upper: {
            append(buf, num, 16); // NOLINT
            auto span = buf.span();
            std::ranges::transform(span, span.begin(), [](char c) {
                return static_cast<char>(static_cast<unsigned char>(std::toupper(c)));
            });
            break;
        }
        default: {
            append(buf, num, 10); // NOLINT
            break;
        }
    }
}

auto access(std::optional<Symbol> sym, std::vector<std::variant<SharedString, size_t>> const &accessors)
    -> std::optional<Symbol> {
    if (!sym) {
        return std::nullopt;
    }
    for (auto const &acc : accessors) {
        sym = std::visit(
            [&sym]<class T>(T const &x) -> std::optional<Symbol> {
                if constexpr (Util::matches<T, size_t>) {
                    if (sym->type() == SymbolType::function || sym->type() == SymbolType::tuple) {
                        if (auto args = sym->args(); x < args.size()) {
                            return args[x];
                        }
                    }
                    return std::nullopt;
                } else {
                    static_assert(Util::matches<T, SharedString>);
                    if (x == "name") {
                        if (sym->type() == SymbolType::function) {
                            return SymbolStore::str_ref(sym->name());
                        }
                    }
                    return std::nullopt;
                }
            },
            acc);
        if (!sym) {
            return std::nullopt;
        }
    }
    return sym;
}

} // namespace

auto TermFormatString::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    buf_.reset();
    for (auto const &elem : elems_) {
        auto res = std::visit(
            [this, &ctx]<class T>(T const &x) {
                if constexpr (Util::matches<T, SharedString>) {
                    buf_.append(x->view());
                    return true;
                } else {
                    static_assert(Util::matches<T, std::pair<UTerm, FormatSpec>>);
                    auto const &[term, spec] = x;
                    auto const val = access(term->eval(ctx), spec.accessors);
                    if (!val) {
                        return false;
                    }
                    if (val->type() == SymbolType::number) {
                        tmp_.reset();
                        if (spec.type == FormatSpec::Type::character) {
                            if (!append_chr(tmp_, val->num())) {
                                return false;
                            }
                            align(buf_, tmp_.view(), spec);
                            return true;
                        }

                        append_num(tmp_, val->num(), spec);
                        size_t start = val->num() < 0 || spec.sign != FormatSpec::Sign::minus ? 1 : 0;

                        if (spec.grouping == FormatSpec::Grouping::comma) {
                            insert_sep(tmp_, start, ',', spec.type);
                        } else if (spec.grouping == FormatSpec::Grouping::underscore) {
                            insert_sep(tmp_, start, '_', spec.type);
                        } else if (spec.type == FormatSpec::Type::locale) {
                            thread_local auto loc = std::locale{""};
                            auto sep = std::use_facet<std::numpunct<char>>(loc).thousands_sep();
                            insert_sep(tmp_, start, sep, spec.type);
                        }

                        if (spec.alternate_form) {
                            switch (spec.type) {
                                case FormatSpec::Type::binary: {
                                    insert_prefix(tmp_, start, "0b");
                                    break;
                                }
                                case FormatSpec::Type::octal: {
                                    insert_prefix(tmp_, start, "0o");
                                    break;
                                }
                                case FormatSpec::Type::hex_lower: {
                                    insert_prefix(tmp_, start, "0x");
                                    break;
                                }
                                case FormatSpec::Type::hex_upper: {
                                    insert_prefix(tmp_, start, "0X");
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }

                        if (spec.align == FormatSpec::Align::number) {
                            auto padding =
                                static_cast<ptrdiff_t>(tmp_.size() < spec.width ? spec.width - tmp_.size() : 0);
                            auto digits = tmp_.size() - start;
                            tmp_.reserve(padding);
                            auto span = tmp_.span();
                            std::ranges::copy_backward(span.subspan(start, digits), span.end());
                            std::ranges::fill(span.subspan(start, static_cast<size_t>(padding)),
                                              spec.fill.value_or(' '));
                        }

                        align(buf_, tmp_.view(), spec, FormatSpec::Align::right);
                        return true;
                    }
                    if (val->type() == SymbolType::string && spec.conversion == FormatSpec::Conversion::str) {
                        align(buf_, val->str().view(), spec);
                        return true;
                    }
                    tmp_.reset();
                    tmp_ << *val;
                    align(buf_, tmp_.view(), spec);
                    return true;
                }
            },
            elem);
        if (!res) {
            return std::nullopt;
        }
    }
    return SymbolStore::str_ref(ctx.store().string_ref(buf_.str()));
}

auto TermFormatString::do_rename([[maybe_unused]] SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm {
    assert(name == nullptr);
    auto renamed = FormatFieldVec{};
    renamed.reserve(elems_.size());
    for (auto const &elem : elems_) {
        std::visit(
            [&]<class T>(T const &x) {
                if constexpr (Util::matches<T, SharedString>) {
                    renamed.emplace_back(x);
                } else {
                    renamed.emplace_back(std::in_place_type<std::pair<UTerm, FormatSpec>>,
                                         x.first->rename(store, mode, name, vars), x.second);
                }
            },
            elem);
    }
    return std::make_unique<TermFormatString>(std::move(renamed));
}

auto TermFormatString::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto renamed = FormatFieldVec{};
    renamed.reserve(elems_.size());
    for (auto const &elem : elems_) {
        std::visit(
            [&]<class T>(T const &x) {
                if constexpr (Util::matches<T, SharedString>) {
                    renamed.emplace_back(x);
                } else {
                    renamed.emplace_back(std::in_place_type<std::pair<UTerm, FormatSpec>>, x.first->rename(vars),
                                         x.second);
                }
            },
            elem);
    }
    return std::make_unique<TermFormatString>(std::move(renamed));
}

void TermFormatString::do_vars(VariableSet &vars, bool provide) const {
    for (auto const &elem : elems_) {
        std::visit(
            [&]<class T>(T const &x) {
                if constexpr (Util::matches<T, std::pair<UTerm, FormatSpec>>) {
                    if (!provide) {
                        x.first->vars(vars, provide);
                    }
                } else {
                    static_assert(Util::matches<T, SharedString>);
                }
            },
            elem);
    }
}

void TermFormatString::do_print(std::ostream &out) const {
    out << "f\"";
    for (auto const &elem : elems_) {
        std::visit(
            [&out]<class T>(T const &x) {
                if constexpr (Util::matches<T, SharedString>) {
                    out << Util::p_quoted(x->view());
                } else {
                    static_assert(Util::matches<T, std::pair<UTerm, FormatSpec>>);
                    out << "{X_" << *x.first << x.second << "}";
                }
            },
            elem);
    }
    out << "\"";
}

auto TermFormatString::do_copy() const -> UTerm {
    auto elems = FormatFieldVec{};
    elems.reserve(elems_.size());
    for (auto const &elem : elems_) {
        std::visit(
            [&]<class T>(T const &x) {
                if constexpr (Util::matches<T, SharedString>) {
                    elems.emplace_back(x);
                } else {
                    static_assert(Util::matches<T, std::pair<UTerm, FormatSpec>>);
                    elems.emplace_back(std::in_place_type<std::pair<UTerm, FormatSpec>>, x.first->copy(), x.second);
                }
            },
            elem);
    }
    return std::make_unique<TermFormatString>(std::move(elems));
}

auto TermFormatString::do_hash() const -> size_t {
    return Util::value_hash_record<TermFormatString>(elems_);
}

auto TermFormatString::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermFormatString const *>(&other);
    return x != nullptr && elems_ == x->elems_;
}

auto TermFormatString::do_compare_to(Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermFormatString const *>(&other); x != nullptr) {
        return std::lexicographical_compare_three_way(elems_.begin(), elems_.end(), x->elems_.begin(), x->elems_.end());
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermVariable

auto TermVariable::do_score(double size, std::vector<bool> const &bound) const -> double {
    return bound[var_] ? 0.0 : size;
}

auto TermVariable::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    if (ctx.ass()[var_]) {
        return ctx.ass()[var_] == sym;
    }
    ctx.ass()[var_] = sym;
    return true;
}

auto TermVariable::do_eval([[maybe_unused]] EvalContext const &ctx) const -> std::optional<Symbol> {
    return ctx.ass()[var_];
}

auto TermVariable::do_rename([[maybe_unused]] SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                             size_t *vars) const -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermVariable>(mode == RenameMode::rename_vars && vars != nullptr ? (*vars)++ : var_);
}

auto TermVariable::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermVariable>(vars.try_emplace(var_, vars.size()).first.value());
}

void TermVariable::do_vars(VariableSet &vars, [[maybe_unused]] bool provide) const {
    vars.emplace(var_);
}

void TermVariable::do_print(std::ostream &out) const {
    out << "X_" << var_;
}

auto TermVariable::do_copy() const -> UTerm {
    return std::make_unique<TermVariable>(var_);
}

auto TermVariable::do_hash() const -> size_t {
    return Util::value_hash_record<TermSymbol>(var_);
}

auto TermVariable::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermVariable const *>(&other);
    return x != nullptr && var_ == x->var_;
}

auto TermVariable::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermVariable const *>(&other); x != nullptr) {
        return var_ <=> x->var_;
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermLinear

auto TermLinear::do_score(double size, std::vector<bool> const &bound) const -> double {
    return bound[var_] ? 0.0 : size;
}

auto TermLinear::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    if (sym.type() != SymbolType::number) {
        return false;
    }
    if (auto var = ctx.ass()[var_]; var) {
        // m * x + n == s
        if (var->type() != SymbolType::number) {
            return expect(ctx, loc_, logged_, "number expected (got ", *var, ")");
        }
        return m_ * var->num() + n_ == sym.num();
    }
    // x == (s - n) / m
    auto sn = sym.num() - n_;
    if (sn % m_ == 0) {
        ctx.ass()[var_] = ctx.store().num_ref(std::move(sn) / m_);
        return true;
    }
    return false;
}

auto TermLinear::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    if (auto var = ctx.ass()[var_];
        var && (var->type() == SymbolType::number || expect(ctx, loc_, logged_, "number expected (got ", *var, ")"))) {
        return ctx.store().num_ref(m_ * var->num() + n_);
    }
    return std::nullopt;
}

auto TermLinear::do_rename([[maybe_unused]] SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name,
                           size_t *vars) const -> UTerm {
    assert(name == nullptr);
    if (mode == RenameMode::rename_vars && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermLinear>(loc_, m_, var_, n_);
}

auto TermLinear::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermLinear>(loc_, m_, vars.try_emplace(var_, vars.size()).first.value(), n_);
}

void TermLinear::do_vars(VariableSet &vars, [[maybe_unused]] bool provide) const {
    vars.emplace(var_);
}

void TermLinear::do_print(std::ostream &out) const {
    out << "(" << m_ << "*X_" << var_ << "+" << n_ << ")";
}

auto TermLinear::do_copy() const -> UTerm {
    return std::make_unique<TermLinear>(loc_, m_, var_, n_);
}

auto TermLinear::do_hash() const -> size_t {
    return Util::value_hash_record<TermSymbol>(var_, m_, n_);
}

auto TermLinear::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermLinear const *>(&other);
    return x != nullptr && std::tie(var_, m_, n_) == std::tie(x->var_, x->m_, x->n_);
}

auto TermLinear::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermLinear const *>(&other); x != nullptr) {
        return std::tie(var_, m_, n_) <=> std::tie(x->var_, x->m_, x->n_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermUnary

auto TermUnary::do_score(double size, std::vector<bool> const &bound) const -> double {
    return op_ == UnaryOperator::minus ? rhs_->score(size, bound) : 0.0;
}

auto TermUnary::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    if (op_ == UnaryOperator::minus) {
        if (sym.type() == SymbolType::function) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return rhs_->match(ctx, *sym.flip_classical_sign());
        }
        if (sym.type() == SymbolType::number) {
            return rhs_->match(ctx, ctx.store().num_ref(-sym.num()));
        }
        return expect(ctx, loc_rhs_, logged_, "number or function expected (got ", sym, ")");
    }
    return (sym.type() == SymbolType::number || expect(ctx, loc_rhs_, logged_, "number expected (got ", sym, ")")) &&
           eval(ctx) == sym;
}

auto TermUnary::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    if (auto rhs = rhs_->eval(ctx); rhs) {
        switch (op_) {
            case UnaryOperator::minus: {
                if (rhs->type() == SymbolType::number) {
                    return ctx.store().num_ref(-rhs->num());
                }
                if (rhs->type() == SymbolType::function) {
                    return rhs->flip_classical_sign();
                }
                expect(ctx, loc_rhs_, logged_, "number or function expected (got ", *rhs, ")");
                break;
            }
            case UnaryOperator::negate: {
                if (rhs->type() == SymbolType::number ||
                    expect(ctx, loc_rhs_, logged_, "number expected (got ", *rhs, ")")) {
                    return ctx.store().num_ref(~rhs->num());
                }
                break;
            }
            case UnaryOperator::abs: {
                if (rhs->type() == SymbolType::number ||
                    expect(ctx, loc_rhs_, logged_, "number expected (got ", *rhs, ")")) {
                    return ctx.store().num_ref(abs(rhs->num()));
                }
                break;
            }
        }
    }
    return std::nullopt;
}

auto TermUnary::do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    assert(name == nullptr);
    if (op_ == UnaryOperator::negate && mode == RenameMode::rename_vars && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    return std::make_unique<TermUnary>(op_, loc_rhs_, rhs_->rename(store, mode, name, vars));
}

auto TermUnary::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    return std::make_unique<TermUnary>(op_, loc_rhs_, rhs_->rename(vars));
}

void TermUnary::do_vars(VariableSet &vars, bool provide) const {
    if (op_ == UnaryOperator::minus || !provide) {
        rhs_->vars(vars, provide);
    }
}

void TermUnary::do_print(std::ostream &out) const {
    out << "(";
    switch (op_) {
        case UnaryOperator::abs: {
            out << "|";
            break;
        }
        case UnaryOperator::negate: {
            out << "~";
            break;
        }
        case UnaryOperator::minus: {
            out << "-";
            break;
        }
    }
    out << *rhs_;
    if (op_ == UnaryOperator::abs) {
        out << "|";
    }
    out << ")";
}

auto TermUnary::do_copy() const -> UTerm {
    return std::make_unique<TermUnary>(op_, loc_rhs_, rhs_->copy());
}

auto TermUnary::do_hash() const -> size_t {
    return Util::value_hash_record<TermUnary>(op_, *rhs_);
}

auto TermUnary::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermUnary const *>(&other);
    return x != nullptr && std::tie(op_, *rhs_) == std::tie(x->op_, *x->rhs_);
}

auto TermUnary::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermUnary const *>(&other); x != nullptr) {
        return std::tie(op_, *rhs_) <=> std::tie(x->op_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermBinary

auto TermBinary::do_score([[maybe_unused]] double size, [[maybe_unused]] std::vector<bool> const &bound) const
    -> double {
    return 0;
}

auto TermBinary::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    return (sym.type() == CppClingo::SymbolType::number ||
            expect(ctx, loc_lhs_ + loc_rhs_, logged_, "number expected (got ", sym, ")")) &&
           eval(ctx) == sym;
}

auto TermBinary::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    if (auto lhs = lhs_->eval(ctx); lhs && (lhs->type() == CppClingo::SymbolType::number ||
                                            expect(ctx, loc_lhs_, logged_, "number expected (got ", *lhs, ")"))) {
        if (auto rhs = rhs_->eval(ctx); rhs && (rhs->type() == CppClingo::SymbolType::number ||
                                                expect(ctx, loc_rhs_, logged_, "number expected (got ", *rhs, ")"))) {
            auto &store = ctx.store();
            switch (op_) {
                case BinaryOperator::and_: {
                    return store.num_ref(lhs->num() & rhs->num());
                }
                case BinaryOperator::div: {
                    if (rhs->num() != 0 ||
                        expect(ctx, loc_rhs_, logged_, "non-zero number expected (got ", *rhs, ")")) {
                        return store.num_ref(lhs->num() / rhs->num());
                    }
                    break;
                }
                case BinaryOperator::minus: {
                    return store.num_ref(lhs->num() - rhs->num());
                }
                case BinaryOperator::mod: {
                    if (rhs->num() != 0 ||
                        expect(ctx, loc_rhs_, logged_, "non-zero number expected (got ", *rhs, ")")) {
                        return store.num_ref(lhs->num() % rhs->num());
                    }
                    break;
                }
                case BinaryOperator::or_: {
                    return store.num_ref(lhs->num() | rhs->num());
                }
                case BinaryOperator::plus: {
                    return store.num_ref(lhs->num() + rhs->num());
                }
                case BinaryOperator::pow: {
                    if (rhs->num() >= 0 ||
                        expect(ctx, loc_rhs_, logged_, "non-negative number expected (got ", *rhs, ")")) {
                        return store.num_ref(pow(lhs->num(), rhs->num()));
                    }
                    break;
                }
                case BinaryOperator::times: {
                    return store.num_ref(lhs->num() * rhs->num());
                }
                case BinaryOperator::xor_: {
                    return store.num_ref(lhs->num() ^ rhs->num());
                }
            }
        }
    }
    return std::nullopt;
}

auto TermBinary::do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    assert(name == nullptr);
    if (mode == RenameMode::rename_vars && vars != nullptr) {
        return std::make_unique<TermVariable>((*vars)++);
    }
    auto lhs = lhs_->rename(store, mode, name, vars);
    return std::make_unique<TermBinary>(loc_lhs_, std::move(lhs), op_, loc_rhs_, rhs_->rename(store, mode, name, vars));
}

auto TermBinary::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto lhs = lhs_->rename(vars);
    auto rhs = rhs_->rename(vars);
    return std::make_unique<TermBinary>(loc_lhs_, std::move(lhs), op_, loc_rhs_, std::move(rhs));
}

void TermBinary::do_vars(VariableSet &vars, bool provide) const {
    if (!provide) {
        lhs_->vars(vars, provide);
        rhs_->vars(vars, provide);
    }
}

void TermBinary::do_print(std::ostream &out) const {
    out << "(";
    out << *lhs_;
    switch (op_) {
        case BinaryOperator::and_: {
            out << "&";
            break;
        }
        case BinaryOperator::div: {
            out << "/";
            break;
        }
        case BinaryOperator::minus: {
            out << "-";
            break;
        }
        case BinaryOperator::mod: {
            out << "%";
            break;
        }
        case BinaryOperator::or_: {
            out << "|";
            break;
        }
        case BinaryOperator::plus: {
            out << "+";
            break;
        }
        case BinaryOperator::pow: {
            out << "^";
            break;
        }
        case BinaryOperator::times: {
            out << "*";
            break;
        }
        case BinaryOperator::xor_: {
            out << "^";
            break;
        }
    }
    out << *rhs_;
    out << ")";
}

auto TermBinary::do_copy() const -> UTerm {
    return std::make_unique<TermBinary>(loc_lhs_, lhs_->copy(), op_, loc_rhs_, rhs_->copy());
}

auto TermBinary::do_hash() const -> size_t {
    return Util::value_hash_record<TermBinary>(*lhs_, op_, *rhs_);
}

auto TermBinary::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermBinary const *>(&other);
    return x != nullptr && std::tie(*lhs_, op_, *rhs_) == std::tie(*x->lhs_, x->op_, *x->rhs_);
}

auto TermBinary::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermBinary const *>(&other); x != nullptr) {
        return std::tie(*lhs_, op_, *rhs_) <=> std::tie(*x->lhs_, x->op_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermTuple

auto TermTuple::do_score(double size, std::vector<bool> const &bound) const -> double {
    double ret = 0.0;
    if (!args_.empty()) {
        auto len = static_cast<double>(args_.size());
        // NOLINTNEXTLINE(readability-magic-numbers)
        double root = std::max(1.0, std::pow(size, 1.0 / len));
        for (const auto &x : args_) {
            ret += x->score(root, bound);
        }
        ret /= len;
    }
    return ret;
}

auto TermTuple::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    return sym.type() == SymbolType::tuple && match_args(ctx, args_, sym.args());
}

auto TermTuple::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    if (eval_args(ctx, args_, eval_)) {
        return ctx.store().tup_ref(eval_);
    }
    return std::nullopt;
}

auto TermTuple::do_rename(SymbolStore &store, RenameMode mode, [[maybe_unused]] String const *name, size_t *vars) const
    -> UTerm {
    assert(name == nullptr);
    return std::make_unique<TermTuple>(rename_args(args_, store, mode, vars));
}

auto TermTuple::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->rename(vars));
    }
    return std::make_unique<TermTuple>(std::move(args));
}

void TermTuple::do_vars(VariableSet &vars, bool provide) const {
    for (auto const &arg : args_) {
        arg->vars(vars, provide);
    }
}

void TermTuple::do_print(std::ostream &out) const {
    out << "(";
    auto n = args_.size();
    if (args_.size() == 1) {
        ++n;
    }
    for (auto const &arg : args_) {
        out << *arg;
        if (--n; n > 0) {
            out << ",";
        }
    }
    out << ")";
}

auto TermTuple::do_copy() const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->copy());
    }
    return std::make_unique<TermTuple>(std::move(args));
}

auto TermTuple::do_hash() const -> size_t {
    return Util::value_hash_record<TermTuple>(args_);
}

auto TermTuple::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermTuple const *>(&other);
    return x != nullptr && std::ranges::equal(args_, x->args_, [](auto const &a, auto const &b) { return *a == *b; });
}

auto TermTuple::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermTuple const *>(&other); x != nullptr) {
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// TermFunction

auto TermFunction::do_score(double size, std::vector<bool> const &bound) const -> double {
    double ret = 0.0;
    if (!args_.empty()) {
        auto len = static_cast<double>(args_.size());
        // NOLINTNEXTLINE(readability-magic-numbers)
        double root = std::max(1.0, std::pow(size / 2.0, 1.0 / len));
        for (const auto &x : args_) {
            ret += x->score(root, bound);
        }
        ret /= len;
    }
    return ret;
}

auto TermFunction::do_match(EvalContext const &ctx, Symbol sym) const -> bool {
    return sym.type() == SymbolType::function && !sym.has_classical_sign() && sym.name() == *name_ &&
           match_args(ctx, args_, sym.args());
}

auto TermFunction::do_eval(EvalContext const &ctx) const -> std::optional<Symbol> {
    if (eval_args(ctx, args_, eval_)) {
        return ctx.store().fun_ref(*name_, eval_, false);
    }
    return std::nullopt;
}

auto TermFunction::do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
    return std::make_unique<TermFunction>(name != nullptr ? *name : *name_, rename_args(args_, store, mode, vars));
}

auto TermFunction::do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->rename(vars));
    }
    return std::make_unique<TermFunction>(*name_, std::move(args));
}

void TermFunction::do_vars(VariableSet &vars, bool provide) const {
    for (auto const &arg : args_) {
        arg->vars(vars, provide);
    }
}

void TermFunction::do_print(std::ostream &out) const {
    out << *name_;
    if (auto n = args_.size(); n >= 1) {
        out << "(";
        for (auto const &arg : args_) {
            out << *arg;
            if (--n; n > 0) {
                out << ",";
            }
        }
        out << ")";
    }
}

auto TermFunction::do_copy() const -> UTerm {
    auto args = UTermVec{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->copy());
    }
    return std::make_unique<TermFunction>(*name_, std::move(args));
}

auto TermFunction::do_hash() const -> size_t {
    return Util::value_hash_record<TermFunction>(name_, args_);
}

auto TermFunction::do_equal_to(Term const &other) const -> bool {
    auto const *x = dynamic_cast<TermFunction const *>(&other);
    return x != nullptr && name_ == x->name_ &&
           std::ranges::equal(args_, x->args_, [](auto const &a, auto const &b) { return *a == *b; });
}

auto TermFunction::do_compare_to([[maybe_unused]] Term const &other) const -> std::strong_ordering {
    if (auto const *x = dynamic_cast<TermFunction const *>(&other); x != nullptr) {
        if (auto n = name_ <=> x->name_; std::is_neq(n)) {
            return n;
        }
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace CppClingo::Ground
