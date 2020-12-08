// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include <clingo/astv2.hh>

namespace Gringo { namespace Input {

// {{{1 ast parsing

namespace {

template <class T>
T &get(AST &ast, char const *name) {
    return mpark::get<T>(ast.value(name));
}

template <class T>
T *getOpt(AST &ast, char const *name) {
    if (!ast.hasValue(name)) {
        return nullptr;
    }
    auto &value = ast.value(name);
    if (value.index() == 0) {
        return nullptr;
    }
    return &mpark::get<T>(value);
}

struct ASTParser {
public:
    ASTParser(Logger &log, INongroundProgramBuilder &prg)
    : log_(log), prg_(prg) { }

    // {{{2 statement

    void parseStatement(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_rule: {
                return prg_.rule(get<Location>(ast, "location"),
                                 parseHeadLiteral(*get<SAST>(ast, "head")),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")));
            }
            case clingo_ast_type_definition: {
                return prg_.define(get<Location>(ast, "location"),
                                   get<String>(ast, "name"), parseTerm(*get<SAST>(ast, "value")),
                                   get<int>(ast, "is_default") != 0, log_);
            }
            case clingo_ast_type_show_signature: {
                return prg_.showsig(get<Location>(ast, "location"),
                                    Sig(get<String>(ast, "name"),
                                        get<int>(ast, "arity"),
                                        get<int>(ast, "sign") != 0),
                                    ast.hasValue("csp") && get<int>(ast, "csp") != 0);
            }
            case clingo_ast_type_show_term: {
                return prg_.show(get<Location>(ast, "location"),
                                 parseTerm(*get<SAST>(ast, "term")),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")),
                                 ast.hasValue("csp") && get<int>(ast, "csp") != 0);
            }
            case clingo_ast_type_minimize: {
                return prg_.optimize(get<Location>(ast, "location"),
                                     parseTerm(*get<SAST>(ast, "weight")),
                                     parseTerm(*get<SAST>(ast, "priority")),
                                     parseTermVec(get<AST::ASTVec>(ast, "tuple")),
                                     parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")));
            }
            case clingo_ast_type_script: {
                switch (get<int>(ast, "type")) {
                    case clingo_ast_script_type_python: { return prg_.python(get<Location>(ast, "location"),
                                                                             get<String>(ast, "code")); }
                    case clingo_ast_script_type_lua:    { return prg_.lua(get<Location>(ast, "location"),
                                                                          get<String>(ast, "code")); }
                }
                break;
            }
            case clingo_ast_type_program: {
                return prg_.block(get<Location>(ast, "location"),
                                  get<String>(ast, "name"),
                                  parseIdVec(get<AST::ASTVec>(ast, "parameters")));
            }
            case clingo_ast_type_external: {
                return prg_.external(get<Location>(ast, "location"),
                                     parseAtom(*get<SAST>(ast, "atom")),
                                     parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")),
                                     parseTerm(*get<SAST>(ast, "type")));
            }
            case clingo_ast_type_edge: {
                return prg_.edge(get<Location>(ast, "location"),
                                 prg_.termvecvec(prg_.termvecvec(),
                                                 prg_.termvec(prg_.termvec(prg_.termvec(),
                                                                           parseTerm(*get<SAST>(ast, "u"))),
                                                              parseTerm(*get<SAST>(ast, "v")))),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")));
            }
            case clingo_ast_type_heuristic: {
                return prg_.heuristic(get<Location>(ast, "location"),
                                      parseAtom(*get<SAST>(ast, "atom")),
                                      parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")),
                                      parseTerm(*get<SAST>(ast, "bias")),
                                      parseTerm(*get<SAST>(ast, "priority")),
                                      parseTerm(*get<SAST>(ast, "modifier")));
            }
            case clingo_ast_type_project_atom: {
                return prg_.project(get<Location>(ast, "location"),
                                    parseAtom(*get<SAST>(ast, "atom")),
                                    parseBodyLiteralVec(get<AST::ASTVec>(ast, "body")));
            }
            case clingo_ast_type_project_signature: {
                return prg_.project(get<Location>(ast, "location"),
                                    Sig(get<String>(ast, "name"), get<int>(ast, "arity"), get<int>(ast, "sign") != 0));
            }
            case clingo_ast_type_defined: {
                return prg_.defined(get<Location>(ast, "location"),
                                    Sig(get<String>(ast, "name"), get<int>(ast, "arity"), get<int>(ast, "sign") != 0));
            }
            case clingo_ast_type_theory_definition: {
                return prg_.theorydef(get<Location>(ast, "location"),
                                      get<String>(ast, "name"),
                                      parseTheoryAtomDefinitionVec(parseTheoryTermDefinitionVec(get<AST::ASTVec>(ast, "terms")),
                                                                   get<AST::ASTVec>(ast, "atoms")),
                                      log_);
            }
            default: {
                throw std::runtime_error("invalid ast: statement expected");
            }
        }
    }

private:
    template <class T>
    T fail_(char const *message) {
        throw std::runtime_error(message);
    }

    bool require_(bool cond, char const *message) {
        if (!cond) { fail_<void>(message); }
        return false;
    }

    // {{{2 terms

    IdVecUid parseIdVec(AST::ASTVec &asts) {
        auto uid = prg_.idvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_id, "invalid ast: id required");
            prg_.idvec(uid, get<Location>(*ast, "location"), get<String>(*ast, "id"));
        }
        return uid;
    }

