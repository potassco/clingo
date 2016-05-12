// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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
#include "gringo/gringo.h"
#include "converter_impl.h"
#include "gringo/converter.h"
#include "gringo/storage.h"

#define ONE   Val::create(Val::NUM, 1)
#define UNDEF Val::create()
#define PRIO  Val::create(Val::NUM, pConverter->level())

struct Aggr
{
	GroundProgramBuilder::Type type;
	uint32_t n;
	static void create(Aggr &a, GroundProgramBuilder::Type t, uint32_t n)
	{
		a.type = t;
		a.n    = n;
	}
};

}

%name             converter
%stack_size       0
%parse_failure    { pConverter->parseError(); }
%syntax_error     { pConverter->syntaxError(); }
%extra_argument   { Converter *pConverter }
%token_type       { Converter::Token }
%token_destructor { (void)pConverter; (void)$$; }
%token_prefix     CONVERTER_
%start_symbol     start

%type body           { uint32_t }
%type nbody          { uint32_t }
%type termlist       { uint32_t }
%type nprio_list     { uint32_t }
%type prio_list      { uint32_t }
%type nprio_set      { uint32_t }
%type prio_set       { uint32_t }
%type head_ccondlist { uint32_t }
%type nweightlist    { uint32_t }
%type weightlist     { uint32_t }
%type nnumweightlist { uint32_t }
%type numweightlist  { uint32_t }
%type ncondlist      { uint32_t }
%type condlist       { uint32_t }

%type aggr { Aggr }
%type aggr_any { Aggr }
%type aggr_num { Aggr }

start ::= program.

program ::= .
program ::= program line DOT.

line ::= rule.
line ::= HIDE.                            { pConverter->add(Converter::META_GLOBALHIDE); }
line ::= SHOW.                            { pConverter->add(Converter::META_GLOBALSHOW); }
line ::= HIDE signed SLASH posnumber.     { pConverter->add(Converter::META_HIDE); }
line ::= HIDE predicate.                  { pConverter->add(Converter::STM_HIDE); }
line ::= SHOW signed SLASH posnumber.     { pConverter->add(Converter::META_SHOW); }
line ::= SHOW predicate.                  { pConverter->add(Converter::STM_SHOW); }
line ::= EXTERNAL predicate.              { pConverter->add(Converter::STM_EXTERNAL); }
line ::= EXTERNAL signed SLASH posnumber. { pConverter->add(Converter::META_EXTERNAL); }
line ::= optimize.
line ::= compute.

id ::= IDENTIFIER(id). { pConverter->addSigned(id.index, false); }

signed ::= id.
signed ::= MINUS IDENTIFIER(id). { pConverter->addSigned(id.index, true); }

rule ::= head IF body(n). { pConverter->add(Converter::STM_RULE, n); }
rule ::= IF body(n).      { pConverter->add(Converter::STM_CONSTRAINT, n); }
rule ::= head.            { pConverter->add(Converter::STM_RULE, 0); }

head ::= predicate.
head ::= aggr_atom.
head ::= disjunction.

nbody(res) ::= body_literal.                { res = 1; }
nbody(res) ::= nbody(n) COMMA body_literal. { res = n + 1; }
body(res) ::= .         { res = 0; }
body(res) ::= nbody(n). { res = n; }

predlit ::= predicate.
predlit ::= NOT predicate. { pConverter->addSign(); }

body_literal ::= predlit.
body_literal ::= aggr_atom.
body_literal ::= NOT aggr_atom. { pConverter->addSign(); }

string    ::= STRING(id).        { pConverter->addVal(Val::create(Val::ID, id.index)); }
empty     ::= .                  { pConverter->addVal(Val::create(Val::ID, pConverter->storage()->index(""))); }
posnumber ::= NUMBER(num).       { pConverter->addVal(Val::create(Val::NUM, num.number)); }
number    ::= MINUS NUMBER(num). { pConverter->addVal(Val::create(Val::NUM, -num.number)); }
number    ::= posnumber.
supremum  ::= SUP.               { pConverter->addVal(Val::sup()); }
infimum   ::= INF.               { pConverter->addVal(Val::inf()); }

numterm ::= number.   { pConverter->add(Converter::TERM, 0); }
numterm ::= supremum. { pConverter->add(Converter::TERM, 0); }
numterm ::= infimum.  { pConverter->add(Converter::TERM, 0); }
term ::= id.                                       { pConverter->add(Converter::TERM, 0); }
term ::= string.                                   { pConverter->add(Converter::TERM, 0); }
term ::= empty LBRAC termlist(n) COMMA term RBRAC. { pConverter->add(Converter::TERM, n+1); }
term ::= id LBRAC termlist(n) RBRAC.               { pConverter->add(Converter::TERM, n); }
term ::= numterm.

nnumweightlist(res) ::= numweightlit.                         { res = 1; }
nnumweightlist(res) ::= nnumweightlist(n) COMMA numweightlit. { res = n + 1; }
numweightlist(res) ::= .                  { res = 0; }
numweightlist(res) ::= nnumweightlist(n). { res = n; }

numweightlit ::= predlit ASSIGN number.
numweightlit ::= predlit.               { pConverter->addVal(ONE); }

aggr_num(res) ::= SUM LSBRAC numweightlist(n) RSBRAC. { Aggr::create(res, Converter::AGGR_SUM, n); }
aggr_num(res) ::= LSBRAC numweightlist(n) RSBRAC.     { Aggr::create(res, Converter::AGGR_SUM, n); }

