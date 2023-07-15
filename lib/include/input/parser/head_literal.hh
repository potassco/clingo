#pragma once

#include <input/head_literal.hh>

#include <input/parser/aggregate.hh>
#include <input/parser/literal.hh>
#include <input/parser/theory.hh>

namespace Gringo::Input::Grammar {

using SHeadAggregate = Util::shared_ptr<HeadAggregate>;
using SHeadSetAggregate = Util::shared_ptr<HeadSetAggregate>;

namespace Detail {

inline auto construct_head_aggr(SHeadAggregate aggr) -> SHeadAggregate { return aggr; }

inline auto construct_head_aggr(SetAggregate aggr) -> SHeadSetAggregate {
    return Util::construct_shared<HeadSetAggregate>(std::move(aggr));
}

struct construct_disjunction_element {
    using return_type = Disjunction::Element;
    auto operator()(std::pair<SLiteral, SLiteralVec> elem) const -> return_type {
        return {SLiteralVec{std::move(elem.first)}, std::move(elem.second)};
    }
};

} // namespace Detail

static constexpr auto disjunction_sep = LEXY_ASCII_ONE_OF(",;|");

struct simple_disjunction_element {
    static constexpr char const *name = "disjunction element";
    static constexpr auto rule = dsl::opt(dsl::list(disjunction_sep >> dsl::p<conditional_literal>));
    static constexpr auto value = lexy::collect<Disjunction::ElementVec>(Detail::construct_disjunction_element{}) >>
                                  lexy::callback<Disjunction::ElementVec>(lexy::forward<Disjunction::ElementVec>,
                                                                          [](lexy::nullopt) {
                                                                              return Disjunction::ElementVec{};
                                                                          });
};

struct simple_disjunction {
    static constexpr char const *name = "disjunction";
    static constexpr auto rule = dsl::list(dsl::p<conditional_literal>, dsl::sep(disjunction_sep));
    static constexpr auto value = lexy::collect<Disjunction::ElementVec>(Detail::construct_disjunction_element{}) >>
                                  lexy::new_<Disjunction, SHeadLiteral>;
};

struct disjunction_element : private junction_element<Disjunction::Element> {
    using junction_element::rule;
    using junction_element::value;
};

struct disjunction : private junction<disjunction_element, Disjunction, HeadLiteral> {
    static constexpr auto rule = make_rule(LEXY_KEYWORD("#or", keyword_base));
    using junction::value;
};

struct head_aggregate_element {
    static constexpr char const *name = "head aggregate element";
    static constexpr auto rule = []() {
        auto tuple = dsl::opt(dsl::peek_not(LEXY_LIT(":")) >> dsl::p<term_list>);
        return tuple + LEXY_LIT(":") + dsl::p<literal> + dsl::p<opt_condition>;
    }();
    static constexpr auto value = lexy::callback<HeadAggregate::Element>(
        [](std::optional<TermVec> tuple, SLiteral lit, std::optional<SLiteralVec> cond) {
            auto ret = HeadAggregate::Element{TermVec{}, std::move(lit), SLiteralVec{}};
            if (tuple) {
                std::get<0>(ret) = std::move(tuple).value();
            }
            if (cond) {
                std::get<2>(ret) = std::move(cond).value();
            }
            return ret;
        });
};

struct head_aggregate_elements {
    static constexpr char const *name = "head aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<head_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<HeadAggregate::ElementVec>;
};

struct head_aggregate {
    static constexpr char const *name = "head aggregate";
    static constexpr auto rule = dsl::p<aggregate_function> >> dsl::p<head_aggregate_elements> + aggregate_right_guard;
    static constexpr auto value = lexy::callback<SHeadAggregate>(
        lexy::new_<HeadAggregate, SHeadAggregate>,
        [](AggregateFunction fun, HeadAggregate::ElementVec elems, Term rhs) {
            return Util::construct_shared<HeadAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct head_literal {
    static constexpr char const *name = "head literal";
    using scan_result = lexy::scan_result<Term>;

    static constexpr auto is_atom = dsl::context_flag<head_literal>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    static constexpr auto rule = []() {
        auto with_rel =                                      //
            dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
            dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition> +
                              dsl::p<simple_disjunction_element>;

        auto with_term =                                                                     //
            dsl::p<relation> >> with_rel | dsl::p<head_aggregate> | dsl::p<set_aggregate> |  //
            is_atom.is_set() >> dsl::p<opt_condition> + dsl::p<simple_disjunction_element> | //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        auto peek = dsl::peek(kw_not | dsl::symbol<atom_bool::bool_symbols>(keyword_base));

        return peek >> dsl::p<simple_disjunction> | dsl::p<disjunction> |             //
               dsl::p<theory_atom> | dsl::p<head_aggregate> | dsl::p<set_aggregate> | //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();

    static constexpr auto value = lexy::callback<SHeadLiteral>(
        lexy::forward<SHeadLiteral>, lexy::new_<HeadTheoryAtom, SHeadLiteral>,
        lexy::new_<HeadSetAggregate, SHeadLiteral>,
        [](Term term, auto aggr) {
            auto ret = Detail::construct_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](Term term, Relation rel, auto aggr) {
            auto ret = Detail::construct_head_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return ret;
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, SLiteralVec cond,
           Disjunction::ElementVec elems) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            elems.insert(elems.begin(),
                         Disjunction::Element{SLiteralVec{Util::construct_shared<LiteralRelation, Literal>(
                                                  std::move(lhs), std::move(guards))},
                                              std::move(cond)});
            return Util::construct_shared<Disjunction, HeadLiteral>(std::move(elems));
        },
        [](Term term, SLiteralVec cond, Disjunction::ElementVec elems) {
            elems.insert(
                elems.begin(),
                Disjunction::Element{SLiteralVec{Util::construct_shared<LiteralSymbolic, Literal>(std::move(term))},
                                     std::move(cond)});
            return Util::construct_shared<Disjunction, HeadLiteral>(std::move(elems));
        });
};

} // namespace Gringo::Input::Grammar
