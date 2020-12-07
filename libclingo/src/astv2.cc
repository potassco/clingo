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

struct ASTParser {
public:
    ASTParser(Logger &log, INongroundProgramBuilder &prg)
    : log_(log), prg_(prg) { }

    // {{{2 statement

    void parseStatement(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_rule: {
                return prg_.rule(ast.value("location").loc(),
                                 parseHeadLiteral(*ast.value("head").ast()),
                                 parseBodyLiteralVec(ast.value("body").asts()));
            }
            case clingo_ast_type_definition: {
                return prg_.define(ast.value("location").loc(),
                                   ast.value("name").str(), parseTerm(*ast.value("value").ast()),
                                   ast.value("is_default").num() != 0, log_);
            }
            case clingo_ast_type_show_signature: {
                return prg_.showsig(ast.value("location").loc(),
                                    Sig(ast.value("name").str(), ast.value("arity").num(), ast.value("sign").num() != 0),
                                    ast.hasValue("csp") && ast.value("csp").num() != 0);
            }
            case clingo_ast_type_show_term: {
                return prg_.show(ast.value("location").loc(),
                                 parseTerm(*ast.value("term").ast()),
                                 parseBodyLiteralVec(ast.value("body").asts()),
                                 ast.hasValue("csp") && ast.value("csp").num() != 0);
            }
            case clingo_ast_type_minimize: {
                return prg_.optimize(ast.value("location").loc(),
                                     parseTerm(*ast.value("weight").ast()),
                                     parseTerm(*ast.value("priority").ast()),
                                     parseTermVec(ast.value("tuple").asts()),
                                     parseBodyLiteralVec(ast.value("body").asts()));
            }
            case clingo_ast_type_script: {
                switch (ast.value("type").num()) {
                    case clingo_ast_script_type_python: { return prg_.python(ast.value("location").loc(), ast.value("code").str()); }
                    case clingo_ast_script_type_lua:    { return prg_.lua(ast.value("location").loc(), ast.value("code").str()); }
                }
                break;
            }
            case clingo_ast_type_program: {
                return prg_.block(ast.value("location").loc(),
                                  ast.value("name").str(),
                                  parseIdVec(ast.value("parameters").asts()));
            }
            case clingo_ast_type_external: {
                return prg_.external(ast.value("location").loc(),
                                     parseAtom(*ast.value("atom").ast()),
                                     parseBodyLiteralVec(ast.value("body").asts()),
                                     parseTerm(*ast.value("type").ast()));
            }
            case clingo_ast_type_edge: {
                return prg_.edge(ast.value("location").loc(),
                                 prg_.termvecvec(prg_.termvecvec(), prg_.termvec(prg_.termvec(prg_.termvec(), parseTerm(*ast.value("u").ast())), parseTerm(*ast.value("v").ast()))),
                                 parseBodyLiteralVec(ast.value("body").asts()));
            }
            case clingo_ast_type_heuristic: {
                return prg_.heuristic(ast.value("location").loc(),
                                      parseAtom(*ast.value("atom").ast()),
                                      parseBodyLiteralVec(ast.value("body").asts()),
                                      parseTerm(*ast.value("bias").ast()),
                                      parseTerm(*ast.value("priority").ast()),
                                      parseTerm(*ast.value("modifier").ast()));
            }
            case clingo_ast_type_project_atom: {
                return prg_.project(ast.value("location").loc(),
                                    parseAtom(*ast.value("atom").ast()),
                                    parseBodyLiteralVec(ast.value("body").asts()));
            }
            case clingo_ast_type_project_signature: {
                return prg_.project(ast.value("location").loc(),
                                    Sig(ast.value("name").str(), ast.value("arity").num(), ast.value("sign").num() != 0));
            }
            case clingo_ast_type_defined: {
                return prg_.defined(ast.value("location").loc(),
                                    Sig(ast.value("name").str(), ast.value("arity").num(), ast.value("sign").num() != 0));
            }
            case clingo_ast_type_theory_definition: {
                return prg_.theorydef(ast.value("location").loc(),
                                      ast.value("name").str(),
                                      parseTheoryAtomDefinitionVec(parseTheoryTermDefinitionVec(ast.value("terms").asts()), ast.value("atoms").asts()), log_);
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

    ASTValue *getOpt(AST &ast, char const *value) {
        if (!ast.hasValue(value)) {
            return nullptr;
        }
        auto &val = ast.value(value);
        if (val.empty()) {
            return nullptr;
        }
        return &val;
    }

    // {{{2 terms

    IdVecUid parseIdVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.idvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_id, "invalid ast: id required");
            prg_.idvec(uid, ast->value("location").loc(), ast->value("id").str());
        }
        return uid;
    }

    UnOp parseUnOp(int num) {
        switch (num) {
            case static_cast<int>(BinOp::XOR): {
                return UnOp::NEG;
            }
            case static_cast<int>(BinOp::OR): {
                return UnOp::NOT;
            }
            case static_cast<int>(BinOp::AND): {
                return UnOp::ABS;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid unary operator");
            }
        }
    }

    BinOp parseBinOp(int num) {
        switch (num) {
            case static_cast<int>(BinOp::XOR): {
                return BinOp::XOR;
            }
            case static_cast<int>(BinOp::OR): {
                return BinOp::OR;
            }
            case static_cast<int>(BinOp::AND): {
                return BinOp::AND;
            }
            case static_cast<int>(BinOp::ADD): {
                return BinOp::ADD;
            }
            case static_cast<int>(BinOp::SUB): {
                return BinOp::SUB;
            }
            case static_cast<int>(BinOp::MUL): {
                return BinOp::MUL;
            }
            case static_cast<int>(BinOp::DIV): {
                return BinOp::DIV;
            }
            case static_cast<int>(BinOp::MOD): {
                return BinOp::MOD;
            }
            case static_cast<int>(BinOp::POW): {
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
                return prg_.term(ast.value("location").loc(),
                                 ast.value("name").str());
            }
            case clingo_ast_type_symbol: {
                return prg_.term(ast.value("location").loc(),
                                 ast.value("symbol").sym());
            }
            case clingo_ast_type_unary_operation: {
                return prg_.term(ast.value("location").loc(),
                                 parseUnOp(ast.value("unary_operator").num()),
                                 parseTerm(*ast.value("argument").ast()));
            }
            case clingo_ast_type_binary_operation: {
                return prg_.term(ast.value("location").loc(),
                                 parseBinOp(ast.value("binary_operator").num()),
                                 parseTerm(*ast.value("left").ast()),
                                 parseTerm(*ast.value("right").ast()));
            }
            case clingo_ast_type_interval: {
                return prg_.term(ast.value("location").loc(),
                                 parseTerm(*ast.value("left").ast()),
                                 parseTerm(*ast.value("right").ast()));
            }
            case clingo_ast_type_function: {
                auto external = ast.hasValue("external") && ast.value("external").num() != 0;
                auto name = ast.value("name").str();
                require_(!name.empty() || !external, "invalid ast: external functions must have a name");
                return !name.empty()
                    ? prg_.term(ast.value("location").loc(),
                                name,
                                prg_.termvecvec(prg_.termvecvec(), parseTermVec(ast.value("arguments").asts())),
                                external)
                    : prg_.term(ast.value("location").loc(),
                                parseTermVec(ast.value("arguments").asts()),
                                true);
            }
            case clingo_ast_type_pool: {
                return prg_.pool(ast.value("location").loc(),
                                 parseTermVec(ast.value("arguments").asts()));
            }
            default: {
                throw std::runtime_error("invalid ast: term expected");
            }
        }
    }

    TermVecUid parseTermVec(ASTValue::ASTVec &asts) {
        TermVecUid uid = prg_.termvec();
        for (auto &ast : asts) {
            prg_.termvec(uid, parseTerm(*ast));
        }
        return uid;
    }

    CSPMulTermUid parseCSPMulTerm(AST &ast) {
        require_(ast.type() == clingo_ast_type_csp_product, "invalid ast: csp product required");
        auto &variable = ast.value("variable");
        return !variable.empty()
            ? prg_.cspmulterm(ast.value("location").loc(),
                              parseTerm(*ast.value("coefficient").ast()),
                              parseTerm(*variable.ast()))
            : prg_.cspmulterm(ast.value("location").loc(),
                              parseTerm(*ast.value("coefficient").ast()));
    }

    CSPAddTermUid parseCSPAddTerm(AST &ast) {
        require_(ast.type() == clingo_ast_type_csp_sum, "invalid ast: csp sum required");
        auto &terms = ast.value("terms").asts();
        require_(!terms.empty(), "invalid ast: csp sums terms must not be empty");
        auto it = terms.begin();
        auto ie = terms.end();
        auto uid = prg_.cspaddterm((*it)->value("location").loc(),
                                   parseCSPMulTerm(**it));
        for (++it; it != ie; ++it) {
            uid = prg_.cspaddterm((*it)->value("location").loc(),
                                  uid,
                                  parseCSPMulTerm(**it), true);

        }
        return uid;
    }

    TheoryTermUid parseTheoryTerm(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_symbol : {
                return prg_.theorytermvalue(ast.value("location").loc(), ast.value("symbol").sym());
            }
            case clingo_ast_type_variable: {
                return prg_.theorytermvar(ast.value("location").loc(), ast.value("variable").str());
            }
            case clingo_ast_type_theory_sequence: {
                switch (ast.value("sequence_type").num()) {
                    case clingo_ast_theory_sequence_type_tuple: {
                        return prg_.theorytermtuple(ast.value("location").loc(), parseTheoryOptermVec(ast.value("terms").asts()));
                    }
                    case clingo_ast_theory_sequence_type_list: {
                        return prg_.theoryoptermlist(ast.value("location").loc(), parseTheoryOptermVec(ast.value("terms").asts()));
                    }
                    case clingo_ast_theory_sequence_type_set: {
                        return prg_.theorytermset(ast.value("location").loc(), parseTheoryOptermVec(ast.value("terms").asts()));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: invalid theory sequence type");

                    }
                }
            }
            case clingo_ast_type_theory_function: {
                return prg_.theorytermfun(ast.value("location").loc(),
                                          ast.value("name").str(),
                                          parseTheoryOptermVec(ast.value("arguments").asts()));
            }
            case clingo_ast_type_theory_unparsed_term: {
                return prg_.theorytermopterm(ast.value("location").loc(),
                                             parseTheoryUnparsedTermElements(ast.value("elements").asts()));
            }
            default: {
                throw std::runtime_error("invalid ast: theory term expected");
            }
        }
    }


    TheoryOptermVecUid parseTheoryOptermVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.theoryopterms();
        for (auto &ast : asts) {
            uid = prg_.theoryopterms(uid, ast->value("location").loc(), parseTheoryOpterm(*ast));
        }
        return uid;
    }

    TheoryOpVecUid parseTheoryOpVec(ASTValue::StrVec &strs) {
        auto uid = prg_.theoryops();
        for (auto &str : strs) {
            uid = prg_.theoryops(uid, str);
        }
        return uid;
    }

    TheoryOptermUid parseTheoryOpterm(AST &ast) {
        if (ast.type() == clingo_ast_type_theory_unparsed_term) {
            return parseTheoryUnparsedTermElements(ast.value("elements").asts());
        }
        return prg_.theoryopterm(prg_.theoryops(), parseTheoryTerm(ast));
    }

    TheoryOptermUid parseTheoryUnparsedTermElements(ASTValue::ASTVec &asts) {
        require_(!asts.empty(), "invalid ast: unparsed term list must not be empty");
        auto it = asts.begin();
        auto ie = asts.end();
        auto uid = prg_.theoryopterm(parseTheoryOpVec((*it)->value("operators").strs()),
                                     parseTheoryTerm(*(*it)->value("term").ast()));
        for (++it; it != ie; ++it) {
            auto &operators = (*it)->value("operators").strs();
            require_(!operators.empty(), "invalid ast: at least one operator necessary on right-hand-side of unparsed theory term");
            uid = prg_.theoryopterm(uid,
                                    parseTheoryOpVec(operators),
                                    parseTheoryTerm(*(*it)->value("term").ast()));
        }
        return uid;
    }

    // {{{2 literals

    TermUid parseAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_symbolic_atom, "invalid ast: symbolic atom expected");
        return parseTerm(*ast.value("term").ast());
    }

    NAF parseSign(int sign) {
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

    Relation parseRelation(int relation) {
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
                auto sign = parseSign(ast.value("sign").num());
                auto &atom = *ast.value("atom").ast();

                switch (atom.type()) {
                    case clingo_ast_type_boolean_constant: {
                        return prg_.boollit(ast.value("location").loc(), ast.value("value").num() != 0);
                    }
                    case clingo_ast_type_comparison: {
                        return prg_.predlit(ast.value("location").loc(),
                                            parseSign(ast.value("sign").num()),
                                            parseAtom(*ast.value("symbol").ast()));
                    }
                    case clingo_ast_type_symbolic_atom: {
                        auto rel = parseRelation(atom.value("comparison").num());
                        return prg_.rellit(ast.value("location").loc(),
                                           sign != NAF::NOT ? rel : neg(rel),
                                           parseTerm(*atom.value("left").ast()),
                                           parseTerm(*atom.value("right").ast()));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: atom expected");
                    }
                }
            }
            case clingo_ast_type_csp_literal: {
                auto &guards = ast.value("guard").asts();
                require_(!guards.empty(), "invalid ast: csp literals need at least one guard");
                auto it = guards.begin();
                auto ie = guards.end();
                auto uid = prg_.csplit(ast.value("location").loc(),
                                       parseCSPAddTerm(*ast.value("term").ast()),
                                       parseRelation((*it)->value("comparison").num()),
                                       parseCSPAddTerm(*(*it)->value("term").ast()));
                for (++it; it != ie; ++it) {
                    uid = prg_.csplit(ast.value("location").loc(),
                                      uid,
                                      parseRelation((*it)->value("comparison").num()),
                                      parseCSPAddTerm(*(*it)->value("term").ast()));
                }
                return prg_.csplit(uid);
            }
            default: {
                throw std::runtime_error("invalid ast: (CSP) literal expected");
            }
        }
    }

    LitVecUid parseLiteralVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.litvec();
        for (auto &ast : asts) {
            uid = prg_.litvec(uid, parseLiteral(*ast));
        }
        return uid;
    }

    // {{{2 aggregates

    AggregateFunction parseAggregateFunction(int fun) {
        switch (fun) {
            case static_cast<int>(AggregateFunction::COUNT): {
                return AggregateFunction::COUNT;
            }
            case static_cast<int>(AggregateFunction::SUM): {
                return AggregateFunction::SUM;
            }
            case static_cast<int>(AggregateFunction::SUMP): {
                return AggregateFunction::SUMP;
            }
            case static_cast<int>(AggregateFunction::MIN): {
                return AggregateFunction::MIN;
            }
            case static_cast<int>(AggregateFunction::MAX): {
                return AggregateFunction::MAX;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid aggregate function");
            }
        };
    }

    BoundVecUid parseBounds(AST &ast) {
        auto ret = prg_.boundvec();
        auto *right = getOpt(ast, "right_guard");
        if (right != nullptr) {
            ret = prg_.boundvec(ret,
                                parseRelation(right->ast()->value("comparison").num()),
                                parseTerm(*right->ast()->value("term").ast()));
        }
        auto *left = getOpt(ast, "left_guard");
        if (left != nullptr) {
            ret = prg_.boundvec(ret,
                                inv(parseRelation(left->ast()->value("comparison").num())),
                                parseTerm(*left->ast()->value("term").ast()));
        }
        return ret;
    }

    CondLitVecUid parseCondLitVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.condlitvec();
        for (auto &ast : asts) {
            uid = prg_.condlitvec(uid,
                                  parseLiteral(*ast->value("literal").ast()),
                                  parseLiteralVec(ast->value("condition").asts()));
        }
        return uid;
    }

    HdAggrElemVecUid parseHdAggrElemVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.headaggrelemvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_head_aggregate_element, "invalid ast: head aggregate element expected");
            auto &clit = ast->value("condition").ast();
            require_(clit->type() == clingo_ast_type_conditional_literal, "invalid ast: conditional literal expected");
            uid = prg_.headaggrelemvec(uid,
                                       parseTermVec(ast->value("tuple").asts()),
                                       parseLiteral(*clit->value("literal").ast()),
                                       parseLiteralVec(clit->value("condition").asts()));
        }
        return uid;
    }

    BdAggrElemVecUid parseBdAggrElemVec(ASTValue::ASTVec &asts) {
        throw std::runtime_error("implement me!!!");
        /*
        uid ret = prg_.bodyaggrelemvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            uid = prg_.bodyaggrelemvec(uid, parseTermVec(it->tuple, it->tuple_size), parseLiteralVec(it->condition, it->condition_size));
        }
        return uid;
        */
    }

    TheoryElemVecUid parseTheoryElemVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.theoryelems();
        for (auto &ast : asts) {
            uid = prg_.theoryelems(uid,
                                   parseTheoryOptermVec(ast->value("tuple").asts()),
                                   parseLiteralVec(ast->value("condition").asts()));
        }
        return uid;
    }

    CSPElemVecUid parseCSPElemVec(ASTValue::ASTVec &asts) {
        throw std::runtime_error("implement me!!!");
        /*
        auto ret = prg_.cspelemvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.cspelemvec(ret, parseLocation(it->location), parseTermVec(it->tuple, it->tuple_size), parseCSPAddTerm(it->term), parseLiteralVec(it->condition, it->condition_size));
        }
        return ret;
        */
    }

    TheoryAtomUid parseTheoryAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_atom, "invalid ast: theory atom expected");
        Location &loc = ast.value("location").loc();
        ASTValue *guard = getOpt(ast, "guard");
        auto term = parseTerm(*ast.value("term").ast());
        auto elements = parseTheoryElemVec(ast.value("elements").asts());
        return guard != nullptr
            ? prg_.theoryatom(term,
                              elements,
                              guard->ast()->value("operator_name").str(),
                              loc,
                              parseTheoryOpterm(*guard->ast()->value("term").ast()))
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
                return prg_.disjunction(ast.value("location").loc(),
                                        parseCondLitVec(ast.value("elements").asts()));
            }
            case clingo_ast_type_aggregate: {
                return prg_.headaggr(ast.value("location").loc(),
                                     AggregateFunction::COUNT,
                                     parseBounds(ast),
                                     parseCondLitVec(ast.value("elements").asts()));
            }
            case clingo_ast_type_head_aggregate: {
                return prg_.headaggr(ast.value("location").loc(),
                                     parseAggregateFunction(ast.value("function").num()),
                                     parseBounds(ast),
                                     parseHdAggrElemVec(ast.value("elements").asts()));
            }
            case clingo_ast_type_theory_atom: {
                Location &loc = ast.value("location").loc();
                return prg_.headaggr(loc, parseTheoryAtom(ast));
            }
            default: {
                throw std::runtime_error("invalid ast: head literal expected");
            }
        }
    }

    // {{{2 bodies

    // TODO
    BdLitVecUid parseBodyLiteralVec(ASTValue::ASTVec &asts) {
        throw std::runtime_error("implement me!!!");
        /*
        auto ret = prg_.body();
        for (auto it = lit, ie = lit + size; it != ie; ++it) {
            switch (static_cast<enum clingo_ast_body_literal_type>(it->type)) {
                case clingo_ast_body_literal_type_literal: {
                    ret = prg_.bodylit(ret, parseLiteral(*it->literal, static_cast<enum clingo_ast_sign>(it->sign)));
                    break;
                }
                case clingo_ast_body_literal_type_conditional: {
                    require_(static_cast<NAF>(it->sign) == NAF::POS, "conditional literals must hot have a sign");
                    auto &y = *it->conditional;
                    ret = prg_.conjunction(ret, parseLocation(it->location), parseLiteral(y.literal), parseLiteralVec(y.condition, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_aggregate: {
                    auto &y = *it->aggregate;
                    ret = prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), AggregateFunction::COUNT, parseBounds(y.left_guard, y.right_guard), parseCondLitVec(y.elements, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_body_aggregate: {
                    auto &y = *it->body_aggregate;
                    ret = prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), static_cast<AggregateFunction>(y.function), parseBounds(y.left_guard, y.right_guard), parseBdAggrElemVec(y.elements, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_theory_atom: {
                    auto &y = *it->theory_atom;
                    ret = y.guard
                        ? prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size), y.guard->operator_name, parseLocation(it->location), parseTheoryOpterm(y.guard->term)))
                        : prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size)));
                    break;
                }
                case clingo_ast_body_literal_type_disjoint: {
                    auto &y = *it->disjoint;
                    ret = prg_.disjoint(ret, parseLocation(it->location), static_cast<NAF>(it->sign), parseCSPElemVec(y.elements, y.size));
                    break;
                }
            }
        }
        return ret;
        */
    }

    // {{{2 theory definitions

    static TheoryOperatorType parseTheoryOperatorType(int num) {
        switch (num) {
            case static_cast<int>(TheoryOperatorType::Unary): {
                return TheoryOperatorType::Unary;
            }
            case static_cast<int>(TheoryOperatorType::BinaryLeft): {
                return TheoryOperatorType::BinaryLeft;
            }
            case static_cast<int>(TheoryOperatorType::BinaryRight): {
                return TheoryOperatorType::BinaryRight;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid theory operator type");
            }
        }
    }

    TheoryOpDefUid parseTheoryOpDef(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_operator_definition, "invalid ast: theory operator definition expected");
        return prg_.theoryopdef(ast.value("location").loc(),
                                ast.value("name").str(),
                                ast.value("priority").num(),
                                parseTheoryOperatorType(ast.value("operator_type").num()));
    }

    TheoryOpDefVecUid parseTheoryOpDefVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.theoryopdefs();
        for (auto &ast : asts) {
            prg_.theoryopdefs(uid, parseTheoryOpDef(*ast));
        }
        return uid;
    }

    TheoryAtomType parseTheoryAtomType(int num) {
        switch (num) {
            case static_cast<int>(TheoryAtomType::Head): {
                return TheoryAtomType::Head;
            }
            case static_cast<int>(TheoryAtomType::Body): {
                return TheoryAtomType::Body;
            }
            case static_cast<int>(TheoryAtomType::Any): {
                return TheoryAtomType::Any;
            }
            case static_cast<int>(TheoryAtomType::Directive): {
                return TheoryAtomType::Directive;
            }
            default: {
                throw std::runtime_error("invalid ast: invalid theory atom type");
            }
        };
    }

    TheoryAtomDefUid parseTheoryAtomDefinition(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_atom_definition, "invalid ast: theory atom definition expected");
        ASTValue *guard = getOpt(ast, "guard");
        auto &loc = ast.value("location").loc();
        auto name = ast.value("name").str();
        auto arity = ast.value("arity").num();
        auto elements = ast.value("elements").str();
        auto type = parseTheoryAtomType(ast.value("atom_type").num());
        return guard != nullptr
            ? prg_.theoryatomdef(loc, name, arity, elements, type,
                                 parseTheoryOpVec(guard->ast()->value("operators").strs()),
                                 guard->ast()->value("term").str())
            : prg_.theoryatomdef(loc, name, arity, elements, type);
    }

    TheoryDefVecUid parseTheoryAtomDefinitionVec(TheoryDefVecUid uid, ASTValue::ASTVec &asts) {
        for (auto &ast : asts) {
            prg_.theorydefs(uid, parseTheoryAtomDefinition(*ast));
        }
        return uid;
    }

    TheoryTermDefUid parseTheoryTermDefinition(AST &ast) {
        return prg_.theorytermdef(ast.value("location").loc(),
                                  ast.value("name").str(),
                                  parseTheoryOpDefVec(ast.value("operators").asts()),
                                  log_);
    }

    TheoryDefVecUid parseTheoryTermDefinitionVec(ASTValue::ASTVec &asts) {
        auto uid = prg_.theorydefs();
        for (auto &ast : asts) {
            prg_.theorydefs(uid, parseTheoryTermDefinition(*ast));
        }
        return uid;
    }

    // }}}2
