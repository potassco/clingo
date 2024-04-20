#pragma once

#include "base.hh"
#include "body_literal.hh"
#include "head_literal.hh"

#include <gringo/input/statement.hh>

namespace Gringo::Input::Grammar {

struct mark_end : lexy::scan_production<void> {
    using scan_result = lexy::scan_result<void>;

    template <typename Reader, typename Context>
    static auto scan(lexy::rule_scanner<Context, Reader> &scanner) -> lexy::scan_result<void> {
        if constexpr (Detail::has_state<lexy::rule_scanner<Context, Reader>>) {
            scanner.remaining_input().reader().state().mark();
        }
        return true;
    }
};

static constexpr auto eos = dsl::p<mark_end> + dsl::period;

struct theory_op_definition {
    static constexpr char const *name = "theory operator definition";
    STRING_TAG(assoc, "left or right expected");

    static constexpr auto sym_type = lexy::symbol_table<TheoryOpType> //
                                         .map<LEXY_SYMBOL("left")>(TheoryOpType::binary_left)
                                         .map<LEXY_SYMBOL("right")>(TheoryOpType::binary_right);
    static constexpr auto rule = []() {
        auto unary = LEXY_KEYWORD("unary", identifier_base);
        auto binary = LEXY_KEYWORD("binary", identifier_base);
        auto associativity = dsl::symbol<sym_type>(identifier_base);
        auto type = unary | binary >> dsl::comma + associativity | dsl::error<expected_assoc>;

        return Detail::location(dsl::p<theory_op> >> dsl::colon + smallint + dsl::comma + type);
    }();
    static constexpr auto value = lexy::callback<TheoryOpDefinition>(
        lexy::construct<TheoryOpDefinition>,
        [](Location loc, String name, int arity) { return TheoryOpDefinition{loc, name, arity, TheoryOpType::unary}; });
};

struct theory_term_definition {
    static constexpr char const *name = "theory term definition";
    static constexpr auto rule = []() {
        auto id = dsl::p<identifier>;
        auto op_def = dsl::p<theory_op_definition>;
        auto sep = dsl::sep(dsl::semicolon);
        return Detail::location(id >> dsl::curly_bracketed.opt_list(op_def, sep));
    }();
    static constexpr auto value = lexy::as_list<std::vector<TheoryOpDefinition>> >>
                                  lexy::callback<TheoryTermDefinition>(lexy::construct<TheoryTermDefinition>,
                                                                       [](Location loc, String name, lexy::nullopt) {
                                                                           return TheoryTermDefinition{loc, name, {}};
                                                                       });
};

struct theory_guard_definition {
    static constexpr char const *name = "theory guard definition";
    static constexpr auto rule = []() {
        auto rels = dsl::curly_bracketed.list(dsl::p<theory_op>, dsl::sep(dsl::comma));
        return rels >> dsl::comma + dsl::p<identifier>;
    }();
    static constexpr auto value = []() {
        auto sink = lexy::as_list<std::vector<String>>;
        auto cb = lexy::construct<TheoryRGuardDefinition>;
        return sink >> cb;
    }();
};

struct theory_atom_definition {
    static constexpr char const *name = "theory atom definition";
    static constexpr auto sym_type = lexy::symbol_table<TheoryAtomType>
        .map<LEXY_SYMBOL("head")>(TheoryAtomType::head)
        .map<LEXY_SYMBOL("body")>(TheoryAtomType::body)
        .map<LEXY_SYMBOL("any")>(TheoryAtomType::any)
        .map<LEXY_SYMBOL("directive")>(TheoryAtomType::directive);
    static constexpr auto rule = []() {
        auto sig = dsl::p<identifier> + dsl::slash + smallint;
        auto term = dsl::p<identifier>;
        auto guard = dsl::opt(dsl::p<theory_guard_definition> >> dsl::comma);
        auto type = dsl::symbol<sym_type>(identifier_base);
        return Detail::location(dsl::ampersand >> sig + dsl::colon + term + dsl::comma + guard + type);
    }();
    static constexpr auto value = lexy::construct<TheoryAtomDefinition>;
};

struct theory_definitions {
    static constexpr char const *name = "theory definitions";
    STRING_TAG(atom, "atom definition expected");
    struct value_type {
        void push_back(TheoryTermDefinition term_def) { term_defs.push_back(std::move(term_def)); }
        void push_back(TheoryAtomDefinition atom_def) { atom_defs.push_back(std::move(atom_def)); }
        std::vector<TheoryTermDefinition> term_defs;
        std::vector<TheoryAtomDefinition> atom_defs;
    };
    static constexpr auto is_atom_def = dsl::context_flag<theory_definitions>;
    static constexpr auto rule = []() {
        auto def = dsl::p<theory_atom_definition> >> is_atom_def.set() |
                   is_atom_def.is_reset() >> dsl::p<theory_term_definition> | dsl::error<expected_atom>;
        return is_atom_def.create() + dsl::list(def, dsl::sep(dsl::semicolon));
    }();
    static constexpr auto value = lexy::as_list<value_type>;
};

struct statement_theory {
    static constexpr char const *name = "theory definition";
    static constexpr auto rule = []() {
        auto kw_theory = LEXY_KEYWORD("#theory", identifier_base);
        return Detail::location(kw_theory >>
                                dsl::p<identifier> + dsl::curly_bracketed.opt(dsl::p<theory_definitions>) + eos);
    }();
    static constexpr auto value = lexy::callback<Stm>(
        // Note: called during error recovery if the expression between the
        // parenthesis did not match.
        [](Location loc, String name) {
            return StmTheory{loc, name, TheoryTermDefinitionArray{}, TheoryAtomDefinitionArray{}};
        },
        [](Location loc, String name, lexy::nullopt) {
            return StmTheory{loc, name, TheoryTermDefinitionArray{}, TheoryAtomDefinitionArray{}};
        },
        [](Location loc, String name, theory_definitions::value_type defs) {
            return StmTheory{loc, name, std::move(defs.term_defs), std::move(defs.atom_defs)};
        });
};

struct statement_body {
    static constexpr char const *name = "body";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::period);
        auto sep = dsl::sep(dsl::comma / dsl::semicolon);
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<std::vector<BdLit>>;
};

