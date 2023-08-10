#pragma once

#include <input/statement.hh>

#include "base.hh"
#include "body_literal.hh"
#include "head_literal.hh"

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

        return dsl::p<theory_op> >> dsl::colon + simple_number + dsl::comma + type;
    }();
    static constexpr auto value =
        lexy::callback<TheoryOpDefinition>(lexy::construct<TheoryOpDefinition>, [](std::string name, int arity) {
            return TheoryOpDefinition{std::move(name), arity, TheoryOpType::unary};
        });
};

struct theory_term_definition {
    static constexpr char const *name = "theory term definition";
    static constexpr auto rule = []() {
        auto id = dsl::p<identifier>;
        auto op_def = dsl::p<theory_op_definition>;
        auto sep = dsl::sep(dsl::semicolon);
        return id >> dsl::curly_bracketed.opt_list(op_def, sep);
    }();
    static constexpr auto
        value = lexy::as_list<TheoryOpDefinitionVec> >>
                lexy::callback<TheoryTermDefinition>(lexy::construct<TheoryTermDefinition>,
                                                     [](std::string name, lexy::nullopt) {
                                                         return TheoryTermDefinition{std::move(name), {}};
                                                     });
};

struct theory_guard_definition {
    static constexpr char const *name = "theory guard definition";
    static constexpr auto rule = []() {
        auto rels = dsl::curly_bracketed.list(dsl::p<theory_op>, dsl::sep(dsl::comma));
        return rels >> dsl::comma + dsl::p<identifier>;
    }();
    static constexpr auto value = []() {
        auto sink = lexy::as_list<std::vector<std::string>>;
        auto cb = lexy::construct<TheoryAtomDefinition::RHS::value_type>;
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
        auto sig = dsl::p<identifier> + dsl::slash + simple_number;
        auto term = dsl::p<identifier>;
        auto guard = dsl::opt(dsl::p<theory_guard_definition> >> dsl::comma);
        auto type = dsl::symbol<sym_type>(identifier_base);
        return dsl::ampersand >> sig + dsl::colon + term + dsl::comma + guard + type;
    }();
    static constexpr auto value = lexy::construct<TheoryAtomDefinition>;
};

struct theory_definitions {
    static constexpr char const *name = "theory definitions";
    STRING_TAG(atom, "atom definition expected");
    struct value_type {
        void push_back(TheoryTermDefinition term_def) { term_defs.push_back(std::move(term_def)); }
        void push_back(TheoryAtomDefinition atom_def) { atom_defs.push_back(std::move(atom_def)); }
        TheoryTermDefinitionVec term_defs;
        TheoryAtomDefinitionVec atom_defs;
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
        return kw_theory >> dsl::p<identifier> + dsl::curly_bracketed.opt(dsl::p<theory_definitions>) + eos;
    }();
    static constexpr auto value = lexy::callback<Statement>(
        // Note: called during error recovery if the expression between the
        // parenthesis did not match.
        [](std::string name) {
            return TheoryDefinition{std::move(name), TheoryTermDefinitionVec{}, TheoryAtomDefinitionVec{}};
        },
        [](std::string name, lexy::nullopt) {
            return TheoryDefinition{std::move(name), TheoryTermDefinitionVec{}, TheoryAtomDefinitionVec{}};
        },
        [](std::string name, theory_definitions::value_type defs) {
            return TheoryDefinition{std::move(name), std::move(defs.term_defs), std::move(defs.atom_defs)};
        });
};

struct statement_body {
    static constexpr char const *name = "body";
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::period);
        auto sep = dsl::sep(dsl::comma / dsl::semicolon);
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<BodyLiteralVec>;
};

struct statement_opt_body {
    static constexpr char const *name = "body";
    static constexpr auto rule = dsl::if_(dsl::colon >> dsl::p<statement_body>);
    static constexpr auto value = lexy::construct<BodyLiteralVec>;
};

