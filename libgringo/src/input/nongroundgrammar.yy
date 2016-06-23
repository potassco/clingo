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

// {{{1 preamble
%require "2.5"
//%define api.namespace {Gringo::Input::NonGroundGrammar}
%define namespace "Gringo::Input::NonGroundGrammar"
//%define api.prefix {GringoNonGroundGrammar_}
%name-prefix "GringoNonGroundGrammar_"
//%define parse.error verbose
%error-verbose
//%define api.location.type {DefaultLocation}
%define location_type "DefaultLocation"
%locations
%defines
%parse-param { Gringo::Input::NonGroundParser *lexer }
%lex-param { Gringo::Input::NonGroundParser *lexer }
%skeleton "lalr1.cc"
//%define parse.trace
//%debug

// {{{1 auxiliary code

%code requires
{
    #include "gringo/input/programbuilder.hh"
    #include "potassco/basic_types.h"

    namespace Gringo { namespace Input { class NonGroundParser; } }
    
    struct DefaultLocation : Gringo::Location {
        DefaultLocation() : Location("<undef>", 0, 0, "<undef>", 0, 0) { }
    };

}

%{

#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include <climits> 

#define BUILDER (lexer->builder())
#define LOGGER (lexer->logger())
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do {                                                               \
        if (N) {                                                       \
            (Current).beginFilename = YYRHSLOC (Rhs, 1).beginFilename; \
            (Current).beginLine     = YYRHSLOC (Rhs, 1).beginLine;     \
            (Current).beginColumn   = YYRHSLOC (Rhs, 1).beginColumn;   \
            (Current).endFilename   = YYRHSLOC (Rhs, N).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, N).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, N).endColumn;     \
        }                                                              \
        else {                                                         \
            (Current).beginFilename = YYRHSLOC (Rhs, 0).endFilename;   \
            (Current).beginLine     = YYRHSLOC (Rhs, 0).endLine;       \
            (Current).beginColumn   = YYRHSLOC (Rhs, 0).endColumn;     \
            (Current).endFilename   = YYRHSLOC (Rhs, 0).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, 0).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, 0).endColumn;     \
        }                                                              \
    }                                                                  \
    while (false)

using namespace Gringo;
using namespace Gringo::Input;

int GringoNonGroundGrammar_lex(void *value, Gringo::Location* loc, NonGroundParser *lexer) {
    return lexer->lex(value, *loc);
}

%}

%code {

void NonGroundGrammar::parser::error(DefaultLocation const &l, std::string const &msg) {
    lexer->parseError(l, msg);
}

}

// {{{1 nonterminals

// {{{2 union type for stack elements
%union
{
    IdVecUid idlist;
    CSPLitUid csplit;
    CSPAddTermUid cspaddterm;
    CSPMulTermUid cspmulterm;
    CSPElemVecUid cspelemvec;
    TermUid term;
    TermVecUid termvec;
    TermVecVecUid termvecvec;
    LitVecUid litvec;
    LitUid lit;
    BdAggrElemVecUid bodyaggrelemvec;
    CondLitVecUid condlitlist;
    HdAggrElemVecUid headaggrelemvec;
    BoundVecUid bounds;
    BdLitVecUid body;
    HdLitUid head;
    Relation rel;
    AggregateFunction fun;
    struct {
        NAF first;
        CSPElemVecUid second;
    } disjoint;
    struct {
        uintptr_t first;
        unsigned second;
    } pair;
    struct {
        TermVecUid first;
        LitVecUid second;
    } bodyaggrelem;
    struct {
        LitUid first;
        LitVecUid second;
    } lbodyaggrelem;
    struct {
        AggregateFunction fun;
        unsigned choice : 1;
        unsigned elems : 31;
    } aggr;
    struct {
        Relation rel;
        TermUid term;
    } bound;
    struct {
        TermUid first;
        TermUid second;
    } termpair;
    unsigned uid;
    uintptr_t str;
    int num;
    Potassco::Heuristic_t::E heu;
    TheoryOpVecUid theoryOps;
    TheoryTermUid theoryTerm;
    TheoryOptermUid theoryOpterm;
    TheoryOptermVecUid theoryOpterms;
    TheoryElemVecUid theoryElems;
    struct {
        TheoryOptermVecUid first;
        LitVecUid second;
    } theoryElem;
    TheoryAtomUid theoryAtom;
    TheoryOpDefUid theoryOpDef;
    TheoryOpDefVecUid theoryOpDefs;
    TheoryTermDefUid theoryTermDef;
    TheoryAtomDefUid theoryAtomDef;
    TheoryDefVecUid theoryDefs;
    TheoryAtomType theoryAtomType;
}

// }}}2

// TODO: improve naming scheme
%type <term>            constterm term tuple theory_atom_name
%type <termvec>         termvec ntermvec consttermvec unaryargvec optimizetuple tuplevec tuplevec_sem
%type <termvecvec>      argvec constargvec binaryargvec
%type <lit>             literal
%type <litvec>          litvec nlitvec optcondition noptcondition
%type <bodyaggrelem>    bodyaggrelem
%type <lbodyaggrelem>   altbodyaggrelem conjunction
%type <bodyaggrelemvec> bodyaggrelemvec
%type <condlitlist>     altbodyaggrelemvec altheadaggrelemvec disjunctionsep disjunction
%type <headaggrelemvec> headaggrelemvec
%type <cspelemvec>      cspelemvec ncspelemvec
%type <bound>           upper
%type <body>            bodycomma bodydot bodyconddot optimizelitvec optimizecond
%type <head>            head
%type <uid>             lubodyaggregate luheadaggregate
%type <str>             identifier theory_definition_identifier
%type <pair>            atom
%type <fun>             aggregatefunction
%type <aggr>            bodyaggregate headaggregate
%type <rel>             cmp csp_rel
%type <termpair>        optimizeweight
%type <cspmulterm>      csp_mul_term
%type <cspaddterm>      csp_add_term
%type <csplit>          csp_literal 
%type <disjoint>        disjoint
%type <idlist>          idlist nidlist
%type <theoryOps>       theory_op_list theory_operator_list theory_operator_nlist
%type <theoryTerm>      theory_term
%type <theoryOpterm>    theory_opterm
%type <theoryOpterms>   theory_opterm_list theory_opterm_nlist
%type <theoryElem>      theory_atom_element
%type <theoryElems>     theory_atom_element_list theory_atom_element_nlist
%type <theoryAtom>      theory_atom
%type <theoryTermDef>   theory_term_definition
%type <theoryAtomDef>   theory_atom_definition
%type <theoryOpDef>     theory_operator_definition
%type <theoryOpDefs>    theory_operator_definition_nlist theory_operator_definiton_list
%type <theoryDefs>      theory_definition_nlist theory_definition_list
%type <theoryAtomType>  theory_atom_type

