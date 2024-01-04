#pragma once

#include <gringo/input/literal.hh>

#include <gringo/input/algo/analyze.hh>

#include "../add_sign.hh"
#include "term.hh"

namespace Gringo::Input::Grammar {

struct relation {
    static constexpr char const *name = "relation";
    static constexpr auto symbols = lexy::symbol_table<Relation> //
                                        .map<LEXY_SYMBOL("<=")>(Relation::less_equal)
                                        .map<LEXY_SYMBOL("<")>(Relation::less)
                                        .map<LEXY_SYMBOL(">=")>(Relation::greater_equal)
                                        .map<LEXY_SYMBOL(">")>(Relation::greater)
                                        .map<LEXY_SYMBOL("!=")>(Relation::inequal)
                                        .map<LEXY_SYMBOL("==")>(Relation::equal)
                                        .map<LEXY_SYMBOL("=")>(Relation::equal);
    static constexpr auto rule = dsl::symbol<symbols>;
    static constexpr auto value = lexy::forward<Relation>;
};

struct right_guard {
    static constexpr char const *name = "guard";
    static constexpr auto rule = dsl::p<relation> >> dsl::p<term>;
    static constexpr auto value = lexy::construct<std::pair<Relation, Term>>;
};

struct right_guards {
    static constexpr char const *name = "guards";
    static constexpr auto rule = dsl::list(dsl::p<right_guard>);
    static constexpr auto value = lexy::as_list<GuardVec>;
};

struct atom_bool : lexy::token_production {
    static constexpr char const *name = "Boolean atom";
    static constexpr auto bool_symbols = lexy::symbol_table<bool> //
                                             .map<LEXY_SYMBOL("#true")>(true)
                                             .map<LEXY_SYMBOL("#false")>(false);
    static constexpr auto rule = Detail::location(dsl::symbol<bool_symbols>(keyword_base));
    static constexpr auto value = lexy::callback<Literal>([](Location loc, bool value) {
        return LiteralBoolean{std::move(loc), Sign::none, value};
    });
};

struct atom {
    static constexpr char const *name = "atom";
    using scan_result = lexy::scan_result<Term>;

    STRING_TAG(relation, "expected relation");

    static constexpr auto is_atom = dsl::context_flag<atom>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = []() {
        auto cont = dsl::p<right_guards> | is_atom.is_set() | dsl::error<expected_relation>;
        auto rel_or_sym_atom = is_atom.create() + dsl::scan + cont;
        return dsl::p<atom_bool> | dsl::else_ >> rel_or_sym_atom;
    }();
    static constexpr auto value = lexy::callback<Literal>(
        lexy::forward<Literal>,
        [](auto term) {
            auto loc = location(term);
            return LiteralSymbolic{std::move(loc), Sign::none, std::move(term)};
        },
        [](auto lhs, auto rhs) {
            auto loc = location(lhs) + location(rhs.back().second);
            return LiteralRelation{std::move(loc), Sign::none, std::move(lhs), std::move(rhs)};
        });
};

struct naf_sign {
    static constexpr char const *name = "default negation";
    static auto constexpr rule = dsl::if_(Detail::position(kw_not) >> dsl::opt(kw_not));
    static auto constexpr value = lexy::callback<std::pair<std::optional<Position>, Sign>>(
        []() { return std::make_pair(std::nullopt, Sign::none); },
        [](Position begin, lexy::nullopt) { return std::make_pair(std::move(begin), Sign::once); },
        [](Position begin) { return std::make_pair(std::move(begin), Sign::twice); });
};

struct literal {
    static constexpr char const *name = "literal";
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<atom>;
    static constexpr auto value = lexy::callback<Literal>([](auto sign, Literal lit) {
        auto res = add_sign(lit, sign.second, std::move(sign.first));
        return std::move(res).value_or(std::move(lit));
    });
};

} // namespace Gringo::Input::Grammar
