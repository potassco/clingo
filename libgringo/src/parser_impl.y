// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

%include {
#include "parser_impl.h"
#include "gringo/gringo.h"
#include "gringo/parser.h"
#include "gringo/grounder.h"
#include "gringo/output.h"
#include "gringo/domain.h"

#include "gringo/rule.h"
#include "gringo/optimize.h"
#include "gringo/display.h"
#include "gringo/external.h"
#include "gringo/compute.h"

#include "gringo/predlit.h"
#include "gringo/rellit.h"

#include "gringo/sumaggrlit.h"
#include "gringo/avgaggrlit.h"
#include "gringo/junctionaggrlit.h"
#include "gringo/minmaxaggrlit.h"
#include "gringo/parityaggrlit.h"

#include "gringo/constterm.h"
#include "gringo/varterm.h"
#include "gringo/mathterm.h"
#include "gringo/poolterm.h"
#include "gringo/argterm.h"
#include "gringo/rangeterm.h"
#include "gringo/functerm.h"
#include "gringo/luaterm.h"

#define GRD pParser->grounder()
#define OUT pParser->grounder()->output()
#define ONE(loc) new ConstTerm(loc, Val::create(Val::NUM, 1))
#define ZERO(loc) new ConstTerm(loc, Val::create(Val::NUM, 0))
#define MINUSONE(loc) new ConstTerm(loc, Val::create(Val::NUM, -1))

template <class T>
void del(T x)
{
	delete x;
}

template <class T>
boost::ptr_vector<T> *vec1(T *x)
{
	boost::ptr_vector<T> *v = new boost::ptr_vector<T>();
	v->push_back(x);
	return v;
}

typedef std::vector<VarSigVec> VarSigVecVec;

}

%name parser
%stack_size 0

%parse_failure { pParser->parseError(); }
%syntax_error  { pParser->syntaxError(); }

%extra_argument { Parser *pParser }
%token_type { Parser::Token }
%token_destructor { (void)pParser; }
%token_prefix PARSER_

%start_symbol start

%type rule { Rule* }
%destructor Rule { del($$); }

%type var_list { VarSigVec* }
%destructor var_list { del($$); }

%type sem_var_list  { VarSigVecVec* }
%destructor sem_var_list  { del($$); }

%type priolit { Optimize* }
%destructor priolit { del($$); }

%type weightedpriolit { Optimize* }
%destructor weightedpriolit { del($$); }

%type computelit { Compute* }
%destructor computelit { del($$); }

%type head         { Lit* }
%type lit          { Lit* }
%type literal      { Lit* }
%type body_literal { Lit* }
%destructor head         { del($$); }
%destructor lit          { del($$); }
%destructor literal      { del($$); }
%destructor body_literal { del($$); }

%type predicate { PredLit* }
%type predlit   { PredLit* }
%destructor predlit   { del($$); }
%destructor predicate { del($$); }

%type cmp { RelLit::Type }

%type body  { LitPtrVec* }
%type nbody { LitPtrVec* }
%type cond  { LitPtrVec* }
%destructor body  { del($$); }
%destructor nbody { del($$); }
%destructor cond  { del($$); }

%type op { MathTerm::Func* }
%destructor op { del($$); }

%type term { Term* }
%destructor term { del($$); }

%type termlist  { TermPtrVec* }
%destructor termlist { del($$); }

%type weightlist  { CondLitVec* }
%type nweightlist { CondLitVec* }
%destructor weightlist  { del($$); }
%destructor nweightlist { del($$); }

%type weightlit { CondLit* }
%destructor weightlit { del($$); }

%type condlist  { CondLitVec* }
%type ncondlist { CondLitVec* }
%type ccondlist { CondLitVec* }
%type head_ccondlist { CondLitVec* }
%destructor condlist  { del($$); }
%destructor ncondlist { del($$); }
%destructor ccondlist { del($$); }
%destructor head_ccondlist { del($$); }

%type condlit { CondLit* }
%type ccondlit { CondLit* }
%type head_ccondlit { CondLit* }
%type head_ccondlit_cond { CondLit* }
%type head_ccondlit_nocond { CondLit* }
%destructor condlit { del($$); }
%destructor ccondlit { del($$); }
%destructor head_ccondlit { del($$); }
%destructor head_ccondlit_cond { del($$); }
%destructor head_ccondlit_nocond { del($$); }