// {{{1 terminals

%token
    ADD         "+"
    AND         "&"
    EQ          "="
    AT          "@"
    BASE        "#base"
    BNOT        "~"
    COLON       ":"
    COMMA       ","
    CONST       "#const"
    COUNT       "#count"
    CSP         "$"
    CSP_ADD     "$+"
    CSP_SUB     "$-"
    CSP_MUL     "$*"
    CSP_LEQ     "$<="
    CSP_LT      "$<"
    CSP_GT      "$>"
    CSP_GEQ     "$>="
    CSP_EQ      "$="
    CSP_NEQ     "$!="
    CUMULATIVE  "#cumulative"
    DISJOINT    "#disjoint"
    DOT         "."
    DOTS        ".."
    END         0 "<EOF>"
    EXTERNAL    "#external"
    FALSE       "#false"
    FORGET      "#forget"
    GEQ         ">="
    GT          ">"
    IF          ":-"
    INCLUDE     "#include"
    INFIMUM     "#inf"
    LBRACE      "{"
    LBRACK      "["
    LEQ         "<="
    LPAREN      "("
    LT          "<"
    MAX         "#max"
    MAXIMIZE    "#maximize"
    MIN         "#min"
    MINIMIZE    "#minimize"
    MOD         "\\"
    MUL         "*"
    NEQ         "!="
    POW         "**"
    QUESTION    "?"
    RBRACE      "}"
    RBRACK      "]"
    RPAREN      ")"
    SEM         ";"
    SHOW        "#show"
    EDGE        "#edge"
    PROJECT     "#project"
    HEURISTIC   "#heuristic"
    SHOWSIG     "#showsig"
    SLASH       "/"
    SUB         "-"
    SUM         "#sum"
    SUMP        "#sum+"
    SUPREMUM    "#sup"
    TRUE        "#true"
    BLOCK       "#program"
    UBNOT
    UMINUS
    VBAR        "|"
    VOLATILE    "#volatile"
    WIF         ":~"
    XOR         "^"
    PARSE_LP    "<program>"
    PARSE_DEF   "<define>"
    ANY         "any"
    UNARY       "unary"
    BINARY      "binary"
    LEFT        "left"
    RIGHT       "right"
    HEAD        "head"
    BODY        "body"
    DIRECTIVE   "directive"
    THEORY      "#theory"


%token <num>
    NUMBER     "<NUMBER>"

%token <str>
    ANONYMOUS  "<ANONYMOUS>"
    IDENTIFIER "<IDENTIFIER>"
    PYTHON     "<PYTHON>"
    LUA        "<LUA>"
    STRING     "<STRING>"
    VARIABLE   "<VARIABLE>"
    THEORY_OP  "<THEORYOP>"
    NOT        "not"

// {{{2 operator precedence and associativity

%left DOTS
%left XOR
%left QUESTION
%left AND
%left ADD SUB
%left MUL SLASH MOD
%right POW
%left UMINUS UBNOT

// }}}1

%%

// {{{1 logic program and global definitions

start
    : PARSE_LP  program
    | PARSE_DEF define
    ;

program
    : program statement
    | 
    ;

// Note: skip until the next "." in case of an error and switch back to normal lexing

statement
    : error disable_theory_lexing DOT
    ;

identifier
    : IDENTIFIER[a] { $$ = $a; }
    ;

// {{{1 terms
// {{{2 constterms are terms without variables and pooling operators

constterm
    : constterm[a] XOR constterm[b]                    { $$ = BUILDER.term(@$, BinOp::XOR, $a, $b); }
    | constterm[a] QUESTION constterm[b]               { $$ = BUILDER.term(@$, BinOp::OR, $a, $b); }
    | constterm[a] AND constterm[b]                    { $$ = BUILDER.term(@$, BinOp::AND, $a, $b); }
    | constterm[a] ADD constterm[b]                    { $$ = BUILDER.term(@$, BinOp::ADD, $a, $b); }
    | constterm[a] SUB constterm[b]                    { $$ = BUILDER.term(@$, BinOp::SUB, $a, $b); }
    | constterm[a] MUL constterm[b]                    { $$ = BUILDER.term(@$, BinOp::MUL, $a, $b); }
    | constterm[a] SLASH constterm[b]                  { $$ = BUILDER.term(@$, BinOp::DIV, $a, $b); }
    | constterm[a] MOD constterm[b]                    { $$ = BUILDER.term(@$, BinOp::MOD, $a, $b); }
    | constterm[a] POW constterm[b]                    { $$ = BUILDER.term(@$, BinOp::POW, $a, $b); }
    | SUB constterm[a] %prec UMINUS                    { $$ = BUILDER.term(@$, UnOp::NEG, $a); }
    | BNOT constterm[a] %prec UBNOT                    { $$ = BUILDER.term(@$, UnOp::NOT, $a); }
    | LPAREN RPAREN                                    { $$ = BUILDER.term(@$, BUILDER.termvec(), false); }
    | LPAREN COMMA RPAREN                              { $$ = BUILDER.term(@$, BUILDER.termvec(), true); }
    | LPAREN consttermvec[a] RPAREN                    { $$ = BUILDER.term(@$, $a, false); }
    | LPAREN consttermvec[a] COMMA RPAREN              { $$ = BUILDER.term(@$, $a, true); }
    | identifier[a] LPAREN constargvec[b] RPAREN       { $$ = BUILDER.term(@$, String::fromRep($a), $b, false); }
    | AT[l] identifier[a] LPAREN constargvec[b] RPAREN { $$ = BUILDER.term(@$, String::fromRep($a), $b, true); }
    | VBAR[l] constterm[a] VBAR                        { $$ = BUILDER.term(@$, UnOp::ABS, $a); }
    | identifier[a]                                    { $$ = BUILDER.term(@$, Symbol::createId(String::fromRep($a))); }
    | NUMBER[a]                                        { $$ = BUILDER.term(@$, Symbol::createNum($a)); }
    | STRING[a]                                        { $$ = BUILDER.term(@$, Symbol::createStr(String::fromRep($a))); }
    | INFIMUM[a]                                       { $$ = BUILDER.term(@$, Symbol::createInf()); }
    | SUPREMUM[a]                                      { $$ = BUILDER.term(@$, Symbol::createSup()); }
    ;