private:
    Logger &log_;
    INongroundProgramBuilder &prg_;
};

}

ASTValue::ASTValue(int num)
: type_{ASTValueType::Num}
, num_{num} { }

ASTValue::ASTValue(Symbol sym)
: type_{ASTValueType::Sym}
, sym_{sym} { }

ASTValue::ASTValue(Location loc)
: type_{ASTValueType::Loc}
, loc_{loc} { }

ASTValue::ASTValue(String str)
: type_{ASTValueType::Str}
, str_{str} { }

ASTValue::ASTValue(StrVec strs)
: type_{ASTValueType::StrVec} {
    new (&strs_) StrVec{};
    strs_ = std::move(strs);
}
ASTValue::ASTValue(ASTVec asts)
: type_{ASTValueType::ASTVec} {
    new (&asts_) ASTVec{};
    asts_ = std::move(asts);
}

ASTValue::ASTValue(ASTValue const &val)
: type_{val.type_} {
    switch (val.type_) {
        case ASTValueType::Empty: {
            break;
        }
        case ASTValueType::Num: {
            num_ = val.num_;
            break;
        }
        case ASTValueType::Loc: {
            loc_ = val.loc_;
            break;
        }
        case ASTValueType::Str: {
            str_ = val.str_;
            break;
        }
        case ASTValueType::Sym: {
            sym_ = val.sym_;
            break;
        }
        case ASTValueType::AST: {
            new (&ast_) SAST{val.ast_};
            break;
        }
        case ASTValueType::StrVec: {
            new (&strs_) StrVec{};
            strs_ = val.strs_;
            break;
        }
        case ASTValueType::ASTVec: {
            new (&asts_) ASTVec{};
            asts_ = val.asts_;
            break;
        }
    }
}