struct statement_optimize_tuple {
    static constexpr char const *name = "tuple";
    static constexpr auto rule = []() {
        auto prio = dsl::if_(dsl::at_sign >> dsl::p<term>);
        auto terms = dsl::opt(dsl::list(dsl::comma >> dsl::p<term>));
        return dsl::p<term> + prio + terms;
    }();
    static constexpr auto
        value = lexy::as_list<TermVec> >>
                lexy::callback<StatementOptimize::Tuple>(
                    [](Term weight, std::optional<TermVec> terms) -> StatementOptimize::Tuple {
                        return {std::move(weight), std::nullopt, std::move(terms).value_or(TermVec{})};
                    },
                    [](Term weight, Term priority, std::optional<TermVec> terms) -> StatementOptimize::Tuple {
                        return {std::move(weight), std::move(priority), std::move(terms).value_or(TermVec{})};
                    });
};

struct statement_optimize_element {
    static constexpr char const *name = "optimize element";
    static constexpr auto rule = dsl::p<statement_optimize_tuple> + dsl::p<opt_condition>;
    static constexpr auto value =
        lexy::callback<StatementOptimize::Element>([](StatementOptimize::Tuple tuple, OptCondition cond) {
            return StatementOptimize::Element{std::move(tuple), std::move(cond).first};
        });
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
        return opt | weak;
    }();
    static constexpr auto
        value = lexy::as_list<StatementOptimize::ElementVec> >>
                lexy::callback<Statement>(
                    [](OptimizeType type, std::optional<StatementOptimize::ElementVec> elems) -> Statement {
                        return StatementOptimize{type, std::move(elems).value_or(StatementOptimize::ElementVec{})};
                    },
                    Detail::construct_v<StatementWeakConstraint, Statement>);
};

struct is_signature : control {
    static constexpr auto rule = dsl::if_(LEXY_LIT("-")) + dsl::p<identifier> + dsl::slash + simple_number;
};

struct statement_show {
    static constexpr char const *name = "show directive";
    static constexpr auto rule = []() {
        auto show = LEXY_KEYWORD("#show", identifier_base);
        auto opt_body = dsl::opt(LEXY_LIT(":") >> dsl::p<statement_body>);
        return show >> dsl::position + dsl::p<term> + dsl::position + opt_body + eos;
    }();
    static constexpr auto value = lexy::callback<Statement>(
        [](auto begin, Term term, auto end, lexy::nullopt) -> Statement {
            CheckTypeResult res;
            if (check_type(term, TermCheckType::sig, &res)) {
                // Note that parsing via the range input does not pass the state
                // to the whitespace parser, which is exactly as intended here.
                auto input = lexy::range_input<encoding, decltype(begin)>{begin, end};
                if (lexy::match<is_signature>(input)) {
                    return StatementShowSig{res.has_sign, res.identifier, res.pos_number};
                }
            }
            return StatementShow{std::move(term), BodyLiteralVec{}};
        },
        [](auto begin, Term term, auto end, BodyLiteralVec body) -> Statement {
            static_cast<void>(begin);
            static_cast<void>(end);
            return StatementShow{std::move(term), std::move(body)};
        });
};

struct sign_classical {
    static constexpr char const *name = "classical negation";
    static constexpr auto rule = dsl::opt(LEXY_LIT("-"));
    static constexpr auto value = lexy::callback<bool>([](lexy::nullopt) { return false; }, []() { return true; });
};

struct symbolic_atom {
    static constexpr char const *name = "symbolic atom";
    static constexpr auto rule = dsl::if_(dsl::position(LEXY_LIT("-"))) + dsl::p<term_function>;
    static constexpr auto value = Detail::with_state<Term>(
        [](auto &state, auto begin, Term term) {
            return TermUnary{Location(state.pos(begin), location(term).end), UnaryOperator::negate, std::move(term)};
        },
        [](auto &state, Term term) {
            static_cast<void>(state);
            return term;
        });
};