// {{{2 arguments lists for functions in constant terms

consttermvec
    : constterm[a]                       { $$ = BUILDER.termvec(BUILDER.termvec(), $a);  }
    | consttermvec[a] COMMA constterm[b] { $$ = BUILDER.termvec($a, $b);  }
    ;

constargvec
    : consttermvec[a] { $$ = BUILDER.termvecvec(BUILDER.termvecvec(), $a);  }
    |                 { $$ = BUILDER.termvecvec();  }
    ;

// {{{2 terms including variables

term
    : term[a] DOTS term[b]                     { $$ = BUILDER.term(@$, $a, $b); }
    | term[a] XOR term[b]                      { $$ = BUILDER.term(@$, BinOp::XOR, $a, $b); }
    | term[a] QUESTION term[b]                 { $$ = BUILDER.term(@$, BinOp::OR, $a, $b); }
    | term[a] AND term[b]                      { $$ = BUILDER.term(@$, BinOp::AND, $a, $b); }
    | term[a] ADD term[b]                      { $$ = BUILDER.term(@$, BinOp::ADD, $a, $b); }
    | term[a] SUB term[b]                      { $$ = BUILDER.term(@$, BinOp::SUB, $a, $b); }
    | term[a] MUL term[b]                      { $$ = BUILDER.term(@$, BinOp::MUL, $a, $b); }
    | term[a] SLASH term[b]                    { $$ = BUILDER.term(@$, BinOp::DIV, $a, $b); }
    | term[a] MOD term[b]                      { $$ = BUILDER.term(@$, BinOp::MOD, $a, $b); }
    | term[a] POW term[b]                      { $$ = BUILDER.term(@$, BinOp::POW, $a, $b); }
    | SUB term[a] %prec UMINUS                 { $$ = BUILDER.term(@$, UnOp::NEG, $a); }
    | BNOT term[a] %prec UBNOT                 { $$ = BUILDER.term(@$, UnOp::NOT, $a); }
    | LPAREN tuplevec[a] RPAREN                { $$ = BUILDER.pool(@$, $a); }
    | identifier[a] LPAREN argvec[b] RPAREN    { $$ = BUILDER.term(@$, String::fromRep($a), $b, false); }
    | AT identifier[a] LPAREN argvec[b] RPAREN { $$ = BUILDER.term(@$, String::fromRep($a), $b, true); }
    | VBAR unaryargvec[a] VBAR                 { $$ = BUILDER.term(@$, UnOp::ABS, $a); }
    | identifier[a]                            { $$ = BUILDER.term(@$, Symbol::createId(String::fromRep($a))); }
    | NUMBER[a]                                { $$ = BUILDER.term(@$, Symbol::createNum($a)); }
    | STRING[a]                                { $$ = BUILDER.term(@$, Symbol::createStr(String::fromRep($a))); }
    | INFIMUM[a]                               { $$ = BUILDER.term(@$, Symbol::createInf()); }
    | SUPREMUM[a]                              { $$ = BUILDER.term(@$, Symbol::createSup()); }
    | VARIABLE[a]                              { $$ = BUILDER.term(@$, String::fromRep($a)); }
    | ANONYMOUS[a]                             { $$ = BUILDER.term(@$, String("_")); }
    ;

// {{{2 argument lists for unary operations

unaryargvec
    : term[a]                    { $$ = BUILDER.termvec(BUILDER.termvec(), $a); }
    | unaryargvec[a] SEM term[b] { $$ = BUILDER.termvec($a, $b); }
    ;

// {{{2 argument lists for functions

ntermvec
    : term[a]                   { $$ = BUILDER.termvec(BUILDER.termvec(), $a); }
    | ntermvec[a] COMMA term[b] { $$ = BUILDER.termvec($a, $b); }
    ;

termvec
    : ntermvec[a] { $$ = $a; }
    |             { $$ = BUILDER.termvec(); }
    ;

tuple
    : ntermvec[a] COMMA { $$ = BUILDER.term(@$, $a, true); }
    | ntermvec[a]       { $$ = BUILDER.term(@$, $a, false); }
    |             COMMA { $$ = BUILDER.term(@$, BUILDER.termvec(), true); }
    |                   { $$ = BUILDER.term(@$, BUILDER.termvec(), false); }

tuplevec_sem
    :                 tuple[b] SEM { $$ = BUILDER.termvec(BUILDER.termvec(), $b); }
    | tuplevec_sem[a] tuple[b] SEM { $$ = BUILDER.termvec($a, $b); }

tuplevec
    :                 tuple[b] { $$ = BUILDER.termvec(BUILDER.termvec(), $b); }
    | tuplevec_sem[a] tuple[b] { $$ = BUILDER.termvec($a, $b); }

argvec
    :               termvec[b] { $$ = BUILDER.termvecvec(BUILDER.termvecvec(), $b); }
    | argvec[a] SEM termvec[b] { $$ = BUILDER.termvecvec($a, $b); }
    ;

binaryargvec
    :                       term[a] COMMA term[b] { $$ = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), $a), $b)); }
    | binaryargvec[vec] SEM term[a] COMMA term[b] { $$ = BUILDER.termvecvec($vec, BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), $a), $b)); }
    ;

// TODO: I might have to create tuples differently
//       parse a tuple as a list of terms
//       each term is either a tuple or a term -> which afterwards is turned into a pool!

// {{{1 literals

cmp
    : GT     { $$ = Relation::GT; }
    | LT     { $$ = Relation::LT; }
    | GEQ    { $$ = Relation::GEQ; }
    | LEQ    { $$ = Relation::LEQ; }
    | EQ     { $$ = Relation::EQ; }
    | NEQ    { $$ = Relation::NEQ; }
    ;

atom
    : identifier[id]                                  { $$ = { $id, BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()) << 1u }; }
    | identifier[id] LPAREN argvec[tvv] RPAREN[r]     { $$ = { $id, $tvv << 1u }; }
    | SUB identifier[id]                              { $$ = { $id, BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()) << 1u | 1u }; }
    | SUB identifier[id] LPAREN argvec[tvv] RPAREN[r] { $$ = { $id, $tvv << 1u | 1u }; }
    ;

