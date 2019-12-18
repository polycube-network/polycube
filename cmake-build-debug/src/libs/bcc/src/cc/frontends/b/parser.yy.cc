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


// First part of user declarations.
#line 44 "parser.yy" // lalr1.cc:404

    #include "node.h"
    #include "parser.h"
    using std::unique_ptr;
    using std::vector;
    using std::string;
    using std::move;

#line 45 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:404

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

#include "parser.yy.hh"

// User implementation prologue.

#line 59 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:412
// Unqualified %code blocks.
#line 38 "parser.yy" // lalr1.cc:413

    static int yylex(ebpf::cc::BisonParser::semantic_type *yylval,
                     ebpf::cc::BisonParser::location_type *yylloc,
                     ebpf::cc::Lexer &lexer);

#line 67 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:413


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

#line 19 "parser.yy" // lalr1.cc:479
namespace ebpf { namespace cc {
#line 153 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:479

  /// Build a parser object.
  BisonParser::BisonParser (ebpf::cc::Lexer &lexer_yyarg, ebpf::cc::Parser &parser_yyarg)
    :
#if YYDEBUG
      yydebug_ (false),
      yycdebug_ (&std::cerr),
#endif
      lexer (lexer_yyarg),
      parser (parser_yyarg)
  {}

  BisonParser::~BisonParser ()
  {}


  /*---------------.
  | Symbol types.  |
  `---------------*/

  inline
  BisonParser::syntax_error::syntax_error (const location_type& l, const std::string& m)
    : std::runtime_error (m)
    , location (l)
  {}

  // basic_symbol.
  template <typename Base>
  inline
  BisonParser::basic_symbol<Base>::basic_symbol ()
    : value ()
  {}

  template <typename Base>
  inline
  BisonParser::basic_symbol<Base>::basic_symbol (const basic_symbol& other)
    : Base (other)
    , value ()
    , location (other.location)
  {
    value = other.value;
  }


  template <typename Base>
  inline
  BisonParser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, const semantic_type& v, const location_type& l)
    : Base (t)
    , value (v)
    , location (l)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  inline
  BisonParser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, const location_type& l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  inline
  BisonParser::basic_symbol<Base>::~basic_symbol ()
  {
    clear ();
  }

  template <typename Base>
  inline
  void
  BisonParser::basic_symbol<Base>::clear ()
  {
    Base::clear ();
  }

  template <typename Base>
  inline
  bool
  BisonParser::basic_symbol<Base>::empty () const
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  inline
  void
  BisonParser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move(s);
    value = s.value;
    location = s.location;
  }

  // by_type.
  inline
  BisonParser::by_type::by_type ()
    : type (empty_symbol)
  {}

  inline
  BisonParser::by_type::by_type (const by_type& other)
    : type (other.type)
  {}

  inline
  BisonParser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  inline
  void
  BisonParser::by_type::clear ()
  {
    type = empty_symbol;
  }

  inline
  void
  BisonParser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  inline
  int
  BisonParser::by_type::type_get () const
  {
    return type;
  }


  // by_state.
  inline
  BisonParser::by_state::by_state ()
    : state (empty_state)
  {}

  inline
  BisonParser::by_state::by_state (const by_state& other)
    : state (other.state)
  {}

  inline
  void
  BisonParser::by_state::clear ()
  {
    state = empty_state;
  }

  inline
  void
  BisonParser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  inline
  BisonParser::by_state::by_state (state_type s)
    : state (s)
  {}

  inline
  BisonParser::symbol_number_type
  BisonParser::by_state::type_get () const
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[state];
  }

  inline
  BisonParser::stack_symbol_type::stack_symbol_type ()
  {}


  inline
  BisonParser::stack_symbol_type::stack_symbol_type (state_type s, symbol_type& that)
    : super_type (s, that.location)
  {
    value = that.value;
    // that is emptied.
    that.type = empty_symbol;
  }

  inline
  BisonParser::stack_symbol_type&
  BisonParser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }


  template <typename Base>
  inline
  void
  BisonParser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YYUSE (yysym.type_get ());
  }

