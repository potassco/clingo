// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

%include {

#include <gringo/gringo.h>

#include <gringo/lparseconverter.h>

#include <gringo/output.h>
#include <gringo/value.h>
#include <gringo/funcsymbol.h>

using namespace gringo;
using namespace NS_OUTPUT;

typedef std::pair<ObjectVector, IntVector> WeightList;
typedef std::pair<Object*, int> WeightLit;

#define STRING(x) (pConverter->createString(x))
#define OUTPUT (pConverter->getOutput())
#define FUNCSYM(x) (pConverter->createFuncSymbol(x))
#define PRED(x,a) (pConverter->createPred(x,a))
#define DELETE_PTR(X) { if(X) delete (X); }
#define DELETE_PTRVECTOR(T, X) { if(X){ for(T::iterator it = (X)->begin(); it != (X)->end(); it++) delete (*it); delete (X); } }

Object *createDisjunction(Object *disj, Object *pred)
{
	if(dynamic_cast<Disjunction*>(disj))
	{
		static_cast<Disjunction*>(disj)->lits_.push_back(pred);
	}
	else
	{
		Disjunction *a = new Disjunction();
		a->lits_.push_back(disj);
		a->lits_.push_back(pred);
		disj = a;
	}
	return disj;
}

}

%name lparseconverter
%stack_size 0
%token_prefix LPARSECONVERTER_

%extra_argument { LparseConverter *pConverter }

%parse_failure {
	pConverter->handleError();
}

%syntax_error {
	pConverter->handleError();
}

// token type/destructor for terminals
%token_type { std::string* }
%token_destructor { DELETE_PTR($$) }

%type variable_list  { int }

%type body { Conjunction* }
%destructor body { DELETE_PTR($$) }

%type body_literal       { Object* }
%type constraint_literal { Object* }
%type body_atom          { Object* }
%type head_atom          { Object* }
%type disjunction        { Object* }
%type predicate          { Object* }
%type aggregate_atom     { Object* }
%type compute            { Object* }
%type minimize           { Object* }
%type maximize           { Object* }
%destructor body_literal       { DELETE_PTR($$) }
%destructor constraint_literal { DELETE_PTR($$) }
%destructor body_atom          { DELETE_PTR($$) }
%destructor head_atom          { DELETE_PTR($$) }
%destructor disjunction        { DELETE_PTR($$) }
%destructor predicate          { DELETE_PTR($$) }
%destructor aggregate_atom     { DELETE_PTR($$) }
%destructor compute            { DELETE_PTR($$) }
%destructor minimize           { DELETE_PTR($$) }
%destructor maximize           { DELETE_PTR($$) }

%type constant { Value* }
%destructor constant { DELETE_PTR($$) }

%type aggregate { Aggregate* }
%destructor aggregate { DELETE_PTR($$) }

%type constant_list  { ValueVector* }
%destructor constant_list  { DELETE_PTR($$) }

%type number { int }
%destructor number { }

%type weight_literal { WeightLit* }
%destructor weight_literal { DELETE_PTR($$) }

%type weight_list    { WeightList* }
%type nweight_list   { WeightList* }
%type constr_list    { WeightList* }
%type nconstr_list   { WeightList* }
%destructor weight_list    { DELETE_PTR($$) }
%destructor nweight_list   { DELETE_PTR($$) }
%destructor constr_list    { DELETE_PTR($$) }
%destructor nconstr_list   { DELETE_PTR($$) }

%type neg_pred { std::string* }
%destructor neg_pred { DELETE_PTR($$) }

// this will define the symbols in the header
// even though they are not used in the rules
%nonassoc ERROR EOI.

%start_symbol start

start ::= header program.

program ::= program rule DOT.
program ::= . { OUTPUT->initialize(pConverter, pConverter->getPred()); }

header ::= header SHOW show_list DOT.
header ::= header HIDE hide_list DOT.
header ::= .

show_list ::= show_list COMMA show_predicate.
show_list ::= show_predicate.

hide_list ::= . { OUTPUT->hideAll(); }
hide_list ::= nhide_list. 

nhide_list ::= nhide_list COMMA hide_predicate.
nhide_list ::= hide_predicate.

neg_pred(res) ::= IDENTIFIER(id).       { res = id; }
neg_pred(res) ::= MINUS IDENTIFIER(id). { id->insert(id->begin(), '-'); res = id; }

