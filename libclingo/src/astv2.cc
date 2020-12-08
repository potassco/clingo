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

class ast {
public:
    ast(clingo_ast_type type, Location const &loc)
    : ast_{std::make_shared<AST>(type)} {
        set("location", loc);
    }
    ast(clingo_ast_type type)
    : ast_{std::make_shared<AST>(type)} { }
    template <typename T>
    ast &set(char const *name, T &&value) {
        ast_->value(name) = std::forward<T>(value);
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
    // {{{2 terms

    TermUid term(Location const &loc, Symbol val) override {
        return terms_.insert(ast(clingo_ast_type_symbol, loc)
            .set("symbol", val));
    }

    TermUid term(Location const &loc, String name) override {
        return terms_.insert(ast(clingo_ast_type_variable, loc)
            .set("variable", name));
    }

    TermUid term(Location const &loc, UnOp op, TermUid a) override {
        return terms_.insert(ast(clingo_ast_type_unary_operation, loc)
            .set("operator", static_cast<int>(op))
            .set("argument", terms_.erase(a)));
    }

    TermUid pool_(Location const &loc, AST::ASTVec vec) {
        if (vec.size() == 1) {
            return terms_.insert(std::move(vec.front()));
        }
        return terms_.insert(ast(clingo_ast_type_pool, loc)
            .set("arguments", std::move(vec)));
    }

    TermUid term(Location const &loc, UnOp op, TermVecUid a) override {
        return term(loc, op, pool_(loc, termvecs_.erase(a)));
    }

    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override {
        return terms_.insert(ast(clingo_ast_type_binary_operation, loc)
            .set("operator", static_cast<int>(op))
            .set("left", terms_.erase(a))
            .set("right", terms_.erase(b)));
    }

    TermUid term(Location const &loc, TermUid a, TermUid b) override {
        return terms_.insert(ast(clingo_ast_type_interval, loc)
            .set("left", terms_.erase(a))
            .set("right", terms_.erase(b)));
    }

    static SAST fun_(Location const &loc, String name, AST::ASTVec args, bool external) {
        return ast(clingo_ast_type_function, loc)
            .set("name", name)
            .set("arguments", std::move(args))
            .set("external", static_cast<int>(external));
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
            .set("id", id));
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
            .set("coefficient", terms_.erase(coe))
            .set("var", terms_.erase(var)));
    }

    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override {
        return cspmulterms_.insert(ast(clingo_ast_type_csp_product, loc)
            .set("coefficient", terms_.erase(coe))
            .set("var", mpark::monostate()));
    }

    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override {
        if (!add) {
            auto &pos = get<SAST>(*cspmulterms_[b], "coefficient");
            pos = ast(clingo_ast_type_unary_operation, loc)
                .set("operator", static_cast<int>(clingo_ast_unary_operator_minus))
                .set("argument", std::move(pos));
        }
        auto &addterm = cspaddterms_[a];
        get<Location>(*addterm, "location") = loc;
        get<AST::ASTVec>(*addterm, "terms").emplace_back(cspmulterms_.erase(b));
        return a;
    }

    CSPAddTermUid cspaddterm(Location const &loc, CSPMulTermUid b) override {
        return cspaddterms_.emplace(ast(clingo_ast_type_csp_sum, loc)
            .set("terms", AST::ASTVec{cspmulterms_.erase(b)}));
    }

    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override {
        return csplits_.insert(ast(clingo_ast_type_csp_literal, loc)
            .set("term", cspaddterms_.erase(a))
            .set("guards", AST::ASTVec{ast(clingo_ast_type_csp_guard)
                .set("comparison", static_cast<int>(rel))
                .set("term", cspaddterms_.erase(b))}));
    }

    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override {
        auto &lit = csplits_[a];
        get<AST::ASTVec>(*lit, "terms").emplace_back(cspaddterms_.erase(b));
        return a;
    }

    /*
    // {{{2 literals

    LitUid boollit(Location const &loc, bool type) {
        clingo_ast_literal_t lit;
        lit.location = convertLoc(loc);
        lit.sign     = clingo_ast_sign_none;
        lit.type     = clingo_ast_literal_type_boolean;
        lit.boolean  = type;
        return lits_.insert(std::move(lit));
    }

    LitUid predlit(Location const &loc, NAF naf, TermUid termUid) {
        clingo_ast_literal_t lit;
        lit.location = convertLoc(loc);
        lit.sign     = static_cast<clingo_ast_sign_t>(naf);
        lit.type     = clingo_ast_literal_type_symbolic;
        lit.symbol   = create_(terms_.erase(termUid));
        return lits_.insert(std::move(lit));
    }

    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) {
        auto comp = create_<clingo_ast_comparison_t>();
        comp->comparison = static_cast<clingo_ast_comparison_operator_t>(rel);
        comp->left       = terms_.erase(termUidLeft);
        comp->right      = terms_.erase(termUidRight);
        clingo_ast_literal_t lit;
        lit.location   = convertLoc(loc);
        lit.sign       = clingo_ast_sign_none;
        lit.type       = clingo_ast_literal_type_comparison;
        lit.comparison = comp;
        return lits_.insert(std::move(lit));
    }

    LitUid csplit(CSPLitUid a) {
        auto rep = csplits_.erase(a);
        assert(rep.second.size() > 1);
        auto guards = createArray_<clingo_ast_csp_guard_t>(rep.second.size()-1);
        auto guardsit = guards;
        for (auto it = rep.second.begin() + 1, ie = rep.second.end(); it != ie; ++it, ++guardsit) {
            guardsit->comparison = static_cast<clingo_ast_comparison_operator_t>(it->first);
            guardsit->term = it->second;
        }
        auto csp = create_<clingo_ast_csp_literal_t>();
        csp->term   = rep.second.front().second;
        csp->size   = rep.second.size() - 1;
        csp->guards = guards;
        clingo_ast_literal_t lit;
        lit.location = convertLoc(rep.first);
        lit.sign        = clingo_ast_sign_none;
        lit.type        = clingo_ast_literal_type_csp;
        lit.csp_literal = csp;
        return lits_.insert(std::move(lit));
    }

    LitVecUid litvec() {
        return litvecs_.emplace();
    }

    LitVecUid litvec(LitVecUid uid, LitUid literalUid) {
        litvecs_[uid].emplace_back(lits_.erase(literalUid));
        return uid;
    }

    // {{{2 conditional literals

    CondLitVecUid condlitvec() {
        return condlitvecs_.emplace();
    }

    CondLitVecUid condlitvec(CondLitVecUid uid, LitUid litUid, LitVecUid litvecUid) {
        auto cond = litvecs_.erase(litvecUid);
        clingo_ast_conditional_literal_t lit;
        lit.size      = cond.size();
        lit.condition = createArray_(cond);
        lit.literal   = lits_.erase(litUid);
        condlitvecs_[uid].emplace_back(lit);
        return uid;
    }

    // {{{2 body aggregate elements

    BdAggrElemVecUid bodyaggrelemvec() {
        return bodyaggrelemvecs_.emplace();
    }

    BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) {
        clingo_ast_body_aggregate_element_t elem;
        auto cond = litvecs_.erase(litvec);
        auto tuple = termvecs_.erase(termvec);
        elem.condition_size = cond.size();
        elem.condition      = createArray_(cond);
        elem.tuple_size     = tuple.size();
        elem.tuple          = createArray_(tuple);
        bodyaggrelemvecs_[uid].emplace_back(elem);
        return uid;
    }

    // {{{2 head aggregate elements

    HdAggrElemVecUid headaggrelemvec() {
        return headaggrelemvecs_.emplace();
    }

    HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid litUid, LitVecUid litvec) {
        clingo_ast_head_aggregate_element_t elem;
        auto cond = litvecs_.erase(litvec);
        clingo_ast_conditional_literal_t lit;
        lit.size      = cond.size();
        lit.condition = createArray_(cond);
        lit.literal   = lits_.erase(litUid);
        auto tuple = termvecs_.erase(termvec);
        elem.conditional_literal = lit;
        elem.tuple_size          = tuple.size();
        elem.tuple               = createArray_(tuple);
        headaggrelemvecs_[uid].emplace_back(elem);
        return uid;
    }

    // {{{2 bounds

    BoundVecUid boundvec() {
        return bounds_.emplace();
    }

    BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) {
        clingo_ast_aggregate_guard_t guard;
        guard.term = terms_.erase(term);
        guard.comparison = static_cast<clingo_ast_comparison_operator_t>(rel);
        bounds_[uid].emplace_back(guard);
        return uid;
    }

    // {{{2 heads

    HdLitUid headlit(LitUid litUid) {
        clingo_ast_head_literal_t head;
        head.type     = clingo_ast_head_literal_type_literal;
        head.literal  = create_(lits_.erase(litUid));
        head.location = head.literal->location;
        return heads_.insert(std::move(head));
    }

    HdLitUid headaggr(Location const &loc, TheoryAtomUid atomUid) {
        clingo_ast_head_literal_t head;
        head.location    = convertLoc(loc);
        head.type        = clingo_ast_head_literal_type_theory_atom;
        head.theory_atom = create_(theoryAtoms_.erase(atomUid));
        return heads_.insert(std::move(head));
    }

    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) {
        auto guards = bounds_.erase(bounds);
        auto elems = headaggrelemvecs_.erase(headaggrelemvec);
        assert(guards.size() < 3);
        if (!guards.empty()) {
            guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
        }
        clingo_ast_head_aggregate_t aggr;
        aggr.function    = static_cast<clingo_ast_aggregate_function>(fun);
        aggr.size        = elems.size();
        aggr.elements    = createArray_(elems);
        aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
        aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
        clingo_ast_head_literal_t head;
        head.location       = convertLoc(loc);
        head.type           = clingo_ast_head_literal_type_head_aggregate;
        head.head_aggregate = create_(aggr);
        return heads_.insert(std::move(head));
    }

    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) {
        (void)fun;
        assert(fun == AggregateFunction::COUNT);
        auto guards = bounds_.erase(bounds);
        auto elems = condlitvecs_.erase(headaggrelemvec);
        assert(guards.size() < 3);
        if (!guards.empty()) {
            guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
        }
        clingo_ast_aggregate_t aggr;
        aggr.size        = elems.size();
        aggr.elements    = createArray_(elems);
        aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
        aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
        clingo_ast_head_literal_t head;
        head.location  = convertLoc(loc);
        head.type      = clingo_ast_head_literal_type_aggregate;
        head.aggregate = create_(aggr);
        return heads_.insert(std::move(head));
    }

    HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) {
        auto elems = condlitvecs_.erase(condlitvec);
        clingo_ast_disjunction_t disj;
        disj.size     = elems.size();
        disj.elements = createArray_(elems);
        clingo_ast_head_literal_t head;
        head.location    = convertLoc(loc);
        head.type        = clingo_ast_head_literal_type_disjunction;
        head.disjunction = create_(disj);
        return heads_.insert(std::move(head));
    }

    // {{{2 bodies

    BdLitVecUid body() {
        return bodies_.emplace();
    }

    BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) {
        auto lit = lits_.erase(bodylit);
        clingo_ast_body_literal_t bd;
        bd.location = lit.location;
        bd.sign     = static_cast<clingo_ast_sign_t>(clingo_ast_sign_none);
        bd.type     = clingo_ast_body_literal_type_literal;;
        bd.literal  = create_(lit);
        bodies_[body].emplace_back(bd);
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atomUid) {
        clingo_ast_body_literal_t bd;
        bd.location    = convertLoc(loc);
        bd.sign        = static_cast<clingo_ast_sign_t>(naf);
        bd.type        = clingo_ast_body_literal_type_theory_atom;;
        bd.theory_atom = create_(theoryAtoms_.erase(atomUid));
        bodies_[body].emplace_back(bd);
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) {
        auto guards = bounds_.erase(bounds);
        auto elems = bodyaggrelemvecs_.erase(bodyaggrelemvec);
        assert(guards.size() < 3);
        if (!guards.empty()) {
            guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
        }
        clingo_ast_body_aggregate_t aggr;
        aggr.function    = static_cast<clingo_ast_aggregate_function>(fun);
        aggr.size        = elems.size();
        aggr.elements    = createArray_(elems);
        aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
        aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
        clingo_ast_body_literal_t bd;
        bd.location       = convertLoc(loc);
        bd.sign           = static_cast<clingo_ast_sign_t>(naf);
        bd.type           = clingo_ast_body_literal_type_body_aggregate;
        bd.body_aggregate = create_(aggr);
        bodies_[body].emplace_back(bd);
        return body;
    }

    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) {
        (void)fun;
        assert(fun == AggregateFunction::COUNT);
        auto guards = bounds_.erase(bounds);
        auto elems = condlitvecs_.erase(bodyaggrelemvec);
        assert(guards.size() < 3);
        if (!guards.empty()) {
            guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
        }
        clingo_ast_aggregate_t aggr;
        aggr.size        = elems.size();
        aggr.elements    = createArray_(elems);
        aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
        aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
        clingo_ast_body_literal_t bd;
        bd.location  = convertLoc(loc);
        bd.sign      = static_cast<clingo_ast_sign_t>(naf);
        bd.type      = clingo_ast_body_literal_type_aggregate;
        bd.aggregate = create_(aggr);
        bodies_[body].emplace_back(bd);
        return body;
    }

    BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) {
        auto cond = litvecs_.erase(litvec);
        clingo_ast_conditional_literal_t lit;
        lit.literal   = lits_.erase(head);
        lit.size      = cond.size();
        lit.condition = createArray_(cond);
        clingo_ast_body_literal_t bd;
        bd.location    = convertLoc(loc);
        bd.sign        = clingo_ast_sign_none;
        bd.type        = clingo_ast_body_literal_type_conditional;
        bd.conditional = create_(lit);
        bodies_[body].emplace_back(bd);
        return body;
    }

    BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) {
        auto elems = cspelems_.erase(elem);
        clingo_ast_disjoint_t disj;
        disj.size     = elems.size();
        disj.elements = createArray_(elems);
        clingo_ast_body_literal_t bd;
        bd.location    = convertLoc(loc);
        bd.sign        = static_cast<clingo_ast_sign_t>(naf);
        bd.type        = clingo_ast_body_literal_type_disjoint;
        bd.disjoint    = create_(disj);
        bodies_[body].emplace_back(bd);
        return body;
    }

    // {{{2 csp constraint elements

    CSPElemVecUid cspelemvec() {
        return cspelems_.emplace();
    }

    CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) {
        auto tuple = termvecs_.erase(termvec);
        auto rep = cspaddterms_.erase(addterm);
        auto cond = litvecs_.erase(litvec);
        clingo_ast_csp_sum_term_t term;
        term.size     = rep.second.size();
        term.location = convertLoc(rep.first);
        term.terms    = createArray_(rep.second);
        clingo_ast_disjoint_element_t elem;
        elem.term           = term;
        elem.location       = convertLoc(loc);
        elem.tuple          = createArray_(tuple);
        elem.tuple_size     = tuple.size();
        elem.condition      = createArray_(cond);
        elem.condition_size = cond.size();
        cspelems_[uid].emplace_back(elem);
        return uid;
    }

    // {{{2 statements

    void rule(Location const &loc, HdLitUid head) {
        rule(loc, head, body());
    }

    void rule(Location const &loc, HdLitUid head, BdLitVecUid body) {
        auto lits = bodies_.erase(body);
        clingo_ast_rule_t rule;
        rule.head = heads_.erase(head);
        rule.size = lits.size();
        rule.body = createArray_(lits);
        clingo_ast_statement stm;
        stm.rule = create_(rule);
        statement_(loc, clingo_ast_statement_type_rule, stm);
    }

    void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &) {
        clingo_ast_definition_t def;
        def.name = name.c_str();
        def.value = terms_.erase(value);
        def.is_default = defaultDef;
        clingo_ast_statement stm;
        stm.definition = create_(def);
        statement_(loc, clingo_ast_statement_type_const, stm);
    }

    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) {
        auto bd = bodies_.erase(body);
        auto tuple = termvecs_.erase(cond);
        clingo_ast_minimize_t min;
        min.weight     = terms_.erase(weight);
        min.priority   = terms_.erase(priority);
        min.body_size  = bd.size();
        min.body       = createArray_(bd);
        min.tuple_size = tuple.size();
        min.tuple      = createArray_(tuple);
        clingo_ast_statement stm;
        stm.minimize = create_(min);
        statement_(loc, clingo_ast_statement_type_minimize, stm);
    }

    void showsig(Location const &loc, Sig sig, bool csp) {
        clingo_ast_show_signature_t show;
        show.signature = sig.rep();
        show.csp = csp;
        clingo_ast_statement stm;
        stm.show_signature = create_(show);
        statement_(loc, clingo_ast_statement_type_show_signature, stm);
    }

    void defined(Location const &loc, Sig sig) {
        clingo_ast_defined_t defined;
        defined.signature = sig.rep();
        clingo_ast_statement stm;
        stm.defined = create_(defined);
        statement_(loc, clingo_ast_statement_type_defined, stm);
    }

    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) {
        auto bd = bodies_.erase(body);
        clingo_ast_show_term_t show;
        show.term = terms_.erase(t);
        show.csp  = csp;
        show.body = createArray_(bd);
        show.size = bd.size();
        clingo_ast_statement stm;
        stm.show_term = create_(show);
        statement_(loc, clingo_ast_statement_type_show_term, stm);
    }

    void python(Location const &loc, String code) {
        clingo_ast_script_t script;
        script.code = code.c_str();
        script.type = clingo_ast_script_type_python;
        clingo_ast_statement stm;
        stm.script = create_(script);
        statement_(loc, clingo_ast_statement_type_script, stm);
    }

    void lua(Location const &loc, String code) {
        clingo_ast_script_t script;
        script.code = code.c_str();
        script.type = clingo_ast_script_type_lua;
        clingo_ast_statement stm;
        stm.script = create_(script);
        statement_(loc, clingo_ast_statement_type_script, stm);
    }

    void block(Location const &loc, String name, IdVecUid args) {
        auto params = idvecs_.erase(args);
        clingo_ast_program_t program;
        program.name       = name.c_str();
        program.parameters = createArray_(params);
        program.size       = params.size();
        clingo_ast_statement stm;
        stm.program = create_(program);
        statement_(loc, clingo_ast_statement_type_program, stm);
    }

    void external(Location const &loc, TermUid head, BdLitVecUid body, TermUid type) {
        auto bd = bodies_.erase(body);
        clingo_ast_external_t ext;
        ext.atom = terms_.erase(head);
        ext.body = createArray_(bd);
        ext.size = bd.size();
        ext.type = terms_.erase(type);
        clingo_ast_statement stm;
        stm.external = create_(ext);
        statement_(loc, clingo_ast_statement_type_external, stm);
    }

    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) {
        auto bd = bodies_.erase(body);
        for (auto &x : termvecvecs_.erase(edges)) {
            assert(x.size() == 2);
            clingo_ast_edge_t edge;
            edge.u = x[0];
            edge.v = x[1];
            edge.size = bd.size();
            edge.body = createArray_(bd);
            clingo_ast_statement stm;
            stm.location = convertLoc(loc);
            stm.type     = clingo_ast_statement_type_edge;
            stm.edge     = create_(edge);
            cb_(stm);
        }
        clear_();
    }

    void heuristic(Location const &loc, TermUid termUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) {
        auto bd = bodies_.erase(body);
        clingo_ast_heuristic_t heu;
        heu.atom     = terms_.erase(termUid);
        heu.bias     = terms_.erase(a);
        heu.priority = terms_.erase(b);
        heu.modifier = terms_.erase(mod);
        heu.body     = createArray_(bd);
        heu.size     = bd.size();
        clingo_ast_statement stm;
        stm.heuristic = create_(heu);
        statement_(loc, clingo_ast_statement_type_heuristic, stm);
    }

    void project(Location const &loc, TermUid termUid, BdLitVecUid body) {
        auto bd = bodies_.erase(body);
        clingo_ast_project_t proj;
        proj.size = bd.size();
        proj.body = createArray_(bd);
        proj.atom = terms_.erase(termUid);
        clingo_ast_statement stm;
        stm.project_atom = create_(proj);
        statement_(loc, clingo_ast_statement_type_project_atom, stm);
    }

    void project(Location const &loc, Sig sig) {
        clingo_ast_statement stm;
        stm.project_signature = sig.rep();
        statement_(loc, clingo_ast_statement_type_project_atom_signature, stm);
    }

    // {{{2 theory atoms

    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) {
        return theorytermarr_(loc, args, clingo_ast_theory_term_type_set);
    }

    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) {
        return theorytermarr_(loc, args, clingo_ast_theory_term_type_list);
    }

    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) {
        return theorytermarr_(loc, args, clingo_ast_theory_term_type_tuple);
    }

    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) {
        return theoryTerms_.insert(opterm_(loc, opterm));
    }

    TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) {
        auto a = theoryOptermVecs_.erase(args);
        clingo_ast_theory_function_t fun;
        fun.name      = name.c_str();
        fun.size      = a.size();
        fun.arguments = createArray_(a);
        clingo_ast_theory_term_t term;
        term.type     = clingo_ast_theory_term_type_function;
        term.location = convertLoc(loc);
        term.function = create_(fun);
        return theoryTerms_.insert(std::move(term));
    }

    TheoryTermUid theorytermvalue(Location const &loc, Symbol val) {
        clingo_ast_theory_term_t term;
        term.type     = clingo_ast_theory_term_type_symbol;
        term.location = convertLoc(loc);
        term.symbol   = val.rep();
        return theoryTerms_.insert(std::move(term));
    }

    TheoryTermUid theorytermvar(Location const &loc, String var) {
        clingo_ast_theory_term_t term;
        term.type     = clingo_ast_theory_term_type_variable;
        term.location = convertLoc(loc);
        term.variable = var.c_str();
        return theoryTerms_.insert(std::move(term));
    }

    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) {
        return theoryOpterms_.insert({opterm_(ops, term)});
    }

    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) {
        theoryOpterms_[opterm].emplace_back(opterm_(ops, term));
        return opterm;
    }

    TheoryOpVecUid theoryops() {
        return theoryOpVecs_.emplace();
    }

    TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) {
        theoryOpVecs_[ops].emplace_back(op.c_str());
        return ops;
    }

    TheoryOptermVecUid theoryopterms() {
        return theoryOptermVecs_.emplace();
    }

    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, Location const &loc, TheoryOptermUid opterm) {
        theoryOptermVecs_[opterms].emplace_back(opterm_(loc, opterm));
        return opterms;
    }

    TheoryOptermVecUid theoryopterms(Location const &loc, TheoryOptermUid opterm, TheoryOptermVecUid opterms) {
        theoryOptermVecs_[opterms].emplace(theoryOptermVecs_[opterms].begin(), opterm_(loc, opterm));
        return opterms;
    }

    TheoryElemVecUid theoryelems() {
        return theoryElems_.emplace();
    }

    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) {
        auto ops = theoryOptermVecs_.erase(opterms);
        auto cnd = litvecs_.erase(cond);
        clingo_ast_theory_atom_element_t elem;
        elem.tuple_size     = ops.size();
        elem.tuple          = createArray_(ops);
        elem.condition_size = cnd.size();
        elem.condition      = createArray_(cnd);
        theoryElems_[elems].emplace_back(elem);
        return elems;
    }

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) {
        auto elms = theoryElems_.erase(elems);
        clingo_ast_theory_atom_t atom;
        atom.term     = terms_.erase(term);
        atom.elements = createArray_(elms);
        atom.size     = elms.size();
        atom.guard    = nullptr;
        return theoryAtoms_.insert(std::move(atom));
    }

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, Location const &loc, TheoryOptermUid opterm) {
        auto elms = theoryElems_.erase(elems);
        clingo_ast_theory_guard_t guard;
        guard.term          = opterm_(loc, opterm);
        guard.operator_name = op.c_str();
        clingo_ast_theory_atom_t atom;
        atom.term     = terms_.erase(term);
        atom.elements = createArray_(elms);
        atom.size     = elms.size();
        atom.guard    = create_(guard);
        return theoryAtoms_.insert(std::move(atom));
    }

    // {{{2 theory definitions

    TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) {
        clingo_ast_theory_operator_definition_t def;
        def.location = convertLoc(loc);
        def.priority = priority;
        def.name     = op.c_str();
        def.type     = static_cast<clingo_ast_theory_operator_type_t>(type);
        return theoryOpDefs_.insert(std::move(def));
    }

    TheoryOpDefVecUid theoryopdefs() {
        return theoryOpDefVecs_.emplace();
    }

    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) {
        theoryOpDefVecs_[defs].emplace_back(theoryOpDefs_.erase(def));
        return defs;
    }

    TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &) {
        auto dfs = theoryOpDefVecs_.erase(defs);
        clingo_ast_theory_term_definition_t def;
        def.location  = convertLoc(loc);
        def.name      = name.c_str();
        def.operators = createArray_(dfs);
        def.size      = dfs.size();
        return theoryTermDefs_.insert(std::move(def));
    }

    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) {
        clingo_ast_theory_atom_definition_t def;
        def.location = convertLoc(loc);
        def.name     = name.c_str();
        def.arity    = arity;
        def.elements = termDef.c_str();
        def.type     = static_cast<clingo_ast_theory_atom_definition_type_t>(type);
        def.guard    = nullptr;
        return theoryAtomDefs_.insert(std::move(def));
    }

    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) {
        auto o = theoryOpVecs_.erase(ops);
        clingo_ast_theory_guard_definition_t guard;
        guard.term      = guardDef.c_str();
        guard.operators = createArray_(o);
        guard.size      = o.size();
        clingo_ast_theory_atom_definition_t def;
        def.location = convertLoc(loc);
        def.name     = name.c_str();
        def.arity    = arity;
        def.elements = termDef.c_str();
        def.type     = static_cast<clingo_ast_theory_atom_definition_type_t>(type);
        def.guard    = create_(guard);
        return theoryAtomDefs_.insert(std::move(def));
    }

    TheoryDefVecUid theorydefs() {
        return theoryDefVecs_.emplace();
    }

    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) {
        theoryDefVecs_[defs].first.emplace_back(theoryTermDefs_.erase(def));
        return defs;
    }

    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) {
        theoryDefVecs_[defs].second.emplace_back(theoryAtomDefs_.erase(def));
        return defs;
    }

    void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &) {
        auto d = theoryDefVecs_.erase(defs);
        clingo_ast_theory_definition_t def;
        def.name = name.c_str();
        def.terms_size = d.first.size();
        def.terms      = createArray_(d.first);
        def.atoms_size = d.second.size();
        def.atoms      = createArray_(d.second);
        clingo_ast_statement stm;
        stm.theory_definition = create_(def);
        statement_(loc, clingo_ast_statement_type_theory_definition, stm);
    }

    // }}}2

private:
    TermUid pool_(Location const &loc, TermVec &&vec) {
        if (vec.size() == 1) {
            return terms_.insert(std::move(vec.front()));
        }
        else {
            auto pool = create_<clingo_ast_pool_t>();
            pool->size      = vec.size();
            pool->arguments = createArray_(vec);
            clingo_ast_term_t term;
            term.location = convertLoc(loc);
            term.type     = clingo_ast_term_type_pool;
            term.pool     = pool;
            return terms_.insert(std::move(term));
        }
    }

    clingo_ast_term_t fun_(Location const &loc, String name, TermVec &&vec, bool external) {
        auto fun = create_<clingo_ast_function_t>();
        fun->name      = name.c_str();
        fun->size      = vec.size();
        fun->arguments = createArray_(vec);
        clingo_ast_term_t term;
        term.location = convertLoc(loc);
        term.type     = external ? clingo_ast_term_type_external_function : clingo_ast_term_type_function;
        term.function = fun;
        return term;
    }

    TheoryTermUid theorytermarr_(Location const &loc, TheoryOptermVecUid args, clingo_ast_theory_term_type_t type) {
        auto a = theoryOptermVecs_.erase(args);
        clingo_ast_theory_term_array_t arr;
        arr.size  = a.size();
        arr.terms = createArray_(a);
        clingo_ast_theory_term_t term;
        term.type     = type;
        term.location = convertLoc(loc);
        term.set      = create_(arr);
        return theoryTerms_.insert(std::move(term));
    }

    clingo_ast_theory_term_t opterm_(Location const &loc, TheoryOptermUid opterm) {
        auto terms = theoryOpterms_.erase(opterm);
        clingo_ast_theory_unparsed_term arr;
        arr.size     = terms.size();
        arr.elements = createArray_(terms);
        clingo_ast_theory_term_t term;
        term.location      = convertLoc(loc);
        term.type          = clingo_ast_theory_term_type_unparsed_term;
        term.unparsed_term = create_(arr);
        return term;
    }

    clingo_ast_theory_unparsed_term_element_t opterm_(TheoryOpVecUid ops, TheoryTermUid term) {
        auto o = theoryOpVecs_.erase(ops);
        clingo_ast_theory_unparsed_term_element_t t;
        t.size      = o.size();
        t.operators = createArray_(o);
        t.term      = theoryTerms_.erase(term);
        return t;
    }

    template <class T>
    T *create_() {
        data_.emplace_back(operator new(sizeof(T)));
        return reinterpret_cast<T*>(data_.back());
    }
    template <class T>
    T *create_(T x) {
        auto *r = create_<T>();
        *r = x;
        return r;
    }
    template <class T>
    T *createArray_(size_t size) {
        arrdata_.emplace_back(operator new[](sizeof(T) * size));
        return reinterpret_cast<T*>(arrdata_.back());
    }
    template <class T>
    T *createArray_(std::vector<T> const &vec) {
        auto *r = createArray_<T>(vec.size());
        std::copy(vec.begin(), vec.end(), reinterpret_cast<T*>(r));
        return r;
    }

    void clear_() noexcept {
        for (auto &x : data_) { operator delete(x); }
        for (auto &x : arrdata_) { operator delete[](x); }
        data_.clear();
        arrdata_.clear();
    }

    void statement_(Location loc, clingo_ast_statement_type_t type, clingo_ast_statement_t &stm) {
        stm.location = convertLoc(loc);
        stm.type = type;
        cb_(stm);
        clear_();
    }

    using Terms             = Indexed<clingo_ast_term_t, TermUid>;
    using TermVecs          = Indexed<TermVec, TermVecUid>;
    using TermVecVecs       = Indexed<TermVecVec, TermVecVecUid>;
    using CSPAddTerms       = Indexed<std::pair<Location, std::vector<clingo_ast_csp_product_term_t>>, CSPAddTermUid>;
    using CSPMulTerms       = Indexed<clingo_ast_csp_product_term_t, CSPMulTermUid>;
    using CSPLits           = Indexed<std::pair<Location, std::vector<std::pair<Relation, clingo_ast_csp_sum_term_t>>>, CSPLitUid>;
    using IdVecs            = Indexed<std::vector<clingo_ast_id_t>, IdVecUid>;
    using Lits              = Indexed<clingo_ast_literal_t, LitUid>;
    using LitVecs           = Indexed<std::vector<clingo_ast_literal_t>, LitVecUid>;
    using CondLitVecs       = Indexed<std::vector<clingo_ast_conditional_literal_t>, CondLitVecUid>;
    using BodyAggrElemVecs  = Indexed<std::vector<clingo_ast_body_aggregate_element_t>, BdAggrElemVecUid>;
    using HeadAggrElemVecs  = Indexed<std::vector<clingo_ast_head_aggregate_element_t>, HdAggrElemVecUid>;
    using Bounds            = Indexed<std::vector<clingo_ast_aggregate_guard_t>, BoundVecUid>;
    using Bodies            = Indexed<std::vector<clingo_ast_body_literal_t>, BdLitVecUid>;
    using Heads             = Indexed<clingo_ast_head_literal_t, HdLitUid>;
    using TheoryAtoms       = Indexed<clingo_ast_theory_atom_t, TheoryAtomUid>;
    using CSPElems          = Indexed<std::vector<clingo_ast_disjoint_element_t>, CSPElemVecUid>;
    using TheoryOpVecs      = Indexed<std::vector<char const *>, TheoryOpVecUid>;
    using TheoryTerms       = Indexed<clingo_ast_theory_term_t, TheoryTermUid>;
    using RawTheoryTerms    = Indexed<std::vector<clingo_ast_theory_unparsed_term_element_t>, TheoryOptermUid>;
    using RawTheoryTermVecs = Indexed<std::vector<clingo_ast_theory_term_t>, TheoryOptermVecUid>;
    using TheoryElementVecs = Indexed<std::vector<clingo_ast_theory_atom_element>, TheoryElemVecUid>;
    using TheoryOpDefs      = Indexed<clingo_ast_theory_operator_definition_t, TheoryOpDefUid>;
    using TheoryOpDefVecs   = Indexed<std::vector<clingo_ast_theory_operator_definition_t>, TheoryOpDefVecUid>;
    using TheoryTermDefs    = Indexed<clingo_ast_theory_term_definition_t, TheoryTermDefUid>;
    using TheoryAtomDefs    = Indexed<clingo_ast_theory_atom_definition_t, TheoryAtomDefUid>;
    using TheoryDefVecs     = Indexed<std::pair<std::vector<clingo_ast_theory_term_definition_t>, std::vector<clingo_ast_theory_atom_definition_t>>, TheoryDefVecUid>;

    Callback            cb_;
    Terms               terms_;
    TermVecs            termvecs_;
    TermVecVecs         termvecvecs_;
    CSPAddTerms         cspaddterms_;
    CSPMulTerms         cspmulterms_;
    CSPLits             csplits_;
    IdVecs              idvecs_;
    Lits                lits_;
    LitVecs             litvecs_;
    CondLitVecs         condlitvecs_;
    BodyAggrElemVecs    bodyaggrelemvecs_;
    HeadAggrElemVecs    headaggrelemvecs_;
    Bounds              bounds_;
    Bodies              bodies_;
    Heads               heads_;
    TheoryAtoms         theoryAtoms_;
    CSPElems            cspelems_;
    TheoryOpVecs        theoryOpVecs_;
    TheoryTerms         theoryTerms_;
    RawTheoryTerms      theoryOpterms_;
    RawTheoryTermVecs   theoryOptermVecs_;
    TheoryElementVecs   theoryElems_;
    TheoryOpDefs        theoryOpDefs_;
    TheoryOpDefVecs     theoryOpDefVecs_;
    TheoryTermDefs      theoryTermDefs_;
    TheoryAtomDefs      theoryAtomDefs_;
    TheoryDefVecs       theoryDefVecs_;
    std::vector<void *> data_;
    std::vector<void *> arrdata_;
    */
private:
    Indexed<SAST, TermUid> terms_;
    Indexed<AST::ASTVec, TermVecUid> termvecs_;
    Indexed<std::vector<AST::ASTVec>, TermVecVecUid> termvecvecs_;
    Indexed<AST::ASTVec, IdVecUid> idvecs_;
    Indexed<SAST, LitUid> lits_;
    Indexed<AST::ASTVec, LitVecUid> litvecs_;
    Indexed<SAST, CSPMulTermUid> cspmulterms_;
    Indexed<SAST, CSPAddTermUid> cspaddterms_;
    Indexed<SAST, CSPLitUid> csplits_;
};

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

AST::AST(clingo_ast_type type)
: type_{type} { }

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