ncondlist(res) ::= predlit.                    { res = 1; }
ncondlist(res) ::= ncondlist(n) COMMA predlit. { res = n + 1; }
condlist(res) ::= .             { res = 0; }
condlist(res) ::= ncondlist(n). { res = n; }

aggr_num(res) ::= COUNT LCBRAC condlist(n) RCBRAC. { Aggr::create(res, Converter::AGGR_COUNT, n); }
aggr_num(res) ::= LCBRAC condlist(n) RCBRAC.       { Aggr::create(res, Converter::AGGR_COUNT, n); }

aggr_num(res) ::= AVG LSBRAC numweightlist(n) RSBRAC. { Aggr::create(res, Converter::AGGR_AVG, n); }

nweightlist(res) ::= weightlit.                      { res = 1; }
nweightlist(res) ::= nweightlist(n) COMMA weightlit. { res = n + 1; }
weightlist(res) ::= .               { res = 0; }
weightlist(res) ::= nweightlist(n). { res = n; }

weightlit ::= predlit ASSIGN term.
weightlit ::= predlit.             { pConverter->addVal(ONE); }

aggr_any(res) ::= MIN LSBRAC weightlist(n) RSBRAC. { Aggr::create(res, Converter::AGGR_MIN, n); }
aggr_any(res) ::= MAX LSBRAC weightlist(n) RSBRAC. { Aggr::create(res, Converter::AGGR_MAX, n); }

aggr(res) ::= EVEN LSBRAC numweightlist(n) RSBRAC. { Aggr::create(res, Converter::AGGR_EVEN, n); }
aggr(res) ::= EVEN LCBRAC condlist(n) RCBRAC.   { Aggr::create(res, Converter::AGGR_EVEN_SET, n); }
aggr(res) ::= ODD LSBRAC numweightlist(n) RSBRAC.  { Aggr::create(res, Converter::AGGR_ODD, n); }
aggr(res) ::= ODD LCBRAC condlist(n) RCBRAC.    { Aggr::create(res, Converter::AGGR_ODD_SET, n); }

head_ccondlist(res) ::= predicate VBAR predicate.         { res = 2; }
head_ccondlist(res) ::= head_ccondlist(n) VBAR predicate. { res = n + 1; }

disjunction ::= head_ccondlist(n). { pConverter->add(Converter::AGGR_DISJUNCTION, n); }

undef ::= . { pConverter->addVal(UNDEF); }

aggr_atom ::= term  aggr_any(aggr) term.  { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= undef aggr_any(aggr) term.  { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= term  aggr_any(aggr) undef. { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= undef aggr_any(aggr) undef. { pConverter->add(aggr.type, aggr.n); }

aggr_atom ::= numterm aggr_num(aggr) numterm. { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= undef  aggr_num(aggr) numterm.  { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= numterm aggr_num(aggr) undef.   { pConverter->add(aggr.type, aggr.n); }
aggr_atom ::= undef aggr_num(aggr) undef.     { pConverter->add(aggr.type, aggr.n); }

aggr_atom ::= undef aggr(aggr) undef. { pConverter->add(aggr.type, aggr.n); }

predicate ::= id LBRAC termlist(n) RBRAC. { pConverter->add(Converter::LIT, n); }
predicate ::= id.                         { pConverter->add(Converter::LIT, 0); }
predicate ::= MINUS IDENTIFIER(id) LBRAC termlist(n) RBRAC. { pConverter->addSigned(id.index, true); pConverter->add(Converter::LIT, n); }
predicate ::= MINUS IDENTIFIER(id).                         { pConverter->addSigned(id.index, true); pConverter->add(Converter::LIT, 0); }

termlist(res) ::= term.                   { res = 1; }
termlist(res) ::= termlist(n) COMMA term. { res = n + 1; }

optimize ::= MINIMIZE LSBRAC prio_list(n) RSBRAC. { pConverter->add(Converter::STM_MINIMIZE, n); pConverter->nextLevel(); }
optimize ::= MAXIMIZE LSBRAC prio_list(n) RSBRAC. { pConverter->add(Converter::STM_MAXIMIZE, n); pConverter->nextLevel(); }
optimize ::= MINIMIZE LCBRAC prio_set(n) RCBRAC.  { pConverter->add(Converter::STM_MINIMIZE_SET, n); pConverter->nextLevel(); }
optimize ::= MAXIMIZE LCBRAC prio_set(n) RCBRAC.  { pConverter->add(Converter::STM_MAXIMIZE_SET, n); pConverter->nextLevel(); }

compute ::= COMPUTE LCBRAC condlist(n) RCBRAC. { pConverter->add(Converter::STM_COMPUTE, n); }
compute ::= COMPUTE NUMBER LCBRAC condlist(n) RCBRAC. { pConverter->add(Converter::STM_COMPUTE, n); }

nprio_list(res) ::= weightedpriolit.                    { res = 1; }
nprio_list(res) ::= prio_list(n) COMMA weightedpriolit. { res = n + 1; }
prio_list(res) ::= .              { res = 0; }
prio_list(res) ::= nprio_list(n). { res = n; }

nprio_set(res) ::= priolit.                   { res = 1; }
nprio_set(res) ::= prio_set(n) COMMA priolit. { res = n + 1; }
prio_set(res) ::= .             { res = 0; }
prio_set(res) ::= nprio_set(n). { res = n; }

prio ::= . { pConverter->addVal(PRIO); }
one  ::= . { pConverter->addVal(ONE); }

weightedpriolit ::= predlit ASSIGN number AT number.
weightedpriolit ::= predlit ASSIGN number prio.
weightedpriolit ::= priolit.

priolit ::= predlit one AT number.
priolit ::= predlit one prio.