    static UnOp parseUnOp(int num) {
        switch (num) {
            case clingo_ast_unary_operator_minus: {
                return UnOp::NEG;
            }
            case clingo_ast_unary_operator_negation: {
                return UnOp::NOT;
            }
            case clingo_ast_unary_operator_absolute: {
                return UnOp::ABS;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid unary operator");
            }
        }
    }

    static BinOp parseBinOp(int num) {
        switch (num) {
            case clingo_ast_binary_operator_xor: {
                return BinOp::XOR;
            }
            case clingo_ast_binary_operator_or: {
                return BinOp::OR;
            }
            case clingo_ast_binary_operator_and: {
                return BinOp::AND;
            }
            case clingo_ast_binary_operator_plus: {
                return BinOp::ADD;
            }
            case clingo_ast_binary_operator_minus: {
                return BinOp::SUB;
            }
            case clingo_ast_binary_operator_multiplication: {
                return BinOp::MUL;
            }
            case clingo_ast_binary_operator_division: {
                return BinOp::DIV;
            }
            case clingo_ast_binary_operator_modulo: {
                return BinOp::MOD;
            }
            case clingo_ast_binary_operator_power: {
                return BinOp::POW;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid binary operator");
            }
        }
    }

    TermUid parseTerm(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_variable: {
                return prg_.term(get<Location>(ast, "location"),
                                 get<String>(ast, "name"));
            }
            case clingo_ast_type_symbol: {
                return prg_.term(get<Location>(ast, "location"),
                                 get<Symbol>(ast, "symbol"));
            }
            case clingo_ast_type_unary_operation: {
                return prg_.term(get<Location>(ast, "location"),
                                 parseUnOp(get<int>(ast, "unary_operator")),
                                 parseTerm(*get<SAST>(ast, "argument")));
            }
            case clingo_ast_type_binary_operation: {
                return prg_.term(get<Location>(ast, "location"),
                                 parseBinOp(get<int>(ast, "binary_operator")),
                                 parseTerm(*get<SAST>(ast, "left")),
                                 parseTerm(*get<SAST>(ast, "right")));
            }
            case clingo_ast_type_interval: {
                return prg_.term(get<Location>(ast, "location"),
                                 parseTerm(*get<SAST>(ast, "left")),
                                 parseTerm(*get<SAST>(ast, "right")));
            }
            case clingo_ast_type_function: {
                auto external = ast.hasValue("external") && get<int>(ast, "external") != 0;
                auto name = get<String>(ast, "name");
                require_(!name.empty() || !external, "invalid ast: external functions must have a name");
                return !name.empty()
                    ? prg_.term(get<Location>(ast, "location"),
                                name,
                                prg_.termvecvec(prg_.termvecvec(),
                                                parseTermVec(get<AST::ASTVec>(ast, "arguments"))),
                                external)
                    : prg_.term(get<Location>(ast, "location"),
                                parseTermVec(get<AST::ASTVec>(ast, "arguments")),
                                true);
            }
            case clingo_ast_type_pool: {
                return prg_.pool(get<Location>(ast, "location"),
                                 parseTermVec(get<AST::ASTVec>(ast, "arguments")));
            }
            default: {
                throw std::runtime_error("invalid ast: term expected");
            }
        }
    }

