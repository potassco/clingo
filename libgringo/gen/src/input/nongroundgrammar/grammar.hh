// A Bison parser, made by GNU Bison 3.0.2.

// Skeleton interface for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2013 Free Software Foundation, Inc.

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
 ** \file /tmp/clingo-release/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh
 ** Define the Gringo::Input::NonGroundGrammar::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

#ifndef YY_GRINGONONGROUNDGRAMMAR_TMP_CLINGO_RELEASE_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
# define YY_GRINGONONGROUNDGRAMMAR_TMP_CLINGO_RELEASE_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
// //                    "%code requires" blocks.
#line 46 "/tmp/clingo-release/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:372

    #include "gringo/input/programbuilder.hh"
    #include "potassco/basic_types.h"

    namespace Gringo { namespace Input { class NonGroundParser; } }
    
    struct DefaultLocation : Gringo::Location {
        DefaultLocation() : Location("<undef>", 0, 0, "<undef>", 0, 0) { }
    };


#line 56 "/tmp/clingo-release/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh" // lalr1.cc:372


# include <vector>
# include <iostream>
# include <stdexcept>
# include <string>
# include "stack.hh"



#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
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

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 28 "/tmp/clingo-release/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:372
namespace Gringo { namespace Input { namespace NonGroundGrammar {
#line 127 "/tmp/clingo-release/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh" // lalr1.cc:372





  /// A Bison parser.
  class parser
  {
  public:
#ifndef YYSTYPE
    /// Symbol semantic values.
    union semantic_type
    {
    #line 108 "/tmp/clingo-release/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:372

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

#line 211 "/tmp/clingo-release/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh" // lalr1.cc:372
    };
#else
    typedef YYSTYPE semantic_type;
#endif
    /// Symbol locations.
    typedef DefaultLocation location_type;

    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const location_type& l, const std::string& m);
      location_type location;
    };

    /// Tokens.
    struct token
    {
      enum yytokentype
      {
        END = 0,
        ADD = 258,
        AND = 259,
        EQ = 260,
        AT = 261,
        BASE = 262,
        BNOT = 263,
        COLON = 264,
        COMMA = 265,
        CONST = 266,
        COUNT = 267,
        CSP = 268,
        CSP_ADD = 269,
        CSP_SUB = 270,
        CSP_MUL = 271,
        CSP_LEQ = 272,
        CSP_LT = 273,
        CSP_GT = 274,
        CSP_GEQ = 275,
        CSP_EQ = 276,
        CSP_NEQ = 277,
        CUMULATIVE = 278,
        DISJOINT = 279,
        DOT = 280,
        DOTS = 281,
        EXTERNAL = 282,
        FALSE = 283,
        FORGET = 284,
        GEQ = 285,
        GT = 286,
        IF = 287,
        INCLUDE = 288,
        INFIMUM = 289,
        LBRACE = 290,
        LBRACK = 291,
        LEQ = 292,
        LPAREN = 293,
        LT = 294,
        MAX = 295,
        MAXIMIZE = 296,
        MIN = 297,
        MINIMIZE = 298,
        MOD = 299,
        MUL = 300,
        NEQ = 301,
        POW = 302,
        QUESTION = 303,
        RBRACE = 304,
        RBRACK = 305,
        RPAREN = 306,
        SEM = 307,
        SHOW = 308,
        EDGE = 309,
        PROJECT = 310,
        HEURISTIC = 311,
        SHOWSIG = 312,
        SLASH = 313,
        SUB = 314,
        SUM = 315,
        SUMP = 316,
        SUPREMUM = 317,
        TRUE = 318,
        BLOCK = 319,
        UBNOT = 320,
        UMINUS = 321,
        VBAR = 322,
        VOLATILE = 323,
        WIF = 324,
        XOR = 325,
        PARSE_LP = 326,
        PARSE_DEF = 327,
        ANY = 328,
        UNARY = 329,
        BINARY = 330,
        LEFT = 331,
        RIGHT = 332,
        HEAD = 333,
        BODY = 334,
        DIRECTIVE = 335,
        THEORY = 336,
        SYNC = 337,
        NUMBER = 338,
        ANONYMOUS = 339,
        IDENTIFIER = 340,
        PYTHON = 341,
        LUA = 342,
        STRING = 343,
        VARIABLE = 344,
        THEORY_OP = 345,
        NOT = 346
      };
    };

    /// (External) token type, as returned by yylex.
    typedef token::yytokentype token_type;

    /// Internal symbol number.
    typedef int symbol_number_type;

    /// Internal symbol number for tokens (subsumed by symbol_number_type).
    typedef unsigned char token_number_type;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol type
    /// via type_get().
    ///
    /// Provide access to semantic value and location.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol ();

      /// Copy constructor.
      basic_symbol (const basic_symbol& other);

      /// Constructor for valueless symbols.
      basic_symbol (typename Base::kind_type t,
                    const location_type& l);