ASTValue::ASTValue(ASTValue &&val) noexcept
: type_{val.type_} {
    switch (val.type_) {
        case ASTValueType::Empty: {
            break;
        }
        case ASTValueType::Num: {
            num_ = val.num_;
            break;
        }
        case ASTValueType::Loc: {
            loc_ = val.loc_;
            break;
        }
        case ASTValueType::Str: {
            str_ = val.str_;
            break;
        }
        case ASTValueType::Sym: {
            sym_ = val.sym_;
            break;
        }
        case ASTValueType::AST: {
            new (&ast_) SAST{std::move(val.ast_)};
            break;
        }
        case ASTValueType::StrVec: {
            new (&strs_) StrVec{};
            strs_ = std::move(val.strs_);
        }
        case ASTValueType::ASTVec: {
            new (&asts_) ASTVec{};
            asts_ = std::move(val.asts_);
            break;
        }
    }
}

ASTValue &ASTValue::operator=(ASTValue const &val) {
    clear_();
    new (this) ASTValue(val);
    return *this;
}

ASTValue &ASTValue::operator=(ASTValue &&val) noexcept {
    clear_();
    new (this) ASTValue(std::move(val));
    return *this;
}

ASTValue::~ASTValue() {
    clear_();
}

void ASTValue::clear_() {
    switch (type_) {
        case ASTValueType::AST: {
            ast_.~SAST();
            type_ = ASTValueType::Num;
            break;
        }
        case ASTValueType::StrVec: {
            strs_.~StrVec();
            type_ = ASTValueType::Num;
            break;
        }
        case ASTValueType::ASTVec: {
            asts_.~ASTVec();
            type_ = ASTValueType::Num;
            break;
        }
        default: {
            break;
        }
    }
}

