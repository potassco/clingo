// A Bison parser, made by GNU Bison 3.3.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2019 Free Software Foundation, Inc.

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

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.


// Take the name prefix into account.
#define yylex   GringoNonGroundGrammar_lex

// First part of user prologue.
#line 58 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:429


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


#line 80 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:429

#include "grammar.hh"


// Unqualified %code blocks.
#line 96 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:435


void NonGroundGrammar::parser::error(DefaultLocation const &l, std::string const &msg) {
    lexer->parseError(l, msg);
}


#line 94 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:435


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Suppress unused-variable warnings by "using" E.
#define YYUSE(E) ((void) (E))

// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:510
namespace Gringo { namespace Input { namespace NonGroundGrammar {
#line 189 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:510

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Gringo::Input::NonGroundParser *lexer_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      lexer (lexer_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/

  // basic_symbol.
#if 201103L <= YY_CPLUSPLUS
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (basic_symbol&& that)
    : Base (std::move (that))
    , value (std::move (that.value))
    , location (std::move (that.location))
  {}
#endif

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (semantic_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}

  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_type.
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

#if 201103L <= YY_CPLUSPLUS
  parser::by_type::by_type (by_type&& that)
    : type (that.type)
  {
    that.clear ();
  }
#endif

  parser::by_type::by_type (const by_type& that)
    : type (that.type)
  {}

  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  int
  parser::by_type::type_get () const YY_NOEXCEPT
  {
    return type;
  }


  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_number_type
  parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YYUSE (yysym.type_get ());
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    // State.
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << yystack_[0].state << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            yyla.type = yytranslate_ (yylex (&yyla.value, &yyla.location, lexer));
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      goto yydefault;

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", yyn, YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* If YYLEN is nonzero, implement the default value of the
         action: '$$ = $1'.  Otherwise, use the top of the stack.

         Otherwise, the following line sets YYLHS.VALUE to garbage.
         This behavior is undocumented and Bison users should not rely
         upon it.  */
      if (yylen)
        yylhs.value = yystack_[yylen - 1].value;
      else
        yylhs.value = yystack_[0].value;

      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 7:
#line 331 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->parseError(yylhs.location, "syntax error, unexpected ."); }
#line 673 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 10:
#line 337 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 679 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 11:
#line 338 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 685 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 12:
#line 339 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 691 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 13:
#line 346 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 697 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 14:
#line 347 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 703 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 15:
#line 348 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 709 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 16:
#line 349 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 715 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 17:
#line 350 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 721 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 18:
#line 351 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 727 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 19:
#line 352 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 733 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 20:
#line 353 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 739 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 21:
#line 354 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 745 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 22:
#line 355 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 751 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 23:
#line 356 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 757 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 24:
#line 357 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 763 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 25:
#line 358 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 769 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 26:
#line 359 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), false); }
#line 775 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 27:
#line 360 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.termvec), true); }
#line 781 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 28:
#line 361 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 787 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 29:
#line 362 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 793 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 30:
#line 363 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.term)); }
#line 799 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 31:
#line 364 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 805 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 32:
#line 365 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), true); }
#line 811 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 33:
#line 366 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 817 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 34:
#line 367 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 823 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 35:
#line 368 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 829 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 36:
#line 369 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 835 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 37:
#line 375 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term));  }
#line 841 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 38:
#line 376 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term));  }
#line 847 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 39:
#line 380 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec));  }
#line 853 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 40:
#line 381 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec());  }
#line 859 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 41:
#line 387 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 865 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 42:
#line 388 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 871 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 43:
#line 389 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 877 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 44:
#line 390 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 883 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 45:
#line 391 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 889 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 46:
#line 392 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 895 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 47:
#line 393 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 901 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 48:
#line 394 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 907 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 49:
#line 395 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 913 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 50:
#line 396 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 919 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 51:
#line 397 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 925 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 52:
#line 398 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 931 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 53:
#line 399 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.pool(yylhs.location, (yystack_[1].value.termvec)); }
#line 937 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 54:
#line 400 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 943 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 55:
#line 401 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 949 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 56:
#line 402 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.termvec)); }
#line 955 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 57:
#line 403 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 961 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 58:
#line 404 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), true); }
#line 967 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 59:
#line 405 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 973 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 60:
#line 406 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 979 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 61:
#line 407 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 985 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 62:
#line 408 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 991 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 63:
#line 409 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 997 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 64:
#line 410 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String("_")); }
#line 1003 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 65:
#line 416 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1009 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 66:
#line 417 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 1015 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 67:
#line 423 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1021 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 68:
#line 424 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 1027 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 69:
#line 428 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1033 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 70:
#line 429 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1039 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 71:
#line 433 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), true); }
#line 1045 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 72:
#line 434 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[0].value.termvec), false); }
#line 1051 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 73:
#line 435 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 1057 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 74:
#line 436 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 1063 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 75:
#line 439 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[1].value.term)); }
#line 1069 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 76:
#line 440 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[1].value.term)); }
#line 1075 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 77:
#line 443 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1081 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 78:
#line 444 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[1].value.termvec), (yystack_[0].value.term)); }
#line 1087 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 79:
#line 447 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec)); }
#line 1093 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 80:
#line 448 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[2].value.termvecvec), (yystack_[0].value.termvec)); }
#line 1099 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 81:
#line 452 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1105 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 82:
#line 453 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[4].value.termvecvec), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1111 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 83:
#line 463 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::GT; }
#line 1117 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 84:
#line 464 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::LT; }
#line 1123 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 85:
#line 465 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::GEQ; }
#line 1129 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 86:
#line 466 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::LEQ; }
#line 1135 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 87:
#line 467 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::EQ; }
#line 1141 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 88:
#line 468 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rel) = Relation::NEQ; }
#line 1147 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 89:
#line 472 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1153 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 90:
#line 473 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1159 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 91:
#line 474 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1165 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 92:
#line 475 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1171 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 93:
#line 479 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rellitvec) = BUILDER.rellitvec(yylhs.location, (yystack_[1].value.rel), (yystack_[0].value.term)); }
#line 1177 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 94:
#line 480 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.rellitvec) = BUILDER.rellitvec(yylhs.location, (yystack_[2].value.rellitvec), (yystack_[1].value.rel), (yystack_[0].value.term)); }
#line 1183 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 95:
#line 484 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1189 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 96:
#line 485 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1195 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 97:
#line 486 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1201 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 98:
#line 487 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1207 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 99:
#line 488 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1213 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 100:
#line 489 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1219 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 101:
#line 490 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::POS, (yystack_[0].value.term)); }
#line 1225 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 102:
#line 491 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOT, (yystack_[0].value.term)); }
#line 1231 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 103:
#line 492 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOTNOT, (yystack_[0].value.term)); }
#line 1237 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 104:
#line 493 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, NAF::POS, (yystack_[1].value.term), (yystack_[0].value.rellitvec)); }
#line 1243 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 105:
#line 494 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, NAF::NOT, (yystack_[1].value.term), (yystack_[0].value.rellitvec)); }
#line 1249 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 106:
#line 495 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, NAF::NOTNOT, (yystack_[1].value.term), (yystack_[0].value.rellitvec)); }
#line 1255 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 107:
#line 503 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = BUILDER.litvec(BUILDER.litvec(), (yystack_[0].value.lit)); }
#line 1261 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 108:
#line 504 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = BUILDER.litvec((yystack_[2].value.litvec), (yystack_[0].value.lit)); }
#line 1267 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 109:
#line 508 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1273 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 110:
#line 509 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1279 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 111:
#line 513 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1285 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 112:
#line 514 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1291 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 113:
#line 518 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.fun) = AggregateFunction::SUM; }
#line 1297 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 114:
#line 519 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.fun) = AggregateFunction::SUMP; }
#line 1303 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 115:
#line 520 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.fun) = AggregateFunction::MIN; }
#line 1309 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 116:
#line 521 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.fun) = AggregateFunction::MAX; }
#line 1315 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 117:
#line 522 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.fun) = AggregateFunction::COUNT; }
#line 1321 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 118:
#line 528 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bodyaggrelem) = { BUILDER.termvec(), (yystack_[0].value.litvec) }; }
#line 1327 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 119:
#line 529 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bodyaggrelem) = { (yystack_[1].value.termvec), (yystack_[0].value.litvec) }; }
#line 1333 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 120:
#line 533 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1339 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 121:
#line 534 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec((yystack_[2].value.bodyaggrelemvec), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1345 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 122:
#line 540 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lbodyaggrelem) = { (yystack_[1].value.lit), (yystack_[0].value.litvec) }; }
#line 1351 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 123:
#line 544 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1357 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 124:
#line 545 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1363 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 125:
#line 551 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
#line 1369 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 126:
#line 552 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist) }; }
#line 1375 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 127:
#line 553 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.bodyaggrelemvec() }; }
#line 1381 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 128:
#line 554 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.bodyaggrelemvec) }; }
#line 1387 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 129:
#line 558 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bound) = { Relation::LEQ, (yystack_[0].value.term) }; }
#line 1393 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 130:
#line 559 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bound) = { (yystack_[1].value.rel), (yystack_[0].value.term) }; }
#line 1399 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 131:
#line 560 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.bound) = { Relation::LEQ, TermUid(-1) }; }
#line 1405 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 132:
#line 564 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1411 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 133:
#line 565 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1417 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 134:
#line 566 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1423 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 135:
#line 567 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1429 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 136:
#line 573 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec((yystack_[5].value.headaggrelemvec), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1435 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 137:
#line 574 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1441 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 138:
#line 578 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1447 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 139:
#line 579 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1453 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 140:
#line 585 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.headaggrelemvec() }; }
#line 1459 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 141:
#line 586 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.headaggrelemvec) }; }
#line 1465 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 142:
#line 587 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
#line 1471 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 143:
#line 588 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist)}; }
#line 1477 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 144:
#line 592 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1483 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 145:
#line 593 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1489 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 146:
#line 594 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1495 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 147:
#line 595 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1501 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 148:
#line 602 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.lbodyaggrelem) = { (yystack_[2].value.lit), (yystack_[0].value.litvec) }; }
#line 1507 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 151:
#line 616 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1513 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 152:
#line 617 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1519 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 153:
#line 618 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[2].value.lit), BUILDER.litvec()); }
#line 1525 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 154:
#line 619 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[4].value.condlitlist), (yystack_[3].value.lit), (yystack_[1].value.litvec)); }
#line 1531 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 155:
#line 620 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1537 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 156:
#line 621 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1543 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 157:
#line 622 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[3].value.lit), (yystack_[1].value.litvec)); }
#line 1549 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 158:
#line 623 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), BUILDER.litvec()); }
#line 1555 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 159:
#line 627 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1561 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 160:
#line 628 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), (yystack_[0].value.litvec)); }
#line 1567 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 161:
#line 635 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1573 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 162:
#line 636 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1579 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 163:
#line 637 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1585 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 164:
#line 638 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1591 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 165:
#line 639 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1597 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 166:
#line 640 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1603 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 167:
#line 641 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1609 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 168:
#line 642 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1615 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 169:
#line 643 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1621 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 170:
#line 644 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.body(); }
#line 1627 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 171:
#line 648 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1633 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 172:
#line 649 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1639 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 173:
#line 650 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1645 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 174:
#line 651 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1651 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 175:
#line 652 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1657 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 176:
#line 656 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.body(); }
#line 1663 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 177:
#line 657 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.body(); }
#line 1669 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 178:
#line 658 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1675 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 179:
#line 661 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.head) = BUILDER.headlit((yystack_[0].value.lit)); }
#line 1681 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 180:
#line 662 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.head) = BUILDER.disjunction(yylhs.location, (yystack_[0].value.condlitlist)); }
#line 1687 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 181:
#line 663 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.head) = lexer->headaggregate(yylhs.location, (yystack_[0].value.uid)); }
#line 1693 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 182:
#line 667 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.rule(yylhs.location, (yystack_[1].value.head)); }
#line 1699 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 183:
#line 668 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head)); }
#line 1705 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 184:
#line 669 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head), (yystack_[0].value.body)); }
#line 1711 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 185:
#line 670 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), (yystack_[0].value.body)); }
#line 1717 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 186:
#line 671 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), BUILDER.body()); }
#line 1723 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 187:
#line 677 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1729 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 188:
#line 678 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1735 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 189:
#line 682 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termpair) = {(yystack_[2].value.term), (yystack_[0].value.term)}; }
#line 1741 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 190:
#line 683 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.termpair) = {(yystack_[0].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0))}; }
#line 1747 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 191:
#line 687 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.bodylit(BUILDER.body(), (yystack_[0].value.lit)); }
#line 1753 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 192:
#line 688 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[0].value.lit)); }
#line 1759 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 193:
#line 692 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1765 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 194:
#line 693 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.body(); }
#line 1771 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 195:
#line 694 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.body) = BUILDER.body(); }
#line 1777 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 196:
#line 698 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[4].value.body)); }
#line 1783 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 197:
#line 699 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), BUILDER.body()); }
#line 1789 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 198:
#line 703 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1795 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 199:
#line 704 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1801 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 200:
#line 708 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1807 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 201:
#line 709 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1813 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 206:
#line 722 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 1819 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 207:
#line 723 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 1825 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 208:
#line 724 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.showsig(yylhs.location, Sig("", 0, false)); }
#line 1831 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 209:
#line 725 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body)); }
#line 1837 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 210:
#line 726 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body()); }
#line 1843 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 211:
#line 732 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 1849 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 212:
#line 733 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 1855 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 213:
#line 738 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.edge(yylhs.location, (yystack_[2].value.termvecvec), (yystack_[0].value.body)); }
#line 1861 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 214:
#line 744 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.heuristic(yylhs.location, (yystack_[8].value.term), (yystack_[7].value.body), (yystack_[5].value.term), (yystack_[3].value.term), (yystack_[1].value.term)); }
#line 1867 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 215:
#line 745 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.heuristic(yylhs.location, (yystack_[6].value.term), (yystack_[5].value.body), (yystack_[3].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0)), (yystack_[1].value.term)); }
#line 1873 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 216:
#line 751 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 1879 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 217:
#line 752 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 1885 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 218:
#line 753 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.project(yylhs.location, (yystack_[1].value.term), (yystack_[0].value.body)); }
#line 1891 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 219:
#line 759 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    {  BUILDER.define(yylhs.location, String::fromRep((yystack_[2].value.str)), (yystack_[0].value.term), false, LOGGER); }
#line 1897 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 220:
#line 763 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.define(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.term), true, LOGGER); }
#line 1903 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 221:
#line 764 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.define(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.term), true, LOGGER); }
#line 1909 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 222:
#line 765 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.define(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.term), false, LOGGER); }
#line 1915 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 223:
#line 771 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.script(yylhs.location, String::fromRep((yystack_[3].value.str)), String::fromRep((yystack_[1].value.str))); }
#line 1921 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 224:
#line 777 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->include(String::fromRep((yystack_[1].value.str)), yylhs.location, false, LOGGER); }
#line 1927 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 225:
#line 778 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->include(String::fromRep((yystack_[2].value.str)), yylhs.location, true, LOGGER); }
#line 1933 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 226:
#line 784 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.idlist) = BUILDER.idvec((yystack_[2].value.idlist), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 1939 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 227:
#line 785 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.idlist) = BUILDER.idvec(BUILDER.idvec(), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 1945 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 228:
#line 789 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.idlist) = BUILDER.idvec(); }
#line 1951 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 229:
#line 790 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.idlist) = (yystack_[0].value.idlist); }
#line 1957 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 230:
#line 794 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[4].value.str)), (yystack_[2].value.idlist)); }
#line 1963 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 231:
#line 795 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[1].value.str)), BUILDER.idvec()); }
#line 1969 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 232:
#line 801 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 1975 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 233:
#line 802 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), BUILDER.body(), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 1981 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 234:
#line 803 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[1].value.term), BUILDER.body(), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 1987 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 235:
#line 804 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[5].value.term), (yystack_[3].value.body), (yystack_[1].value.term)); }
#line 1993 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 236:
#line 805 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[5].value.term), BUILDER.body(), (yystack_[1].value.term)); }
#line 1999 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 237:
#line 806 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.external(yylhs.location, (yystack_[4].value.term), BUILDER.body(), (yystack_[1].value.term)); }
#line 2005 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 238:
#line 812 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2011 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 239:
#line 813 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2017 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 240:
#line 819 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[1].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2023 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 241:
#line 820 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2029 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 242:
#line 824 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermset(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2035 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 243:
#line 825 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theoryoptermlist(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2041 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 244:
#line 826 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms()); }
#line 2047 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 245:
#line 827 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermopterm(yylhs.location, (yystack_[1].value.theoryOpterm)); }
#line 2053 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 246:
#line 828 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[2].location, (yystack_[2].value.theoryOpterm))); }
#line 2059 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 247:
#line 829 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(yystack_[3].location, (yystack_[3].value.theoryOpterm), (yystack_[1].value.theoryOpterms))); }
#line 2065 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 248:
#line 830 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermfun(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.theoryOpterms)); }
#line 2071 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 249:
#line 831 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 2077 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 250:
#line 832 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 2083 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 251:
#line 833 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 2089 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 252:
#line 834 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createInf()); }
#line 2095 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 253:
#line 835 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createSup()); }
#line 2101 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 254:
#line 836 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvar(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 2107 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 255:
#line 840 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[2].value.theoryOpterm), (yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2113 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 256:
#line 841 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2119 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 257:
#line 842 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm(BUILDER.theoryops(), (yystack_[0].value.theoryTerm)); }
#line 2125 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 258:
#line 846 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms((yystack_[2].value.theoryOpterms), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2131 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 259:
#line 847 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2137 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 260:
#line 851 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterms) = (yystack_[0].value.theoryOpterms); }
#line 2143 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 261:
#line 852 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(); }
#line 2149 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 262:
#line 856 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElem) = { (yystack_[2].value.theoryOpterms), (yystack_[0].value.litvec) }; }
#line 2155 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 263:
#line 857 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElem) = { BUILDER.theoryopterms(), (yystack_[0].value.litvec) }; }
#line 2161 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 264:
#line 861 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElems) = BUILDER.theoryelems((yystack_[3].value.theoryElems), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2167 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 265:
#line 862 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(BUILDER.theoryelems(), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2173 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 266:
#line 866 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElems) = (yystack_[0].value.theoryElems); }
#line 2179 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 267:
#line 867 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(); }
#line 2185 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 268:
#line 871 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), false); }
#line 2191 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 269:
#line 872 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 2197 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 270:
#line 875 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[0].value.term), BUILDER.theoryelems()); }
#line 2203 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 271:
#line 876 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[6].value.term), (yystack_[3].value.theoryElems)); }
#line 2209 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 272:
#line 877 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[8].value.term), (yystack_[5].value.theoryElems), String::fromRep((yystack_[2].value.str)), yystack_[1].location, (yystack_[1].value.theoryOpterm)); }
#line 2215 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 273:
#line 883 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2221 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 274:
#line 884 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[2].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2227 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 275:
#line 888 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = (yystack_[0].value.theoryOps); }
#line 2233 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 276:
#line 889 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOps) = BUILDER.theoryops(); }
#line 2239 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 277:
#line 893 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.num), TheoryOperatorType::Unary); }
#line 2245 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 278:
#line 894 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryLeft); }
#line 2251 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 279:
#line 895 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryRight); }
#line 2257 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 280:
#line 899 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(BUILDER.theoryopdefs(), (yystack_[0].value.theoryOpDef)); }
#line 2263 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 281:
#line 900 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs((yystack_[3].value.theoryOpDefs), (yystack_[0].value.theoryOpDef)); }
#line 2269 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 282:
#line 904 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDefs) = (yystack_[0].value.theoryOpDefs); }
#line 2275 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 283:
#line 905 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(); }
#line 2281 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 284:
#line 909 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2287 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 285:
#line 910 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("left"); }
#line 2293 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 286:
#line 911 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("right"); }
#line 2299 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 287:
#line 912 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("unary"); }
#line 2305 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 288:
#line 913 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("binary"); }
#line 2311 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 289:
#line 914 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("head"); }
#line 2317 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 290:
#line 915 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("body"); }
#line 2323 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 291:
#line 916 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("any"); }
#line 2329 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 292:
#line 917 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.str) = String::toRep("directive"); }
#line 2335 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 293:
#line 921 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryTermDef) = BUILDER.theorytermdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.theoryOpDefs), LOGGER); }
#line 2341 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 294:
#line 925 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Head; }
#line 2347 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 295:
#line 926 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Body; }
#line 2353 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 296:
#line 927 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Any; }
#line 2359 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 297:
#line 928 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Directive; }
#line 2365 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 298:
#line 933 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[14].value.str)), (yystack_[12].value.num), String::fromRep((yystack_[10].value.str)), (yystack_[0].value.theoryAtomType), (yystack_[6].value.theoryOps), String::fromRep((yystack_[2].value.str))); }
#line 2371 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 299:
#line 934 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.num), String::fromRep((yystack_[2].value.str)), (yystack_[0].value.theoryAtomType)); }
#line 2377 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 300:
#line 938 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryAtomDef)); }
#line 2383 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 301:
#line 939 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryTermDef)); }
#line 2389 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 302:
#line 940 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryAtomDef)); }
#line 2395 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 303:
#line 941 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryTermDef)); }
#line 2401 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 304:
#line 945 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = (yystack_[0].value.theoryDefs); }
#line 2407 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 305:
#line 946 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(); }
#line 2413 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 306:
#line 950 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { BUILDER.theorydef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[3].value.theoryDefs), LOGGER); }
#line 2419 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 307:
#line 956 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->theoryLexing(TheoryLexing::Theory); }
#line 2425 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 308:
#line 960 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->theoryLexing(TheoryLexing::Definition); }
#line 2431 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;

  case 309:
#line 964 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:919
    { lexer->theoryLexing(TheoryLexing::Disabled); }
#line 2437 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
    break;


#line 2441 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:919
            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yyterror_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yyterror_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = yyn;
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    size_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state
         merging (from LALR or IELR) and default reductions corrupt the
         expected token list.  However, the list is correct for
         canonical LR with one exception: it will still contain any
         token that will not be accepted due to an error action in a
         later state.
    */
    if (!yyla.empty ())
      {
        int yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];
        int yyn = yypact_[yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yyterror_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    size_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const short parser::yypact_ninf_ = -508;

  const short parser::yytable_ninf_ = -310;

  const short
  parser::yypact_[] =
  {
      36,  -508,   202,    59,   716,  -508,  -508,  -508,    75,    39,
    -508,  -508,   202,   202,  1442,   202,  -508,  -508,     3,    45,
    -508,   120,    17,  -508,  1003,   612,  -508,   131,  -508,   144,
    1287,   155,   106,     3,   157,  1442,  -508,  -508,  -508,  -508,
     202,  1442,   181,   202,  -508,  -508,  -508,   177,  -508,  -508,
    1161,  -508,   323,  1655,  -508,    57,   185,   875,  -508,  1172,
    -508,   156,  -508,   701,  -508,    31,   184,   211,   213,  1442,
     215,  -508,   240,   202,   232,    49,   202,   199,  -508,   577,
    -508,   202,   248,  -508,  1733,   265,   105,  -508,  1930,   266,
     246,   612,   241,  1299,  1347,  -508,   998,  1442,   202,    53,
     195,   195,   202,   244,   582,  -508,   125,  1930,    66,   279,
     281,  -508,   283,  -508,  -508,  1209,  1733,  -508,  1442,  1442,
    1442,  -508,  1442,  -508,  -508,  -508,  -508,  1442,  1442,  -508,
    1442,  1442,  1442,  1442,  1442,   910,   871,   875,  1045,  -508,
    -508,  -508,  -508,  1379,  1930,  1442,  -508,    83,  -508,   330,
     202,   701,  -508,   375,   701,  -508,   701,  -508,  -508,   287,
    1948,  -508,  -508,  1442,   321,  1442,  1442,   701,   336,  1442,
     362,   360,   342,   305,  1082,   796,  1696,   289,   381,   875,
     221,    46,  -508,   389,  -508,  1442,  1172,  -508,  -508,  1172,
    1442,  -508,   369,  -508,   400,  1760,   405,   118,   404,   405,
     152,  -508,  -508,  1769,   104,   113,   350,   411,  -508,  -508,
     402,   382,   356,  1442,  -508,   202,  1442,  -508,  1442,  1442,
     408,   393,  -508,  -508,  1733,  -508,   871,   426,  -508,   134,
     180,   290,  1954,   407,   407,   407,   459,   407,   180,  1360,
    1930,   875,  1442,  -508,  -508,  -508,    73,  -508,  -508,   430,
     159,  1930,  1124,  -508,  -508,  -508,  -508,  -508,   413,  -508,
     406,  -508,  1948,    63,  -508,  1499,   701,   701,   701,   701,
     701,   701,   701,   701,   701,   701,   210,  1502,   271,   288,
    1602,  1442,   301,   425,   428,  1442,   383,   438,  -508,   265,
    -508,   219,   811,  1696,   245,   966,   875,  1172,  -508,  -508,
    -508,  1251,  -508,  -508,  -508,  -508,  -508,  -508,   445,   450,
    -508,   265,  1930,  -508,  -508,  1442,  1442,   452,   451,  1442,
    -508,   452,   454,  1442,  -508,  1442,   195,  1442,   391,   455,
    -508,  -508,  1442,   415,   461,   326,  -508,   468,   449,  1930,
     405,   405,   253,   421,   871,  1442,   269,  1442,  -508,  1930,
    1172,  -508,  1172,  -508,  1442,  -508,    73,   701,  -508,   504,
    -508,  -508,   482,   458,   357,   506,   463,   463,   463,   708,
     463,   357,  1972,  -508,  -508,  1539,  1539,  1514,  -508,  -508,
    -508,  -508,  -508,  -508,   473,  -508,  1539,  -508,   291,   492,
    -508,   464,  -508,   494,  -508,  -508,   479,   328,  -508,  1442,
    1442,  1787,   497,  -508,  -508,  -508,  1172,  1696,   292,  -508,
    -508,  -508,   875,  -508,  -508,  1172,  -508,   379,  -508,   247,
    -508,  -508,  1930,   426,  1172,  -508,  -508,   405,  -508,  -508,
     405,  1930,  -508,  1827,   500,  -508,  1441,   508,  -508,   398,
     202,   509,   484,   485,  1959,  -508,  -508,  -508,  -508,  -508,
    -508,  -508,  -508,  -508,  -508,  -508,  -508,  -508,   262,   516,
    -508,  -508,   265,   524,  -508,   495,  -508,  1948,   701,  -508,
     492,   496,   498,  -508,    41,  1539,  -508,  -508,  1539,  1539,
     265,   502,   507,  1172,   313,  -508,  1811,  1851,  -508,  -508,
    -508,  -508,  -508,  -508,  -508,  -508,  -508,  -508,  1431,  -508,
     529,   452,   452,  1442,  -508,  1442,  1442,  -508,  -508,  -508,
    -508,  -508,   501,   523,  -508,   253,  -508,  -508,  1172,  -508,
    -508,  -508,  1574,  -508,   510,  -508,   291,  -508,  1539,   291,
    -508,   512,   518,  -508,  -508,  -508,  1172,  -508,  -508,  1930,
    1869,  1890,   486,   291,   546,  -508,  -508,   265,  -508,    65,
    -508,  -508,  1539,  -508,  -508,  -508,  -508,  1442,  -508,   553,
    -508,  -508,   464,  -508,  -508,  -508,  -508,   291,  1906,  1959,
     554,   522,   527,  -508,  -508,   559,   499,   291,  -508,   284,
     563,  -508,  -508,  -508,  -508,  -508,  -508,   549,   333,   291,
    -508,   565,  -508,   566,  -508,   334,   291,   539,  -508,  -508,
    -508,   570,  1959,   574,   284,  -508
  };

  const unsigned short
  parser::yydefact_[] =
  {
       0,     5,     0,     0,     0,    10,    11,    12,     0,     0,
       1,   309,     0,     0,     0,     0,   117,     7,     0,     0,
      98,   170,     0,    61,     0,    74,   116,     0,   115,     0,
       0,     0,     0,     0,     0,     0,   113,   114,    62,    95,
       0,     0,   170,     0,     6,    59,    64,     0,    60,    63,
       0,     4,    57,     0,   101,   179,     0,   131,   181,     0,
     180,     0,   147,     0,     3,     0,   268,   270,    58,     0,
      57,    52,     0,     0,    89,     0,     0,     0,   186,     0,
     185,     0,     0,   142,     0,   112,     0,    73,    67,    72,
      77,    74,     0,     0,     0,   208,     0,     0,     0,    89,
       0,     0,     0,     0,    57,    51,     0,    65,     0,     0,
       0,   308,     0,    99,    96,     0,     0,   102,    70,     0,
       0,    87,     0,    85,    83,    86,    84,     0,     0,    88,
       0,     0,     0,     0,     0,     0,   104,   131,   110,   155,
     149,   150,   156,    70,   129,     0,   146,   112,   182,   170,
       0,     0,    35,     0,     0,    36,     0,    33,    34,    31,
     219,     8,     9,    70,     0,    70,    70,     0,    91,    70,
     170,   234,     0,     0,     0,     0,     0,     0,     0,   131,
       0,     0,   135,     0,   224,     0,   110,   138,   143,     0,
      71,    75,    78,    53,     0,   190,   188,     0,     0,   188,
       0,   170,   210,     0,     0,    91,     0,   170,   176,   218,
       0,     0,     0,    70,   231,   228,     0,    56,     0,     0,
       0,     0,   100,    97,     0,   103,   105,    69,    79,     0,
      45,    44,    41,    49,    47,    50,    43,    48,    46,    42,
      93,   131,     0,   144,   158,   107,   109,   160,   140,     0,
       0,   130,   110,   151,   159,   152,   183,   184,    32,    23,
       0,    24,    37,     0,    22,     0,    40,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   267,     0,     0,
       0,    70,     0,   233,   232,     0,     0,     0,   125,   112,
     123,     0,     0,     0,     0,     0,   131,   110,   161,   171,
     162,     0,   134,   163,   172,   164,   175,   169,     0,   109,
     111,   112,    68,    76,   203,     0,     0,   195,     0,     0,
     202,   195,     0,     0,   209,     0,     0,     0,     0,     0,
     177,   178,     0,     0,     0,     0,   227,   229,     0,    66,
     188,   188,   305,     0,   106,     0,    54,    70,   145,    94,
       0,   157,     0,   141,    70,   153,   109,    40,    25,     0,
      26,    30,    39,     0,    16,    15,    20,    18,    21,    14,
      19,    17,    13,   269,   252,   261,   261,     0,   253,   250,
     251,   254,   238,   239,   249,   241,     0,   257,   259,   309,
     265,   266,   307,     0,    55,    54,   220,     0,    90,     0,
       0,     0,     0,   211,   122,   126,     0,     0,     0,   165,
     173,   166,   131,   132,   148,   110,   127,   112,   120,     0,
     225,   139,   189,   187,   194,   198,   205,   188,   200,   204,
     188,    81,   213,     0,     0,   216,     0,     0,   206,    54,
       0,     0,     0,     0,     0,   291,   287,   288,   285,   286,
     289,   290,   292,   284,   307,   303,   302,   304,     0,     0,
      80,   108,   112,     0,   154,     0,    27,    38,     0,    28,
     260,     0,     0,   244,     0,   261,   240,   256,     0,     0,
     112,     0,     0,   110,     0,    92,     0,     0,   237,   212,
     124,   167,   174,   168,   133,   118,   119,   128,     0,   191,
     193,   195,   195,     0,   217,     0,     0,   207,   226,   230,
     197,   196,     0,     0,   309,     0,   223,   137,     0,    29,
     242,   243,     0,   245,     0,   255,   258,   262,   309,   309,
     263,     0,     0,   236,   235,   121,     0,   199,   201,    82,
       0,     0,     0,   283,     0,   301,   300,   112,   246,     0,
     248,   264,     0,   271,   221,   222,   192,     0,   215,     0,
     308,   280,   282,   308,   306,   136,   247,   309,     0,     0,
       0,     0,     0,   272,   214,     0,     0,     0,   293,   307,
       0,   281,   296,   294,   295,   297,   299,     0,     0,   276,
     277,     0,   273,   275,   308,     0,     0,     0,   278,   279,
     274,     0,     0,     0,     0,   298
  };

  const short
  parser::yypgoto_[] =
  {
    -508,  -508,  -508,  -508,    -2,   -49,   435,   233,   352,  -508,
      -7,  -120,   515,  -508,  -508,  -101,  -508,   -47,    22,  -107,
      11,  -117,  -134,  -121,    23,    99,  -508,   192,  -508,  -169,
    -132,  -133,  -508,  -508,   -19,  -508,  -508,  -127,  -508,  -508,
    -508,     1,   -82,  -508,  -174,   -86,  -508,  -309,  -508,  -508,
    -508,  -508,  -508,  -347,  -364,  -336,  -341,  -274,  -332,    71,
    -508,  -508,  -508,   599,  -508,  -508,    34,  -508,  -508,  -395,
      93,     8,    98,  -508,  -508,  -363,  -507,   -10
  };

  const short
  parser::yydefgoto_[] =
  {
      -1,     3,     4,    51,    70,   262,   362,   363,    84,   108,
     227,   228,    90,    91,    92,   229,   204,   145,    54,   136,
     245,   309,   310,   187,   178,   418,   419,   290,   291,   179,
     146,   180,   250,    86,    57,    58,   181,   142,    59,    60,
      79,    80,   209,    61,   317,   196,   500,   425,   197,   200,
       9,   337,   338,   385,   386,   387,   388,   470,   471,   390,
     391,   392,    67,   182,   593,   594,   561,   562,   563,   454,
     455,   586,   456,   457,   458,   164,   220,   393
  };

  const short
  parser::yytable_[] =
  {
       8,    65,    52,   389,   247,   243,   135,   296,   199,   226,
      66,    68,   428,    72,   160,    55,    74,    77,    89,   210,
     255,   246,    52,   249,   478,   321,   254,    56,   481,   482,
      99,    74,   103,   104,   137,    85,   474,   185,   106,   476,
      75,   111,   294,   110,   472,   161,    81,   302,    52,   512,
     477,   522,    73,   570,   100,   101,   572,    52,   170,    10,
     306,   159,   276,   171,   278,   279,   138,   139,   282,   185,
     147,   168,   117,   359,   172,   479,    56,    52,     5,   183,
      63,   169,   523,   350,    89,     6,     7,   597,   307,   242,
     177,   513,   252,   253,    76,    82,   205,     1,     2,   140,
     211,   206,   259,   162,   360,   264,   566,   265,   216,   348,
     478,    64,   335,    52,   141,   140,   241,   344,   280,   351,
       5,   382,   383,   217,   296,   140,   412,     6,     7,   295,
     141,   476,   340,   341,    78,   356,    52,   225,   526,   214,
     141,   281,   525,   524,   188,   326,   327,   189,   258,   159,
     257,   159,   159,   215,   159,    98,    93,   318,    56,   408,
     319,   328,   478,   414,   413,   159,   442,   443,   404,    94,
     148,   284,    52,    52,   575,   346,   347,   185,   149,   242,
     397,     5,   552,    97,    52,   289,   226,    52,     6,     7,
     421,   322,   537,   538,   323,   109,   560,   117,   353,   571,
     311,   354,   324,   478,   207,   112,   102,   603,   331,   208,
     143,   567,   163,   336,   127,   128,   587,   130,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   460,   132,   464,
     560,   303,     5,   427,   463,   304,  -307,   430,   296,     6,
       7,   165,   592,   166,   432,   167,   295,   173,   549,   600,
      52,   373,   347,   501,   389,   409,   502,   444,   405,   410,
     169,   406,   184,   305,   159,   159,   159,   159,   159,   159,
     159,   159,   159,   159,   186,   384,   190,     5,   -90,   -90,
     494,   495,   193,   -90,     6,     7,   497,   411,   191,   498,
      52,   -90,   212,   119,   417,    52,   496,   242,   297,   298,
     344,   514,   491,   299,   515,   218,   492,   219,   -90,   423,
     467,   -90,   394,   347,   225,   266,   445,   446,   447,   448,
     449,   450,   451,   452,   127,   128,   -90,   130,     5,   395,
     347,   300,   -89,   -89,   493,     6,     7,   -89,   132,   133,
     453,   517,   398,   347,   256,   -89,   277,   582,    52,   530,
      52,   118,   583,   584,   585,   159,    53,   159,   221,   527,
     295,   461,   -89,   462,   281,   -89,    71,   439,   347,   485,
     347,   382,   383,   384,   384,   384,   283,    88,   287,   480,
     -89,   150,    96,   151,   384,   260,   285,   105,   186,   345,
     286,   269,   270,   107,   271,   531,   532,   590,   591,   152,
     598,   599,   116,   153,    52,   273,   301,   -92,   -92,   144,
     308,   313,   -92,    52,   314,   316,   261,   289,   320,   467,
     -92,   105,    52,   329,   154,   330,   565,   155,   332,   334,
     333,   176,   156,   342,   343,   499,   345,   -92,   508,   352,
     -92,   357,   453,    88,   130,   195,   195,   358,   157,   203,
       5,   399,   403,   158,   400,   -92,   402,     6,     7,   420,
     350,   424,   119,   120,   434,   426,   159,   224,   429,   435,
      88,   230,   231,   384,   232,   438,   384,   384,   440,   233,
     234,    52,   235,   236,   237,   238,   239,   240,   437,   144,
     441,   417,   468,   127,   128,    88,   130,   251,   459,   469,
     271,   475,   479,   483,   544,   484,  -307,   132,   133,   267,
     150,   489,   151,   453,   504,    88,    52,    88,    88,   553,
     384,    88,   507,   509,   510,   511,   384,   293,   152,   547,
     516,   144,   153,   518,    52,   520,   519,   240,   521,   536,
     269,   270,   312,   271,   528,   466,   529,   556,   543,   542,
     384,   550,   554,   154,   273,   274,   155,   573,   555,   559,
     564,   156,   569,   576,   577,    88,   578,   453,   339,   579,
     195,   195,   580,   588,   589,   595,   596,   157,   601,     5,
     602,    12,   158,    13,   604,    14,     6,     7,   263,    16,
     465,   -91,   -91,   144,   349,    20,   -91,   535,   490,   551,
     453,    23,   174,    62,   -91,    25,   192,    26,   545,    28,
     213,   581,   605,   546,     0,     0,     0,     0,    13,     0,
      14,   -91,    87,     0,   -91,     0,    35,    36,    37,    38,
      39,     0,     0,    88,    41,     0,    23,   401,     0,   -91,
      25,     0,     0,     0,   407,     0,     0,   240,   144,     0,
      45,    46,     5,    88,     0,    48,    49,     0,   175,     6,
       7,    69,     0,     0,    38,     0,     0,   422,    88,    41,
       0,   195,     0,     0,     0,   195,     0,   431,     0,   433,
       0,     0,     0,     0,   436,    45,    46,     5,     0,     0,
      48,    49,     0,     0,     6,     7,     0,   312,     0,    88,
       0,     0,     0,     0,     0,     0,    88,   150,     0,   151,
       0,   267,   268,     0,     0,     0,    -2,    11,     0,     0,
      12,     0,    13,     0,    14,   152,     0,    15,    16,   153,
      17,     0,    18,    19,    20,     0,     0,     0,    21,    22,
      23,    24,   269,   270,    25,   271,    26,    27,    28,    29,
     154,   486,   487,   155,     0,     0,   273,   274,   156,    30,
      31,    32,    33,    34,   144,    35,    36,    37,    38,    39,
      40,     0,     0,    41,   157,    42,     5,     0,     0,   158,
       0,     0,     0,     6,     7,     0,     0,    43,    44,    45,
      46,     5,    47,     0,    48,    49,     0,    50,     6,     7,
      12,     0,    13,     0,    14,     0,     0,     0,    16,     0,
       0,     0,     0,     0,   113,    12,     0,    13,     0,    14,
      23,   174,     0,    16,    25,     0,    26,     0,    28,   222,
       0,     0,     0,     0,     0,    23,   174,     0,     0,    25,
       0,    26,     0,    28,     0,    35,    36,    37,    38,   114,
      88,     0,     0,    41,     0,   539,     0,   540,   541,     0,
      35,    36,    37,    38,   223,     0,     0,     0,    41,    45,
      46,     5,     0,     0,    48,    49,   121,   292,     6,     7,
     121,    13,     0,    14,    45,    46,     5,     0,     0,    48,
      49,   123,   124,     6,     7,   123,   124,     0,   125,    23,
     126,     0,   125,    25,   126,     0,     0,   129,     0,   568,
       0,   129,     0,     0,     0,     0,    13,     0,    14,     0,
       0,     0,    16,     0,    69,     0,     0,    38,     0,     0,
       0,     0,    41,     0,    23,    24,     0,     0,    25,     0,
      26,     0,    28,     0,     0,     0,     0,     0,    45,    46,
       5,     0,     0,    48,    49,     0,     0,     6,     7,    69,
      36,    37,    38,     0,     0,     0,     0,    41,     0,     0,
       0,     0,    13,     0,    14,     0,     0,     0,    16,     0,
       0,     0,     0,    45,    46,     5,     0,     0,    48,    49,
      23,   174,     6,     7,    25,     0,    26,     0,    28,     0,
       0,   119,   120,     0,     0,     0,     0,   201,     0,    13,
       0,    14,   202,   122,     0,    69,    36,    37,    38,     0,
       0,    20,     0,    41,     0,     0,     0,    23,     0,     0,
       0,    25,   127,   128,     0,   130,   131,     0,     0,    45,
      46,     5,    83,     0,    48,    49,   132,   133,     6,     7,
       0,    13,    35,    14,     0,    38,    39,     0,   134,     0,
      41,     0,     0,    20,     0,     0,     0,     0,     0,    23,
       0,     0,     0,    25,     0,     0,    45,    46,     5,     0,
       0,    48,    49,     0,    50,     6,     7,   244,    13,     0,
      14,     0,     0,     0,    35,     0,     0,    38,    39,     0,
      20,     0,    41,     0,     0,     0,    23,     0,     0,     0,
      25,     0,     0,     0,     0,     0,     0,     0,    45,    46,
       5,   288,     0,    48,    49,     0,    50,     6,     7,     0,
      13,    35,    14,     0,    38,    39,     0,     0,     0,    41,
       0,     0,    20,     0,     0,     0,     0,     0,    23,     0,
       0,     0,    25,     0,     0,    45,    46,     5,     0,     0,
      48,    49,     0,    50,     6,     7,   355,    13,     0,    14,
       0,     0,     0,    35,     0,     0,    38,    39,    13,   113,
      14,    41,     0,     0,     0,    23,     0,     0,     0,    25,
      20,     0,     0,     0,     0,     0,    23,    45,    46,     5,
      25,     0,    48,    49,     0,    50,     6,     7,     0,     0,
      35,     0,     0,    38,   114,    13,     0,    14,    41,     0,
       0,    35,     0,     0,    38,    39,     0,   222,     0,    41,
       0,     0,     0,    23,    45,    46,     5,    25,     0,    48,
      49,     0,   115,     6,     7,    45,    46,     5,     0,     0,
      48,    49,     0,    50,     6,     7,     0,    13,    35,    14,
     415,    38,   223,     0,     0,     0,    41,     0,     0,     0,
       0,     0,     0,     0,     0,    23,     0,     0,     0,    25,
       0,     0,    45,    46,     5,     0,     0,    48,    49,     0,
     416,     6,     7,    13,     0,    14,     0,     0,     0,     0,
      69,    95,     0,    38,     0,    13,     0,    14,    41,     0,
       0,    23,     0,     0,     0,    25,     0,     0,     0,     0,
       0,     0,     0,    23,    45,    46,     5,    25,     0,    48,
      49,     0,     0,     6,     7,     0,    69,     0,   194,    38,
       0,     0,     0,     0,    41,     0,     0,     0,    69,     0,
       0,    38,     0,    13,     0,    14,    41,     0,     0,     0,
      45,    46,     5,   119,   120,    48,    49,     0,     0,     6,
       7,    23,    45,    46,     5,    25,     0,    48,    49,     0,
       0,     6,     7,     0,     0,    13,   198,    14,     0,     0,
       0,     0,     0,     0,   127,   128,    69,   130,   131,    38,
       0,     0,     0,    23,    41,     0,     0,    25,   132,   133,
       0,     0,     0,     0,     0,     0,     0,     0,   248,     0,
      45,    46,     5,     0,     0,    48,    49,     0,    69,     6,
       7,    38,     0,     0,     0,     0,    41,    13,     0,    14,
     415,     0,     0,     0,   119,   120,     0,   505,    13,     0,
      14,   506,    45,    46,     5,    23,   122,    48,    49,    25,
       0,     6,     7,     0,     0,     0,    23,     0,     0,     0,
      25,     0,     0,     0,     0,   127,   128,     0,   130,   131,
      69,     0,     0,    38,     0,     0,     0,     0,    41,   132,
     133,    69,     0,     0,    38,     0,     0,     0,     0,    41,
       0,   134,   267,   268,    45,    46,     5,     0,     0,    48,
      49,  -309,     0,     6,     7,    45,    46,     5,     0,     0,
      48,    49,     0,     0,     6,     7,   374,   375,   376,     0,
     377,     0,     0,   269,   270,     0,   271,   272,   374,   375,
     376,     0,   377,     0,     0,     0,     0,   273,   274,     0,
       0,     0,     0,     0,   378,   473,   361,     0,     0,   275,
       0,     0,     0,   374,   375,   376,   378,   377,     0,     0,
       0,     0,     0,     0,     0,   379,     0,     5,     0,     0,
     380,   381,   382,   383,     6,     7,     0,   379,     0,     5,
       0,   378,   380,   381,   382,   383,     6,     7,   374,   375,
     376,     0,   377,     0,     0,   267,   268,     0,     0,     0,
       0,     0,   379,     0,     5,   548,   396,   380,   381,   382,
     383,     6,     7,     0,     0,     0,   378,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   269,   270,     0,   271,
     272,     0,     0,     0,     0,     0,     0,   379,     0,     5,
     273,   274,   380,   381,   382,   383,     6,     7,   119,   120,
     121,     0,   275,     0,     0,     0,     0,    16,     0,     0,
     122,     0,     0,     0,     0,   123,   124,     0,     0,     0,
      24,     0,   125,     0,   126,    26,     0,    28,     0,   127,
     128,   129,   130,   131,     0,     0,     0,     0,     0,   119,
     120,   121,     0,   132,   133,    36,    37,     0,    16,     0,
       0,   122,     0,     0,     0,   134,   123,   124,     0,     0,
       0,   174,     0,   125,     0,   126,    26,     0,    28,     0,
     127,   128,   129,   130,   131,     0,   119,   120,   121,     0,
       0,     0,     0,     0,   132,   133,    36,    37,   122,     0,
       0,     0,     0,   123,   124,     0,   134,     0,     0,     0,
     125,     0,   126,   119,   120,     0,   315,   127,   128,   129,
     130,   131,   119,   120,     0,   122,     0,     0,     0,   325,
       0,   132,   133,     0,   122,     0,     0,     0,     0,     0,
     119,   120,     0,   134,   127,   128,     0,   130,   131,     0,
       0,     0,   122,   127,   128,     0,   130,   131,   132,   133,
       0,     0,     0,     0,   119,   120,     0,   132,   133,     0,
     134,   127,   128,     0,   130,   131,   122,   488,     0,   134,
     119,   120,     0,     0,     0,   132,   133,   503,     0,     0,
       0,     0,   122,     0,     0,   127,   128,   134,   130,   131,
       0,   533,     0,     0,   119,   120,     0,     0,     0,   132,
     133,   127,   128,     0,   130,   131,   122,     0,     0,     0,
       0,   134,   119,   120,     0,   132,   133,     0,     0,   557,
       0,     0,     0,     0,   122,   127,   128,   134,   130,   131,
       0,   534,     0,   119,   120,     0,     0,     0,     0,   132,
     133,     0,     0,   127,   128,   122,   130,   131,     0,   119,
     120,   134,     0,     0,     0,     0,     0,   132,   133,     0,
       0,   122,     0,     0,   127,   128,     0,   130,   131,   134,
     558,     0,     0,   119,   120,     0,     0,     0,   132,   133,
     127,   128,     0,   130,   131,   122,   574,     0,     0,     0,
     134,   267,   268,     0,   132,   133,     0,   119,   120,     0,
       0,     0,     0,     0,   127,   128,   134,   130,   131,     0,
       0,     0,     0,     0,     0,   267,   268,     0,   132,   133,
       0,     0,   269,   270,     0,   271,   272,     0,   127,   128,
     134,   130,   131,     0,     0,     0,   273,   274,     0,     0,
       0,     0,   132,   133,     0,     0,   269,   270,   275,   271,
     272,     0,     0,     0,   134,     0,     0,     0,     0,     0,
     273,   274,   445,   446,   447,   448,   449,   450,   451,   452,
       0,     0,     0,     0,     5,     0,     0,     0,     0,     0,
       0,     6,     7
  };

  const short
  parser::yycheck_[] =
  {
       2,    11,     4,   277,   138,   137,    53,   176,    94,   116,
      12,    13,   321,    15,    63,     4,    18,    19,    25,   101,
     147,   138,    24,   143,   388,   199,   147,     4,   391,   392,
      32,    33,    34,    35,    53,    24,   377,    84,    40,   386,
      18,    43,   175,    42,   376,    14,    29,   179,    50,   444,
     386,    10,    49,   560,    32,    33,   563,    59,     9,     0,
      14,    63,   163,    14,   165,   166,     9,    10,   169,   116,
      59,    73,    50,    10,    76,    10,    53,    79,    75,    81,
       5,    28,    41,    10,    91,    82,    83,   594,    42,   136,
      79,   454,     9,    10,    49,    78,    98,    61,    62,    42,
     102,    48,   151,    72,    41,   154,    41,   156,    42,   241,
     474,    72,   213,   115,    57,    42,   135,   224,   167,   246,
      75,    80,    81,    57,   293,    42,   295,    82,    83,   176,
      57,   478,   218,   219,    14,   252,   138,   115,   479,    14,
      57,    28,   478,   475,    39,    41,    42,    42,   150,   151,
     149,   153,   154,    28,   156,    49,    25,    39,   135,   292,
      42,    48,   526,   297,   296,   167,   340,   341,   289,    25,
      14,   170,   174,   175,   569,    41,    42,   224,    22,   226,
     281,    75,   529,    28,   186,   174,   293,   189,    82,    83,
     311,    39,   501,   502,    42,    14,   543,   175,    39,   562,
     189,    42,   201,   567,     9,    28,    49,   602,   207,    14,
      25,   552,    28,   215,    34,    35,   579,    37,   267,   268,
     269,   270,   271,   272,   273,   274,   275,   347,    48,   356,
     577,    10,    75,   319,   354,    14,    25,   323,   407,    82,
      83,    28,   589,    28,   326,     5,   293,    48,   522,   596,
     252,    41,    42,   427,   528,    10,   430,     4,    39,    14,
      28,    42,    14,    42,   266,   267,   268,   269,   270,   271,
     272,   273,   274,   275,     9,   277,    10,    75,     9,    10,
     412,   415,    41,    14,    82,    83,    39,    42,    42,    42,
     292,    22,    48,     3,   301,   297,   417,   344,     9,    10,
     407,    39,    10,    14,    42,    26,    14,    26,    39,   316,
     359,    42,    41,    42,   292,    28,    63,    64,    65,    66,
      67,    68,    69,    70,    34,    35,    57,    37,    75,    41,
      42,    42,     9,    10,    42,    82,    83,    14,    48,    49,
     342,   462,    41,    42,    14,    22,    25,    63,   350,   483,
     352,    28,    68,    69,    70,   357,     4,   359,    75,   480,
     407,   350,    39,   352,    28,    42,    14,    41,    42,    41,
      42,    80,    81,   375,   376,   377,    14,    25,    73,   389,
      57,     6,    30,     8,   386,    10,    26,    35,     9,    10,
      48,    34,    35,    41,    37,    82,    83,    64,    65,    24,
      66,    67,    50,    28,   406,    48,    25,     9,    10,    57,
      21,    42,    14,   415,    14,    10,    41,   406,    14,   468,
      22,    69,   424,    73,    49,    14,   547,    52,    26,    73,
      48,    79,    57,    25,    41,   424,    10,    39,   440,     9,
      42,    28,   444,    91,    37,    93,    94,    41,    73,    97,
      75,    26,    14,    78,    26,    57,    73,    82,    83,    14,
      10,     9,     3,     4,    73,    14,   468,   115,    14,    14,
     118,   119,   120,   475,   122,    14,   478,   479,    10,   127,
     128,   483,   130,   131,   132,   133,   134,   135,    73,   137,
      41,   498,    10,    34,    35,   143,    37,   145,    77,    41,
      37,    28,    10,     9,   514,    26,    42,    48,    49,     3,
       6,    14,     8,   515,    14,   163,   518,   165,   166,   529,
     522,   169,    14,    14,    40,    40,   528,   175,    24,   518,
      14,   179,    28,     9,   536,    39,    41,   185,    40,    10,
      34,    35,   190,    37,    42,    41,    39,   536,    25,    48,
     552,    41,    40,    49,    48,    49,    52,   567,    40,    73,
      14,    57,     9,     9,    42,   213,    39,   569,   216,    10,
     218,   219,    73,    10,    25,    10,    10,    73,    39,    75,
      10,     4,    78,     6,    10,     8,    82,    83,   153,    12,
     357,     9,    10,   241,   242,    18,    14,   498,   406,   528,
     602,    24,    25,     4,    22,    28,    91,    30,   515,    32,
      28,   577,   604,   515,    -1,    -1,    -1,    -1,     6,    -1,
       8,    39,    10,    -1,    42,    -1,    49,    50,    51,    52,
      53,    -1,    -1,   281,    57,    -1,    24,   285,    -1,    57,
      28,    -1,    -1,    -1,   292,    -1,    -1,   295,   296,    -1,
      73,    74,    75,   301,    -1,    78,    79,    -1,    81,    82,
      83,    49,    -1,    -1,    52,    -1,    -1,   315,   316,    57,
      -1,   319,    -1,    -1,    -1,   323,    -1,   325,    -1,   327,
      -1,    -1,    -1,    -1,   332,    73,    74,    75,    -1,    -1,
      78,    79,    -1,    -1,    82,    83,    -1,   345,    -1,   347,
      -1,    -1,    -1,    -1,    -1,    -1,   354,     6,    -1,     8,
      -1,     3,     4,    -1,    -1,    -1,     0,     1,    -1,    -1,
       4,    -1,     6,    -1,     8,    24,    -1,    11,    12,    28,
      14,    -1,    16,    17,    18,    -1,    -1,    -1,    22,    23,
      24,    25,    34,    35,    28,    37,    30,    31,    32,    33,
      49,   399,   400,    52,    -1,    -1,    48,    49,    57,    43,
      44,    45,    46,    47,   412,    49,    50,    51,    52,    53,
      54,    -1,    -1,    57,    73,    59,    75,    -1,    -1,    78,
      -1,    -1,    -1,    82,    83,    -1,    -1,    71,    72,    73,
      74,    75,    76,    -1,    78,    79,    -1,    81,    82,    83,
       4,    -1,     6,    -1,     8,    -1,    -1,    -1,    12,    -1,
      -1,    -1,    -1,    -1,    18,     4,    -1,     6,    -1,     8,
      24,    25,    -1,    12,    28,    -1,    30,    -1,    32,    18,
      -1,    -1,    -1,    -1,    -1,    24,    25,    -1,    -1,    28,
      -1,    30,    -1,    32,    -1,    49,    50,    51,    52,    53,
     498,    -1,    -1,    57,    -1,   503,    -1,   505,   506,    -1,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    57,    73,
      74,    75,    -1,    -1,    78,    79,     5,    81,    82,    83,
       5,     6,    -1,     8,    73,    74,    75,    -1,    -1,    78,
      79,    20,    21,    82,    83,    20,    21,    -1,    27,    24,
      29,    -1,    27,    28,    29,    -1,    -1,    36,    -1,   557,
      -1,    36,    -1,    -1,    -1,    -1,     6,    -1,     8,    -1,
      -1,    -1,    12,    -1,    49,    -1,    -1,    52,    -1,    -1,
      -1,    -1,    57,    -1,    24,    25,    -1,    -1,    28,    -1,
      30,    -1,    32,    -1,    -1,    -1,    -1,    -1,    73,    74,
      75,    -1,    -1,    78,    79,    -1,    -1,    82,    83,    49,
      50,    51,    52,    -1,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,     6,    -1,     8,    -1,    -1,    -1,    12,    -1,
      -1,    -1,    -1,    73,    74,    75,    -1,    -1,    78,    79,
      24,    25,    82,    83,    28,    -1,    30,    -1,    32,    -1,
      -1,     3,     4,    -1,    -1,    -1,    -1,     9,    -1,     6,
      -1,     8,    14,    15,    -1,    49,    50,    51,    52,    -1,
      -1,    18,    -1,    57,    -1,    -1,    -1,    24,    -1,    -1,
      -1,    28,    34,    35,    -1,    37,    38,    -1,    -1,    73,
      74,    75,    39,    -1,    78,    79,    48,    49,    82,    83,
      -1,     6,    49,     8,    -1,    52,    53,    -1,    60,    -1,
      57,    -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    24,
      -1,    -1,    -1,    28,    -1,    -1,    73,    74,    75,    -1,
      -1,    78,    79,    -1,    81,    82,    83,    42,     6,    -1,
       8,    -1,    -1,    -1,    49,    -1,    -1,    52,    53,    -1,
      18,    -1,    57,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    73,    74,
      75,    39,    -1,    78,    79,    -1,    81,    82,    83,    -1,
       6,    49,     8,    -1,    52,    53,    -1,    -1,    -1,    57,
      -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,    24,    -1,
      -1,    -1,    28,    -1,    -1,    73,    74,    75,    -1,    -1,
      78,    79,    -1,    81,    82,    83,    42,     6,    -1,     8,
      -1,    -1,    -1,    49,    -1,    -1,    52,    53,     6,    18,
       8,    57,    -1,    -1,    -1,    24,    -1,    -1,    -1,    28,
      18,    -1,    -1,    -1,    -1,    -1,    24,    73,    74,    75,
      28,    -1,    78,    79,    -1,    81,    82,    83,    -1,    -1,
      49,    -1,    -1,    52,    53,     6,    -1,     8,    57,    -1,
      -1,    49,    -1,    -1,    52,    53,    -1,    18,    -1,    57,
      -1,    -1,    -1,    24,    73,    74,    75,    28,    -1,    78,
      79,    -1,    81,    82,    83,    73,    74,    75,    -1,    -1,
      78,    79,    -1,    81,    82,    83,    -1,     6,    49,     8,
       9,    52,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    24,    -1,    -1,    -1,    28,
      -1,    -1,    73,    74,    75,    -1,    -1,    78,    79,    -1,
      39,    82,    83,     6,    -1,     8,    -1,    -1,    -1,    -1,
      49,    14,    -1,    52,    -1,     6,    -1,     8,    57,    -1,
      -1,    24,    -1,    -1,    -1,    28,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    24,    73,    74,    75,    28,    -1,    78,
      79,    -1,    -1,    82,    83,    -1,    49,    -1,    39,    52,
      -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    49,    -1,
      -1,    52,    -1,     6,    -1,     8,    57,    -1,    -1,    -1,
      73,    74,    75,     3,     4,    78,    79,    -1,    -1,    82,
      83,    24,    73,    74,    75,    28,    -1,    78,    79,    -1,
      -1,    82,    83,    -1,    -1,     6,    39,     8,    -1,    -1,
      -1,    -1,    -1,    -1,    34,    35,    49,    37,    38,    52,
      -1,    -1,    -1,    24,    57,    -1,    -1,    28,    48,    49,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,
      73,    74,    75,    -1,    -1,    78,    79,    -1,    49,    82,
      83,    52,    -1,    -1,    -1,    -1,    57,     6,    -1,     8,
       9,    -1,    -1,    -1,     3,     4,    -1,     6,     6,    -1,
       8,    10,    73,    74,    75,    24,    15,    78,    79,    28,
      -1,    82,    83,    -1,    -1,    -1,    24,    -1,    -1,    -1,
      28,    -1,    -1,    -1,    -1,    34,    35,    -1,    37,    38,
      49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,    48,
      49,    49,    -1,    -1,    52,    -1,    -1,    -1,    -1,    57,
      -1,    60,     3,     4,    73,    74,    75,    -1,    -1,    78,
      79,     9,    -1,    82,    83,    73,    74,    75,    -1,    -1,
      78,    79,    -1,    -1,    82,    83,    24,    25,    26,    -1,
      28,    -1,    -1,    34,    35,    -1,    37,    38,    24,    25,
      26,    -1,    28,    -1,    -1,    -1,    -1,    48,    49,    -1,
      -1,    -1,    -1,    -1,    52,    41,    57,    -1,    -1,    60,
      -1,    -1,    -1,    24,    25,    26,    52,    28,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    73,    -1,    75,    -1,    -1,
      78,    79,    80,    81,    82,    83,    -1,    73,    -1,    75,
      -1,    52,    78,    79,    80,    81,    82,    83,    24,    25,
      26,    -1,    28,    -1,    -1,     3,     4,    -1,    -1,    -1,
      -1,    -1,    73,    -1,    75,    41,    14,    78,    79,    80,
      81,    82,    83,    -1,    -1,    -1,    52,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    34,    35,    -1,    37,
      38,    -1,    -1,    -1,    -1,    -1,    -1,    73,    -1,    75,
      48,    49,    78,    79,    80,    81,    82,    83,     3,     4,
       5,    -1,    60,    -1,    -1,    -1,    -1,    12,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    20,    21,    -1,    -1,    -1,
      25,    -1,    27,    -1,    29,    30,    -1,    32,    -1,    34,
      35,    36,    37,    38,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,    48,    49,    50,    51,    -1,    12,    -1,
      -1,    15,    -1,    -1,    -1,    60,    20,    21,    -1,    -1,
      -1,    25,    -1,    27,    -1,    29,    30,    -1,    32,    -1,
      34,    35,    36,    37,    38,    -1,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    48,    49,    50,    51,    15,    -1,
      -1,    -1,    -1,    20,    21,    -1,    60,    -1,    -1,    -1,
      27,    -1,    29,     3,     4,    -1,     6,    34,    35,    36,
      37,    38,     3,     4,    -1,    15,    -1,    -1,    -1,    10,
      -1,    48,    49,    -1,    15,    -1,    -1,    -1,    -1,    -1,
       3,     4,    -1,    60,    34,    35,    -1,    37,    38,    -1,
      -1,    -1,    15,    34,    35,    -1,    37,    38,    48,    49,
      -1,    -1,    -1,    -1,     3,     4,    -1,    48,    49,    -1,
      60,    34,    35,    -1,    37,    38,    15,    40,    -1,    60,
       3,     4,    -1,    -1,    -1,    48,    49,    10,    -1,    -1,
      -1,    -1,    15,    -1,    -1,    34,    35,    60,    37,    38,
      -1,    40,    -1,    -1,     3,     4,    -1,    -1,    -1,    48,
      49,    34,    35,    -1,    37,    38,    15,    -1,    -1,    -1,
      -1,    60,     3,     4,    -1,    48,    49,    -1,    -1,    10,
      -1,    -1,    -1,    -1,    15,    34,    35,    60,    37,    38,
      -1,    40,    -1,     3,     4,    -1,    -1,    -1,    -1,    48,
      49,    -1,    -1,    34,    35,    15,    37,    38,    -1,     3,
       4,    60,    -1,    -1,    -1,    -1,    -1,    48,    49,    -1,
      -1,    15,    -1,    -1,    34,    35,    -1,    37,    38,    60,
      40,    -1,    -1,     3,     4,    -1,    -1,    -1,    48,    49,
      34,    35,    -1,    37,    38,    15,    40,    -1,    -1,    -1,
      60,     3,     4,    -1,    48,    49,    -1,     3,     4,    -1,
      -1,    -1,    -1,    -1,    34,    35,    60,    37,    38,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    48,    49,
      -1,    -1,    34,    35,    -1,    37,    38,    -1,    34,    35,
      60,    37,    38,    -1,    -1,    -1,    48,    49,    -1,    -1,
      -1,    -1,    48,    49,    -1,    -1,    34,    35,    60,    37,
      38,    -1,    -1,    -1,    60,    -1,    -1,    -1,    -1,    -1,
      48,    49,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    -1,    -1,    75,    -1,    -1,    -1,    -1,    -1,
      -1,    82,    83
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,    61,    62,    85,    86,    75,    82,    83,    88,   134,
       0,     1,     4,     6,     8,    11,    12,    14,    16,    17,
      18,    22,    23,    24,    25,    28,    30,    31,    32,    33,
      43,    44,    45,    46,    47,    49,    50,    51,    52,    53,
      54,    57,    59,    71,    72,    73,    74,    76,    78,    79,
      81,    87,    88,    92,   102,   104,   108,   118,   119,   122,
     123,   127,   147,     5,    72,   161,    88,   146,    88,    49,
      88,    92,    88,    49,    88,   102,    49,    88,    14,   124,
     125,    29,    78,    39,    92,   104,   117,    10,    92,    94,
      96,    97,    98,    25,    25,    14,    92,    28,    49,    88,
     102,   102,    49,    88,    88,    92,    88,    92,    93,    14,
     125,    88,    28,    18,    53,    81,    92,   102,    28,     3,
       4,     5,    15,    20,    21,    27,    29,    34,    35,    36,
      37,    38,    48,    49,    60,   101,   103,   118,     9,    10,
      42,    57,   121,    25,    92,   101,   114,   104,    14,    22,
       6,     8,    24,    28,    49,    52,    57,    73,    78,    88,
      89,    14,    72,    28,   159,    28,    28,     5,    88,    28,
       9,    14,    88,    48,    25,    81,    92,   104,   108,   113,
     115,   120,   147,    88,    14,   101,     9,   107,    39,    42,
      10,    42,    96,    41,    39,    92,   129,   132,    39,   129,
     133,     9,    14,    92,   100,    88,    48,     9,    14,   126,
     126,    88,    48,    28,    14,    28,    42,    57,    26,    26,
     160,    75,    18,    53,    92,   102,   103,    94,    95,    99,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      92,   118,   101,   114,    42,   104,   105,   106,    39,    95,
     116,    92,     9,    10,   107,   121,    14,   125,    88,    89,
      10,    41,    89,    90,    89,    89,    28,     3,     4,    34,
      35,    37,    38,    48,    49,    60,    99,    25,    99,    99,
      89,    28,    99,    14,   125,    26,    48,    73,    39,   104,
     111,   112,    81,    92,   115,   101,   113,     9,    10,    14,
      42,    25,   114,    10,    14,    42,    14,    42,    21,   105,
     106,   104,    92,    42,    14,     6,    10,   128,    39,    42,
      14,   128,    39,    42,   125,    10,    41,    42,    48,    73,
      14,   125,    26,    48,    73,    99,    88,   135,   136,    92,
     129,   129,    25,    41,   103,    10,    41,    42,   114,    92,
      10,   121,     9,    39,    42,    42,   105,    28,    41,    10,
      41,    57,    90,    91,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    41,    24,    25,    26,    28,    52,    73,
      78,    79,    80,    81,    88,   137,   138,   139,   140,   141,
     143,   144,   145,   161,    41,    41,    14,    99,    41,    26,
      26,    92,    73,    14,   107,    39,    42,    92,   115,    10,
      14,    42,   113,   114,   106,     9,    39,    94,   109,   110,
      14,   107,    92,    94,     9,   131,    14,   129,   131,    14,
     129,    92,   126,    92,    73,    14,    92,    73,    14,    41,
      10,    41,   128,   128,     4,    63,    64,    65,    66,    67,
      68,    69,    70,    88,   153,   154,   156,   157,   158,    77,
      95,   104,   104,    95,   121,    91,    41,    89,    10,    41,
     141,   142,   142,    41,   140,    28,   137,   139,   138,    10,
     161,   159,   159,     9,    26,    41,    92,    92,    40,    14,
     111,    10,    14,    42,   114,   106,   107,    39,    42,   104,
     130,   128,   128,    10,    14,     6,    10,    14,    88,    14,
      40,    40,   153,   159,    39,    42,    14,   107,     9,    41,
      39,    40,    10,    41,   142,   139,   140,   107,    42,    39,
     106,    82,    83,    40,    40,   109,    10,   131,   131,    92,
      92,    92,    48,    25,   161,   154,   156,   104,    41,   141,
      41,   143,   137,   161,    40,    40,   104,    10,    40,    73,
     137,   150,   151,   152,    14,   107,    41,   140,    92,     9,
     160,   159,   160,   161,    40,   153,     9,    42,    39,    10,
      73,   150,    63,    68,    69,    70,   155,   159,    10,    25,
      64,    65,   137,   148,   149,    10,    10,   160,    66,    67,
     137,    39,    10,   153,    10,   155
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    84,    85,    85,    86,    86,    87,    87,    87,    87,
      88,    88,    88,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    89,    89,
      89,    89,    89,    89,    89,    89,    89,    90,    90,    91,
      91,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    92,
      92,    92,    92,    92,    92,    93,    93,    94,    94,    95,
      95,    96,    96,    96,    96,    97,    97,    98,    98,    99,
      99,   100,   100,   101,   101,   101,   101,   101,   101,   102,
     102,   102,   102,   103,   103,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   105,   105,   106,
     106,   107,   107,   108,   108,   108,   108,   108,   109,   109,
     110,   110,   111,   112,   112,   113,   113,   113,   113,   114,
     114,   114,   115,   115,   115,   115,   116,   116,   117,   117,
     118,   118,   118,   118,   119,   119,   119,   119,   120,   121,
     121,   122,   122,   122,   122,   122,   122,   122,   122,   123,
     123,   124,   124,   124,   124,   124,   124,   124,   124,   124,
     124,   125,   125,   125,   125,   125,   126,   126,   126,   127,
     127,   127,    87,    87,    87,    87,    87,   128,   128,   129,
     129,   130,   130,   131,   131,   131,    87,    87,   132,   132,
     133,   133,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,   134,
      87,    87,    87,    87,    87,    87,   135,   135,   136,   136,
      87,    87,    87,    87,    87,    87,    87,    87,   137,   137,
     138,   138,   139,   139,   139,   139,   139,   139,   139,   139,
     139,   139,   139,   139,   139,   140,   140,   140,   141,   141,
     142,   142,   143,   143,   144,   144,   145,   145,   146,   146,
     147,   147,   147,   148,   148,   149,   149,   150,   150,   150,
     151,   151,   152,   152,   153,   153,   153,   153,   153,   153,
     153,   153,   153,   154,   155,   155,   155,   155,   156,   156,
     157,   157,   157,   157,   158,   158,    87,   159,   160,   161
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     2,     3,     2,     0,     1,     1,     3,     3,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     3,     3,     4,     4,     5,
       3,     1,     2,     1,     1,     1,     1,     1,     3,     1,
       0,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     3,     4,     5,     3,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     3,     1,
       0,     2,     1,     1,     0,     2,     3,     1,     2,     1,
       3,     3,     5,     1,     1,     1,     1,     1,     1,     1,
       4,     2,     5,     2,     3,     1,     2,     3,     1,     2,
       3,     1,     2,     3,     2,     3,     4,     1,     3,     1,
       0,     2,     0,     1,     1,     1,     1,     1,     2,     2,
       1,     3,     2,     1,     3,     2,     3,     3,     4,     1,
       2,     0,     3,     4,     2,     1,     6,     4,     2,     4,
       3,     4,     2,     3,     3,     4,     2,     1,     3,     1,
       1,     3,     3,     4,     5,     2,     2,     4,     3,     3,
       3,     3,     3,     3,     3,     4,     4,     5,     5,     3,
       0,     3,     3,     4,     5,     3,     1,     2,     2,     1,
       1,     1,     2,     3,     3,     2,     2,     2,     0,     3,
       1,     1,     3,     2,     1,     0,     6,     6,     3,     5,
       3,     5,     4,     4,     5,     5,     5,     6,     2,     4,
       3,     5,     6,     5,    10,     8,     5,     6,     3,     3,
       5,     8,     8,     6,     3,     5,     3,     1,     0,     1,
       6,     3,     4,     4,     3,     7,     7,     6,     1,     1,
       2,     1,     3,     3,     2,     3,     4,     5,     4,     1,
       1,     1,     1,     1,     1,     3,     2,     1,     3,     1,
       1,     0,     3,     3,     4,     1,     1,     0,     1,     4,
       2,     8,    10,     1,     3,     1,     0,     6,     8,     8,
       1,     4,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     6,     1,     1,     1,     1,    16,     8,
       3,     3,     1,     1,     1,     0,     8,     0,     0,     0
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"<EOF>\"", "error", "$undefined", "\"+\"", "\"&\"", "\"=\"", "\"@\"",
  "\"#base\"", "\"~\"", "\":\"", "\",\"", "\"#const\"", "\"#count\"",
  "\"#cumulative\"", "\".\"", "\"..\"", "\"#external\"", "\"#defined\"",
  "\"#false\"", "\"#forget\"", "\">=\"", "\">\"", "\":-\"", "\"#include\"",
  "\"#inf\"", "\"{\"", "\"[\"", "\"<=\"", "\"(\"", "\"<\"", "\"#max\"",
  "\"#maximize\"", "\"#min\"", "\"#minimize\"", "\"\\\\\"", "\"*\"",
  "\"!=\"", "\"**\"", "\"?\"", "\"}\"", "\"]\"", "\")\"", "\";\"",
  "\"#show\"", "\"#edge\"", "\"#project\"", "\"#heuristic\"",
  "\"#showsig\"", "\"/\"", "\"-\"", "\"#sum\"", "\"#sum+\"", "\"#sup\"",
  "\"#true\"", "\"#program\"", "UBNOT", "UMINUS", "\"|\"", "\"#volatile\"",
  "\":~\"", "\"^\"", "\"<program>\"", "\"<define>\"", "\"any\"",
  "\"unary\"", "\"binary\"", "\"left\"", "\"right\"", "\"head\"",
  "\"body\"", "\"directive\"", "\"#theory\"", "\"EOF\"", "\"<NUMBER>\"",
  "\"<ANONYMOUS>\"", "\"<IDENTIFIER>\"", "\"<SCRIPT>\"", "\"<CODE>\"",
  "\"<STRING>\"", "\"<VARIABLE>\"", "\"<THEORYOP>\"", "\"not\"",
  "\"default\"", "\"override\"", "$accept", "start", "program",
  "statement", "identifier", "constterm", "consttermvec", "constargvec",
  "term", "unaryargvec", "ntermvec", "termvec", "tuple", "tuplevec_sem",
  "tuplevec", "argvec", "binaryargvec", "cmp", "atom", "rellitvec",
  "literal", "nlitvec", "litvec", "optcondition", "aggregatefunction",
  "bodyaggrelem", "bodyaggrelemvec", "altbodyaggrelem",
  "altbodyaggrelemvec", "bodyaggregate", "upper", "lubodyaggregate",
  "headaggrelemvec", "altheadaggrelemvec", "headaggregate",
  "luheadaggregate", "conjunction", "dsym", "disjunctionsep",
  "disjunction", "bodycomma", "bodydot", "bodyconddot", "head",
  "optimizetuple", "optimizeweight", "optimizelitvec", "optimizecond",
  "maxelemlist", "minelemlist", "define", "nidlist", "idlist", "theory_op",
  "theory_op_list", "theory_term", "theory_opterm", "theory_opterm_nlist",
  "theory_opterm_list", "theory_atom_element", "theory_atom_element_nlist",
  "theory_atom_element_list", "theory_atom_name", "theory_atom",
  "theory_operator_nlist", "theory_operator_list",
  "theory_operator_definition", "theory_operator_definition_nlist",
  "theory_operator_definiton_list", "theory_definition_identifier",
  "theory_term_definition", "theory_atom_type", "theory_atom_definition",
  "theory_definition_nlist", "theory_definition_list",
  "enable_theory_lexing", "enable_theory_definition_lexing",
  "disable_theory_lexing", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned short
  parser::yyrline_[] =
  {
       0,   318,   318,   319,   323,   324,   330,   331,   332,   333,
     337,   338,   339,   346,   347,   348,   349,   350,   351,   352,
     353,   354,   355,   356,   357,   358,   359,   360,   361,   362,
     363,   364,   365,   366,   367,   368,   369,   375,   376,   380,
     381,   387,   388,   389,   390,   391,   392,   393,   394,   395,
     396,   397,   398,   399,   400,   401,   402,   403,   404,   405,
     406,   407,   408,   409,   410,   416,   417,   423,   424,   428,
     429,   433,   434,   435,   436,   439,   440,   443,   444,   447,
     448,   452,   453,   463,   464,   465,   466,   467,   468,   472,
     473,   474,   475,   479,   480,   484,   485,   486,   487,   488,
     489,   490,   491,   492,   493,   494,   495,   503,   504,   508,
     509,   513,   514,   518,   519,   520,   521,   522,   528,   529,
     533,   534,   540,   544,   545,   551,   552,   553,   554,   558,
     559,   560,   564,   565,   566,   567,   573,   574,   578,   579,
     585,   586,   587,   588,   592,   593,   594,   595,   602,   609,
     610,   616,   617,   618,   619,   620,   621,   622,   623,   627,
     628,   635,   636,   637,   638,   639,   640,   641,   642,   643,
     644,   648,   649,   650,   651,   652,   656,   657,   658,   661,
     662,   663,   667,   668,   669,   670,   671,   677,   678,   682,
     683,   687,   688,   692,   693,   694,   698,   699,   703,   704,
     708,   709,   713,   714,   715,   716,   722,   723,   724,   725,
     726,   732,   733,   738,   744,   745,   751,   752,   753,   759,
     763,   764,   765,   771,   777,   778,   784,   785,   789,   790,
     794,   795,   801,   802,   803,   804,   805,   806,   812,   813,
     819,   820,   824,   825,   826,   827,   828,   829,   830,   831,
     832,   833,   834,   835,   836,   840,   841,   842,   846,   847,
     851,   852,   856,   857,   861,   862,   866,   867,   871,   872,
     875,   876,   877,   883,   884,   888,   889,   893,   894,   895,
     899,   900,   904,   905,   909,   910,   911,   912,   913,   914,
     915,   916,   917,   921,   925,   926,   927,   928,   932,   934,
     938,   939,   940,   941,   945,   946,   950,   956,   960,   964
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << i->state;
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    unsigned yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const token_number_type
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83
    };
    const unsigned user_token_number_max_ = 338;
    const token_number_type undef_token_ = 2;

    if (static_cast<int> (t) <= yyeof_)
      return yyeof_;
    else if (static_cast<unsigned> (t) <= user_token_number_max_)
      return translate_table[t];
    else
      return undef_token_;
  }

#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:1242
} } } // Gringo::Input::NonGroundGrammar
#line 3600 "/home/kaminski/Documents/git/potassco/clingo/build/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:1242
