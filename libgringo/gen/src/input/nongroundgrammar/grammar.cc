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
#line 351 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->parseError(yylhs.location, "syntax error, unexpected ."); }
#line 666 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 10:
#line 357 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 672 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 11:
#line 364 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 678 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 12:
#line 365 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 684 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 13:
#line 366 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 690 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 14:
#line 367 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 696 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 15:
#line 368 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 702 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 16:
#line 369 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 708 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 17:
#line 370 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 714 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 18:
#line 371 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 720 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 19:
#line 372 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 726 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 20:
#line 373 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 732 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 21:
#line 374 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 738 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 22:
#line 375 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 744 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 23:
#line 376 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 750 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 24:
#line 377 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), false); }
#line 756 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 25:
#line 378 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.termvec), true); }
#line 762 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 26:
#line 379 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 768 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 27:
#line 380 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 774 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 28:
#line 381 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.term)); }
#line 780 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 29:
#line 382 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 786 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 30:
#line 383 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 792 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 31:
#line 384 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 798 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 32:
#line 385 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 804 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 33:
#line 386 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 810 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 34:
#line 392 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term));  }
#line 816 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 35:
#line 393 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term));  }
#line 822 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 36:
#line 397 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec));  }
#line 828 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 37:
#line 398 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec();  }
#line 834 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 38:
#line 404 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 840 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 39:
#line 405 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 846 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 40:
#line 406 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 852 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 41:
#line 407 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 858 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 42:
#line 408 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 864 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 43:
#line 409 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 870 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 44:
#line 410 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 876 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 45:
#line 411 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 882 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 46:
#line 412 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 888 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 47:
#line 413 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 894 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 48:
#line 414 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 900 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 49:
#line 415 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 906 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 50:
#line 416 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.pool(yylhs.location, (yystack_[1].value.termvec)); }
#line 912 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 51:
#line 417 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 918 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 52:
#line 418 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 924 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 53:
#line 419 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.termvec)); }
#line 930 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 54:
#line 420 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 936 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 55:
#line 421 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 942 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 56:
#line 422 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 948 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 57:
#line 423 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 954 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 58:
#line 424 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 960 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 59:
#line 425 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 966 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 60:
#line 426 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String("_")); }
#line 972 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 61:
#line 432 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 978 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 62:
#line 433 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 984 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 63:
#line 439 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 990 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 64:
#line 440 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 996 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 65:
#line 444 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1002 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 66:
#line 445 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1008 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 67:
#line 449 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), true); }
#line 1014 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 68:
#line 450 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[0].value.termvec), false); }
#line 1020 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 69:
#line 451 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 1026 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 70:
#line 452 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 1032 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 71:
#line 455 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[1].value.term)); }
#line 1038 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 72:
#line 456 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[1].value.term)); }
#line 1044 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 73:
#line 459 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1050 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 74:
#line 460 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec((yystack_[1].value.termvec), (yystack_[0].value.term)); }
#line 1056 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 75:
#line 463 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec)); }
#line 1062 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 76:
#line 464 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[2].value.termvecvec), (yystack_[0].value.termvec)); }
#line 1068 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 77:
#line 468 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1074 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 78:
#line 469 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[4].value.termvecvec), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1080 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 79:
#line 479 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GT; }
#line 1086 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 80:
#line 480 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LT; }
#line 1092 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 81:
#line 481 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GEQ; }
#line 1098 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 82:
#line 482 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LEQ; }
#line 1104 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 83:
#line 483 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::EQ; }
#line 1110 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 84:
#line 484 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::NEQ; }
#line 1116 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 85:
#line 488 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1122 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 86:
#line 489 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1128 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 87:
#line 490 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1134 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 88:
#line 491 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1140 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 89:
#line 495 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1146 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 90:
#line 496 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1152 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 91:
#line 497 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1158 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 92:
#line 498 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1164 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 93:
#line 499 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1170 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 94:
#line 500 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1176 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 95:
#line 501 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::POS, (yystack_[0].value.term)); }
#line 1182 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 96:
#line 502 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOT, (yystack_[0].value.term)); }
#line 1188 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 97:
#line 503 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOTNOT, (yystack_[0].value.term)); }
#line 1194 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 98:
#line 504 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1200 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 99:
#line 505 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, neg((yystack_[1].value.rel)), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1206 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 100:
#line 506 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1212 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 101:
#line 507 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lit) = BUILDER.csplit((yystack_[0].value.csplit)); }
#line 1218 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 102:
#line 511 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term),                     (yystack_[2].value.term)); }
#line 1224 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 103:
#line 512 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[3].value.term),                     (yystack_[0].value.term)); }
#line 1230 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 104:
#line 513 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, BUILDER.term(yylhs.location, Symbol::createNum(1)), (yystack_[0].value.term)); }
#line 1236 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 105:
#line 514 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term)); }
#line 1242 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 106:
#line 518 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), true); }
#line 1248 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 107:
#line 519 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), false); }
#line 1254 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 108:
#line 520 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[0].value.cspmulterm)); }
#line 1260 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 109:
#line 524 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GT; }
#line 1266 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 110:
#line 525 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LT; }
#line 1272 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 111:
#line 526 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::GEQ; }
#line 1278 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 112:
#line 527 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::LEQ; }
#line 1284 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 113:
#line 528 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::EQ; }
#line 1290 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 114:
#line 529 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.rel) = Relation::NEQ; }
#line 1296 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 115:
#line 533 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.csplit), (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1302 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 116:
#line 534 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.cspaddterm),   (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1308 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 117:
#line 542 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(BUILDER.litvec(), (yystack_[0].value.lit)); }
#line 1314 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 118:
#line 543 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec((yystack_[2].value.litvec), (yystack_[0].value.lit)); }
#line 1320 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 119:
#line 547 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1326 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 120:
#line 548 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1332 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 121:
#line 552 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1338 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 122:
#line 553 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1344 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 123:
#line 557 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1350 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 124:
#line 558 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1356 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 125:
#line 562 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::SUM; }
#line 1362 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 126:
#line 563 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::SUMP; }
#line 1368 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 127:
#line 564 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::MIN; }
#line 1374 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 128:
#line 565 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::MAX; }
#line 1380 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 129:
#line 566 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.fun) = AggregateFunction::COUNT; }
#line 1386 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 130:
#line 572 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelem) = { BUILDER.termvec(), (yystack_[0].value.litvec) }; }
#line 1392 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 131:
#line 573 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelem) = { (yystack_[1].value.termvec), (yystack_[0].value.litvec) }; }
#line 1398 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 132:
#line 577 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1404 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 133:
#line 578 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec((yystack_[2].value.bodyaggrelemvec), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1410 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 134:
#line 584 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lbodyaggrelem) = { (yystack_[1].value.lit), (yystack_[0].value.litvec) }; }
#line 1416 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 135:
#line 588 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1422 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 136:
#line 589 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1428 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 137:
#line 595 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
#line 1434 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 138:
#line 596 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist) }; }
#line 1440 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 139:
#line 597 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.bodyaggrelemvec() }; }
#line 1446 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 140:
#line 598 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.bodyaggrelemvec) }; }
#line 1452 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 141:
#line 602 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { Relation::LEQ, (yystack_[0].value.term) }; }
#line 1458 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 142:
#line 603 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { (yystack_[1].value.rel), (yystack_[0].value.term) }; }
#line 1464 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 143:
#line 604 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.bound) = { Relation::LEQ, TermUid(-1) }; }
#line 1470 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 144:
#line 608 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1476 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 145:
#line 609 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1482 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 146:
#line 610 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1488 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 147:
#line 611 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1494 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 148:
#line 617 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec((yystack_[5].value.headaggrelemvec), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1500 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 149:
#line 618 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1506 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 150:
#line 622 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1512 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 151:
#line 623 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1518 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 152:
#line 629 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.headaggrelemvec() }; }
#line 1524 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 153:
#line 630 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.headaggrelemvec) }; }
#line 1530 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 154:
#line 631 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
#line 1536 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 155:
#line 632 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist)}; }
#line 1542 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 156:
#line 636 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1548 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 157:
#line 637 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1554 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 158:
#line 638 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1560 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 159:
#line 639 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1566 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 160:
#line 645 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(BUILDER.cspelemvec(), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1572 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 161:
#line 646 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec((yystack_[5].value.cspelemvec), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1578 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 162:
#line 650 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = (yystack_[0].value.cspelemvec); }
#line 1584 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 163:
#line 651 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(); }
#line 1590 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 164:
#line 655 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::POS, (yystack_[1].value.cspelemvec) }; }
#line 1596 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 165:
#line 656 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::NOT, (yystack_[1].value.cspelemvec) }; }
#line 1602 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 166:
#line 657 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.disjoint) = { NAF::NOTNOT, (yystack_[1].value.cspelemvec) }; }
#line 1608 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 167:
#line 664 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.lbodyaggrelem) = { (yystack_[2].value.lit), (yystack_[0].value.litvec) }; }
#line 1614 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 170:
#line 676 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1620 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 171:
#line 677 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[2].value.lit), (yystack_[1].value.litvec)); }
#line 1626 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 172:
#line 678 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(); }
#line 1632 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 173:
#line 683 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[4].value.lit), BUILDER.litvec()); }
#line 1638 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 174:
#line 684 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[4].value.lit), BUILDER.litvec()); }
#line 1644 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 175:
#line 685 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)), (yystack_[6].value.lit), (yystack_[4].value.litvec)); }
#line 1650 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 176:
#line 686 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), (yystack_[0].value.litvec)); }
#line 1656 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 177:
#line 693 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1662 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 178:
#line 694 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1668 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 179:
#line 695 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1674 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 180:
#line 696 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1680 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 181:
#line 697 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1686 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 182:
#line 698 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1692 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 183:
#line 699 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1698 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 184:
#line 700 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1704 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 185:
#line 701 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1710 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 186:
#line 702 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1716 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 187:
#line 703 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1722 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 188:
#line 707 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1728 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 189:
#line 708 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1734 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 190:
#line 709 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1740 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 191:
#line 710 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1746 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 192:
#line 711 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1752 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 193:
#line 712 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1758 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 194:
#line 716 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1764 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 195:
#line 717 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1770 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 196:
#line 718 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1776 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 197:
#line 721 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = BUILDER.headlit((yystack_[0].value.lit)); }
#line 1782 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 198:
#line 722 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = BUILDER.disjunction(yylhs.location, (yystack_[0].value.condlitlist)); }
#line 1788 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 199:
#line 723 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.head) = lexer->headaggregate(yylhs.location, (yystack_[0].value.uid)); }
#line 1794 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 200:
#line 727 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[1].value.head)); }
#line 1800 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 201:
#line 728 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head)); }
#line 1806 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 202:
#line 729 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, (yystack_[2].value.head), (yystack_[0].value.body)); }
#line 1812 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 203:
#line 730 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), (yystack_[0].value.body)); }
#line 1818 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 204:
#line 731 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), BUILDER.body()); }
#line 1824 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 205:
#line 737 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint((yystack_[0].value.body), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1830 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 206:
#line 738 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1836 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 207:
#line 739 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[1].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[1].location, inv((yystack_[1].value.disjoint).first), (yystack_[1].value.disjoint).second)); }
#line 1842 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 208:
#line 745 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1848 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 209:
#line 746 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1854 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 210:
#line 750 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termpair) = {(yystack_[2].value.term), (yystack_[0].value.term)}; }
#line 1860 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 211:
#line 751 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.termpair) = {(yystack_[0].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0))}; }
#line 1866 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 212:
#line 755 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit(BUILDER.body(), (yystack_[0].value.lit)); }
#line 1872 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 213:
#line 756 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[0].value.lit)); }
#line 1878 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 214:
#line 760 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1884 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 215:
#line 761 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1890 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 216:
#line 762 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.body) = BUILDER.body(); }
#line 1896 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 217:
#line 766 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[4].value.body)); }
#line 1902 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 218:
#line 767 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), BUILDER.body()); }
#line 1908 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 219:
#line 771 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1914 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 220:
#line 772 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1920 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 221:
#line 776 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1926 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 222:
#line 777 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1932 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 227:
#line 790 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), false); }
#line 1938 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 228:
#line 791 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true), false); }
#line 1944 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 229:
#line 792 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig("", 0, false), false); }
#line 1950 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 230:
#line 793 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), false); }
#line 1956 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 231:
#line 794 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), false); }
#line 1962 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 232:
#line 795 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), true); }
#line 1968 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 233:
#line 796 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), true); }
#line 1974 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 234:
#line 797 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), true); }
#line 1980 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 235:
#line 803 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 1986 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 236:
#line 804 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 1992 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 237:
#line 809 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.edge(yylhs.location, (yystack_[2].value.termvecvec), (yystack_[0].value.body)); }
#line 1998 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 238:
#line 815 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.heuristic(yylhs.location, (yystack_[8].value.term), (yystack_[7].value.body), (yystack_[5].value.term), (yystack_[3].value.term), (yystack_[1].value.term)); }
#line 2004 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 239:
#line 816 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.heuristic(yylhs.location, (yystack_[6].value.term), (yystack_[5].value.body), (yystack_[3].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0)), (yystack_[1].value.term)); }
#line 2010 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 240:
#line 822 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 2016 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 241:
#line 823 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 2022 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 242:
#line 824 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.project(yylhs.location, (yystack_[1].value.term), (yystack_[0].value.body)); }
#line 2028 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 243:
#line 830 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    {  BUILDER.define(yylhs.location, String::fromRep((yystack_[2].value.str)), (yystack_[0].value.term), false, LOGGER); }
#line 2034 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 244:
#line 834 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    {  BUILDER.define(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.term), true, LOGGER); }
#line 2040 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 245:
#line 840 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.python(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2046 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 246:
#line 841 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.lua(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2052 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 247:
#line 847 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->include(String::fromRep((yystack_[1].value.str)), yylhs.location, false, LOGGER); }
#line 2058 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 248:
#line 848 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->include(String::fromRep((yystack_[2].value.str)), yylhs.location, true, LOGGER); }
#line 2064 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 249:
#line 854 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec((yystack_[2].value.idlist), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2070 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 250:
#line 855 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec(BUILDER.idvec(), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2076 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 251:
#line 859 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = BUILDER.idvec(); }
#line 2082 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 252:
#line 860 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.idlist) = (yystack_[0].value.idlist); }
#line 2088 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 253:
#line 864 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[4].value.str)), (yystack_[2].value.idlist)); }
#line 2094 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 254:
#line 865 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.block(yylhs.location, String::fromRep((yystack_[1].value.str)), BUILDER.idvec()); }
#line 2100 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 255:
#line 871 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body)); }
#line 2106 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 256:
#line 872 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[2].value.term), BUILDER.body()); }
#line 2112 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 257:
#line 873 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.external(yylhs.location, (yystack_[1].value.term), BUILDER.body()); }
#line 2118 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 258:
#line 881 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[1].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2124 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 259:
#line 882 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2130 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 260:
#line 886 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermset(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2136 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 261:
#line 887 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theoryoptermlist(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2142 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 262:
#line 888 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms()); }
#line 2148 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 263:
#line 889 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermopterm(yylhs.location, (yystack_[1].value.theoryOpterm)); }
#line 2154 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 264:
#line 890 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[2].location, (yystack_[2].value.theoryOpterm))); }
#line 2160 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 265:
#line 891 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(yystack_[3].location, (yystack_[3].value.theoryOpterm), (yystack_[1].value.theoryOpterms))); }
#line 2166 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 266:
#line 892 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermfun(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.theoryOpterms)); }
#line 2172 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 267:
#line 893 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 2178 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 268:
#line 894 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 2184 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 269:
#line 895 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 2190 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 270:
#line 896 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createInf()); }
#line 2196 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 271:
#line 897 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createSup()); }
#line 2202 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 272:
#line 898 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTerm) = BUILDER.theorytermvar(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 2208 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 273:
#line 902 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[2].value.theoryOpterm), (yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2214 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 274:
#line 903 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2220 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 275:
#line 904 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm(BUILDER.theoryops(), (yystack_[0].value.theoryTerm)); }
#line 2226 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 276:
#line 908 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms((yystack_[2].value.theoryOpterms), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2232 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 277:
#line 909 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2238 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 278:
#line 913 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = (yystack_[0].value.theoryOpterms); }
#line 2244 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 279:
#line 914 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(); }
#line 2250 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 280:
#line 918 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElem) = { (yystack_[2].value.theoryOpterms), (yystack_[0].value.litvec) }; }
#line 2256 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 281:
#line 919 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElem) = { BUILDER.theoryopterms(), (yystack_[0].value.litvec) }; }
#line 2262 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 282:
#line 923 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems((yystack_[3].value.theoryElems), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2268 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 283:
#line 924 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(BUILDER.theoryelems(), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2274 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 284:
#line 928 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = (yystack_[0].value.theoryElems); }
#line 2280 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 285:
#line 929 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryElems) = BUILDER.theoryelems(); }
#line 2286 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 286:
#line 933 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), false); }
#line 2292 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 287:
#line 934 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 2298 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 288:
#line 937 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[0].value.term), BUILDER.theoryelems()); }
#line 2304 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 289:
#line 938 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[6].value.term), (yystack_[3].value.theoryElems)); }
#line 2310 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 290:
#line 939 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[8].value.term), (yystack_[5].value.theoryElems), String::fromRep((yystack_[2].value.str)), yystack_[1].location, (yystack_[1].value.theoryOpterm)); }
#line 2316 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 291:
#line 945 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2322 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 292:
#line 946 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[2].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2328 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 293:
#line 950 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = (yystack_[0].value.theoryOps); }
#line 2334 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 294:
#line 951 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOps) = BUILDER.theoryops(); }
#line 2340 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 295:
#line 955 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.num), TheoryOperatorType::Unary); }
#line 2346 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 296:
#line 956 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryLeft); }
#line 2352 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 297:
#line 957 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryRight); }
#line 2358 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 298:
#line 961 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(BUILDER.theoryopdefs(), (yystack_[0].value.theoryOpDef)); }
#line 2364 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 299:
#line 962 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs((yystack_[3].value.theoryOpDefs), (yystack_[0].value.theoryOpDef)); }
#line 2370 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 300:
#line 966 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = (yystack_[0].value.theoryOpDefs); }
#line 2376 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 301:
#line 967 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(); }
#line 2382 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 302:
#line 971 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2388 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 303:
#line 972 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("left"); }
#line 2394 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 304:
#line 973 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("right"); }
#line 2400 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 305:
#line 974 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("unary"); }
#line 2406 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 306:
#line 975 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("binary"); }
#line 2412 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 307:
#line 976 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("head"); }
#line 2418 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 308:
#line 977 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("body"); }
#line 2424 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 309:
#line 978 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("any"); }
#line 2430 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 310:
#line 979 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.str) = String::toRep("directive"); }
#line 2436 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 311:
#line 983 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryTermDef) = BUILDER.theorytermdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.theoryOpDefs), LOGGER); }
#line 2442 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 312:
#line 987 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Head; }
#line 2448 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 313:
#line 988 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Body; }
#line 2454 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 314:
#line 989 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Any; }
#line 2460 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 315:
#line 990 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomType) = TheoryAtomType::Directive; }
#line 2466 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 316:
#line 995 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[14].value.str)), (yystack_[12].value.num), String::fromRep((yystack_[10].value.str)), (yystack_[0].value.theoryAtomType), (yystack_[6].value.theoryOps), String::fromRep((yystack_[2].value.str))); }
#line 2472 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 317:
#line 996 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.num), String::fromRep((yystack_[2].value.str)), (yystack_[0].value.theoryAtomType)); }
#line 2478 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 318:
#line 1000 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryAtomDef)); }
#line 2484 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 319:
#line 1001 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryTermDef)); }
#line 2490 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 320:
#line 1002 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryAtomDef)); }
#line 2496 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 321:
#line 1003 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryTermDef)); }
#line 2502 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 322:
#line 1007 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = (yystack_[0].value.theoryDefs); }
#line 2508 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 323:
#line 1008 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(); }
#line 2514 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 324:
#line 1012 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { BUILDER.theorydef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[3].value.theoryDefs), LOGGER); }
#line 2520 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 325:
#line 1018 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Theory); }
#line 2526 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 326:
#line 1022 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Definition); }
#line 2532 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 327:
#line 1026 "/home/kaminski/git/clingo/libgringo/src/input/nongroundgrammar.yy" // lalr1.cc:859
    { lexer->theoryLexing(TheoryLexing::Disabled); }