%type weightcond { LitPtrVec* }
%type nweightcond { LitPtrVec* }
%type priolit_cond { LitPtrVec* }
%type npriolit_cond { LitPtrVec* }
%destructor weightcond { del($$); }
%destructor nweightcond { del($$); }
%destructor priolit_cond { del($$); }
%destructor npriolit_cond { del($$); }

%type aggr      { AggrLit* }
%type aggr_ass  { AggrLit* }
%type aggr_num  { AggrLit* }
%type aggr_atom { AggrLit* }
%type conjunction { AggrLit* }
%type disjunction { AggrLit* }
%destructor aggr      { del($$); }
%destructor aggr_ass  { del($$); }
%destructor aggr_num  { del($$); }
%destructor aggr_atom { del($$); }
%destructor conjunction { del($$); }
%destructor disjunction { del($$); }

%left SEM.
%left DOTS.
%left XOR.
%left QUESTION.
%left AND.
%left PLUS MINUS.
%left MULT SLASH MOD PMOD DIV PDIV.
%right POW PPOW.
%left UMINUS UBNOT.

%left DSEM.
%left COMMA.
%left VBAR.

start ::= program.

program ::= .
program ::= program line DOT.

line ::= INCLUDE STRING(file).                         { pParser->include(file.index); }
line ::= rule(r).                                      { pParser->add(r); }
line ::= HIDE.                                         { OUT->show(false); }
line ::= SHOW.                                         { OUT->show(true); }
line ::= HIDE signed(id) SLASH NUMBER(num).            { OUT->show(id.index, num.number, false); }
line ::= HIDE(tok) predicate(pred) cond(list).         { pParser->add(new Display(tok.loc(), false, pred, *list)); delete list; }
line ::= SHOW signed(id) SLASH NUMBER(num).            { OUT->show(id.index, num.number, true); }
line ::= SHOW(tok) predicate(pred) cond(list).         { pParser->add(new Display(tok.loc(), true, pred, *list)); delete list; }
line ::= CONST IDENTIFIER(id) ASSIGN term(term).       { pParser->constTerm(id.index, term); }
line ::= EXTERNAL(tok) predicate(pred) cond(list).     { pParser->add(new External(tok.loc(), pred, *list)); delete list; }
line ::= EXTERNAL signed(id) SLASH NUMBER(num).        { GRD->externalStm(id.index, num.number); }
line ::= CUMULATIVE IDENTIFIER(id).                    { pParser->incremental(Parser::IPART_CUMULATIVE, id.index); }
line ::= VOLATILE IDENTIFIER(id).                      { pParser->incremental(Parser::IPART_VOLATILE, id.index); }
line ::= BASE.                                         { pParser->incremental(Parser::IPART_BASE); }
line ::= optimize.
line ::= compute.
line ::= DOMAIN signed(id) LBRAC sem_var_list(list) RBRAC. {
	foreach (VarSigVec &x, *list) {
		pParser->domainStm(id.loc(), id.index, x);
	}
	del(list);
}

signed(res) ::= IDENTIFIER(id).              { res = id; }
signed(res) ::= MINUS(minus) IDENTIFIER(id). { res = minus; res.index = GRD->index(std::string("-") + GRD->string(id.index)); }

cond(res) ::= .                              { res = new LitPtrVec(); }
cond(res) ::= COLON literal(lit) cond(list). { res = list; res->push_back(lit); }

var_list(res) ::= VARIABLE(var).                      { res = new VarSigVec(); res->push_back(VarSig(var.loc(), var.index)); }
var_list(res) ::= var_list(list) COMMA VARIABLE(var). { list->push_back(VarSig(var.loc(), var.index)); res = list; }

sem_var_list(res) ::= var_list(x).                        { res = new VarSigVecVec(); res->push_back(*x); del(x); }
sem_var_list(res) ::= sem_var_list(list) SEM var_list(x). { res = list;               res->push_back(*x); del(x); }

rule(res) ::= head(head) IF body(body). { res = new Rule(head->loc(), head, *body); del(body); }
rule(res) ::= IF(tok) body(body).       { res = new Rule(tok.loc(), 0, *body); del(body); }
rule(res) ::= head(head).               { LitPtrVec v; res = new Rule(head->loc(), head, v); }

head(res) ::= predicate(pred).  { res = pred; }
head(res) ::= aggr_atom(lit).   { res = lit; }
head(res) ::= disjunction(lit). { res = lit; }

