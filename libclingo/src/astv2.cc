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

// TODO:
// - C binding
// - !!!TEST!!!
// - string representation
// - unpooling

namespace Gringo { namespace Input {

namespace {

template <class T>
T &get(AST &ast, clingo_ast_attribute name) {
    return mpark::get<T>(ast.value(name));
}

template <class T>
T *getOpt(AST &ast, clingo_ast_attribute name) {
    if (!ast.hasValue(name)) {
        return nullptr;
    }
    auto &value = ast.value(name);
    if (value.index() == 0) {
        return nullptr;
    }
    return &mpark::get<T>(value);
}

struct Deepcopy {
    template <typename T>
    AST::Value operator()(T const &val) {
        return val;
    }
    AST::Value operator()(SAST &ast) {
        return ast->deepcopy();
    }
    AST::Value operator()(AST::ASTVec &asts) {
        AST::ASTVec ret;
        ret.reserve(asts.size());
        for (auto &ast : asts) {
            ret.emplace_back(ast->deepcopy());
        }
        return AST::Value{std::move(ret)};
    }
};

// {{{1 ast building

class ast {
public:
    ast(clingo_ast_type type, Location const &loc)
    : ast_{std::make_shared<AST>(type)} {
        set(clingo_ast_attribute_location, loc);
    }
    ast(clingo_ast_type type)
    : ast_{std::make_shared<AST>(type)} { }
    template <typename T>
    ast &set(clingo_ast_attribute name, T &&value) {
        ast_->value(name, std::forward<T>(value));
        return *this;
    }
    SAST move() {
        return std::move(ast_);
    }
    operator SAST() {
        return std::move(ast_);
    }
private:
    SAST ast_;
};

class ASTBuilder : public Gringo::Input::INongroundProgramBuilder {
public:
    ASTBuilder(SASTCallback cb)
    : cb_{std::move(cb)} { }
    // {{{2 terms

    TermUid term(Location const &loc, Symbol val) override {
        return terms_.insert(ast(clingo_ast_type_symbol, loc)
            .set(clingo_ast_attribute_symbol, val));
    }

    TermUid term(Location const &loc, String name) override {
        return terms_.insert(ast(clingo_ast_type_variable, loc)
            .set(clingo_ast_attribute_variable, name));
    }

    TermUid term(Location const &loc, UnOp op, TermUid a) override {
        return terms_.insert(ast(clingo_ast_type_unary_operation, loc)
            .set(clingo_ast_attribute_operator, static_cast<int>(op))
            .set(clingo_ast_attribute_argument, terms_.erase(a)));
    }

    TermUid pool_(Location const &loc, AST::ASTVec vec) {
        if (vec.size() == 1) {
            return terms_.insert(std::move(vec.front()));
        }
        return terms_.insert(ast(clingo_ast_type_pool, loc)
            .set(clingo_ast_attribute_arguments, std::move(vec)));
    }

    TermUid term(Location const &loc, UnOp op, TermVecUid a) override {
        return term(loc, op, pool_(loc, termvecs_.erase(a)));
    }

    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override {
        return terms_.insert(ast(clingo_ast_type_binary_operation, loc)
            .set(clingo_ast_attribute_operator, static_cast<int>(op))
            .set(clingo_ast_attribute_left, terms_.erase(a))
            .set(clingo_ast_attribute_right, terms_.erase(b)));
    }

    TermUid term(Location const &loc, TermUid a, TermUid b) override {
        return terms_.insert(ast(clingo_ast_type_interval, loc)
            .set(clingo_ast_attribute_left, terms_.erase(a))
            .set(clingo_ast_attribute_right, terms_.erase(b)));
    }

    static SAST fun_(Location const &loc, String name, AST::ASTVec args, bool external) {
        return ast(clingo_ast_type_function, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_arguments, std::move(args))
            .set(clingo_ast_attribute_external, static_cast<int>(external));
    }

    TermUid term(Location const &loc, String name, TermVecVecUid a, bool lua) override {
        AST::ASTVec pool;
        for (auto &args : termvecvecs_.erase(a)) {
            pool.emplace_back(fun_(loc, name, std::move(args), lua));
        }
        return pool_(loc, std::move(pool));
    }

    TermUid term(Location const &loc, TermVecUid a, bool forceTuple) override {
        return terms_.insert((termvecs_[a].size() == 1 && !forceTuple)
            ? std::move(termvecs_.erase(a).front())
            : fun_(loc, "", termvecs_.erase(a), false));
    }

    TermUid pool(Location const &loc, TermVecUid a) override {
        return pool_(loc, termvecs_.erase(a));
    }

    IdVecUid idvec() override {
        return idvecs_.emplace();
    }

    IdVecUid idvec(IdVecUid uid, Location const &loc, String id) override {
        idvecs_[uid].emplace_back(ast(clingo_ast_type_id, loc)
            .set(clingo_ast_attribute_id, id));
        return uid;
    }

    TermVecUid termvec() override {
        return termvecs_.emplace();
    }

    TermVecUid termvec(TermVecUid uid, TermUid termUid) override {
        termvecs_[uid].emplace_back(terms_.erase(termUid));
        return uid;
    }

    TermVecVecUid termvecvec() override {
        return termvecvecs_.emplace();
    }

    TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) override {
        termvecvecs_[uid].emplace_back(termvecs_.erase(termvecUid));
        return uid;
    }


   // {{{2 csp

    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) override {
        return cspmulterms_.insert(ast(clingo_ast_type_csp_product, loc)
            .set(clingo_ast_attribute_coefficient, terms_.erase(coe))
            .set(clingo_ast_attribute_var, terms_.erase(var)));
    }

    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override {
        return cspmulterms_.insert(ast(clingo_ast_type_csp_product, loc)
            .set(clingo_ast_attribute_coefficient, terms_.erase(coe))
            .set(clingo_ast_attribute_var, mpark::monostate()));
    }

    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override {
        if (!add) {
            auto &pos = get<SAST>(*cspmulterms_[b], clingo_ast_attribute_coefficient);
            pos = ast(clingo_ast_type_unary_operation, loc)
                .set(clingo_ast_attribute_operator, static_cast<int>(clingo_ast_unary_operator_minus))
                .set(clingo_ast_attribute_argument, std::move(pos));
        }
        auto &addterm = cspaddterms_[a];
        get<Location>(*addterm, clingo_ast_attribute_location) = loc;
        get<AST::ASTVec>(*addterm, clingo_ast_attribute_terms).emplace_back(cspmulterms_.erase(b));
        return a;
    }