struct statement_defined {
    static constexpr char const *name = "defined directive";
    static constexpr auto rule = []() {
        auto def = LEXY_KEYWORD("#defined", keyword_base);
        return def >> dsl::p<sign_classical> + dsl::p<identifier> + dsl::slash + simple_number + eos;
    }();
    static constexpr auto value = Detail::construct_v<StatementDefined, Statement>;
};

struct statement_edge {
    static constexpr char const *name = "edge directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#edge", keyword_base);
        auto pair = dsl::p<term> + dsl::comma + dsl::p<term>;
        auto edge = dsl::round_bracketed.list(pair, dsl::sep(dsl::semicolon));
        return kw >> edge + dsl::p<statement_opt_body> + eos;
    }();
    static constexpr auto value = []() {
        auto sink = lexy::collect<StatementEdge::EdgeVec>(lexy::construct<StatementEdge::Edge>);
        auto cb = Detail::construct_v<StatementEdge, Statement>;
        return sink >> cb;
    }();
};

struct statement_heuristic {
    static constexpr char const *name = "heuristic directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#heuristic", keyword_base);
        auto tuple =
            square_bracketed_end(dsl::p<term> + dsl::if_(dsl::at_sign >> dsl::p<term>) + dsl::comma + dsl::p<term>);
        return kw >> dsl::p<symbolic_atom> + dsl::p<statement_opt_body> + eos + tuple;
    }();
    static constexpr auto value = Detail::construct_v<StatementHeuristic, Statement>;
};

struct statement_project {
    static constexpr char const *name = "project directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#project", keyword_base);
        auto arity = dsl::slash >> simple_number;
        auto pool = dsl::if_(LEXY_LIT("(") >> dsl::p<term_function_pool> + Detail::post_position(LEXY_LIT(")"))) +
                    dsl::p<statement_opt_body>;
        auto name = dsl::position(dsl::p<sign_classical> + dsl::inline_<identifier>);
        return kw >> name + (arity | dsl::else_ >> pool) + eos;
    }();
    static constexpr auto value = Detail::with_state<Statement>(
        [](auto &state, auto begin_sign, bool has_sign, auto name, int arity) {
            static_cast<void>(state);
            static_cast<void>(begin_sign);
            return StatementProjectSig{has_sign, Detail::as_string(name), arity};
        },
        [](auto &state, auto begin_sign, bool has_sign, auto name, PoolVec pool, auto end_atom, BodyLiteralVec body) {
            Term atom = TermFunction{Detail::loc(state, name.begin(), end_atom), Detail::as_string(name),
                                     std::move(pool), false};
            if (has_sign) {
                atom = TermUnary{Detail::loc(state, begin_sign, end_atom), UnaryOperator::negate, std::move(atom)};
            }
            return StatementProject{std::move(atom), std::move(body)};
        },
        [](auto &state, auto begin_sign, bool has_sign, auto name, BodyLiteralVec body) {
            Term atom = TermFunction{Detail::loc(state, name), Detail::as_string(name), PoolVec{TupleVec{}}, false};
            if (has_sign) {
                atom = TermUnary{Detail::loc(state, begin_sign, name.end()), UnaryOperator::negate, std::move(atom)};
            }
            return StatementProject{std::move(atom), std::move(body)};
        });
};

struct statement_script {
    static constexpr char const *name = "script block";
    static constexpr auto sym_type = lexy::symbol_table<ScriptType> //
                                         .map<LEXY_SYMBOL("lua")>(ScriptType::lua)
                                         .map<LEXY_SYMBOL("python")>(ScriptType::python);
    static constexpr auto rule = []() {
        auto script = LEXY_KEYWORD("#script", keyword_base);
        auto open = LEXY_LIT("(");
        auto type = dsl::symbol<sym_type>(identifier_base);
        auto close = LEXY_LIT(")");
        auto end = LEXY_KEYWORD("#end", keyword_base);
        return script >> open + type + dsl::delimited(close, end)(dsl::code_point) + eos;
    }();
    static constexpr auto value = Detail::as_string >> Detail::construct_v<StatementScript, Statement>;
};

