#pragma once

#include <statement.hh>

#include <parser/base.hh>
#include <parser/body_literal.hh>
#include <parser/head_literal.hh>

namespace grammar {

struct theory_op_definition {
    static constexpr auto sym_types = lexy::symbol_table<TheoryOpType> //
                                          .map<LEXY_SYMBOL("left")>(TheoryOpType::binary_left)
                                          .map<LEXY_SYMBOL("right")>(TheoryOpType::binary_right);
    static constexpr auto rule = []() {
        auto unary = LEXY_KEYWORD("unary", identifier_base);
        auto binary = LEXY_KEYWORD("binary", identifier_base);
        auto associativity = dsl::symbol<sym_types>(identifier_base);
        auto type = unary | binary >> dsl::comma + associativity;

        return dsl::p<theory_op> >> dsl::colon + simple_number + dsl::comma + type;
    }();
    static constexpr auto value =
        lexy::callback<TheoryOpDefinition>(lexy::construct<TheoryOpDefinition>, [](std::string name, int arity) {
            return TheoryOpDefinition{std::move(name), arity, TheoryOpType::unary};
        });
};

struct theory_term_definition {
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
    static constexpr auto rule = []() {
        auto rels = dsl::curly_bracketed.list(dsl::p<theory_op>, dsl::sep(dsl::comma));
        return rels >> dsl::comma + dsl::p<identifier>;
    }();
    static constexpr auto value = []() {
        auto sink = lexy::as_list<std::vector<std::string>>;
        auto cb = lexy::construct<TheoryAtomDefinition::Guard::value_type>;
        return sink >> cb;
    }();
};

struct theory_atom_definition {
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
    struct value_type {
        void push_back(TheoryTermDefinition term_def) { term_defs.push_back(std::move(term_def)); }
        void push_back(TheoryAtomDefinition atom_def) { atom_defs.push_back(std::move(atom_def)); }
        TheoryTermDefinitionVec term_defs;
        TheoryAtomDefinitionVec atom_defs;
    };
    static constexpr auto is_atom_def = dsl::context_flag<theory_definitions>;
    static constexpr auto rule = []() {
        auto def = dsl::p<theory_atom_definition> >> is_atom_def.set() |
                   is_atom_def.is_reset() >> dsl::p<theory_term_definition>;
        return is_atom_def.create() + dsl::list(def, dsl::sep(dsl::semicolon));
    }();
    static constexpr auto value = lexy::as_list<value_type>;
};

struct statement_theory {
    static constexpr auto rule = []() {
        auto kw_theory = LEXY_KEYWORD("#theory", identifier_base);
        return kw_theory >> dsl::p<identifier> + dsl::curly_bracketed.opt(dsl::p<theory_definitions>) + dsl::period;
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        // TODO: where is this coming from
        [](std::string name) {
            return std::make_unique<TheoryDefinition>(std::move(name), TheoryTermDefinitionVec{},
                                                      TheoryAtomDefinitionVec{});
        },
        [](std::string name, lexy::nullopt) {
            return std::make_unique<TheoryDefinition>(std::move(name), TheoryTermDefinitionVec{},
                                                      TheoryAtomDefinitionVec{});
        },
        [](std::string name, theory_definitions::value_type defs) {
            return std::make_unique<TheoryDefinition>(std::move(name), std::move(defs.term_defs),
                                                      std::move(defs.atom_defs));
        });
};

struct statement_body {
    static constexpr auto rule = []() {
        auto peek = dsl::peek_not(dsl::period);
        auto sep = dsl::sep(dsl::comma / dsl::semicolon);
        return dsl::opt(peek >> dsl::list(dsl::p<body_literal>, sep));
    }();
    static constexpr auto value = lexy::as_list<UBodyLiteralVec>;
};

struct statement_optimize_tuple {
    static constexpr auto rule = []() {
        auto prio = dsl::opt(dsl::at_sign >> dsl::p<term>);
        auto terms = dsl::opt(dsl::list(dsl::comma >> dsl::p<term>));
        return dsl::p<term> + prio + terms;
    }();
    static constexpr auto
        value = lexy::as_list<UTermVec> >>
                lexy::callback<StatementOptimize::Tuple>([](UTerm weight, UTerm priority,
                                                            std::optional<UTermVec> terms) -> StatementOptimize::Tuple {
                    // NOTE: lexy behaves a bit funny constructing a std::optional<std::unique_ptr<T>>
                    auto opt_prio = std::optional<UTerm>{std::nullopt};
                    if (priority) {
                        opt_prio = std::move(priority);
                    }
                    return {std::move(weight), std::move(opt_prio), std::move(terms).value_or(UTermVec{})};
                });
};

struct statement_optimize_element {
    static constexpr auto rule = dsl::p<statement_optimize_tuple> + dsl::p<opt_condition>;
    static constexpr auto value = lexy::construct<StatementOptimize::Element>;
};

struct statement_optimize {
    static constexpr auto sym_type = lexy::symbol_table<OptimizeType> //
                                         .map<LEXY_SYMBOL("#minimize")>(OptimizeType::minimize)
                                         .map<LEXY_SYMBOL("#minimise")>(OptimizeType::minimize)
                                         .map<LEXY_SYMBOL("#maximize")>(OptimizeType::maximize)
                                         .map<LEXY_SYMBOL("#maximise")>(OptimizeType::maximize);
    static constexpr auto rule = []() {
        auto elems = dsl::curly_bracketed.opt_list(dsl::p<statement_optimize_element>, dsl::sep(dsl::semicolon));
        auto opt = dsl::symbol<sym_type>(keyword_base) >> elems + dsl::period;
        auto tuple = dsl::square_bracketed(dsl::p<statement_optimize_tuple>);
        auto weak = LEXY_LIT(":~") >> dsl::p<statement_body> + dsl::period + tuple;
        return opt | weak;
    }();
    static constexpr auto value = lexy::as_list<StatementOptimize::ElementVec> >>
                                  lexy::callback<UStatement>(
                                      [](OptimizeType type, std::optional<StatementOptimize::ElementVec> elems) {
                                          return std::make_unique<StatementOptimize>(
                                              type, std::move(elems).value_or(StatementOptimize::ElementVec{}));
                                      },
                                      lexy::new_<StatementWeakConstraint, UStatement>);
};

struct statement_show {
    static constexpr auto rule = []() {
        auto show = LEXY_KEYWORD("#show", identifier_base);
        auto opt_body = dsl::opt(LEXY_LIT(":") >> dsl::p<statement_body>);
        return show >> dsl::p<term> + opt_body + dsl::period;
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        [](UTerm term, lexy::nullopt) -> UStatement {
            CheckTypeResult res;
            // Note: this will match any term of form -a/2 including ones with
            // parenthesis inside. Avoiding this would be a bit tricky.
            if (term->check_type(TermCheckType::sig, &res)) {
                return std::make_unique<StatementShowSig>(res.has_sign, res.identifier, res.pos_number);
            }
            return std::make_unique<StatementShow>(std::move(term), UBodyLiteralVec{});
        },
        lexy::new_<StatementShow, UStatement>);
};

struct sign_classical {
    static constexpr auto rule = dsl::opt(LEXY_LIT("-"));
    static constexpr auto value = lexy::callback<bool>([](lexy::nullopt) { return false; }, []() { return true; });
};

struct statement_defined {
    static constexpr auto rule = []() {
        auto def = LEXY_KEYWORD("#defined", keyword_base);
        auto opt_body = dsl::opt(LEXY_LIT(":") >> dsl::p<statement_body>);
        return def >> dsl::p<sign_classical> + dsl::p<identifier> + dsl::slash + dsl::p<number> + dsl::period;
    }();
    static constexpr auto value = lexy::new_<StatementDefined, UStatement>;
};

struct statement_edge {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#edge", keyword_base);
        auto pair = dsl::p<term> + dsl::comma + dsl::p<term>;
        auto edge = dsl::round_bracketed.list(pair, dsl::sep(dsl::semicolon));
        return kw >> edge + dsl::if_(dsl::colon >> dsl::p<statement_body>) + dsl::period;
    }();
    static constexpr auto value = []() {
        auto sink = lexy::collect<StatementEdge::EdgeVec>(lexy::construct<StatementEdge::Edge>);
        auto cb = lexy::new_<StatementEdge, UStatement>;
        return sink >> cb;
    }();
};