show_predicate ::= neg_pred(id). { OUTPUT->setVisible(STRING(id), 0, true); }
hide_predicate ::= neg_pred(id). { OUTPUT->setVisible(STRING(id), 0, false); }
show_predicate ::= neg_pred(id) SLASH NUMBER(n). { OUTPUT->setVisible(STRING(id), atol(n->c_str()), true); DELETE_PTR(n); }
hide_predicate ::= neg_pred(id) SLASH NUMBER(n). { OUTPUT->setVisible(STRING(id), atol(n->c_str()), false); DELETE_PTR(n); }
show_predicate ::= neg_pred(id) LPARA variable_list(count) RPARA. { OUTPUT->setVisible(STRING(id), count, true); }
hide_predicate ::= neg_pred(id) LPARA variable_list(count) RPARA. { OUTPUT->setVisible(STRING(id), count, false); }

variable_list(res) ::= variable_list(list) COMMA VARIABLE. { res = list + 1; }
variable_list(res) ::= VARIABLE.                           { res = 1; }

rule ::= head_atom(head) IF body(body). { Rule r(head, body); OUTPUT->print(&r); }
rule ::= head_atom(head) IF .           { Fact r(head); OUTPUT->print(&r); }
rule ::= head_atom(head).               { Fact r(head); OUTPUT->print(&r); }
rule ::= IF body(body).                 { Integrity r(body); OUTPUT->print(&r); }
rule ::= IF.                            { Integrity r(new Conjunction()); OUTPUT->print(&r); }
rule ::= maximize(r).                   { OUTPUT->print(r); DELETE_PTR(r); }
rule ::= minimize(r).                   { OUTPUT->print(r); DELETE_PTR(r); }
rule ::= compute(r).                    { OUTPUT->print(r); DELETE_PTR(r); }

body(res) ::= body(body) COMMA body_literal(lit). { res = body; res->lits_.push_back(lit); }
body(res) ::= body_literal(lit).                  { res = new Conjunction(); res->lits_.push_back(lit); }

body_literal(res) ::= body_atom(atom).          { res = atom; }
body_literal(res) ::= NOT body_atom(atom).      { res = atom; res->neg_ = true; }
body_literal(res) ::= aggregate_atom(aggr).     { res = aggr; }
body_literal(res) ::= NOT aggregate_atom(aggr). { res = aggr; res->neg_ = true; }

constraint_literal(res) ::= predicate(pred).     { res = pred; }
constraint_literal(res) ::= NOT predicate(pred). { res = pred; res->neg_ = true; }

body_atom(res) ::= predicate(pred).      { res = pred; }

head_atom(res) ::= aggregate_atom(aggr). { res = aggr; }
head_atom(res) ::= disjunction(disj).    { res = disj; }

disjunction(res) ::= disjunction(list) DISJUNCTION predicate(pred). { res = createDisjunction(list, pred); }
disjunction(res) ::= predicate(pred).                               { res = pred; }

predicate(res) ::= IDENTIFIER(id) LPARA constant_list(list) RPARA. { res = new Atom(false, PRED(STRING(id), list->size()), *list); DELETE_PTR(list); }
predicate(res) ::= IDENTIFIER(id).                                 { res = new Atom(false, PRED(STRING(id), 0)); }
predicate(res) ::= MINUS IDENTIFIER(id) LPARA constant_list(list) RPARA.
  { id->insert(id->begin(), '-'); res = new Atom(false, PRED(STRING(id), list->size()), *list); DELETE_PTR(list); }
predicate(res) ::= MINUS IDENTIFIER(id).
  { id->insert(id->begin(), '-'); res = new Atom(false, PRED(STRING(id), 0)); }

aggregate_atom(res) ::= number(l) aggregate(aggr) number(u). { res = aggr; aggr->bounds_ = Aggregate::LU; aggr->lower_ = l; aggr->upper_ = u; }
aggregate_atom(res) ::= aggregate(aggr) number(u).           { res = aggr; aggr->bounds_ = Aggregate::U; aggr->upper_ = u; }
aggregate_atom(res) ::= number(l) aggregate(aggr).           { res = aggr; aggr->bounds_ = Aggregate::L; aggr->lower_ = l; }
aggregate_atom(res) ::= aggregate(aggr).                     { res = aggr; }

aggregate_atom(res) ::= EVEN LBRAC constr_list(list) RBRAC.  { res = new Aggregate(false, Aggregate::PARITY, 0, list->first, list->second, 0); DELETE_PTR(list); }
aggregate_atom(res) ::= ODD LBRAC constr_list(list) RBRAC.   { res = new Aggregate(false, Aggregate::PARITY, 1, list->first, list->second, 1); DELETE_PTR(list); }

