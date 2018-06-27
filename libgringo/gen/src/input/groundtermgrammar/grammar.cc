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
#define yylex   GringoGroundTermGrammar_lex

// First part of user declarations.
#line 39 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:404


#include "gringo/term.hh"
#include "gringo/input/groundtermparser.hh"
#include <climits> 

using namespace Gringo;
using namespace Gringo::Input;

int GringoGroundTermGrammar_lex(void *value, void *, GroundTermParser *lexer) {
    return lexer->lex(value, lexer->logger());
}


#line 53 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:404

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

#include "grammar.hh"

// User implementation prologue.

#line 67 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:412
// Unqualified %code blocks.
#line 54 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:413


void GroundTermGrammar::parser::error(GroundTermGrammar::parser::location_type const &, std::string const &msg) {
    lexer->parseError(msg, lexer->logger());
}


#line 77 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:413


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

#line 26 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:479
namespace Gringo { namespace Input { namespace GroundTermGrammar {
#line 163 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:479

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
  parser::parser (Gringo::Input::GroundTermParser *lexer_yyarg)
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
  case 2:
#line 117 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { lexer->value = Symbol((yystack_[0].value.value)); }
#line 643 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 3:
#line 120 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::XOR, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 649 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 4:
#line 121 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::OR,  Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 655 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 5:
#line 122 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::AND, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 661 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 6:
#line 123 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::ADD, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 667 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 7:
#line 124 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::SUB, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 673 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 8:
#line 125 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::MUL, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 679 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 9:
#line 126 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::DIV, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 685 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 10:
#line 127 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::MOD, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 691 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 11:
#line 128 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(BinOp::POW, Symbol((yystack_[2].value.value)), Symbol((yystack_[0].value.value))).rep(); }
#line 697 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 12:
#line 129 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(UnOp::NEG, Symbol((yystack_[0].value.value))).rep(); }
#line 703 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 13:
#line 130 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(UnOp::NOT, Symbol((yystack_[0].value.value))).rep(); }
#line 709 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 14:
#line 131 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createTuple(Potassco::toSpan<Symbol>()).rep(); }
#line 715 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 15:
#line 132 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createTuple(Potassco::toSpan<Symbol>()).rep(); }
#line 721 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 16:
#line 133 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->tuple((yystack_[1].value.uid), false).rep(); }
#line 727 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 17:
#line 134 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->tuple((yystack_[2].value.uid), true).rep(); }
#line 733 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 18:
#line 135 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createFun((yystack_[3].value.str), Potassco::toSpan(lexer->terms((yystack_[1].value.uid)))).rep(); }
#line 739 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 19:
#line 136 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = lexer->term(UnOp::ABS, Symbol((yystack_[1].value.value))).rep(); }
#line 745 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 20:
#line 137 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createId((yystack_[0].value.str)).rep(); }
#line 751 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 21:
#line 138 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createNum((yystack_[0].value.num)).rep(); }
#line 757 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 22:
#line 139 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createStr((yystack_[0].value.str)).rep(); }
#line 763 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 23:
#line 140 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createInf().rep(); }
#line 769 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 24:
#line 141 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.value) = Symbol::createSup().rep(); }
#line 775 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 25:
#line 145 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->terms(lexer->terms(), Symbol((yystack_[0].value.value)));  }
#line 781 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 26:
#line 146 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->terms((yystack_[2].value.uid), Symbol((yystack_[0].value.value)));  }
#line 787 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 27:
#line 150 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = (yystack_[0].value.uid);  }
#line 793 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;

  case 28:
#line 151 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:859
    { (yylhs.value.uid) = lexer->terms();  }
#line 799 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
    break;


#line 803 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:859
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


  const signed char parser::yypact_ninf_ = -6;

  const signed char parser::yytable_ninf_ = -1;

  const signed char
  parser::yypact_[] =
  {
      57,    57,    32,    57,    57,    -6,    -6,    -6,     7,    -6,
       9,    76,    -6,    -5,    -6,    76,    -4,    -6,     2,    57,
      -6,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      -6,    49,    -6,    -6,    13,     8,   116,   109,    20,    20,
      20,   101,    20,   116,    89,    -6,    76,    57,    -6
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,     0,     0,     0,     0,    24,    23,    21,    20,    22,
       0,     2,    13,     0,    14,    25,     0,    12,     0,    28,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,     0,    16,    19,    27,     0,     6,     5,    10,     8,
      11,     4,     9,     7,     3,    17,    26,     0,    18
  };

  const signed char
  parser::yypgoto_[] =
  {
      -6,    -6,     0,    14,    -6
  };