    CSPAddTermUid cspaddterm(Location const &loc, CSPMulTermUid b) override {
        return cspaddterms_.emplace(ast(clingo_ast_type_csp_sum, loc)
            .set(clingo_ast_attribute_terms, AST::ASTVec{cspmulterms_.erase(b)}));
    }

    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override {
        return csplits_.insert(ast(clingo_ast_type_csp_literal, loc)
            .set(clingo_ast_attribute_term, cspaddterms_.erase(a))
            .set(clingo_ast_attribute_guards, AST::ASTVec{ast(clingo_ast_type_csp_guard)
                .set(clingo_ast_attribute_comparison, static_cast<int>(rel))
                .set(clingo_ast_attribute_term, cspaddterms_.erase(b))}));
    }

    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override {
        auto &lit = csplits_[a];
        get<AST::ASTVec>(*lit, clingo_ast_attribute_terms).emplace_back(cspaddterms_.erase(b));
        return a;
    }

    // {{{2 literals

    SAST symbolicatom(TermUid termUid) {
        auto &loc = get<Location>(*terms_[termUid], clingo_ast_attribute_location);
        return ast(clingo_ast_type_symbolic_atom, loc)
                .set(clingo_ast_attribute_symbol, terms_.erase(termUid));
    }

    LitUid boollit(Location const &loc, bool type) override {
        return lits_.insert(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(clingo_ast_sign_none))
            .set(clingo_ast_attribute_atom, ast(clingo_ast_type_boolean_constant, loc)
                .set(clingo_ast_attribute_value, static_cast<int>(type))));
    }