    TermVecUid parseTermVec(AST::ASTVec &asts) {
        TermVecUid uid = prg_.termvec();
        for (auto &ast : asts) {
            prg_.termvec(uid, parseTerm(*ast));
        }
        return uid;
    }

    CSPMulTermUid parseCSPMulTerm(AST &ast) {
        require_(ast.type() == clingo_ast_type_csp_product, "invalid ast: csp product required");
        auto *variable = getOpt<SAST>(ast, "variable");
        return variable != nullptr
            ? prg_.cspmulterm(get<Location>(ast, "location"),
                              parseTerm(*get<SAST>(ast, "coefficient")),
                              parseTerm(**variable))
            : prg_.cspmulterm(get<Location>(ast, "location"),
                              parseTerm(*get<SAST>(ast, "coefficient")));
    }

    CSPAddTermUid parseCSPAddTerm(AST &ast) {
        require_(ast.type() == clingo_ast_type_csp_sum, "invalid ast: csp sum required");
        auto &terms = get<AST::ASTVec>(ast, "terms");
        require_(!terms.empty(), "invalid ast: csp sums terms must not be empty");
        auto it = terms.begin();
        auto ie = terms.end();
        auto uid = prg_.cspaddterm(get<Location>(**it, "location"),
                                   parseCSPMulTerm(**it));
        for (++it; it != ie; ++it) {
            uid = prg_.cspaddterm(get<Location>(**it, "location"),
                                  uid,
                                  parseCSPMulTerm(**it), true);

        }
        return uid;
    }

    TheoryTermUid parseTheoryTerm(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_symbol : {
                return prg_.theorytermvalue(get<Location>(ast, "location"), get<Symbol>(ast, "symbol"));
            }
            case clingo_ast_type_variable: {
                return prg_.theorytermvar(get<Location>(ast, "location"), get<String>(ast, "variable"));
            }
            case clingo_ast_type_theory_sequence: {
                switch (get<int>(ast, "sequence_type")) {
                    case clingo_ast_theory_sequence_type_tuple: {
                        return prg_.theorytermtuple(get<Location>(ast, "location"),
                                                    parseTheoryOptermVec(get<AST::ASTVec>(ast, "terms")));
                    }
                    case clingo_ast_theory_sequence_type_list: {
                        return prg_.theoryoptermlist(get<Location>(ast, "location"),
                                                     parseTheoryOptermVec(get<AST::ASTVec>(ast, "terms")));
                    }
                    case clingo_ast_theory_sequence_type_set: {
                        return prg_.theorytermset(get<Location>(ast, "location"),
                                                  parseTheoryOptermVec(get<AST::ASTVec>(ast, "terms")));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: invalid theory sequence type");

                    }
                }
            }
            case clingo_ast_type_theory_function: {
                return prg_.theorytermfun(get<Location>(ast, "location"),
                                          get<String>(ast, "name"),
                                          parseTheoryOptermVec(get<AST::ASTVec>(ast, "arguments")));
            }
            case clingo_ast_type_theory_unparsed_term: {
                return prg_.theorytermopterm(get<Location>(ast, "location"),
                                             parseTheoryUnparsedTermElements(get<AST::ASTVec>(ast, "elements")));
            }
            default: {
                throw std::runtime_error("invalid ast: theory term expected");
            }
        }
    }


    TheoryOptermVecUid parseTheoryOptermVec(AST::ASTVec &asts) {
        auto uid = prg_.theoryopterms();
        for (auto &ast : asts) {
            uid = prg_.theoryopterms(uid, get<Location>(*ast, "location"), parseTheoryOpterm(*ast));
        }
        return uid;
    }

    TheoryOpVecUid parseTheoryOpVec(AST::StrVec &strs) {
        auto uid = prg_.theoryops();
        for (auto &str : strs) {
            uid = prg_.theoryops(uid, str);
        }
        return uid;
    }