literal
    :         TRUE                     { $$ = BUILDER.boollit(@$, true); }
    |     NOT TRUE                     { $$ = BUILDER.boollit(@$, false); }
    | NOT NOT TRUE                     { $$ = BUILDER.boollit(@$, true); }
    |         FALSE                    { $$ = BUILDER.boollit(@$, false); }
    |     NOT FALSE                    { $$ = BUILDER.boollit(@$, true); }
    | NOT NOT FALSE                    { $$ = BUILDER.boollit(@$, false); }
    |         atom[a]                  { $$ = BUILDER.predlit(@$, NAF::POS, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u)); }
    |     NOT atom[a]                  { $$ = BUILDER.predlit(@$, NAF::NOT, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u)); }
    | NOT NOT atom[a]                  { $$ = BUILDER.predlit(@$, NAF::NOTNOT, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u)); }
    |         term[l] cmp[rel] term[r] { $$ = BUILDER.rellit(@$, $rel, $l, $r); }
    |     NOT term[l] cmp[rel] term[r] { $$ = BUILDER.rellit(@$, neg($rel), $l, $r); }
    | NOT NOT term[l] cmp[rel] term[r] { $$ = BUILDER.rellit(@$, $rel, $l, $r); }
    | csp_literal[lit]                 { $$ = BUILDER.csplit($lit); }
    ;

csp_mul_term
    : CSP term[var] CSP_MUL term[coe] { $$ = BUILDER.cspmulterm(@$, $coe,                     $var); }
    | term[coe] CSP_MUL CSP term[var] { $$ = BUILDER.cspmulterm(@$, $coe,                     $var); }
    | CSP term[var]                   { $$ = BUILDER.cspmulterm(@$, BUILDER.term(@$, Symbol::createNum(1)), $var); }
    | term[coe]                       { $$ = BUILDER.cspmulterm(@$, $coe); }
    ;

csp_add_term
    : csp_add_term[add] CSP_ADD csp_mul_term[mul] { $$ = BUILDER.cspaddterm(@$, $add, $mul, true); }
    | csp_add_term[add] CSP_SUB csp_mul_term[mul] { $$ = BUILDER.cspaddterm(@$, $add, $mul, false); }
    | csp_mul_term[mul]                           { $$ = BUILDER.cspaddterm(@$, $mul); }
    ;

csp_rel
    : CSP_GT  { $$ = Relation::GT; }
    | CSP_LT  { $$ = Relation::LT; }
    | CSP_GEQ { $$ = Relation::GEQ; }
    | CSP_LEQ { $$ = Relation::LEQ; }
    | CSP_EQ  { $$ = Relation::EQ; }
    | CSP_NEQ { $$ = Relation::NEQ; }
    ;

csp_literal 
    : csp_literal[lit] csp_rel[rel] csp_add_term[b] { $$ = BUILDER.csplit(@$, $lit, $rel, $b); }
    | csp_add_term[a]  csp_rel[rel] csp_add_term[b] { $$ = BUILDER.csplit(@$, $a,   $rel, $b); }
    ;

// {{{1 aggregates

// {{{2 auxiliary rules

nlitvec
    : literal[lit]                    { $$ = BUILDER.litvec(BUILDER.litvec(), $lit); }
    | nlitvec[vec] COMMA literal[lit] { $$ = BUILDER.litvec($vec, $lit); }
    ;

litvec
    : nlitvec[vec] { $$ = $vec; }
    |              { $$ = BUILDER.litvec(); }
    ;

optcondition
    : COLON litvec[vec] { $$ = $vec; }
    |                   { $$ = BUILDER.litvec(); }
    ;

noptcondition
    : COLON nlitvec[vec] { $$ = $vec; }
    |                    { $$ = BUILDER.litvec(); }
    ;

aggregatefunction
    : SUM   { $$ = AggregateFunction::SUM; }
    | SUMP  { $$ = AggregateFunction::SUMP; }
    | MIN   { $$ = AggregateFunction::MIN; }
    | MAX   { $$ = AggregateFunction::MAX; }
    | COUNT { $$ = AggregateFunction::COUNT; }
    ;

// {{{2 body aggregate elements

bodyaggrelem
    : COLON litvec[cond]                { $$ = { BUILDER.termvec(), $cond }; }
    | ntermvec[args] optcondition[cond] { $$ = { $args, $cond }; }
    ;

bodyaggrelemvec
    : bodyaggrelem[elem]                          { $$ = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), $elem.first, $elem.second); }
    | bodyaggrelemvec[vec] SEM bodyaggrelem[elem] { $$ = BUILDER.bodyaggrelemvec($vec, $elem.first, $elem.second); }
    ;

// Note: alternative syntax (without weight)

altbodyaggrelem
    : literal[lit] optcondition[cond] { $$ = { $lit, $cond }; }
    ;

altbodyaggrelemvec
    : altbodyaggrelem[elem]                             { $$ = BUILDER.condlitvec(BUILDER.condlitvec(), $elem.first, $elem.second); }
    | altbodyaggrelemvec[vec] SEM altbodyaggrelem[elem] { $$ = BUILDER.condlitvec($vec, $elem.first, $elem.second); }
    ;

// {{{2 body aggregates

bodyaggregate
    : LBRACE RBRACE                                               { $$ = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
    | LBRACE altbodyaggrelemvec[elems] RBRACE                     { $$ = { AggregateFunction::COUNT, true, $elems }; }
    | aggregatefunction[fun] LBRACE RBRACE                        { $$ = { $fun, false, BUILDER.bodyaggrelemvec() }; }
    | aggregatefunction[fun] LBRACE bodyaggrelemvec[elems] RBRACE { $$ = { $fun, false, $elems }; }
    ;

upper
    : term[t]          { $$ = { Relation::LEQ, $t }; }
    | cmp[rel] term[t] { $$ = { $rel, $t }; }
    |                  { $$ = { Relation::LEQ, TermUid(-1) }; }
    ;

lubodyaggregate
    : term[l]          bodyaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec(Relation::LEQ, $l, $u.rel, $u.term)); }
    | term[l] cmp[rel] bodyaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec($rel, $l, $u.rel, $u.term)); }
    |                  bodyaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec(Relation::LEQ, TermUid(-1), $u.rel, $u.term)); }
    | theory_atom[atom]                          { $$ = lexer->aggregate($atom); }
    ;

