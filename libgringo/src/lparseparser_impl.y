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

#include <gringo/lparseparser.h>

// literals
#include <gringo/literal.h>
#include <gringo/aggregateliteral.h>  
#include <gringo/predicateliteral.h>
#include <gringo/conditionalliteral.h>
#include <gringo/relationliteral.h>
#include <gringo/assignmentliteral.h>

// miscellaneous literal
#include <gringo/computeliteral.h>
#include <gringo/optimizeliteral.h>

// aggregates
#include <gringo/sumaggregate.h>  
#include <gringo/minaggregate.h>  
#include <gringo/maxaggregate.h>  
#include <gringo/countaggregate.h>  
#include <gringo/avgaggregate.h>
#include <gringo/timesaggregate.h>
#include <gringo/disjunctionaggregate.h>
#include <gringo/conjunctionaggregate.h>
#include <gringo/parityaggregate.h>

// terms
#include <gringo/term.h>
#include <gringo/constant.h>
#include <gringo/variable.h>
#include <gringo/functionterm.h>
#include <gringo/funcsymbolterm.h>
#include <gringo/rangeterm.h>
#include <gringo/multipleargsterm.h>

// statements
#include <gringo/normalrule.h>
#include <gringo/literalstatement.h>

// misc
#include <gringo/grounder.h>

using namespace gringo;

#define OUTPUT (pParser->getGrounder()->getOutput())
#define GROUNDER (pParser->getGrounder())
#define STRING(x) (pParser->getGrounder()->createString(x))
#define DELETE_PTR(X) { if(X) delete (X); }
#define DELETE_PTRVECTOR(T, X) { if(X){ for(T::iterator it = (X)->begin(); it != (X)->end(); it++) delete (*it); delete (X); } }
}  

%name lparseparser
%stack_size 0
%token_prefix LPARSEPARSER_

%extra_argument { LparseParser *pParser }

%parse_failure {
	pParser->handleError();
}

%syntax_error {
	pParser->handleError();
}

// token type/destructor for terminals
%token_type { std::string* }
%token_destructor { DELETE_PTR($$) }

// token types/destructors for nonterminals
%type rule     { Statement* }
%type minimize { Statement* }
%type maximize { Statement* }
%type compute  { Statement* }
%destructor rule     { DELETE_PTR($$) }
%destructor minimze  { DELETE_PTR($$) }
%destructor maximize { DELETE_PTR($$) }
%destructor compute  { DELETE_PTR($$) }

%type body  { LiteralVector* }
%destructor body { DELETE_PTR($$) }

%type conditional_list { LiteralVector* }
%destructor conditional_list { DELETE_PTRVECTOR(LiteralVector, $$) }

%type weight_term        { ConditionalLiteral* }
%type constr_term        { ConditionalLiteral* }
%type constraint_literal { ConditionalLiteral* }
%type constraint_atom    { ConditionalLiteral* }
%destructor constraint_literal { DELETE_PTR($$) }
%destructor constraint_atom    { DELETE_PTR($$) }
%destructor weight_term        { DELETE_PTR($$) }
%destructor constr_term        { DELETE_PTR($$) }

%type predicate { PredicateLiteral* }
%destructor predicate { DELETE_PTR($$) }

%type body_atom          { Literal* }
%type head_atom          { Literal* }
%type body_literal       { Literal* }
%type relation_literal   { Literal* }
%type conditional        { Literal* }
%type disjunction        { Literal* }

%destructor body_atom        { DELETE_PTR($$) }
%destructor head_atom        { DELETE_PTR($$) }
%destructor body_literal     { DELETE_PTR($$) }
%destructor relation_literal { DELETE_PTR($$) }
%destructor conditional      { DELETE_PTR($$) }
%destructor disjuntion       { DELETE_PTR($$) }