nbody(res) ::= body_literal(lit).                   { res = vec1(lit); }
nbody(res) ::= nbody(body) COMMA body_literal(lit). { res = body; res->push_back(lit); }

body(res) ::= .            { res = new LitPtrVec(); }
body(res) ::= nbody(body). { res = body; }

predlit(res) ::= predicate(pred).          { res = pred; }
predlit(res) ::= NOT(tok) predicate(pred). { res = pred; pred->sign(true); pred->loc(tok.loc()); }

lit(res) ::= predlit(pred).            { res = pred; }
lit(res) ::= term(a) cmp(cmp) term(b). { res = new RelLit(a->loc(), cmp, a, b); }

literal(res) ::= lit(lit).                     { res = lit; }
literal(res) ::= VARIABLE(var) ASSIGN term(b). { res = new RelLit(var.loc(), RelLit::ASSIGN, new VarTerm(var.loc(), var.index), b); }
literal(res) ::= term(a) CASSIGN term(b).      { res = new RelLit(a->loc(), RelLit::ASSIGN, a, b); }

body_literal(res) ::= literal(lit).                       { res = lit; }
body_literal(res) ::= aggr_atom(lit).                     { res = lit; }
body_literal(res) ::= NOT aggr_atom(lit).                 { res = lit; lit->sign(true); }
body_literal(res) ::= VARIABLE(var) ASSIGN aggr_ass(lit). { res = lit; lit->assign(new VarTerm(var.loc(), var.index)); }
body_literal(res) ::= term(a) CASSIGN aggr_ass(lit).      { res = lit; lit->assign(a); }
body_literal(res) ::= conjunction(lit).                   { res = lit; }

cmp(res) ::= GREATER. { res = RelLit::GREATER; }
cmp(res) ::= LOWER.   { res = RelLit::LOWER; }
cmp(res) ::= GTHAN.   { res = RelLit::GTHAN; }
cmp(res) ::= LTHAN.   { res = RelLit::LTHAN; }
cmp(res) ::= EQUAL.   { res = RelLit::EQUAL; }
cmp(res) ::= INEQUAL. { res = RelLit::INEQUAL; }

term(res) ::= VARIABLE(var).  { res = new VarTerm(var.loc(), var.index); }
term(res) ::= IDENTIFIER(id). { res = pParser->term(Val::ID, id.loc(), id.index); }
term(res) ::= STRING(id).     { res = pParser->term(Val::STRING, id.loc(), id.index); }
term(res) ::= NUMBER(num).    { res = new ConstTerm(num.loc(), Val::create(Val::NUM, num.number)); }
term(res) ::= ANONYMOUS(var). { res = new VarTerm(var.loc()); }
term(res) ::= INFIMUM(inf).   { res = new ConstTerm(inf.loc(), Val::create(Val::INF, 0)); }
term(res) ::= SUPREMUM(sup).  { res = new ConstTerm(sup.loc(), Val::create(Val::SUP, 0)); }