struct statement_opt_body {
    static constexpr char const *name = "body";
    static constexpr auto rule = dsl::if_(dsl::colon >> dsl::p<statement_body>);
    static constexpr auto value = lexy::construct<std::vector<BdLit>>;
};

struct statement_optimize_tuple {
    static constexpr char const *name = "tuple";
    static constexpr auto rule = []() {
        auto prio = dsl::if_(dsl::at_sign >> dsl::p<term>);
        auto terms = dsl::opt(dsl::list(dsl::comma >> dsl::p<term>));
        return dsl::p<term> + prio + terms;
    }();
    static constexpr auto value = lexy::as_list<std::vector<Term>> >>
                                  lexy::callback<OptimizeTuple>(
                                      [](Term weight, std::optional<std::vector<Term>> terms) {
                                          return OptimizeTuple{std::move(weight), std::nullopt,
                                                               std::move(terms).value_or(std::vector<Term>{})};
                                      },
                                      [](Term weight, Term priority, std::optional<std::vector<Term>> terms) {
                                          return OptimizeTuple{std::move(weight), std::move(priority),
                                                               std::move(terms).value_or(std::vector<Term>{})};
                                      });
};

struct statement_optimize_element {
    static constexpr char const *name = "optimize element";
    static constexpr auto rule = dsl::p<statement_optimize_tuple> + dsl::p<if_condition>;
    static constexpr auto value = lexy::callback<OptimizeElement>(
        [](OptimizeTuple tuple, LitArray cond) { return OptimizeElement{std::move(tuple), std::move(cond)}; });
};

