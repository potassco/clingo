// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.


/**
 ** \file /home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh
 ** Define the Gringo::Input::NonGroundGrammar::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_GRINGONONGROUNDGRAMMAR_HOME_KAMINSKI_DOCUMENTS_GIT_POTASSCO_CLINGO_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
# define YY_GRINGONONGROUNDGRAMMAR_HOME_KAMINSKI_DOCUMENTS_GIT_POTASSCO_CLINGO_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
// "%code requires" blocks.
#line 46 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"

    #include "gringo/input/programbuilder.hh"
    #include "potassco/basic_types.h"

    namespace Gringo { namespace Input { class NonGroundParser; } }
    
    struct DefaultLocation : Gringo::Location {
        DefaultLocation() : Location("<undef>", 0, 0, "<undef>", 0, 0) { }
    };


#line 61 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh"


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif



#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
namespace Gringo { namespace Input { namespace NonGroundGrammar {
#line 197 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh"




  /// A Bison parser.
  class parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
    /// Symbol semantic values.
    union value_type
    {
#line 108 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"

    IdVecUid idlist;
    TermUid term;
    TermVecUid termvec;
    TermVecVecUid termvecvec;
    LitVecUid litvec;
    LitUid lit;
    RelLitVecUid rellitvec;
    BdAggrElemVecUid bodyaggrelemvec;
    CondLitVecUid condlitlist;
    HdAggrElemVecUid headaggrelemvec;
    BoundVecUid bounds;
    BdLitVecUid body;
    HdLitUid head;
    Relation rel;
    AggregateFunction fun;
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

#line 278 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh"

    };
#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;

    /// Symbol locations.
    typedef DefaultLocation location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m)
        : std::runtime_error (m)
        , location (l)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
        , location (s.location)
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;

      location_type location;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    END = 0,                       // "<EOF>"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    ADD = 258,                     // "+"
    AND = 259,                     // "&"
    EQ = 260,                      // "="
    AT = 261,                      // "@"
    BASE = 262,                    // "#base"
    BNOT = 263,                    // "~"
    COLON = 264,                   // ":"
    COMMA = 265,                   // ","
    CONST = 266,                   // "#const"
    COUNT = 267,                   // "#count"
    CUMULATIVE = 268,              // "#cumulative"
    DOT = 269,                     // "."
    DOTS = 270,                    // ".."
    EXTERNAL = 271,                // "#external"
    DEFINED = 272,                 // "#defined"
    FALSE = 273,                   // "#false"
    FORGET = 274,                  // "#forget"
    GEQ = 275,                     // ">="
    GT = 276,                      // ">"
    IF = 277,                      // ":-"
    INCLUDE = 278,                 // "#include"
    INFIMUM = 279,                 // "#inf"
    LBRACE = 280,                  // "{"
    LBRACK = 281,                  // "["
    LEQ = 282,                     // "<="
    LPAREN = 283,                  // "("
    LT = 284,                      // "<"
    MAX = 285,                     // "#max"
    MAXIMIZE = 286,                // "#maximize"
    MIN = 287,                     // "#min"
    MINIMIZE = 288,                // "#minimize"
    MOD = 289,                     // "\\"
    MUL = 290,                     // "*"
    NEQ = 291,                     // "!="
    POW = 292,                     // "**"
    QUESTION = 293,                // "?"
    RBRACE = 294,                  // "}"
    RBRACK = 295,                  // "]"
    RPAREN = 296,                  // ")"
    SEM = 297,                     // ";"
    SHOW = 298,                    // "#show"
    EDGE = 299,                    // "#edge"
    PROJECT = 300,                 // "#project"
    HEURISTIC = 301,               // "#heuristic"
    SHOWSIG = 302,                 // "#showsig"
    SLASH = 303,                   // "/"
    SUB = 304,                     // "-"
    SUM = 305,                     // "#sum"
    SUMP = 306,                    // "#sum+"
    SUPREMUM = 307,                // "#sup"
    TRUE = 308,                    // "#true"
    BLOCK = 309,                   // "#program"
    UBNOT = 310,                   // UBNOT
    UMINUS = 311,                  // UMINUS
    VBAR = 312,                    // "|"
    VOLATILE = 313,                // "#volatile"
    WIF = 314,                     // ":~"
    XOR = 315,                     // "^"
    PARSE_LP = 316,                // "<program>"
    PARSE_DEF = 317,               // "<define>"
    ANY = 318,                     // "any"
    UNARY = 319,                   // "unary"
    BINARY = 320,                  // "binary"
    LEFT = 321,                    // "left"
    RIGHT = 322,                   // "right"
    HEAD = 323,                    // "head"
    BODY = 324,                    // "body"
    DIRECTIVE = 325,               // "directive"
    THEORY = 326,                  // "#theory"
    SYNC = 327,                    // "EOF"
    NUMBER = 328,                  // "<NUMBER>"
    ANONYMOUS = 329,               // "<ANONYMOUS>"
    IDENTIFIER = 330,              // "<IDENTIFIER>"
    SCRIPT = 331,                  // "<SCRIPT>"
    CODE = 332,                    // "<CODE>"
    STRING = 333,                  // "<STRING>"
    VARIABLE = 334,                // "<VARIABLE>"
    THEORY_OP = 335,               // "<THEORYOP>"
    NOT = 336,                     // "not"
    DEFAULT = 337,                 // "default"
    OVERRIDE = 338                 // "override"
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 84, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "<EOF>"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_ADD = 3,                               // "+"
        S_AND = 4,                               // "&"
        S_EQ = 5,                                // "="
        S_AT = 6,                                // "@"
        S_BASE = 7,                              // "#base"
        S_BNOT = 8,                              // "~"
        S_COLON = 9,                             // ":"
        S_COMMA = 10,                            // ","
        S_CONST = 11,                            // "#const"
        S_COUNT = 12,                            // "#count"
        S_CUMULATIVE = 13,                       // "#cumulative"
        S_DOT = 14,                              // "."
        S_DOTS = 15,                             // ".."
        S_EXTERNAL = 16,                         // "#external"
        S_DEFINED = 17,                          // "#defined"
        S_FALSE = 18,                            // "#false"
        S_FORGET = 19,                           // "#forget"
        S_GEQ = 20,                              // ">="
        S_GT = 21,                               // ">"
        S_IF = 22,                               // ":-"
        S_INCLUDE = 23,                          // "#include"
        S_INFIMUM = 24,                          // "#inf"
        S_LBRACE = 25,                           // "{"
        S_LBRACK = 26,                           // "["
        S_LEQ = 27,                              // "<="
        S_LPAREN = 28,                           // "("
        S_LT = 29,                               // "<"
        S_MAX = 30,                              // "#max"
        S_MAXIMIZE = 31,                         // "#maximize"
        S_MIN = 32,                              // "#min"
        S_MINIMIZE = 33,                         // "#minimize"
        S_MOD = 34,                              // "\\"
        S_MUL = 35,                              // "*"
        S_NEQ = 36,                              // "!="
        S_POW = 37,                              // "**"
        S_QUESTION = 38,                         // "?"
        S_RBRACE = 39,                           // "}"
        S_RBRACK = 40,                           // "]"
        S_RPAREN = 41,                           // ")"
        S_SEM = 42,                              // ";"
        S_SHOW = 43,                             // "#show"
        S_EDGE = 44,                             // "#edge"
        S_PROJECT = 45,                          // "#project"
        S_HEURISTIC = 46,                        // "#heuristic"
        S_SHOWSIG = 47,                          // "#showsig"
        S_SLASH = 48,                            // "/"
        S_SUB = 49,                              // "-"
        S_SUM = 50,                              // "#sum"
        S_SUMP = 51,                             // "#sum+"
        S_SUPREMUM = 52,                         // "#sup"
        S_TRUE = 53,                             // "#true"
        S_BLOCK = 54,                            // "#program"
        S_UBNOT = 55,                            // UBNOT
        S_UMINUS = 56,                           // UMINUS
        S_VBAR = 57,                             // "|"
        S_VOLATILE = 58,                         // "#volatile"
        S_WIF = 59,                              // ":~"
        S_XOR = 60,                              // "^"
        S_PARSE_LP = 61,                         // "<program>"
        S_PARSE_DEF = 62,                        // "<define>"
        S_ANY = 63,                              // "any"
        S_UNARY = 64,                            // "unary"
        S_BINARY = 65,                           // "binary"
        S_LEFT = 66,                             // "left"
        S_RIGHT = 67,                            // "right"
        S_HEAD = 68,                             // "head"
        S_BODY = 69,                             // "body"
        S_DIRECTIVE = 70,                        // "directive"
        S_THEORY = 71,                           // "#theory"
        S_SYNC = 72,                             // "EOF"
        S_NUMBER = 73,                           // "<NUMBER>"
        S_ANONYMOUS = 74,                        // "<ANONYMOUS>"
        S_IDENTIFIER = 75,                       // "<IDENTIFIER>"
        S_SCRIPT = 76,                           // "<SCRIPT>"
        S_CODE = 77,                             // "<CODE>"
        S_STRING = 78,                           // "<STRING>"
        S_VARIABLE = 79,                         // "<VARIABLE>"
        S_THEORY_OP = 80,                        // "<THEORYOP>"
        S_NOT = 81,                              // "not"
        S_DEFAULT = 82,                          // "default"
        S_OVERRIDE = 83,                         // "override"
        S_YYACCEPT = 84,                         // $accept
        S_start = 85,                            // start
        S_program = 86,                          // program
        S_statement = 87,                        // statement
        S_identifier = 88,                       // identifier
        S_constterm = 89,                        // constterm
        S_consttermvec = 90,                     // consttermvec
        S_constargvec = 91,                      // constargvec
        S_term = 92,                             // term
        S_unaryargvec = 93,                      // unaryargvec
        S_ntermvec = 94,                         // ntermvec
        S_termvec = 95,                          // termvec
        S_tuple = 96,                            // tuple
        S_tuplevec_sem = 97,                     // tuplevec_sem
        S_tuplevec = 98,                         // tuplevec
        S_argvec = 99,                           // argvec
        S_binaryargvec = 100,                    // binaryargvec
        S_cmp = 101,                             // cmp
        S_atom = 102,                            // atom
        S_rellitvec = 103,                       // rellitvec
        S_literal = 104,                         // literal
        S_nlitvec = 105,                         // nlitvec
        S_litvec = 106,                          // litvec
        S_optcondition = 107,                    // optcondition
        S_aggregatefunction = 108,               // aggregatefunction
        S_bodyaggrelem = 109,                    // bodyaggrelem
        S_bodyaggrelemvec = 110,                 // bodyaggrelemvec
        S_altbodyaggrelem = 111,                 // altbodyaggrelem
        S_altbodyaggrelemvec = 112,              // altbodyaggrelemvec
        S_bodyaggregate = 113,                   // bodyaggregate
        S_upper = 114,                           // upper
        S_lubodyaggregate = 115,                 // lubodyaggregate
        S_headaggrelemvec = 116,                 // headaggrelemvec
        S_altheadaggrelemvec = 117,              // altheadaggrelemvec
        S_headaggregate = 118,                   // headaggregate
        S_luheadaggregate = 119,                 // luheadaggregate
        S_conjunction = 120,                     // conjunction
        S_dsym = 121,                            // dsym
        S_disjunctionsep = 122,                  // disjunctionsep
        S_disjunction = 123,                     // disjunction
        S_bodycomma = 124,                       // bodycomma
        S_bodydot = 125,                         // bodydot
        S_bodyconddot = 126,                     // bodyconddot
        S_head = 127,                            // head
        S_optimizetuple = 128,                   // optimizetuple
        S_optimizeweight = 129,                  // optimizeweight
        S_optimizelitvec = 130,                  // optimizelitvec
        S_optimizecond = 131,                    // optimizecond
        S_maxelemlist = 132,                     // maxelemlist
        S_minelemlist = 133,                     // minelemlist
        S_define = 134,                          // define
        S_nidlist = 135,                         // nidlist
        S_idlist = 136,                          // idlist
        S_theory_op = 137,                       // theory_op
        S_theory_op_list = 138,                  // theory_op_list
        S_theory_term = 139,                     // theory_term
        S_theory_opterm = 140,                   // theory_opterm
        S_theory_opterm_nlist = 141,             // theory_opterm_nlist
        S_theory_opterm_list = 142,              // theory_opterm_list
        S_theory_atom_element = 143,             // theory_atom_element
        S_theory_atom_element_nlist = 144,       // theory_atom_element_nlist
        S_theory_atom_element_list = 145,        // theory_atom_element_list
        S_theory_atom_name = 146,                // theory_atom_name
        S_theory_atom = 147,                     // theory_atom
        S_theory_operator_nlist = 148,           // theory_operator_nlist
        S_theory_operator_list = 149,            // theory_operator_list
        S_theory_operator_definition = 150,      // theory_operator_definition
        S_theory_operator_definition_nlist = 151, // theory_operator_definition_nlist
        S_theory_operator_definiton_list = 152,  // theory_operator_definiton_list
        S_theory_definition_identifier = 153,    // theory_definition_identifier
        S_theory_term_definition = 154,          // theory_term_definition
        S_theory_atom_type = 155,                // theory_atom_type
        S_theory_atom_definition = 156,          // theory_atom_definition
        S_theory_definition_nlist = 157,         // theory_definition_nlist
        S_theory_definition_list = 158,          // theory_definition_list
        S_enable_theory_lexing = 159,            // enable_theory_lexing
        S_enable_theory_definition_lexing = 160, // enable_theory_definition_lexing
        S_disable_theory_lexing = 161            // disable_theory_lexing
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
        , location ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value (std::move (that.value))
        , location (std::move (that.location))
      {}
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);
      /// Constructor for valueless symbols.
      basic_symbol (typename Base::kind_type t,
                    YY_MOVE_REF (location_type) l);

      /// Constructor for symbols with semantic value.
      basic_symbol (typename Base::kind_type t,
                    YY_RVREF (value_type) v,
                    YY_RVREF (location_type) l);

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        Base::clear ();
      }

      /// The user-facing name of this symbol.
      std::string name () const YY_NOEXCEPT
      {
        return parser::symbol_name (this->kind ());
      }

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

      /// The location.
      location_type location;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {};

    /// Build a parser object.
    parser (Gringo::Input::NonGroundParser *lexer_yyarg);
    virtual ~parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    parser (const parser&) = delete;
    /// Non copyable.
    parser& operator= (const parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param loc    where the syntax error is found.
    /// \param msg    a description of the syntax error.
    virtual void error (const location_type& loc, const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static std::string symbol_name (symbol_kind_type yysymbol);



    class context
    {
    public:
      context (const parser& yyparser, const symbol_type& yyla);
      const symbol_type& lookahead () const YY_NOEXCEPT { return yyla_; }
      symbol_kind_type token () const YY_NOEXCEPT { return yyla_.kind (); }
      const location_type& location () const YY_NOEXCEPT { return yyla_.location; }

      /// Put in YYARG at most YYARGN of the expected tokens, and return the
      /// number of tokens stored in YYARG.  If YYARG is null, return the
      /// number of expected tokens (guaranteed to be less than YYNTOKENS).
      int expected_tokens (symbol_kind_type yyarg[], int yyargn) const;

    private:
      const parser& yyparser_;
      const symbol_type& yyla_;
    };

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    parser (const parser&);
    /// Non copyable.
    parser& operator= (const parser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef short state_type;

    /// The arguments of the error message.
    int yy_syntax_error_arguments_ (const context& yyctx,
                                    symbol_kind_type yyarg[], int yyargn) const;

    /// Generate an error message.
    /// \param yyctx     the context in which the error occurred.
    virtual std::string yysyntax_error_ (const context& yyctx) const;
    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const short yypact_ninf_;
    static const short yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *yystr);

    /// For a symbol, its name in clear.
    static const char* const yytname_[];


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const short yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const short yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const unsigned char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const unsigned char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 2042,     ///< Last index in yytable_.
      yynnts_ = 78,  ///< Number of nonterminal symbols.
      yyfinal_ = 10 ///< Termination state number.
    };


    // User arguments.
    Gringo::Input::NonGroundParser *lexer;

  };


#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
} } } // Gringo::Input::NonGroundGrammar
#line 1101 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh"




#endif // !YY_GRINGONONGROUNDGRAMMAR_HOME_KAMINSKI_DOCUMENTS_GIT_POTASSCO_CLINGO_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