// {{{2 head aggregate elements

headaggrelemvec
    : headaggrelemvec[vec] SEM termvec[tuple] COLON literal[head] optcondition[cond] { $$ = BUILDER.headaggrelemvec($vec, $tuple, $head, $cond); }
    | termvec[tuple] COLON literal[head] optcondition[cond]                          { $$ = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), $tuple, $head, $cond); }
    ;

altheadaggrelemvec
    : literal[lit] optcondition[cond]                             { $$ = BUILDER.condlitvec(BUILDER.condlitvec(), $lit, $cond); }
    | altheadaggrelemvec[vec] SEM literal[lit] optcondition[cond] { $$ = BUILDER.condlitvec($vec, $lit, $cond); }
    ;

// {{{2 head aggregates

headaggregate
    : aggregatefunction[fun] LBRACE RBRACE                        { $$ = { $fun, false, BUILDER.headaggrelemvec() }; }
    | aggregatefunction[fun] LBRACE headaggrelemvec[elems] RBRACE { $$ = { $fun, false, $elems }; }
    | LBRACE RBRACE                                               { $$ = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
    | LBRACE altheadaggrelemvec[elems] RBRACE                     { $$ = { AggregateFunction::COUNT, true, $elems}; }
    ;

luheadaggregate
    : term[l]          headaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec(Relation::LEQ, $l, $u.rel, $u.term)); }
    | term[l] cmp[rel] headaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec($rel, $l, $u.rel, $u.term)); }
    |                  headaggregate[a] upper[u] { $$ = lexer->aggregate($a.fun, $a.choice, $a.elems, lexer->boundvec(Relation::LEQ, TermUid(-1), $u.rel, $u.term)); }
    | theory_atom[atom]                          { $$ = lexer->aggregate($atom); }
    ;

// {{{2 disjoint aggregate

ncspelemvec
    :                     termvec[tuple] COLON csp_add_term[add] optcondition[cond] { $$ = BUILDER.cspelemvec(BUILDER.cspelemvec(), @$, $tuple, $add, $cond); }
    | cspelemvec[vec] SEM termvec[tuple] COLON csp_add_term[add] optcondition[cond] { $$ = BUILDER.cspelemvec($vec, @$, $tuple, $add, $cond); }
    ;

cspelemvec
    : ncspelemvec[vec] { $$ = $vec; }
    |                  { $$ = BUILDER.cspelemvec(); }
    ;

disjoint
    :         DISJOINT LBRACE cspelemvec[elems] RBRACE { $$ = { NAF::POS, $elems }; }
    | NOT     DISJOINT LBRACE cspelemvec[elems] RBRACE { $$ = { NAF::NOT, $elems }; }
    | NOT NOT DISJOINT LBRACE cspelemvec[elems] RBRACE { $$ = { NAF::NOTNOT, $elems }; }
    ;

///}}}
// {{{2 conjunctions

conjunction
    : literal[lit] COLON litvec[cond] { $$ = { $lit, $cond }; }
    ;

// }}}
// {{{2 disjunctions

dsym
    : SEM
    | VBAR
    ;

disjunctionsep
    : disjunctionsep[vec] literal[lit] COMMA                    { $$ = BUILDER.condlitvec($vec, $lit, BUILDER.litvec()); }
    | disjunctionsep[vec] literal[lit] noptcondition[cond] dsym { $$ = BUILDER.condlitvec($vec, $lit, $cond); }
    |                                                           { $$ = BUILDER.condlitvec(); }
    ;

// Note: for simplicity appending first condlit here
disjunction
    : literal[lit] COMMA  disjunctionsep[vec] literal[clit] noptcondition[ccond]                    { $$ = BUILDER.condlitvec(BUILDER.condlitvec($vec, $clit, $ccond), $lit, BUILDER.litvec()); }
    | literal[lit] dsym    disjunctionsep[vec] literal[clit] noptcondition[ccond]                   { $$ = BUILDER.condlitvec(BUILDER.condlitvec($vec, $clit, $ccond), $lit, BUILDER.litvec()); }
    | literal[lit]  COLON nlitvec[cond] dsym disjunctionsep[vec] literal[clit] noptcondition[ccond] { $$ = BUILDER.condlitvec(BUILDER.condlitvec($vec, $clit, $ccond), $lit, $cond); }
    | literal[clit] COLON nlitvec[ccond]                                                            { $$ = BUILDER.condlitvec(BUILDER.condlitvec(), $clit, $ccond); }
    ;

// {{{1 statements
// {{{2 rules

bodycomma
    : bodycomma[body] literal[lit] COMMA                      { $$ = BUILDER.bodylit($body, $lit); }
    | bodycomma[body] literal[lit] SEM                        { $$ = BUILDER.bodylit($body, $lit); }
    | bodycomma[body] lubodyaggregate[aggr] COMMA             { $$ = lexer->bodyaggregate($body, @aggr, NAF::POS, $aggr); }
    | bodycomma[body] lubodyaggregate[aggr] SEM               { $$ = lexer->bodyaggregate($body, @aggr, NAF::POS, $aggr); }
    | bodycomma[body] NOT[l] lubodyaggregate[aggr] COMMA      { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOT, $aggr); }
    | bodycomma[body] NOT[l] lubodyaggregate[aggr] SEM        { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOT, $aggr); }
    | bodycomma[body] NOT[l] NOT lubodyaggregate[aggr] COMMA  { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOTNOT, $aggr); }
    | bodycomma[body] NOT[l] NOT lubodyaggregate[aggr] SEM    { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOTNOT, $aggr); }
    | bodycomma[body] conjunction[conj] SEM                   { $$ = BUILDER.conjunction($body, @conj, $conj.first, $conj.second); }
    | bodycomma[body] disjoint[cons] SEM                      { $$ = BUILDER.disjoint($body, @cons, $cons.first, $cons.second); }
    |                                                         { $$ = BUILDER.body(); }
    ;