constexpr auto square_bracketed_end =
    dsl::brackets(dsl::lit_c<'['>, dsl::peek(dsl::lit_c<']'>) >> dsl::p<mark_end> + dsl::lit_c<']'>);

struct statement_optimize {
    static constexpr char const *name = "optimize directive";
    static constexpr auto sym_type = lexy::symbol_table<OptimizeType> //
                                         .map<LEXY_SYMBOL("#minimize")>(OptimizeType::minimize)
                                         .map<LEXY_SYMBOL("#minimise")>(OptimizeType::minimize)
                                         .map<LEXY_SYMBOL("#maximize")>(OptimizeType::maximize)
                                         .map<LEXY_SYMBOL("#maximise")>(OptimizeType::maximize);
    static constexpr auto rule = []() {
        auto elems = dsl::curly_bracketed.opt_list(dsl::p<statement_optimize_element>, dsl::sep(dsl::semicolon));
        auto opt = dsl::symbol<sym_type>(keyword_base) >> elems + eos;
        auto tuple = square_bracketed_end(dsl::p<statement_optimize_tuple>);
        auto weak = LEXY_LIT(":~") >> dsl::p<statement_body> + eos + tuple;
        return Detail::location(opt | weak);
    }();
    static constexpr auto
        value = lexy::as_list<std::vector<OptimizeElement>> >>
                lexy::callback<Stm>(
                    [](Location loc, OptimizeType type, std::optional<OptimizeElementArray> elems) -> Stm {
                        return StmOptimize{loc, type, std::move(elems).value_or(OptimizeElementArray{})};
                    },
                    Detail::construct_v<StmWeakConstraint, Stm>);
};

struct is_signature : control {
    static constexpr auto rule = dsl::if_(LEXY_LIT("-")) + dsl::p<identifier> + dsl::slash + smallint;
};

struct statement_show {
    static constexpr char const *name = "show directive";
    static constexpr auto rule = []() {
        auto show = LEXY_KEYWORD("#show", identifier_base);
        auto opt_body = dsl::opt(LEXY_LIT(":") >> dsl::p<statement_body>);
        return Detail::location(show >> dsl::position + dsl::p<term> + dsl::position + opt_body + eos);
    }();
    static constexpr auto value = lexy::callback<Stm>(
        [](Location loc, auto begin, Term term, auto end, lexy::nullopt) -> Stm {
            CheckTypeResult res;
            if (check_type(term, TermCheckType::sig, &res)) {
                // Note that parsing via the range input does not pass the state
                // to the whitespace parser, which is exactly as intended here.
                auto input = lexy::range_input<encoding, decltype(begin)>{begin, end};
                if (lexy::match<is_signature>(input)) {
                    if (auto num = res.pos_number->as_int(); num.has_value()) {
                        return StmShowSig{loc, res.has_sign, res.identifier, *num};
                    }
                }
            }
            return StmShow{loc, std::move(term), BdLitArray{}};
        },
        [](Location loc, [[maybe_unused]] auto begin, Term term, [[maybe_unused]] auto end, BdLitArray body) -> Stm {
            return StmShow{loc, std::move(term), std::move(body)};
        });
};

struct sign_classical {
    static constexpr char const *name = "classical negation";
    static constexpr auto rule = dsl::opt(LEXY_LIT("-"));
    static constexpr auto value = lexy::callback<bool>([](lexy::nullopt) { return false; }, []() { return true; });
};

struct symbolic_atom {
    static constexpr char const *name = "symbolic atom";
    static constexpr auto rule = dsl::if_(Detail::position(LEXY_LIT("-"))) + dsl::p<term_function>;
    static constexpr auto value = lexy::callback<Term>(
        [](Position begin, Term term) {
            return TermUnary{begin + location(term), UnaryOperator::negate, std::move(term)};
        },
        [](Term term) { return term; });
};

struct statement_defined {
    static constexpr char const *name = "defined directive";
    static constexpr auto rule = []() {
        auto def = LEXY_KEYWORD("#defined", keyword_base);
        return Detail::location(def >> dsl::p<sign_classical> + dsl::p<identifier> + dsl::slash + smallint + eos);
    }();
    static constexpr auto value = Detail::construct_v<StmDefined, Stm>;
};