term(res) ::= term(a) DOTS term(b).                         { res = new RangeTerm(a->loc(), a, b); }
term(res) ::= term(a) SEM term(b).                          { res = new PoolTerm(a->loc(), a, b); }
term(res) ::= term(a) PLUS term(b).                         { res = new MathTerm(a->loc(), MathTerm::PLUS, a, b); }
term(res) ::= term(a) MINUS term(b).                        { res = new MathTerm(a->loc(), MathTerm::MINUS, a, b); }
term(res) ::= term(a) MULT term(b).                         { res = new MathTerm(a->loc(), MathTerm::MULT, a, b); }
term(res) ::= term(a) SLASH term(b).                        { res = new MathTerm(a->loc(), MathTerm::DIV, a, b); }
term(res) ::= term(a) DIV term(b).                          { res = new MathTerm(a->loc(), MathTerm::DIV, a, b); }
term(res) ::= term(a) PDIV term(b).                         { res = new MathTerm(a->loc(), MathTerm::DIV, a, b); }
term(res) ::= term(a) MOD term(b).                          { res = new MathTerm(a->loc(), MathTerm::MOD, a, b); }
term(res) ::= term(a) PMOD term(b).                         { res = new MathTerm(a->loc(), MathTerm::MOD, a, b); }
term(res) ::= term(a) POW term(b).                          { res = new MathTerm(a->loc(), MathTerm::POW, a, b); }
term(res) ::= term(a) PPOW term(b).                         { res = new MathTerm(a->loc(), MathTerm::POW, a, b); }
term(res) ::= term(a) AND term(b).                          { res = new MathTerm(a->loc(), MathTerm::AND, a, b); }
term(res) ::= term(a) XOR term(b).                          { res = new MathTerm(a->loc(), MathTerm::XOR, a, b); }
term(res) ::= term(a) QUESTION term(b).                     { res = new MathTerm(a->loc(), MathTerm::OR, a, b); }
term(res) ::= PABS LBRAC term(a) RBRAC.                     { res = new MathTerm(a->loc(), MathTerm::ABS, a); }
term(res) ::= VBAR term(a) VBAR.                            { res = new MathTerm(a->loc(), MathTerm::ABS, a); }
term(res) ::= PPOW LBRAC term(a) COMMA term(b) RBRAC.       { res = new MathTerm(a->loc(), MathTerm::POW, a, b); }
term(res) ::= PMOD LBRAC term(a) COMMA term(b) RBRAC.       { res = new MathTerm(a->loc(), MathTerm::MOD, a, b); }
term(res) ::= PDIV LBRAC term(a) COMMA term(b) RBRAC.       { res = new MathTerm(a->loc(), MathTerm::DIV, a, b); }
term(res) ::= IDENTIFIER(id) LBRAC termlist(args) RBRAC.    { res = new FuncTerm(id.loc(), id.index, *args); delete args; }
term(res) ::= LBRAC(l) termlist(args) RBRAC.                { res = args->size() == 1 ? args->pop_back().release() : new FuncTerm(l.loc(), GRD->index(""), *args); delete args; }
term(res) ::= LBRAC(l) termlist(args) COMMA RBRAC.          { res = new FuncTerm(l.loc(), GRD->index(""), *args); delete args; }
term(res) ::= AT IDENTIFIER(id) LBRAC termlist(args) RBRAC. { res = new LuaTerm(id.loc(), id.index, *args); delete args; }
term(res) ::= AT IDENTIFIER(id) LBRAC RBRAC.                { TermPtrVec args; res = new LuaTerm(id.loc(), id.index, args); }
term(res) ::= MINUS(m) term(a). [UMINUS]                    { res = new MathTerm(m.loc(), MathTerm::MINUS, ZERO(m.loc()), a); }
term(res) ::= BNOT(m) term(a). [UBNOT]                      { res = new MathTerm(m.loc(), MathTerm::XOR, MINUSONE(m.loc()), a); }

nweightlist(res) ::= weightlit(lit).                         { res = vec1(lit); }
nweightlist(res) ::= nweightlist(list) COMMA weightlit(lit). { res = list; res->push_back(lit); }
weightlist(res)  ::= .                  { res = new CondLitVec(); }
weightlist(res)  ::= nweightlist(list). { res = list; }

weightlit(res) ::= lit(lit) nweightcond(cond) ASSIGN term(weight). { res = new CondLit(lit->loc(), lit, weight, *cond); delete cond; }
weightlit(res) ::= lit(lit) ASSIGN term(weight) weightcond(cond).  { res = new CondLit(lit->loc(), lit, weight, *cond); delete cond; }
weightlit(res) ::= lit(lit) weightcond(cond).                      { res = new CondLit(lit->loc(), lit, ONE(lit->loc()), *cond); delete cond; }

nweightcond(res) ::= COLON literal(lit).                   { res = vec1(lit); }
nweightcond(res) ::= COLON literal(lit) nweightcond(list). { res = list; res->push_back(lit); }

weightcond(res) ::= .                  { res = new LitPtrVec(); }
weightcond(res) ::= nweightcond(list). { res = list; }

aggr_ass(res) ::= SUM(tok) LSBRAC weightlist(list) RSBRAC. { res = new SumAggrLit(tok.loc(), *list, false); delete list; }
aggr_ass(res) ::= LSBRAC(tok) weightlist(list) RSBRAC.     { res = new SumAggrLit(tok.loc(), *list, false); delete list; }

aggr_num(res) ::= AVG(tok) LSBRAC weightlist(list) RSBRAC. { res = new AvgAggrLit(tok.loc(), *list); delete list; }

ncondlist(res) ::= condlit(lit).                       { res = vec1(lit); }
ncondlist(res) ::= ncondlist(list) COMMA condlit(lit). { res = list; res->push_back(lit); }
condlist(res)  ::= .                { res = new CondLitVec(); }
condlist(res)  ::= ncondlist(list). { res = list; }