constant_list(res) ::= constant_list(list) COMMA constant(val). { res = list; res->push_back(*val); DELETE_PTR(val); }
constant_list(res) ::= constant(val).                           { res = new ValueVector(); res->push_back(*val); DELETE_PTR(val); }

constant(res) ::= IDENTIFIER(id). { res = new Value(Value::STRING, STRING(id)); }
constant(res) ::= number(n).      { res = new Value(Value::INT, n); }
constant(res) ::= STRING(id).     { res = new Value(Value::STRING, STRING(id)); }
constant(res) ::= IDENTIFIER(id) LPARA constant_list(list) RPARA. { res = new Value(Value::FUNCSYMBOL, FUNCSYM(new FuncSymbol(STRING(id), *list))); DELETE_PTR(list); }

aggregate(res) ::= TIMES LSBRAC weight_list(list) RSBRAC. { res = new Aggregate(false, Aggregate::TIMES, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= AVG LSBRAC weight_list(list) RSBRAC.   { res = new Aggregate(false, Aggregate::AVG, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= SUM LSBRAC weight_list(list) RSBRAC.   { res = new Aggregate(false, Aggregate::SUM, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= MIN LSBRAC weight_list(list) RSBRAC.   { res = new Aggregate(false, Aggregate::MIN, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= MAX LSBRAC weight_list(list) RSBRAC.   { res = new Aggregate(false, Aggregate::MAX, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= COUNT LBRAC constr_list(list) RBRAC.   { res = new Aggregate(false, Aggregate::COUNT, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= LSBRAC weight_list(list) RSBRAC.       { res = new Aggregate(false, Aggregate::SUM, list->first, list->second); DELETE_PTR(list); }
aggregate(res) ::= LBRAC constr_list(list) RBRAC.         { res = new Aggregate(false, Aggregate::COUNT, list->first, list->second); DELETE_PTR(list); }

compute(res)  ::= COMPUTE LBRAC constr_list(list) RBRAC.           { res = new Compute(list->first, 1); DELETE_PTR(list); }
compute(res)  ::= COMPUTE number(n) LBRAC constr_list(list) RBRAC. { res = new Compute(list->first, n); DELETE_PTR(list); }
minimize(res) ::= MINIMIZE LBRAC  constr_list(list) RBRAC.  { res = new Optimize(Optimize::MINIMIZE, list->first, list->second); DELETE_PTR(list); }
minimize(res) ::= MINIMIZE LSBRAC weight_list(list) RSBRAC. { res = new Optimize(Optimize::MINIMIZE, list->first, list->second); DELETE_PTR(list); }
maximize(res) ::= MAXIMIZE LBRAC  constr_list(list) RBRAC.  { res = new Optimize(Optimize::MAXIMIZE, list->first, list->second); DELETE_PTR(list); }
maximize(res) ::= MAXIMIZE LSBRAC weight_list(list) RSBRAC. { res = new Optimize(Optimize::MAXIMIZE, list->first, list->second); DELETE_PTR(list); }

weight_list(res) ::= nweight_list(list). { res = list; }
weight_list(res) ::= .                   { res = new WeightList(); }
nweight_list(res) ::= nweight_list(list) COMMA weight_literal(lit).
  { res = list; res->first.push_back(lit->first); res->second.push_back(lit->second); DELETE_PTR(lit); }
nweight_list(res) ::= weight_literal(lit).
  { res = new WeightList(); res->first.push_back(lit->first); res->second.push_back(lit->second); DELETE_PTR(lit); }


weight_literal(res) ::= constraint_literal(lit) ASSIGN number(n). { res = new WeightLit(lit, n); }
weight_literal(res) ::= constraint_literal(lit).                  { res = new WeightLit(lit, 1); }

number(res) ::= NUMBER(n).       { res = atol(n->c_str()); DELETE_PTR(n); }
number(res) ::= MINUS NUMBER(n). { res = -atol(n->c_str()); DELETE_PTR(n); }

constr_list(res) ::= nconstr_list(list). { res = list; }
constr_list(res) ::= .                   { res = new WeightList(); }
nconstr_list(res) ::= nconstr_list(list) COMMA constraint_literal(lit). { res = list; res->first.push_back(lit); res->second.push_back(1); }
nconstr_list(res) ::= constraint_literal(lit).                          { res = new WeightList(); res->first.push_back(lit); res->second.push_back(1); }