struct statement_edge {
    static constexpr char const *name = "edge directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#edge", keyword_base);
        auto pair = dsl::p<term> + dsl::comma + dsl::p<term>;
        auto edge = dsl::round_bracketed.list(pair, dsl::sep(dsl::semicolon));
        return Detail::location(kw >> edge + dsl::p<statement_opt_body> + eos);
    }();
    static constexpr auto value = []() {
        auto sink = lexy::collect<std::vector<Edge>>(lexy::construct<Edge>);
        auto cb = Detail::construct_v<StmEdge, Stm>;
        return sink >> cb;
    }();
};

struct statement_heuristic {
    static constexpr char const *name = "heuristic directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#heuristic", keyword_base);
        auto tuple =
            square_bracketed_end(dsl::p<term> + dsl::if_(dsl::at_sign >> dsl::p<term>) + dsl::comma + dsl::p<term>);
        return Detail::location(kw >> dsl::p<symbolic_atom> + dsl::p<statement_opt_body> + eos + tuple);
    }();
    static constexpr auto value = Detail::construct_v<StmHeuristic, Stm>;
};

struct statement_project {
    static constexpr char const *name = "project directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#project", keyword_base);
        auto arity = dsl::slash >> smallint;
        auto pool = dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + LEXY_LIT(")")) + dsl::p<statement_opt_body>;
        auto name = dsl::p<sign_classical> + Detail::position(dsl::p<identifier>);
        return Detail::location(kw >> Detail::location(name + (arity | dsl::else_ >> pool)) + eos);
    }();
    static constexpr auto value = lexy::callback<Stm>(
        [](Location loc, [[maybe_unused]] Location loc_term, bool has_sign, [[maybe_unused]] Position begin_name,
           String name, int arity) { return StmProjectSig{loc, has_sign, name, arity}; },
        [](Location loc, Location loc_term, bool has_sign, Position begin_name, String name, PoolArray pool,
           BdLitArray body) {
            Term atom = TermFunction{begin_name + loc_term, name, std::move(pool), false};
            if (has_sign) {
                atom = TermUnary{loc_term, UnaryOperator::negate, std::move(atom)};
            }
            return StmProject{loc, std::move(atom), std::move(body)};
        },
        [](Location loc, Location loc_term, bool has_sign, Position begin_name, String name, BdLitArray body) {
            Term atom = TermFunction{begin_name + loc_term, name, PoolArray{ArgumentTuple{ArgumentArray{}}}, false};
            if (has_sign) {
                atom = TermUnary{loc_term, UnaryOperator::negate, std::move(atom)};
            }
            return StmProject{loc, std::move(atom), std::move(body)};
        });
};

struct statement_script {
    static constexpr char const *name = "script block";
    static constexpr auto rule = []() {
        auto script = LEXY_KEYWORD("#script", keyword_base);
        auto open = LEXY_LIT("(");
        auto type = dsl::p<identifier>;
        auto close = LEXY_LIT(")");
        auto end = LEXY_KEYWORD("#end", keyword_base);
        return Detail::location(script >> open + type + dsl::delimited(close, end)(dsl::code_point) + eos);
    }();
    static constexpr auto value = Detail::as_string >> Detail::construct_v<StmScript, Stm>;
};

struct statement_external {
    static constexpr char const *name = "external directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#external", keyword_base);
        auto atom = Detail::location(dsl::p<sign_classical> + dsl::p<term_function>);
        return Detail::location(kw >>
                                atom + dsl::p<statement_opt_body> + eos + dsl::if_(square_bracketed_end(dsl::p<term>)));
    }();
    static constexpr auto value =
        lexy::callback<Stm>([](Location loc, Location loc_atom, bool has_sign, Term atom, auto &&...args) {
            if (has_sign) {
                atom = TermUnary{loc_atom, UnaryOperator::negate, std::move(atom)};
            }
            return StmExternal{loc, std::move(atom), std::forward<decltype(args)>(args)...};
        });
};

