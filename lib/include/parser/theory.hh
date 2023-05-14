#pragma once

#include <theory.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>

namespace grammar {

static constexpr auto theory_op = []() {
    auto theory_op_base = dsl::identifier(LEXY_ASCII_ONE_OF("/<=>+\\-*/?&@|:;~^.!"));
    auto kw_semicolon = LEXY_KEYWORD(";", theory_op_base);
    auto kw_colon = LEXY_KEYWORD(":", theory_op_base);
    auto kw_dot = LEXY_KEYWORD(".", theory_op_base);
    return theory_op_base //
               .reserve(kw_semicolon)
               .reserve(kw_colon)
               .reserve(kw_dot) |
           kw_not;
}();
static constexpr auto theory_ops = dsl::list(theory_op);

struct theory_term {
    static constexpr auto rule = dsl::recurse<struct rec_theory_term>;
    static constexpr auto value = lexy::noop;
};

struct theory_term_root {
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::lit_c<','>)) |
                                 dsl::angle_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) |
                                 dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) |
                                 dsl::p<identifier> >>
                                     dsl::opt(dsl::parenthesized.opt_list(dsl::p<theory_term>,
                                                                          dsl::sep(dsl::lit_c<','>))) |
                                 dsl::p<constant_term> | dsl::p<number> | dsl::p<string> | dsl::p<variable_term> |
                                 dsl::p<anonymous_variable_term>;
    static constexpr auto value = lexy::noop;
};

struct rec_theory_term {
    static constexpr auto rule =
        dsl::opt(theory_ops) + dsl::p<theory_term_root> + dsl::while_(theory_ops >> dsl::p<theory_term_root>);
    static constexpr auto value = lexy::noop;
};

struct theory_atom {
    static constexpr auto rule = []() {
        auto theory_guard = theory_op >> dsl::p<theory_term>;
        auto theory_elem = dsl::p<condition> | dsl::else_ >> dsl::list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)) +
                                                                 dsl::p<opt_condition>;

        auto theory_name = dsl::p<identifier> + dsl::opt(dsl::p<term_pool>);
        return LEXY_LIT("&") >>
               theory_name + dsl::if_(dsl::curly_bracketed.opt_list(theory_elem, dsl::sep(dsl::lit_c<';'>)) >>
                                      dsl::if_(theory_guard));
    }();
    static constexpr auto value = lexy::noop >> lexy::callback<TheoryAtom>([](auto &&...) { return TheoryAtom{}; });
};

} // namespace grammar
