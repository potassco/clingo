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
    return lexer->lex(value);
}

%}

%code {

void GroundTermGrammar::parser::error(GroundTermGrammar::parser::location_type const &, std::string const &msg) {
    lexer->parseError(msg);
}

}

// {{{1 nonterminals
%union {
    int        num;
    unsigned   uid;
    Value::POD value;
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

%token <uid>
    IDENTIFIER "<IDENTIFIER>"
    STRING     "<STRING>"
    NOT        "not"

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

start : term[r] { lexer->value = $r; };

term
    : term[a] XOR term[b]                     { $$ = lexer->term(BinOp::XOR, $a, $b); }
    | term[a] QUESTION term[b]                { $$ = lexer->term(BinOp::OR, $a, $b); }
    | term[a] AND term[b]                     { $$ = lexer->term(BinOp::AND, $a, $b); }
    | term[a] ADD term[b]                     { $$ = lexer->term(BinOp::ADD, $a, $b); }
    | term[a] SUB term[b]                     { $$ = lexer->term(BinOp::SUB, $a, $b); }
    | term[a] MUL term[b]                     { $$ = lexer->term(BinOp::MUL, $a, $b); }
    | term[a] SLASH term[b]                   { $$ = lexer->term(BinOp::DIV, $a, $b); }
    | term[a] MOD term[b]                     { $$ = lexer->term(BinOp::MOD, $a, $b); }
    | term[a] POW term[b]                     { $$ = lexer->term(BinOp::POW, $a, $b); }
    | SUB term[a] %prec UMINUS                { $$ = lexer->term(UnOp::NEG, $a); }
    | BNOT term[a] %prec UBNOT                { $$ = lexer->term(UnOp::NOT, $a); }
    | LPAREN RPAREN                           { $$ = Value::createTuple({}); }
    | LPAREN COMMA RPAREN                     { $$ = Value::createTuple({}); }
    | LPAREN nterms[a] RPAREN                 { $$ = lexer->tuple($a, false); }
    | LPAREN nterms[a] COMMA RPAREN           { $$ = lexer->tuple($a, true); }
    | IDENTIFIER[a] LPAREN terms[b] RPAREN    { $$ = Value::createFun($a, lexer->terms($b)); }
    | VBAR term[a] VBAR                       { $$ = lexer->term(UnOp::ABS, $a); }
    | IDENTIFIER[a]                           { $$ = Value::createId(FWString($a)); }
    | NUMBER[a]                               { $$ = Value::createNum($a); }
    | STRING[a]                               { $$ = Value::createStr(FWString($a)); }
    | INFIMUM[a]                              { $$ = Value::createInf(); }
    | SUPREMUM[a]                             { $$ = Value::createSup(); }
    ;

nterms
    : term[a]                 { $$ = lexer->terms(lexer->terms(), $a);  }
    | nterms[a] COMMA term[b] { $$ = lexer->terms($a, $b);  }
    ;

terms
    : nterms[a] { $$ = $a;  }
    |           { $$ = lexer->terms();  }
    ;