struct statement_include {
    static constexpr char const *name = "include directive";
    STRING_TAG(path, "path expected");
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#include", keyword_base);
        auto sys = dsl::delimited(LEXY_LIT("<"), LEXY_LIT(">"))(dsl::ascii::alpha_digit_underscore);
        return Detail::location(kw >> (string | sys >> dsl::nullopt | dsl::error<expected_path>)+eos);
    }();
    static constexpr auto value =
        Detail::as_string >>
        lexy::callback<Stm>([](Location loc,
                               std::string path) { return StmInclude{loc, IncludeType::system, std::move(path)}; },
                            [](Location loc, std::string path, lexy::nullopt) {
                                return StmInclude{loc, IncludeType::inbuild, std::move(path)};
                            });
};

struct statement_program {
    static constexpr char const *name = "program directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#program", keyword_base);
        auto id = dsl::p<identifier>;
        return Detail::location(kw >> id + dsl::opt(dsl::round_bracketed.opt_list(id, dsl::sep(dsl::comma))) + eos);
    }();
    static constexpr auto
        value = lexy::as_list<std::vector<String>> >>
                lexy::callback<Stm>([](Location loc, String name,
                                       std::vector<String> args) { return StmProgram{loc, name, std::move(args)}; },
                                    [](Location loc, String name, lexy::nullopt) {
                                        return StmProgram{loc, name, std::vector<String>{}};
                                    });
};

struct statement_const {
    static constexpr char const *name = "const directive";
    static constexpr auto sym_type = lexy::symbol_table<ConstType> //
                                         .map<LEXY_SYMBOL("default")>(ConstType::default_)
                                         .map<LEXY_SYMBOL("override")>(ConstType::override_);
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#const", keyword_base);
        auto id = dsl::p<identifier>;
        auto type = dsl::if_(square_bracketed_end(dsl::symbol<sym_type>(identifier_base)));
        // Note: we overparse here to avoid duplicating code
        return Detail::location(kw >> id + dsl::equal_sign + dsl::p<term> + eos + type);
    }();
    static constexpr auto value =
        lexy::callback<Stm>([](Location loc, String name,
                               Term value) { return StmConst{loc, ConstType::default_, name, std::move(value)}; },
                            [](Location loc, String name, Term value, ConstType type) {
                                return StmConst{loc, type, name, std::move(value)};
                            });
};

struct statement_rule {
    static constexpr char const *name = "rule";
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<statement_body>;
        return Detail::location((if_body | dsl::else_ >> dsl::p<head_literal> + dsl::if_(if_body)) + eos);
    }();
    static constexpr auto value = lexy::callback<Stm>(
        [](Location loc, HdLit head, BdLitArray body) { return StmRule{loc, std::move(head), std::move(body)}; },
        [](Location loc, HdLit head) { return StmRule{loc, std::move(head), BdLitArray{}}; },
        [](Location loc, BdLitArray body) {
            auto loc_head = loc + loc.begin;
            return StmRule{loc, HdLitSimple{LitBool{loc_head, Sign::none, false}}, std::move(body)};
        });
};

struct statement {
    static constexpr char const *name = "statement";
    static constexpr auto rule = dsl::p<statement_theory> | dsl::p<statement_optimize> | dsl::p<statement_show> |
                                 dsl::p<statement_defined> | dsl::p<statement_edge> | dsl::p<statement_heuristic> |
                                 dsl::p<statement_project> | dsl::p<statement_script> | dsl::p<statement_external> |
                                 dsl::p<statement_include> | dsl::p<statement_program> | dsl::p<statement_const> |
                                 dsl::else_ >> dsl::p<statement_rule>;
    static constexpr auto value = lexy::forward<Stm>;
};

} // namespace Gringo::Input::Grammar