%type aggregate      { AggregateLiteral* }
%type aggregate_lu   { AggregateLiteral* }
%type aggregate_atom { AggregateLiteral* }
%destructor aggregate      { DELETE_PTR($$) }
%destructor aggregate_lu   { DELETE_PTR($$) }
%destructor aggregate_atom { DELETE_PTR($$) }

%type termlist       { TermVector* }
%type const_termlist { TermVector* }
%destructor termlist       { DELETE_PTRVECTOR(TermVector, $$) }
%destructor const_termlist { DELETE_PTRVECTOR(TermVector, $$) }

%type term       { Term* }
%type const_term { Term* }
%destructor term       { DELETE_PTR($$) }
%destructor const_term { DELETE_PTR($$) }

%type weight_list  { ConditionalLiteralVector* }
%type nweight_list { ConditionalLiteralVector* }
%type disj_list   { ConditionalLiteralVector* }
%type constr_list  { ConditionalLiteralVector* }
%type nconstr_list { ConditionalLiteralVector* }
%destructor disj_list    { DELETE_PTRVECTOR(ConditionalLiteralVector, $$) }
%destructor weight_list  { DELETE_PTRVECTOR(ConditionalLiteralVector, $$) }
%destructor nweight_list { DELETE_PTRVECTOR(ConditionalLiteralVector, $$) }
%destructor constr_list  { DELETE_PTRVECTOR(ConditionalLiteralVector, $$) }
%destructor nconstr_list { DELETE_PTRVECTOR(ConditionalLiteralVector, $$) }

%type variable_list { int }
%destructor variable_list { }

%type domain_var { IntVector* }
%destructor domain_var { DELETE_PTR($$) }

%type domain_list { std::vector<IntVector*>* }
%destructor domain_list { DELETE_PTRVECTOR(std::vector<IntVector*>, $$) }

%type neg_pred { std::string* }
%destructor neg_pred { DELETE_PTR($$) }

%left SEMI.
%left DOTS.
%left OR.
%left XOR.
%left AND.
%left PLUS MINUS.
%left TIMES DIVIDE MOD.
%right POWER.
%left UMINUS TILDE.

// this will define the symbols in the header 
// even though they are not used in the rules
%nonassoc ERROR EOI OR2.

%start_symbol start

start ::= program.

program ::= program BASE DOT.                  { GROUNDER->setIncPart(BASE, Value()); }
program ::= program LAMBDA IDENTIFIER(id) DOT. { GROUNDER->setIncPart(LAMBDA, Value(Value::STRING, STRING(id))); }
program ::= program DELTA IDENTIFIER(id) DOT.  { GROUNDER->setIncPart(DELTA, Value(Value::STRING, STRING(id))); }
program ::= program rule(rule) DOT.            { if(rule) pParser->getGrounder()->addStatement(rule); }
program ::= program SHOW show_list DOT.
program ::= program HIDE hide_list DOT.
program ::= program ISHIFT shift_list DOT.  
program ::= program DOMAIN domain_list DOT.
program ::= program CONST const_list DOT.
program ::= .

const_list ::= constid.
const_list ::= const_list COMMA constid.

constid ::= IDENTIFIER(id) ASSIGN const_term(term). { pParser->getGrounder()->setConstValue(STRING(id), term); }

shift_list ::= shift.
shift_list ::= shift_list COMMA shift.

shift ::= IDENTIFIER(id) DIVIDE NUMBER(n).                         { GROUNDER->setIncShift(*id, atol(n->c_str())); DELETE_PTR(n); DELETE_PTR(id); }
shift ::= IDENTIFIER(a) RIGHTARROW IDENTIFIER(b) DIVIDE NUMBER(n). { GROUNDER->setIncShift(*a, *b, atol(n->c_str())); DELETE_PTR(n); DELETE_PTR(a); DELETE_PTR(b); }

show_list ::= show_list COMMA show_predicate.
show_list ::= show_predicate.

hide_list ::= .           { OUTPUT->hideAll(); }
hide_list ::= nhide_list.

