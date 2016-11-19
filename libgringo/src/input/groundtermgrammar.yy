// {{{ GPL License 

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

%require "2.5"
%define namespace "Gringo::Input::GroundTermGrammar"
%name-prefix "GringoGroundTermGrammar_"
%error-verbose
%locations // NOTE: different behavior for location-less parsers for different bison versions 
%defines
%parse-param { Gringo::Input::GroundTermParser *lexer }
%lex-param { Gringo::Input::GroundTermParser *lexer }
%skeleton "lalr1.cc"
//%define parse.trace
//%debug

// {{{1 auxiliary code

%{

#include "gringo/term.hh"
#include "gringo/input/groundtermparser.hh"
#include <climits> 

using namespace Gringo;
using namespace Gringo::Input;

int GringoGroundTermGrammar_lex(void *value, void *, GroundTermParser *lexer) {
    return lexer->lex(value, lexer->logger());
}

%}

%code {

void GroundTermGrammar::parser::error(GroundTermGrammar::parser::location_type const &, std::string const &msg) {
    lexer->parseError(msg, lexer->logger());
}

}

// {{{1 nonterminals
%union {
    char const *str;
    int         num;
    unsigned    uid;
    uint64_t    value;
}

// {{{1 terminals

%token
    ADD         "+"
    AND         "&"
    BNOT        "~"
    COMMA       ","
    END         0 "<EOF>"
    LPAREN      "("
    MOD         "\\"
    MUL         "*"
    POW         "**"
    QUESTION    "?"
    RPAREN      ")"
    SLASH       "/"
    SUB         "-"
    XOR         "^"
    VBAR        "|"
    SUPREMUM    "#sup"
    INFIMUM     "#inf"

%token <num>
    NUMBER     "<NUMBER>"

%token <str>
    IDENTIFIER "<IDENTIFIER>"
    STRING     "<STRING>"

// {{{1 operator precedence and associativity

%left XOR
%left QUESTION
%left AND
%left ADD SUB
%left MUL SLASH MOD
%right POW
%left UMINUS UBNOT

%type <value> term
%type <uid> nterms terms

// }}}1

%%

// {{{1 the parser

start : term[r] { lexer->value = Symbol($r); };

term
    : term[a] XOR term[b]                     { $$ = lexer->term(BinOp::XOR, Symbol($a), Symbol($b)).rep(); }
    | term[a] QUESTION term[b]                { $$ = lexer->term(BinOp::OR,  Symbol($a), Symbol($b)).rep(); }
    | term[a] AND term[b]                     { $$ = lexer->term(BinOp::AND, Symbol($a), Symbol($b)).rep(); }
    | term[a] ADD term[b]                     { $$ = lexer->term(BinOp::ADD, Symbol($a), Symbol($b)).rep(); }
    | term[a] SUB term[b]                     { $$ = lexer->term(BinOp::SUB, Symbol($a), Symbol($b)).rep(); }
    | term[a] MUL term[b]                     { $$ = lexer->term(BinOp::MUL, Symbol($a), Symbol($b)).rep(); }
    | term[a] SLASH term[b]                   { $$ = lexer->term(BinOp::DIV, Symbol($a), Symbol($b)).rep(); }
    | term[a] MOD term[b]                     { $$ = lexer->term(BinOp::MOD, Symbol($a), Symbol($b)).rep(); }
    | term[a] POW term[b]                     { $$ = lexer->term(BinOp::POW, Symbol($a), Symbol($b)).rep(); }
    | SUB term[a] %prec UMINUS                { $$ = lexer->term(UnOp::NEG, Symbol($a)).rep(); }
    | BNOT term[a] %prec UBNOT                { $$ = lexer->term(UnOp::NOT, Symbol($a)).rep(); }
    | LPAREN RPAREN                           { $$ = Symbol::createTuple(Potassco::toSpan<Symbol>()).rep(); }
    | LPAREN COMMA RPAREN                     { $$ = Symbol::createTuple(Potassco::toSpan<Symbol>()).rep(); }
    | LPAREN nterms[a] RPAREN                 { $$ = lexer->tuple($a, false).rep(); }
    | LPAREN nterms[a] COMMA RPAREN           { $$ = lexer->tuple($a, true).rep(); }
    | IDENTIFIER[a] LPAREN terms[b] RPAREN    { $$ = Symbol::createFun($a, Potassco::toSpan(lexer->terms($b))).rep(); }
    | VBAR term[a] VBAR                       { $$ = lexer->term(UnOp::ABS, Symbol($a)).rep(); }
    | IDENTIFIER[a]                           { $$ = Symbol::createId($a).rep(); }
    | NUMBER[a]                               { $$ = Symbol::createNum($a).rep(); }
    | STRING[a]                               { $$ = Symbol::createStr($a).rep(); }
    | INFIMUM[a]                              { $$ = Symbol::createInf().rep(); }
    | SUPREMUM[a]                             { $$ = Symbol::createSup().rep(); }
    ;

nterms
    : term[a]                 { $$ = lexer->terms(lexer->terms(), Symbol($a));  }
    | nterms[a] COMMA term[b] { $$ = lexer->terms($a, Symbol($b));  }
    ;

terms
    : nterms[a] { $$ = $a;  }
    |           { $$ = lexer->terms();  }
    ;