#line 2538 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
    break;


#line 2542 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:859
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


  const short int parser::yypact_ninf_ = -463;

  const short int parser::yytable_ninf_ = -328;

  const short int
  parser::yypact_[] =
  {
     122,  -463,   -12,    87,   957,  -463,    86,    15,  -463,  -463,
     -12,   -12,  1456,   -12,  -463,  1456,   101,  -463,    24,   106,
    -463,   114,    18,  -463,  1022,  1310,  -463,   105,  -463,   126,
    1240,   144,   131,    24,    39,  1456,  -463,  -463,  -463,  -463,
     -12,  1456,   171,   -12,  -463,  -463,  -463,   236,   240,  -463,
    -463,   526,  -463,   136,  1545,  -463,    61,  -463,  1548,   478,
     259,   719,  -463,   203,  -463,   206,  -463,   858,  -463,    47,
     271,   303,   276,  1456,   314,  -463,   354,  1804,  1298,   -12,
     324,   193,   -12,   249,  -463,   352,  -463,   -12,   317,  -463,
    1132,  1703,   357,    28,  -463,  1973,   364,   325,  1310,   328,
    1342,  1351,  1456,  -463,  1751,  1456,   -12,    34,   194,   194,
     -12,   -12,   327,   302,  -463,   133,  1973,   170,   361,   370,
    -463,  -463,  -463,   368,  -463,  -463,  1206,  1733,  -463,  1456,
    1456,  1456,  -463,   381,  1456,  -463,  -463,  -463,  -463,  1456,
    1456,  -463,  1456,  1456,  1456,  1456,  1456,   367,   719,  1096,
    -463,  -463,  -463,  -463,  1385,  1385,  -463,  -463,  -463,  -463,
    -463,  -463,  1385,  1385,  1417,  1973,  1456,  -463,  -463,   386,
    -463,   418,   -12,   858,  -463,  1512,   858,  -463,   858,  -463,
    -463,   407,    57,  -463,  -463,  1456,   423,  1456,  1456,   858,
    1456,   450,   445,  -463,   180,   426,  1456,   436,  -463,   403,
     382,  1060,   756,  1605,   110,   439,   719,    44,    41,   111,
    -463,   446,  -463,  1225,  1456,  1096,  -463,  -463,  1096,  1456,
    -463,   431,  -463,   451,  1839,   475,   184,   462,   475,   192,
    1780,  -463,  -463,  1856,   153,    36,   404,   466,  -463,  -463,
     464,   449,   452,   421,  1456,  -463,   -12,  1456,  -463,  1456,
    1456,   473,  1298,   481,  -463,  -463,  1733,  -463,  1456,  -463,
     269,   323,   575,  1456,   831,   470,   470,   470,   603,   470,
     323,   693,  1973,   719,  -463,  -463,    58,  1096,  1096,  1874,
    -463,  -463,   272,   272,  -463,   505,   209,  1973,  -463,  -463,
    -463,  -463,   482,  -463,   468,  -463,    57,    49,  -463,   444,
     858,   858,   858,   858,   858,   858,   858,   858,   858,   858,
     279,   234,   298,   309,   673,  1973,  1456,  1385,  -463,  1456,
    1456,   332,  -463,  -463,   440,   504,  -463,   357,  -463,   213,
     846,  1656,    90,  1166,   719,  1096,  -463,  -463,  -463,  1278,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,   510,   520,
    -463,   357,  1973,  -463,  -463,  1456,  1456,   529,   515,  1456,
    -463,   529,   532,  1456,  -463,  -463,  -463,  1456,   194,  1456,
     474,   537,  -463,  -463,  1456,   480,   483,   541,   337,  -463,
     544,   499,  1973,   475,   475,   393,   232,  1298,  1456,  1973,
     304,  1456,  1973,  -463,  1096,  -463,   412,   412,  1096,  -463,
    1456,   858,  -463,   619,  -463,  -463,   558,   518,   435,   853,
     524,   524,   524,  1294,   524,   435,  1073,  -463,  -463,  1975,
    1975,   888,  -463,  -463,  -463,  -463,  -463,   536,  1989,  -463,
     485,   571,  -463,   534,  -463,   576,  -463,  -463,  -463,   265,
     582,   372,  -463,   567,  -463,  -463,  -463,  1096,  1656,   140,
    1166,  -463,  -463,  -463,   719,  -463,  -463,  1096,  -463,   422,
    -463,   241,  -463,  -463,  1973,   450,  1096,  -463,  -463,   475,
    -463,  -463,   475,  -463,  1973,  -463,  1903,   568,  -463,  1809,
     577,   579,  -463,   527,   -12,   580,   545,   557,   467,  -463,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,  -463,
    -463,  -463,   243,  -463,   266,  1973,  -463,  -463,  1096,  1096,
    -463,   172,   172,   357,   604,   562,  -463,    57,   858,  -463,
     571,   569,   585,  -463,    40,  1975,  -463,  -463,  1989,  1975,
     357,   573,   572,  1096,  -463,  1385,  -463,  -463,  -463,  1166,
    -463,  -463,  -463,  -463,  -463,  -463,  -463,  1449,  -463,   620,
     529,   529,  1456,  -463,  1456,  1456,  -463,  -463,  -463,  -463,
    -463,  -463,   574,   595,  -463,   393,  -463,   412,   520,  -463,
    -463,  1096,  -463,  -463,  -463,  1424,  -463,   587,  -463,   485,
    -463,  1975,   564,  -463,   265,  -463,  1096,  -463,  -463,  1973,
    1921,  1938,   581,   565,   632,  -463,  -463,   172,   357,  -463,
      80,  -463,  -463,  1975,  -463,  -463,  -463,  1456,  -463,   650,
    -463,  -463,   534,  -463,  -463,  -463,  -463,   485,  1956,   467,
     657,   614,   618,  -463,  -463,   659,   586,   565,  -463,   264,
     662,  -463,  -463,  -463,  -463,  -463,  -463,   637,   358,   583,
    -463,   665,  -463,   668,  -463,   362,   589,   631,  -463,  -463,
    -463,   674,   467,   675,   264,  -463
  };

  const unsigned short int
  parser::yydefact_[] =
  {
       0,     5,     0,     0,     0,    10,     0,     0,     1,   327,
       0,     0,     0,     0,   129,     0,     0,     7,     0,     0,
      92,   187,     0,    57,     0,    70,   128,     0,   127,     0,
       0,     0,     0,     0,     0,     0,   125,   126,    58,    89,
       0,     0,   187,     0,     6,    55,    60,     0,     0,    56,
      59,     0,     4,    54,   105,    95,   197,   108,     0,   101,
       0,   143,   199,     0,   198,     0,   159,     0,     3,     0,
     286,   288,     0,     0,    54,    49,     0,   104,   163,     0,
      85,     0,     0,     0,   204,     0,   203,     0,     0,   154,
       0,   105,   122,     0,    69,    63,    68,    73,    70,     0,
       0,     0,     0,   229,     0,     0,     0,    85,     0,     0,
       0,     0,     0,    54,    48,     0,    61,     0,     0,     0,
     326,   245,   246,     0,    93,    90,     0,     0,    96,    66,
       0,     0,    83,     0,     0,    81,    79,    82,    80,     0,
       0,    84,     0,     0,     0,     0,     0,     0,   143,     0,
     172,   168,   169,   172,     0,     0,   112,   110,   109,   111,
     113,   114,     0,     0,    66,   141,     0,   158,   207,   187,
     200,   187,     0,     0,    32,     0,     0,    33,     0,    30,
      31,    29,   243,     8,     9,    66,     0,    66,    66,     0,
       0,    65,     0,   162,     0,    87,    66,   187,   257,     0,
       0,     0,     0,   105,     0,     0,   143,     0,     0,     0,
     147,     0,   247,     0,     0,   120,   150,   155,     0,    67,
      71,    74,    50,     0,   211,   209,     0,     0,   209,     0,
       0,   187,   231,     0,     0,    87,     0,   187,   194,   242,
       0,     0,     0,     0,    66,   254,   251,     0,    53,     0,
       0,     0,   163,     0,    94,    91,     0,    97,     0,    75,
       0,    42,    41,     0,    38,    46,    44,    47,    40,    45,
      43,    39,    98,   143,   156,   117,   176,     0,     0,   105,
     106,   107,   116,   115,   152,     0,     0,   142,   206,   205,
     201,   202,     0,    21,     0,    22,    34,     0,    20,     0,
      37,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   285,     0,     0,     0,   102,     0,     0,   164,    66,
      66,     0,   256,   255,     0,     0,   137,   122,   135,     0,
       0,     0,     0,     0,   143,   120,   177,   188,   178,     0,
     146,   179,   189,   180,   193,   186,   192,   185,     0,   119,
     121,   122,    64,    72,   224,     0,     0,   216,     0,     0,
     223,   216,     0,     0,   187,   234,   230,     0,     0,     0,
       0,     0,   195,   196,     0,     0,     0,     0,     0,   250,
     252,     0,    62,   209,   209,   323,     0,   163,     0,    99,
      51,    66,   103,   157,     0,   172,   124,   124,     0,   153,
      66,    37,    23,     0,    24,    28,    36,     0,    14,    13,
      18,    16,    19,    12,    17,    15,    11,   287,   270,   279,
     279,     0,   271,   268,   269,   272,   259,   267,     0,   275,
     277,   327,   283,   284,   325,     0,    52,    51,   244,   122,
       0,     0,    86,     0,   235,   134,   138,     0,     0,     0,
       0,   181,   190,   182,   143,   144,   167,   120,   139,   122,
     132,     0,   248,   151,   210,   208,   215,   219,   226,   209,
     221,   225,   209,   233,    77,   237,     0,     0,   240,     0,
       0,     0,   227,    51,     0,     0,     0,     0,     0,   309,
     305,   306,   303,   304,   307,   308,   310,   302,   325,   321,
     320,   322,     0,   165,     0,   100,    76,   118,     0,     0,
     170,   173,   174,   122,     0,     0,    25,    35,     0,    26,
     278,     0,     0,   262,     0,   279,   258,   274,     0,     0,
     122,     0,     0,   120,   160,     0,    88,   236,   136,     0,
     183,   191,   184,   145,   130,   131,   140,     0,   212,   214,
     216,   216,     0,   241,     0,     0,   232,   228,   249,   253,
     218,   217,     0,     0,   327,     0,   166,   124,   123,   171,
     149,     0,    27,   260,   261,     0,   263,     0,   273,   276,
     280,   327,   327,   281,   122,   133,     0,   220,   222,    78,
       0,     0,     0,   301,     0,   319,   318,   175,   122,   264,
       0,   266,   282,     0,   289,   161,   213,     0,   239,     0,
     326,   298,   300,   326,   324,   148,   265,   327,     0,     0,
       0,     0,     0,   290,   238,     0,     0,     0,   311,   325,
       0,   299,   314,   312,   313,   315,   317,     0,     0,   294,
     295,     0,   291,   293,   326,     0,     0,     0,   296,   297,
     292,     0,     0,     0,     0,   316
  };

  const short int
  parser::yypgoto_[] =
  {
    -463,  -463,  -463,  -463,    -2,   -55,   508,   285,   498,  -463,
     -22,   -51,   591,  -463,  -463,  -132,  -463,   -48,    -4,    11,
     295,  -158,   633,  -463,  -142,  -301,  -306,  -346,    35,   143,
    -463,   244,  -463,  -193,  -124,  -145,  -463,  -463,    13,  -463,
    -463,  -210,   608,  -463,   -20,  -135,  -463,  -463,   -17,   -96,
    -463,  -205,   -82,  -463,  -324,  -463,  -463,  -463,  -463,  -463,
    -382,  -384,  -381,  -291,  -374,   113,  -463,  -463,  -463,   691,
    -463,  -463,    74,  -463,  -463,  -462,   137,    52,   142,  -463,
    -463,  -371,  -429,    -8
  };

  const short int
  parser::yydefgoto_[] =
  {
      -1,     3,     4,    52,    74,   296,   406,   407,    95,   117,
     191,   259,    97,    98,    99,   260,   234,   166,    55,   275,
      57,    58,   162,    59,   349,   350,   216,   511,   205,   460,
     461,   328,   329,   206,   167,   207,   286,    93,    61,    62,
     193,   194,    63,   209,   569,   277,    64,    85,    86,   239,
      65,   357,   225,   549,   467,   226,   229,     7,   380,   381,
     428,   429,   430,   520,   521,   432,   433,   434,    71,   210,
     643,   644,   611,   612,   613,   498,   499,   636,   500,   501,
     502,   186,   251,   435
  };

  const short int
  parser::yytable_[] =
  {
       6,    69,    53,    96,   282,   283,   147,   276,    70,    72,
     334,    76,   182,   240,    81,    56,    80,    83,   278,   228,
     431,   445,    53,   361,   274,   119,   562,   192,   108,   109,
     107,    80,   112,   113,   456,    92,   153,   470,   115,    60,
     524,   120,   386,   214,   527,   463,   522,   128,   528,    53,
     575,   512,   110,   310,   341,   312,   313,   332,    87,   403,
     301,   302,   531,   532,   321,   181,   344,   148,   394,   342,
     149,   150,   183,   196,     5,   320,    96,   195,   217,   258,
     199,   218,   340,    53,    79,   211,   128,     8,    53,    60,
     529,    67,   576,   236,   345,   370,   204,   343,    68,   111,
     451,   404,   303,   304,   235,   305,   306,    88,   241,   242,
       5,   151,   378,   285,   151,   452,   307,   308,   293,   335,
     336,   298,   257,   299,    53,     5,   152,   563,   309,   152,
     184,   426,   616,   534,   314,   337,   346,    78,   334,    84,
     454,   100,   528,   453,   578,   -85,   -85,    53,   579,   393,
     540,   577,   289,   545,   291,   333,   544,   625,   245,   439,
     273,   -85,   101,   338,   347,   541,    82,   383,   384,   -85,
     292,   181,   246,   181,   181,   129,   181,   504,   486,   487,
     323,   620,    60,   105,   622,   449,   -85,   181,   441,   -85,
     653,   106,     5,   542,     1,     2,   118,   528,   128,    53,
      53,   192,   197,   237,   -85,   368,   369,   570,   388,   257,
     455,    53,   327,    53,   366,   647,    53,     5,   198,   238,
     373,   597,   617,   247,   580,   151,   587,   588,   168,   351,
     318,   170,   583,   319,   358,   528,   169,   359,   248,   171,
     152,   621,   362,  -327,   379,   363,   408,   409,   410,   411,
     412,   413,   414,   415,   416,   334,   395,   454,   637,   399,
     508,   121,   400,   446,   550,   122,   447,   551,   440,   418,
     419,   420,   475,   421,   215,    53,    53,   469,   605,   154,
     155,   472,   503,   450,   600,   319,   154,   155,   396,   397,
     431,   546,   615,   564,   547,   164,   565,   422,   181,   181,
     181,   181,   181,   181,   181,   181,   181,   181,   200,   427,
     185,   -87,   -87,   -86,   -86,   187,   566,   459,   423,   319,
       5,   390,   391,   424,   425,   426,   257,   -87,    53,   -86,
     543,   417,   391,    53,   465,   -87,   192,   -86,   632,  -325,
     506,   244,   212,   633,   634,   635,   454,   473,   517,   514,
     436,   391,   -87,   188,   -86,   -87,    10,   -86,    11,   189,
      12,   437,   391,   196,    14,    15,   215,   568,   139,   140,
     -87,   142,   -86,    11,   219,    12,    16,   584,   220,    14,
     222,    20,   144,   497,   442,   391,   243,    23,   201,   483,
     391,    25,    53,    26,   263,    28,    53,   488,   249,   181,
     539,   181,    23,    24,   252,   507,    25,   250,    26,   513,
      28,   288,    35,    36,    37,    38,    39,   427,   427,   427,
      41,   509,   510,   530,   536,   391,   427,    73,    36,    37,
      38,   215,   316,   640,   641,    41,    45,    46,     5,   648,
     649,    49,    50,   290,   202,    53,   300,   301,   302,   280,
     281,    45,    46,     5,   317,    53,    49,    50,   327,   311,
     316,   322,   324,   517,    53,   320,   325,   489,   490,   491,
     492,   493,   494,   495,   496,   339,   354,   548,   348,     5,
     303,   304,   558,   305,   353,   356,   497,   360,   371,   303,
     304,   372,   305,   306,   307,   156,   157,   158,   159,   160,
     161,   374,    54,   307,   308,   377,    53,    53,   375,   385,
      75,   376,   405,    77,   398,   309,   181,   387,   142,   567,
     402,   401,    91,   427,   443,   459,   427,   427,   104,   444,
     394,    53,    11,   114,    12,   462,   -88,   -88,   466,   116,
     468,   489,   490,   491,   492,   493,   494,   495,   496,   127,
     123,   485,   -88,     5,   484,   124,   594,   471,   477,   165,
     -88,    23,   478,   497,   480,    25,   482,   481,   518,    53,
     519,   114,   305,   427,   604,   525,   426,   -88,   130,   427,
     -88,   529,   598,   203,    53,   533,    35,  -325,   127,    38,
     125,   535,   537,   553,    41,   -88,   560,   606,   224,   224,
     230,   427,   556,   233,   557,   559,   130,   131,   561,   623,
      45,    46,     5,   571,   572,    49,    50,   497,   126,   573,
     139,   140,   582,   142,   256,   172,   581,   173,   261,   262,
     586,   593,   264,   592,   144,   145,   574,   265,   266,   601,
     267,   268,   269,   270,   271,   272,   165,    91,   139,   140,
     497,   142,   279,   279,   174,   603,   610,   614,   175,   619,
     279,   279,   144,   145,   287,   609,   626,   627,   628,   629,
     630,   516,   638,   639,   642,   645,   301,   302,   646,   176,
     650,   651,   177,   297,   652,   654,   515,   178,   315,   221,
     585,   538,   163,   208,   602,    66,   130,   131,   438,    91,
     331,   631,   595,   179,   165,     5,   655,   596,   180,     0,
       0,   256,   272,    91,     0,     0,    91,   352,   303,   304,
       0,   305,   306,     0,   132,    11,     0,    12,     0,     0,
       0,     0,   307,   308,     0,     0,     0,     0,   139,   140,
       0,   142,   143,     0,   309,   382,     0,   224,   224,     0,
     135,   136,   144,   145,    23,     0,   389,   137,    25,   138,
      10,   392,    11,     0,    12,     0,   141,     0,    14,     0,
       0,   165,     0,     0,     0,    91,    91,     0,     0,    73,
     123,     0,    38,     0,     0,   124,     0,    41,     0,     0,
       0,    23,   201,     0,     0,    25,     0,    26,     0,    28,
       0,     0,     0,    45,    46,     5,     0,     0,    49,    50,
       0,     0,     0,     0,   352,   279,    35,    36,    37,    38,
     125,     0,     0,     0,    41,     0,     0,     0,   448,     0,
       0,   272,   165,    91,   130,   131,     0,     0,     0,     0,
      45,    46,     5,     0,     0,    49,    50,     0,   330,     0,
      10,     0,    11,   464,    12,     0,   301,   224,    14,     0,
       0,   224,     0,     0,   172,   474,   173,   476,     0,     0,
     253,     0,   479,     0,     0,   254,   139,   140,     0,   142,
     143,    23,   201,     0,     0,    25,   505,    26,     0,    28,
     144,   145,    91,   174,     0,     0,    91,   175,   303,   304,
       0,   305,   146,     0,     0,     0,    35,    36,    37,    38,
     255,     0,   307,   308,    41,     0,     0,     0,   176,     0,
       0,   177,     0,   418,   419,   420,   178,   421,     0,     0,
      45,    46,     5,     0,     0,    49,    50,     0,     0,     0,
     523,     0,   179,     0,     5,    91,     0,   180,   389,     0,
       0,   422,   165,     0,     0,    91,     0,    -2,     9,     0,
       0,    10,     0,    11,    91,    12,     0,     0,    13,    14,
      15,     0,   423,     0,     5,     0,     0,   424,   425,   426,
       0,    16,    17,     0,    18,    19,    20,     0,     0,     0,
      21,    22,    23,    24,     0,     0,    25,     0,    26,    27,
      28,    29,     0,     0,     0,     0,    91,    91,     0,     0,
       0,    30,    31,    32,    33,    34,     0,    35,    36,    37,
      38,    39,    40,     0,     0,    41,     0,    42,    11,     0,
      12,    91,     0,   279,     0,    15,     0,   505,     0,    43,
      44,    45,    46,     5,    47,    48,    49,    50,     0,    51,
     589,    20,   590,   591,     0,     0,     0,    23,     0,     0,
       0,    25,     0,     0,     0,     0,    11,     0,    12,    91,
       0,     0,    89,    15,     0,     0,   301,   302,     0,     0,
       0,     0,    35,     0,    91,    38,    39,     0,     0,    20,
      41,     0,     0,     0,     0,    23,     0,     0,     0,    25,
       0,     0,    11,     0,    12,   618,    45,    46,     5,    15,
     326,    49,    50,     0,    90,     0,     0,     0,   303,   304,
      35,   305,   306,    38,    39,    20,     0,     0,    41,     0,
       0,    23,   307,   308,     0,    25,     0,     0,    11,     0,
      12,     0,     0,     0,    45,    46,     5,     0,     0,    49,
      50,     0,    90,     0,     0,     0,    35,     0,     0,    38,
      39,   124,     0,     0,    41,     0,     0,    23,     0,     0,
       0,    25,    11,     0,    12,     0,     0,     0,    14,     0,
      45,    46,     5,     0,     0,    49,    50,     0,    90,     0,
       0,     0,    35,     0,     0,    38,   125,     0,     0,     0,
      41,    23,   201,     0,     0,    25,     0,    26,     0,    28,
       0,     0,    11,     0,    12,     0,    45,    46,     5,     0,
       0,    49,    50,     0,   213,     0,    73,    36,    37,    38,
     253,    11,     0,    12,    41,   254,     0,     0,     0,     0,
       0,    23,     0,     0,     0,    25,    11,     0,    12,     0,
      45,    46,     5,   102,   254,    49,    50,     0,     0,     0,
      23,     0,     0,     0,    25,   103,    35,     0,     0,    38,
     255,     0,     0,     0,    41,    23,     0,     0,     0,    25,
       0,     0,     0,     0,    11,    35,    12,   457,    38,   255,
      45,    46,     5,    41,     0,    49,    50,   301,   302,     0,
      73,     0,     0,    38,    11,     0,    12,   -66,    41,    45,
      46,     5,     0,    23,    49,    50,    11,    25,    12,     0,
      94,     0,     0,     0,    45,    46,     5,     0,   458,    49,
      50,     0,     0,    23,     0,     0,     0,    25,    73,   303,
     304,    38,   305,     0,     0,    23,    41,     0,    11,    25,
      12,     0,     0,   307,   308,     0,     0,    11,    73,    12,
       0,    38,    45,    46,     5,     0,    41,    49,    50,     0,
      73,     0,     0,    38,     0,     0,     0,    23,    41,     0,
       0,    25,    45,    46,     5,     0,    23,    49,    50,     0,
      25,    11,   223,    12,    45,    46,     5,     0,    15,    49,
      50,   227,    73,     0,     0,    38,     0,     0,     0,     0,
      41,    73,     0,     0,    38,     0,     0,     0,     0,    41,
      23,     0,     0,    11,    25,    12,    45,    46,     5,     0,
       0,    49,    50,     0,     0,    45,    46,     5,     0,     0,
      49,    50,     0,     0,     0,    73,     0,     0,    38,     0,
       0,     0,    23,    41,     0,    11,    25,    12,   457,   418,
     419,   420,    11,   421,    12,     0,     0,   284,     0,    45,
      46,     5,     0,     0,    49,    50,   599,    73,     0,     0,
      38,     0,     0,     0,    23,    41,     0,   422,    25,     0,
       0,    23,     0,     0,     0,    25,     0,     0,     0,     0,
       0,    45,    46,     5,     0,     0,    49,    50,   423,    73,
       5,     0,    38,   424,   425,   426,    73,    41,   172,    38,
     173,     0,   294,     0,    41,     0,     0,     0,     0,     0,
       0,     0,     0,    45,    46,     5,     0,     0,    49,    50,
      45,    46,     5,     0,     0,    49,    50,   174,   130,   131,
     132,   175,     0,     0,     0,     0,     0,    14,     0,     0,
       0,   133,   154,   155,   295,   156,   157,   158,   159,   160,
     161,   134,   176,     0,     0,   177,   135,   136,     0,     0,
     178,    24,     0,   137,     0,   138,    26,     0,    28,     0,
     139,   140,   141,   142,   143,     0,   179,     0,     5,     0,
       0,   180,     0,     0,   144,   145,    36,    37,   130,   131,
     132,     0,     0,     0,     0,     0,   146,    14,     0,     0,
       0,   133,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   134,     0,     0,     0,     0,   135,   136,     0,     0,
       0,   201,     0,   137,     0,   138,    26,     0,    28,     0,
     139,   140,   141,   142,   143,     0,     0,     0,     0,   130,
     131,   132,     0,     0,   144,   145,    36,    37,    14,     0,
       0,     0,     0,     0,     0,     0,   146,     0,     0,     0,
       0,     0,   134,     0,     0,     0,     0,   135,   136,     0,
       0,     0,   201,     0,   137,     0,   138,    26,     0,    28,
       0,   139,   140,   141,   142,   143,   130,   131,   132,     0,
       0,     0,     0,     0,     0,   144,   145,    36,    37,   133,
       0,     0,     0,     0,     0,     0,     0,   146,     0,   134,
       0,     0,     0,     0,   135,   136,   130,   131,   132,     0,
       0,   137,     0,   138,     0,     0,     0,     0,   139,   140,
     141,   142,   143,     0,   130,   131,     0,     0,     0,   134,
     231,     0,   144,   145,   135,   136,     0,     0,     0,     0,
       0,   137,     0,   138,   146,     0,   232,   134,   139,   140,
     141,   142,   143,   130,   131,     0,     0,     0,     0,   364,
       0,     0,   144,   145,     0,     0,   139,   140,     0,   142,
     143,     0,     0,     0,   146,   365,   134,   130,   131,     0,
     144,   145,   130,   131,     0,   554,     0,     0,     0,   555,
     190,     0,   146,     0,     0,   139,   140,     0,   142,   143,
     134,     0,     0,     0,     0,   134,     0,     0,     0,   144,
     145,     0,   130,   131,     0,   355,     0,     0,     0,   139,
     140,   146,   142,   143,   139,   140,     0,   142,   143,   130,
     131,     0,     0,   144,   145,   134,   367,     0,   144,   145,
       0,     0,     0,     0,     0,   146,     0,   130,   131,     0,
     146,     0,   134,     0,   139,   140,     0,   142,   143,     0,
     133,     0,     0,     0,     0,     0,     0,     0,   144,   145,
     134,   139,   140,     0,   142,   143,   130,   131,     0,     0,
     146,     0,     0,   552,     0,   144,   145,     0,     0,   139,
     140,     0,   142,   143,   130,   131,     0,   146,     0,   134,
       0,   607,     0,   144,   145,     0,     0,     0,     0,     0,
       0,   130,   131,     0,     0,   146,     0,   134,   139,   140,
       0,   142,   143,     0,     0,     0,     0,     0,     0,   130,
     131,     0,   144,   145,   134,     0,   139,   140,     0,   142,
     143,     0,     0,     0,   146,     0,   130,   131,     0,     0,
     144,   145,   134,   139,   140,     0,   142,   143,     0,   608,
       0,     0,   146,     0,     0,     0,     0,   144,   145,   134,
       0,   139,   140,     0,   142,   143,     0,   624,     0,   146,
     418,   419,   420,     0,   421,   144,   145,     0,   139,   140,
       0,   142,   143,     0,   418,   419,   420,   146,   421,     0,
       0,     0,   144,   145,     0,     0,     0,     0,   422,     0,
       0,     0,     0,     0,   146,     0,     0,     0,     0,     0,
       0,     0,   422,     0,     0,     0,     0,     0,     0,   423,
       0,     5,     0,     0,   424,   425,   426,     0,     0,     0,
       0,     0,     0,   423,     0,     5,     0,     0,   424,   425,
     526
  };

  const short int
  parser::yycheck_[] =
  {
       2,     9,     4,    25,   162,   163,    54,   149,    10,    11,
     203,    13,    67,   109,    18,     4,    18,    19,   153,   101,
     311,   327,    24,   228,   148,    42,   488,    78,    32,    33,
      32,    33,    34,    35,   335,    24,    56,   361,    40,     4,
     421,    43,   252,    91,   428,   351,   420,    51,   430,    51,
      10,   397,    13,   185,    10,   187,   188,   202,    40,    10,
       3,     4,   433,   434,   196,    67,    25,    54,    10,    25,
       9,    10,    25,    39,    86,    39,    98,    79,    50,   127,
      82,    53,   206,    85,    60,    87,    90,     0,    90,    54,
      10,     5,    52,    59,    53,    59,    85,    53,    83,    60,
      10,    52,    45,    46,   106,    48,    49,    89,   110,   111,
      86,    53,   244,   164,    53,    25,    59,    60,   173,     9,
      10,   176,   126,   178,   126,    86,    68,   498,    71,    68,
      83,    91,    52,   439,   189,    25,    25,    36,   331,    25,
     333,    36,   524,    53,   528,     9,    10,   149,   529,   273,
      10,   525,   169,   459,   171,   203,   457,   619,    25,   317,
     147,    25,    36,    53,    53,    25,    60,   249,   250,    33,
     172,   173,    39,   175,   176,    39,   178,   387,   383,   384,
     197,   610,   147,    39,   613,   330,    50,   189,   320,    53,
     652,    60,    86,    53,    72,    73,    25,   579,   202,   201,
     202,   252,     9,     9,    68,    52,    53,   513,   256,   213,
     334,   213,   201,   215,   231,   644,   218,    86,    25,    25,
     237,   567,   603,    53,   530,    53,   550,   551,    25,   218,
      50,    25,   533,    53,    50,   617,    33,    53,    68,    33,
      68,   612,    50,     9,   246,    53,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   448,   276,   450,   629,    50,
     395,    25,    53,    50,   469,    25,    53,   472,   319,    35,
      36,    37,   368,    39,     9,   277,   278,   359,   584,    14,
      15,   363,    50,   331,   575,    53,    14,    15,   277,   278,
     581,    50,   598,    50,    53,    36,    53,    63,   300,   301,
     302,   303,   304,   305,   306,   307,   308,   309,    59,   311,
      39,     9,    10,     9,    10,    39,    50,   339,    84,    53,
      86,    52,    53,    89,    90,    91,   330,    25,   330,    25,
     454,    52,    53,   335,   356,    33,   387,    33,    74,    36,
     391,    39,    25,    79,    80,    81,   539,   364,   403,   400,
      52,    53,    50,    39,    50,    53,     4,    53,     6,     5,
       8,    52,    53,    39,    12,    13,     9,   509,    45,    46,
      68,    48,    68,     6,    10,     8,    24,   535,    53,    12,
      52,    29,    59,   385,    52,    53,    59,    35,    36,    52,
      53,    39,   394,    41,    13,    43,   398,     4,    37,   401,
     448,   403,    35,    36,    36,   394,    39,    37,    41,   398,
      43,    25,    60,    61,    62,    63,    64,   419,   420,   421,
      68,     9,    10,   431,    52,    53,   428,    60,    61,    62,
      63,     9,    10,    75,    76,    68,    84,    85,    86,    77,
      78,    89,    90,    25,    92,   447,    39,     3,     4,   154,
     155,    84,    85,    86,     9,   457,    89,    90,   447,    36,
      10,    25,    59,   518,   466,    39,    84,    74,    75,    76,
      77,    78,    79,    80,    81,    36,    25,   466,    32,    86,
      45,    46,   484,    48,    53,    10,   488,    25,    84,    45,
      46,    25,    48,    49,    59,    17,    18,    19,    20,    21,
      22,    37,     4,    59,    60,    84,   508,   509,    59,    36,
      12,    59,    68,    15,     9,    71,   518,    36,    48,   508,
      52,    39,    24,   525,    84,   547,   528,   529,    30,    25,
      10,   533,     6,    35,     8,    25,     9,    10,     9,    41,
      25,    74,    75,    76,    77,    78,    79,    80,    81,    51,
      24,    52,    25,    86,    10,    29,   564,    25,    84,    61,
      33,    35,    25,   565,    84,    39,    25,    84,    10,   571,
      52,    73,    48,   575,   582,    39,    91,    50,     3,   581,
      53,    10,   571,    85,   586,     9,    60,    53,    90,    63,
      64,     9,    25,    25,    68,    68,    51,   586,   100,   101,
     102,   603,    25,   105,    25,    25,     3,     4,    51,   617,
      84,    85,    86,     9,    52,    89,    90,   619,    92,    50,
      45,    46,    50,    48,   126,     6,    53,     8,   130,   131,
      10,    36,   134,    59,    59,    60,    51,   139,   140,    52,
     142,   143,   144,   145,   146,   147,   148,   149,    45,    46,
     652,    48,   154,   155,    35,    91,    91,    25,    39,     9,
     162,   163,    59,    60,   166,    84,     9,    53,    50,    10,
      84,    52,    10,    36,    91,    10,     3,     4,    10,    60,
      91,    50,    63,   175,    10,    10,   401,    68,   190,    98,
     547,   447,    59,    85,   581,     4,     3,     4,    25,   201,
     202,   627,   565,    84,   206,    86,   654,   565,    89,    -1,
      -1,   213,   214,   215,    -1,    -1,   218,   219,    45,    46,
      -1,    48,    49,    -1,     5,     6,    -1,     8,    -1,    -1,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    45,    46,
      -1,    48,    49,    -1,    71,   247,    -1,   249,   250,    -1,
      31,    32,    59,    60,    35,    -1,   258,    38,    39,    40,
       4,   263,     6,    -1,     8,    -1,    47,    -1,    12,    -1,
      -1,   273,    -1,    -1,    -1,   277,   278,    -1,    -1,    60,
      24,    -1,    63,    -1,    -1,    29,    -1,    68,    -1,    -1,
      -1,    35,    36,    -1,    -1,    39,    -1,    41,    -1,    43,
      -1,    -1,    -1,    84,    85,    86,    -1,    -1,    89,    90,
      -1,    -1,    -1,    -1,   316,   317,    60,    61,    62,    63,
      64,    -1,    -1,    -1,    68,    -1,    -1,    -1,   330,    -1,
      -1,   333,   334,   335,     3,     4,    -1,    -1,    -1,    -1,
      84,    85,    86,    -1,    -1,    89,    90,    -1,    92,    -1,
       4,    -1,     6,   355,     8,    -1,     3,   359,    12,    -1,
      -1,   363,    -1,    -1,     6,   367,     8,   369,    -1,    -1,
      24,    -1,   374,    -1,    -1,    29,    45,    46,    -1,    48,
      49,    35,    36,    -1,    -1,    39,   388,    41,    -1,    43,
      59,    60,   394,    35,    -1,    -1,   398,    39,    45,    46,
      -1,    48,    71,    -1,    -1,    -1,    60,    61,    62,    63,
      64,    -1,    59,    60,    68,    -1,    -1,    -1,    60,    -1,
      -1,    63,    -1,    35,    36,    37,    68,    39,    -1,    -1,
      84,    85,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,
      52,    -1,    84,    -1,    86,   447,    -1,    89,   450,    -1,
      -1,    63,   454,    -1,    -1,   457,    -1,     0,     1,    -1,
      -1,     4,    -1,     6,   466,     8,    -1,    -1,    11,    12,
      13,    -1,    84,    -1,    86,    -1,    -1,    89,    90,    91,
      -1,    24,    25,    -1,    27,    28,    29,    -1,    -1,    -1,
      33,    34,    35,    36,    -1,    -1,    39,    -1,    41,    42,
      43,    44,    -1,    -1,    -1,    -1,   508,   509,    -1,    -1,
      -1,    54,    55,    56,    57,    58,    -1,    60,    61,    62,
      63,    64,    65,    -1,    -1,    68,    -1,    70,     6,    -1,
       8,   533,    -1,   535,    -1,    13,    -1,   539,    -1,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    -1,    92,
     552,    29,   554,   555,    -1,    -1,    -1,    35,    -1,    -1,
      -1,    39,    -1,    -1,    -1,    -1,     6,    -1,     8,   571,
      -1,    -1,    50,    13,    -1,    -1,     3,     4,    -1,    -1,
      -1,    -1,    60,    -1,   586,    63,    64,    -1,    -1,    29,
      68,    -1,    -1,    -1,    -1,    35,    -1,    -1,    -1,    39,
      -1,    -1,     6,    -1,     8,   607,    84,    85,    86,    13,
      50,    89,    90,    -1,    92,    -1,    -1,    -1,    45,    46,
      60,    48,    49,    63,    64,    29,    -1,    -1,    68,    -1,
      -1,    35,    59,    60,    -1,    39,    -1,    -1,     6,    -1,
       8,    -1,    -1,    -1,    84,    85,    86,    -1,    -1,    89,
      90,    -1,    92,    -1,    -1,    -1,    60,    -1,    -1,    63,
      64,    29,    -1,    -1,    68,    -1,    -1,    35,    -1,    -1,
      -1,    39,     6,    -1,     8,    -1,    -1,    -1,    12,    -1,
      84,    85,    86,    -1,    -1,    89,    90,    -1,    92,    -1,
      -1,    -1,    60,    -1,    -1,    63,    64,    -1,    -1,    -1,
      68,    35,    36,    -1,    -1,    39,    -1,    41,    -1,    43,
      -1,    -1,     6,    -1,     8,    -1,    84,    85,    86,    -1,
      -1,    89,    90,    -1,    92,    -1,    60,    61,    62,    63,
      24,     6,    -1,     8,    68,    29,    -1,    -1,    -1,    -1,
      -1,    35,    -1,    -1,    -1,    39,     6,    -1,     8,    -1,
      84,    85,    86,    13,    29,    89,    90,    -1,    -1,    -1,
      35,    -1,    -1,    -1,    39,    25,    60,    -1,    -1,    63,
      64,    -1,    -1,    -1,    68,    35,    -1,    -1,    -1,    39,
      -1,    -1,    -1,    -1,     6,    60,     8,     9,    63,    64,
      84,    85,    86,    68,    -1,    89,    90,     3,     4,    -1,
      60,    -1,    -1,    63,     6,    -1,     8,     9,    68,    84,
      85,    86,    -1,    35,    89,    90,     6,    39,     8,    -1,
      10,    -1,    -1,    -1,    84,    85,    86,    -1,    50,    89,
      90,    -1,    -1,    35,    -1,    -1,    -1,    39,    60,    45,
      46,    63,    48,    -1,    -1,    35,    68,    -1,     6,    39,
       8,    -1,    -1,    59,    60,    -1,    -1,     6,    60,     8,
      -1,    63,    84,    85,    86,    -1,    68,    89,    90,    -1,
      60,    -1,    -1,    63,    -1,    -1,    -1,    35,    68,    -1,
      -1,    39,    84,    85,    86,    -1,    35,    89,    90,    -1,
      39,     6,    50,     8,    84,    85,    86,    -1,    13,    89,
      90,    50,    60,    -1,    -1,    63,    -1,    -1,    -1,    -1,
      68,    60,    -1,    -1,    63,    -1,    -1,    -1,    -1,    68,
      35,    -1,    -1,     6,    39,     8,    84,    85,    86,    -1,
      -1,    89,    90,    -1,    -1,    84,    85,    86,    -1,    -1,
      89,    90,    -1,    -1,    -1,    60,    -1,    -1,    63,    -1,
      -1,    -1,    35,    68,    -1,     6,    39,     8,     9,    35,
      36,    37,     6,    39,     8,    -1,    -1,    50,    -1,    84,
      85,    86,    -1,    -1,    89,    90,    52,    60,    -1,    -1,
      63,    -1,    -1,    -1,    35,    68,    -1,    63,    39,    -1,
      -1,    35,    -1,    -1,    -1,    39,    -1,    -1,    -1,    -1,
      -1,    84,    85,    86,    -1,    -1,    89,    90,    84,    60,
      86,    -1,    63,    89,    90,    91,    60,    68,     6,    63,
       8,    -1,    10,    -1,    68,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    85,    86,    -1,    -1,    89,    90,
      84,    85,    86,    -1,    -1,    89,    90,    35,     3,     4,
       5,    39,    -1,    -1,    -1,    -1,    -1,    12,    -1,    -1,
      -1,    16,    14,    15,    52,    17,    18,    19,    20,    21,
      22,    26,    60,    -1,    -1,    63,    31,    32,    -1,    -1,
      68,    36,    -1,    38,    -1,    40,    41,    -1,    43,    -1,
      45,    46,    47,    48,    49,    -1,    84,    -1,    86,    -1,
      -1,    89,    -1,    -1,    59,    60,    61,    62,     3,     4,
       5,    -1,    -1,    -1,    -1,    -1,    71,    12,    -1,    -1,
      -1,    16,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    -1,    -1,    -1,    -1,    31,    32,    -1,    -1,
      -1,    36,    -1,    38,    -1,    40,    41,    -1,    43,    -1,
      45,    46,    47,    48,    49,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,    -1,    59,    60,    61,    62,    12,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,
      -1,    -1,    26,    -1,    -1,    -1,    -1,    31,    32,    -1,
      -1,    -1,    36,    -1,    38,    -1,    40,    41,    -1,    43,
      -1,    45,    46,    47,    48,    49,     3,     4,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    60,    61,    62,    16,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    -1,    26,
      -1,    -1,    -1,    -1,    31,    32,     3,     4,     5,    -1,
      -1,    38,    -1,    40,    -1,    -1,    -1,    -1,    45,    46,
      47,    48,    49,    -1,     3,     4,    -1,    -1,    -1,    26,
       9,    -1,    59,    60,    31,    32,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    40,    71,    -1,    25,    26,    45,    46,
      47,    48,    49,     3,     4,    -1,    -1,    -1,    -1,     9,
      -1,    -1,    59,    60,    -1,    -1,    45,    46,    -1,    48,
      49,    -1,    -1,    -1,    71,    25,    26,     3,     4,    -1,
      59,    60,     3,     4,    -1,     6,    -1,    -1,    -1,    10,
      16,    -1,    71,    -1,    -1,    45,    46,    -1,    48,    49,
      26,    -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    59,
      60,    -1,     3,     4,    -1,     6,    -1,    -1,    -1,    45,
      46,    71,    48,    49,    45,    46,    -1,    48,    49,     3,
       4,    -1,    -1,    59,    60,    26,    10,    -1,    59,    60,
      -1,    -1,    -1,    -1,    -1,    71,    -1,     3,     4,    -1,
      71,    -1,    26,    -1,    45,    46,    -1,    48,    49,    -1,
      16,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    59,    60,
      26,    45,    46,    -1,    48,    49,     3,     4,    -1,    -1,
      71,    -1,    -1,    10,    -1,    59,    60,    -1,    -1,    45,
      46,    -1,    48,    49,     3,     4,    -1,    71,    -1,    26,
      -1,    10,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,
      -1,     3,     4,    -1,    -1,    71,    -1,    26,    45,    46,
      -1,    48,    49,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,    -1,    59,    60,    26,    -1,    45,    46,    -1,    48,
      49,    -1,    -1,    -1,    71,    -1,     3,     4,    -1,    -1,
      59,    60,    26,    45,    46,    -1,    48,    49,    -1,    51,
      -1,    -1,    71,    -1,    -1,    -1,    -1,    59,    60,    26,
      -1,    45,    46,    -1,    48,    49,    -1,    51,    -1,    71,
      35,    36,    37,    -1,    39,    59,    60,    -1,    45,    46,
      -1,    48,    49,    -1,    35,    36,    37,    71,    39,    -1,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    63,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      -1,    86,    -1,    -1,    89,    90,    91,    -1,    -1,    -1,
      -1,    -1,    -1,    84,    -1,    86,    -1,    -1,    89,    90,
      91
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,    72,    73,    94,    95,    86,    97,   150,     0,     1,
       4,     6,     8,    11,    12,    13,    24,    25,    27,    28,
      29,    33,    34,    35,    36,    39,    41,    42,    43,    44,
      54,    55,    56,    57,    58,    60,    61,    62,    63,    64,
      65,    68,    70,    82,    83,    84,    85,    87,    88,    89,
      90,    92,    96,    97,   101,   111,   112,   113,   114,   116,
     121,   131,   132,   135,   139,   143,   162,     5,    83,   176,
      97,   161,    97,    60,    97,   101,    97,   101,    36,    60,
      97,   111,    60,    97,    25,   140,   141,    40,    89,    50,
      92,   101,   112,   130,    10,   101,   103,   105,   106,   107,
      36,    36,    13,    25,   101,    39,    60,    97,   111,   111,
      13,    60,    97,    97,   101,    97,   101,   102,    25,   141,
      97,    25,    25,    24,    29,    64,    92,   101,   111,    39,
       3,     4,     5,    16,    26,    31,    32,    38,    40,    45,
      46,    47,    48,    49,    59,    60,    71,   110,   131,     9,
      10,    53,    68,   137,    14,    15,    17,    18,    19,    20,
      21,    22,   115,   115,    36,   101,   110,   127,    25,    33,
      25,    33,     6,     8,    35,    39,    60,    63,    68,    84,
      89,    97,    98,    25,    83,    39,   174,    39,    39,     5,
      16,   103,   104,   133,   134,    97,    39,     9,    25,    97,
      59,    36,    92,   101,   112,   121,   126,   128,   135,   136,
     162,    97,    25,    92,   110,     9,   119,    50,    53,    10,
      53,   105,    52,    50,   101,   145,   148,    50,   145,   149,
     101,     9,    25,   101,   109,    97,    59,     9,    25,   142,
     142,    97,    97,    59,    39,    25,    39,    53,    68,    37,
      37,   175,    36,    24,    29,    64,   101,   111,   110,   104,
     108,   101,   101,    13,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   131,   127,   112,   117,   138,   138,   101,
     113,   113,   114,   114,    50,   104,   129,   101,    25,   141,
      25,   141,    97,    98,    10,    52,    98,    99,    98,    98,
      39,     3,     4,    45,    46,    48,    49,    59,    60,    71,
     108,    36,   108,   108,    98,   101,    10,     9,    50,    53,
      39,   108,    25,   141,    59,    84,    50,   112,   124,   125,
      92,   101,   128,   110,   126,     9,    10,    25,    53,    36,
     127,    10,    25,    53,    25,    53,    25,    53,    32,   117,
     118,   112,   101,    53,    25,     6,    10,   144,    50,    53,
      25,   144,    50,    53,     9,    25,   141,    10,    52,    53,
      59,    84,    25,   141,    37,    59,    59,    84,   108,    97,
     151,   152,   101,   145,   145,    36,   134,    36,   110,   101,
      52,    53,   101,   127,    10,   137,   112,   112,     9,    50,
      53,    39,    52,    10,    52,    68,    99,   100,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    52,    35,    36,
      37,    39,    63,    84,    89,    90,    91,    97,   153,   154,
     155,   156,   158,   159,   160,   176,    52,    52,    25,   114,
     104,   108,    52,    84,    25,   119,    50,    53,   101,   128,
     110,    10,    25,    53,   126,   127,   118,     9,    50,   103,
     122,   123,    25,   119,   101,   103,     9,   147,    25,   145,
     147,    25,   145,   141,   101,   142,   101,    84,    25,   101,
      84,    84,    25,    52,    10,    52,   144,   144,     4,    74,
      75,    76,    77,    78,    79,    80,    81,    97,   168,   169,
     171,   172,   173,    50,   134,   101,   104,   112,   138,     9,
      10,   120,   120,   112,   104,   100,    52,    98,    10,    52,
     156,   157,   157,    52,   155,    39,    91,   154,   153,    10,
     176,   174,   174,     9,   119,     9,    52,    25,   124,   110,
      10,    25,    53,   127,   118,   119,    50,    53,   112,   146,
     144,   144,    10,    25,     6,    10,    25,    25,    97,    25,
      51,    51,   168,   174,    50,    53,    50,   112,   117,   137,
     119,     9,    52,    50,    51,    10,    52,   157,   154,   155,
     119,    53,    50,   118,   114,   122,    10,   147,   147,   101,
     101,   101,    59,    36,   176,   169,   171,   120,   112,    52,
     156,    52,   158,    91,   176,   119,   112,    10,    51,    84,
      91,   165,   166,   167,    25,   119,    52,   155,   101,     9,
     175,   174,   175,   176,    51,   168,     9,    53,    50,    10,
      84,   165,    74,    79,    80,    81,   170,   174,    10,    36,
      75,    76,    91,   163,   164,    10,    10,   175,    77,    78,
      91,    50,    10,   168,    10,   170
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    93,    94,    94,    95,    95,    96,    96,    96,    96,
      97,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    99,    99,   100,   100,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   101,   101,   101,   101,   101,   101,   101,   101,   101,
     101,   102,   102,   103,   103,   104,   104,   105,   105,   105,
     105,   106,   106,   107,   107,   108,   108,   109,   109,   110,
     110,   110,   110,   110,   110,   111,   111,   111,   111,   112,
     112,   112,   112,   112,   112,   112,   112,   112,   112,   112,
     112,   112,   113,   113,   113,   113,   114,   114,   114,   115,
     115,   115,   115,   115,   115,   116,   116,   117,   117,   118,
     118,   119,   119,   120,   120,   121,   121,   121,   121,   121,
     122,   122,   123,   123,   124,   125,   125,   126,   126,   126,
     126,   127,   127,   127,   128,   128,   128,   128,   129,   129,
     130,   130,   131,   131,   131,   131,   132,   132,   132,   132,
     133,   133,   134,   134,   135,   135,   135,   136,   137,   137,
     138,   138,   138,   139,   139,   139,   139,   140,   140,   140,
     140,   140,   140,   140,   140,   140,   140,   140,   141,   141,
     141,   141,   141,   141,   142,   142,   142,   143,   143,   143,
      96,    96,    96,    96,    96,    96,    96,    96,   144,   144,
     145,   145,   146,   146,   147,   147,   147,    96,    96,   148,
     148,   149,   149,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,   150,    96,    96,    96,    96,    96,   151,
     151,   152,   152,    96,    96,    96,    96,    96,   153,   153,
     154,   154,   154,   154,   154,   154,   154,   154,   154,   154,
     154,   154,   154,   155,   155,   155,   156,   156,   157,   157,
     158,   158,   159,   159,   160,   160,   161,   161,   162,   162,
     162,   163,   163,   164,   164,   165,   165,   165,   166,   166,
     167,   167,   168,   168,   168,   168,   168,   168,   168,   168,
     168,   169,   170,   170,   170,   170,   171,   171,   172,   172,
     172,   172,   173,   173,    96,   174,   175,   176
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
       4,     3,     6,     5,     4,     5,     6,     5,    10,     8,
       5,     6,     3,     3,     5,     2,     2,     3,     5,     3,
       1,     0,     1,     6,     3,     4,     4,     3,     2,     1,
       3,     3,     2,     3,     4,     5,     4,     1,     1,     1,
       1,     1,     1,     3,     2,     1,     3,     1,     1,     0,
       3,     3,     4,     1,     1,     0,     1,     4,     2,     8,
      10,     1,     3,     1,     0,     6,     8,     8,     1,     4,
       1,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     6,     1,     1,     1,     1,    16,     8,     3,     3,
       1,     1,     1,     0,     8,     0,     0,     0
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
  "\".\"", "\"..\"", "\"#external\"", "\"#defined\"", "\"#false\"",
  "\"#forget\"", "\">=\"", "\">\"", "\":-\"", "\"#include\"", "\"#inf\"",
  "\"{\"", "\"[\"", "\"<=\"", "\"(\"", "\"<\"", "\"#max\"",
  "\"#maximize\"", "\"#min\"", "\"#minimize\"", "\"\\\\\"", "\"*\"",
  "\"!=\"", "\"**\"", "\"?\"", "\"}\"", "\"]\"", "\")\"", "\";\"",
  "\"#show\"", "\"#edge\"", "\"#project\"", "\"#heuristic\"",
  "\"#showsig\"", "\"/\"", "\"-\"", "\"#sum\"", "\"#sum+\"", "\"#sup\"",
  "\"#true\"", "\"#program\"", "UBNOT", "UMINUS", "\"|\"", "\"#volatile\"",
  "\":~\"", "\"^\"", "\"<program>\"", "\"<define>\"", "\"any\"",
  "\"unary\"", "\"binary\"", "\"left\"", "\"right\"", "\"head\"",
  "\"body\"", "\"directive\"", "\"#theory\"", "\"EOF\"", "\"<NUMBER>\"",
  "\"<ANONYMOUS>\"", "\"<IDENTIFIER>\"", "\"<PYTHON>\"", "\"<LUA>\"",
  "\"<STRING>\"", "\"<VARIABLE>\"", "\"<THEORYOP>\"", "\"not\"", "$accept",
  "start", "program", "statement", "identifier", "constterm",
  "consttermvec", "constargvec", "term", "unaryargvec", "ntermvec",
  "termvec", "tuple", "tuplevec_sem", "tuplevec", "argvec", "binaryargvec",
  "cmp", "atom", "literal", "csp_mul_term", "csp_add_term", "csp_rel",
  "csp_literal", "nlitvec", "litvec", "optcondition", "noptcondition",
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
       0,   338,   338,   339,   343,   344,   350,   351,   352,   353,
     357,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   392,   393,   397,   398,   404,   405,
     406,   407,   408,   409,   410,   411,   412,   413,   414,   415,
     416,   417,   418,   419,   420,   421,   422,   423,   424,   425,
     426,   432,   433,   439,   440,   444,   445,   449,   450,   451,
     452,   455,   456,   459,   460,   463,   464,   468,   469,   479,
     480,   481,   482,   483,   484,   488,   489,   490,   491,   495,
     496,   497,   498,   499,   500,   501,   502,   503,   504,   505,
     506,   507,   511,   512,   513,   514,   518,   519,   520,   524,
     525,   526,   527,   528,   529,   533,   534,   542,   543,   547,
     548,   552,   553,   557,   558,   562,   563,   564,   565,   566,
     572,   573,   577,   578,   584,   588,   589,   595,   596,   597,
     598,   602,   603,   604,   608,   609,   610,   611,   617,   618,
     622,   623,   629,   630,   631,   632,   636,   637,   638,   639,
     645,   646,   650,   651,   655,   656,   657,   664,   671,   672,
     676,   677,   678,   683,   684,   685,   686,   693,   694,   695,
     696,   697,   698,   699,   700,   701,   702,   703,   707,   708,
     709,   710,   711,   712,   716,   717,   718,   721,   722,   723,
     727,   728,   729,   730,   731,   737,   738,   739,   745,   746,
     750,   751,   755,   756,   760,   761,   762,   766,   767,   771,
     772,   776,   777,   781,   782,   783,   784,   790,   791,   792,
     793,   794,   795,   796,   797,   803,   804,   809,   815,   816,
     822,   823,   824,   830,   834,   840,   841,   847,   848,   854,
     855,   859,   860,   864,   865,   871,   872,   873,   881,   882,
     886,   887,   888,   889,   890,   891,   892,   893,   894,   895,
     896,   897,   898,   902,   903,   904,   908,   909,   913,   914,
     918,   919,   923,   924,   928,   929,   933,   934,   937,   938,
     939,   945,   946,   950,   951,   955,   956,   957,   961,   962,
     966,   967,   971,   972,   973,   974,   975,   976,   977,   978,
     979,   983,   987,   988,   989,   990,   994,   996,  1000,  1001,
    1002,  1003,  1007,  1008,  1012,  1018,  1022,  1026
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
      85,    86,    87,    88,    89,    90,    91,    92
    };
    const unsigned int user_token_number_max_ = 347;
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
#line 3721 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/nongroundgrammar/grammar.cc" // lalr1.cc:1167