bodydot
    : bodycomma[body] literal[lit] DOT                      { $$ = BUILDER.bodylit($body, $lit); }
    | bodycomma[body] lubodyaggregate[aggr] DOT             { $$ = lexer->bodyaggregate($body, @aggr, NAF::POS, $aggr); }
    | bodycomma[body] NOT[l] lubodyaggregate[aggr] DOT      { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOT, $aggr); }
    | bodycomma[body] NOT[l] NOT lubodyaggregate[aggr] DOT  { $$ = lexer->bodyaggregate($body, @aggr + @l, NAF::NOTNOT, $aggr); }
    | bodycomma[body] conjunction[conj] DOT                 { $$ = BUILDER.conjunction($body, @conj, $conj.first, $conj.second); }
    | bodycomma[body] disjoint[cons] DOT                    { $$ = BUILDER.disjoint($body, @cons, $cons.first, $cons.second); }
    ;

bodyconddot
    : DOT             { $$ = BUILDER.body(); }
    | COLON DOT       { $$ = BUILDER.body(); }
    | COLON bodydot[body]   { $$ = $body; }

head
    : literal[lit]            { $$ = BUILDER.headlit($lit); }
    | disjunction[elems]      { $$ = BUILDER.disjunction(@$, $elems); }
    | luheadaggregate[aggr]   { $$ = lexer->headaggregate(@$, $aggr); }
    ;

statement
    : head[hd] DOT            { BUILDER.rule(@$, $hd); }
    | head[hd] IF DOT         { BUILDER.rule(@$, $hd); }
    | head[hd] IF bodydot[bd] { BUILDER.rule(@$, $hd, $bd); }
    | IF bodydot[bd]          { BUILDER.rule(@$, BUILDER.headlit(BUILDER.boollit(@$, false)), $bd); }
    | IF DOT                  { BUILDER.rule(@$, BUILDER.headlit(BUILDER.boollit(@$, false)), BUILDER.body()); }
    ;

// {{{2 CSP

statement
    : disjoint[hd] IF bodydot[body] { BUILDER.rule(@$, BUILDER.headlit(BUILDER.boollit(@hd, false)), BUILDER.disjoint($body, @hd, inv($hd.first), $hd.second)); }
    | disjoint[hd] IF DOT           { BUILDER.rule(@$, BUILDER.headlit(BUILDER.boollit(@hd, false)), BUILDER.disjoint(BUILDER.body(), @hd, inv($hd.first), $hd.second)); }
    | disjoint[hd] DOT              { BUILDER.rule(@$, BUILDER.headlit(BUILDER.boollit(@hd, false)), BUILDER.disjoint(BUILDER.body(), @hd, inv($hd.first), $hd.second)); }
    ;

// {{{2 optimization

optimizetuple
    : COMMA ntermvec[vec] { $$ = $vec; }
    |                     { $$ = BUILDER.termvec(); }
    ;

optimizeweight
    : term[w] AT term[p] { $$ = {$w, $p}; }
    | term[w]            { $$ = {$w, BUILDER.term(@$, Symbol::createNum(0))}; }
    ;

optimizelitvec
    : literal[lit]                          { $$ = BUILDER.bodylit(BUILDER.body(), $lit); }
    | optimizelitvec[bd] COMMA literal[lit] { $$ = BUILDER.bodylit($bd, $lit); }
    ;

optimizecond
    : COLON optimizelitvec[bd] { $$ = $bd; }
    | COLON                    { $$ = BUILDER.body(); }
    |                          { $$ = BUILDER.body(); }
    ;

statement
    : WIF bodydot[bd] LBRACK optimizeweight[w] optimizetuple[t] RBRACK { BUILDER.optimize(@$, $w.first, $w.second, $t, $bd); }
    | WIF         DOT LBRACK optimizeweight[w] optimizetuple[t] RBRACK { BUILDER.optimize(@$, $w.first, $w.second, $t, BUILDER.body()); }
    ;

maxelemlist
    :                 optimizeweight[w] optimizetuple[t] optimizecond[bd] { BUILDER.optimize(@$, BUILDER.term(@w, UnOp::NEG, $w.first), $w.second, $t, $bd); }
    | maxelemlist SEM optimizeweight[w] optimizetuple[t] optimizecond[bd] { BUILDER.optimize(@$, BUILDER.term(@w, UnOp::NEG, $w.first), $w.second, $t, $bd); }
    ;

minelemlist
    :                 optimizeweight[w] optimizetuple[t] optimizecond[bd] { BUILDER.optimize(@$, $w.first, $w.second, $t, $bd); }
    | minelemlist SEM optimizeweight[w] optimizetuple[t] optimizecond[bd] { BUILDER.optimize(@$, $w.first, $w.second, $t, $bd); }
    ;

statement
    : MINIMIZE LBRACE RBRACE DOT
    | MAXIMIZE LBRACE RBRACE DOT
    | MINIMIZE LBRACE minelemlist RBRACE DOT
    | MAXIMIZE LBRACE maxelemlist RBRACE DOT
    ;

// {{{2 visibility

statement
    : SHOWSIG identifier[id] SLASH NUMBER[num] DOT     { BUILDER.showsig(@$, Sig(String::fromRep($id), $num, false), false); }
    | SHOWSIG SUB identifier[id] SLASH NUMBER[num] DOT { BUILDER.showsig(@$, Sig(String::fromRep($id), $num, true), false); }
    | SHOW DOT                                         { BUILDER.showsig(@$, Sig("", 0, false), false); }
    | SHOW term[t] COLON bodydot[bd]                   { BUILDER.show(@$, $t, $bd, false); }
    | SHOW term[t] DOT                                 { BUILDER.show(@$, $t, BUILDER.body(), false); }
    | SHOWSIG CSP identifier[id] SLASH NUMBER[num] DOT { BUILDER.showsig(@$, Sig(String::fromRep($id), $num, false), true); }
    | SHOW CSP term[t] COLON bodydot[bd]               { BUILDER.show(@$, $t, $bd, true); }
    | SHOW CSP term[t] DOT                             { BUILDER.show(@$, $t, BUILDER.body(), true); }
    ;

// {{{2 acyclicity

statement
    : EDGE LPAREN binaryargvec[args] RPAREN bodyconddot[body] { BUILDER.edge(@$, $args, $body); }
    ;

// {{{2 heuristic

statement
    : HEURISTIC atom[a] bodyconddot[body] LBRACK term[t] AT term[p] COMMA term[mod] RBRACK { BUILDER.heuristic(@$, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u), $body, $t, $p, $mod); }
    | HEURISTIC atom[a] bodyconddot[body] LBRACK term[t]            COMMA term[mod] RBRACK { BUILDER.heuristic(@$, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u), $body, $t, BUILDER.term(@$, Symbol::createNum(0)), $mod); }
    ;

