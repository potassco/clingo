#pragma once

#include <input/literal.hh>

#include <input/algo/check_type.hh>

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
    static constexpr auto rule = dsl::position(dsl::symbol<bool_symbols>(keyword_base));
    static constexpr auto value = Detail::with_state<Literal>([](auto &state, auto begin, bool value) {
        auto a = state.pos(begin);
        auto b = a;
        // NOLINTNEXTLINE(readability-magic-numbers)
        b.column += value ? 5 : 6;
        return LiteralBoolean{Location{std::move(a), std::move(b)}, Sign::none, value};
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
    static auto constexpr rule = dsl::if_(dsl::position(kw_not) >> dsl::if_(dsl::position(kw_not)));
    static auto constexpr value = Detail::with_state<std::pair<std::optional<Position>, Sign>>(
        [](auto &state) {
            static_cast<void>(state);
            return std::make_pair(std::nullopt, Sign::none);
        },
        [](auto &state, auto begin) { return std::make_pair(state.pos(begin), Sign::once); },
        [](auto &state, auto begin, auto sentinel) {
            static_cast<void>(sentinel);
            return std::make_pair(state.pos(begin), Sign::twice);
        });
};

struct literal {
    static constexpr char const *name = "literal";
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<atom>;
    static constexpr auto value = lexy::callback<Literal>([](auto sign, Literal lit) {
        add_sign(lit, sign.second, std::move(sign.first));
        return lit;
    });
};

} // namespace Gringo::Input::Grammar