    LitUid predlit(Location const &loc, NAF naf, TermUid termUid) override {
        return lits_.insert(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(naf))
            .set(clingo_ast_attribute_atom, symbolicatom(termUid)));
    }

    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) override {
        return lits_.insert(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(clingo_ast_sign_none))
            .set(clingo_ast_attribute_atom, ast(clingo_ast_type_comparison, loc)
                .set(clingo_ast_attribute_comparison, static_cast<int>(rel))
                .set(clingo_ast_attribute_left, terms_.erase(termUidLeft))
                .set(clingo_ast_attribute_right, terms_.erase(termUidRight))));
    }

    LitUid csplit(CSPLitUid a) override {
        auto &loc = get<Location>(*csplits_[a], clingo_ast_attribute_location);
        return lits_.insert(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(clingo_ast_sign_none))
            .set(clingo_ast_attribute_atom, csplits_.erase(a)));
    }

    LitVecUid litvec() override {
        return litvecs_.emplace();
    }

    LitVecUid litvec(LitVecUid uid, LitUid literalUid) override {
        litvecs_[uid].emplace_back(lits_.erase(literalUid));
        return uid;
    }

    // {{{2 aggregates

    CondLitVecUid condlitvec() override {
        return condlitvecs_.emplace();
    }

    SAST condlit(LitUid litUid, LitVecUid litvecUid, Location const *oloc=nullptr) {
        auto const &loc = oloc != nullptr
            ? *oloc
            : get<Location>(*lits_[litUid], clingo_ast_attribute_location);
        return ast(clingo_ast_type_conditional_literal, loc)
            .set(clingo_ast_attribute_literal, lits_.erase(litUid))
            .set(clingo_ast_attribute_condition, litvecs_.erase(litvecUid));
    }

    CondLitVecUid condlitvec(CondLitVecUid uid, LitUid litUid, LitVecUid litvecUid) override {
        condlitvecs_[uid].emplace_back(condlit(litUid, litvecUid));
        return uid;
    }

    BdAggrElemVecUid bodyaggrelemvec() override {
        return bdaggrelemvecs_.emplace();
    }

    BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) override {
        bdaggrelemvecs_[uid].emplace_back(ast(clingo_ast_type_body_aggregate_element)
            .set(clingo_ast_attribute_tuple, termvecs_.erase(termvec))
            .set(clingo_ast_attribute_condition, litvecs_.erase(litvec)));
        return uid;
    }

    HdAggrElemVecUid headaggrelemvec() override {
        return hdaggrelemvecs_.emplace();
    }

    HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid litUid, LitVecUid litvec) override {
        auto &loc = get<Location>(*lits_[litUid], clingo_ast_attribute_location);
        hdaggrelemvecs_[uid].emplace_back(ast(clingo_ast_type_body_aggregate_element)
            .set(clingo_ast_attribute_tuple, termvecs_.erase(termvec))
            .set(clingo_ast_attribute_condition, condlit(litUid, litvec)));
        return uid;
    }

    BoundVecUid boundvec() override {
        return boundvecs_.emplace();
    }

    BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) override {
        boundvecs_[uid].emplace_back(ast(clingo_ast_type_aggregate_guard)
            .set(clingo_ast_attribute_comparison, static_cast<int>(rel))
            .set(clingo_ast_attribute_term, terms_.erase(term)));
        return uid;
    }

    CSPElemVecUid cspelemvec() override {
        return cspelems_.emplace();
    }

    CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) override {
        cspelems_[uid].emplace_back(ast(clingo_ast_type_disjoint, loc)
            .set(clingo_ast_attribute_tuple, termvecs_.erase(termvec))
            .set(clingo_ast_attribute_term, cspaddterms_.erase(addterm))
            .set(clingo_ast_attribute_condition, litvecs_.erase(litvec)));
        return uid;
    }

    // {{{2 heads

    HdLitUid headlit(LitUid litUid) override {
        return heads_.insert(lits_.erase(litUid));
    }

    HdLitUid headaggr(Location const &loc, TheoryAtomUid atomUid) override {
        return heads_.insert(theoryatoms_.erase(atomUid));
    }

    std::pair<AST::Value, AST::Value> guards_(BoundVecUid bounds) {
        AST::Value leftGuard = mpark::monostate();
        AST::Value rightGuard = mpark::monostate();
        auto guards = boundvecs_.erase(bounds);
        assert(guards.size() < 3);
        if (!guards.empty()) {
            auto &rel = get<int>(*guards.front(), clingo_ast_attribute_comparison);
            rel = static_cast<int>(inv(static_cast<Relation>(rel)));
            leftGuard = std::move(guards.front());
        }
        if (guards.size() > 1) {
            rightGuard = std::move(guards.back());
        }
        return {std::move(leftGuard), std::move(rightGuard)};
    }

    SAST aggr(Location const &loc, BoundVecUid bounds, CondLitVecUid elems) {
        auto guards = guards_(bounds);
        return ast(clingo_ast_type_aggregate, loc)
            .set(clingo_ast_attribute_left_guard, std::move(guards.first))
            .set(clingo_ast_attribute_elements, condlitvecs_.erase(elems))
            .set(clingo_ast_attribute_right_guard, std::move(guards.second));
    }

    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) override {
        auto guards = guards_(bounds);
        return heads_.insert(ast(clingo_ast_type_head_aggregate, loc)
            .set(clingo_ast_attribute_left_guard, std::move(guards.first))
            .set(clingo_ast_attribute_function, static_cast<int>(fun))
            .set(clingo_ast_attribute_elements, hdaggrelemvecs_.erase(headaggrelemvec))
            .set(clingo_ast_attribute_right_guard, std::move(guards.second)));
    }

    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) override {
        (void)fun;
        assert(fun == AggregateFunction::COUNT);
        return heads_.insert(aggr(loc, bounds, headaggrelemvec));
    }

    HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) override {
        return heads_.insert(ast(clingo_ast_type_head_aggregate, loc)
            .set(clingo_ast_attribute_elements, condlitvecs_.erase(condlitvec)));
    }

    // {{{2 bodies

    BdLitVecUid body() override {
        return bodylitvecs_.emplace();
    }

    BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) override {
        bodylitvecs_[body].emplace_back(lits_.erase(bodylit));
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atomUid) override {
        bodylitvecs_[body].emplace_back(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(naf))
            .set(clingo_ast_attribute_atom, theoryatoms_.erase(atomUid)));
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) override {
        auto guards = guards_(bounds);
        bodylitvecs_[body].emplace_back(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(naf))
            .set(clingo_ast_attribute_atom, ast(clingo_ast_type_body_aggregate, loc)
                .set(clingo_ast_attribute_left_guard, std::move(guards.first))
                .set(clingo_ast_attribute_function, static_cast<int>(fun))
                .set(clingo_ast_attribute_elements, bdaggrelemvecs_.erase(bodyaggrelemvec))
                .set(clingo_ast_attribute_right_guard, std::move(guards.second))));
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) override {
        bodylitvecs_[body].emplace_back(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(naf))
            .set(clingo_ast_attribute_atom, aggr(loc, bounds, bodyaggrelemvec)));
        return body;
    }

    BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) override {
        bodylitvecs_[body].emplace_back(condlit(head, litvec, &loc));
        return body;
    }

    BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) override {
        bodylitvecs_[body].emplace_back(ast(clingo_ast_type_literal, loc)
            .set(clingo_ast_attribute_sign, static_cast<int>(naf))
            .set(clingo_ast_attribute_atom, ast(clingo_ast_type_disjoint, loc)
                .set(clingo_ast_attribute_elements, cspelems_.erase(elem))));
        return body;
    }

    // {{{2 statements

    void rule(Location const &loc, HdLitUid head) override {
        rule(loc, head, body());
    }

    void rule(Location const &loc, HdLitUid head, BdLitVecUid body) override {
        cb_(ast(clingo_ast_type_rule, loc)
            .set(clingo_ast_attribute_head, heads_.erase(head))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body)));
    }

    void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &logger) override {
        static_cast<void>(logger);
        cb_(ast(clingo_ast_type_definition, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_value, terms_.erase(value))
            .set(clingo_ast_attribute_is_default, static_cast<int>(defaultDef)));
    }

    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override {
        cb_(ast(clingo_ast_type_minimize, loc)
            .set(clingo_ast_attribute_weight, terms_.erase(weight))
            .set(clingo_ast_attribute_priority, terms_.erase(priority))
            .set(clingo_ast_attribute_tuple, termvecs_.erase(cond))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body)));
    }

    void showsig(Location const &loc, Sig sig, bool csp) override {
        cb_(ast(clingo_ast_type_show_signature, loc)
            .set(clingo_ast_attribute_name, sig.name())
            .set(clingo_ast_attribute_arity, static_cast<int>(sig.arity()))
            .set(clingo_ast_attribute_sign, static_cast<int>(sig.sign()))
            .set(clingo_ast_attribute_is_default, static_cast<int>(csp)));
    }

    void defined(Location const &loc, Sig sig) override {
        cb_(ast(clingo_ast_type_defined, loc)
            .set(clingo_ast_attribute_name, sig.name())
            .set(clingo_ast_attribute_arity, static_cast<int>(sig.arity()))
            .set(clingo_ast_attribute_sign, static_cast<int>(sig.sign())));
    }

    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override {
        cb_(ast(clingo_ast_type_show_term, loc)
            .set(clingo_ast_attribute_term, terms_.erase(t))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body))
            .set(clingo_ast_attribute_csp, static_cast<int>(csp)));
    }

    void python(Location const &loc, String code) override {
        cb_(ast(clingo_ast_type_script, loc)
            .set(clingo_ast_attribute_script_type, static_cast<int>(clingo_ast_script_type_python))
            .set(clingo_ast_attribute_code, code));
    }

    void lua(Location const &loc, String code) override {
        cb_(ast(clingo_ast_type_script, loc)
            .set(clingo_ast_attribute_script_type, static_cast<int>(clingo_ast_script_type_lua))
            .set(clingo_ast_attribute_code, code));
    }

    void block(Location const &loc, String name, IdVecUid args) override {
        cb_(ast(clingo_ast_type_program, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_parameters, idvecs_.erase(args)));
    }

    void external(Location const &loc, TermUid head, BdLitVecUid body, TermUid type) override {
        cb_(ast(clingo_ast_type_external, loc)
            .set(clingo_ast_attribute_atom, symbolicatom(head))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body))
            .set(clingo_ast_attribute_external_type, terms_.erase(type)));
    }

    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override {
        auto bd = bodylitvecs_.erase(body);
        for (auto &x : termvecvecs_.erase(edges)) {
            for (auto &x : bd) {
                if (x.use_count() > 1) {
                    x = x->deepcopy();
                }
            }
            assert(x.size() == 2);
            cb_(ast(clingo_ast_type_edge, loc)
                .set(clingo_ast_attribute_node_u, std::move(x.front()))
                .set(clingo_ast_attribute_node_v, std::move(x.back()))
                .set(clingo_ast_attribute_body, bd));
        }
    }

    void heuristic(Location const &loc, TermUid termUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override {
        cb_(ast(clingo_ast_type_heuristic, loc)
            .set(clingo_ast_attribute_atom, symbolicatom(termUid))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body))
            .set(clingo_ast_attribute_bias, symbolicatom(a))
            .set(clingo_ast_attribute_priority, symbolicatom(b))
            .set(clingo_ast_attribute_modifier, symbolicatom(mod)));
    }

    void project(Location const &loc, TermUid termUid, BdLitVecUid body) override {
        cb_(ast(clingo_ast_type_project_atom, loc)
            .set(clingo_ast_attribute_atom, symbolicatom(termUid))
            .set(clingo_ast_attribute_body, bodylitvecs_.erase(body)));
    }

    void project(Location const &loc, Sig sig) override {
        cb_(ast(clingo_ast_type_project_signature, loc)
            .set(clingo_ast_attribute_name, sig.name())
            .set(clingo_ast_attribute_arity, static_cast<int>(sig.arity()))
            .set(clingo_ast_attribute_sign, static_cast<int>(sig.sign())));
    }

    // {{{2 theory atoms

    TheoryTermUid theorytermseq(Location const &loc, TheoryOptermVecUid args, clingo_ast_theory_sequence_type type) {
        return theoryterms_.insert(ast(clingo_ast_type_theory_sequence, loc)
            .set(clingo_ast_attribute_sequence_type, type)
            .set(clingo_ast_attribute_terms, theoryoptermvecs_.erase(args)));
    }

    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override {
        return theorytermseq(loc, args, clingo_ast_theory_sequence_type_set);
    }

    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override {
        return theorytermseq(loc, args, clingo_ast_theory_sequence_type_list);
    }

    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override {
        return theorytermseq(loc, args, clingo_ast_theory_sequence_type_tuple);
    }

    SAST unparsedterm(Location const &loc, TheoryOptermUid opterm) {
        auto elems = theoryopterms_.erase(opterm);
        if (elems.size() == 1 && get<AST::ASTVec>(*elems.front(), clingo_ast_attribute_operators).empty()) {
            return std::move(get<SAST>(*elems.front(), clingo_ast_attribute_term));
        }
        return ast(clingo_ast_type_theory_unparsed_term, loc)
            .set(clingo_ast_attribute_elements, std::move(elems));
    }

    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override {
        return theoryterms_.insert(unparsedterm(loc, opterm));
    }

    TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) override {
        return theoryterms_.insert(ast(clingo_ast_type_theory_function, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_arguments, theoryoptermvecs_.erase(args)));
    }

    TheoryTermUid theorytermvalue(Location const &loc, Symbol val) override {
        return theoryterms_.insert(ast(clingo_ast_type_symbol, loc)
            .set(clingo_ast_attribute_symbol, val));
    }

    TheoryTermUid theorytermvar(Location const &loc, String var) override {
        return theoryterms_.insert(ast(clingo_ast_type_symbol, loc)
            .set(clingo_ast_attribute_symbol, var));
    }

    SAST theoryunparsedelem(TheoryOpVecUid ops, TheoryTermUid term) {
        return ast(clingo_ast_type_theory_unparsed_term_element)
            .set(clingo_ast_attribute_operators, theoryopvecs_.erase(ops))
            .set(clingo_ast_attribute_term, theoryterms_.erase(term));
    }

    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override {
        return theoryopterms_.insert({theoryunparsedelem(ops, term)});
    }

    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override {
        theoryopterms_[opterm].emplace_back(theoryunparsedelem(ops, term));
        return opterm;
    }

    TheoryOpVecUid theoryops() override {
        return theoryopvecs_.emplace();
    }

    TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) override {
        theoryopvecs_[ops].emplace_back(op);
        return ops;
    }

    TheoryOptermVecUid theoryopterms() override {
        return theoryoptermvecs_.emplace();
    }

    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, Location const &loc, TheoryOptermUid opterm) override {
        theoryoptermvecs_[opterms].emplace_back(unparsedterm(loc, opterm));
        return opterms;
    }

    TheoryOptermVecUid theoryopterms(Location const &loc, TheoryOptermUid opterm, TheoryOptermVecUid opterms) override {
        theoryoptermvecs_[opterms].emplace(theoryoptermvecs_[opterms].begin(), unparsedterm(loc, opterm));
        return opterms;
    }

    TheoryElemVecUid theoryelems() override {
        return theoryelemvecs_.emplace();
    }

    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override {
        theoryelemvecs_[elems].emplace_back(ast(clingo_ast_type_theory_atom_element)
            .set(clingo_ast_attribute_tuple, theoryoptermvecs_.erase(opterms))
            .set(clingo_ast_attribute_elements, litvecs_.erase(cond)));
        return elems;
    }

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override {
        auto &loc = get<Location>(*terms_[term], clingo_ast_attribute_location);
        return theoryatoms_.insert(ast(clingo_ast_type_theory_atom, loc)
            .set(clingo_ast_attribute_term, terms_.erase(term))
            .set(clingo_ast_attribute_elements, theoryelemvecs_.erase(elems))
            .set(clingo_ast_attribute_guard, mpark::monostate()));
    }

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, Location const &loc, TheoryOptermUid opterm) override {
        auto &aloc = get<Location>(*terms_[term], clingo_ast_attribute_location);
        return theoryatoms_.insert(ast(clingo_ast_type_theory_atom, aloc)
            .set(clingo_ast_attribute_term, terms_.erase(term))
            .set(clingo_ast_attribute_elements, theoryelemvecs_.erase(elems))
            .set(clingo_ast_attribute_guard, ast(clingo_ast_type_theory_guard)
                .set(clingo_ast_attribute_operator_name, op)
                .set(clingo_ast_attribute_term, unparsedterm(loc, opterm))));
    }

    // {{{2 theory definitions

    TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) override {
        return theoryopdefs_.insert(ast(clingo_ast_type_theory_operator_definition, loc)
            .set(clingo_ast_attribute_name, op)
            .set(clingo_ast_attribute_priority, static_cast<int>(priority))
            .set(clingo_ast_attribute_operator_type, static_cast<int>(type)));
    }

    TheoryOpDefVecUid theoryopdefs() override {
        return theoryopdefvecs_.emplace();
    }

    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override {
        theoryopdefvecs_[defs].emplace_back(theoryopdefs_.erase(def));
        return defs;
    }

    TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &logger) override {
        static_cast<void>(logger);
        return theorytermdefs_.insert(ast(clingo_ast_type_theory_term_definition, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_operators, theoryopdefvecs_.erase(defs)));
    }

    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, AST::Value guard) {
        return theoryatomdefs_.insert(ast(clingo_ast_type_theory_atom_definition, loc)
            .set(clingo_ast_attribute_atom_type, static_cast<int>(type))
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_arity, static_cast<int>(arity))
            .set(clingo_ast_attribute_elements, termDef)
            .set(clingo_ast_attribute_guard, std::move(guard)));
    }

    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) override {
        return theoryatomdef(loc, name, arity, termDef, type, mpark::monostate());
    }

    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) override {
        return theoryatomdef(loc, name, arity, termDef, type, ast(clingo_ast_type_theory_guard_definition)
            .set(clingo_ast_attribute_operators, theoryopvecs_.erase(ops))
            .set(clingo_ast_attribute_term, guardDef));
    }

    TheoryDefVecUid theorydefs() override {
        return theorydefvecs_.emplace();
    }

    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override {
        theorydefvecs_[defs].first.emplace_back(theorytermdefs_.erase(def));
        return defs;
    }

    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override {
        theorydefvecs_[defs].second.emplace_back(theoryatomdefs_.erase(def));
        return defs;
    }

    void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &logger) override {
        static_cast<void>(logger);
        auto x = theorydefvecs_.erase(defs);
        cb_(ast(clingo_ast_type_theory_definition, loc)
            .set(clingo_ast_attribute_name, name)
            .set(clingo_ast_attribute_terms, std::move(x.first))
            .set(clingo_ast_attribute_atoms, std::move(x.second)));
    }

    // }}}2
