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

namespace detail {

template <TheoryTermTupleType type> struct make_tuple {
    static constexpr auto
        value = lexy::as_list<UTheoryTermVec> >>
                lexy::callback<UTheoryTerm>(
                    [](lexy::nullopt) { return std::make_unique<TheoryTermTuple>(type, UTheoryTermVec{}); },
                    [](UTheoryTermVec elems) { return std::make_unique<TheoryTermTuple>(type, std::move(elems)); });
};

struct tuple_trail_vec {
    void push_back(UTheoryTerm term) { vec.emplace_back(std::move(term)); }
    template <class Reader> void push_back(lexy::lexeme<Reader> /* unused */) { trail = true; }
    UTheoryTermVec vec;
    bool trail = false;
};

} // namespace detail

struct theory_term_tuple : detail::make_tuple<TheoryTermTupleType::Tuple> {
    static constexpr auto rule =
        dsl::round_bracketed.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::capture(dsl::lit_c<','>)));
    static constexpr auto
        value = lexy::as_list<detail::tuple_trail_vec> >>
                lexy::callback<UTheoryTerm>(
                    [](lexy::nullopt) -> UTheoryTerm {
                        return std::make_unique<TheoryTermTuple>(TheoryTermTupleType::Tuple, UTheoryTermVec{});
                    },
                    [](detail::tuple_trail_vec elems) -> UTheoryTerm {
                        if (elems.vec.size() == 1 && !elems.trail) {
                            return std::move(elems.vec.back());
                        }
                        return std::make_unique<TheoryTermTuple>(TheoryTermTupleType::Tuple, std::move(elems.vec));
                    });
};

struct theory_term_set : detail::make_tuple<TheoryTermTupleType::Set> {
    static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
};

struct theory_term_list : detail::make_tuple<TheoryTermTupleType::List> {
    static constexpr auto rule = dsl::square_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
};

struct theory_term_arguments {
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::vector<UTheoryTerm>>;
};

struct theory_term_function {
    static constexpr auto rule = dsl::p<identifier> >> dsl::if_(dsl::p<theory_term_arguments>);
    static constexpr auto value = lexy::new_<TheoryTermFunction, UTheoryTerm>;
};

struct theory_term_variable {
    static constexpr auto rule = dsl::p<variable>;
    static constexpr auto value = lexy::new_<TheoryTermVariable, UTheoryTerm>;
};

struct theory_term_anonymous_variable {
    static constexpr auto rule = anonymous_variable;
    static constexpr auto value =
        lexy::callback<UTheoryTerm>([]() { return std::make_unique<TheoryTermVariable>("_"); });
};

struct theory_term_root {
    STRING_TAG(term, "theory term expected");
    static constexpr auto rule = dsl::p<theory_term_tuple> | dsl::p<theory_term_set> | dsl::p<theory_term_list> |
                                 dsl::p<theory_term_function> | constant | dsl::p<number> | dsl::p<string> |
                                 dsl::p<theory_term_variable> | dsl::p<theory_term_anonymous_variable> |
                                 dsl::error<expected_term>;
    static constexpr auto value = lexy::callback<UTheoryTerm>(
        lexy::forward<UTheoryTerm>, lexy::new_<TheoryTermInteger, UTheoryTerm>,
        lexy::new_<TheoryTermString, UTheoryTerm>, lexy::new_<TheoryTermConstant, UTheoryTerm>);
};

struct theory_term_unparsed_guards {
    static constexpr auto rule = dsl::list(dsl::p<theory_ops> >> dsl::p<theory_term_root>);
    static constexpr auto value =
        lexy::collect<TheoryTermUnparsed::GuardVec>(lexy::construct<TheoryTermUnparsed::Guard>);
};

struct theory_term_unparsed : lexy::transparent_production {
    static constexpr auto rule =
        dsl::if_(dsl::p<theory_ops>) + dsl::p<theory_term_root> + dsl::if_(dsl::p<theory_term_unparsed_guards>);
    static constexpr auto value =
        lexy::callback<UTheoryTerm>(lexy::forward<UTheoryTerm>, lexy::new_<TheoryTermUnparsed, UTheoryTerm>);
};

struct theory_atom_element_tuple {
    static constexpr auto rule = dsl::list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<UTheoryTermVec>;
};

struct theory_atom_element {
    static constexpr auto rule =
        dsl::p<condition> | dsl::else_ >> dsl::p<theory_atom_element_tuple> + dsl::p<opt_condition>;
    static constexpr auto value =
        lexy::callback<TheoryAtom::Element>(lexy::construct<TheoryAtom::Element>, [](ULiteralVec cond) {
            return TheoryAtom::Element{UTheoryTermVec{}, std::move(cond)};
        });
};

struct theory_atom_elements {
    static constexpr auto rule =
        dsl::opt(dsl::curly_bracketed.opt_list(dsl::p<theory_atom_element>, dsl::sep(dsl::lit_c<';'>)));
    static constexpr auto value = lexy::as_list<TheoryAtom::ElementVec>;
};

struct theory_atom {
    static constexpr auto rule = []() {
        auto guard = dsl::p<theory_op> >> dsl::p<theory_term>;
        return LEXY_LIT("&") >> dsl::p<term_function> + dsl::p<theory_atom_elements> >> dsl::if_(guard);
    }();
    static constexpr auto value = lexy::construct<TheoryAtom>;
};

} // namespace grammar
