#pragma once

#include <input/theory.hh>

#include "aggregate.hh"

namespace Gringo::Input::Grammar {

namespace Detail {

struct theory_tuple_trail {
    void push_back(TheoryTerm term) { vec.emplace_back(std::move(term)); }
    template <class Reader> void push_back(lexy::lexeme<Reader> /* unused */) { trail = true; }
    TheoryTermVec vec;
    bool trail = false;
};

template <TheoryTermTupleType type>
static constexpr auto theory_tuple = Detail::with_state<TheoryTerm>(
    [](auto &state, auto begin, lexy::nullopt, auto end) {
        return TheoryTermTuple{Detail::loc(state, begin, end), type, TheoryTermVec{}};
    },
    [](auto &state, auto begin, TheoryTermVec elems, auto end) {
        return TheoryTermTuple{Detail::loc(state, begin, end), type, std::move(elems)};
    },
    [](auto &state, auto begin, Detail::theory_tuple_trail elems, auto end) -> TheoryTerm {
        if (elems.vec.size() == 1 && !elems.trail) {
            return std::move(elems.vec.back());
        }
        return TheoryTermTuple{Detail::loc(state, begin, end), TheoryTermTupleType::tuple, std::move(elems.vec)};
    });

} // namespace Detail

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

    static constexpr auto value = lexy::callback<std::string>(Detail::as_string, lexy::constant("not"));
};

struct theory_ops {
    static constexpr char const *name = "theory operators";
    static constexpr auto rule = dsl::list(dsl::p<theory_op>);
    static constexpr auto value = lexy::as_list<std::vector<std::string>>;
};

struct theory_term {
    static constexpr char const *name = "theory term";
    static constexpr auto rule = dsl::recurse<struct theory_term_unparsed>;
    static constexpr auto value = lexy::forward<TheoryTerm>;
};

struct theory_term_tuple {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = dsl::brackets(dsl::position(LEXY_LIT("(")), Detail::post_position(LEXY_LIT(")")))
                                     .opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::capture(dsl::lit_c<','>)));
    static constexpr auto value =
        lexy::as_list<Detail::theory_tuple_trail> >> Detail::theory_tuple<TheoryTermTupleType::tuple>;
};

struct theory_term_set {
    static constexpr char const *name = "theory term set";
    static constexpr auto rule = dsl::brackets(dsl::position(LEXY_LIT("{")), Detail::post_position(LEXY_LIT("}")))
                                     .opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<TheoryTermVec> >> Detail::theory_tuple<TheoryTermTupleType::set>;
};

struct theory_term_list {
    static constexpr char const *name = "theory term list";
    static constexpr auto rule = dsl::brackets(dsl::position(LEXY_LIT("[")), Detail::post_position(LEXY_LIT("]")))
                                     .opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<TheoryTermVec> >> Detail::theory_tuple<TheoryTermTupleType::list>;
};

struct theory_term_arguments {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<std::vector<TheoryTerm>>;
};

struct theory_term_function {
    static constexpr char const *name = "theory function";
    static constexpr auto rule = dsl::inline_<identifier> >> dsl::if_(dsl::p<theory_term_arguments>);
    static constexpr auto value = Detail::with_state<TheoryTerm>([](auto &state, auto id,
                                                                    std::vector<TheoryTerm> args = {}) {
        auto begin = state.pos(id.begin());
        auto end = args.empty() ? state.pos(id.end()) : location(args.back()).end;
        return TheoryTermFunction{Location{std::move(begin), std::move(end)}, Detail::as_string(id), std::move(args)};
    });
};

struct theory_term_variable : lexy::token_production {
    static constexpr char const *name = "variable";
    static constexpr auto rule = dsl::capture(dsl::token(variable));
    static constexpr auto value = Detail::with_state<TheoryTerm>([](auto &state, auto var) {
        return TheoryTermVariable{Detail::loc(state, var), Detail::as_string(var)};
    });
};

struct theory_term_anonymous_variable : lexy::token_production {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = dsl::position(anonymous_variable);
    static constexpr auto value = Detail::with_state<TheoryTerm>([](auto &state, auto pos) {
        return TheoryTermVariable{Detail::loc(state, pos, pos + 1), "_", true};
    });
};

struct theory_term_number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = dsl::position(number >> dsl::position);
    static constexpr auto value = Detail::with_state<TheoryTerm>([](auto &state, auto begin, auto num, auto end) {
        return TheoryTermSymbol(Detail::loc(state, begin, end), num);
    });
};

struct theory_term_string : lexy::token_production {
    static constexpr char const *name = "string";
    static constexpr auto rule = dsl::position(string >> dsl::position);
    static constexpr auto value = Detail::as_string >>
                                  Detail::with_state<TheoryTerm>([](auto &state, auto begin, auto str, auto end) {
                                      return TheoryTermSymbol{Detail::loc(state, begin, end), QuotedString{str}};
                                  });
};

struct theory_term_constant : lexy::token_production {
    static constexpr char const *name = "constant";
    static constexpr auto rule = dsl::position(constant >> dsl::position);
    static constexpr auto value = Detail::with_state<TheoryTerm>([](auto &state, auto begin, auto val, auto end) {
        return TheoryTermSymbol(Detail::loc(state, begin, end), val);
    });
};

