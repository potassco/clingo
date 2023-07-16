#pragma once

#include <input/body_literal.hh>

#include <input/parser/aggregate.hh>
#include <input/parser/base.hh>
#include <input/parser/literal.hh>
#include <input/parser/theory.hh>

namespace Gringo::Input::Grammar {

using SBodyAggregate = Util::shared_ptr<BodyAggregate>;
using SBodySetAggregate = Util::shared_ptr<BodySetAggregate>;

namespace Detail {

inline auto construct_body_aggr(SBodyAggregate aggr) -> SBodyAggregate { return aggr; }

inline auto construct_body_aggr(SetAggregate aggr) -> SBodySetAggregate {
    return Util::construct_shared<BodySetAggregate>(std::move(aggr));
}

auto construct_conjunction(SLiteral lit, SLiteralVec cond) {
    return Util::construct_shared<Conjunction, BodyLiteral>(
        Conjunction::ElementVec{Conjunction::Element{SLiteralVec{std::move(lit)}, std::move(cond)}});
}

} // namespace Detail

struct body_aggregate_element {
    static constexpr char const *name = "body aggregate element";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT(":"));
        return dsl::opt(peek >> dsl::p<term_list>) + dsl::p<opt_condition>;
    }();
    static constexpr auto value =
        lexy::callback<BodyAggregate::Element>([](std::optional<TermVec> tuple, std::optional<SLiteralVec> cond) {
            auto ret = BodyAggregate::Element{TermVec{}, SLiteralVec{}};
            if (tuple) {
                std::get<0>(ret) = std::move(tuple).value();
            }
            if (cond) {
                std::get<1>(ret) = std::move(cond).value();
            }
            return ret;
        });
};

struct body_aggregate_elements {
    static constexpr char const *name = "body aggregate elements";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(LEXY_LIT("}"));
        auto elems = dsl::list(dsl::p<body_aggregate_element>, dsl::sep(LEXY_LIT(";")));
        return LEXY_LIT("{") + dsl::opt(peek >> elems) + LEXY_LIT("}");
    }();
    static constexpr auto value = lexy::as_list<BodyAggregate::ElementVec>;
};

struct body_aggregate {
    static constexpr char const *name = "body aggregate";
    static constexpr auto rule = dsl::p<aggregate_function> >> dsl::p<body_aggregate_elements> + aggregate_right_guard;
    static constexpr auto value = lexy::callback<SBodyAggregate>(
        Detail::construct_shared<BodyAggregate, BodyAggregate>,
        [](AggregateFunction fun, BodyAggregate::ElementVec elems, Term rhs) {
            return Util::construct_shared<BodyAggregate>(fun, std::move(elems), Relation::less_equal, std::move(rhs));
        });
};

struct body_atom : lexy::transparent_production {
    static constexpr char const *name = "body atom";
    using scan_result = lexy::scan_result<Term>;

    static constexpr auto is_atom = dsl::context_flag<body_atom>;

    STRING_TAG(rel_aggr, "relation or aggregate expected");

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> scan_result {
        auto res_term = scanner.template parse<Term>(dsl::p<term>);
        if (res_term.has_value() && check_type(res_term.value(), TermCheckType::atom)) {
            scanner.parse(is_atom.set());
        }
        return res_term;
    }

    static constexpr auto rule = []() {
        auto with_rel = dsl::p<body_aggregate> | dsl::p<set_aggregate> |
                        dsl::else_ >> dsl::p<term> + dsl::opt(dsl::p<right_guards>) + dsl::p<opt_condition>;

        auto with_term =                                     //
            dsl::p<relation> >> with_rel |                   //
            dsl::p<body_aggregate> | dsl::p<set_aggregate> | //
            is_atom.is_set() >> dsl::p<opt_condition> |      //
            dsl::else_ >> dsl::error<expected_rel_aggr>;

        return dsl::p<theory_atom> | dsl::p<body_aggregate> | dsl::p<set_aggregate> | //
               dsl::p<atom_bool> >> dsl::p<opt_condition> |                           //
               dsl::else_ >> is_atom.create() + dsl::scan + with_term;
    }();
    static constexpr auto value = lexy::callback<SBodyLiteral>(
        lexy::forward<SBodyLiteral>, Detail::construct_shared<BodySetAggregate, BodyLiteral>,
        Detail::construct_shared<BodyTheoryAtom, BodyLiteral>,
        [](Term term, auto aggr) {
            auto ret = Detail::construct_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), Relation::less_equal);
            return ret;
        },
        [](Term term, Relation rel, auto aggr) {
            auto ret = Detail::construct_body_aggr(std::move(aggr));
            ret->set_left_guard(std::move(term), rel);
            return ret;
        },
        [](Term lhs, Relation rel, Term rhs, std::optional<GuardVec> opt_guards, SLiteralVec cond) {
            GuardVec guards;
            if (opt_guards.has_value()) {
                guards = std::move(opt_guards).value();
            }
            guards.insert(guards.begin(), Guard{rel, std::move(rhs)});
            auto lit = Util::construct_shared<LiteralRelation, Literal>(std::move(lhs), std::move(guards));
            return Detail::construct_conjunction(std::move(lit), std::move(cond));
        },
        [](SLiteral lit, SLiteralVec cond) { return Detail::construct_conjunction(std::move(lit), std::move(cond)); },
        [](Term term, SLiteralVec cond) {
            auto lit = Util::construct_shared<LiteralSymbolic, Literal>(std::move(term));
            return Detail::construct_conjunction(std::move(lit), std::move(cond));
        });
};

struct conjunction_element : private junction_element<Conjunction::Element> {
    using junction_element::rule;
    using junction_element::value;
};

struct conjunction : private junction<conjunction_element, Conjunction, BodyLiteral> {
    static constexpr auto rule = junction::make_rule(LEXY_KEYWORD("#and", keyword_base));
    using junction::value;
};

struct body_literal {
    static constexpr char const *name = "body literal";
    static constexpr auto rule = dsl::p<conjunction> | dsl::else_ >> dsl::p<naf_sign> + dsl::p<body_atom>;
    static constexpr auto value =
        lexy::callback<SBodyLiteral>(lexy::forward<SBodyLiteral>, [](Sign sign, SBodyLiteral literal) {
            literal->add_sign(sign);
            return literal;
        });
};

} // namespace Gringo::Input::Grammar