struct statement_opt_body {
    static constexpr auto rule = dsl::if_(dsl::colon >> dsl::p<statement_body>);
    static constexpr auto value = lexy::construct<UBodyLiteralVec>;
};

struct statement_heuristic {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#heuristic", keyword_base);
        auto atom = dsl::p<sign_classical> + dsl::p<term_function>;
        auto tuple =
            dsl::square_bracketed(dsl::p<term> + dsl::if_(dsl::at_sign >> dsl::p<term>) + dsl::comma + dsl::p<term>);
        return kw >> atom + dsl::p<statement_opt_body> + dsl::period + tuple;
    }();
    static constexpr auto value = lexy::new_<StatementHeuristic, UStatement>;
};

struct statement_project {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#project", keyword_base);
        auto arity = dsl::slash >> simple_number;
        auto pool = dsl::opt(dsl::p<term_function_pool>) + dsl::p<statement_opt_body>;
        auto name = dsl::p<sign_classical> + dsl::p<identifier>;
        return kw >> name + (arity | dsl::else_ >> pool) + dsl::period;
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<StatementProjectSig, UStatement>,
        [](bool has_sign, std::string name, std::optional<UTermVecVec> pool, UBodyLiteralVec body) {
            UTerm atom =
                std::make_unique<TermFunction>(std::move(name), std::move(pool).value_or(UTermVecVec{}), false);
            if (has_sign) {
                atom = std::make_unique<TermUnary>(UnaryOperator::negate, std::move(atom));
            }
            return std::make_unique<StatementProject>(std::move(atom), std::move(body));
        });
};