condlit(res) ::= lit(lit) weightcond(cond). { res = new CondLit(lit->loc(), lit, ONE(lit->loc()), *cond); delete cond; }

aggr_ass(res) ::= COUNT(tok) LCBRAC condlist(list) RCBRAC. { res = new SumAggrLit(tok.loc(), *list, true); delete list; }
aggr_ass(res) ::= LCBRAC(tok) condlist(list) RCBRAC.       { res = new SumAggrLit(tok.loc(), *list, true); delete list; }

aggr(res) ::= EVEN(tok) LCBRAC condlist(list) RCBRAC. { res = new ParityAggrLit(tok.loc(), *list, true, true); delete list; }
aggr(res) ::= ODD(tok)  LCBRAC condlist(list) RCBRAC. { res = new ParityAggrLit(tok.loc(), *list, false, true); delete list; }
aggr(res) ::= EVEN(tok) LSBRAC weightlist(list) RSBRAC. { res = new ParityAggrLit(tok.loc(), *list, true, false); delete list; }
aggr(res) ::= ODD(tok)  LSBRAC weightlist(list) RSBRAC. { res = new ParityAggrLit(tok.loc(), *list, false, false); delete list; }

ccondlit(res) ::= lit(lit) nweightcond(cond). { res = new CondLit(lit->loc(), lit, ONE(lit->loc()), *cond); delete cond; }
ccondlist(res) ::= ccondlit(lit). { res = new CondLitVec(); res->push_back(lit); }

conjunction(res) ::= ccondlist(list). { res = new JunctionAggrLit(list->at(0).loc(), *list); delete list; }

head_ccondlit_nocond(res) ::= predicate(lit).                   { LitPtrVec cond; res = new CondLit(lit->loc(), lit, ONE(lit->loc()), cond); }
head_ccondlit_cond(res)   ::= predicate(lit) nweightcond(cond). { res = new CondLit(lit->loc(), lit, ONE(lit->loc()), *cond); delete cond; }
head_ccondlit(res) ::= head_ccondlit_nocond(lit). { res = lit; }
head_ccondlit(res) ::= head_ccondlit_cond(lit).   { res = lit; }

head_ccondlist(res) ::= head_ccondlit_cond(lit).                            { res = new CondLitVec(); res->push_back(lit); }
head_ccondlist(res) ::= head_ccondlit_nocond(lit) VBAR head_ccondlit(lit2). { res = new CondLitVec(); res->push_back(lit); res->push_back(lit2); }
head_ccondlist(res) ::= head_ccondlist(list) VBAR head_ccondlit(lit).       { res = list; res->push_back(lit); }

disjunction(res) ::= head_ccondlist(list). { res = new JunctionAggrLit(list->at(0).loc(), *list); delete list; }

aggr_ass(res) ::= MIN(tok) LSBRAC weightlist(list) RSBRAC. { res = new MinMaxAggrLit(tok.loc(), *list, false); delete list; }
aggr_ass(res) ::= MAX(tok) LSBRAC weightlist(list) RSBRAC. { res = new MinMaxAggrLit(tok.loc(), *list, true); delete list; }

aggr_num(res) ::= aggr_ass(lit). { res = lit; }
aggr(res)     ::= aggr_num(lit). { res = lit; }

aggr_atom(res) ::= term(l) aggr_num(aggr) term(u). { res = aggr; res->lower(l); res->upper(u); }
aggr_atom(res) ::= aggr_num(aggr) term(u).         { res = aggr; res->upper(u); }
aggr_atom(res) ::= term(l) aggr_num(aggr).         { res = aggr; res->lower(l); }
aggr_atom(res) ::= aggr(aggr).                     { res = aggr; }

predicate(res) ::= MINUS(sign) IDENTIFIER(id) LBRAC termlist(terms) RBRAC. { res = pParser->predLit(sign.loc(), id.index, *terms, true); delete terms; }
predicate(res) ::= IDENTIFIER(id) LBRAC termlist(terms) RBRAC.             { res = pParser->predLit(id.loc(), id.index, *terms, false); delete terms; }
predicate(res) ::= MINUS(sign) IDENTIFIER(id).                             { TermPtrVec terms; res = pParser->predLit(sign.loc(), id.index, terms, true); }
predicate(res) ::= IDENTIFIER(id).                                         { TermPtrVec terms; res = pParser->predLit(id.loc(), id.index, terms, false); }

