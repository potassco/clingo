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
static constexpr auto theory_tuple = lexy::callback<TheoryTerm>(
    [](Location loc, lexy::nullopt) {
        return TheoryTermTuple{std::move(loc), type, TheoryTermVec{}};
    },
    [](Location loc, TheoryTermVec elems) {
        return TheoryTermTuple{std::move(loc), type, std::move(elems)};
    },
    [](Location loc, Detail::theory_tuple_trail elems) -> TheoryTerm {
        if (elems.vec.size() == 1 && !elems.trail) {
            return std::move(elems.vec.back());
        }
        return TheoryTermTuple{std::move(loc), TheoryTermTupleType::tuple, std::move(elems.vec)};
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

    static constexpr auto value = lexy::callback_with_state<String>(
        [](auto &state, auto lexeme) { return state.string(Detail::as_string(lexeme)); },
        [](auto &state) { return state.string("not"); });
};

struct theory_ops {
    static constexpr char const *name = "theory operators";
    static constexpr auto rule = dsl::list(dsl::p<theory_op>);
    static constexpr auto value = lexy::as_list<std::vector<String>>;
};

struct theory_term {
    static constexpr char const *name = "theory term";
    static constexpr auto rule = dsl::recurse<struct theory_term_unparsed>;
    static constexpr auto value = lexy::forward<TheoryTerm>;
};

struct theory_term_tuple {
    static constexpr char const *name = "theory term tuple";
    static constexpr auto rule = Detail::location(
        dsl::parenthesized.opt_list(dsl::p<theory_term>, dsl::trailing_sep(dsl::capture(dsl::lit_c<','>))));
    static constexpr auto value =
        lexy::as_list<Detail::theory_tuple_trail> >> Detail::theory_tuple<TheoryTermTupleType::tuple>;
};

struct theory_term_set {
    static constexpr char const *name = "theory term set";
    static constexpr auto rule =
        Detail::location(dsl::curly_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)));
    static constexpr auto value = lexy::as_list<TheoryTermVec> >> Detail::theory_tuple<TheoryTermTupleType::set>;
};

struct theory_term_list {
    static constexpr char const *name = "theory term list";
    static constexpr auto rule =
        Detail::location(dsl::square_bracketed.opt_list(dsl::p<theory_term>, dsl::sep(dsl::lit_c<','>)));
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
    static constexpr auto value =
        lexy::callback_with_state<TheoryTerm>([](auto &state, auto id, std::vector<TheoryTerm> args = {}) {
            auto begin = state.pos(id.begin());
            auto end = args.empty() ? state.pos(id.end()) : location(args.back()).end;
            return TheoryTermFunction{Location{std::move(begin), std::move(end)}, state.string(Detail::as_string(id)),
                                      std::move(args)};
        });
};

struct theory_term_variable : lexy::token_production {
    static constexpr char const *name = "variable";
    static constexpr auto rule = dsl::capture(dsl::token(variable));
    static constexpr auto value = lexy::callback_with_state<TheoryTerm>([](auto &state, auto var) {
        return TheoryTermVariable{Location{state.pos(var.begin()), state.pos(var.end())},
                                  state.string(Detail::as_string(var))};
    });
};

struct theory_term_anonymous_variable : lexy::token_production {
    static constexpr char const *name = "anonymous variable";
    static constexpr auto rule = Detail::location(anonymous_variable);
    static constexpr auto value = lexy::callback_with_state<TheoryTerm>([](auto &state, Location loc) {
        return TheoryTermVariable{std::move(loc), state.string("_"), true};
    });
};

struct theory_term_number : lexy::token_production {
    static constexpr char const *name = "number";
    static constexpr auto rule = Detail::position(number >> Detail::position);
    static constexpr auto value = lexy::callback<TheoryTerm>([](Position begin, int num, Position end) {
        return TheoryTermSymbol(Location{std::move(begin), std::move(end)}, SymbolStore::num(num));
    });
};

struct theory_term_string : lexy::token_production {
    static constexpr char const *name = "string";
    static constexpr auto rule = Detail::location(string);
    static constexpr auto value = as_stored_string >> lexy::callback<TheoryTerm>([](Location loc, auto str) {
                                      return TheoryTermSymbol{std::move(loc), SymbolStore::str(str)};
                                  });
};

struct theory_term_constant : lexy::token_production {
    static constexpr char const *name = "constant";
    static constexpr auto rule = Detail::location(constant);
    static constexpr auto value = lexy::callback<TheoryTerm>([](Location loc, Constant val) {
        return TheoryTermSymbol(std::move(loc), val == Constant::infimum ? SymbolStore::inf() : SymbolStore::sup());
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
    static constexpr auto rule = Detail::position + dsl::if_(dsl::p<theory_ops>) + dsl::p<theory_term_root> +
                                 dsl::if_(dsl::p<theory_term_unparsed_guards>);
    static constexpr auto value = lexy::callback_with_state<TheoryTerm>(
        [](auto &state, auto begin, TheoryTerm term) {
            static_cast<void>(state);
            static_cast<void>(begin);
            return term;
        },
        [](auto &state, auto begin, std::vector<String> ops, TheoryTerm term,
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
    static constexpr auto value = lexy::callback<TheoryElement>(
        [](TheoryTermVec tuple, LiteralVec cond) {
            return TheoryElement{std::move(tuple), std::move(cond)};
        },
        [](LiteralVec cond) {
            return TheoryElement{TheoryTermVec{}, std::move(cond)};
        });
};

struct theory_atom_elements {
    static constexpr char const *name = "theory atom elements";
    static constexpr auto rule = []() {
        auto elems = dsl::if_(dsl::curly_bracketed.opt_list(dsl::p<theory_atom_element>, dsl::sep(dsl::lit_c<';'>)));
        return elems;
    }();
    static constexpr auto value = lexy::as_list<TheoryElementVec>;
};

template <bool HasSign> struct theory_atom {
    static constexpr char const *name = "theory atom";
    static constexpr auto rule = []() {
        auto guard = dsl::p<theory_op> >> dsl::p<theory_term>;
        auto atom = LEXY_LIT("&") >> dsl::p<term_function> + dsl::p<theory_atom_elements> + dsl::if_(guard);
        return Detail::location(atom);
    }();
    static constexpr auto value = lexy::callback<TheoryAtom<HasSign>>(
        [](Location loc, Term name, TheoryElementVec elems, String op, TheoryTerm rhs) {
            if constexpr (HasSign) {
                return TheoryAtom<HasSign>{std::move(loc), Sign::none, std::move(name), std::move(elems),
                                           std::make_pair(std::move(op), std::move(rhs))};
            } else {
                return TheoryAtom<HasSign>{std::move(loc), std::move(name), std::move(elems),
                                           std::make_pair(std::move(op), std::move(rhs))};
            }
        },
        [](Location loc, Term name, TheoryElementVec elems) {
            if constexpr (HasSign) {
                return TheoryAtom<HasSign>{std::move(loc), Sign::none, std::move(name), std::move(elems), std::nullopt};
            } else {
                return TheoryAtom<HasSign>{std::move(loc), std::move(name), std::move(elems), std::nullopt};
            }
        });
};

using body_theory_atom = theory_atom<true>;
using head_theory_atom = theory_atom<false>;

} // namespace Gringo::Input::Grammar