struct statement_external {
    static constexpr char const *name = "external directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#external", keyword_base);
        auto atom = dsl::position(dsl::p<sign_classical> + dsl::p<term_function>);
        return kw >> atom + dsl::p<statement_opt_body> + eos + dsl::if_(square_bracketed_end(dsl::p<term>));
    }();
    static constexpr auto value =
        Detail::with_state<Statement>([](auto &state, auto begin_atom, bool has_sign, Term atom, auto &&...args) {
            if (has_sign) {
                atom = TermUnary{Location(state.pos(begin_atom), location(atom).end), UnaryOperator::negate,
                                 std::move(atom)};
            }
            return StatementExternal{std::move(atom), std::forward<decltype(args)>(args)...};
        });
};

struct statement_include {
    static constexpr char const *name = "include directive";
    STRING_TAG(path, "path expected");
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#include", keyword_base);
        auto sys = dsl::delimited(LEXY_LIT("<"), LEXY_LIT(">"))(dsl::ascii::alpha_digit_underscore);
        return kw >> (string | sys >> dsl::nullopt | dsl::error<expected_path>)+eos;
    }();
    static constexpr auto value = Detail::as_string >>
                                  lexy::callback<Statement>(
                                      [](std::string path) {
                                          return StatementInclude{IncludeType::system, std::move(path)};
                                      },
                                      [](std::string path, lexy::nullopt) {
                                          return StatementInclude{IncludeType::inbuild, std::move(path)};
                                      });
};

struct statement_program {
    static constexpr char const *name = "program directive";
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#program", keyword_base);
        auto id = dsl::p<identifier>;
        return kw >> id + dsl::opt(dsl::round_bracketed.opt_list(id, dsl::sep(dsl::comma))) + eos;
    }();
    static constexpr auto value = lexy::as_list<std::vector<std::string>> >>
                                  lexy::callback<Statement>(
                                      [](std::string name, std::vector<std::string> args) {
                                          return StatementProgram{std::move(name), std::move(args)};
                                      },
                                      [](std::string name, lexy::nullopt) {
                                          return StatementProgram{std::move(name), std::vector<std::string>{}};
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
        return kw >> id + dsl::equal_sign + dsl::p<term> + eos + type;
    }();
    static constexpr auto value = lexy::callback<Statement>(
        [](std::string name, Term value) {
            return StatementConst{ConstType::default_, std::move(name), std::move(value)};
        },
        [](std::string name, Term value, ConstType type) {
            return StatementConst{type, std::move(name), std::move(value)};
        });
};

struct statement_rule {
    static constexpr char const *name = "rule";
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<statement_body>;
        return (if_body | dsl::else_ >> dsl::p<head_literal> + dsl::if_(if_body)) + eos;
    }();
    static constexpr auto value = lexy::callback<Statement>(
        Detail::construct_v<Rule, Statement>,
        [](HeadLiteral head) {
            return Rule{std::move(head), BodyLiteralVec{}};
        },
        [](BodyLiteralVec body) {
            return Rule{Disjunction{ConditionalLiteralVec{}}, std::move(body)};
        });
};

struct statement {
    static constexpr char const *name = "statement";
    static constexpr auto rule = dsl::p<statement_theory> | dsl::p<statement_optimize> | dsl::p<statement_show> |
                                 dsl::p<statement_defined> | dsl::p<statement_edge> | dsl::p<statement_heuristic> |
                                 dsl::p<statement_project> | dsl::p<statement_script> | dsl::p<statement_external> |
                                 dsl::p<statement_include> | dsl::p<statement_program> | dsl::p<statement_const> |
                                 dsl::else_ >> dsl::p<statement_rule>;
    static constexpr auto value = lexy::forward<Statement>;
};

} // namespace Gringo::Input::Grammar