// {{{2 project

statement
    : PROJECT identifier[name] SLASH NUMBER[arity] DOT     { BUILDER.project(@$, Sig(String::fromRep($name), $arity, false)); }
    | PROJECT SUB identifier[name] SLASH NUMBER[arity] DOT { BUILDER.project(@$, Sig(String::fromRep($name), $arity, true)); }
    | PROJECT atom[a] bodyconddot[body]                    { BUILDER.project(@$, $a.second & 1, String::fromRep($a.first), TermVecVecUid($a.second >> 1u), $body); }
    ;

// {{{2 constants

define
    : identifier[uid] EQ constterm[rhs] {  BUILDER.define(@$, String::fromRep($uid), $rhs, false, LOGGER); }
    ;

statement 
    : CONST identifier[uid] EQ constterm[rhs] DOT {  BUILDER.define(@$, String::fromRep($uid), $rhs, true, LOGGER); }
    ;

// {{{2 scripts

statement
    : PYTHON[code] DOT { BUILDER.python(@$, String::fromRep($code)); }
    | LUA[code]    DOT { BUILDER.lua(@$, String::fromRep($code)); }
    ;

// {{{2 include

statement
    : INCLUDE    STRING[file]        DOT { lexer->include(String::fromRep($file), @$, false, LOGGER); }
    | INCLUDE LT identifier[file] GT DOT { lexer->include(String::fromRep($file), @$, true, LOGGER); }
    ;

// {{{2 blocks

nidlist 
    : nidlist[list] COMMA identifier[id] { $$ = BUILDER.idvec($list, @id, String::fromRep($id)); }
    | identifier[id]                     { $$ = BUILDER.idvec(BUILDER.idvec(), @id, String::fromRep($id)); }
    ;

idlist 
    :               { $$ = BUILDER.idvec(); }
    | nidlist[list] { $$ = $list; }
    ;

statement
    : BLOCK identifier[name] LPAREN idlist[args] RPAREN DOT { BUILDER.block(@$, String::fromRep($name), $args); }
    | BLOCK identifier[name] DOT                            { BUILDER.block(@$, String::fromRep($name), BUILDER.idvec()); }
    ;

// {{{2 external

statement
    : EXTERNAL atom[hd] COLON bodydot[bd] { BUILDER.external(@$, BUILDER.predlit(@hd, NAF::POS, $hd.second & 1, String::fromRep($hd.first), TermVecVecUid($hd.second >> 1u)), $bd); }
    | EXTERNAL atom[hd] COLON DOT         { BUILDER.external(@$, BUILDER.predlit(@hd, NAF::POS, $hd.second & 1, String::fromRep($hd.first), TermVecVecUid($hd.second >> 1u)), BUILDER.body()); }
    | EXTERNAL atom[hd] DOT               { BUILDER.external(@$, BUILDER.predlit(@hd, NAF::POS, $hd.second & 1, String::fromRep($hd.first), TermVecVecUid($hd.second >> 1u)), BUILDER.body()); }
    ;

// {{{1 theory

// {{{2 theory atoms

theory_op_list
    : theory_op_list[ops] THEORY_OP[op] { $$ = BUILDER.theoryops($ops, String::fromRep($op)); }
    | THEORY_OP[op]                     { $$ = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep($op)); }
    ;

theory_term
    : LBRACE theory_opterm_list[list] RBRACE                              { $$ = BUILDER.theorytermset(@$, $list); }
    | LBRACK theory_opterm_list[list] RBRACK                              { $$ = BUILDER.theoryoptermlist(@$, $list); }
    | LPAREN RPAREN                                                       { $$ = BUILDER.theorytermtuple(@$, BUILDER.theoryopterms()); }
    | LPAREN theory_opterm[term] RPAREN                                   { $$ = BUILDER.theorytermopterm(@$, $term); }
    | LPAREN theory_opterm[opterm] COMMA RPAREN                           { $$ = BUILDER.theorytermtuple(@$, BUILDER.theoryopterms(BUILDER.theoryopterms(), $opterm)); }
    | LPAREN theory_opterm[opterm] COMMA theory_opterm_nlist[list] RPAREN { $$ = BUILDER.theorytermtuple(@$, BUILDER.theoryopterms($opterm, $list)); }
    | identifier[id] LPAREN theory_opterm_list[list] RPAREN               { $$ = BUILDER.theorytermfun(@$, String::fromRep($id), $list); }
    | identifier[id]                                                      { $$ = BUILDER.theorytermvalue(@$, Symbol::createId(String::fromRep($id))); }
    | NUMBER[num]                                                         { $$ = BUILDER.theorytermvalue(@$, Symbol::createNum($num)); }
    | STRING[str]                                                         { $$ = BUILDER.theorytermvalue(@$, Symbol::createStr(String::fromRep($str))); }
    | INFIMUM                                                             { $$ = BUILDER.theorytermvalue(@$, Symbol::createInf()); }
    | SUPREMUM                                                            { $$ = BUILDER.theorytermvalue(@$, Symbol::createSup()); }
    | VARIABLE[var]                                                       { $$ = BUILDER.theorytermvar(@$, String::fromRep($var)); }
    ;

theory_opterm
    : theory_opterm[opterm] theory_op_list[ops] theory_term[term] { $$ = BUILDER.theoryopterm($opterm, $ops, $term); }
    | theory_op_list[ops] theory_term[term]                       { $$ = BUILDER.theoryopterm($ops, $term); }
    | theory_term[term]                                           { $$ = BUILDER.theoryopterm(BUILDER.theoryops(), $term); }
    ;

theory_opterm_nlist
    : theory_opterm_nlist[list] COMMA theory_opterm[opterm] { $$ = BUILDER.theoryopterms($list, $opterm); }
    | theory_opterm[opterm]                                 { $$ = BUILDER.theoryopterms(BUILDER.theoryopterms(), $opterm); }
    ;

theory_opterm_list
    : theory_opterm_nlist[list] { $$ = $list; }
    |                           { $$ = BUILDER.theoryopterms(); }
    ;

theory_atom_element 
    : theory_opterm_nlist[list] disable_theory_lexing optcondition[cond] { $$ = { $list, $cond }; }
    |                           disable_theory_lexing COLON litvec[cond] { $$ = { BUILDER.theoryopterms(), $cond }; }
    ;