      /// Constructor for symbols with semantic value.
      basic_symbol (typename Base::kind_type t,
                    const semantic_type& v,
                    const location_type& l);

      ~basic_symbol ();

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      semantic_type value;

      /// The location.
      location_type location;

    private:
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& other);
    };

    /// Type access provider for token (enum) based symbols.
    struct by_type
    {
      /// Default constructor.
      by_type ();

      /// Copy constructor.
      by_type (const by_type& other);

      /// The symbol type as needed by the constructor.
      typedef token_type kind_type;

      /// Constructor from (external) token numbers.
      by_type (kind_type t);

      /// Steal the symbol type from \a that.
      void move (by_type& that);

      /// The (internal) type number (corresponding to \a type).
      /// -1 when this symbol is empty.
      symbol_number_type type_get () const;

      /// The token.
      token_type token () const;

      enum { empty = 0 };

      /// The symbol type.
      /// -1 when this symbol is empty.
      token_number_type type;
    };

    /// "External" symbols: returned by the scanner.
    typedef basic_symbol<by_type> symbol_type;


    /// Build a parser object.
    parser (Gringo::Input::NonGroundParser *lexer_yyarg);
    virtual ~parser ();

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

  private:
    /// This class is not copyable.
    parser (const parser&);
    parser& operator= (const parser&);

    /// State numbers.
    typedef int state_type;

    /// Generate an error message.
    /// \param yystate   the state where the error occurred.
    /// \param yytoken   the lookahead token type, or yyempty_.
    virtual std::string yysyntax_error_ (state_type yystate,
                                         symbol_number_type yytoken) const;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue);

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue);

    static const short int yypact_ninf_;
    static const short int yytable_ninf_;

    /// Convert a scanner token number \a t to a symbol number.
    static token_number_type yytranslate_ (int t);

    // Tables.
  // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
  // STATE-NUM.
  static const short int yypact_[];

  // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
  // Performed when YYTABLE does not specify something else to do.  Zero
  // means the default is an error.
  static const unsigned short int yydefact_[];

  // YYPGOTO[NTERM-NUM].
  static const short int yypgoto_[];

  // YYDEFGOTO[NTERM-NUM].
  static const short int yydefgoto_[];

  // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
  // positive, shift that token.  If negative, reduce the rule whose
  // number is the opposite.  If YYTABLE_NINF, syntax error.
  static const short int yytable_[];

  static const short int yycheck_[];

  // YYSTOS[STATE-NUM] -- The (internal number of the) accessing
  // symbol of state STATE-NUM.
  static const unsigned char yystos_[];

  // YYR1[YYN] -- Symbol number of symbol that rule YYN derives.
  static const unsigned char yyr1_[];

  // YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.
  static const unsigned char yyr2_[];


    /// Convert the symbol name \a n to a form suitable for a diagnostic.
    static std::string yytnamerr_ (const char *n);


    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#if YYDEBUG
  // YYRLINE[YYN] -- Source line where rule number YYN was defined.
  static const unsigned short int yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r);
    /// Print the state stack on the debug stream.
    virtual void yystack_print_ ();

    // Debugging.
    int yydebug_;
    std::ostream* yycdebug_;

    /// \brief Display a symbol type, value and location.
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
      by_state ();

      /// The symbol type as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s);

      /// Copy constructor.
      by_state (const by_state& other);

      /// Steal the symbol type from \a that.
      void move (by_state& that);

      /// The (internal) type number (corresponding to \a state).
      /// "empty" when empty.
      symbol_number_type type_get () const;

      enum { empty = 0 };

      /// The state.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, symbol_type& sym);
      /// Assignment, needed by push_back.
      stack_symbol_type& operator= (const stack_symbol_type& that);
    };

    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, stack_symbol_type& s);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, state_type s, symbol_type& sym);

    /// Pop \a n symbols the three stacks.
    void yypop_ (unsigned int n = 1);

    // Constants.
    enum
    {
      yyeof_ = 0,
      yylast_ = 2044,     ///< Last index in yytable_.
      yynnts_ = 84,  ///< Number of nonterminal symbols.
      yyempty_ = -2,
      yyfinal_ = 8, ///< Termination state number.
      yyterror_ = 1,
      yyerrcode_ = 256,
      yyntokens_ = 92  ///< Number of tokens.
    };


    // User arguments.
    Gringo::Input::NonGroundParser *lexer;
  };


#line 28 "/tmp/clingo-release/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:372
} } } // Gringo::Input::NonGroundGrammar
#line 628 "/tmp/clingo-release/build/debug/libgringo/src/input/nongroundgrammar/grammar.hh" // lalr1.cc:372




#endif // !YY_GRINGONONGROUNDGRAMMAR_TMP_CLINGO_RELEASE_BUILD_DEBUG_LIBGRINGO_SRC_INPUT_NONGROUNDGRAMMAR_GRAMMAR_HH_INCLUDED