nhide_list ::= nhide_list COMMA hide_predicate.
nhide_list ::= hide_predicate.

domain_list ::= domain_list COMMA domain_predicate.
domain_list ::= domain_predicate.

neg_pred(res) ::= IDENTIFIER(id).       { res = id; }
neg_pred(res) ::= MINUS IDENTIFIER(id). { id->insert(id->begin(), '-'); res = id; }

show_predicate ::= neg_pred(id) LPARA variable_list(list) RPARA.   { OUTPUT->setVisible(STRING(id), list, true); }
show_predicate ::= neg_pred(id).                                   { OUTPUT->setVisible(STRING(id), 0, true); }
show_predicate ::= neg_pred(id) DIVIDE NUMBER(n).                   { OUTPUT->setVisible(STRING(id), atol(n->c_str()), true); DELETE_PTR(n); }
hide_predicate ::= neg_pred(id) LPARA variable_list(list) RPARA.   { OUTPUT->setVisible(STRING(id), list, false); }
hide_predicate ::= neg_pred(id).                                   { OUTPUT->setVisible(STRING(id), 0, false); }
hide_predicate ::= neg_pred(id) DIVIDE NUMBER(n).                   { OUTPUT->setVisible(STRING(id), atol(n->c_str()), false); DELETE_PTR(n); }

domain_predicate ::= neg_pred(id) LPARA domain_list(list) RPARA. { GROUNDER->addDomains(STRING(id), list); }

variable_list(res) ::= variable_list(list) COMMA VARIABLE. { res = list + 1; }
variable_list(res) ::= VARIABLE.                           { res = 1; }

domain_var(res) ::= VARIABLE(var).                       { res = new IntVector(); res->push_back(STRING(var)); }
domain_var(res) ::= domain_var(list) SEMI VARIABLE(var). { res = list; res->push_back(STRING(var)); }

domain_list(res) ::= domain_list(list) COMMA domain_var(var). { res = list; res->push_back(var); }
domain_list(res) ::= domain_var(var).                         { res = new std::vector<IntVector*>(); res->push_back(var); }

rule(res) ::= head_atom(head) IF body(body). { res = new NormalRule(head, body); }
rule(res) ::= head_atom(head) IF .           { res = new NormalRule(head, 0); }
rule(res) ::= head_atom(head).               { res = new NormalRule(head, 0); }
rule(res) ::= IF body(body).                 { res = new NormalRule(0, body); }
rule(res) ::= IF.                            { res = new NormalRule(0, 0); }
rule(res) ::= maximize(min).                 { res = min; }
rule(res) ::= minimize(max).                 { res = max; }
rule(res) ::= compute(comp).                 { res = comp; }

body(res) ::= body(body) COMMA body_literal(literal). { res = body; res->push_back(literal);  }
body(res) ::= body_literal(literal).                  { res = new LiteralVector(); res->push_back(literal); }

conditional(res) ::= predicate(pred).       { res = pred; }
conditional(res) ::= NOT predicate(pred).   { res = pred; res->setNeg(true); }
conditional(res) ::= relation_literal(rel). { res = rel; }

conditional_list(res) ::= conditional_list(list) DDOT conditional(cond). { res = list ? list : new LiteralVector(); res->push_back(cond); }
conditional_list(res) ::= .                                              { res = 0; }

body_literal(res) ::= body_atom(atom).                     { res = atom; }
body_literal(res) ::= NOT body_atom(atom).                 { res = atom; res->setNeg(true); }
body_literal(res) ::= relation_literal(rel).               { res = rel; }
body_literal(res) ::= VARIABLE(id) ASSIGN aggregate(aggr). { res = aggr; aggr->setEqual(new Variable(GROUNDER, STRING(id))); }
body_literal(res) ::= aggregate_atom(atom).                { res = atom; }
body_literal(res) ::= NOT aggregate_atom(atom).            { res = atom; res->setNeg(true); }

