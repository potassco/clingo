#pragma once

#include <literal.hh>

#include <parser/base.hh>
#include <parser/term.hh>

namespace grammar {

struct relation {
    static constexpr auto symbols = lexy::symbol_table<Relation> //
                                        .map<LEXY_SYMBOL("<=")>(Relation::less_equal)
                                        .map<LEXY_SYMBOL("<")>(Relation::less)
                                        .map<LEXY_SYMBOL(">=")>(Relation::greater_equal)
                                        .map<LEXY_SYMBOL(">")>(Relation::greater)
                                        .map<LEXY_SYMBOL("!=")>(Relation::inequal)
                                        .map<LEXY_SYMBOL("=")>(Relation::equal);
    static constexpr auto rule = dsl::symbol<symbols>;
    static constexpr auto value = lexy::forward<Relation>;
};

struct right_guard {
    static constexpr auto rule = dsl::p<relation> >> dsl::p<term>;
    static constexpr auto value = lexy::construct<std::pair<Relation, UTerm>>;
};

struct right_guards {
    static constexpr auto rule = dsl::list(dsl::p<right_guard>);
    static constexpr auto value = lexy::as_list<GuardVec>;
};

struct atom_bool : lexy::token_production {
    static constexpr auto bool_symbols = lexy::symbol_table<bool> //
                                             .map<LEXY_SYMBOL("#true")>(true)
                                             .map<LEXY_SYMBOL("#false")>(false);
    static constexpr auto rule = dsl::symbol<bool_symbols>(keyword_base);
    static constexpr auto value = lexy::new_<LiteralBoolean, ULiteral>;
};

struct atom {
    using scan_result = lexy::scan_result<UTerm>;

    struct expected_relation {
        static constexpr auto name = "expected relation";
    };

    static constexpr auto is_atom = dsl::context_flag<atom>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<UTerm>(dsl::p<term>);
        if (res_term.has_value() && res_term.value()->is_atom()) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = []() {
        auto cont = dsl::p<right_guards> | is_atom.is_set() | dsl::error<expected_relation>;
        auto rel_or_sym_atom = is_atom.create() + dsl::scan + cont;
        return dsl::p<atom_bool> | dsl::else_ >> rel_or_sym_atom;
    }();
    static constexpr auto value = lexy::callback<ULiteral>(
        lexy::forward<ULiteral>, lexy::new_<LiteralSymbolic, ULiteral>, lexy::new_<LiteralRelation, ULiteral>);
};

struct naf_sign {
    static auto constexpr rule = dsl::opt(kw_not) + dsl::opt(kw_not);
    static auto constexpr value = lexy::callback<Sign>([](lexy::nullopt, lexy::nullopt) { return Sign::none; }, //
                                                       [](lexy::nullopt) { return Sign::once; },                //
                                                       []() { return Sign::twice; });
};

struct literal {
    static constexpr auto rule = dsl::p<naf_sign> + dsl::p<atom>;
    static constexpr auto value = lexy::callback<ULiteral>([](Sign sign, ULiteral lit) {
        lit->add_sign(sign);
        return std::move(lit);
    });
};

} // namespace grammar