  const signed char
  parser::yydefgoto_[] =
  {
      -1,    10,    15,    16,    35
  };

  const unsigned char
  parser::yytable_[] =
  {
      11,    12,    31,    17,    18,    21,    22,    30,    32,    20,
      23,    24,    25,    26,    19,    27,    28,    29,    33,    47,
      48,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      25,    46,     0,    34,     0,     0,     0,     1,    13,     2,
       0,     0,     0,     0,    14,     0,     3,    46,     4,     5,
       6,     7,     8,     9,     1,     0,     2,     0,     0,     0,
       0,    45,     1,     3,     2,     4,     5,     6,     7,     8,
       9,     3,     0,     4,     5,     6,     7,     8,     9,    21,
      22,     0,     0,     0,    23,    24,    25,    26,     0,    27,
      28,    29,    21,    22,     0,     0,     0,    23,    24,    25,
      26,     0,    27,    28,    21,    22,     0,     0,     0,    23,
      24,    25,    21,     0,    27,    28,     0,    23,    24,    25,
       0,     0,    27,    28,    23,    24,    25,     0,     0,    27
  };

  const signed char
  parser::yycheck_[] =
  {
       0,     1,     6,     3,     4,     3,     4,    12,    12,     0,
       8,     9,    10,    11,     7,    13,    14,    15,    16,     6,
      12,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      10,    31,    -1,    19,    -1,    -1,    -1,     5,     6,     7,
      -1,    -1,    -1,    -1,    12,    -1,    14,    47,    16,    17,
      18,    19,    20,    21,     5,    -1,     7,    -1,    -1,    -1,
      -1,    12,     5,    14,     7,    16,    17,    18,    19,    20,
      21,    14,    -1,    16,    17,    18,    19,    20,    21,     3,
       4,    -1,    -1,    -1,     8,     9,    10,    11,    -1,    13,
      14,    15,     3,     4,    -1,    -1,    -1,     8,     9,    10,
      11,    -1,    13,    14,     3,     4,    -1,    -1,    -1,     8,
       9,    10,     3,    -1,    13,    14,    -1,     8,     9,    10,
      -1,    -1,    13,    14,     8,     9,    10,    -1,    -1,    13
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,     5,     7,    14,    16,    17,    18,    19,    20,    21,
      25,    26,    26,     6,    12,    26,    27,    26,    26,     7,
       0,     3,     4,     8,     9,    10,    11,    13,    14,    15,
      12,     6,    12,    16,    27,    28,    26,    26,    26,    26,
      26,    26,    26,    26,    26,    12,    26,     6,    12
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    24,    25,    26,    26,    26,    26,    26,    26,    26,
      26,    26,    26,    26,    26,    26,    26,    26,    26,    26,
      26,    26,    26,    26,    26,    27,    27,    28,    28
  };

  const unsigned char
  parser::yyr2_[] =
  {
       0,     2,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     3,     3,     4,     4,     3,
       1,     1,     1,     1,     1,     1,     3,     1,     0
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"<EOF>\"", "error", "$undefined", "\"+\"", "\"&\"", "\"~\"", "\",\"",
  "\"(\"", "\"\\\\\"", "\"*\"", "\"**\"", "\"?\"", "\")\"", "\"/\"",
  "\"-\"", "\"^\"", "\"|\"", "\"#sup\"", "\"#inf\"", "\"<NUMBER>\"",
  "\"<IDENTIFIER>\"", "\"<STRING>\"", "UMINUS", "UBNOT", "$accept",
  "start", "term", "nterms", "terms", YY_NULLPTR
  };

#if YYDEBUG
  const unsigned char
  parser::yyrline_[] =
  {
       0,   117,   117,   120,   121,   122,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   139,   140,   141,   145,   146,   150,   151
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
      15,    16,    17,    18,    19,    20,    21,    22,    23
    };
    const unsigned int user_token_number_max_ = 278;
    const token_number_type undef_token_ = 2;

    if (static_cast<int>(t) <= yyeof_)
      return yyeof_;
    else if (static_cast<unsigned int> (t) <= user_token_number_max_)
      return translate_table[t];
    else
      return undef_token_;
  }

#line 26 "/home/kaminski/git/clingo/libgringo/src/input/groundtermgrammar.yy" // lalr1.cc:1167
} } } // Gringo::Input::GroundTermGrammar
#line 1259 "/mnt/scratch/kaminski/build/clingo/debug/libgringo/src/input/groundtermgrammar/grammar.cc" // lalr1.cc:1167
