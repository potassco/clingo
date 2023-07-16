#pragma once

#include <input/theory.hh>

#include <input/parser/aggregate.hh>
#include <input/parser/literal.hh>

namespace Gringo::Input::Grammar {

struct theory_op {
    static constexpr char const *name = "theory operator";
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
    static constexpr char const *name = "theory operators";
    static constexpr auto rule = dsl::list(dsl::p<theory_op>);
    static constexpr auto value = lexy::as_list<std::vector<std::string>>;
};

struct theory_term {
    static constexpr char const *name = "theory term";
    static constexpr auto rule = dsl::recurse<struct theory_term_unparsed>;
    static constexpr auto value = lexy::forward<STheoryTerm>;
};

namespace Detail {

template <TheoryTermTupleType type> struct make_tuple {
    static constexpr auto value =
        lexy::as_list<STheoryTermVec> >>
        lexy::callback<STheoryTerm>(
            [](lexy::nullopt) { return Util::construct_shared<TheoryTermTuple, TheoryTerm>(type, STheoryTermVec{}); },
            [](STheoryTermVec elems) {
                return Util::construct_shared<TheoryTermTuple, TheoryTerm>(type, std::move(elems));
            });
};

struct tuple_trail_vec {
    void push_back(STheoryTerm term) { vec.emplace_back(std::move(term)); }
    template <class Reader> void push_back(lexy::lexeme<Reader> /* unused */) { trail = true; }
    STheoryTermVec vec;
    bool trail = false;
};

} // namespace Detail

struct theory_term_tuple : Detail::make_tuple<TheoryTermTupleType::Tuple> {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule =
        dsl::round_bracketed.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::capture(dsl::lit_c<','>)));
    static constexpr auto value = lexy::as_list<Detail::tuple_trail_vec> >>
                                  lexy::callback<STheoryTerm>(
                                      [](lexy::nullopt) -> STheoryTerm {
                                          return Util::construct_shared<TheoryTermTuple, TheoryTerm>(
                                              TheoryTermTupleType::Tuple, STheoryTermVec{});
                                      },
                                      [](Detail::tuple_trail_vec elems) -> STheoryTerm {
                                          if (elems.vec.size() == 1 && !elems.trail) {
                                              return std::move(elems.vec.back());
                                          }
                                          return Util::construct_shared<TheoryTermTuple, TheoryTerm>(
                                              TheoryTermTupleType::Tuple, std::move(elems.vec));
                                      });
};

struct theory_term_set : Detail::make_tuple<TheoryTermTupleType::Set> {
    static constexpr char const *name = "theory term set";
    static constexpr auto rule = dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
};

struct theory_term_list : Detail::make_tuple<TheoryTermTupleType::List> {
    static constexpr char const *name = "theory term list";
    static constexpr auto rule = dsl::square_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
};

struct theory_term_arguments {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::vector<STheoryTerm>>;
};

struct theory_term_function {
    static constexpr char const *name = "theory function";
    static constexpr auto rule = dsl::p<identifier> >> dsl::if_(dsl::p<theory_term_arguments>);
    static constexpr auto value = Detail::construct_shared<TheoryTermFunction, TheoryTerm>;
};

struct theory_term_variable {
    static constexpr char const *name = "variable";
    static constexpr auto rule = dsl::p<variable>;
    static constexpr auto value = Detail::construct_shared<TheoryTermVariable, TheoryTerm>;
};

struct theory_term_anonymous_variable {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = anonymous_variable;
    static constexpr auto value =
        lexy::callback<STheoryTerm>([]() { return Util::construct_shared<TheoryTermVariable, TheoryTerm>("_", true); });
};

struct theory_term_root {
    static constexpr char const *name = "theory term";
    STRING_TAG(term, "theory term expected");
    static constexpr auto rule = dsl::p<theory_term_tuple> | dsl::p<theory_term_set> | dsl::p<theory_term_list> |
                                 dsl::p<theory_term_function> | constant | dsl::p<number> | dsl::p<string> |
                                 dsl::p<theory_term_variable> | dsl::p<theory_term_anonymous_variable> |
                                 dsl::error<expected_term>;
    static constexpr auto value = lexy::callback<STheoryTerm>(
        lexy::forward<STheoryTerm>,
        [](int value) { return Util::construct_shared<TheoryTermSymbol, TheoryTerm>(Symbol{value}); },
        [](std::string value) {
            return Util::construct_shared<TheoryTermSymbol, TheoryTerm>(Symbol{QuotedString{value}});
        },
        [](Constant value) { return Util::construct_shared<TheoryTermSymbol, TheoryTerm>(Symbol{value}); });
};

struct theory_term_unparsed_guards {
    static constexpr char const *name = "theory term guards";
    static constexpr auto rule = dsl::list(dsl::p<theory_ops> >> dsl::p<theory_term_root>);
    static constexpr auto value =
        lexy::collect<TheoryTermUnparsed::ElementVec>(lexy::construct<TheoryTermUnparsed::Element>);
};

struct theory_term_unparsed : lexy::transparent_production {
    static constexpr char const *name = "theory term";
    static constexpr auto rule =
        dsl::if_(dsl::p<theory_ops>) + dsl::p<theory_term_root> + dsl::if_(dsl::p<theory_term_unparsed_guards>);
    static constexpr auto value = lexy::callback<STheoryTerm>(
        lexy::forward<STheoryTerm>,
        [](std::vector<std::string> ops, STheoryTerm term, TheoryTermUnparsed::ElementVec guards = {}) {
            guards.insert(guards.begin(), TheoryTermUnparsed::Element{std::move(ops), std::move(term)});
            return construct_shared<TheoryTermUnparsed, TheoryTerm>(std::move(guards));
        },
        [](STheoryTerm term, TheoryTermUnparsed::ElementVec guards) {
            guards.insert(guards.begin(), TheoryTermUnparsed::Element{{}, std::move(term)});
            return construct_shared<TheoryTermUnparsed, TheoryTerm>(std::move(guards));
        });
};

struct theory_atom_element_tuple {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = dsl::list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<STheoryTermVec>;
};

struct theory_atom_element {
    static constexpr char const *name = "theory atom element";
    static constexpr auto rule =
        dsl::p<condition> | dsl::else_ >> dsl::p<theory_atom_element_tuple> + dsl::p<opt_condition>;
    static constexpr auto value =
        lexy::callback<TheoryAtom::Element>(lexy::construct<TheoryAtom::Element>, [](SLiteralVec cond) {
            return TheoryAtom::Element{STheoryTermVec{}, std::move(cond)};
        });
};

struct theory_atom_elements {
    static constexpr char const *name = "theory atom elements";
    static constexpr auto rule =
        dsl::opt(dsl::curly_bracketed.opt_list(dsl::p<theory_atom_element>, dsl::sep(dsl::lit_c<';'>)));
    static constexpr auto value = lexy::as_list<TheoryAtom::ElementVec>;
};

struct theory_atom {
    static constexpr char const *name = "theory atom";
    static constexpr auto rule = []() {
        auto guard = dsl::p<theory_op> >> dsl::p<theory_term>;
        return LEXY_LIT("&") >> dsl::p<term_function> + dsl::p<theory_atom_elements> >> dsl::if_(guard);
    }();
    static constexpr auto value = lexy::construct<TheoryAtom>;
};

} // namespace Gringo::Input::Grammar
