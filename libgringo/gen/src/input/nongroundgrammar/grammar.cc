// A Bison parser, made by GNU Bison 3.0.4.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015 Free Software Foundation, Inc.

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

// Take the name prefix into account.
#define yylex   GringoNonGroundGrammar_lex

// First part of user declarations.
#line 58 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:404


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


#line 76 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:404

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

#include "grammar.hh"

// User implementation prologue.

#line 90 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:412
// Unqualified %code blocks.
#line 96 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:413


void NonGroundGrammar::parser::error(DefaultLocation const &l, std::string const &msg) {
    lexer->parseError(l, msg);
}


#line 100 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:413


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
    while (/*CONSTCOND*/ false)
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
      *yycdebug_ << std::endl;                  \
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
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE(Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void>(0)
# define YY_STACK_PRINT()                static_cast<void>(0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 28 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:479
namespace Gringo { namespace Input { namespace NonGroundGrammar {
#line 186 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:479

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
        std::string yyr = "";
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
              // Fall through.
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


  /*---------------.
  | Symbol types.  |
  `---------------*/

  inline
  parser::syntax_error::syntax_error (const location_type& l, const std::string& m)
    : std::runtime_error (m)
    , location (l)
  {}

  // basic_symbol.
  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol ()
    : value ()
  {}

  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& other)
    : Base (other)
    , value ()
    , location (other.location)
  {
    value = other.value;
  }


  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, const semantic_type& v, const location_type& l)
    : Base (t)
    , value (v)
    , location (l)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  inline
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, const location_type& l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  inline
  parser::basic_symbol<Base>::~basic_symbol ()
  {
    clear ();
  }

  template <typename Base>
  inline
  void
  parser::basic_symbol<Base>::clear ()
  {
    Base::clear ();
  }

  template <typename Base>
  inline
  bool
  parser::basic_symbol<Base>::empty () const
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  inline
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move(s);
    value = s.value;
    location = s.location;
  }

  // by_type.
  inline
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

  inline
  parser::by_type::by_type (const by_type& other)
    : type (other.type)
  {}

  inline
  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  parser::by_type::type_get () const
  {
    return type;
  }


  // by_state.
  inline
  parser::by_state::by_state ()
    : state (empty_state)
  {}

  inline
  parser::by_state::by_state (const by_state& other)
    : state (other.state)
  {}

  inline
  void
  parser::by_state::clear ()
  {
    state = empty_state;
  }

  inline
  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  inline
  parser::by_state::by_state (state_type s)
    : state (s)
  {}

  inline
  parser::symbol_number_type
  parser::by_state::type_get () const
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  inline
  parser::stack_symbol_type::stack_symbol_type ()
  {}


  inline
  parser::stack_symbol_type::stack_symbol_type (state_type s, symbol_type& that)
    : super_type (s, that.location)
  {
    value = that.value;
    // that is emptied.
    that.type = empty_symbol;
  }

  inline
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }


  template <typename Base>
  inline
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
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  inline
  void
  parser::yypush_ (const char* m, state_type s, symbol_type& sym)
  {
    stack_symbol_type t (s, sym);
    yypush_ (m, t);
  }

  inline
  void
  parser::yypush_ (const char* m, stack_symbol_type& s)
  {
    if (m)
      YY_SYMBOL_PRINT (m, s);
    yystack_.push (s);
  }

  inline
  void
  parser::yypop_ (unsigned int n)
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

  inline parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  inline bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
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

    // FIXME: This shoud be completely indented.  It is not yet to
    // avoid gratuitous conflicts when merging into the master branch.
    try
      {
    YYCDEBUG << "Starting parse" << std::endl;


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, yyla);

    // A new symbol was pushed on the stack.
  yynewstate:
    YYCDEBUG << "Entering state " << yystack_[0].state << std::endl;

    // Accept?
    if (yystack_[0].state == yyfinal_)
      goto yyacceptlab;

    goto yybackup;

    // Backup.
  yybackup:

    // Try to take a decision without lookahead.
    yyn = yypact_[yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
        try
          {
            yyla.type = yytranslate_ (yylex (&yyla.value, &yyla.location, lexer));
          }
        catch (const syntax_error& yyexc)
          {
            error (yyexc);
            goto yyerrlab1;
          }
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
    yypush_ ("Shifting", yyn, yyla);
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
  | yyreduce -- Do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_(yystack_[yylen].state, yyr1_[yyn]);
      /* If YYLEN is nonzero, implement the default value of the
         action: '$$ = $1'.  Otherwise, use the top of the stack.

         Otherwise, the following line sets YYLHS.VALUE to garbage.
         This behavior is undocumented and Bison users should not rely
         upon it.  */
      if (yylen)
        yylhs.value = yystack_[yylen - 1].value;
      else
        yylhs.value = yystack_[0].value;

      // Compute the default @$.
      {
        slice<stack_symbol_type, stack_type> slice (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, slice, yylen);
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
      try
        {
          switch (yyn)
            {
  case 7:
#line 350 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->parseError(yylhs.location, "syntax error, unexpected ."); }
#line 666 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 10:
#line 356 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 672 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 11:
#line 363 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 678 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 12:
#line 364 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 684 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 13:
#line 365 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 690 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 14:
#line 366 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 696 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 15:
#line 367 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 702 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 16:
#line 368 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 708 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 17:
#line 369 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 714 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 18:
#line 370 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 720 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 19:
#line 371 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 726 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 20:
#line 372 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 732 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 21:
#line 373 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 738 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 22:
#line 374 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 744 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 23:
#line 375 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 750 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 24:
#line 376 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), false); }
#line 756 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 25:
#line 377 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.termvec), true); }
#line 762 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 26:
#line 378 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 768 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 27:
#line 379 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 774 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 28:
#line 380 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.term)); }
#line 780 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 29:
#line 381 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 786 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 30:
#line 382 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 792 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 31:
#line 383 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 798 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 32:
#line 384 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 804 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 33:
#line 385 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 810 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 34:
#line 391 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term));  }
#line 816 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 35:
#line 392 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term));  }
#line 822 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 36:
#line 396 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec));  }
#line 828 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 37:
#line 397 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec();  }
#line 834 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 38:
#line 403 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 840 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 39:
#line 404 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 846 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 40:
#line 405 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 852 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 41:
#line 406 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 858 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 42:
#line 407 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 864 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 43:
#line 408 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 870 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 44:
#line 409 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 876 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 45:
#line 410 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 882 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 46:
#line 411 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 888 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 47:
#line 412 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 894 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 48:
#line 413 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 900 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 49:
#line 414 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 906 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 50:
#line 415 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.pool(yylhs.location, (yystack_[1].value.termvec)); }
#line 912 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 51:
#line 416 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 918 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 52:
#line 417 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 924 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 53:
#line 418 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.termvec)); }
#line 930 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 54:
#line 419 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 936 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 55:
#line 420 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 942 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 56:
#line 421 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 948 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 57:
#line 422 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 954 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 58:
#line 423 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 960 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 59:
#line 424 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 966 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 60:
#line 425 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String("_")); }
#line 972 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 61:
#line 431 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 978 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 62:
#line 432 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 984 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 63:
#line 438 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 990 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 64:
#line 439 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 996 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 65:
#line 443 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1002 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 66:
#line 444 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1008 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 67:
#line 448 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), true); }
#line 1014 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 68:
#line 449 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[0].value.termvec), false); }
#line 1020 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 69:
#line 450 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 1026 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 70:
#line 451 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 1032 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 71:
#line 454 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[1].value.term)); }
#line 1038 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 72:
#line 455 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[1].value.term)); }
#line 1044 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 73:
#line 458 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1050 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 74:
#line 459 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[1].value.termvec), (yystack_[0].value.term)); }
#line 1056 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 75:
#line 462 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec)); }
#line 1062 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 76:
#line 463 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[2].value.termvecvec), (yystack_[0].value.termvec)); }
#line 1068 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 77:
#line 467 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1074 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 78:
#line 468 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[4].value.termvecvec), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1080 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 79:
#line 478 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GT; }
#line 1086 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 80:
#line 479 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LT; }
#line 1092 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 81:
#line 480 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GEQ; }
#line 1098 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 82:
#line 481 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LEQ; }
#line 1104 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 83:
#line 482 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::EQ; }
#line 1110 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 84:
#line 483 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::NEQ; }
#line 1116 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 85:
#line 487 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1122 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 86:
#line 488 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1128 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 87:
#line 489 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1134 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 88:
#line 490 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1140 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 89:
#line 494 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1146 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 90:
#line 495 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1152 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 91:
#line 496 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1158 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 92:
#line 497 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1164 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 93:
#line 498 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1170 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 94:
#line 499 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1176 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 95:
#line 500 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::POS, (yystack_[0].value.term)); }
#line 1182 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 96:
#line 501 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOT, (yystack_[0].value.term)); }
#line 1188 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 97:
#line 502 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOTNOT, (yystack_[0].value.term)); }
#line 1194 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 98:
#line 503 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1200 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 99:
#line 504 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, neg((yystack_[1].value.rel)), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1206 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 100:
#line 505 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1212 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 101:
#line 506 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.csplit((yystack_[0].value.csplit)); }
#line 1218 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 102:
#line 510 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term),                     (yystack_[2].value.term)); }
#line 1224 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 103:
#line 511 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[3].value.term),                     (yystack_[0].value.term)); }
#line 1230 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 104:
#line 512 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, BUILDER.term(yylhs.location, Symbol::createNum(1)), (yystack_[0].value.term)); }
#line 1236 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 105:
#line 513 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term)); }
#line 1242 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 106:
#line 517 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), true); }
#line 1248 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 107:
#line 518 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), false); }
#line 1254 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 108:
#line 519 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[0].value.cspmulterm)); }
#line 1260 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 109:
#line 523 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GT; }
#line 1266 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 110:
#line 524 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LT; }
#line 1272 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 111:
#line 525 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GEQ; }
#line 1278 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 112:
#line 526 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LEQ; }
#line 1284 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 113:
#line 527 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::EQ; }
#line 1290 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 114:
#line 528 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::NEQ; }
#line 1296 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 115:
#line 532 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.csplit), (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1302 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 116:
#line 533 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.cspaddterm),   (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1308 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 117:
#line 541 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(BUILDER.litvec(), (yystack_[0].value.lit)); }
#line 1314 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 118:
#line 542 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec((yystack_[2].value.litvec), (yystack_[0].value.lit)); }
#line 1320 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 119:
#line 546 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1326 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 120:
#line 547 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1332 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 121:
#line 551 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1338 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 122:
#line 552 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1344 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 123:
#line 556 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1350 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 124:
#line 557 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1356 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 125:
#line 561 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::SUM; }
#line 1362 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 126:
#line 562 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::SUMP; }
#line 1368 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 127:
#line 563 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::MIN; }
#line 1374 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 128:
#line 564 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::MAX; }
#line 1380 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 129:
#line 565 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::COUNT; }
#line 1386 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 130:
#line 571 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelem) = { BUILDER.termvec(), (yystack_[0].value.litvec) }; }
#line 1392 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 131:
#line 572 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelem) = { (yystack_[1].value.termvec), (yystack_[0].value.litvec) }; }
#line 1398 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 132:
#line 576 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1404 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 133:
#line 577 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec((yystack_[2].value.bodyaggrelemvec), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1410 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 134:
#line 583 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lbodyaggrelem) = { (yystack_[1].value.lit), (yystack_[0].value.litvec) }; }
#line 1416 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 135:
#line 587 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1422 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 136:
#line 588 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1428 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 137:
#line 594 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
#line 1434 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 138:
#line 595 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist) }; }
#line 1440 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 139:
#line 596 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.bodyaggrelemvec() }; }
#line 1446 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 140:
#line 597 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.bodyaggrelemvec) }; }
#line 1452 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 141:
#line 601 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { Relation::LEQ, (yystack_[0].value.term) }; }
#line 1458 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 142:
#line 602 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { (yystack_[1].value.rel), (yystack_[0].value.term) }; }
#line 1464 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 143:
#line 603 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { Relation::LEQ, TermUid(-1) }; }
#line 1470 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 144:
#line 607 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1476 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 145:
#line 608 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1482 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 146:
#line 609 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1488 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 147:
#line 610 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1494 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 148:
#line 616 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec((yystack_[5].value.headaggrelemvec), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1500 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 149:
#line 617 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1506 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 150:
#line 621 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1512 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 151:
#line 622 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1518 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 152:
#line 628 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.headaggrelemvec() }; }
#line 1524 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 153:
#line 629 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.headaggrelemvec) }; }
#line 1530 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 154:
#line 630 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
#line 1536 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 155:
#line 631 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist)}; }
#line 1542 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 156:
#line 635 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1548 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 157:
#line 636 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1554 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 158:
#line 637 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1560 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 159:
#line 638 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1566 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 160:
#line 644 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(BUILDER.cspelemvec(), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1572 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 161:
#line 645 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec((yystack_[5].value.cspelemvec), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1578 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 162:
#line 649 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = (yystack_[0].value.cspelemvec); }
#line 1584 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 163:
#line 650 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(); }
#line 1590 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 164:
#line 654 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::POS, (yystack_[1].value.cspelemvec) }; }
#line 1596 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 165:
#line 655 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::NOT, (yystack_[1].value.cspelemvec) }; }
#line 1602 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 166:
#line 656 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::NOTNOT, (yystack_[1].value.cspelemvec) }; }
#line 1608 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 167:
#line 663 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lbodyaggrelem) = { (yystack_[2].value.lit), (yystack_[0].value.litvec) }; }
#line 1614 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 170:
#line 675 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1620 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 171:
#line 676 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[2].value.lit), (yystack_[1].value.litvec)); }
#line 1626 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 172:
#line 677 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(); }
#line 1632 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 173:
#line 682 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[4].value.lit), BUILDER.litvec()); }
#line 1638 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 174:
#line 683 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[4].value.lit), BUILDER.litvec()); }
#line 1644 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 175:
#line 684 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[6].value.lit), (yystack_[4].value.litvec)); }
#line 1650 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 176:
#line 685 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), (yystack_[0].value.litvec)); }
#line 1656 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 177:
#line 692 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1662 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 178:
#line 693 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1668 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 179:
#line 694 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1674 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 180:
#line 695 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1680 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 181:
#line 696 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1686 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 182:
#line 697 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1692 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 183:
#line 698 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1698 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 184:
#line 699 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1704 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 185:
#line 700 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1710 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 186:
#line 701 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1716 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 187:
#line 702 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1722 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 188:
#line 706 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1728 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 189:
#line 707 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1734 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 190:
#line 708 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1740 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 191:
#line 709 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1746 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 192:
#line 710 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1752 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 193:
#line 711 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1758 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 194:
#line 715 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1764 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 195:
#line 716 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1770 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 196:
#line 717 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1776 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 197:
#line 720 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = BUILDER.headlit((yystack_[0].value.lit)); }
#line 1782 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 198:
#line 721 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = BUILDER.disjunction(yylhs.location, (yystack_[0].value.condlitlist)); }
#line 1788 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 199:
#line 722 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = lexer->headaggregate(yylhs.location, (yystack_[0].value.uid)); }
#line 1794 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 200:
#line 726 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[1].value.head)); }
#line 1800 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 201:
#line 727 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head)); }
#line 1806 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 202:
#line 728 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head), (yystack_[0].value.body)); }
#line 1812 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 203:
#line 729 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), (yystack_[0].value.body)); }
#line 1818 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 204:
#line 730 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), BUILDER.body()); }
#line 1824 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 205:
#line 736 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint((yystack_[0].value.body), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1830 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 206:
#line 737 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1836 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 207:
#line 738 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[1].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[1].location, inv((yystack_[1].value.disjoint).first), (yystack_[1].value.disjoint).second)); }
#line 1842 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 208:
#line 744 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1848 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 209:
#line 745 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1854 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 210:
#line 749 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termpair) = {(yystack_[2].value.term), (yystack_[0].value.term)}; }
#line 1860 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 211:
#line 750 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termpair) = {(yystack_[0].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0))}; }
#line 1866 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 212:
#line 754 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit(BUILDER.body(), (yystack_[0].value.lit)); }
#line 1872 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 213:
#line 755 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[0].value.lit)); }
#line 1878 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 214:
#line 759 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1884 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 215:
#line 760 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1890 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 216:
#line 761 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1896 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 217:
#line 765 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[4].value.body)); }
#line 1902 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 218:
#line 766 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), BUILDER.body()); }
#line 1908 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 219:
#line 770 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1914 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 220:
#line 771 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1920 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 221:
#line 775 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1926 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 222:
#line 776 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1932 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 227:
#line 789 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), false); }
#line 1938 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 228:
#line 790 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true), false); }
#line 1944 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 229:
#line 791 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig("", 0, false), false); }
#line 1950 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 230:
#line 792 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), false); }
#line 1956 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 231:
#line 793 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), false); }
#line 1962 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 232:
#line 794 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), true); }
#line 1968 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 233:
#line 795 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), true); }
#line 1974 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 234:
#line 796 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), true); }
#line 1980 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 235:
#line 802 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.edge(yylhs.location, (yystack_[2].value.termvecvec), (yystack_[0].value.body)); }
#line 1986 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 236:
#line 808 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.heuristic(yylhs.location, (yystack_[8].value.term), (yystack_[7].value.body), (yystack_[5].value.term), (yystack_[3].value.term), (yystack_[1].value.term)); }
#line 1992 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 237:
#line 809 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.heuristic(yylhs.location, (yystack_[6].value.term), (yystack_[5].value.body), (yystack_[3].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0)), (yystack_[1].value.term)); }
#line 1998 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 238:
#line 815 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 2004 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 239:
#line 816 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 2010 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 240:
#line 817 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, (yystack_[1].value.term), (yystack_[0].value.body)); }
#line 2016 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 241:
#line 823 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    {  BUILDER.define(yylhs.location, String::fromRep((yystack_[2].value.str)), (yystack_[0].value.term), false, LOGGER); }
#line 2022 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 242:
#line 827 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    {  BUILDER.define(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.term), true, LOGGER); }
#line 2028 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 243:
#line 833 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.python(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2034 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 244:
#line 834 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.lua(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2040 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 245:
#line 840 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->include(String::fromRep((yystack_[1].value.str)), yylhs.location, false, LOGGER); }
#line 2046 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 246:
#line 841 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->include(String::fromRep((yystack_[2].value.str)), yylhs.location, true, LOGGER); }
#line 2052 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 247:
#line 847 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec((yystack_[2].value.idlist), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2058 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 248:
#line 848 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec(BUILDER.idvec(), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2064 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 249:
#line 852 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec(); }
#line 2070 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 250:
#line 853 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = (yystack_[0].value.idlist); }
#line 2076 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 251:
#line 857 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[4].value.str)), (yystack_[2].value.idlist)); }
#line 2082 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 252:
#line 858 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[1].value.str)), BUILDER.idvec()); }
#line 2088 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 253:
#line 864 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body)); }
#line 2094 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 254:
#line 865 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), BUILDER.body()); }
#line 2100 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 255:
#line 866 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[1].value.term), BUILDER.body()); }
#line 2106 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 256:
#line 874 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[1].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2112 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 257:
#line 875 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2118 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 258:
#line 879 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermset(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2124 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 259:
#line 880 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theoryoptermlist(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2130 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 260:
#line 881 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms()); }
#line 2136 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 261:
#line 882 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermopterm(yylhs.location, (yystack_[1].value.theoryOpterm)); }
#line 2142 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 262:
#line 883 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[2].location, (yystack_[2].value.theoryOpterm))); }
#line 2148 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 263:
#line 884 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(yystack_[3].location, (yystack_[3].value.theoryOpterm), (yystack_[1].value.theoryOpterms))); }
#line 2154 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 264:
#line 885 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermfun(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.theoryOpterms)); }
#line 2160 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 265:
#line 886 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 2166 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 266:
#line 887 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 2172 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 267:
#line 888 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 2178 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 268:
#line 889 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createInf()); }
#line 2184 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 269:
#line 890 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createSup()); }
#line 2190 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 270:
#line 891 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvar(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 2196 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 271:
#line 895 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[2].value.theoryOpterm), (yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2202 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 272:
#line 896 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2208 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 273:
#line 897 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm(BUILDER.theoryops(), (yystack_[0].value.theoryTerm)); }
#line 2214 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 274:
#line 901 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms((yystack_[2].value.theoryOpterms), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2220 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 275:
#line 902 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2226 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 276:
#line 906 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = (yystack_[0].value.theoryOpterms); }
#line 2232 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 277:
#line 907 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(); }
#line 2238 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 278:
#line 911 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElem) = { (yystack_[2].value.theoryOpterms), (yystack_[0].value.litvec) }; }
#line 2244 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 279:
#line 912 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElem) = { BUILDER.theoryopterms(), (yystack_[0].value.litvec) }; }
#line 2250 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 280:
#line 916 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems((yystack_[3].value.theoryElems), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2256 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 281:
#line 917 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(BUILDER.theoryelems(), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2262 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 282:
#line 921 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = (yystack_[0].value.theoryElems); }
#line 2268 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 283:
#line 922 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(); }
#line 2274 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 284:
#line 926 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), false); }
#line 2280 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 285:
#line 927 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 2286 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 286:
#line 930 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[6].value.term), (yystack_[3].value.theoryElems)); }
#line 2292 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 287:
#line 931 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[8].value.term), (yystack_[5].value.theoryElems), String::fromRep((yystack_[2].value.str)), yystack_[1].location, (yystack_[1].value.theoryOpterm)); }
#line 2298 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 288:
#line 937 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2304 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 289:
#line 938 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[2].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2310 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 290:
#line 942 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = (yystack_[0].value.theoryOps); }
#line 2316 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 291:
#line 943 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(); }
#line 2322 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 292:
#line 947 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.num), TheoryOperatorType::Unary); }
#line 2328 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 293:
#line 948 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryLeft); }
#line 2334 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 294:
#line 949 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryRight); }
#line 2340 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 295:
#line 953 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(BUILDER.theoryopdefs(), (yystack_[0].value.theoryOpDef)); }
#line 2346 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 296:
#line 954 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs((yystack_[3].value.theoryOpDefs), (yystack_[0].value.theoryOpDef)); }
#line 2352 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 297:
#line 958 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = (yystack_[0].value.theoryOpDefs); }
#line 2358 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 298:
#line 959 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(); }
#line 2364 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 299:
#line 963 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2370 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 300:
#line 964 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("left"); }
#line 2376 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 301:
#line 965 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("right"); }
#line 2382 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 302:
#line 966 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("unary"); }
#line 2388 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 303:
#line 967 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("binary"); }
#line 2394 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 304:
#line 968 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("head"); }
#line 2400 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 305:
#line 969 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("body"); }
#line 2406 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 306:
#line 970 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("any"); }
#line 2412 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 307:
#line 971 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("directive"); }
#line 2418 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 308:
#line 975 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTermDef) = BUILDER.theorytermdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.theoryOpDefs), LOGGER); }
#line 2424 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 309:
#line 979 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Head; }
#line 2430 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 310:
#line 980 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Body; }
#line 2436 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 311:
#line 981 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Any; }
#line 2442 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 312:
#line 982 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Directive; }
#line 2448 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 313:
#line 987 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[14].value.str)), (yystack_[12].value.num), String::fromRep((yystack_[10].value.str)), (yystack_[0].value.theoryAtomType), (yystack_[6].value.theoryOps), String::fromRep((yystack_[2].value.str))); }
#line 2454 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 314:
#line 988 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.num), String::fromRep((yystack_[2].value.str)), (yystack_[0].value.theoryAtomType)); }
#line 2460 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 315:
#line 992 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryAtomDef)); }
#line 2466 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 316:
#line 993 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryTermDef)); }
#line 2472 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 317:
#line 994 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryAtomDef)); }
#line 2478 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 318:
#line 995 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryTermDef)); }
#line 2484 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 319:
#line 999 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = (yystack_[0].value.theoryDefs); }
#line 2490 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 320:
#line 1000 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(); }
#line 2496 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 321:
#line 1004 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.theorydef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[3].value.theoryDefs), LOGGER); }
#line 2502 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 322:
#line 1010 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Theory); }
#line 2508 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 323:
#line 1014 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Definition); }
#line 2514 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 324:
#line 1018 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Disabled); }
#line 2520 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;


#line 2524 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
            default:
              break;
            }
        }
      catch (const syntax_error& yyexc)
        {
          error (yyexc);
          YYERROR;
        }
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, yylhs);
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

    /* Pacify compilers like GCC when the user code never invokes
       YYERROR and the label yyerrorlab therefore never appears in user
       code.  */
    if (false)
      goto yyerrorlab;
    yyerror_range[1].location = yystack_[yylen - 1].location;
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
      yypush_ ("Shifting", error_token);
    }
    goto yynewstate;

    // Accept.
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;

    // Abort.
  yyabortlab:
    yyresult = 1;
    goto yyreturn;

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
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack"
                 << std::endl;
        // Do not try to display the values of the reclaimed symbols,
        // as their printer might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what());
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
        YYCASE_(0, YY_("syntax error"));
        YYCASE_(1, YY_("syntax error, unexpected %s"));
        YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
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


  const short int parser::yypact_ninf_ = -478;

  const short int parser::yytable_ninf_ = -325;

  const short int
  parser::yypact_[] =
  {
     159,  -478,   -20,    67,   842,  -478,    72,    47,  -478,  -478,
     -20,   -20,  1371,   -20,  -478,  1371,   116,  -478,     2,  -478,
     136,    21,  -478,   346,  1202,  -478,   128,  -478,   162,  1162,
     161,    22,     2,    37,  1371,  -478,  -478,  -478,  -478,   -20,
    1371,   190,   -20,  -478,  -478,  -478,   194,   196,  -478,  -478,
     708,  -478,    93,  1494,  -478,   123,  -478,  1464,  1483,   188,
    1008,  -478,   137,  -478,   154,  -478,   166,  -478,    26,   199,
    -478,   222,  1371,   243,  -478,   269,   287,  1233,   -20,   249,
      53,  -478,   650,  -478,   -20,   267,  -478,   719,  1640,   296,
     180,  -478,  1870,   300,   281,  1202,   268,  1264,  1271,  1371,
    -478,  1686,  1371,   -20,    96,   139,   139,   -20,   -20,   293,
     339,  -478,    61,  1870,   151,   308,   317,  -478,  -478,  -478,
     325,  -478,  -478,  1131,  1670,  -478,  1371,  1371,  1371,  -478,
     349,  1371,  -478,  -478,  -478,  -478,  1371,  1371,  -478,  1371,
    1371,  1371,  1371,  1371,  1060,  1008,  1021,  -478,  -478,  -478,
    -478,  1302,  1302,  -478,  -478,  -478,  -478,  -478,  -478,  1302,
    1302,  1333,  1870,  1371,  -478,  -478,   343,  -478,   354,   -20,
     166,  -478,  1429,   166,  -478,   166,  -478,  -478,   345,  1907,
    -478,  -478,  1371,   338,  1371,  1371,   166,  1371,   383,   388,
    -478,   186,   363,  1371,   378,  -478,   584,   935,  1544,    63,
     379,  1008,    46,    38,    64,  -478,   389,  -478,  1171,  1371,
    1021,  -478,  -478,  1021,  1371,  -478,   381,  -478,   398,   635,
     433,   236,   426,   433,   237,  1716,  -478,  -478,  1762,   215,
      98,   373,   437,  -478,  -478,   427,   411,   414,   393,  1371,
    -478,   -20,  1371,  -478,  1371,  1371,   431,  1233,   445,  -478,
    -478,  1670,  -478,  1371,  -478,   228,   214,   777,  1371,  1928,
     434,   434,   434,  1018,   434,   214,   523,  1870,  1008,  -478,
    -478,    43,  1021,  1021,  1767,  -478,  -478,   303,   303,  -478,
     473,   257,  1870,  -478,  -478,  -478,  -478,   448,  -478,   441,
    -478,  1907,    69,  -478,  1877,   166,   166,   166,   166,   166,
     166,   166,   166,   166,   166,   273,   728,   286,   330,  1901,
    1870,  1371,  1302,  -478,  1371,  1371,   335,  -478,  -478,  -478,
     296,  -478,   263,   948,  1594,    59,  1073,  1008,  1021,  -478,
    -478,  -478,   416,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,   463,   483,  -478,   296,  1870,  -478,  -478,  1371,  1371,
     486,   471,  1371,  -478,   486,   478,  1371,  -478,  -478,  -478,
    1371,   139,  1371,   438,   484,  -478,  -478,  1371,   442,   447,
     499,   348,  -478,   529,   489,  1870,   433,   433,   550,   290,
    1233,  1371,  1870,   298,  1371,  1870,  -478,  1021,  -478,   408,
     408,  1021,  -478,  1371,   166,  -478,  1407,  -478,  -478,   531,
     491,   311,  1144,   498,   498,   498,  1127,   498,   311,  1518,
    -478,  -478,   423,   423,  1944,  -478,  -478,  -478,  -478,  -478,
     509,   825,  -478,   458,   539,  -478,   504,  -478,   541,  -478,
    -478,  -478,   178,   548,   376,  -478,  -478,  -478,  1021,  1594,
     103,  1073,  -478,  -478,  -478,  1008,  -478,  -478,  1021,  -478,
     429,  -478,   314,  -478,  -478,  1870,   383,  1021,  -478,  -478,
     433,  -478,  -478,   433,  -478,  1870,  -478,  1791,   533,  -478,
    1733,   535,   536,  -478,   435,   -20,   537,   515,   516,   622,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,  -478,
    -478,  -478,  -478,   318,  -478,   326,  1870,  -478,  -478,  1021,
    1021,  -478,   153,   153,   296,   564,   524,  -478,  1907,   166,
    -478,   539,   527,   528,  -478,    49,   423,  -478,  -478,   825,
     423,   296,   525,   534,  1021,  -478,  1302,  -478,  -478,  1073,
    -478,  -478,  -478,  -478,  -478,  -478,  -478,  1364,  -478,   570,
     486,   486,  1371,  -478,  1371,  1371,  -478,  -478,  -478,  -478,
    -478,  -478,   526,   551,  -478,   550,  -478,   408,   483,  -478,
    -478,  1021,  -478,  -478,  -478,  1954,  -478,   543,  -478,   458,
    -478,   423,   495,  -478,   178,  -478,  1021,  -478,  -478,  1870,
    1819,  1843,   512,   506,   573,  -478,  -478,   153,   296,  -478,
      73,  -478,  -478,   423,  -478,  -478,  -478,  1371,  -478,   594,
    -478,  -478,   504,  -478,  -478,  -478,  -478,   458,  1850,   622,
     595,   556,   560,  -478,  -478,   600,   530,   506,  -478,   184,
     601,  -478,  -478,  -478,  -478,  -478,  -478,   579,   366,   542,
    -478,   605,  -478,   606,  -478,   371,   552,   568,  -478,  -478,
    -478,   609,   622,   610,   184,  -478
  };

  const unsigned short int
  parser::yydefact_[] =
  {
       0,     5,     0,     0,     0,    10,     0,     0,     1,   324,
       0,     0,     0,     0,   129,     0,     0,     7,     0,    92,
     187,     0,    57,     0,    70,   128,     0,   127,     0,     0,
       0,     0,     0,     0,     0,   125,   126,    58,    89,     0,
       0,   187,     0,     6,    55,    60,     0,     0,    56,    59,
       0,     4,    54,   105,    95,   197,   108,     0,   101,     0,
     143,   199,     0,   198,     0,   159,     0,     3,     0,   284,
     322,     0,     0,    54,    49,     0,   104,   163,     0,    85,
       0,   204,     0,   203,     0,     0,   154,     0,   105,   122,
       0,    69,    63,    68,    73,    70,     0,     0,     0,     0,
     229,     0,     0,     0,    85,     0,     0,     0,     0,     0,
      54,    48,     0,    61,     0,     0,     0,   323,   243,   244,
       0,    93,    90,     0,     0,    96,    66,     0,     0,    83,
       0,     0,    81,    79,    82,    80,     0,     0,    84,     0,
       0,     0,     0,     0,     0,   143,     0,   172,   168,   169,
     172,     0,     0,   112,   110,   109,   111,   113,   114,     0,
       0,    66,   141,     0,   158,   207,   187,   200,   187,     0,
       0,    32,     0,     0,    33,     0,    30,    31,    29,   241,
       8,     9,    66,     0,    66,    66,     0,     0,    65,     0,
     162,     0,    87,    66,   187,   255,     0,     0,   105,     0,
       0,   143,     0,     0,     0,   147,     0,   245,     0,     0,
     120,   150,   155,     0,    67,    71,    74,    50,     0,   211,
     209,     0,     0,   209,     0,     0,   187,   231,     0,     0,
      87,     0,   187,   194,   240,     0,     0,     0,     0,    66,
     252,   249,     0,    53,     0,     0,     0,   163,     0,    94,
      91,     0,    97,     0,    75,     0,    42,    41,     0,    38,
      46,    44,    47,    40,    45,    43,    39,    98,   143,   156,
     117,   176,     0,     0,   105,   106,   107,   116,   115,   152,
       0,     0,   142,   206,   205,   201,   202,     0,    21,     0,
      22,    34,     0,    20,     0,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   283,     0,     0,     0,
     102,     0,     0,   164,    66,    66,     0,   254,   253,   137,
     122,   135,     0,     0,     0,     0,     0,   143,   120,   177,
     188,   178,     0,   146,   179,   189,   180,   193,   186,   192,
     185,     0,   119,   121,   122,    64,    72,   224,     0,     0,
     216,     0,     0,   223,   216,     0,     0,   187,   234,   230,
       0,     0,     0,     0,     0,   195,   196,     0,     0,     0,
       0,     0,   248,   250,     0,    62,   209,   209,   320,     0,
     163,     0,    99,    51,    66,   103,   157,     0,   172,   124,
     124,     0,   153,    66,    37,    23,     0,    24,    28,    36,
       0,    14,    13,    18,    16,    19,    12,    17,    15,    11,
     285,   268,   277,   277,     0,   269,   266,   267,   270,   257,
     265,     0,   273,   275,   324,   281,   282,   322,     0,    52,
      51,   242,   122,     0,     0,    86,   134,   138,     0,     0,
       0,     0,   181,   190,   182,   143,   144,   167,   120,   139,
     122,   132,     0,   246,   151,   210,   208,   215,   219,   226,
     209,   221,   225,   209,   233,    77,   235,     0,     0,   238,
       0,     0,     0,   227,    51,     0,     0,     0,     0,     0,
     306,   302,   303,   300,   301,   304,   305,   307,   299,   322,
     318,   317,   319,     0,   165,     0,   100,    76,   118,     0,
       0,   170,   173,   174,   122,     0,     0,    25,    35,     0,
      26,   276,     0,     0,   260,     0,   277,   256,   272,     0,
       0,   122,     0,     0,   120,   160,     0,    88,   136,     0,
     183,   191,   184,   145,   130,   131,   140,     0,   212,   214,
     216,   216,     0,   239,     0,     0,   232,   228,   247,   251,
     218,   217,     0,     0,   324,     0,   166,   124,   123,   171,
     149,     0,    27,   258,   259,     0,   261,     0,   271,   274,
     278,   324,   324,   279,   122,   133,     0,   220,   222,    78,
       0,     0,     0,   298,     0,   316,   315,   175,   122,   262,
       0,   264,   280,     0,   286,   161,   213,     0,   237,     0,
     323,   295,   297,   323,   321,   148,   263,   324,     0,     0,
       0,     0,     0,   287,   236,     0,     0,     0,   308,   322,
       0,   296,   311,   309,   310,   312,   314,     0,     0,   291,
     292,     0,   288,   290,   323,     0,     0,     0,   293,   294,
     289,     0,     0,     0,     0,   313
  };

  const short int
  parser::yypgoto_[] =
  {
    -478,  -478,  -478,  -478,    -2,   -56,   449,   250,   392,  -478,
     -21,   -64,   553,  -478,  -478,  -127,  -478,   -49,     4,    11,
     319,  -154,   591,  -478,  -139,  -302,  -252,  -366,    41,    94,
    -478,   217,  -478,  -186,  -131,  -164,  -478,  -478,    -1,  -478,
    -478,  -203,   571,  -478,   -35,  -132,  -478,  -478,   -16,   -83,
    -478,  -195,   -79,  -478,  -327,  -478,  -478,  -478,  -478,  -478,
    -380,  -372,  -367,  -289,  -375,    81,  -478,  -478,  -478,   653,
    -478,  -478,    42,  -478,  -478,  -433,   111,    27,   115,  -478,
    -478,  -385,  -477,    -8
  };

  const short int
  parser::yydefgoto_[] =
  {
      -1,     3,     4,    51,    73,   291,   399,   400,    92,   114,
     188,   254,    94,    95,    96,   255,   229,   163,    54,   270,
      56,    57,   159,    58,   342,   343,   211,   502,   200,   451,
     452,   321,   322,   201,   164,   202,   281,    90,    60,    61,
     190,   191,    62,   204,   559,   272,    63,    82,    83,   234,
      64,   350,   220,   539,   458,   221,   224,     7,   373,   374,
     421,   422,   423,   511,   512,   425,   426,   427,    70,   205,
     633,   634,   601,   602,   603,   489,   490,   626,   491,   492,
     493,   183,   246,   428
  };

  const short int
  parser::yytable_[] =
  {
       6,    68,    52,    93,   144,   277,   278,   271,    69,    71,
     179,    75,   327,   189,   269,    55,    79,   424,   273,   223,
     150,    52,    80,   235,   503,   116,   447,   461,   354,   104,
      79,   109,   110,   325,    89,   105,   106,   112,   513,   209,
     117,   522,   523,   519,   379,    59,   552,   515,    52,   518,
     107,   180,   145,   387,   125,   305,   334,   307,   308,   565,
      84,    78,   194,   337,   178,     5,   316,     8,   436,   442,
     333,   335,   328,   329,    93,   253,   192,    66,   195,   396,
      52,   103,   206,   520,   443,    52,   240,     5,   330,   339,
     338,   125,   454,   199,    59,   148,   108,   280,   336,   241,
     566,   230,   -85,   -85,   553,   236,   237,     5,   181,    85,
     149,   444,   371,   530,   288,   331,   340,   293,   -85,   294,
     397,    52,     5,   610,   606,   -85,   612,   252,   531,    67,
     309,   126,   146,   147,   193,   519,   315,   386,   327,   419,
     445,   567,   -85,   268,    52,   -85,   534,   568,   232,   326,
     284,    77,   286,   569,   231,   532,   363,   637,   432,   440,
     -85,    81,   165,    97,   233,   376,   377,   287,   178,   166,
     178,   178,   169,   178,   170,   148,   615,   495,   318,   167,
     525,   477,   478,   189,   178,    59,   168,   210,   434,   519,
     149,   587,   151,   152,    52,    52,   446,    98,   535,   102,
     171,   125,   381,   242,   172,   148,    52,   320,    52,   643,
     359,    52,   252,   577,   578,   115,   366,   611,   243,   118,
     149,   119,   573,   161,   344,   173,   607,   519,   174,   212,
       1,     2,   213,   175,   627,   313,   388,   182,   314,   372,
     401,   402,   403,   404,   405,   406,   407,   408,   409,   176,
     433,     5,   560,   327,   177,   445,   499,   622,   136,   137,
     184,   139,   623,   624,   625,   540,   361,   362,   541,   570,
      52,    52,   141,   460,   186,   441,   590,   463,   466,   383,
     384,   185,   424,   389,   390,   351,   355,   193,   352,   356,
     127,   128,   207,   178,   178,   178,   178,   178,   178,   178,
     178,   178,   178,   187,   420,   210,   392,   -86,   -86,   393,
     214,   450,   437,   131,   533,   438,   189,   151,   152,   217,
     497,    52,   595,   -86,   410,   384,    52,   252,   456,   505,
     -86,   136,   137,   215,   139,   140,   605,   429,   384,   494,
     508,   464,   314,   445,   244,   141,   142,   -86,   -87,   -87,
     -86,   238,    11,   245,    12,   298,   299,   143,   300,    15,
     247,   558,   258,   536,   -87,   -86,   537,   554,   283,   302,
     555,   -87,   574,   306,    19,   556,   488,   239,   314,   285,
      22,   430,   384,   295,    24,    52,   435,   384,   -87,    52,
     529,   -87,   178,   311,   178,    86,    53,   312,   498,   474,
     384,   315,   504,   317,    74,    34,   -87,    76,    37,    38,
     420,   420,   420,    40,   332,    88,   521,   500,   501,   420,
     341,   101,    11,   347,    12,   448,   111,   527,   384,    44,
      45,     5,   113,   346,    48,    49,    52,    87,   210,   311,
     630,   631,   124,   349,   -88,   -88,    52,   638,   639,   320,
      22,   353,   162,   508,    24,    52,   364,   411,   412,   413,
     -88,   414,   365,   367,   111,   449,   378,   -88,   538,   368,
     275,   276,   369,   548,   198,    72,   370,   488,    37,   124,
     380,   139,   391,    40,   -88,   415,   394,   -88,   453,   219,
     219,   225,   395,   387,   228,   457,   459,    52,    52,    44,
      45,     5,   -88,   462,    48,    49,   416,   178,     5,   469,
     557,   417,   418,   419,   420,   251,   450,   420,   420,   256,
     257,   468,    52,   259,   473,   471,   127,   128,   260,   261,
     472,   262,   263,   264,   265,   266,   267,   162,    88,   475,
     476,   509,   510,   274,   274,   300,   584,   516,   419,   520,
     524,   274,   274,   488,   479,   282,  -322,   526,   543,    52,
     546,   547,   549,   420,   594,   550,   551,   136,   137,   420,
     139,   140,   588,   561,    52,   562,   563,   571,   564,   310,
     576,   141,   142,   572,   582,   593,   583,   596,    88,   324,
      11,   420,    12,   162,   591,   599,   600,    15,   604,   613,
     251,   267,    88,   609,   616,    88,   345,   488,   617,   618,
     619,   628,    19,   620,   629,   635,   636,   641,    22,   642,
     644,   292,    24,   480,   481,   482,   483,   484,   485,   486,
     487,   575,   632,   319,   375,     5,   219,   219,   127,   128,
     488,   348,   640,    34,   506,   382,    37,    38,   216,   160,
     385,    40,   592,   203,    10,   528,    11,    65,    12,   621,
     162,   131,    14,    15,    88,    88,   585,    44,    45,     5,
     586,   645,    48,    49,    16,    87,     0,     0,    19,   136,
     137,     0,   139,   140,    22,   196,     0,     0,    24,     0,
      25,     0,    27,   141,   142,   480,   481,   482,   483,   484,
     485,   486,   487,   345,   274,   143,     0,     5,     0,    34,
      35,    36,    37,    38,    11,   439,    12,    40,   267,   162,
      88,     0,     0,     0,     0,    11,     0,    12,     0,     0,
       0,     0,   120,    44,    45,     5,   121,  -324,    48,    49,
     455,   197,    22,     0,   219,     0,    24,   121,   219,     0,
       0,     0,   465,    22,   467,     0,     0,    24,     0,   470,
       0,     0,   411,   412,   413,     0,   414,    34,     0,     0,
      37,   122,     0,   496,     0,    40,     0,     0,    34,    88,
     127,    37,   122,    88,     0,     0,    40,     0,     0,     0,
     415,    44,    45,     5,     0,     0,    48,    49,     0,   123,
       0,     0,    44,    45,     5,     0,     0,    48,    49,     0,
     208,   416,     0,     5,     0,     0,   417,   418,   419,     0,
       0,   136,   137,     0,   139,     0,     0,     0,     0,     0,
      88,     0,     0,   382,     0,   141,   142,   162,     0,     0,
      88,     0,    -2,     9,     0,     0,    10,     0,    11,    88,
      12,     0,     0,    13,    14,    15,     0,     0,     0,   411,
     412,   413,     0,   414,     0,     0,    16,    17,     0,    18,
      19,     0,     0,     0,    20,    21,    22,    23,     0,     0,
      24,     0,    25,    26,    27,    28,     0,   415,     0,     0,
       0,    88,    88,     0,     0,    29,    30,    31,    32,    33,
       0,    34,    35,    36,    37,    38,    39,     0,   416,    40,
       5,    41,     0,   417,   418,   517,    88,     0,   274,     0,
       0,   496,     0,    42,    43,    44,    45,     5,    46,    47,
      48,    49,     0,    50,   579,     0,   580,   581,     0,    10,
       0,    11,     0,    12,     0,     0,     0,    14,     0,     0,
       0,     0,    10,    88,    11,     0,    12,     0,     0,   120,
      14,     0,     0,   121,     0,     0,     0,     0,    88,    22,
     196,     0,   248,    24,     0,    25,   249,    27,     0,     0,
       0,     0,    22,   196,     0,     0,    24,     0,    25,   608,
      27,     0,     0,     0,    34,    35,    36,    37,   122,     0,
       0,     0,    40,     0,     0,     0,     0,    34,    35,    36,
      37,   250,     0,   129,    11,    40,    12,     0,    44,    45,
       5,   127,   128,    48,    49,     0,   323,    11,     0,    12,
       0,    44,    45,     5,    15,     0,    48,    49,   132,   133,
       0,     0,    22,     0,     0,   134,    24,   135,     0,    19,
       0,     0,     0,     0,   138,    22,     0,     0,     0,    24,
       0,     0,   136,   137,     0,   139,    11,    72,    12,     0,
      37,     0,    14,     0,     0,    40,   141,   142,     0,    11,
      34,    12,     0,    37,    38,    14,     0,     0,    40,     0,
       0,    44,    45,     5,    22,    23,    48,    49,    24,     0,
      25,     0,    27,     0,    44,    45,     5,    22,   196,    48,
      49,    24,    87,    25,     0,    27,     0,     0,     0,    72,
      35,    36,    37,     0,     0,     0,     0,    40,     0,     0,
     296,   297,    72,    35,    36,    37,     0,    11,     0,    12,
      40,     0,     0,    44,    45,     5,     0,   296,    48,    49,
       0,     0,     0,     0,     0,   248,    44,    45,     5,   249,
       0,    48,    49,     0,     0,    22,     0,     0,    11,    24,
      12,   298,   299,     0,   300,    99,     0,    11,     0,    12,
       0,     0,     0,     0,     0,   302,   303,   100,   298,   299,
      34,   300,     0,    37,   250,     0,    22,     0,    40,   249,
      24,     0,   302,   303,     0,    22,     0,     0,    11,    24,
      12,     0,    91,     0,    44,    45,     5,     0,     0,    48,
      49,    72,     0,     0,    37,     0,     0,     0,     0,    40,
      34,     0,     0,    37,   250,     0,    22,     0,    40,    11,
      24,    12,   -66,     0,     0,    44,    45,     5,     0,     0,
      48,    49,     0,     0,    44,    45,     5,     0,     0,    48,
      49,    72,     0,     0,    37,     0,     0,    22,     0,    40,
      11,    24,    12,     0,     0,     0,     0,    11,     0,    12,
       0,     0,     0,     0,     0,    44,    45,     5,     0,     0,
      48,    49,    72,     0,     0,    37,     0,     0,    22,     0,
      40,     0,    24,     0,     0,    22,     0,     0,    11,    24,
      12,     0,     0,   218,     0,    15,    44,    45,     5,     0,
     222,    48,    49,    72,     0,     0,    37,     0,     0,     0,
      72,    40,     0,    37,     0,     0,    22,     0,    40,    11,
      24,    12,     0,     0,     0,     0,     0,    44,    45,     5,
       0,     0,    48,    49,    44,    45,     5,     0,     0,    48,
      49,    72,     0,     0,    37,     0,     0,    22,     0,    40,
      11,    24,    12,   448,     0,     0,     0,    11,     0,    12,
       0,     0,   279,     0,     0,    44,    45,     5,     0,     0,
      48,    49,    72,     0,     0,    37,     0,     0,    22,     0,
      40,     0,    24,     0,     0,    22,     0,     0,     0,    24,
       0,     0,     0,   169,     0,   170,    44,    45,     5,     0,
       0,    48,    49,    72,     0,     0,    37,     0,     0,     0,
      72,    40,     0,    37,     0,   169,     0,   170,    40,   289,
       0,   171,     0,     0,     0,   172,     0,    44,    45,     5,
       0,     0,    48,    49,    44,    45,     5,     0,   507,    48,
      49,     0,     0,   171,     0,     0,   173,   172,     0,   174,
       0,     0,     0,     0,   175,     0,     0,     0,   151,   152,
     290,   153,   154,   155,   156,   157,   158,     0,   173,     0,
     176,   174,     5,     0,     0,   177,   175,   127,   128,   129,
     153,   154,   155,   156,   157,   158,    14,     0,     0,     0,
     130,     0,   176,     0,     5,     0,     0,   177,     0,     0,
     131,   296,   297,     0,   132,   133,     0,     0,     0,    23,
       0,   134,     0,   135,    25,     0,    27,     0,   136,   137,
     138,   139,   140,     0,     0,     0,     0,   127,   128,   129,
       0,     0,   141,   142,    35,    36,    14,     0,     0,     0,
     130,     0,   298,   299,   143,   300,   301,     0,     0,     0,
     131,     0,     0,     0,   132,   133,   302,   303,     0,   196,
       0,   134,     0,   135,    25,     0,    27,     0,   136,   137,
     138,   139,   140,     0,     0,     0,     0,   127,   128,   129,
       0,     0,   141,   142,    35,    36,    14,     0,     0,     0,
       0,     0,     0,     0,   143,     0,     0,     0,     0,     0,
     131,     0,     0,     0,   132,   133,     0,     0,     0,   196,
       0,   134,     0,   135,    25,     0,    27,     0,   136,   137,
     138,   139,   140,   127,   128,   129,     0,     0,     0,     0,
       0,     0,   141,   142,    35,    36,   130,     0,     0,     0,
       0,     0,     0,     0,   143,     0,   131,     0,     0,     0,
     132,   133,     0,   127,   128,   129,     0,   134,     0,   135,
       0,     0,     0,     0,   136,   137,   138,   139,   140,   127,
     128,     0,     0,     0,     0,   226,   131,     0,   141,   142,
     132,   133,     0,     0,     0,     0,     0,   134,     0,   135,
     143,   227,   131,     0,   136,   137,   138,   139,   140,   127,
     128,     0,     0,     0,     0,   357,     0,     0,   141,   142,
     136,   137,     0,   139,   140,     0,   127,   128,     0,   544,
     143,   358,   131,   545,   141,   142,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   143,     0,     0,   131,
     136,   137,     0,   139,   140,   127,   128,     0,     0,     0,
     127,   128,   360,     0,   141,   142,     0,   136,   137,     0,
     139,   140,     0,   130,     0,     0,   143,     0,   131,     0,
       0,   141,   142,   131,   127,   128,     0,     0,     0,     0,
       0,   542,     0,   143,     0,     0,   136,   137,     0,   139,
     140,   136,   137,     0,   139,   140,     0,   131,     0,     0,
     141,   142,   127,   128,     0,   141,   142,     0,     0,   597,
       0,     0,   143,     0,     0,   136,   137,   143,   139,   140,
       0,     0,     0,     0,     0,   131,   127,   128,     0,   141,
     142,     0,     0,   127,   128,     0,     0,     0,     0,     0,
       0,   143,     0,   136,   137,     0,   139,   140,     0,   131,
       0,     0,     0,   127,   128,     0,   131,   141,   142,     0,
     296,   297,     0,     0,     0,     0,     0,   136,   137,   143,
     139,   140,     0,   598,   136,   137,   131,   139,   140,     0,
     614,   141,   142,     0,   296,   297,     0,     0,   141,   142,
     296,   297,     0,   143,   136,   137,     0,   139,   140,     0,
     143,   298,   299,     0,   300,   301,   431,     0,   141,   142,
       0,   127,   128,     0,     0,   302,   303,     0,     0,     0,
     143,     0,     0,     0,   398,   298,   299,   304,   300,   301,
       0,   298,   299,     0,   300,   301,     0,     0,     0,   302,
     303,     0,     0,     0,     0,   302,   303,     0,     0,     0,
       0,   304,   136,   137,     0,   139,   140,   304,   411,   412,
     413,     0,   414,     0,     0,     0,   141,   142,   411,   412,
     413,     0,   414,     0,     0,   514,     0,     0,   143,     0,
       0,     0,     0,     0,     0,   589,   415,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   415,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   416,     0,     5,
       0,     0,   417,   418,   419,     0,     0,   416,     0,     5,
       0,     0,   417,   418,   419
  };

  const short int
  parser::yycheck_[] =
  {
       2,     9,     4,    24,    53,   159,   160,   146,    10,    11,
      66,    13,   198,    77,   145,     4,    18,   306,   150,    98,
      55,    23,    18,   106,   390,    41,   328,   354,   223,    31,
      32,    33,    34,   197,    23,    31,    32,    39,   413,    88,
      42,   426,   427,   423,   247,     4,   479,   414,    50,   421,
      13,    25,    53,    10,    50,   182,    10,   184,   185,    10,
      39,    59,     9,    25,    66,    85,   193,     0,   320,    10,
     201,    25,     9,    10,    95,   124,    78,     5,    25,    10,
      82,    59,    84,    10,    25,    87,    25,    85,    25,    25,
      52,    87,   344,    82,    53,    52,    59,   161,    52,    38,
      51,   103,     9,    10,   489,   107,   108,    85,    82,    88,
      67,    52,   239,    10,   170,    52,    52,   173,    25,   175,
      51,   123,    85,   600,    51,    32,   603,   123,    25,    82,
     186,    38,     9,    10,    38,   515,    38,   268,   324,    90,
     326,   516,    49,   144,   146,    52,   448,   519,     9,   198,
     166,    35,   168,   520,    58,    52,    58,   634,   312,   323,
      67,    25,    25,    35,    25,   244,   245,   169,   170,    32,
     172,   173,     6,   175,     8,    52,   609,   380,   194,    25,
     432,   376,   377,   247,   186,   144,    32,     9,   315,   569,
      67,   557,    14,    15,   196,   197,   327,    35,   450,    38,
      34,   197,   251,    52,    38,    52,   208,   196,   210,   642,
     226,   213,   208,   540,   541,    25,   232,   602,    67,    25,
      67,    25,   524,    35,   213,    59,   593,   607,    62,    49,
      71,    72,    52,    67,   619,    49,   271,    38,    52,   241,
     296,   297,   298,   299,   300,   301,   302,   303,   304,    83,
     314,    85,   504,   439,    88,   441,   388,    73,    44,    45,
      38,    47,    78,    79,    80,   460,    51,    52,   463,   521,
     272,   273,    58,   352,     5,   324,   565,   356,   361,    51,
      52,    38,   571,   272,   273,    49,    49,    38,    52,    52,
       3,     4,    25,   295,   296,   297,   298,   299,   300,   301,
     302,   303,   304,    16,   306,     9,    49,     9,    10,    52,
      10,   332,    49,    26,   445,    52,   380,    14,    15,    51,
     384,   323,   574,    25,    51,    52,   328,   323,   349,   393,
      32,    44,    45,    52,    47,    48,   588,    51,    52,    49,
     396,   357,    52,   529,    36,    58,    59,    49,     9,    10,
      52,    58,     6,    36,     8,    44,    45,    70,    47,    13,
      35,   500,    13,    49,    25,    67,    52,    49,    25,    58,
      52,    32,   526,    35,    28,    49,   378,    38,    52,    25,
      34,    51,    52,    38,    38,   387,    51,    52,    49,   391,
     439,    52,   394,    10,   396,    49,     4,     9,   387,    51,
      52,    38,   391,    25,    12,    59,    67,    15,    62,    63,
     412,   413,   414,    67,    35,    23,   424,     9,    10,   421,
      31,    29,     6,    25,     8,     9,    34,    51,    52,    83,
      84,    85,    40,    52,    88,    89,   438,    91,     9,    10,
      74,    75,    50,    10,     9,    10,   448,    76,    77,   438,
      34,    25,    60,   509,    38,   457,    83,    34,    35,    36,
      25,    38,    25,    36,    72,    49,    35,    32,   457,    58,
     151,   152,    58,   475,    82,    59,    83,   479,    62,    87,
      35,    47,     9,    67,    49,    62,    38,    52,    25,    97,
      98,    99,    51,    10,   102,     9,    25,   499,   500,    83,
      84,    85,    67,    25,    88,    89,    83,   509,    85,    25,
     499,    88,    89,    90,   516,   123,   537,   519,   520,   127,
     128,    83,   524,   131,    25,    83,     3,     4,   136,   137,
      83,   139,   140,   141,   142,   143,   144,   145,   146,    10,
      51,    10,    51,   151,   152,    47,   554,    38,    90,    10,
       9,   159,   160,   555,     4,   163,    52,     9,    25,   561,
      25,    25,    25,   565,   572,    50,    50,    44,    45,   571,
      47,    48,   561,     9,   576,    51,    49,    52,    50,   187,
      10,    58,    59,    49,    58,    90,    35,   576,   196,   197,
       6,   593,     8,   201,    51,    83,    90,    13,    25,   607,
     208,   209,   210,     9,     9,   213,   214,   609,    52,    49,
      10,    10,    28,    83,    35,    10,    10,    49,    34,    10,
      10,   172,    38,    73,    74,    75,    76,    77,    78,    79,
      80,   537,    90,    49,   242,    85,   244,   245,     3,     4,
     642,     6,    90,    59,   394,   253,    62,    63,    95,    58,
     258,    67,   571,    82,     4,   438,     6,     4,     8,   617,
     268,    26,    12,    13,   272,   273,   555,    83,    84,    85,
     555,   644,    88,    89,    24,    91,    -1,    -1,    28,    44,
      45,    -1,    47,    48,    34,    35,    -1,    -1,    38,    -1,
      40,    -1,    42,    58,    59,    73,    74,    75,    76,    77,
      78,    79,    80,   311,   312,    70,    -1,    85,    -1,    59,
      60,    61,    62,    63,     6,   323,     8,    67,   326,   327,
     328,    -1,    -1,    -1,    -1,     6,    -1,     8,    -1,    -1,
      -1,    -1,    24,    83,    84,    85,    28,     9,    88,    89,
     348,    91,    34,    -1,   352,    -1,    38,    28,   356,    -1,
      -1,    -1,   360,    34,   362,    -1,    -1,    38,    -1,   367,
      -1,    -1,    34,    35,    36,    -1,    38,    59,    -1,    -1,
      62,    63,    -1,   381,    -1,    67,    -1,    -1,    59,   387,
       3,    62,    63,   391,    -1,    -1,    67,    -1,    -1,    -1,
      62,    83,    84,    85,    -1,    -1,    88,    89,    -1,    91,
      -1,    -1,    83,    84,    85,    -1,    -1,    88,    89,    -1,
      91,    83,    -1,    85,    -1,    -1,    88,    89,    90,    -1,
      -1,    44,    45,    -1,    47,    -1,    -1,    -1,    -1,    -1,
     438,    -1,    -1,   441,    -1,    58,    59,   445,    -1,    -1,
     448,    -1,     0,     1,    -1,    -1,     4,    -1,     6,   457,
       8,    -1,    -1,    11,    12,    13,    -1,    -1,    -1,    34,
      35,    36,    -1,    38,    -1,    -1,    24,    25,    -1,    27,
      28,    -1,    -1,    -1,    32,    33,    34,    35,    -1,    -1,
      38,    -1,    40,    41,    42,    43,    -1,    62,    -1,    -1,
      -1,   499,   500,    -1,    -1,    53,    54,    55,    56,    57,
      -1,    59,    60,    61,    62,    63,    64,    -1,    83,    67,
      85,    69,    -1,    88,    89,    90,   524,    -1,   526,    -1,
      -1,   529,    -1,    81,    82,    83,    84,    85,    86,    87,
      88,    89,    -1,    91,   542,    -1,   544,   545,    -1,     4,
      -1,     6,    -1,     8,    -1,    -1,    -1,    12,    -1,    -1,
      -1,    -1,     4,   561,     6,    -1,     8,    -1,    -1,    24,
      12,    -1,    -1,    28,    -1,    -1,    -1,    -1,   576,    34,
      35,    -1,    24,    38,    -1,    40,    28,    42,    -1,    -1,
      -1,    -1,    34,    35,    -1,    -1,    38,    -1,    40,   597,
      42,    -1,    -1,    -1,    59,    60,    61,    62,    63,    -1,
      -1,    -1,    67,    -1,    -1,    -1,    -1,    59,    60,    61,
      62,    63,    -1,     5,     6,    67,     8,    -1,    83,    84,
      85,     3,     4,    88,    89,    -1,    91,     6,    -1,     8,
      -1,    83,    84,    85,    13,    -1,    88,    89,    30,    31,
      -1,    -1,    34,    -1,    -1,    37,    38,    39,    -1,    28,
      -1,    -1,    -1,    -1,    46,    34,    -1,    -1,    -1,    38,
      -1,    -1,    44,    45,    -1,    47,     6,    59,     8,    -1,
      62,    -1,    12,    -1,    -1,    67,    58,    59,    -1,     6,
      59,     8,    -1,    62,    63,    12,    -1,    -1,    67,    -1,
      -1,    83,    84,    85,    34,    35,    88,    89,    38,    -1,
      40,    -1,    42,    -1,    83,    84,    85,    34,    35,    88,
      89,    38,    91,    40,    -1,    42,    -1,    -1,    -1,    59,
      60,    61,    62,    -1,    -1,    -1,    -1,    67,    -1,    -1,
       3,     4,    59,    60,    61,    62,    -1,     6,    -1,     8,
      67,    -1,    -1,    83,    84,    85,    -1,     3,    88,    89,
      -1,    -1,    -1,    -1,    -1,    24,    83,    84,    85,    28,
      -1,    88,    89,    -1,    -1,    34,    -1,    -1,     6,    38,
       8,    44,    45,    -1,    47,    13,    -1,     6,    -1,     8,
      -1,    -1,    -1,    -1,    -1,    58,    59,    25,    44,    45,
      59,    47,    -1,    62,    63,    -1,    34,    -1,    67,    28,
      38,    -1,    58,    59,    -1,    34,    -1,    -1,     6,    38,
       8,    -1,    10,    -1,    83,    84,    85,    -1,    -1,    88,
      89,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    67,
      59,    -1,    -1,    62,    63,    -1,    34,    -1,    67,     6,
      38,     8,     9,    -1,    -1,    83,    84,    85,    -1,    -1,
      88,    89,    -1,    -1,    83,    84,    85,    -1,    -1,    88,
      89,    59,    -1,    -1,    62,    -1,    -1,    34,    -1,    67,
       6,    38,     8,    -1,    -1,    -1,    -1,     6,    -1,     8,
      -1,    -1,    -1,    -1,    -1,    83,    84,    85,    -1,    -1,
      88,    89,    59,    -1,    -1,    62,    -1,    -1,    34,    -1,
      67,    -1,    38,    -1,    -1,    34,    -1,    -1,     6,    38,
       8,    -1,    -1,    49,    -1,    13,    83,    84,    85,    -1,
      49,    88,    89,    59,    -1,    -1,    62,    -1,    -1,    -1,
      59,    67,    -1,    62,    -1,    -1,    34,    -1,    67,     6,
      38,     8,    -1,    -1,    -1,    -1,    -1,    83,    84,    85,
      -1,    -1,    88,    89,    83,    84,    85,    -1,    -1,    88,
      89,    59,    -1,    -1,    62,    -1,    -1,    34,    -1,    67,
       6,    38,     8,     9,    -1,    -1,    -1,     6,    -1,     8,
      -1,    -1,    49,    -1,    -1,    83,    84,    85,    -1,    -1,
      88,    89,    59,    -1,    -1,    62,    -1,    -1,    34,    -1,
      67,    -1,    38,    -1,    -1,    34,    -1,    -1,    -1,    38,
      -1,    -1,    -1,     6,    -1,     8,    83,    84,    85,    -1,
      -1,    88,    89,    59,    -1,    -1,    62,    -1,    -1,    -1,
      59,    67,    -1,    62,    -1,     6,    -1,     8,    67,    10,
      -1,    34,    -1,    -1,    -1,    38,    -1,    83,    84,    85,
      -1,    -1,    88,    89,    83,    84,    85,    -1,    51,    88,
      89,    -1,    -1,    34,    -1,    -1,    59,    38,    -1,    62,
      -1,    -1,    -1,    -1,    67,    -1,    -1,    -1,    14,    15,
      51,    17,    18,    19,    20,    21,    22,    -1,    59,    -1,
      83,    62,    85,    -1,    -1,    88,    67,     3,     4,     5,
      17,    18,    19,    20,    21,    22,    12,    -1,    -1,    -1,
      16,    -1,    83,    -1,    85,    -1,    -1,    88,    -1,    -1,
      26,     3,     4,    -1,    30,    31,    -1,    -1,    -1,    35,
      -1,    37,    -1,    39,    40,    -1,    42,    -1,    44,    45,
      46,    47,    48,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,    -1,    58,    59,    60,    61,    12,    -1,    -1,    -1,
      16,    -1,    44,    45,    70,    47,    48,    -1,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    58,    59,    -1,    35,
      -1,    37,    -1,    39,    40,    -1,    42,    -1,    44,    45,
      46,    47,    48,    -1,    -1,    -1,    -1,     3,     4,     5,
      -1,    -1,    58,    59,    60,    61,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    -1,    -1,    -1,    -1,
      26,    -1,    -1,    -1,    30,    31,    -1,    -1,    -1,    35,
      -1,    37,    -1,    39,    40,    -1,    42,    -1,    44,    45,
      46,    47,    48,     3,     4,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    58,    59,    60,    61,    16,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    -1,    26,    -1,    -1,    -1,
      30,    31,    -1,     3,     4,     5,    -1,    37,    -1,    39,
      -1,    -1,    -1,    -1,    44,    45,    46,    47,    48,     3,
       4,    -1,    -1,    -1,    -1,     9,    26,    -1,    58,    59,
      30,    31,    -1,    -1,    -1,    -1,    -1,    37,    -1,    39,
      70,    25,    26,    -1,    44,    45,    46,    47,    48,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    58,    59,
      44,    45,    -1,    47,    48,    -1,     3,     4,    -1,     6,
      70,    25,    26,    10,    58,    59,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    -1,    -1,    26,
      44,    45,    -1,    47,    48,     3,     4,    -1,    -1,    -1,
       3,     4,    10,    -1,    58,    59,    -1,    44,    45,    -1,
      47,    48,    -1,    16,    -1,    -1,    70,    -1,    26,    -1,
      -1,    58,    59,    26,     3,     4,    -1,    -1,    -1,    -1,
      -1,    10,    -1,    70,    -1,    -1,    44,    45,    -1,    47,
      48,    44,    45,    -1,    47,    48,    -1,    26,    -1,    -1,
      58,    59,     3,     4,    -1,    58,    59,    -1,    -1,    10,
      -1,    -1,    70,    -1,    -1,    44,    45,    70,    47,    48,
      -1,    -1,    -1,    -1,    -1,    26,     3,     4,    -1,    58,
      59,    -1,    -1,     3,     4,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    -1,    44,    45,    -1,    47,    48,    -1,    26,
      -1,    -1,    -1,     3,     4,    -1,    26,    58,    59,    -1,
       3,     4,    -1,    -1,    -1,    -1,    -1,    44,    45,    70,
      47,    48,    -1,    50,    44,    45,    26,    47,    48,    -1,
      50,    58,    59,    -1,     3,     4,    -1,    -1,    58,    59,
       3,     4,    -1,    70,    44,    45,    -1,    47,    48,    -1,
      70,    44,    45,    -1,    47,    48,    25,    -1,    58,    59,
      -1,     3,     4,    -1,    -1,    58,    59,    -1,    -1,    -1,
      70,    -1,    -1,    -1,    67,    44,    45,    70,    47,    48,
      -1,    44,    45,    -1,    47,    48,    -1,    -1,    -1,    58,
      59,    -1,    -1,    -1,    -1,    58,    59,    -1,    -1,    -1,
      -1,    70,    44,    45,    -1,    47,    48,    70,    34,    35,
      36,    -1,    38,    -1,    -1,    -1,    58,    59,    34,    35,
      36,    -1,    38,    -1,    -1,    51,    -1,    -1,    70,    -1,
      -1,    -1,    -1,    -1,    -1,    51,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    83,    -1,    85,
      -1,    -1,    88,    89,    90,    -1,    -1,    83,    -1,    85,
      -1,    -1,    88,    89,    90
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,    71,    72,    93,    94,    85,    96,   149,     0,     1,
       4,     6,     8,    11,    12,    13,    24,    25,    27,    28,
      32,    33,    34,    35,    38,    40,    41,    42,    43,    53,
      54,    55,    56,    57,    59,    60,    61,    62,    63,    64,
      67,    69,    81,    82,    83,    84,    86,    87,    88,    89,
      91,    95,    96,   100,   110,   111,   112,   113,   115,   120,
     130,   131,   134,   138,   142,   161,     5,    82,   175,    96,
     160,    96,    59,    96,   100,    96,   100,    35,    59,    96,
     110,    25,   139,   140,    39,    88,    49,    91,   100,   111,
     129,    10,   100,   102,   104,   105,   106,    35,    35,    13,
      25,   100,    38,    59,    96,   110,   110,    13,    59,    96,
      96,   100,    96,   100,   101,    25,   140,    96,    25,    25,
      24,    28,    63,    91,   100,   110,    38,     3,     4,     5,
      16,    26,    30,    31,    37,    39,    44,    45,    46,    47,
      48,    58,    59,    70,   109,   130,     9,    10,    52,    67,
     136,    14,    15,    17,    18,    19,    20,    21,    22,   114,
     114,    35,   100,   109,   126,    25,    32,    25,    32,     6,
       8,    34,    38,    59,    62,    67,    83,    88,    96,    97,
      25,    82,    38,   173,    38,    38,     5,    16,   102,   103,
     132,   133,    96,    38,     9,    25,    35,    91,   100,   111,
     120,   125,   127,   134,   135,   161,    96,    25,    91,   109,
       9,   118,    49,    52,    10,    52,   104,    51,    49,   100,
     144,   147,    49,   144,   148,   100,     9,    25,   100,   108,
      96,    58,     9,    25,   141,   141,    96,    96,    58,    38,
      25,    38,    52,    67,    36,    36,   174,    35,    24,    28,
      63,   100,   110,   109,   103,   107,   100,   100,    13,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   130,   126,
     111,   116,   137,   137,   100,   112,   112,   113,   113,    49,
     103,   128,   100,    25,   140,    25,   140,    96,    97,    10,
      51,    97,    98,    97,    97,    38,     3,     4,    44,    45,
      47,    48,    58,    59,    70,   107,    35,   107,   107,    97,
     100,    10,     9,    49,    52,    38,   107,    25,   140,    49,
     111,   123,   124,    91,   100,   127,   109,   125,     9,    10,
      25,    52,    35,   126,    10,    25,    52,    25,    52,    25,
      52,    31,   116,   117,   111,   100,    52,    25,     6,    10,
     143,    49,    52,    25,   143,    49,    52,     9,    25,   140,
      10,    51,    52,    58,    83,    25,   140,    36,    58,    58,
      83,   107,    96,   150,   151,   100,   144,   144,    35,   133,
      35,   109,   100,    51,    52,   100,   126,    10,   136,   111,
     111,     9,    49,    52,    38,    51,    10,    51,    67,    98,
      99,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      51,    34,    35,    36,    38,    62,    83,    88,    89,    90,
      96,   152,   153,   154,   155,   157,   158,   159,   175,    51,
      51,    25,   113,   103,   107,    51,   118,    49,    52,   100,
     127,   109,    10,    25,    52,   125,   126,   117,     9,    49,
     102,   121,   122,    25,   118,   100,   102,     9,   146,    25,
     144,   146,    25,   144,   140,   100,   141,   100,    83,    25,
     100,    83,    83,    25,    51,    10,    51,   143,   143,     4,
      73,    74,    75,    76,    77,    78,    79,    80,    96,   167,
     168,   170,   171,   172,    49,   133,   100,   103,   111,   137,
       9,    10,   119,   119,   111,   103,    99,    51,    97,    10,
      51,   155,   156,   156,    51,   154,    38,    90,   153,   152,
      10,   175,   173,   173,     9,   118,     9,    51,   123,   109,
      10,    25,    52,   126,   117,   118,    49,    52,   111,   145,
     143,   143,    10,    25,     6,    10,    25,    25,    96,    25,
      50,    50,   167,   173,    49,    52,    49,   111,   116,   136,
     118,     9,    51,    49,    50,    10,    51,   156,   153,   154,
     118,    52,    49,   117,   113,   121,    10,   146,   146,   100,
     100,   100,    58,    35,   175,   168,   170,   119,   111,    51,
     155,    51,   157,    90,   175,   118,   111,    10,    50,    83,
      90,   164,   165,   166,    25,   118,    51,   154,   100,     9,
     174,   173,   174,   175,    50,   167,     9,    52,    49,    10,
      83,   164,    73,    78,    79,    80,   169,   173,    10,    35,
      74,    75,    90,   162,   163,    10,    10,   174,    76,    77,
      90,    49,    10,   167,    10,   169
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    92,    93,    93,    94,    94,    95,    95,    95,    95,
      96,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    97,    97,    97,    97,    97,    97,
      97,    97,    97,    97,    98,    98,    99,    99,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   101,   101,   102,   102,   103,   103,   104,   104,   104,
     104,   105,   105,   106,   106,   107,   107,   108,   108,   109,
     109,   109,   109,   109,   109,   110,   110,   110,   110,   111,
     111,   111,   111,   111,   111,   111,   111,   111,   111,   111,
     111,   111,   112,   112,   112,   112,   113,   113,   113,   114,
     114,   114,   114,   114,   114,   115,   115,   116,   116,   117,
     117,   118,   118,   119,   119,   120,   120,   120,   120,   120,
     121,   121,   122,   122,   123,   124,   124,   125,   125,   125,
     125,   126,   126,   126,   127,   127,   127,   127,   128,   128,
     129,   129,   130,   130,   130,   130,   131,   131,   131,   131,
     132,   132,   133,   133,   134,   134,   134,   135,   136,   136,
     137,   137,   137,   138,   138,   138,   138,   139,   139,   139,
     139,   139,   139,   139,   139,   139,   139,   139,   140,   140,
     140,   140,   140,   140,   141,   141,   141,   142,   142,   142,
      95,    95,    95,    95,    95,    95,    95,    95,   143,   143,
     144,   144,   145,   145,   146,   146,   146,    95,    95,   147,
     147,   148,   148,    95,    95,    95,    95,    95,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    95,    95,
      95,   149,    95,    95,    95,    95,    95,   150,   150,   151,
     151,    95,    95,    95,    95,    95,   152,   152,   153,   153,
     153,   153,   153,   153,   153,   153,   153,   153,   153,   153,
     153,   154,   154,   154,   155,   155,   156,   156,   157,   157,
     158,   158,   159,   159,   160,   160,   161,   161,   162,   162,
     163,   163,   164,   164,   164,   165,   165,   166,   166,   167,
     167,   167,   167,   167,   167,   167,   167,   167,   168,   169,
     169,   169,   169,   170,   170,   171,   171,   171,   171,   172,
     172,    95,   173,   174,   175
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     2,     3,     2,     0,     1,     1,     3,     3,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       2,     2,     2,     3,     3,     4,     4,     5,     3,     1,
       1,     1,     1,     1,     1,     3,     1,     0,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     2,
       3,     4,     5,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     1,     3,     1,     0,     2,     1,     1,
       0,     2,     3,     1,     2,     1,     3,     3,     5,     1,
       1,     1,     1,     1,     1,     1,     4,     2,     5,     1,
       2,     3,     1,     2,     3,     1,     2,     3,     3,     4,
       5,     1,     4,     4,     2,     1,     3,     3,     1,     1,
       1,     1,     1,     1,     1,     3,     3,     1,     3,     1,
       0,     2,     0,     2,     0,     1,     1,     1,     1,     1,
       2,     2,     1,     3,     2,     1,     3,     2,     3,     3,
       4,     1,     2,     0,     3,     4,     2,     1,     6,     4,
       2,     4,     3,     4,     2,     3,     3,     4,     2,     1,
       4,     6,     1,     0,     4,     5,     6,     3,     1,     1,
       3,     4,     0,     5,     5,     7,     3,     3,     3,     3,
       3,     4,     4,     5,     5,     3,     3,     0,     3,     3,
       4,     5,     3,     3,     1,     2,     2,     1,     1,     1,
       2,     3,     3,     2,     2,     3,     3,     2,     2,     0,
       3,     1,     1,     3,     2,     1,     0,     6,     6,     3,
       5,     3,     5,     4,     4,     5,     5,     5,     6,     2,
       4,     3,     6,     5,     4,     5,    10,     8,     5,     6,
       3,     3,     5,     2,     2,     3,     5,     3,     1,     0,
       1,     6,     3,     4,     4,     3,     2,     1,     3,     3,
       2,     3,     4,     5,     4,     1,     1,     1,     1,     1,
       1,     3,     2,     1,     3,     1,     1,     0,     3,     3,
       4,     1,     1,     0,     1,     4,     8,    10,     1,     3,
       1,     0,     6,     8,     8,     1,     4,     1,     0,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     6,     1,
       1,     1,     1,    16,     8,     3,     3,     1,     1,     1,
       0,     8,     0,     0,     0
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"<EOF>\"", "error", "$undefined", "\"+\"", "\"&\"", "\"=\"", "\"@\"",
  "\"#base\"", "\"~\"", "\":\"", "\",\"", "\"#const\"", "\"#count\"",
  "\"$\"", "\"$+\"", "\"$-\"", "\"$*\"", "\"$<=\"", "\"$<\"", "\"$>\"",
  "\"$>=\"", "\"$=\"", "\"$!=\"", "\"#cumulative\"", "\"#disjoint\"",
  "\".\"", "\"..\"", "\"#external\"", "\"#false\"", "\"#forget\"",
  "\">=\"", "\">\"", "\":-\"", "\"#include\"", "\"#inf\"", "\"{\"",
  "\"[\"", "\"<=\"", "\"(\"", "\"<\"", "\"#max\"", "\"#maximize\"",
  "\"#min\"", "\"#minimize\"", "\"\\\\\"", "\"*\"", "\"!=\"", "\"**\"",
  "\"?\"", "\"}\"", "\"]\"", "\")\"", "\";\"", "\"#show\"", "\"#edge\"",
  "\"#project\"", "\"#heuristic\"", "\"#showsig\"", "\"/\"", "\"-\"",
  "\"#sum\"", "\"#sum+\"", "\"#sup\"", "\"#true\"", "\"#program\"",
  "UBNOT", "UMINUS", "\"|\"", "\"#volatile\"", "\":~\"", "\"^\"",
  "\"<program>\"", "\"<define>\"", "\"any\"", "\"unary\"", "\"binary\"",
  "\"left\"", "\"right\"", "\"head\"", "\"body\"", "\"directive\"",
  "\"#theory\"", "\"EOF\"", "\"<NUMBER>\"", "\"<ANONYMOUS>\"",
  "\"<IDENTIFIER>\"", "\"<PYTHON>\"", "\"<LUA>\"", "\"<STRING>\"",
  "\"<VARIABLE>\"", "\"<THEORYOP>\"", "\"not\"", "$accept", "start",
  "program", "statement", "identifier", "constterm", "consttermvec",
  "constargvec", "term", "unaryargvec", "ntermvec", "termvec", "tuple",
  "tuplevec_sem", "tuplevec", "argvec", "binaryargvec", "cmp", "atom",
  "literal", "csp_mul_term", "csp_add_term", "csp_rel", "csp_literal",
  "nlitvec", "litvec", "optcondition", "noptcondition",
  "aggregatefunction", "bodyaggrelem", "bodyaggrelemvec",
  "altbodyaggrelem", "altbodyaggrelemvec", "bodyaggregate", "upper",
  "lubodyaggregate", "headaggrelemvec", "altheadaggrelemvec",
  "headaggregate", "luheadaggregate", "ncspelemvec", "cspelemvec",
  "disjoint", "conjunction", "dsym", "disjunctionsep", "disjunction",
  "bodycomma", "bodydot", "bodyconddot", "head", "optimizetuple",
  "optimizeweight", "optimizelitvec", "optimizecond", "maxelemlist",
  "minelemlist", "define", "nidlist", "idlist", "theory_op_list",
  "theory_term", "theory_opterm", "theory_opterm_nlist",
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
  const unsigned short int
  parser::yyrline_[] =
  {
       0,   337,   337,   338,   342,   343,   349,   350,   351,   352,
     356,   363,   364,   365,   366,   367,   368,   369,   370,   371,
     372,   373,   374,   375,   376,   377,   378,   379,   380,   381,
     382,   383,   384,   385,   391,   392,   396,   397,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   431,   432,   438,   439,   443,   444,   448,   449,   450,
     451,   454,   455,   458,   459,   462,   463,   467,   468,   478,
     479,   480,   481,   482,   483,   487,   488,   489,   490,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   510,   511,   512,   513,   517,   518,   519,   523,
     524,   525,   526,   527,   528,   532,   533,   541,   542,   546,
     547,   551,   552,   556,   557,   561,   562,   563,   564,   565,
     571,   572,   576,   577,   583,   587,   588,   594,   595,   596,
     597,   601,   602,   603,   607,   608,   609,   610,   616,   617,
     621,   622,   628,   629,   630,   631,   635,   636,   637,   638,
     644,   645,   649,   650,   654,   655,   656,   663,   670,   671,
     675,   676,   677,   682,   683,   684,   685,   692,   693,   694,
     695,   696,   697,   698,   699,   700,   701,   702,   706,   707,
     708,   709,   710,   711,   715,   716,   717,   720,   721,   722,
     726,   727,   728,   729,   730,   736,   737,   738,   744,   745,
     749,   750,   754,   755,   759,   760,   761,   765,   766,   770,
     771,   775,   776,   780,   781,   782,   783,   789,   790,   791,
     792,   793,   794,   795,   796,   802,   808,   809,   815,   816,
     817,   823,   827,   833,   834,   840,   841,   847,   848,   852,
     853,   857,   858,   864,   865,   866,   874,   875,   879,   880,
     881,   882,   883,   884,   885,   886,   887,   888,   889,   890,
     891,   895,   896,   897,   901,   902,   906,   907,   911,   912,
     916,   917,   921,   922,   926,   927,   930,   931,   937,   938,
     942,   943,   947,   948,   949,   953,   954,   958,   959,   963,
     964,   965,   966,   967,   968,   969,   970,   971,   975,   979,
     980,   981,   982,   986,   988,   992,   993,   994,   995,   999,
    1000,  1004,  1010,  1014,  1018
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
    *yycdebug_ << std::endl;
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    unsigned int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):" << std::endl;
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  // Symbol number corresponding to token number t.
  inline
  parser::token_number_type
  parser::yytranslate_ (int t)
  {
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
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91
    };
    const unsigned int user_token_number_max_ = 346;
    const token_number_type undef_token_ = 2;

    if (static_cast<int>(t) <= yyeof_)
      return yyeof_;
    else if (static_cast<unsigned int> (t) <= user_token_number_max_)
      return translate_table[t];
    else
      return undef_token_;
  }

#line 28 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:1167
} } } // Gringo::Input::NonGroundGrammar
#line 3692 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:1167