constraint_literal(res) ::= constraint_atom(atom).     { res = atom; }
constraint_literal(res) ::= NOT constraint_atom(atom). { res = atom; res->setNeg(true); }

body_atom(res) ::= predicate(pred) conditional_list(list). { res = ConjunctionAggregate::createBody(pred, list); }

// assignment literals are not realy relation literals but they are used like them
relation_literal(res) ::= VARIABLE(a) ASSIGN term(b). { res = new AssignmentLiteral(new Variable(GROUNDER, STRING(a)), b); }
relation_literal(res) ::= term(a) EQ term(b).         { res = new RelationLiteral(RelationLiteral::EQ, a, b); }
relation_literal(res) ::= term(a) NE term(b).         { res = new RelationLiteral(RelationLiteral::NE, a, b); }
relation_literal(res) ::= term(a) GT term(b).         { res = new RelationLiteral(RelationLiteral::GT, a, b); }
relation_literal(res) ::= term(a) GE term(b).         { res = new RelationLiteral(RelationLiteral::GE, a, b); }
relation_literal(res) ::= term(a) LT term(b).         { res = new RelationLiteral(RelationLiteral::LT, a, b); }
relation_literal(res) ::= term(a) LE term(b).         { res = new RelationLiteral(RelationLiteral::LE, a, b); }

head_atom(res) ::= aggregate_atom(atom). { res = atom; }
head_atom(res) ::= disjunction(disj).    { res = disj; }

disjunction(res) ::= disj_list(list). { res = DisjunctionAggregate::createHead(list); }
disj_list(res) ::= disj_list(list) DISJUNCTION constraint_atom(term). { res = list; res->push_back(term); }
disj_list(res) ::= constraint_atom(term).                             { res = new ConditionalLiteralVector(); res->push_back(term); }

constraint_atom(res) ::= predicate(pred) conditional_list(list). { res = new ConditionalLiteral(pred, list); }

predicate(res) ::= IDENTIFIER(id) LPARA termlist(list) RPARA. { res = new PredicateLiteral(GROUNDER, STRING(id), list); }
predicate(res) ::= IDENTIFIER(id).                            { res = new PredicateLiteral(GROUNDER, STRING(id), new TermVector()); }
predicate(res) ::= MINUS IDENTIFIER(id) LPARA termlist(list) RPARA. { id->insert(id->begin(), '-'); res = new PredicateLiteral(GROUNDER, STRING(id), list); }
predicate(res) ::= MINUS IDENTIFIER(id).                            { id->insert(id->begin(), '-'); res = new PredicateLiteral(GROUNDER, STRING(id), new TermVector()); }

aggregate_lu(res)   ::= aggregate(aggr).                    { res = aggr; }
aggregate_atom(res) ::= term(l) aggregate_lu(aggr) term(u). { res = aggr; aggr->setBounds(l, u);}
aggregate_atom(res) ::= aggregate_lu(aggr) term(u).         { res = aggr; aggr->setBounds(0, u);}
aggregate_atom(res) ::= term(l) aggregate_lu(aggr).         { res = aggr; aggr->setBounds(l, 0);}
aggregate_atom(res) ::= aggregate_lu(aggr).                 { res = aggr; aggr->setBounds(0, 0);}
aggregate_atom(res) ::= EVEN LBRAC constr_list(list) RBRAC. { res = new ParityAggregate(true, list); }
aggregate_atom(res) ::= ODD  LBRAC constr_list(list) RBRAC. { res = new ParityAggregate(false, list); }

termlist(res) ::= termlist(list) COMMA term(term). { res = list; res->push_back(term); }
termlist(res) ::= term(term).                      { res = new TermVector(); res->push_back(term); }