termlist(res) ::= term(term).                                { res = vec1(term); }
termlist(res) ::= termlist(list) COMMA term(term).           { res = list; list->push_back(term); }
termlist(res) ::= termlist(list) DSEM(dsem) termlist(terms). { res = list; list->push_back(new ArgTerm(dsem.loc(), res->pop_back().release(), *terms)); delete terms; }

optimize ::= soptimize LSBRAC prio_list RSBRAC. { pParser->nextLevel(); }
optimize ::= soptimize_set LCBRAC prio_set RCBRAC. { pParser->nextLevel(); }

soptimize ::= MINIMIZE. { pParser->maximize(false); pParser->optimizeSet(false); }
soptimize ::= MAXIMIZE. { pParser->maximize(true); pParser->optimizeSet(false); }
soptimize_set ::= MINIMIZE. { pParser->maximize(false); pParser->optimizeSet(true); }
soptimize_set ::= MAXIMIZE. { pParser->maximize(true); pParser->optimizeSet(true); }

prio_list ::= .
prio_list ::= nprio_list.

nprio_list ::= weightedpriolit(lit).                 { pParser->add(lit); }
nprio_list ::= weightedpriolit(lit) COMMA prio_list. { pParser->add(lit); }


prio_set ::= .
prio_set ::= nprio_set.

nprio_set ::= priolit(lit).                { pParser->add(lit); }
nprio_set ::= priolit(lit) COMMA prio_set. { pParser->add(lit); }

weightedpriolit(res) ::= predlit(head) ASSIGN term(weight) AT term(prio) priolit_cond(body).  { res = new Optimize(head->loc(), head, weight, prio, *body, pParser->maximize()); pParser->setUniques(res); del(body); }
weightedpriolit(res) ::= predlit(head) ASSIGN term(weight) priolit_cond(body).                { res = new Optimize(head->loc(), head, weight, new ConstTerm(head->loc(), Val::create(Val::NUM, pParser->level())), *body, pParser->maximize()); pParser->setUniques(res); del(body); }
weightedpriolit(res) ::= predlit(head) npriolit_cond(body) ASSIGN term(weight) AT term(prio). { res = new Optimize(head->loc(), head, weight, prio, *body, pParser->maximize()); pParser->setUniques(res); del(body); }
weightedpriolit(res) ::= predlit(head) npriolit_cond(body) ASSIGN term(weight).               { res = new Optimize(head->loc(), head, weight, new ConstTerm(head->loc(), Val::create(Val::NUM, pParser->level())), *body, pParser->maximize()); pParser->setUniques(res); del(body); }
weightedpriolit(res) ::= priolit(lit). { res = lit; }

priolit(res) ::= predlit(head) AT term(prio) priolit_cond(body).  { res = new Optimize(head->loc(), head, ONE(head->loc()), prio, *body, pParser->maximize()); pParser->setUniques(res); del(body); }
priolit(res) ::= predlit(head) npriolit_cond(body) AT term(prio). { res = new Optimize(head->loc(), head, ONE(head->loc()), prio, *body, pParser->maximize()); pParser->setUniques(res); del(body); }
priolit(res) ::= predlit(head) priolit_cond(body).                { res = new Optimize(head->loc(), head, ONE(head->loc()), new ConstTerm(head->loc(), Val::create(Val::NUM, pParser->level())), *body, pParser->maximize()); pParser->setUniques(res); del(body); }

npriolit_cond(res) ::= COLON literal(lit).                     { res = vec1(lit); }
npriolit_cond(res) ::= npriolit_cond(list) COLON literal(lit). { res = list; list->push_back(lit); }

priolit_cond(res) ::= .                    { res = new LitPtrVec(); }
priolit_cond(res) ::= npriolit_cond(list). { res = list; }

compute ::= COMPUTE LCBRAC compute_list RCBRAC.
compute ::= COMPUTE NUMBER LCBRAC compute_list RCBRAC.

compute_list ::= .
compute_list ::= ncompute_list.

ncompute_list ::= computelit(lit).                     { pParser->add(lit); }
ncompute_list ::= computelit(lit) COMMA ncompute_list. { pParser->add(lit); }

computelit(res) ::= predlit(head) priolit_cond(body). { res = new Compute(head->loc(), head, *body); del(body); }