private:
    SASTCallback cb_;
    Indexed<SAST, TermUid> terms_;
    Indexed<AST::ASTVec, TermVecUid> termvecs_;
    Indexed<std::vector<AST::ASTVec>, TermVecVecUid> termvecvecs_;
    Indexed<AST::ASTVec, IdVecUid> idvecs_;
    Indexed<SAST, LitUid> lits_;
    Indexed<AST::ASTVec, LitVecUid> litvecs_;
    Indexed<SAST, CSPMulTermUid> cspmulterms_;
    Indexed<SAST, CSPAddTermUid> cspaddterms_;
    Indexed<SAST, CSPLitUid> csplits_;
    Indexed<AST::ASTVec, CondLitVecUid> condlitvecs_;
    Indexed<AST::ASTVec, BdAggrElemVecUid> bdaggrelemvecs_;
    Indexed<AST::ASTVec, HdAggrElemVecUid> hdaggrelemvecs_;
    Indexed<AST::ASTVec, BoundVecUid> boundvecs_;
    Indexed<AST::ASTVec, BdLitVecUid> bodylitvecs_;
    Indexed<SAST, HdLitUid> heads_;
    Indexed<SAST, TheoryAtomUid> theoryatoms_;
    Indexed<AST::ASTVec, CSPElemVecUid> cspelems_;
    Indexed<SAST, TheoryTermUid> theoryterms_;
    Indexed<AST::ASTVec, TheoryOptermUid> theoryopterms_;
    Indexed<AST::ASTVec, TheoryOptermVecUid> theoryoptermvecs_;
    Indexed<AST::StrVec, TheoryOpVecUid> theoryopvecs_;
    Indexed<AST::ASTVec, TheoryElemVecUid> theoryelemvecs_;
    Indexed<SAST, TheoryOpDefUid> theoryopdefs_;
    Indexed<AST::ASTVec, TheoryOpDefVecUid> theoryopdefvecs_;
    Indexed<SAST, TheoryTermDefUid> theorytermdefs_;
    Indexed<SAST, TheoryAtomDefUid> theoryatomdefs_;
    Indexed<std::pair<AST::ASTVec, AST::ASTVec>, TheoryDefVecUid> theorydefvecs_;
};

