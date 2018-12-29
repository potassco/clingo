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

#ifndef CLINGO_AST_BUILDER_HH
#define CLINGO_AST_BUILDER_HH

#include <gringo/input/programbuilder.hh>
#include <clingo.h>

namespace Gringo { namespace Input {

// {{{1 declaration of ASTBuilder

class ASTBuilder : public Gringo::Input::INongroundProgramBuilder {
public:
    using Callback = std::function<void (clingo_ast_statement const &ast)>;
    ASTBuilder(Callback cb);
    virtual ~ASTBuilder() noexcept;

    // {{{2 terms
    TermUid term(Location const &loc, Symbol val) override;
    TermUid term(Location const &loc, String name) override;
    TermUid term(Location const &loc, UnOp op, TermUid a) override;
    TermUid term(Location const &loc, UnOp op, TermVecUid a) override;
    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, String name, TermVecVecUid a, bool lua) override;
    TermUid term(Location const &loc, TermVecUid a, bool forceTuple) override;
    TermUid pool(Location const &loc, TermVecUid a) override;
    // {{{2 csp
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) override;
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override;
    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override;
    CSPAddTermUid cspaddterm(Location const &, CSPMulTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override;
    // {{{2 id vectors
    IdVecUid idvec() override;
    IdVecUid idvec(IdVecUid uid, Location const &loc, String id) override;
    // {{{2 term vectors
    TermVecUid termvec() override;
    TermVecUid termvec(TermVecUid uid, TermUid termUid) override;
    // {{{2 term vector vectors
    TermVecVecUid termvecvec() override;
    TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) override;
    // {{{2 literals
    LitUid boollit(Location const &loc, bool type) override;
    LitUid predlit(Location const &loc, NAF naf, TermUid term) override;
    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) override;
    LitUid csplit(CSPLitUid a) override;
    // {{{2 literal vectors
    LitVecUid litvec() override;
    LitVecUid litvec(LitVecUid uid, LitUid literalUid) override;
    // {{{2 conditional literals
    CondLitVecUid condlitvec() override;
    CondLitVecUid condlitvec(CondLitVecUid uid, LitUid litUid, LitVecUid litvecUid) override;
    // {{{2 body aggregate elements
    BdAggrElemVecUid bodyaggrelemvec() override;
    BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) override;
    // {{{2 head aggregate elements
    HdAggrElemVecUid headaggrelemvec() override;
    HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec) override;
    // {{{2 bounds
    BoundVecUid boundvec() override;
    BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) override;
    // {{{2 heads
    HdLitUid headlit(LitUid litUid) override;
    HdLitUid headaggr(Location const &loc, TheoryAtomUid atomUid) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) override;
    HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) override;
    // {{{2 bodies
    BdLitVecUid body() override;
    BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atomUid) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) override;
    BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) override;
    BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) override;
    // {{{2 csp constraint elements
    CSPElemVecUid cspelemvec() override;
    CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) override;
    // {{{2 statements
    void rule(Location const &loc, HdLitUid head) override;
    void rule(Location const &loc, HdLitUid head, BdLitVecUid body) override;
    void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) override;
    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override;
    void showsig(Location const &loc, Sig sig, bool csp) override;
    void defined(Location const &loc, Sig sig) override;
    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override;
    void python(Location const &loc, String code) override;
    void lua(Location const &loc, String code) override;
    void block(Location const &loc, String name, IdVecUid args) override;
    void external(Location const &loc, TermUid head, BdLitVecUid body, TermUid type) override;
    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override;
    void heuristic(Location const &loc, TermUid termUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override;
    void project(Location const &loc, TermUid termUid, BdLitVecUid body) override;
    void project(Location const &loc, Sig sig) override;
    // {{{2 theory atoms
    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override;
    TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermvalue(Location const &loc, Symbol val) override;
    TheoryTermUid theorytermvar(Location const &loc, String var) override;
    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOpVecUid theoryops() override;
    TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) override;
    TheoryOptermVecUid theoryopterms() override;
    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, Location const &loc, TheoryOptermUid opterm) override;
    TheoryOptermVecUid theoryopterms(Location const &loc, TheoryOptermUid opterm, TheoryOptermVecUid opterms) override;
    TheoryElemVecUid theoryelems() override;
    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, Location const &loc, TheoryOptermUid opterm) override;
    // {{{2 theory definitions
    TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) override;
    TheoryOpDefVecUid theoryopdefs() override;
    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override;
    TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) override;
    TheoryDefVecUid theorydefs() override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override;
    void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) override;
    // }}}2

private:
    using TermVec = std::vector<clingo_ast_term_t>;
    using TermVecVec = std::vector<TermVec>;

    TermUid pool_(Location const &loc, TermVec &&vec);
    clingo_ast_term_t fun_(Location const &loc, String name, TermVec &&vec, bool external);
    TheoryTermUid theorytermarr_(Location const &loc, TheoryOptermVecUid args, clingo_ast_theory_term_type_t type);
    clingo_ast_theory_unparsed_term_element_t opterm_(TheoryOpVecUid ops, TheoryTermUid term);
    clingo_ast_theory_term_t opterm_(Location const &loc, TheoryOptermUid opterm);
    template <class T>
    T *create_();
    template <class T>
    T *create_(T x);
    template <class T>
    T *createArray_(size_t size);
    template <class T>
    T *createArray_(std::vector<T> const &vec);

    void statement_(Location loc, clingo_ast_statement_type_t type, clingo_ast_statement_t &stm);
    void clear_() noexcept;

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
};

void parseStatement(INongroundProgramBuilder &prg, Logger &log, clingo_ast_statement_t const &stm);

// }}}1

} } // namespace Input Gringo

#endif // CLINGO_AST_BUILDER_HH