term(res) ::= VARIABLE(x).   { res = new Variable(GROUNDER, STRING(x)); }
term(res) ::= IDENTIFIER(x). { res = new Constant(Value(Value::STRING, STRING(x))); }
term(res) ::= STRING(x).     { res = new Constant(Value(Value::STRING, STRING(x))); }
term(res) ::= NUMBER(x).     { res = new Constant(Value(Value::INT, atol(x->c_str()))); DELETE_PTR(x); }
term(res) ::= LPARA term(a) RPARA.     { res = a; }
term(res) ::= term(a) MOD term(b).     { res = new FunctionTerm(FunctionTerm::MOD, a, b); }
term(res) ::= term(a) PLUS term(b).    { res = new FunctionTerm(FunctionTerm::PLUS, a, b); }
term(res) ::= term(a) TIMES term(b).   { res = new FunctionTerm(FunctionTerm::TIMES, a, b); }
term(res) ::= term(a) POWER term(b).   { res = new FunctionTerm(FunctionTerm::POWER, a, b); }
term(res) ::= term(a) MINUS term(b).   { res = new FunctionTerm(FunctionTerm::MINUS, a, b); }
term(res) ::= term(a) DIVIDE term(b).  { res = new FunctionTerm(FunctionTerm::DIVIDE, a, b); }
term(res) ::= term(a) XOR term(b).     { res = new FunctionTerm(FunctionTerm::BITXOR, a, b); }
term(res) ::= term(a) AND term(b).     { res = new FunctionTerm(FunctionTerm::BITAND, a, b); }
term(res) ::= term(a) OR term(b).      { res = new FunctionTerm(FunctionTerm::BITOR, a, b); }
term(res) ::= TILDE term(a).           { res = new FunctionTerm(FunctionTerm::COMPLEMENT, a); }
term(res) ::= term(l) DOTS term(u).    { res = new RangeTerm(l, u); }
term(res) ::= MINUS term(b). [UMINUS]  { res = new FunctionTerm(FunctionTerm::MINUS, new Constant(Value(Value::INT, 0)), b); }
term(res) ::= ABS LPARA term(a) RPARA. { res = new FunctionTerm(FunctionTerm::ABS, a); }
term(res) ::= term(a) SEMI term(b).    { res = new MultipleArgsTerm(a, b); }
term(res) ::= IDENTIFIER(id) LPARA termlist(list) RPARA. { res = new FuncSymbolTerm(GROUNDER, STRING(id), list); }

const_termlist(res) ::= const_termlist(list) COMMA const_term(term). { res = list; res->push_back(term); }
const_termlist(res) ::= const_term(term).                            { res = new TermVector(); res->push_back(term); }

const_term(res) ::= IDENTIFIER(x). { res = new Constant(Value(Value::STRING, STRING(x))); }
const_term(res) ::= STRING(x).     { res = new Constant(Value(Value::STRING, STRING(x))); }
const_term(res) ::= NUMBER(x).     { res = new Constant(Value(Value::INT, atol(x->c_str()))); DELETE_PTR(x); }
const_term(res) ::= LPARA const_term(a) RPARA.           { res = a; }
const_term(res) ::= const_term(a) MOD const_term(b).     { res = new FunctionTerm(FunctionTerm::MOD, a, b); }
const_term(res) ::= const_term(a) PLUS const_term(b).    { res = new FunctionTerm(FunctionTerm::PLUS, a, b); }
const_term(res) ::= const_term(a) TIMES const_term(b).   { res = new FunctionTerm(FunctionTerm::TIMES, a, b); }
const_term(res) ::= const_term(a) POWER const_term(b).     { res = new FunctionTerm(FunctionTerm::POWER, a, b); }
const_term(res) ::= const_term(a) MINUS const_term(b).   { res = new FunctionTerm(FunctionTerm::MINUS, a, b); }
const_term(res) ::= const_term(a) DIVIDE const_term(b).  { res = new FunctionTerm(FunctionTerm::DIVIDE, a, b); }
const_term(res) ::= MINUS const_term(b). [UMINUS]        { res = new FunctionTerm(FunctionTerm::MINUS, new Constant(Value(Value::INT, 0)), b); }
const_term(res) ::= ABS LPARA const_term(a) RPARA.       { res = new FunctionTerm(FunctionTerm::ABS, a); }
const_term(res) ::= IDENTIFIER(id) LPARA const_termlist(list) RPARA. { res = new FuncSymbolTerm(GROUNDER, STRING(id), list); }