// {{{1 ast parsing

struct ASTParser {
public:
    ASTParser(Logger &log, INongroundProgramBuilder &prg)
    : log_(log), prg_(prg) { }

    // {{{2 statement

    void parseStatement(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_rule: {
                return prg_.rule(get<Location>(ast, clingo_ast_attribute_location),
                                 parseHeadLiteral(*get<SAST>(ast, clingo_ast_attribute_head)),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)));
            }
            case clingo_ast_type_definition: {
                return prg_.define(get<Location>(ast, clingo_ast_attribute_location),
                                   get<String>(ast, clingo_ast_attribute_name), parseTerm(*get<SAST>(ast, clingo_ast_attribute_value)),
                                   get<int>(ast, clingo_ast_attribute_is_default) != 0, log_);
            }
            case clingo_ast_type_show_signature: {
                return prg_.showsig(get<Location>(ast, clingo_ast_attribute_location),
                                    Sig(get<String>(ast, clingo_ast_attribute_name),
                                        get<int>(ast, clingo_ast_attribute_arity),
                                        get<int>(ast, clingo_ast_attribute_sign) != 0),
                                    ast.hasValue(clingo_ast_attribute_csp) && get<int>(ast, clingo_ast_attribute_csp) != 0);
            }
            case clingo_ast_type_show_term: {
                return prg_.show(get<Location>(ast, clingo_ast_attribute_location),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_term)),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)),
                                 ast.hasValue(clingo_ast_attribute_csp) && get<int>(ast, clingo_ast_attribute_csp) != 0);
            }
            case clingo_ast_type_minimize: {
                return prg_.optimize(get<Location>(ast, clingo_ast_attribute_location),
                                     parseTerm(*get<SAST>(ast, clingo_ast_attribute_weight)),
                                     parseTerm(*get<SAST>(ast, clingo_ast_attribute_priority)),
                                     parseTermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_tuple)),
                                     parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)));
            }
            case clingo_ast_type_script: {
                switch (get<int>(ast, clingo_ast_attribute_script_type)) {
                    case clingo_ast_script_type_python: { return prg_.python(get<Location>(ast, clingo_ast_attribute_location),
                                                                             get<String>(ast, clingo_ast_attribute_code)); }
                    case clingo_ast_script_type_lua:    { return prg_.lua(get<Location>(ast, clingo_ast_attribute_location),
                                                                          get<String>(ast, clingo_ast_attribute_code)); }
                }
                break;
            }
            case clingo_ast_type_program: {
                return prg_.block(get<Location>(ast, clingo_ast_attribute_location),
                                  get<String>(ast, clingo_ast_attribute_name),
                                  parseIdVec(get<AST::ASTVec>(ast, clingo_ast_attribute_parameters)));
            }
            case clingo_ast_type_external: {
                return prg_.external(get<Location>(ast, clingo_ast_attribute_location),
                                     parseAtom(*get<SAST>(ast, clingo_ast_attribute_atom)),
                                     parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)),
                                     parseTerm(*get<SAST>(ast, clingo_ast_attribute_external_type)));
            }
            case clingo_ast_type_edge: {
                return prg_.edge(get<Location>(ast, clingo_ast_attribute_location),
                                 prg_.termvecvec(prg_.termvecvec(),
                                                 prg_.termvec(prg_.termvec(prg_.termvec(),
                                                                           parseTerm(*get<SAST>(ast, clingo_ast_attribute_node_u))),
                                                              parseTerm(*get<SAST>(ast, clingo_ast_attribute_node_v)))),
                                 parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)));
            }
            case clingo_ast_type_heuristic: {
                return prg_.heuristic(get<Location>(ast, clingo_ast_attribute_location),
                                      parseAtom(*get<SAST>(ast, clingo_ast_attribute_atom)),
                                      parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)),
                                      parseTerm(*get<SAST>(ast, clingo_ast_attribute_bias)),
                                      parseTerm(*get<SAST>(ast, clingo_ast_attribute_priority)),
                                      parseTerm(*get<SAST>(ast, clingo_ast_attribute_modifier)));
            }
            case clingo_ast_type_project_atom: {
                return prg_.project(get<Location>(ast, clingo_ast_attribute_location),
                                    parseAtom(*get<SAST>(ast, clingo_ast_attribute_atom)),
                                    parseBodyLiteralVec(get<AST::ASTVec>(ast, clingo_ast_attribute_body)));
            }
            case clingo_ast_type_project_signature: {
                return prg_.project(get<Location>(ast, clingo_ast_attribute_location),
                                    Sig(get<String>(ast, clingo_ast_attribute_name), get<int>(ast, clingo_ast_attribute_arity), get<int>(ast, clingo_ast_attribute_sign) != 0));
            }
            case clingo_ast_type_defined: {
                return prg_.defined(get<Location>(ast, clingo_ast_attribute_location),
                                    Sig(get<String>(ast, clingo_ast_attribute_name), get<int>(ast, clingo_ast_attribute_arity), get<int>(ast, clingo_ast_attribute_sign) != 0));
            }
            case clingo_ast_type_theory_definition: {
                return prg_.theorydef(get<Location>(ast, clingo_ast_attribute_location),
                                      get<String>(ast, clingo_ast_attribute_name),
                                      parseTheoryAtomDefinitionVec(parseTheoryTermDefinitionVec(get<AST::ASTVec>(ast, clingo_ast_attribute_terms)),
                                                                   get<AST::ASTVec>(ast, clingo_ast_attribute_atoms)),
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
            prg_.idvec(uid, get<Location>(*ast, clingo_ast_attribute_location), get<String>(*ast, clingo_ast_attribute_id));
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
                return prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                 get<String>(ast, clingo_ast_attribute_name));
            }
            case clingo_ast_type_symbol: {
                return prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                 get<Symbol>(ast, clingo_ast_attribute_symbol));
            }
            case clingo_ast_type_unary_operation: {
                return prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                 parseUnOp(get<int>(ast, clingo_ast_attribute_operator)),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_argument)));
            }
            case clingo_ast_type_binary_operation: {
                return prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                 parseBinOp(get<int>(ast, clingo_ast_attribute_operator)),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_left)),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_right)));
            }
            case clingo_ast_type_interval: {
                return prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_left)),
                                 parseTerm(*get<SAST>(ast, clingo_ast_attribute_right)));
            }
            case clingo_ast_type_function: {
                auto external = ast.hasValue(clingo_ast_attribute_external) && get<int>(ast, clingo_ast_attribute_external) != 0;
                auto name = get<String>(ast, clingo_ast_attribute_name);
                require_(!name.empty() || !external, "invalid ast: external functions must have a name");
                return !name.empty()
                    ? prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                name,
                                prg_.termvecvec(prg_.termvecvec(),
                                                parseTermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_arguments))),
                                external)
                    : prg_.term(get<Location>(ast, clingo_ast_attribute_location),
                                parseTermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_arguments)),
                                true);
            }
            case clingo_ast_type_pool: {
                return prg_.pool(get<Location>(ast, clingo_ast_attribute_location),
                                 parseTermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_arguments)));
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
        auto *variable = getOpt<SAST>(ast, clingo_ast_attribute_variable);
        return variable != nullptr
            ? prg_.cspmulterm(get<Location>(ast, clingo_ast_attribute_location),
                              parseTerm(*get<SAST>(ast, clingo_ast_attribute_coefficient)),
                              parseTerm(**variable))
            : prg_.cspmulterm(get<Location>(ast, clingo_ast_attribute_location),
                              parseTerm(*get<SAST>(ast, clingo_ast_attribute_coefficient)));
    }

    CSPAddTermUid parseCSPAddTerm(AST &ast) {
        require_(ast.type() == clingo_ast_type_csp_sum, "invalid ast: csp sum required");
        auto &terms = get<AST::ASTVec>(ast, clingo_ast_attribute_terms);
        require_(!terms.empty(), "invalid ast: csp sums terms must not be empty");
        auto it = terms.begin();
        auto ie = terms.end();
        auto uid = prg_.cspaddterm(get<Location>(**it, clingo_ast_attribute_location),
                                   parseCSPMulTerm(**it));
        for (++it; it != ie; ++it) {
            uid = prg_.cspaddterm(get<Location>(**it, clingo_ast_attribute_location),
                                  uid,
                                  parseCSPMulTerm(**it), true);

        }
        return uid;
    }

    TheoryTermUid parseTheoryTerm(AST &ast) {
        switch (ast.type()) {
            case clingo_ast_type_symbol : {
                return prg_.theorytermvalue(get<Location>(ast, clingo_ast_attribute_location), get<Symbol>(ast, clingo_ast_attribute_symbol));
            }
            case clingo_ast_type_variable: {
                return prg_.theorytermvar(get<Location>(ast, clingo_ast_attribute_location), get<String>(ast, clingo_ast_attribute_variable));
            }
            case clingo_ast_type_theory_sequence: {
                switch (get<int>(ast, clingo_ast_attribute_sequence_type)) {
                    case clingo_ast_theory_sequence_type_tuple: {
                        return prg_.theorytermtuple(get<Location>(ast, clingo_ast_attribute_location),
                                                    parseTheoryOptermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_terms)));
                    }
                    case clingo_ast_theory_sequence_type_list: {
                        return prg_.theoryoptermlist(get<Location>(ast, clingo_ast_attribute_location),
                                                     parseTheoryOptermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_terms)));
                    }
                    case clingo_ast_theory_sequence_type_set: {
                        return prg_.theorytermset(get<Location>(ast, clingo_ast_attribute_location),
                                                  parseTheoryOptermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_terms)));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: invalid theory sequence type");

                    }
                }
            }
            case clingo_ast_type_theory_function: {
                return prg_.theorytermfun(get<Location>(ast, clingo_ast_attribute_location),
                                          get<String>(ast, clingo_ast_attribute_name),
                                          parseTheoryOptermVec(get<AST::ASTVec>(ast, clingo_ast_attribute_arguments)));
            }
            case clingo_ast_type_theory_unparsed_term: {
                return prg_.theorytermopterm(get<Location>(ast, clingo_ast_attribute_location),
                                             parseTheoryUnparsedTermElements(get<AST::ASTVec>(ast, clingo_ast_attribute_elements)));
            }
            default: {
                throw std::runtime_error("invalid ast: theory term expected");
            }
        }
    }


    TheoryOptermVecUid parseTheoryOptermVec(AST::ASTVec &asts) {
        auto uid = prg_.theoryopterms();
        for (auto &ast : asts) {
            uid = prg_.theoryopterms(uid, get<Location>(*ast, clingo_ast_attribute_location), parseTheoryOpterm(*ast));
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
            return parseTheoryUnparsedTermElements(get<AST::ASTVec>(ast, clingo_ast_attribute_elements));
        }
        return prg_.theoryopterm(prg_.theoryops(), parseTheoryTerm(ast));
    }

    TheoryOptermUid parseTheoryUnparsedTermElements(AST::ASTVec &asts) {
        require_(!asts.empty(), "invalid ast: unparsed term list must not be empty");
        auto it = asts.begin();
        auto ie = asts.end();
        auto uid = prg_.theoryopterm(parseTheoryOpVec(get<AST::StrVec>(**it, clingo_ast_attribute_operators)),
                                     parseTheoryTerm(*get<SAST>(**it, clingo_ast_attribute_term)));
        for (++it; it != ie; ++it) {
            auto &operators = get<AST::StrVec>(**it, clingo_ast_attribute_operators);
            require_(!operators.empty(), "invalid ast: at least one operator necessary on right-hand-side of unparsed theory term");
            uid = prg_.theoryopterm(uid,
                                    parseTheoryOpVec(operators),
                                    parseTheoryTerm(*get<SAST>(**it, clingo_ast_attribute_term)));
        }
        return uid;
    }

    // {{{2 literals

    TermUid parseAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_symbolic_atom, "invalid ast: symbolic atom expected");
        return parseTerm(*get<SAST>(ast, clingo_ast_attribute_term));
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
                auto sign = parseSign(get<int>(ast, clingo_ast_attribute_sign));
                auto &atom = *get<SAST>(ast, clingo_ast_attribute_atom);

                switch (atom.type()) {
                    case clingo_ast_type_boolean_constant: {
                        return prg_.boollit(get<Location>(ast, clingo_ast_attribute_location), get<int>(ast, clingo_ast_attribute_value) != 0);
                    }
                    case clingo_ast_type_symbolic_atom: {
                        return prg_.predlit(get<Location>(ast, clingo_ast_attribute_location),
                                            parseSign(get<int>(ast, clingo_ast_attribute_sign)),
                                            parseAtom(*get<SAST>(ast, clingo_ast_attribute_symbol)));
                    }
                    case clingo_ast_type_comparison: {
                        auto rel = parseRelation(get<int>(atom, clingo_ast_attribute_comparison));
                        return prg_.rellit(get<Location>(ast, clingo_ast_attribute_location),
                                           sign != NAF::NOT ? rel : neg(rel),
                                           parseTerm(*get<SAST>(atom, clingo_ast_attribute_left)),
                                           parseTerm(*get<SAST>(atom, clingo_ast_attribute_right)));
                    }
                    default: {
                        throw std::runtime_error("invalid ast: atom expected");
                    }
                }
            }
            case clingo_ast_type_csp_literal: {
                auto &guards = get<AST::ASTVec>(ast, clingo_ast_attribute_guard);
                require_(!guards.empty(), "invalid ast: csp literals need at least one guard");
                auto it = guards.begin();
                auto ie = guards.end();
                auto uid = prg_.csplit(get<Location>(ast, clingo_ast_attribute_location),
                                       parseCSPAddTerm(*get<SAST>(ast, clingo_ast_attribute_term)),
                                       parseRelation(get<int>(**it, clingo_ast_attribute_comparison)),
                                       parseCSPAddTerm(*get<SAST>(**it, clingo_ast_attribute_term)));
                for (++it; it != ie; ++it) {
                    uid = prg_.csplit(get<Location>(ast, clingo_ast_attribute_location),
                                      uid,
                                      parseRelation(get<int>(**it, clingo_ast_attribute_comparison)),
                                      parseCSPAddTerm(*get<SAST>(**it, clingo_ast_attribute_term)));
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
        auto *right = getOpt<SAST>(ast, clingo_ast_attribute_right_guard);
        if (right != nullptr) {
            ret = prg_.boundvec(ret,
                                parseRelation(get<int>(**right, clingo_ast_attribute_comparison)),
                                parseTerm(*get<SAST>(**right, clingo_ast_attribute_term)));
        }
        auto *left = getOpt<SAST>(ast, clingo_ast_attribute_left_guard);
        if (left != nullptr) {
            ret = prg_.boundvec(ret,
                                inv(parseRelation(get<int>(**left, clingo_ast_attribute_comparison))),
                                parseTerm(*get<SAST>(**left, clingo_ast_attribute_term)));
        }
        return ret;
    }

    CondLitVecUid parseCondLitVec(AST::ASTVec &asts) {
        auto uid = prg_.condlitvec();
        for (auto &ast : asts) {
            uid = prg_.condlitvec(uid,
                                  parseLiteral(*get<SAST>(*ast, clingo_ast_attribute_literal)),
                                  parseLiteralVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_condition)));
        }
        return uid;
    }

    HdAggrElemVecUid parseHdAggrElemVec(AST::ASTVec &asts) {
        auto uid = prg_.headaggrelemvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_head_aggregate_element, "invalid ast: head aggregate element expected");
            auto &clit = get<SAST>(*ast, clingo_ast_attribute_condition);
            require_(clit->type() == clingo_ast_type_conditional_literal, "invalid ast: conditional literal expected");
            uid = prg_.headaggrelemvec(uid,
                                       parseTermVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_tuple)),
                                       parseLiteral(*get<SAST>(*clit, clingo_ast_attribute_literal)),
                                       parseLiteralVec(get<AST::ASTVec>(*clit, clingo_ast_attribute_condition)));
        }
        return uid;
    }

    BdAggrElemVecUid parseBdAggrElemVec(AST::ASTVec &asts) {
        auto uid = prg_.bodyaggrelemvec();
        for (auto &ast : asts) {
            require_(ast->type() == clingo_ast_type_body_aggregate_element, "invalid ast: body aggregate element expected");
            uid = prg_.bodyaggrelemvec(uid,
                                       parseTermVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_tuple)),
                                       parseLiteralVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_condition)));
        }
        return uid;
    }

    TheoryElemVecUid parseTheoryElemVec(AST::ASTVec &asts) {
        auto uid = prg_.theoryelems();
        for (auto &ast : asts) {
            uid = prg_.theoryelems(uid,
                                   parseTheoryOptermVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_tuple)),
                                   parseLiteralVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_condition)));
        }
        return uid;
    }

    CSPElemVecUid parseCSPElemVec(AST::ASTVec &asts) {
        auto ret = prg_.cspelemvec();
        for (auto &ast  : asts) {
            require_(ast->type() == clingo_ast_type_body_aggregate_element, "invalid ast: body aggregate element expected");
            ret = prg_.cspelemvec(ret,
                                  get<Location>(*ast, clingo_ast_attribute_location),
                                  parseTermVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_tuple)),
                                  parseCSPAddTerm(*get<SAST>(*ast, clingo_ast_attribute_term)),
                                  parseLiteralVec(get<AST::ASTVec>(*ast, clingo_ast_attribute_condition)));
        }
        return ret;
    }

    TheoryAtomUid parseTheoryAtom(AST &ast) {
        require_(ast.type() == clingo_ast_type_theory_atom, "invalid ast: theory atom expected");
        auto &loc = get<Location>(ast, clingo_ast_attribute_location);
        auto *guard = getOpt<SAST>(ast, clingo_ast_attribute_guard);
        auto term = parseTerm(*get<SAST>(ast, clingo_ast_attribute_term));
        auto elements = parseTheoryElemVec(get<AST::ASTVec>(ast, clingo_ast_attribute_elements));
        return guard != nullptr
            ? prg_.theoryatom(term,
                              elements,
                              get<String>(**guard, clingo_ast_attribute_operator_name),
                              loc,
                              parseTheoryOpterm(*get<SAST>(**guard, clingo_ast_attribute_term)))
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
                return prg_.disjunction(get<Location>(ast, clingo_ast_attribute_location),
                                        parseCondLitVec(get<AST::ASTVec>(ast, clingo_ast_attribute_elements)));
            }
            case clingo_ast_type_aggregate: {
                return prg_.headaggr(get<Location>(ast, clingo_ast_attribute_location),
                                     AggregateFunction::COUNT,
                                     parseBounds(ast),
                                     parseCondLitVec(get<AST::ASTVec>(ast, clingo_ast_attribute_elements)));
            }
            case clingo_ast_type_head_aggregate: {
                return prg_.headaggr(get<Location>(ast, clingo_ast_attribute_location),
                                     parseAggregateFunction(get<int>(ast, clingo_ast_attribute_function)),
                                     parseBounds(ast),
                                     parseHdAggrElemVec(get<AST::ASTVec>(ast, clingo_ast_attribute_elements)));
            }
            case clingo_ast_type_theory_atom: {
                auto &loc = get<Location>(ast, clingo_ast_attribute_location);
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
                    auto &loc = get<Location>(*lit, clingo_ast_attribute_location);
                    auto sign = parseSign(get<int>(*lit, clingo_ast_attribute_sign));
                    auto &atom = *get<SAST>(*lit, clingo_ast_attribute_atom);
                    switch (atom.type()) {
                        case clingo_ast_type_aggregate: {
                            uid = prg_.bodyaggr(uid, loc, sign, AggregateFunction::COUNT,
                                                parseBounds(atom),
                                                parseCondLitVec(get<AST::ASTVec>(atom, clingo_ast_attribute_elements)));
                            break;
                        }
                        case clingo_ast_type_body_aggregate: {
                            uid = prg_.bodyaggr(uid, loc, sign,
                                                parseAggregateFunction(get<int>(atom, clingo_ast_attribute_function)),
                                                parseBounds(atom),
                                                parseBdAggrElemVec(get<AST::ASTVec>(atom, clingo_ast_attribute_elements)));
                            break;
                        }
                        case clingo_ast_type_theory_atom: {
                            uid = prg_.bodyaggr(uid, loc, sign, parseTheoryAtom(atom));
                            break;
                        }
                        case clingo_ast_type_disjoint: {
                            uid = prg_.disjoint(uid, loc, sign,
                                                parseCSPElemVec(get<AST::ASTVec>(atom, clingo_ast_attribute_elements)));
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
                                           get<Location>(*lit, clingo_ast_attribute_location),
                                           parseLiteral(*get<SAST>(*lit, clingo_ast_attribute_literal)),
                                           parseLiteralVec(get<AST::ASTVec>(*lit, clingo_ast_attribute_condition)));
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
        return prg_.theoryopdef(get<Location>(ast, clingo_ast_attribute_location),
                                get<String>(ast, clingo_ast_attribute_name),
                                get<int>(ast, clingo_ast_attribute_priority),
                                parseTheoryOperatorType(get<int>(ast, clingo_ast_attribute_operator_type)));
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
        auto *guard = getOpt<SAST>(ast, clingo_ast_attribute_guard);
        auto &loc = get<Location>(ast, clingo_ast_attribute_location);
        auto name = get<String>(ast, clingo_ast_attribute_name);
        auto arity = get<int>(ast, clingo_ast_attribute_arity);
        auto elements = get<String>(ast, clingo_ast_attribute_elements);
        auto type = parseTheoryAtomType(get<int>(ast, clingo_ast_attribute_atom_type));
        return guard != nullptr
            ? prg_.theoryatomdef(loc, name, arity, elements, type,
                                 parseTheoryOpVec(get<AST::StrVec>(**guard, clingo_ast_attribute_operators)),
                                 get<String>(**guard, clingo_ast_attribute_term))
            : prg_.theoryatomdef(loc, name, arity, elements, type);
    }

    TheoryDefVecUid parseTheoryAtomDefinitionVec(TheoryDefVecUid uid, AST::ASTVec &asts) {
        for (auto &ast : asts) {
            prg_.theorydefs(uid, parseTheoryAtomDefinition(*ast));
        }
        return uid;
    }

    TheoryTermDefUid parseTheoryTermDefinition(AST &ast) {
        return prg_.theorytermdef(get<Location>(ast, clingo_ast_attribute_location),
                                  get<String>(ast, clingo_ast_attribute_name),
                                  parseTheoryOpDefVec(get<AST::ASTVec>(ast, clingo_ast_attribute_operators)),
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

// 1}}}

} // namespace

std::unique_ptr<INongroundProgramBuilder> build(SASTCallback cb) {
    return std::make_unique<ASTBuilder>(std::move(cb));
}

void parse(INongroundProgramBuilder &prg, Logger &log, AST &ast) {
    ASTParser{log, prg}.parseStatement(ast);
}

// {{{1 AST

AST::AST(clingo_ast_type type)
: type_{type} { }

bool AST::hasValue(clingo_ast_attribute name) const {
    return values_.find(name) != values_.end();
}

AST::Value const &AST::value(clingo_ast_attribute name) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
        throw std::runtime_error("ast does not contain the given key");
    }
    return it->second;
}

AST::Value &AST::value(clingo_ast_attribute name) {
    auto it = values_.find(name);
    if (it == values_.end()) {
        throw std::runtime_error("ast does not contain the given key");
    }
    return it->second;
}

void AST::value(clingo_ast_attribute name, Value value) {
    values_[name] = std::move(value);
}

SAST AST::copy() {
    return std::make_shared<AST>(*this);
}

SAST AST::deepcopy() {
    auto ast = std::make_shared<AST>(type_);
    for (auto &val : values_) {
        ast->values_.emplace(val.first, mpark::visit(Deepcopy{}, val.second));
    }
    return ast;
}

clingo_ast_type AST::type() const {
    return type_;
}

// 1}}}

} } // namespace Input Gringo