    TheoryOptermUid parseTheoryOpterm(AST &ast) {
        if (ast.type() == clingo_ast_type_theory_unparsed_term) {
            return parseTheoryUnparsedTermElements(get<AST::ASTVec>(ast, "elements"));
        }
        return prg_.theoryopterm(prg_.theoryops(), parseTheoryTerm(ast));
    }

    TheoryOptermUid parseTheoryUnparsedTermElements(AST::ASTVec &asts) {
        require_(!asts.empty(), "invalid ast: unparsed term list must not be empty");
        auto it = asts.begin();
        auto ie = asts.end();
        auto uid = prg_.theoryopterm(parseTheoryOpVec(get<AST::StrVec>(**it, "operators")),
                                     parseTheoryTerm(*get<SAST>(**it, "term")));
        for (++it; it != ie; ++it) {
            auto &operators = get<AST::StrVec>(**it, "operators");
            require_(!operators.empty(), "invalid ast: at least one operator necessary on right-hand-side of unparsed theory term");
            uid = prg_.theoryopterm(uid,
                                    parseTheoryOpVec(operators),
                                    parseTheoryTerm(*get<SAST>(**it, "term")));
        }
        return uid;
    }

    // {{{2 literals

    TermUid parseAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_symbolic_atom, "invalid ast: symbolic atom expected");
        return parseTerm(*get<SAST>(ast, "term"));
    }

    static NAF parseSign(int sign) {
        switch (sign) {
            case clingo_ast_sign_none: {
                return NAF::POS;
            }
            case clingo_ast_sign_negation: {
                return NAF::NOT;
            }
            case clingo_ast_sign_double_negation: {
                return NAF::NOTNOT;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid sign");
            }
        }
    }

    static Relation parseRelation(int relation) {
        switch (relation) {
            case clingo_ast_comparison_operator_less_equal: {
                return Relation::LEQ;
            }
            case clingo_ast_comparison_operator_less_than: {
                return Relation::LT;
            }
            case clingo_ast_comparison_operator_greater_equal: {
                return Relation::GEQ;
            }
            case clingo_ast_comparison_operator_greater_than: {
                return Relation::GT;
            }
            case clingo_ast_comparison_operator_equal: {
                return Relation::EQ;
            }
            case clingo_ast_comparison_operator_not_equal: {
                return Relation::NEQ;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid sign");
            }
        }
    }

    LitUid parseLiteral(AST &ast) {
        require_(ast.type() == clingo_ast_type_literal, "invalid ast: literal expected");
        switch (ast.type()) {
            case clingo_ast_type_literal: {
                auto sign = parseSign(get<int>(ast, "sign"));
                auto &atom = *get<SAST>(ast, "atom");

                switch (atom.type()) {
                    case clingo_ast_type_boolean_constant: {
                        return prg_.boollit(get<Location>(ast, "location"), get<int>(ast, "value") != 0);
                    }
                    case clingo_ast_type_comparison: {
                        return prg_.predlit(get<Location>(ast, "location"),
                                            parseSign(get<int>(ast, "sign")),
                                            parseAtom(*get<SAST>(ast, "symbol")));
                    }
                    case clingo_ast_type_symbolic_atom: {
                        auto rel = parseRelation(get<int>(atom, "comparison"));
                        return prg_.rellit(get<Location>(ast, "location"),
                                           sign != NAF::NOT ? rel : neg(rel),
                                           parseTerm(*get<SAST>(atom, "left")),
                                           parseTerm(*get<SAST>(atom, "right")));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: atom expected");
                    }
                }
            }
            case clingo_ast_type_csp_literal: {
                auto &guards = get<AST::ASTVec>(ast, "guard");
                require_(!guards.empty(), "invalid ast: csp literals need at least one guard");
                auto it = guards.begin();
                auto ie = guards.end();
                auto uid = prg_.csplit(get<Location>(ast, "location"),
                                       parseCSPAddTerm(*get<SAST>(ast, "term")),
                                       parseRelation(get<int>(**it, "comparison")),
                                       parseCSPAddTerm(*get<SAST>(**it, "term")));
                for (++it; it != ie; ++it) {
                    uid = prg_.csplit(get<Location>(ast, "location"),
                                      uid,
                                      parseRelation(get<int>(**it, "comparison")),
                                      parseCSPAddTerm(*get<SAST>(**it, "term")));
                }
                return prg_.csplit(uid);
            }
            default: {
                throw std::runtime_error("invalid ast: (CSP) literal expected");
            }
        }
    }

    LitVecUid parseLiteralVec(AST::ASTVec &asts) {
        auto uid = prg_.litvec();
        for (auto &ast : asts) {
            uid = prg_.litvec(uid, parseLiteral(*ast));
        }
        return uid;
    }

    // {{{2 aggregates

    static AggregateFunction parseAggregateFunction(int fun) {
        switch (fun) {
            case clingo_ast_aggregate_function_count: {
                return AggregateFunction::COUNT;
            }
            case clingo_ast_aggregate_function_sum: {
                return AggregateFunction::SUM;
            }
            case clingo_ast_aggregate_function_sump: {
                return AggregateFunction::SUMP;
            }
            case clingo_ast_aggregate_function_min: {
                return AggregateFunction::MIN;
            }
            case clingo_ast_aggregate_function_max: {
                return AggregateFunction::MAX;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid aggregate function");
            }
        };
    }

    BoundVecUid parseBounds(AST &ast) {
        auto ret = prg_.boundvec();
        auto *right = getOpt<SAST>(ast, "right_guard");
        if (right != nullptr) {
            ret = prg_.boundvec(ret,
                                parseRelation(get<int>(**right, "comparison")),
                                parseTerm(*get<SAST>(**right, "term")));
        }
        auto *left = getOpt<SAST>(ast, "left_guard");
        if (left != nullptr) {
            ret = prg_.boundvec(ret,
                                inv(parseRelation(get<int>(**left, "comparison"))),
                                parseTerm(*get<SAST>(**left, "term")));
        }
        return ret;
    }

    CondLitVecUid parseCondLitVec(AST::ASTVec &asts) {
        auto uid = prg_.condlitvec();
        for (auto &ast : asts) {
            uid = prg_.condlitvec(uid,
                                  parseLiteral(*get<SAST>(*ast, "literal")),
                                  parseLiteralVec(get<AST::ASTVec>(*ast, "condition")));
        }
        return uid;
    }

    HdAggrElemVecUid parseHdAggrElemVec(AST::ASTVec &asts) {
        auto uid = prg_.headaggrelemvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_head_aggregate_element, "invalid ast: head aggregate element expected");
            auto &clit = get<SAST>(*ast, "condition");
            require_(clit->type() == clingo_ast_type_conditional_literal, "invalid ast: conditional literal expected");
            uid = prg_.headaggrelemvec(uid,
                                       parseTermVec(get<AST::ASTVec>(*ast, "tuple")),
                                       parseLiteral(*get<SAST>(*clit, "literal")),
                                       parseLiteralVec(get<AST::ASTVec>(*clit, "condition")));
        }
        return uid;
    }

    BdAggrElemVecUid parseBdAggrElemVec(AST::ASTVec &asts) {
        auto uid = prg_.bodyaggrelemvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_body_aggregate_element, "invalid ast: body aggregate element expected");
            uid = prg_.bodyaggrelemvec(uid,
                                       parseTermVec(get<AST::ASTVec>(*ast, "tuple")),
                                       parseLiteralVec(get<AST::ASTVec>(*ast, "condition")));
        }
        return uid;
    }

    TheoryElemVecUid parseTheoryElemVec(AST::ASTVec &asts) {
        auto uid = prg_.theoryelems();
        for (auto &ast : asts) {
            uid = prg_.theoryelems(uid,
                                   parseTheoryOptermVec(get<AST::ASTVec>(*ast, "tuple")),
                                   parseLiteralVec(get<AST::ASTVec>(*ast, "condition")));
        }
        return uid;
    }

    CSPElemVecUid parseCSPElemVec(AST::ASTVec &asts) {
        auto ret = prg_.cspelemvec();
        for (auto &ast  : asts) {
            require_(ast->type() == clingo_ast_type_body_aggregate_element, "invalid ast: body aggregate element expected");
            ret = prg_.cspelemvec(ret,
                                  get<Location>(*ast, "location"),
                                  parseTermVec(get<AST::ASTVec>(*ast, "tuple")),
                                  parseCSPAddTerm(*get<SAST>(*ast, "term")),
                                  parseLiteralVec(get<AST::ASTVec>(*ast, "condition")));
        }
        return ret;
    }

    TheoryAtomUid parseTheoryAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_atom, "invalid ast: theory atom expected");
        auto &loc = get<Location>(ast, "location");
        auto *guard = getOpt<SAST>(ast, "guard");
        auto term = parseTerm(*get<SAST>(ast, "term"));
        auto elements = parseTheoryElemVec(get<AST::ASTVec>(ast, "elements"));
        return guard != nullptr
            ? prg_.theoryatom(term,
                              elements,
                              get<String>(**guard, "operator_name"),
                              loc,
                              parseTheoryOpterm(*get<SAST>(**guard, "term")))
            : prg_.theoryatom(term, elements);

    }

    // {{{2 heads

    HdLitUid parseHeadLiteral(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_literal:
            case clingo_ast_type_csp_literal: {
                return prg_.headlit(parseLiteral(ast));
            }
            case clingo_ast_type_disjunction: {
                return prg_.disjunction(get<Location>(ast, "location"),
                                        parseCondLitVec(get<AST::ASTVec>(ast, "elements")));
            }
            case clingo_ast_type_aggregate: {
                return prg_.headaggr(get<Location>(ast, "location"),
                                     AggregateFunction::COUNT,
                                     parseBounds(ast),
                                     parseCondLitVec(get<AST::ASTVec>(ast, "elements")));
            }
            case clingo_ast_type_head_aggregate: {
                return prg_.headaggr(get<Location>(ast, "location"),
                                     parseAggregateFunction(get<int>(ast, "function")),
                                     parseBounds(ast),
                                     parseHdAggrElemVec(get<AST::ASTVec>(ast, "elements")));
            }
            case clingo_ast_type_theory_atom: {
                auto &loc = get<Location>(ast, "location");
                return prg_.headaggr(loc, parseTheoryAtom(ast));
            }
            default: {
                throw std::runtime_error("invalid ast: head literal expected");
            }
        }
    }

    // {{{2 bodies

    BdLitVecUid parseBodyLiteralVec(AST::ASTVec &asts) {
        auto uid = prg_.body();
        for (auto &lit : asts) {
            switch (lit->type()) {
                case clingo_ast_type_literal: {
                    auto &loc = get<Location>(*lit, "location");
                    auto sign = parseSign(get<int>(*lit, "sign"));
                    auto &atom = *get<SAST>(*lit, "atom");
                    switch (atom.type()) {
                        case clingo_ast_type_aggregate: {
                            uid = prg_.bodyaggr(uid, loc, sign, AggregateFunction::COUNT,
                                                parseBounds(atom),
                                                parseCondLitVec(get<AST::ASTVec>(atom, "elements")));
                            break;
                        }
                        case clingo_ast_type_body_aggregate: {
                            uid = prg_.bodyaggr(uid, loc, sign,
                                                parseAggregateFunction(get<int>(atom, "function")),
                                                parseBounds(atom),
                                                parseBdAggrElemVec(get<AST::ASTVec>(atom, "elements")));
                            break;
                        }
                        case clingo_ast_type_theory_atom: {
                            uid = prg_.bodyaggr(uid, loc, sign, parseTheoryAtom(atom));
                            break;
                        }
                        case clingo_ast_type_disjoint: {
                            uid = prg_.disjoint(uid, loc, sign,
                                                parseCSPElemVec(get<AST::ASTVec>(atom, "elements")));
                            break;
                        }
                        default: {
                            uid = prg_.bodylit(uid, parseLiteral(*lit));
                            break;
                        }
                    }
                }
                case clingo_ast_type_conditional_literal: {
                    uid = prg_.conjunction(uid,
                                           get<Location>(*lit, "location"),
                                           parseLiteral(*get<SAST>(*lit, "literal")),
                                           parseLiteralVec(get<AST::ASTVec>(*lit, "condition")));
                    break;
                }
                default: {
                    throw std::runtime_error("invalid ast: body literal expected");
                }
            }
        }
        return uid;
    }

    // {{{2 theory definitions

    static TheoryOperatorType parseTheoryOperatorType(int num) {
        switch (num) {
            case clingo_ast_theory_operator_type_unary: {
                return TheoryOperatorType::Unary;
            }
            case clingo_ast_theory_operator_type_binary_left: {
                return TheoryOperatorType::BinaryLeft;
            }
            case clingo_ast_theory_operator_type_binary_right: {
                return TheoryOperatorType::BinaryRight;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid theory operator type");
            }
        }
    }

    TheoryOpDefUid parseTheoryOpDef(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_operator_definition, "invalid ast: theory operator definition expected");
        return prg_.theoryopdef(get<Location>(ast, "location"),
                                get<String>(ast, "name"),
                                get<int>(ast, "priority"),
                                parseTheoryOperatorType(get<int>(ast, "operator_type")));
    }

    TheoryOpDefVecUid parseTheoryOpDefVec(AST::ASTVec &asts) {
        auto uid = prg_.theoryopdefs();
        for (auto &ast : asts) {
            prg_.theoryopdefs(uid, parseTheoryOpDef(*ast));
        }
        return uid;
    }

    static TheoryAtomType parseTheoryAtomType(int num) {
        switch (num) {
            case clingo_ast_theory_atom_definition_type_head: {
                return TheoryAtomType::Head;
            }
            case clingo_ast_theory_atom_definition_type_body: {
                return TheoryAtomType::Body;
            }
            case clingo_ast_theory_atom_definition_type_any: {
                return TheoryAtomType::Any;
            }
            case clingo_ast_theory_atom_definition_type_directive: {
                return TheoryAtomType::Directive;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid theory atom type");
            }
        };
    }

    TheoryAtomDefUid parseTheoryAtomDefinition(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_atom_definition, "invalid ast: theory atom definition expected");
        auto *guard = getOpt<SAST>(ast, "guard");
        auto &loc = get<Location>(ast, "location");
        auto name = get<String>(ast, "name");
        auto arity = get<int>(ast, "arity");
        auto elements = get<String>(ast, "elements");
        auto type = parseTheoryAtomType(get<int>(ast, "atom_type"));
        return guard != nullptr
            ? prg_.theoryatomdef(loc, name, arity, elements, type,
                                 parseTheoryOpVec(get<AST::StrVec>(**guard, "operators")),
                                 get<String>(**guard, "term"))
            : prg_.theoryatomdef(loc, name, arity, elements, type);
    }

    TheoryDefVecUid parseTheoryAtomDefinitionVec(TheoryDefVecUid uid, AST::ASTVec &asts) {
        for (auto &ast : asts) {
            prg_.theorydefs(uid, parseTheoryAtomDefinition(*ast));
        }
        return uid;
    }

    TheoryTermDefUid parseTheoryTermDefinition(AST &ast) {
        return prg_.theorytermdef(get<Location>(ast, "location"),
                                  get<String>(ast, "name"),
                                  parseTheoryOpDefVec(get<AST::ASTVec>(ast, "operators")),
                                  log_);
    }

    TheoryDefVecUid parseTheoryTermDefinitionVec(AST::ASTVec &asts) {
        auto uid = prg_.theorydefs();
        for (auto &ast : asts) {
            prg_.theorydefs(uid, parseTheoryTermDefinition(*ast));
        }
        return uid;
    }

    // }}}2

    Logger &log_;
    INongroundProgramBuilder &prg_;
};

} // namespace

void parseStatement(INongroundProgramBuilder &prg, Logger &log, AST &ast) {
    ASTParser{log, prg}.parseStatement(ast);
}

/////////////////////////// AST //////////////////////////////////

bool AST::hasValue(char const *name) const {
    return values_.find(name) != values_.end();
}

AST::Value const &AST::value(char const *name) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
        throw std::runtime_error("ast does not contain the given key");
    }
    return it->second;
}

AST::Value &AST::value(char const *name) {
    auto it = values_.find(name);
    if (it == values_.end()) {
        throw std::runtime_error("ast does not contain the given key");
    }
    return it->second;
}

clingo_ast_type AST::type() const {
    return type_;
}

} } // namespace Input Gringo
