#pragma once

#include <theory.hh>

#include <parser/aggregate.hh>
#include <parser/literal.hh>

namespace grammar {

struct theory_op {
    static constexpr auto rule = []() {
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

    static constexpr auto value =
        lexy::callback<std::string>(lexy::as_string<std::string, encoding>, lexy::constant("not"));
};

struct theory_ops {
    static constexpr auto rule = dsl::list(dsl::p<theory_op>);
    static constexpr auto value = lexy::as_list<std::vector<std::string>>;
};

struct theory_term {
    static constexpr auto rule = dsl::recurse<struct theory_term_unparsed>;
    static constexpr auto value = lexy::forward<UTheoryTerm>;
};

struct theory_term_arguments {
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::vector<UTheoryTerm>>;
};

struct theory_term_tuple {
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::lit_c<','>));
    static constexpr auto
        value = lexy::as_list<UTheoryTermVec> >> lexy::callback<UTheoryTerm>([](UTheoryTermVec elems) {
                    return std::make_unique<TheoryTermTuple>(TheoryTermTupleType::Tuple, std::move(elems));
                });
};

struct theory_term_set {
    static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto
        value = lexy::as_list<UTheoryTermVec> >> lexy::callback<UTheoryTerm>([](UTheoryTermVec elems) {
                    return std::make_unique<TheoryTermTuple>(TheoryTermTupleType::Set, std::move(elems));
                });
};

struct theory_term_list {
    static constexpr auto rule = dsl::square_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto
        value = lexy::as_list<UTheoryTermVec> >> lexy::callback<UTheoryTerm>([](UTheoryTermVec elems) {
                    return std::make_unique<TheoryTermTuple>(TheoryTermTupleType::List, std::move(elems));
                });
};

struct theory_term_function {
    static constexpr auto rule = dsl::p<identifier> >> dsl::if_(dsl::p<theory_term_arguments>);
    static constexpr auto value = lexy::new_<TheoryTermFunction, UTheoryTerm>;
};

struct theory_term_root {
    static constexpr auto rule = dsl::p<theory_term_tuple> | dsl::p<theory_term_set> | dsl::p<theory_term_list> |
                                 dsl::p<theory_term_function> | dsl::p<constant_term> | dsl::p<number> |
                                 dsl::p<string_term> | dsl::p<variable_term> | dsl::p<anonymous_variable_term>;
    static constexpr auto value = lexy::callback<UTheoryTerm>(
        lexy::forward<UTheoryTerm>,
        // lexy::new_<TheoryTermInteger, UTheoryTerm>,
        [](int number) { return std::make_unique<TheoryTermInteger>(number); },
        // TODO: provide constant, string, and variable without arguments!
        [](auto &&...args) -> UTheoryTerm { throw std::logic_error("implement me!!!"); });
};

struct theory_term_unparsed_guards {
    static constexpr auto rule = dsl::list(dsl::p<theory_ops> >> dsl::p<theory_term_root>);
    static constexpr auto value =
        lexy::collect<TheoryTermUnparsed::GuardVec>(lexy::construct<TheoryTermUnparsed::Guard>);
};

struct theory_term_unparsed {
    static constexpr auto rule =
        dsl::if_(dsl::p<theory_ops>) + dsl::p<theory_term_root> + dsl::if_(dsl::p<theory_term_unparsed_guards>);
    static constexpr auto value = lexy::new_<TheoryTermUnparsed, UTheoryTerm>;
};

struct theory_atom {
    static constexpr auto rule = []() {
        auto theory_guard = dsl::p<theory_op> >> dsl::p<theory_term>;
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