struct statement_script {
    static constexpr auto sym_type = lexy::symbol_table<ScriptType> //
                                         .map<LEXY_SYMBOL("lua")>(ScriptType::lua)
                                         .map<LEXY_SYMBOL("python")>(ScriptType::python);
    static constexpr auto rule = []() {
        auto script = LEXY_KEYWORD("#script", keyword_base);
        auto open = LEXY_LIT("(");
        auto type = dsl::symbol<sym_type>(identifier_base);
        auto close = LEXY_LIT(")");
        auto end = LEXY_KEYWORD("#end", keyword_base);
        return script >> open + type + dsl::delimited(close, end)(dsl::code_point) + dsl::period;
    }();
    static constexpr auto value = lexy::as_string<std::string, encoding> >> lexy::new_<StatementScript, UStatement>;
};

struct statement_external {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#external", keyword_base);
        auto atom = dsl::p<sign_classical> + dsl::p<term_function>;
        return kw >> atom + dsl::p<statement_opt_body> + dsl::period + dsl::if_(dsl::square_bracketed(dsl::p<term>));
    }();
    static constexpr auto value = lexy::callback<UStatement>([](bool has_sign, UTerm atom, auto &&...args) {
        if (has_sign) {
            atom = std::make_unique<TermUnary>(UnaryOperator::negate, std::move(atom));
        }
        return std::make_unique<StatementExternal>(std::move(atom), std::forward<decltype(args)>(args)...);
    });
};

struct statement_include {
    static constexpr auto rule = []() {
        auto kw = LEXY_KEYWORD("#include", keyword_base);
        auto sys = dsl::delimited(LEXY_LIT("<"), LEXY_LIT(">"))(dsl::ascii::alpha_digit_underscore);
        return kw >> (dsl::inline_<string> | sys >> dsl::nullopt) + dsl::period;
    }();
    static constexpr auto value =
        lexy::as_string<std::string, encoding> >>
        lexy::callback<UStatement>(
            [](std::string path) { return std::make_unique<StatementInclude>(IncludeType::system, std::move(path)); },
            [](std::string path, lexy::nullopt) {
                return std::make_unique<StatementInclude>(IncludeType::inbuild, std::move(path));
            });
};

// TODO
/*/////////////////// CONST STATEMENTS /////////////////////

// like Term excluding variables, pools, and intervals
ConstTerm ::= ...
Const ::= '#const' Identifier '=' ConstTerm '.'
          ('[' ('default' | 'override') ']')?

//////////////////// BLOCK STATEMENTS //////////////////////

Params ::= Identifier (',' Identifier)?
Block ::= '#program' Identifier ('(' Params? ')')? '.'
*/

struct statement_rule {
    static constexpr auto rule = []() {
        auto if_body = LEXY_LIT(":-") >> dsl::p<statement_body> + dsl::period;
        return if_body | dsl::else_ >> dsl::p<head_literal> + (dsl::period | if_body);
    }();
    static constexpr auto value = lexy::callback<UStatement>(
        lexy::new_<Rule, UStatement>,
        [](UHeadLiteral head) { return std::make_unique<Rule>(std::move(head), UBodyLiteralVec{}); },
        [](UBodyLiteralVec body) {
            return std::make_unique<Rule>(std::make_unique<Disjunction>(Disjunction::ElementVec{}), std::move(body));
        });
};

struct statement {
    static constexpr auto rule = dsl::p<statement_theory> | dsl::p<statement_optimize> | dsl::p<statement_show> |
                                 dsl::p<statement_defined> | dsl::p<statement_edge> | dsl::p<statement_heuristic> |
                                 dsl::p<statement_project> | dsl::p<statement_script> | dsl::p<statement_external> |
                                 dsl::p<statement_include> | dsl::else_ >> dsl::p<statement_rule>;
    static constexpr auto value = lexy::forward<UStatement>;
};

} // namespace grammar