aggregate(res) ::= SUM LSBRAC weight_list(list) RSBRAC.   { res = new SumAggregate(list); }
aggregate(res) ::= LSBRAC weight_list(list) RSBRAC.       { res = new SumAggregate(list); }
aggregate(res) ::= COUNT LBRAC constr_list(list) RBRAC.   { res = new CountAggregate(list); }
aggregate(res) ::= LBRAC constr_list(list) RBRAC.         { res = new CountAggregate(list); }
aggregate(res) ::= MIN LSBRAC weight_list(list) RSBRAC.   { res = new MinAggregate(list); }
aggregate(res) ::= MAX LSBRAC weight_list(list) RSBRAC.   { res = new MaxAggregate(list); }

aggregate_lu(res) ::= AVG LSBRAC weight_list(list) RSBRAC.   { res = new AvgAggregate(list); }
aggregate_lu(res) ::= TIMES LSBRAC weight_list(list) RSBRAC. { res = new TimesAggregate(list); }

compute(res)  ::= COMPUTE LBRAC  constr_list(list) RBRAC.           { res = new LiteralStatement(new ComputeLiteral(list, 1), false); }
compute(res)  ::= COMPUTE NUMBER(x) LBRAC  constr_list(list) RBRAC. { res = new LiteralStatement(new ComputeLiteral(list, atol(x->c_str())), false); DELETE_PTR(x); }
minimize(res) ::= MINIMIZE LBRAC  constr_list(list) RBRAC.          { res = new LiteralStatement(new OptimizeLiteral(OptimizeLiteral::MINIMIZE, list, true), true); }
minimize(res) ::= MINIMIZE LSBRAC weight_list(list) RSBRAC.         { res = new LiteralStatement(new OptimizeLiteral(OptimizeLiteral::MINIMIZE, list, false), true); }
maximize(res) ::= MAXIMIZE LBRAC  constr_list(list) RBRAC.          { res = new LiteralStatement(new OptimizeLiteral(OptimizeLiteral::MAXIMIZE, list, true), true); }
maximize(res) ::= MAXIMIZE LSBRAC weight_list(list) RSBRAC.         { res = new LiteralStatement(new OptimizeLiteral(OptimizeLiteral::MAXIMIZE, list, false), true); }

weight_list(res) ::= nweight_list(list). { res = list;}
weight_list(res) ::= .                   { res = new ConditionalLiteralVector();}
nweight_list(res) ::= nweight_list(list) COMMA weight_term(term). { res = list; res->push_back(term); }
nweight_list(res) ::= weight_term(term). { res = new ConditionalLiteralVector(); res->push_back(term);}

weight_term(res) ::= constraint_literal(literal) ASSIGN term(term). { res = literal; res->setWeight(term); }
weight_term(res) ::= constraint_literal(literal).                   { res = literal; res->setWeight(new Constant(Value(Value::INT, 1))); }

constr_list(res) ::= nconstr_list(list). { res = list; }
constr_list(res) ::= .                   { res = new ConditionalLiteralVector(); }
nconstr_list(res) ::= nconstr_list(list) COMMA constr_term(term). { res = list; res->push_back(term); }
nconstr_list(res) ::= constr_term(term).                          { res = new ConditionalLiteralVector(); res->push_back(term); }

constr_term(res) ::= constraint_literal(literal).  { res = literal; res->setWeight(new Constant(Value(Value::INT, 1))); }