struct theory_term_root {
    static constexpr char const *name = "theory term";
    STRING_TAG(term, "theory term expected");
    static constexpr auto rule =
        dsl::p<theory_term_tuple> | dsl::p<theory_term_set> | dsl::p<theory_term_list> | dsl::p<theory_term_function> |
        dsl::p<theory_term_constant> | dsl::p<theory_term_number> | dsl::p<theory_term_string> |
        dsl::p<theory_term_variable> | dsl::p<theory_term_anonymous_variable> | dsl::error<expected_term>;
    static constexpr auto value = lexy::forward<TheoryTerm>;
};

struct theory_term_unparsed_guards {
    static constexpr char const *name = "theory term guards";
    static constexpr auto rule = dsl::list(dsl::p<theory_ops> >> dsl::p<theory_term_root>);
    static constexpr auto value =
        lexy::collect<TheoryTermUnparsed::ElementVec>(lexy::construct<TheoryTermUnparsed::Element>);
};

struct theory_term_unparsed : lexy::transparent_production {
    static constexpr char const *name = "theory term";
    static constexpr auto rule = dsl::position + dsl::if_(dsl::p<theory_ops>) + dsl::p<theory_term_root> +
                                 dsl::if_(dsl::p<theory_term_unparsed_guards>);
    static constexpr auto value = Detail::with_state<TheoryTerm>(
        [](auto &state, auto begin, TheoryTerm term) {
            static_cast<void>(state);
            static_cast<void>(begin);
            return term;
        },
        [](auto &state, auto begin, std::vector<std::string> ops, TheoryTerm term,
           TheoryTermUnparsed::ElementVec guards = {}) {
            guards.insert(guards.begin(), TheoryTermUnparsed::Element{std::move(ops), std::move(term)});
            auto loc = Location{state.pos(begin), location(guards.back().second).end};
            return TheoryTermUnparsed{std::move(loc), std::move(guards)};
        },
        [](auto &state, auto begin, TheoryTerm term, TheoryTermUnparsed::ElementVec guards) {
            guards.insert(guards.begin(), TheoryTermUnparsed::Element{{}, std::move(term)});
            auto loc = Location{state.pos(begin), location(guards.back().second).end};
            return TheoryTermUnparsed{std::move(loc), std::move(guards)};
        });
};

struct theory_atom_element_tuple {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = dsl::list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>));
    static constexpr auto value = lexy::as_list<TheoryTermVec>;
};

struct theory_atom_element {
    static constexpr char const *name = "theory atom element";
    static constexpr auto rule =
        dsl::p<condition> | dsl::else_ >> dsl::p<theory_atom_element_tuple> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::callback<TheoryAtom::Element>(
        [](TheoryTermVec tuple, OptCondition cond) {
            return TheoryAtom::Element{std::move(tuple), std::move(cond.first)};
        },
        [](LiteralVec cond) {
            return TheoryAtom::Element{TheoryTermVec{}, std::move(cond)};
        });
};

struct theory_atom_elements {
    using value_type = std::pair<TheoryAtom::ElementVec, std::optional<Position>>;
    static constexpr char const *name = "theory atom elements";
    static constexpr auto rule = []() {
        auto braces = dsl::brackets(LEXY_LIT("{"), Detail::post_position(LEXY_LIT("}")));
        auto elems = dsl::if_(braces.opt_list(dsl::p<theory_atom_element>, dsl::sep(dsl::lit_c<';'>)));
        return elems;
    }();
    static constexpr auto value = lexy::as_list<TheoryAtom::ElementVec> >>
                                  Detail::with_state<value_type>(
                                      [](auto &state, lexy::nullopt, auto end) {
                                          return value_type{{}, state.pos(end)};
                                      },
                                      [](auto &state, TheoryAtom::ElementVec elems, auto end) {
                                          return value_type{std::move(elems), state.pos(end)};
                                      },
                                      [](auto &state) {
                                          static_cast<void>(state);
                                          return value_type{{}, std::nullopt};
                                      });
};

struct theory_atom {
    static constexpr char const *name = "theory atom";
    static constexpr auto rule = []() {
        auto guard = dsl::p<theory_op> >> dsl::p<theory_term>;
        return dsl::position(LEXY_LIT("&")) >> dsl::p<term_function> + dsl::p<theory_atom_elements> + dsl::if_(guard);
    }();
    static constexpr auto value = Detail::with_state<TheoryAtom>(
        [](auto &state, auto begin, Term name, theory_atom_elements::value_type elems, std::string op, TheoryTerm rhs) {
            auto loc = Location{state.pos(begin), location(rhs).end};
            return TheoryAtom{std::move(loc), std::move(name), std::move(elems.first),
                              std::make_pair(std::move(op), std::move(rhs))};
        },
        [](auto &state, auto begin, Term name, theory_atom_elements::value_type elems) {
            auto loc = Location{state.pos(begin), std::move(elems.second).value_or(location(name).end)};
            return TheoryAtom{std::move(loc), std::move(name), std::move(elems.first), std::nullopt};
        });
};

} // namespace Gringo::Input::Grammar