void ASTValue::require_(ASTValueType type) const {
    if (type_ != type) {
        throw std::runtime_error("invalid attribute");
    }
}

ASTValueType ASTValue::type() const {
    return type_;
}

bool ASTValue::empty() const {
    return type_ == ASTValueType::Empty;
}

int &ASTValue::num() {
    require_(ASTValueType::Num);
    return num_;
}

int ASTValue::num() const {
    require_(ASTValueType::Num);
    return num_;
}

String &ASTValue::str() {
    require_(ASTValueType::Str);
    return str_;
}

String ASTValue::str() const {
    require_(ASTValueType::Str);
    return str_;
}

Location &ASTValue::loc() {
    require_(ASTValueType::Sym);
    return loc_;
}

Location const &ASTValue::loc() const {
    require_(ASTValueType::Sym);
    return loc_;
}

Symbol &ASTValue::sym() {
    require_(ASTValueType::Sym);
    return sym_;
}

Symbol ASTValue::sym() const {
    require_(ASTValueType::Sym);
    return sym_;
}

SAST &ASTValue::ast() {
    require_(ASTValueType::AST);
    return ast_;
}

SAST const &ASTValue::ast() const {
    require_(ASTValueType::AST);
    return ast_;
}

ASTValue::ASTVec &ASTValue::asts() {
    require_(ASTValueType::ASTVec);
    return asts_;
}

ASTValue::ASTVec const &ASTValue::asts() const {
    require_(ASTValueType::ASTVec);
    return asts_;
}

ASTValue::StrVec &ASTValue::strs() {
    require_(ASTValueType::StrVec);
    return strs_;
}

ASTValue::StrVec const &ASTValue::strs() const {
    require_(ASTValueType::StrVec);
    return strs_;
}

} } // namespace Input Gringo