#if YYDEBUG
  template <typename Base>
  void
  BisonParser::yy_print_ (std::ostream& yyo,
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
  BisonParser::yypush_ (const char* m, state_type s, symbol_type& sym)
  {
    stack_symbol_type t (s, sym);
    yypush_ (m, t);
  }

  inline
  void
  BisonParser::yypush_ (const char* m, stack_symbol_type& s)
  {
    if (m)
      YY_SYMBOL_PRINT (m, s);
    yystack_.push (s);
  }

  inline
  void
  BisonParser::yypop_ (unsigned int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  BisonParser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  BisonParser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  BisonParser::debug_level_type
  BisonParser::debug_level () const
  {
    return yydebug_;
  }

  void
  BisonParser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  inline BisonParser::state_type
  BisonParser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  inline bool
  BisonParser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  inline bool
  BisonParser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  BisonParser::parse ()
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
#line 133 "parser.yy" // lalr1.cc:859
    { parser.root_node_ = (yystack_[2].value.block); (yystack_[2].value.block)->scope_ = (yystack_[3].value.var_scope); }
#line 596 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 3:
#line 139 "parser.yy" // lalr1.cc:859
    { (yylhs.value.block) = new BlockStmtNode; (yylhs.value.block)->stmts_.push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 602 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 4:
#line 141 "parser.yy" // lalr1.cc:859
    { (yystack_[1].value.block)->stmts_.push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 608 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 11:
#line 162 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new BlockStmtNode; parser.add_pragma(*(yystack_[1].value.string), *(yystack_[0].value.string)); delete (yystack_[1].value.string); delete (yystack_[0].value.string); }
#line 614 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 12:
#line 164 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new BlockStmtNode; parser.add_pragma(*(yystack_[1].value.string), *(yystack_[0].value.string)); delete (yystack_[1].value.string); delete (yystack_[0].value.string); }
#line 620 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 13:
#line 169 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmts) = new StmtNodeList; (yylhs.value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 626 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 14:
#line 171 "parser.yy" // lalr1.cc:859
    { (yystack_[1].value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 632 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 15:
#line 176 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new ExprStmtNode(ExprNode::Ptr((yystack_[1].value.expr)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 639 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 16:
#line 179 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new ExprStmtNode(ExprNode::Ptr((yystack_[1].value.expr)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 646 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 17:
#line 182 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new ExprStmtNode(ExprNode::Ptr((yystack_[1].value.expr)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 653 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 18:
#line 185 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new ExprStmtNode(ExprNode::Ptr((yystack_[6].value.call)));
      (yystack_[6].value.call)->block_->stmts_ = move(*(yystack_[3].value.stmts)); delete (yystack_[3].value.stmts);
      (yystack_[6].value.call)->block_->scope_ = (yystack_[4].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 662 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 19:
#line 190 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new ExprStmtNode(ExprNode::Ptr((yystack_[3].value.call)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 669 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 22:
#line 195 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = (yystack_[1].value.stmt); }
#line 675 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 25:
#line 202 "parser.yy" // lalr1.cc:859
    { (yylhs.value.call) = new MethodCallExprNode(IdentExprNode::Ptr((yystack_[3].value.ident)), move(*(yystack_[1].value.args)), lexer.lineno()); delete (yystack_[1].value.args);
      parser.set_loc((yylhs.value.call), yylhs.location); }
#line 682 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 26:
#line 208 "parser.yy" // lalr1.cc:859
    { (yylhs.value.block) = new BlockStmtNode; (yylhs.value.block)->stmts_ = move(*(yystack_[1].value.stmts)); delete (yystack_[1].value.stmts);
      parser.set_loc((yylhs.value.block), yylhs.location); }
#line 689 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 27:
#line 211 "parser.yy" // lalr1.cc:859
    { (yylhs.value.block) = new BlockStmtNode;
      parser.set_loc((yylhs.value.block), yylhs.location); }
#line 696 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 28:
#line 215 "parser.yy" // lalr1.cc:859
    { (yylhs.value.var_scope) = parser.scopes_->enter_var_scope(); }
#line 702 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 29:
#line 216 "parser.yy" // lalr1.cc:859
    { (yylhs.value.var_scope) = parser.scopes_->exit_var_scope(); }
#line 708 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 30:
#line 217 "parser.yy" // lalr1.cc:859
    { (yylhs.value.state_scope) = parser.scopes_->enter_state_scope(); }
#line 714 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 31:
#line 218 "parser.yy" // lalr1.cc:859
    { (yylhs.value.state_scope) = parser.scopes_->exit_state_scope(); }
#line 720 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 32:
#line 222 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.struct_add((yystack_[3].value.ident), (yystack_[1].value.formals)); delete (yystack_[1].value.formals);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 727 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 33:
#line 228 "parser.yy" // lalr1.cc:859
    { (yylhs.value.formals) = new FormalList; (yylhs.value.formals)->push_back(VariableDeclStmtNode::Ptr((yystack_[1].value.decl))); }
#line 733 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 34:
#line 230 "parser.yy" // lalr1.cc:859
    { (yystack_[3].value.formals)->push_back(VariableDeclStmtNode::Ptr((yystack_[1].value.decl))); }
#line 739 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 35:
#line 235 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.table_add((yystack_[7].value.ident), (yystack_[5].value.ident_args), (yystack_[3].value.ident), (yystack_[1].value.string)); delete (yystack_[5].value.ident_args);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 746 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 36:
#line 241 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident_args) = new IdentExprNodeList; (yylhs.value.ident_args)->push_back(IdentExprNode::Ptr((yystack_[0].value.ident))); }
#line 752 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 37:
#line 243 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident_args)->push_back(IdentExprNode::Ptr((yystack_[0].value.ident))); }
#line 758 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 38:
#line 248 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.state_add((yystack_[4].value.state_scope), (yystack_[5].value.ident), (yystack_[2].value.block)); (yystack_[2].value.block)->scope_ = (yystack_[3].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 766 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 39:
#line 252 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.state_add((yystack_[4].value.state_scope), (yystack_[7].value.ident), new IdentExprNode(""), (yystack_[2].value.block)); (yystack_[2].value.block)->scope_ = (yystack_[3].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 774 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 40:
#line 256 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.state_add((yystack_[4].value.state_scope), (yystack_[7].value.ident), (yystack_[5].value.ident), (yystack_[2].value.block)); (yystack_[2].value.block)->scope_ = (yystack_[3].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 782 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 41:
#line 263 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.func_add((yystack_[9].value.type_specifiers), (yystack_[7].value.state_scope), (yystack_[8].value.ident), (yystack_[4].value.formals), (yystack_[2].value.block)); (yystack_[2].value.block)->scope_ = (yystack_[6].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 790 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 42:
#line 270 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmts) = new StmtNodeList; (yylhs.value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 796 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 43:
#line 272 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 802 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 44:
#line 277 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.result_add((yystack_[8].value.token), (yystack_[7].value.ident), (yystack_[4].value.formals), (yystack_[2].value.block)); delete (yystack_[4].value.formals); (yystack_[2].value.block)->scope_ = (yystack_[6].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 810 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 45:
#line 281 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.result_add((yystack_[7].value.token), (yystack_[6].value.ident), new FormalList, (yystack_[2].value.block)); (yystack_[2].value.block)->scope_ = (yystack_[5].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 818 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 46:
#line 285 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.result_add((yystack_[8].value.token), (yystack_[7].value.ident), (yystack_[4].value.formals), (yystack_[2].value.block)); delete (yystack_[4].value.formals); (yystack_[2].value.block)->scope_ = (yystack_[6].value.var_scope);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 826 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 47:
#line 292 "parser.yy" // lalr1.cc:859
    { (yylhs.value.formals) = new FormalList; (yylhs.value.formals)->push_back(VariableDeclStmtNode::Ptr(parser.variable_add(nullptr, (yystack_[0].value.type_decl)))); }
#line 832 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 48:
#line 294 "parser.yy" // lalr1.cc:859
    { (yystack_[3].value.formals)->push_back(VariableDeclStmtNode::Ptr(parser.variable_add(nullptr, (yystack_[0].value.type_decl)))); }
#line 838 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 53:
#line 305 "parser.yy" // lalr1.cc:859
    { (yylhs.value.type_specifiers) = new std::vector<int>; (yylhs.value.type_specifiers)->push_back((yystack_[0].value.token)); }
#line 844 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 54:
#line 306 "parser.yy" // lalr1.cc:859
    { (yylhs.value.type_specifiers)->push_back((yystack_[0].value.token)); }
#line 850 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 55:
#line 311 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.variable_add((yystack_[1].value.type_specifiers), (yystack_[0].value.decl));
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 858 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 56:
#line 315 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.variable_add((yystack_[3].value.type_specifiers), (yystack_[2].value.decl), (yystack_[0].value.expr));
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 866 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 57:
#line 319 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.variable_add((yystack_[4].value.type_decl), (yystack_[1].value.args), true);
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 874 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 58:
#line 326 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = parser.variable_add(nullptr, (yystack_[0].value.decl));
      if (!(yylhs.value.stmt)) YYERROR;
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 882 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 59:
#line 332 "parser.yy" // lalr1.cc:859
    { (yylhs.value.decl) = (yystack_[0].value.decl); }
#line 888 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 60:
#line 332 "parser.yy" // lalr1.cc:859
    { (yylhs.value.decl) = (yystack_[0].value.type_decl); }
#line 894 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 61:
#line 334 "parser.yy" // lalr1.cc:859
    { (yylhs.value.decl) = new IntegerVariableDeclStmtNode(IdentExprNode::Ptr((yystack_[2].value.ident)), *(yystack_[0].value.string)); delete (yystack_[0].value.string);
      parser.set_loc((yylhs.value.decl), yylhs.location); }
#line 901 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 62:
#line 339 "parser.yy" // lalr1.cc:859
    { (yylhs.value.type_decl) = new StructVariableDeclStmtNode(IdentExprNode::Ptr((yystack_[1].value.ident)), IdentExprNode::Ptr((yystack_[0].value.ident)));
      parser.set_loc((yylhs.value.type_decl), yylhs.location); }
#line 908 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 63:
#line 344 "parser.yy" // lalr1.cc:859
    { (yylhs.value.decl) = (yystack_[0].value.type_decl); }
#line 914 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 64:
#line 346 "parser.yy" // lalr1.cc:859
    { (yylhs.value.type_decl) = new StructVariableDeclStmtNode(IdentExprNode::Ptr((yystack_[2].value.ident)), IdentExprNode::Ptr((yystack_[0].value.ident)),
                                          VariableDeclStmtNode::STRUCT_REFERENCE);
      parser.set_loc((yylhs.value.type_decl), yylhs.location); }
#line 922 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 65:
#line 359 "parser.yy" // lalr1.cc:859
    { (yylhs.value.args) = new ExprNodeList; (yylhs.value.args)->push_back(ExprNode::Ptr((yystack_[0].value.expr))); }
#line 928 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 66:
#line 360 "parser.yy" // lalr1.cc:859
    { (yylhs.value.args)->push_back(ExprNode::Ptr((yystack_[0].value.expr))); }
#line 934 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 67:
#line 364 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new AssignExprNode(IdentExprNode::Ptr((yystack_[2].value.ident)), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 941 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 68:
#line 367 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new AssignExprNode(IdentExprNode::Ptr((yystack_[3].value.ident)), ExprNode::Ptr((yystack_[0].value.expr))); (yylhs.value.expr)->bitop_ = BitopExprNode::Ptr((yystack_[2].value.bitop));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 948 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 69:
#line 373 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new IfStmtNode(ExprNode::Ptr((yystack_[3].value.expr)), StmtNode::Ptr((yystack_[1].value.block)));
      (yystack_[1].value.block)->scope_ = (yystack_[2].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 956 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 70:
#line 377 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new IfStmtNode(ExprNode::Ptr((yystack_[7].value.expr)), StmtNode::Ptr((yystack_[5].value.block)), StmtNode::Ptr((yystack_[1].value.block)));
      (yystack_[5].value.block)->scope_ = (yystack_[6].value.var_scope); (yystack_[1].value.block)->scope_ = (yystack_[2].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 964 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 71:
#line 381 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new IfStmtNode(ExprNode::Ptr((yystack_[5].value.expr)), StmtNode::Ptr((yystack_[3].value.block)), StmtNode::Ptr((yystack_[0].value.stmt)));
      (yystack_[3].value.block)->scope_ = (yystack_[4].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 972 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 72:
#line 388 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new OnValidStmtNode(IdentExprNode::Ptr((yystack_[4].value.ident)), StmtNode::Ptr((yystack_[1].value.block)));
      (yystack_[1].value.block)->scope_ = (yystack_[2].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 980 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 73:
#line 392 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new OnValidStmtNode(IdentExprNode::Ptr((yystack_[8].value.ident)), StmtNode::Ptr((yystack_[5].value.block)), StmtNode::Ptr((yystack_[1].value.block)));
      (yystack_[5].value.block)->scope_ = (yystack_[6].value.var_scope); (yystack_[1].value.block)->scope_ = (yystack_[2].value.var_scope);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 988 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 74:
#line 399 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new SwitchStmtNode(ExprNode::Ptr((yystack_[3].value.expr)), make_unique<BlockStmtNode>(move(*(yystack_[1].value.stmts)))); delete (yystack_[1].value.stmts);
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 995 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 75:
#line 405 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmts) = new StmtNodeList; (yylhs.value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 1001 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 76:
#line 407 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmts)->push_back(StmtNode::Ptr((yystack_[0].value.stmt))); }
#line 1007 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 77:
#line 412 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new CaseStmtNode(IntegerExprNode::Ptr((yystack_[2].value.numeric)), BlockStmtNode::Ptr((yystack_[1].value.block)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 1014 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 78:
#line 415 "parser.yy" // lalr1.cc:859
    { (yylhs.value.stmt) = new CaseStmtNode(BlockStmtNode::Ptr((yystack_[1].value.block)));
      parser.set_loc((yylhs.value.stmt), yylhs.location); }
#line 1021 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 79:
#line 421 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode((yystack_[0].value.string));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1028 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 80:
#line 424 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode((yystack_[0].value.string));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1035 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 81:
#line 427 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode((yystack_[2].value.string), (yystack_[0].value.string));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1042 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 82:
#line 430 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode((yystack_[2].value.string), (yystack_[0].value.string));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1049 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 83:
#line 433 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode(new string("1"), new string("1"));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1056 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 84:
#line 436 "parser.yy" // lalr1.cc:859
    { (yylhs.value.numeric) = new IntegerExprNode(new string("0"), new string("1"));
      parser.set_loc((yylhs.value.numeric), yylhs.location); }
#line 1063 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 85:
#line 442 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new AssignExprNode(ExprNode::Ptr((yystack_[2].value.expr)), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1070 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 86:
#line 456 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new ReturnExprNode(ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1077 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 87:
#line 462 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[0].value.call); }
#line 1083 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 88:
#line 464 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[1].value.call); (yylhs.value.expr)->bitop_ = BitopExprNode::Ptr((yystack_[0].value.bitop)); }
#line 1089 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 89:
#line 466 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[0].value.table_index); }
#line 1095 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 90:
#line 468 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[2].value.table_index); (yystack_[2].value.table_index)->sub_ = IdentExprNode::Ptr((yystack_[0].value.ident)); }
#line 1101 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 91:
#line 470 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[0].value.ident); }
#line 1107 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 92:
#line 472 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new PacketExprNode(IdentExprNode::Ptr((yystack_[0].value.ident)));
      (yylhs.value.expr)->flags_[ExprNode::IS_REF] = true;
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1115 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 93:
#line 476 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new PacketExprNode(IdentExprNode::Ptr((yystack_[0].value.ident)));
      (yylhs.value.expr)->flags_[ExprNode::IS_PKT] = true;
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1123 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 94:
#line 480 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new PacketExprNode(IdentExprNode::Ptr((yystack_[1].value.ident))); (yylhs.value.expr)->bitop_ = BitopExprNode::Ptr((yystack_[0].value.bitop));
      (yylhs.value.expr)->flags_[ExprNode::IS_PKT] = true;
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1131 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 95:
#line 484 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new GotoExprNode(IdentExprNode::Ptr((yystack_[0].value.ident)), false);
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1138 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 96:
#line 487 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new GotoExprNode(IdentExprNode::Ptr((yystack_[0].value.ident)), false);
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1145 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 97:
#line 490 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new GotoExprNode(IdentExprNode::Ptr((yystack_[0].value.ident)), true);
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1152 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 98:
#line 493 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[1].value.expr); }
#line 1158 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 99:
#line 495 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[2].value.expr); (yylhs.value.expr)->bitop_ = BitopExprNode::Ptr((yystack_[0].value.bitop)); }
#line 1164 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 100:
#line 497 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new StringExprNode((yystack_[0].value.string));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1171 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 101:
#line 500 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[0].value.numeric); }
#line 1177 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 102:
#line 502 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = (yystack_[1].value.numeric); (yylhs.value.expr)->bitop_ = BitopExprNode::Ptr((yystack_[0].value.bitop)); }
#line 1183 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 103:
#line 504 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1190 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 104:
#line 507 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1197 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 105:
#line 510 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1204 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 106:
#line 513 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1211 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 107:
#line 516 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1218 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 108:
#line 519 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1225 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 109:
#line 522 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1232 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 110:
#line 525 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1239 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 111:
#line 528 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1246 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 112:
#line 531 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1253 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 113:
#line 534 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1260 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 114:
#line 537 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1267 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 115:
#line 540 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1274 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 116:
#line 543 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1281 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 117:
#line 546 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1288 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 118:
#line 549 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new BinopExprNode(ExprNode::Ptr((yystack_[2].value.expr)), (yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1295 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 119:
#line 554 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new UnopExprNode((yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1302 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 120:
#line 557 "parser.yy" // lalr1.cc:859
    { (yylhs.value.expr) = new UnopExprNode((yystack_[1].value.token), ExprNode::Ptr((yystack_[0].value.expr)));
      parser.set_loc((yylhs.value.expr), yylhs.location); }
#line 1309 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 121:
#line 563 "parser.yy" // lalr1.cc:859
    { (yylhs.value.args) = new ExprNodeList; }
#line 1315 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 122:
#line 565 "parser.yy" // lalr1.cc:859
    { (yylhs.value.args) = new ExprNodeList; (yylhs.value.args)->push_back(ExprNode::Ptr((yystack_[0].value.expr))); }
#line 1321 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 123:
#line 567 "parser.yy" // lalr1.cc:859
    { (yylhs.value.args)->push_back(ExprNode::Ptr((yystack_[0].value.expr))); }
#line 1327 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 124:
#line 572 "parser.yy" // lalr1.cc:859
    { (yylhs.value.bitop) = new BitopExprNode(string("0"), *(yystack_[1].value.string)); delete (yystack_[1].value.string);
      parser.set_loc((yylhs.value.bitop), yylhs.location); }
#line 1334 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 125:
#line 575 "parser.yy" // lalr1.cc:859
    { (yylhs.value.bitop) = new BitopExprNode(*(yystack_[4].value.string), *(yystack_[1].value.string)); delete (yystack_[4].value.string); delete (yystack_[1].value.string);
      parser.set_loc((yylhs.value.bitop), yylhs.location); }
#line 1341 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 126:
#line 581 "parser.yy" // lalr1.cc:859
    { (yylhs.value.table_index) = new TableIndexExprNode(IdentExprNode::Ptr((yystack_[3].value.ident)), IdentExprNode::Ptr((yystack_[1].value.ident)));
      parser.set_loc((yylhs.value.table_index), yylhs.location); }
#line 1348 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 127:
#line 587 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident) = (yystack_[0].value.ident); }
#line 1354 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 128:
#line 589 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident)->append_scope(*(yystack_[0].value.string)); delete (yystack_[0].value.string); }
#line 1360 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 129:
#line 594 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident) = (yystack_[0].value.ident); }
#line 1366 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 130:
#line 596 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident)->append_dot(*(yystack_[0].value.string)); delete (yystack_[0].value.string); }
#line 1372 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 131:
#line 601 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident) = (yystack_[0].value.ident); }
#line 1378 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 132:
#line 603 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident)->append_dot(*(yystack_[0].value.string)); delete (yystack_[0].value.string); }
#line 1384 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 133:
#line 605 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident)->append_dot(*(yystack_[0].value.string)); delete (yystack_[0].value.string); }
#line 1390 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 134:
#line 607 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident)->append_scope(*(yystack_[0].value.string)); delete (yystack_[0].value.string); }
#line 1396 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;

  case 135:
#line 612 "parser.yy" // lalr1.cc:859
    { (yylhs.value.ident) = new IdentExprNode(*(yystack_[0].value.string)); delete (yystack_[0].value.string);
      parser.set_loc((yylhs.value.ident), yylhs.location); }
#line 1403 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
    break;


#line 1407 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:859
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
  BisonParser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what());
  }

  // Generate an error message.
  std::string
  BisonParser::yysyntax_error_ (state_type, const symbol_type&) const
  {
    return YY_("syntax error");
  }


  const short int BisonParser::yypact_ninf_ = -184;

  const short int BisonParser::yytable_ninf_ = -131;

  const short int
  BisonParser::yypact_[] =
  {
    -184,    35,  -184,  -184,   182,  -184,    49,  -184,  -184,  -184,
    -184,   116,   116,   182,  -184,  -184,    84,   105,  -184,  -184,
    -184,   277,   106,    48,   141,   142,  -184,  -184,    16,   134,
      52,  -184,  -184,  -184,  -184,  -184,  -184,  -184,   149,  -184,
      18,     7,  -184,   116,  -184,  -184,   161,   116,   184,  -184,
     303,    23,  -184,  -184,    90,   190,  -184,    13,  -184,   171,
    -184,  -184,   206,   277,  -184,   174,   186,   181,   188,  -184,
      90,   116,    90,    90,   116,   116,   116,  -184,  -184,   116,
     196,   196,   457,   201,   198,   223,   218,   207,  -184,   219,
     116,   116,   116,   104,  -184,  -184,   277,   202,  -184,   209,
    -184,  -184,   255,  -184,   240,   249,   395,   125,  -184,   457,
     457,   174,   174,   174,   234,    14,  -184,  -184,    90,    90,
      90,    90,    90,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,   116,   264,   116,   267,   268,    90,
     230,   257,  -184,    20,  -184,   171,   241,  -184,   186,   186,
    -184,   116,    90,    90,    90,   261,   314,  -184,    66,  -184,
     277,   243,  -184,  -184,  -184,   258,   269,   364,  -184,  -184,
    -184,   196,   286,  -184,   260,   271,   595,    65,   578,   578,
     578,   578,    60,    92,   559,   568,   488,   519,   528,   177,
     252,  -184,  -184,   282,   300,   265,  -184,   457,     9,   116,
      11,   323,    90,   316,  -184,  -184,  -184,  -184,   457,   457,
     426,   116,  -184,  -184,   305,  -184,  -184,  -184,    90,  -184,
    -184,  -184,  -184,   301,   326,  -184,  -184,    90,  -184,    89,
     186,   288,   311,   457,    90,  -184,  -184,   186,   278,   315,
     296,   108,   457,   335,   317,   457,  -184,   116,  -184,   457,
    -184,  -184,  -184,    64,     5,  -184,  -184,  -184,   116,   116,
     116,   108,  -184,   318,  -184,  -184,  -184,   287,   186,   186,
    -184,  -184,   186,  -184,  -184,  -184,   321,  -184,  -184,  -184,
     289,   306,   307,  -184,   319,   327,   328,   312,   186,  -184,
    -184,  -184,   297,   230,   332,   230,  -184,  -184,  -184,    12,
     186,    77,  -184,   186,   186,  -184,   186,  -184,  -184,   320,
    -184,  -184,   330,  -184,   331,  -184,  -184
  };

  const unsigned char
  BisonParser::yydefact_[] =
  {
      30,     0,    28,     1,     0,   135,     0,    49,    50,    51,
      52,     0,     0,    29,     3,     9,     0,     0,     7,    10,
      53,     0,     0,     0,     0,     0,    58,    63,     0,   127,
      30,   127,     4,    31,     6,     8,    54,    55,    59,    60,
       0,   127,     5,     0,    11,    12,     0,     0,     0,    62,
       0,     0,    28,     2,     0,     0,    28,     0,    36,     0,
      64,   128,     0,     0,    30,    30,     0,    79,    80,   100,
       0,     0,     0,     0,     0,     0,     0,    83,    84,     0,
      87,   101,    56,    89,     0,     0,    91,   131,    61,     0,
       0,     0,     0,     0,    65,    32,     0,     0,    59,   127,
      28,    28,     0,    29,     0,     0,     0,    93,   129,   119,
     120,    95,    97,    96,    92,     0,    88,   102,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   121,
       0,     0,    37,     0,    57,     0,     0,    33,     0,     0,
      27,     0,     0,     0,     0,     0,     0,    13,    87,    23,
       0,     0,    20,    24,    21,     0,     0,     0,    31,    81,
      82,    98,     0,    94,     0,     0,   108,   107,   103,   106,
     104,   105,   115,   116,   109,   110,   111,   112,   113,   114,
     117,   118,    90,   134,     0,   133,   132,   122,     0,     0,
       0,     0,     0,     0,    66,    34,    29,    29,    86,    28,
       0,     0,    26,    14,    28,    22,    16,    17,     0,    15,
      38,    99,   130,     0,     0,   126,    25,     0,    47,     0,
       0,     0,     0,    67,     0,    31,    31,     0,     0,     0,
       0,     0,    85,     0,     0,   123,    29,     0,    35,    68,
      39,    40,    29,     0,     0,    75,    28,    19,     0,     0,
       0,    29,    42,     0,   124,    31,    48,    69,     0,     0,
      74,    76,     0,    28,    28,    28,     0,    43,   125,    41,
      28,     0,     0,    29,     0,     0,     0,     0,     0,    71,
      78,    77,    72,     0,     0,     0,    18,    29,    28,     0,
       0,     0,    70,     0,     0,    29,     0,    29,    29,     0,
      29,    73,     0,    45,     0,    44,    46
  };

  const short int
  BisonParser::yypgoto_[] =
  {
    -184,  -184,  -184,   343,  -184,  -184,   212,   -99,   -31,   -51,
     -97,    95,  -153,  -184,  -184,  -184,  -184,   -90,  -184,  -184,
      96,   -62,   -17,   -40,   -82,   -38,   -36,    -9,  -184,  -183,
    -184,   227,    93,  -184,  -184,  -184,   131,   133,  -184,  -184,
      50,  -184,   -70,  -184,     2,    34,  -184,    -4
  };

  const short int
  BisonParser::yydefgoto_[] =
  {
      -1,     1,    13,    14,    15,   156,   157,    80,   103,     4,
      33,     2,    53,    16,    62,    17,    57,    18,    19,   261,
     262,   200,    20,    21,    22,    37,    38,    39,    26,    27,
      93,    94,   162,   163,   164,   254,   255,    81,   165,   166,
     167,   198,   116,    83,    84,    85,    86,    87
  };

  const short int
  BisonParser::yytable_[] =
  {
      23,    66,    25,   158,    36,    89,   168,    29,    31,    23,
      63,   117,   159,    28,    30,   220,   228,    41,   174,     5,
     161,     5,    96,    40,    49,    97,     5,    98,   -30,   270,
      90,   226,   202,   230,   304,     3,    49,   173,   227,    58,
     231,   231,    91,    60,    55,   115,    36,    31,    47,   148,
     149,   175,    24,    65,    48,    64,    48,   158,   146,    99,
      98,   253,   160,    43,   266,    40,   159,   108,    67,    68,
      31,    31,    31,   203,   161,   108,   111,   112,   113,    36,
     125,    51,   250,   251,   124,   125,   141,   142,   143,   214,
      48,   115,    99,     5,    67,    68,   268,    69,    40,   306,
     131,   221,   132,   133,    82,   107,   231,   132,   133,   235,
     236,    70,   279,   114,    77,    78,   160,   206,   207,     5,
     106,    47,   109,   110,    34,    52,    71,    48,   144,    72,
     192,    73,   194,   145,   132,   133,    56,    74,    75,    76,
      77,    78,    25,    36,    44,    35,    42,    31,    45,   265,
     115,    79,   172,    28,    46,   267,    99,    50,   237,   100,
     101,    54,    40,   241,   276,   258,   259,   260,   176,   177,
     178,   179,   180,   181,   182,   183,   184,   185,   186,   187,
     188,   189,   190,   191,    59,     5,   292,    61,     6,   197,
       7,     8,     9,    10,    88,    31,   124,   125,    92,   246,
     302,   229,   208,   209,   210,   272,   252,   239,   309,   102,
     311,   312,    48,   314,     7,     8,     9,    10,   104,   132,
     133,   115,   284,   285,   286,   105,    11,    12,   134,   288,
      95,   299,  -129,   301,  -129,  -129,   135,   281,   282,   139,
     140,   283,   147,    31,   169,  -127,    55,   303,   136,   229,
     137,   138,   233,   170,   273,   274,   275,   297,     5,    67,
      68,   172,    69,     7,     8,     9,    10,   193,   242,   305,
     195,   196,   307,   308,   199,   310,    70,   245,   201,   150,
       5,   205,   211,   215,   249,     7,     8,     9,    10,   222,
    -130,    71,  -130,  -130,    72,   133,    73,   223,   216,   151,
      12,   224,    74,    75,    76,    77,    78,   152,   153,   217,
     154,     7,     8,     9,    10,   155,    79,     5,    67,    68,
    -128,    69,     7,     8,     9,    10,   225,   232,   234,   240,
     244,   243,   247,   248,   253,    70,   257,   256,   212,   263,
     293,   280,   153,   264,   278,   287,   290,   291,   294,   295,
      71,   298,   296,    72,   300,    73,    32,   277,   151,    12,
     313,    74,    75,    76,    77,    78,   152,   153,   213,   154,
     315,   316,   204,   289,   155,    79,   218,   118,   119,   120,
     121,   122,   123,   124,   125,   271,   269,     0,     0,     0,
       0,     0,     0,     0,   126,   127,   128,   129,   130,   131,
       0,     0,     0,     0,   219,     0,   132,   133,   118,   119,
     120,   121,   122,   123,   124,   125,     0,   171,     0,     0,
       0,     0,     0,     0,     0,   126,   127,   128,   129,   130,
     131,     0,     0,     0,     0,     0,     0,   132,   133,   118,
     119,   120,   121,   122,   123,   124,   125,     0,     0,   238,
       0,     0,     0,     0,     0,     0,   126,   127,   128,   129,
     130,   131,     0,     0,     0,     0,     0,     0,   132,   133,
     118,   119,   120,   121,   122,   123,   124,   125,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   126,   127,   128,
     129,   130,   131,     0,     0,     0,     0,     0,     0,   132,
     133,   118,   119,   120,   121,   122,   123,   124,   125,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   126,   127,
       0,   129,   130,   131,     0,     0,     0,     0,     0,     0,
     132,   133,   118,   119,   120,   121,   122,   123,   124,   125,
       0,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,     0,     0,   130,   131,     0,     0,     0,   126,   127,
       0,   132,   133,   131,     0,     0,     0,     0,     0,     0,
     132,   133,   118,   119,   120,   121,   122,   123,   124,   125,
       0,   118,   119,   120,   121,   122,   123,   124,   125,     0,
     127,   118,   119,     0,   131,     0,     0,   124,   125,     0,
       0,   132,   133,   131,     0,     0,     0,     0,     0,   119,
     132,   133,     0,   131,   124,   125,     0,     0,     0,     0,
     132,   133,     0,     0,     0,     0,     0,     0,     0,     0,
     131,     0,     0,     0,     0,     0,     0,   132,   133
  };

  const short int
  BisonParser::yycheck_[] =
  {
       4,    52,    11,   102,    21,    56,   103,    11,    12,    13,
      50,    81,   102,    11,    12,   168,   199,    21,     4,     3,
     102,     3,    62,    21,    28,    63,     3,    63,    21,    24,
      17,    22,    12,    22,    22,     0,    40,   107,    29,    43,
      29,    29,    29,    47,    37,    25,    63,    51,    32,   100,
     101,    37,     3,    51,    38,    32,    38,   156,    96,    63,
      96,    56,   102,    15,   247,    63,   156,    71,     4,     5,
      74,    75,    76,   143,   156,    79,    74,    75,    76,    96,
      20,    29,   235,   236,    19,    20,    90,    91,    92,    23,
      38,    25,    96,     3,     4,     5,    32,     7,    96,    22,
      35,   171,    42,    43,    54,    71,    29,    42,    43,   206,
     207,    21,   265,    79,    50,    51,   156,   148,   149,     3,
      70,    32,    72,    73,    40,    30,    36,    38,    24,    39,
     134,    41,   136,    29,    42,    43,    41,    47,    48,    49,
      50,    51,   151,   160,     3,    40,    40,   151,     7,   246,
      25,    61,    27,   151,    12,   252,   160,    23,   209,    64,
      65,    12,   160,   214,   261,    57,    58,    59,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,    23,     3,   283,     3,     6,   139,
       8,     9,    10,    11,     4,   199,    19,    20,    27,   230,
     297,   199,   152,   153,   154,   256,   237,   211,   305,    23,
     307,   308,    38,   310,     8,     9,    10,    11,    37,    42,
      43,    25,   273,   274,   275,    37,    44,    45,    27,   280,
      24,   293,    25,   295,    27,    28,    38,   268,   269,    21,
      21,   272,    40,   247,     4,    38,    37,   298,    25,   247,
      27,    28,   202,     4,   258,   259,   260,   288,     3,     4,
       5,    27,     7,     8,     9,    10,    11,     3,   218,   300,
       3,     3,   303,   304,    44,   306,    21,   227,    21,    24,
       3,    40,    21,    40,   234,     8,     9,    10,    11,     3,
      25,    36,    27,    28,    39,    43,    41,    37,    40,    44,
      45,    30,    47,    48,    49,    50,    51,    52,    53,    40,
      55,     8,     9,    10,    11,    60,    61,     3,     4,     5,
      38,     7,     8,     9,    10,    11,    26,     4,    12,    24,
       4,    30,    44,    22,    56,    21,    40,    22,    24,     4,
      21,    54,    53,    26,    26,    24,    40,    40,    21,    21,
      36,    54,    40,    39,    22,    41,    13,   261,    44,    45,
      40,    47,    48,    49,    50,    51,    52,    53,   156,    55,
      40,    40,   145,   280,    60,    61,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   254,   253,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,    35,
      -1,    -1,    -1,    -1,    40,    -1,    42,    43,    13,    14,
      15,    16,    17,    18,    19,    20,    -1,    22,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,    34,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    -1,    23,
      -1,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,    33,
      34,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43,
      13,    14,    15,    16,    17,    18,    19,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    31,    32,
      33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,    42,
      43,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    30,    31,
      -1,    33,    34,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    43,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    30,
      31,    -1,    -1,    34,    35,    -1,    -1,    -1,    30,    31,
      -1,    42,    43,    35,    -1,    -1,    -1,    -1,    -1,    -1,
      42,    43,    13,    14,    15,    16,    17,    18,    19,    20,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      31,    13,    14,    -1,    35,    -1,    -1,    19,    20,    -1,
      -1,    42,    43,    35,    -1,    -1,    -1,    -1,    -1,    14,
      42,    43,    -1,    35,    19,    20,    -1,    -1,    -1,    -1,
      42,    43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      35,    -1,    -1,    -1,    -1,    -1,    -1,    42,    43
  };

  const unsigned char
  BisonParser::yystos_[] =
  {
       0,    65,    75,     0,    73,     3,     6,     8,     9,    10,
      11,    44,    45,    66,    67,    68,    77,    79,    81,    82,
      86,    87,    88,   111,     3,    91,    92,    93,   108,   111,
     108,   111,    67,    74,    40,    40,    86,    89,    90,    91,
     108,   111,    40,    15,     3,     7,    12,    32,    38,   111,
      23,    29,    75,    76,    12,    37,    75,    80,   111,    23,
     111,     3,    78,    87,    32,   108,    73,     4,     5,     7,
      21,    36,    39,    41,    47,    48,    49,    50,    51,    61,
      71,   101,   104,   107,   108,   109,   110,   111,     4,    73,
      17,    29,    27,    94,    95,    24,    87,    89,    90,   111,
      75,    75,    23,    72,    37,    37,   104,   109,   111,   104,
     104,   108,   108,   108,   109,    25,   106,   106,    13,    14,
      15,    16,    17,    18,    19,    20,    30,    31,    32,    33,
      34,    35,    42,    43,    27,    38,    25,    27,    28,    21,
      21,   111,   111,   111,    24,    29,    89,    40,    73,    73,
      24,    44,    52,    53,    55,    60,    69,    70,    71,    81,
      87,    88,    96,    97,    98,   102,   103,   104,    74,     4,
       4,    22,    27,   106,     4,    37,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   111,     3,   111,     3,     3,   104,   105,    44,
      85,    21,    12,   106,    95,    40,    72,    72,   104,   104,
     104,    21,    24,    70,    23,    40,    40,    40,    12,    40,
      76,   106,     3,    37,    30,    26,    22,    29,    93,   108,
      22,    29,     4,   104,    12,    74,    74,    73,    23,   111,
      24,    73,   104,    30,     4,   104,    72,    44,    22,   104,
      76,    76,    72,    56,    99,   100,    22,    40,    57,    58,
      59,    83,    84,     4,    26,    74,    93,    74,    32,   101,
      24,   100,    73,   111,   111,   111,    74,    84,    26,    76,
      54,    72,    72,    72,    73,    73,    73,    24,    73,    96,
      40,    40,    74,    21,    21,    21,    40,    72,    54,    85,
      22,    85,    74,    73,    22,    72,    22,    72,    72,    74,
      72,    74,    74,    40,    74,    40,    40
  };

  const unsigned char
  BisonParser::yyr1_[] =
  {
       0,    64,    65,    66,    66,    67,    67,    67,    67,    67,
      67,    68,    68,    69,    69,    70,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    71,    72,    72,    73,    74,
      75,    76,    77,    78,    78,    79,    80,    80,    81,    81,
      81,    82,    83,    83,    84,    84,    84,    85,    85,    86,
      86,    86,    86,    87,    87,    88,    88,    88,    88,    89,
      89,    90,    91,    92,    93,    94,    94,    95,    95,    96,
      96,    96,    97,    97,    98,    99,    99,   100,   100,   101,
     101,   101,   101,   101,   101,   102,   103,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   104,   104,   104,   104,   104,   104,   104,   104,   104,
     104,   105,   105,   105,   106,   106,   107,   108,   108,   109,
     109,   110,   110,   110,   110,   111
  };

  const unsigned char
  BisonParser::yyr2_[] =
  {
       0,     2,     5,     1,     2,     2,     2,     1,     2,     1,
       1,     3,     3,     1,     2,     2,     2,     2,     7,     4,
       1,     1,     2,     1,     1,     4,     3,     2,     0,     0,
       0,     0,     5,     3,     4,     8,     1,     3,     7,     9,
       9,    10,     1,     2,     9,     8,     9,     2,     4,     1,
       1,     1,     1,     1,     2,     2,     4,     6,     2,     1,
       1,     3,     2,     1,     3,     1,     3,     4,     5,     5,
       9,     7,     7,    11,     5,     1,     2,     4,     4,     1,
       1,     3,     3,     1,     1,     3,     2,     1,     2,     1,
       3,     1,     2,     2,     3,     2,     2,     2,     3,     4,
       1,     1,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       2,     0,     1,     3,     5,     6,     4,     1,     3,     1,
       3,     1,     3,     3,     3,     1
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const BisonParser::yytname_[] =
  {
  "$end", "error", "$undefined", "TIDENTIFIER", "TINTEGER", "THEXINTEGER",
  "TPRAGMA", "TSTRING", "TU8", "TU16", "TU32", "TU64", "TEQUAL", "TCEQ",
  "TCNE", "TCLT", "TCLE", "TCGT", "TCGE", "TAND", "TOR", "TLPAREN",
  "TRPAREN", "TLBRACE", "TRBRACE", "TLBRACK", "TRBRACK", "TDOT", "TARROW",
  "TCOMMA", "TPLUS", "TMINUS", "TMUL", "TDIV", "TMOD", "TXOR", "TDOLLAR",
  "TCOLON", "TSCOPE", "TNOT", "TSEMI", "TCMPL", "TLAND", "TLOR", "TSTRUCT",
  "TSTATE", "TFUNC", "TGOTO", "TCONTINUE", "TNEXT", "TTRUE", "TFALSE",
  "TRETURN", "TIF", "TELSE", "TSWITCH", "TCASE", "TMATCH", "TMISS",
  "TFAILURE", "TVALID", "TAT", "TINCR", "TDECR", "$accept", "program",
  "prog_decls", "prog_decl", "pragma_decl", "stmts", "stmt", "call_expr",
  "block", "enter_varscope", "exit_varscope", "enter_statescope",
  "exit_statescope", "struct_decl", "struct_decl_stmts", "table_decl",
  "table_decl_args", "state_decl", "func_decl", "table_result_stmts",
  "table_result_stmt", "formals", "type_specifier", "type_specifiers",
  "var_decl", "decl_stmt", "int_decl", "type_decl", "ref_stmt", "ptr_decl",
  "init_args_kv", "init_arg_kv", "if_stmt", "onvalid_stmt", "switch_stmt",
  "case_stmts", "case_stmt", "numeric", "assign_expr", "return_expr",
  "expr", "call_args", "bitop", "table_index_expr", "scoped_ident",
  "dotted_ident", "any_ident", "ident", YY_NULLPTR
  };


  const unsigned short int
  BisonParser::yyrline_[] =
  {
       0,   132,   132,   138,   140,   152,   153,   154,   155,   156,
     157,   161,   163,   168,   170,   175,   178,   181,   184,   189,
     192,   193,   194,   196,   197,   201,   207,   210,   215,   216,
     217,   218,   221,   227,   229,   234,   240,   242,   247,   251,
     255,   262,   269,   271,   276,   280,   284,   291,   293,   298,
     299,   300,   301,   305,   306,   310,   314,   318,   325,   332,
     332,   333,   338,   344,   345,   359,   360,   363,   366,   372,
     376,   380,   387,   391,   398,   404,   406,   411,   414,   420,
     423,   426,   429,   432,   435,   441,   455,   461,   463,   465,
     467,   469,   471,   475,   479,   483,   486,   489,   492,   494,
     496,   499,   501,   503,   506,   509,   512,   515,   518,   521,
     524,   527,   530,   533,   536,   539,   542,   545,   548,   553,
     556,   563,   564,   566,   571,   574,   580,   586,   588,   593,
     595,   600,   602,   604,   606,   611
  };

  // Print the state stack on the debug stream.
  void
  BisonParser::yystack_print_ ()
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
  BisonParser::yy_reduce_print_ (int yyrule)
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
  BisonParser::token_number_type
  BisonParser::yytranslate_ (int t)
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
      55,    56,    57,    58,    59,    60,    61,    62,    63
    };
    const unsigned int user_token_number_max_ = 318;
    const token_number_type undef_token_ = 2;

    if (static_cast<int>(t) <= yyeof_)
      return yyeof_;
    else if (static_cast<unsigned int> (t) <= user_token_number_max_)
      return translate_table[t];
    else
      return undef_token_;
  }

#line 19 "parser.yy" // lalr1.cc:1167
} } // ebpf::cc
#line 2016 "/home/pino/polycube/cmake-build-debug/src/libs/bcc/src/cc/frontends/b/parser.yy.cc" // lalr1.cc:1167
#line 616 "parser.yy" // lalr1.cc:1168


void ebpf::cc::BisonParser::error(const ebpf::cc::BisonParser::location_type &loc,
                            const string& msg) {
    std::cerr << "Error: " << loc << " " << msg << std::endl;
}

#include "lexer.h"
static int yylex(ebpf::cc::BisonParser::semantic_type *yylval,
                 ebpf::cc::BisonParser::location_type *yylloc,
                 ebpf::cc::Lexer &lexer) {
    return lexer.yylex(yylval, yylloc);
}