theory_atom_element_nlist
    : theory_atom_element_nlist[list] enable_theory_lexing SEM theory_atom_element[elem] { $$ = BUILDER.theoryelems($list, $elem.first, $elem.second); }
    | theory_atom_element[elem]                                                          { $$ = BUILDER.theoryelems(BUILDER.theoryelems(), $elem.first, $elem.second); }
    ;

theory_atom_element_list
    : theory_atom_element_nlist[list] { $$ = $list; }
    |                                 { $$ = BUILDER.theoryelems(); }
    ;

theory_atom_name
    : identifier[id]                                  { $$ = BUILDER.term(@$, String::fromRep($id), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), false); }
    | identifier[id] LPAREN argvec[tvv] RPAREN[r]     { $$ = BUILDER.term(@$, String::fromRep($id), $tvv, false); }

theory_atom
    : AND theory_atom_name[name] enable_theory_lexing LBRACE theory_atom_element_list[elems] enable_theory_lexing RBRACE                                     disable_theory_lexing { $$ = BUILDER.theoryatom($name, $elems); }
    | AND theory_atom_name[name] enable_theory_lexing LBRACE theory_atom_element_list[elems] enable_theory_lexing RBRACE THEORY_OP[op] theory_opterm[opterm] disable_theory_lexing { $$ = BUILDER.theoryatom($name, $elems, String::fromRep($op), $opterm); }
    ;

// {{{2 theory definition

theory_operator_nlist
    : THEORY_OP[op]                                  { $$ = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep($op)); }
    | theory_operator_nlist[ops] COMMA THEORY_OP[op] { $$ = BUILDER.theoryops($ops, String::fromRep($op)); }
    ;

theory_operator_list
    : theory_operator_nlist[ops] { $$ = $ops; }
    |                            { $$ = BUILDER.theoryops(); }
    ;

theory_operator_definition
    : THEORY_OP[op] enable_theory_definition_lexing COLON NUMBER[arity] COMMA UNARY              { $$ = BUILDER.theoryopdef(@$, String::fromRep($op), $arity, TheoryOperatorType::Unary); }
    | THEORY_OP[op] enable_theory_definition_lexing COLON NUMBER[arity] COMMA BINARY COMMA LEFT  { $$ = BUILDER.theoryopdef(@$, String::fromRep($op), $arity, TheoryOperatorType::BinaryLeft); }
    | THEORY_OP[op] enable_theory_definition_lexing COLON NUMBER[arity] COMMA BINARY COMMA RIGHT { $$ = BUILDER.theoryopdef(@$, String::fromRep($op), $arity, TheoryOperatorType::BinaryRight); }
    ;

theory_operator_definition_nlist
    : theory_operator_definition[def]                                                                 { $$ = BUILDER.theoryopdefs(BUILDER.theoryopdefs(), $def); }
    | theory_operator_definition_nlist[defs] enable_theory_lexing SEM theory_operator_definition[def] { $$ = BUILDER.theoryopdefs($defs, $def); }
    ;

theory_operator_definiton_list
    : theory_operator_definition_nlist[defs] { $$ = $defs; }
    |                                        { $$ = BUILDER.theoryopdefs(); }
    ;

theory_definition_identifier
    : identifier[id] { $$ = $id; }
    | LEFT           { $$ = String::toRep("left"); }
    | RIGHT          { $$ = String::toRep("right"); }
    | UNARY          { $$ = String::toRep("unary"); }
    | BINARY         { $$ = String::toRep("binary"); }
    | HEAD           { $$ = String::toRep("head"); }
    | BODY           { $$ = String::toRep("body"); }
    | ANY            { $$ = String::toRep("any"); }
    | DIRECTIVE      { $$ = String::toRep("directive"); }
    ;

theory_term_definition
    : theory_definition_identifier[name] enable_theory_lexing LBRACE theory_operator_definiton_list[ops] enable_theory_definition_lexing RBRACE { $$ = BUILDER.theorytermdef(@$, String::fromRep($name), $ops, LOGGER); }
    ;

theory_atom_type
    : HEAD      { $$ = TheoryAtomType::Head; }
    | BODY      { $$ = TheoryAtomType::Body; }
    | ANY       { $$ = TheoryAtomType::Any; }
    | DIRECTIVE { $$ = TheoryAtomType::Directive; }
    ;

theory_atom_definition
    : AND theory_definition_identifier[name] SLASH NUMBER[arity] COLON theory_definition_identifier[termdef] COMMA
      enable_theory_lexing LBRACE theory_operator_list[ops] enable_theory_definition_lexing RBRACE COMMA theory_definition_identifier[guarddef] COMMA theory_atom_type[type] { $$ = BUILDER.theoryatomdef(@$, String::fromRep($name), $arity, String::fromRep($termdef), $type, $ops, String::fromRep($guarddef)); }
    | AND theory_definition_identifier[name] SLASH NUMBER[arity] COLON theory_definition_identifier[termdef] COMMA                                    theory_atom_type[type] { $$ = BUILDER.theoryatomdef(@$, String::fromRep($name), $arity, String::fromRep($termdef), $type); }
    ;

theory_definition_nlist
    : theory_definition_list[defs] SEM theory_atom_definition[def] { $$ = BUILDER.theorydefs($defs, $def); }
    | theory_definition_list[defs] SEM theory_term_definition[def] { $$ = BUILDER.theorydefs($defs, $def); }
    | theory_atom_definition[def] { $$ = BUILDER.theorydefs(BUILDER.theorydefs(), $def); }
    | theory_term_definition[def] { $$ = BUILDER.theorydefs(BUILDER.theorydefs(), $def); }
    ;

theory_definition_list
    : theory_definition_nlist[defs] { $$ = $defs; }
    |                               { $$ = BUILDER.theorydefs(); }
    ;

statement
    : THEORY identifier[name] enable_theory_definition_lexing LBRACE theory_definition_list[defs] RBRACE disable_theory_lexing DOT { BUILDER.theorydef(@$, String::fromRep($name), $defs, LOGGER); }
    ;

// {{{2 lexing

enable_theory_lexing
    : { lexer->theoryLexing(TheoryLexing::Theory); }
    ;

enable_theory_definition_lexing
    : { lexer->theoryLexing(TheoryLexing::Definition); }
    ;

disable_theory_lexing
    : { lexer->theoryLexing(TheoryLexing::Disabled); }
    ;

// }}}1
