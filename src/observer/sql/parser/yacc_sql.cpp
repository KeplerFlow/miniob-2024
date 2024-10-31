/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 1



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     SEMICOLON = 258,
     BY = 259,
     CREATE = 260,
     DROP = 261,
     GROUP = 262,
     TABLE = 263,
     TABLES = 264,
     INDEX = 265,
     CALC = 266,
     SELECT = 267,
     DESC = 268,
     SHOW = 269,
     SYNC = 270,
     INSERT = 271,
     DELETE = 272,
     UPDATE = 273,
     LBRACE = 274,
     RBRACE = 275,
     COMMA = 276,
     TRX_BEGIN = 277,
     TRX_COMMIT = 278,
     TRX_ROLLBACK = 279,
     INT_T = 280,
     STRING_T = 281,
     FLOAT_T = 282,
     DATE_T = 283,
     VECTOR_T = 284,
     HELP = 285,
     EXIT = 286,
     DOT = 287,
     INTO = 288,
     VALUES = 289,
     FROM = 290,
     WHERE = 291,
     AND = 292,
     SET = 293,
     ON = 294,
     LOAD = 295,
     DATA = 296,
     INFILE = 297,
     EXPLAIN = 298,
     STORAGE = 299,
     FORMAT = 300,
     EQ = 301,
     LT = 302,
     GT = 303,
     LE = 304,
     GE = 305,
     NE = 306,
     NUMBER = 307,
     FLOAT = 308,
     ID = 309,
     SSS = 310,
     UMINUS = 311
   };
#endif
/* Tokens.  */
#define SEMICOLON 258
#define BY 259
#define CREATE 260
#define DROP 261
#define GROUP 262
#define TABLE 263
#define TABLES 264
#define INDEX 265
#define CALC 266
#define SELECT 267
#define DESC 268
#define SHOW 269
#define SYNC 270
#define INSERT 271
#define DELETE 272
#define UPDATE 273
#define LBRACE 274
#define RBRACE 275
#define COMMA 276
#define TRX_BEGIN 277
#define TRX_COMMIT 278
#define TRX_ROLLBACK 279
#define INT_T 280
#define STRING_T 281
#define FLOAT_T 282
#define DATE_T 283
#define VECTOR_T 284
#define HELP 285
#define EXIT 286
#define DOT 287
#define INTO 288
#define VALUES 289
#define FROM 290
#define WHERE 291
#define AND 292
#define SET 293
#define ON 294
#define LOAD 295
#define DATA 296
#define INFILE 297
#define EXPLAIN 298
#define STORAGE 299
#define FORMAT 300
#define EQ 301
#define LT 302
#define GT 303
#define LE 304
#define GE 305
#define NE 306
#define NUMBER 307
#define FLOAT 308
#define ID 309
#define SSS 310
#define UMINUS 311




/* Copy the first part of user declarations.  */
#line 2 "yacc_sql.y"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.hpp"
#include "sql/parser/lex_sql.h"
#include "sql/expr/expression.h"

using namespace std;

string token_name(const char *sql_string, YYLTYPE *llocp)
{
  return string(sql_string + llocp->first_column, llocp->last_column - llocp->first_column + 1);
}

int yyerror(YYLTYPE *llocp, const char *sql_string, ParsedSqlResult *sql_result, yyscan_t scanner, const char *msg)
{
  std::unique_ptr<ParsedSqlNode> error_sql_node = std::make_unique<ParsedSqlNode>(SCF_ERROR);
  error_sql_node->error.error_msg = msg;
  error_sql_node->error.line = llocp->first_line;
  error_sql_node->error.column = llocp->first_column;
  sql_result->add_sql_node(std::move(error_sql_node));
  return 0;
}

ArithmeticExpr *create_arithmetic_expression(ArithmeticExpr::Type type,
                                             Expression *left,
                                             Expression *right,
                                             const char *sql_string,
                                             YYLTYPE *llocp)
{
  ArithmeticExpr *expr = new ArithmeticExpr(type, left, right);
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}

UnboundAggregateExpr *create_aggregate_expression(const char *aggregate_name,
                                           Expression *child,
                                           const char *sql_string,
                                           YYLTYPE *llocp)
{
  UnboundAggregateExpr *expr = new UnboundAggregateExpr(aggregate_name, child);
  expr->set_name(token_name(sql_string, llocp));
  return expr;
}



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 118 "yacc_sql.y"
{
  ParsedSqlNode *                            sql_node;
  ConditionSqlNode *                         condition;
  Value *                                    value;
  enum CompOp                                comp;
  RelAttrSqlNode *                           rel_attr;
  std::vector<AttrInfoSqlNode> *             attr_infos;
  AttrInfoSqlNode *                          attr_info;
  Expression *                               expression;
  std::vector<std::unique_ptr<Expression>> * expression_list;
  std::vector<Value> *                       value_list;
  std::vector<ConditionSqlNode> *            condition_list;
  std::vector<RelAttrSqlNode> *              rel_attr_list;
  std::vector<std::string> *                 relation_list;
  char *                                     string;
  int                                        number;
  float                                      floats;
}
/* Line 193 of yacc.c.  */
#line 281 "yacc_sql.cpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 306 "yacc_sql.cpp"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
	     && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
    YYLTYPE yyls;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE) + sizeof (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  65
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   141

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  61
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  41
/* YYNRULES -- Number of rules.  */
#define YYNRULES  93
/* YYNRULES -- Number of states.  */
#define YYNSTATES  167

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   311

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,    58,    56,     2,    57,     2,    59,     2,     2,
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
      55,    60
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     8,    10,    12,    14,    16,    18,
      20,    22,    24,    26,    28,    30,    32,    34,    36,    38,
      40,    42,    44,    46,    48,    50,    52,    54,    56,    58,
      62,    65,    68,    77,    83,    92,    93,    97,   103,   106,
     108,   110,   112,   114,   116,   118,   127,   128,   132,   134,
     136,   138,   139,   144,   149,   157,   164,   167,   169,   173,
     177,   181,   185,   189,   193,   196,   198,   200,   202,   204,
     208,   210,   212,   216,   217,   220,   221,   223,   227,   231,
     235,   239,   243,   245,   247,   249,   251,   253,   255,   256,
     264,   267,   272,   273
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      62,     0,    -1,    63,   101,    -1,    87,    -1,    86,    -1,
      80,    -1,    85,    -1,    84,    -1,    75,    -1,    70,    -1,
      71,    -1,    72,    -1,    73,    -1,    74,    -1,    66,    -1,
      67,    -1,    68,    -1,    69,    -1,    98,    -1,    99,    -1,
     100,    -1,    65,    -1,    64,    -1,    31,    -1,    30,    -1,
      15,    -1,    22,    -1,    23,    -1,    24,    -1,     6,     8,
      54,    -1,    14,     9,    -1,    13,    54,    -1,     5,    10,
      54,    39,    54,    19,    54,    20,    -1,     6,    10,    54,
      39,    54,    -1,     5,     8,    54,    19,    77,    76,    20,
      83,    -1,    -1,    21,    77,    76,    -1,    54,    79,    19,
      78,    20,    -1,    54,    79,    -1,    52,    -1,    25,    -1,
      26,    -1,    27,    -1,    28,    -1,    29,    -1,    16,    33,
      54,    34,    19,    82,    81,    20,    -1,    -1,    21,    82,
      81,    -1,    52,    -1,    53,    -1,    55,    -1,    -1,    44,
      45,    46,    54,    -1,    17,    35,    54,    93,    -1,    18,
      54,    38,    54,    46,    82,    93,    -1,    12,    88,    35,
      92,    93,    97,    -1,    11,    88,    -1,    89,    -1,    89,
      21,    88,    -1,    89,    56,    89,    -1,    89,    57,    89,
      -1,    89,    58,    89,    -1,    89,    59,    89,    -1,    19,
      89,    20,    -1,    57,    89,    -1,    82,    -1,    90,    -1,
      58,    -1,    54,    -1,    54,    32,    54,    -1,    54,    -1,
      91,    -1,    91,    21,    92,    -1,    -1,    36,    94,    -1,
      -1,    95,    -1,    95,    37,    94,    -1,    90,    96,    82,
      -1,    82,    96,    82,    -1,    90,    96,    90,    -1,    82,
      96,    90,    -1,    46,    -1,    47,    -1,    48,    -1,    49,
      -1,    50,    -1,    51,    -1,    -1,    40,    41,    42,    55,
      33,     8,    54,    -1,    43,    63,    -1,    38,    54,    46,
      82,    -1,    -1,     3,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   190,   190,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   221,   227,   232,   238,   244,   250,   256,
     263,   269,   277,   291,   301,   325,   328,   341,   349,   359,
     362,   363,   364,   365,   366,   369,   386,   389,   400,   404,
     408,   417,   420,   427,   439,   454,   479,   488,   493,   504,
     507,   510,   513,   516,   520,   523,   528,   534,   541,   546,
     556,   561,   566,   580,   583,   589,   592,   597,   604,   616,
     628,   640,   655,   656,   657,   658,   659,   660,   666,   671,
     684,   692,   702,   703
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "SEMICOLON", "BY", "CREATE", "DROP",
  "GROUP", "TABLE", "TABLES", "INDEX", "CALC", "SELECT", "DESC", "SHOW",
  "SYNC", "INSERT", "DELETE", "UPDATE", "LBRACE", "RBRACE", "COMMA",
  "TRX_BEGIN", "TRX_COMMIT", "TRX_ROLLBACK", "INT_T", "STRING_T",
  "FLOAT_T", "DATE_T", "VECTOR_T", "HELP", "EXIT", "DOT", "INTO", "VALUES",
  "FROM", "WHERE", "AND", "SET", "ON", "LOAD", "DATA", "INFILE", "EXPLAIN",
  "STORAGE", "FORMAT", "EQ", "LT", "GT", "LE", "GE", "NE", "NUMBER",
  "FLOAT", "ID", "SSS", "'+'", "'-'", "'*'", "'/'", "UMINUS", "$accept",
  "commands", "command_wrapper", "exit_stmt", "help_stmt", "sync_stmt",
  "begin_stmt", "commit_stmt", "rollback_stmt", "drop_table_stmt",
  "show_tables_stmt", "desc_table_stmt", "create_index_stmt",
  "drop_index_stmt", "create_table_stmt", "attr_def_list", "attr_def",
  "number", "type", "insert_stmt", "value_list", "value", "storage_format",
  "delete_stmt", "update_stmt", "select_stmt", "calc_stmt",
  "expression_list", "expression", "rel_attr", "relation", "rel_list",
  "where", "condition_list", "condition", "comp_op", "group_by",
  "load_data_stmt", "explain_stmt", "set_variable_stmt", "opt_semicolon", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,    43,    45,    42,    47,
     311
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    61,    62,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    63,
      63,    63,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    76,    77,    77,    78,
      79,    79,    79,    79,    79,    80,    81,    81,    82,    82,
      82,    83,    83,    84,    85,    86,    87,    88,    88,    89,
      89,    89,    89,    89,    89,    89,    89,    89,    90,    90,
      91,    92,    92,    93,    93,    94,    94,    94,    95,    95,
      95,    95,    96,    96,    96,    96,    96,    96,    97,    98,
      99,   100,   101,   101
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       2,     2,     8,     5,     8,     0,     3,     5,     2,     1,
       1,     1,     1,     1,     1,     8,     0,     3,     1,     1,
       1,     0,     4,     4,     7,     6,     2,     1,     3,     3,
       3,     3,     3,     3,     2,     1,     1,     1,     1,     3,
       1,     1,     3,     0,     2,     0,     1,     3,     3,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     0,     7,
       2,     4,     0,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,    25,     0,     0,
       0,    26,    27,    28,    24,    23,     0,     0,     0,     0,
      92,    22,    21,    14,    15,    16,    17,     9,    10,    11,
      12,    13,     8,     5,     7,     6,     4,     3,    18,    19,
      20,     0,     0,     0,     0,     0,    48,    49,    68,    50,
       0,    67,    65,    56,    57,    66,     0,    31,    30,     0,
       0,     0,     0,     0,    90,     1,    93,     2,     0,     0,
      29,     0,     0,     0,    64,     0,     0,     0,     0,     0,
       0,     0,    73,     0,     0,     0,     0,     0,     0,    63,
      69,    58,    59,    60,    61,    62,    70,    71,    73,     0,
      75,    53,     0,    91,     0,     0,    35,     0,    33,     0,
      88,     0,     0,     0,    74,    76,     0,     0,    40,    41,
      42,    43,    44,    38,     0,     0,     0,    72,    55,    46,
      82,    83,    84,    85,    86,    87,     0,     0,    75,    73,
       0,     0,    35,    51,     0,     0,     0,    79,    81,    78,
      80,    77,    54,    89,    39,     0,    36,     0,    34,    32,
      46,    45,    37,     0,    47,     0,    52
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,   125,   106,   155,   123,    33,
     146,    52,   158,    34,    35,    36,    37,    53,    54,    55,
      97,    98,   101,   114,   115,   136,   128,    38,    39,    40,
      67
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -97
static const yytype_int8 yypact[] =
{
      52,     6,    16,   -16,   -16,   -39,     2,   -97,    -6,   -10,
     -23,   -97,   -97,   -97,   -97,   -97,   -22,    -1,    52,    34,
      32,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,   -97,
     -97,    -2,    19,    24,    25,   -16,   -97,   -97,    48,   -97,
     -16,   -97,   -97,   -97,    -8,   -97,    21,   -97,   -97,    37,
      39,    43,    50,    55,   -97,   -97,   -97,   -97,    75,    59,
     -97,    60,   -12,    46,   -97,   -16,   -16,   -16,   -16,   -16,
      47,    68,    71,    54,   -43,    56,    58,    61,    62,   -97,
     -97,   -97,   -29,   -29,   -97,   -97,   -97,    88,    71,    91,
     -48,   -97,    67,   -97,    81,    -7,    96,    99,   -97,    47,
     -97,   -43,    38,    38,   -97,    82,   -43,   112,   -97,   -97,
     -97,   -97,   -97,   102,    58,   103,    70,   -97,   -97,   101,
     -97,   -97,   -97,   -97,   -97,   -97,   -48,   -48,   -48,    71,
      72,    73,    96,    83,   108,   -43,   109,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,   -97,   110,   -97,    86,   -97,   -97,
     101,   -97,   -97,    87,   -97,    78,   -97
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -97,   -97,   116,   -97,   -97,   -97,   -97,   -97,   -97,   -97,
     -97,   -97,   -97,   -97,   -97,    -5,    11,   -97,   -97,   -97,
     -24,   -83,   -97,   -97,   -97,   -97,   -97,    -4,    27,   -77,
     -97,    29,   -96,     1,   -97,    28,   -97,   -97,   -97,   -97,
     -97
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const yytype_uint8 yytable[] =
{
      56,   103,   110,    45,    46,    47,    48,    49,    89,    46,
      47,    58,    49,    75,    41,    57,    42,   112,   118,   119,
     120,   121,   122,   113,    43,    60,    44,    59,   129,    78,
      79,    61,    62,   139,    65,    66,    46,    47,    48,    49,
      63,    50,    51,   152,    76,    77,    78,    79,    76,    77,
      78,    79,    68,   147,   149,   112,    80,     1,     2,   148,
     150,   113,   160,     3,     4,     5,     6,     7,     8,     9,
      10,    91,    72,    69,    11,    12,    13,    74,    70,    71,
      73,    83,    14,    15,   130,   131,   132,   133,   134,   135,
      16,    81,    17,    82,    86,    18,    84,    85,    87,    88,
      90,    96,    99,    92,    93,    94,    95,   100,   102,   109,
     111,   104,   105,   116,   117,   107,   108,   124,   126,   138,
     140,   141,   145,   143,   144,   154,   153,   157,   159,   161,
     162,   163,   166,   165,    64,   142,   164,   156,   127,   151,
       0,   137
};

static const yytype_int16 yycheck[] =
{
       4,    84,    98,    19,    52,    53,    54,    55,    20,    52,
      53,     9,    55,    21,     8,    54,    10,   100,    25,    26,
      27,    28,    29,   100,     8,    35,    10,    33,   111,    58,
      59,    54,    54,   116,     0,     3,    52,    53,    54,    55,
      41,    57,    58,   139,    56,    57,    58,    59,    56,    57,
      58,    59,    54,   136,   137,   138,    35,     5,     6,   136,
     137,   138,   145,    11,    12,    13,    14,    15,    16,    17,
      18,    75,    45,    54,    22,    23,    24,    50,    54,    54,
      32,    38,    30,    31,    46,    47,    48,    49,    50,    51,
      38,    54,    40,    54,    19,    43,    46,    42,    39,    39,
      54,    54,    34,    76,    77,    78,    79,    36,    54,    21,
      19,    55,    54,    46,    33,    54,    54,    21,    19,    37,
       8,    19,    21,    20,    54,    52,    54,    44,    20,    20,
      20,    45,    54,    46,    18,   124,   160,   142,   109,   138,
      -1,   113
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     5,     6,    11,    12,    13,    14,    15,    16,    17,
      18,    22,    23,    24,    30,    31,    38,    40,    43,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    80,    84,    85,    86,    87,    98,    99,
     100,     8,    10,     8,    10,    19,    52,    53,    54,    55,
      57,    58,    82,    88,    89,    90,    88,    54,     9,    33,
      35,    54,    54,    41,    63,     0,     3,   101,    54,    54,
      54,    54,    89,    32,    89,    21,    56,    57,    58,    59,
      35,    54,    54,    38,    46,    42,    19,    39,    39,    20,
      54,    88,    89,    89,    89,    89,    54,    91,    92,    34,
      36,    93,    54,    82,    55,    54,    77,    54,    54,    21,
      93,    19,    82,    90,    94,    95,    46,    33,    25,    26,
      27,    28,    29,    79,    21,    76,    19,    92,    97,    82,
      46,    47,    48,    49,    50,    51,    96,    96,    37,    82,
       8,    19,    77,    20,    54,    21,    81,    82,    90,    82,
      90,    94,    93,    54,    52,    78,    76,    44,    83,    20,
      82,    20,    20,    45,    81,    46,    54
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (sql_string, sql_result, scanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex (scanner)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, Location, sql_string, sql_result, scanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, sql_string, sql_result, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    const char * sql_string;
    ParsedSqlResult * sql_result;
    void * scanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (yylocationp);
  YYUSE (sql_string);
  YYUSE (sql_result);
  YYUSE (scanner);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, yylocationp, sql_string, sql_result, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    YYLTYPE const * const yylocationp;
    const char * sql_string;
    ParsedSqlResult * sql_result;
    void * scanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
  yy_symbol_value_print (yyoutput, yytype, yyvaluep, yylocationp, sql_string, sql_result, scanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, YYLTYPE *yylsp, int yyrule, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
#else
static void
yy_reduce_print (yyvsp, yylsp, yyrule, sql_string, sql_result, scanner)
    YYSTYPE *yyvsp;
    YYLTYPE *yylsp;
    int yyrule;
    const char * sql_string;
    ParsedSqlResult * sql_result;
    void * scanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       , &(yylsp[(yyi + 1) - (yynrhs)])		       , sql_string, sql_result, scanner);
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, yylsp, Rule, sql_string, sql_result, scanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
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
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, yylocationp, sql_string, sql_result, scanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    YYLTYPE *yylocationp;
    const char * sql_string;
    ParsedSqlResult * sql_result;
    void * scanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (yylocationp);
  YYUSE (sql_string);
  YYUSE (sql_result);
  YYUSE (scanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (const char * sql_string, ParsedSqlResult * sql_result, void * scanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;
/* Location data for the look-ahead symbol.  */
YYLTYPE yylloc;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (const char * sql_string, ParsedSqlResult * sql_result, void * scanner)
#else
int
yyparse (sql_string, sql_result, scanner)
    const char * sql_string;
    ParsedSqlResult * sql_result;
    void * scanner;
#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;

  /* The location stack.  */
  YYLTYPE yylsa[YYINITDEPTH];
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;
  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[2];

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;
  yylsp = yyls;
#if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  /* Initialize the default location before parsing starts.  */
  yylloc.first_line   = yylloc.last_line   = 1;
  yylloc.first_column = yylloc.last_column = 0;
#endif

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;
	YYLTYPE *yyls1 = yyls;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yyls1, yysize * sizeof (*yylsp),
		    &yystacksize);
	yyls = yyls1;
	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);
	YYSTACK_RELOCATE (yyls);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;
  *++yylsp = yylloc;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location.  */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 191 "yacc_sql.y"
    {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[(1) - (2)].sql_node));
    sql_result->add_sql_node(std::move(sql_node));
  ;}
    break;

  case 23:
#line 221 "yacc_sql.y"
    {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXIT);
    ;}
    break;

  case 24:
#line 227 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_HELP);
    ;}
    break;

  case 25:
#line 232 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SYNC);
    ;}
    break;

  case 26:
#line 238 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_BEGIN);
    ;}
    break;

  case 27:
#line 244 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_COMMIT);
    ;}
    break;

  case 28:
#line 250 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_ROLLBACK);
    ;}
    break;

  case 29:
#line 256 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_TABLE);
      (yyval.sql_node)->drop_table.relation_name = (yyvsp[(3) - (3)].string);
      free((yyvsp[(3) - (3)].string));
    ;}
    break;

  case 30:
#line 263 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SHOW_TABLES);
    ;}
    break;

  case 31:
#line 269 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DESC_TABLE);
      (yyval.sql_node)->desc_table.relation_name = (yyvsp[(2) - (2)].string);
      free((yyvsp[(2) - (2)].string));
    ;}
    break;

  case 32:
#line 278 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = (yyval.sql_node)->create_index;
      create_index.index_name = (yyvsp[(3) - (8)].string);
      create_index.relation_name = (yyvsp[(5) - (8)].string);
      create_index.attribute_name = (yyvsp[(7) - (8)].string);
      free((yyvsp[(3) - (8)].string));
      free((yyvsp[(5) - (8)].string));
      free((yyvsp[(7) - (8)].string));
    ;}
    break;

  case 33:
#line 292 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DROP_INDEX);
      (yyval.sql_node)->drop_index.index_name = (yyvsp[(3) - (5)].string);
      (yyval.sql_node)->drop_index.relation_name = (yyvsp[(5) - (5)].string);
      free((yyvsp[(3) - (5)].string));
      free((yyvsp[(5) - (5)].string));
    ;}
    break;

  case 34:
#line 302 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CREATE_TABLE);
      CreateTableSqlNode &create_table = (yyval.sql_node)->create_table;
      create_table.relation_name = (yyvsp[(3) - (8)].string);
      free((yyvsp[(3) - (8)].string));

      std::vector<AttrInfoSqlNode> *src_attrs = (yyvsp[(6) - (8)].attr_infos);

      if (src_attrs != nullptr) {
        create_table.attr_infos.swap(*src_attrs);
        delete src_attrs;
      }
      create_table.attr_infos.emplace_back(*(yyvsp[(5) - (8)].attr_info));
      std::reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
      delete (yyvsp[(5) - (8)].attr_info);
      if ((yyvsp[(8) - (8)].string) != nullptr) {
        create_table.storage_format = (yyvsp[(8) - (8)].string);
        free((yyvsp[(8) - (8)].string));
      }
    ;}
    break;

  case 35:
#line 325 "yacc_sql.y"
    {
      (yyval.attr_infos) = nullptr;
    ;}
    break;

  case 36:
#line 329 "yacc_sql.y"
    {
      if ((yyvsp[(3) - (3)].attr_infos) != nullptr) {
        (yyval.attr_infos) = (yyvsp[(3) - (3)].attr_infos);
      } else {
        (yyval.attr_infos) = new std::vector<AttrInfoSqlNode>;
      }
      (yyval.attr_infos)->emplace_back(*(yyvsp[(2) - (3)].attr_info));
      delete (yyvsp[(2) - (3)].attr_info);
    ;}
    break;

  case 37:
#line 342 "yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->type = (AttrType)(yyvsp[(2) - (5)].number);
      (yyval.attr_info)->name = (yyvsp[(1) - (5)].string);
      (yyval.attr_info)->length = (yyvsp[(4) - (5)].number);
      free((yyvsp[(1) - (5)].string));
    ;}
    break;

  case 38:
#line 350 "yacc_sql.y"
    {
      (yyval.attr_info) = new AttrInfoSqlNode;
      (yyval.attr_info)->type = (AttrType)(yyvsp[(2) - (2)].number);
      (yyval.attr_info)->name = (yyvsp[(1) - (2)].string);
      (yyval.attr_info)->length = 4;
      free((yyvsp[(1) - (2)].string));
    ;}
    break;

  case 39:
#line 359 "yacc_sql.y"
    {(yyval.number) = (yyvsp[(1) - (1)].number);;}
    break;

  case 40:
#line 362 "yacc_sql.y"
    { (yyval.number) = static_cast<int>(AttrType::INTS); ;}
    break;

  case 41:
#line 363 "yacc_sql.y"
    { (yyval.number) = static_cast<int>(AttrType::CHARS); ;}
    break;

  case 42:
#line 364 "yacc_sql.y"
    { (yyval.number) = static_cast<int>(AttrType::FLOATS); ;}
    break;

  case 43:
#line 365 "yacc_sql.y"
    { (yyval.number) = static_cast<int>(AttrType::DATES); ;}
    break;

  case 44:
#line 366 "yacc_sql.y"
    { (yyval.number) = static_cast<int>(AttrType::VECTORS); ;}
    break;

  case 45:
#line 370 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_INSERT);
      (yyval.sql_node)->insertion.relation_name = (yyvsp[(3) - (8)].string);
      if ((yyvsp[(7) - (8)].value_list) != nullptr) {
        (yyval.sql_node)->insertion.values.swap(*(yyvsp[(7) - (8)].value_list));
        delete (yyvsp[(7) - (8)].value_list);
      }
      (yyval.sql_node)->insertion.values.emplace_back(*(yyvsp[(6) - (8)].value));
      std::reverse((yyval.sql_node)->insertion.values.begin(), (yyval.sql_node)->insertion.values.end());
      delete (yyvsp[(6) - (8)].value);
      free((yyvsp[(3) - (8)].string));
    ;}
    break;

  case 46:
#line 386 "yacc_sql.y"
    {
      (yyval.value_list) = nullptr;
    ;}
    break;

  case 47:
#line 389 "yacc_sql.y"
    { 
      if ((yyvsp[(3) - (3)].value_list) != nullptr) {
        (yyval.value_list) = (yyvsp[(3) - (3)].value_list);
      } else {
        (yyval.value_list) = new std::vector<Value>;
      }
      (yyval.value_list)->emplace_back(*(yyvsp[(2) - (3)].value));
      delete (yyvsp[(2) - (3)].value);
    ;}
    break;

  case 48:
#line 400 "yacc_sql.y"
    {
      (yyval.value) = new Value((int)(yyvsp[(1) - (1)].number));
      (yyloc) = (yylsp[(1) - (1)]);
    ;}
    break;

  case 49:
#line 404 "yacc_sql.y"
    {
      (yyval.value) = new Value((float)(yyvsp[(1) - (1)].floats));
      (yyloc) = (yylsp[(1) - (1)]);
    ;}
    break;

  case 50:
#line 408 "yacc_sql.y"
    {
      char *tmp = common::substr((yyvsp[(1) - (1)].string),1,strlen((yyvsp[(1) - (1)].string))-2);
      (yyval.value) = new Value(tmp);
      free(tmp);
      free((yyvsp[(1) - (1)].string));
    ;}
    break;

  case 51:
#line 417 "yacc_sql.y"
    {
      (yyval.string) = nullptr;
    ;}
    break;

  case 52:
#line 421 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[(4) - (4)].string);
    ;}
    break;

  case 53:
#line 428 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_DELETE);
      (yyval.sql_node)->deletion.relation_name = (yyvsp[(3) - (4)].string);
      if ((yyvsp[(4) - (4)].condition_list) != nullptr) {
        (yyval.sql_node)->deletion.conditions.swap(*(yyvsp[(4) - (4)].condition_list));
        delete (yyvsp[(4) - (4)].condition_list);
      }
      free((yyvsp[(3) - (4)].string));
    ;}
    break;

  case 54:
#line 440 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_UPDATE);
      (yyval.sql_node)->update.relation_name = (yyvsp[(2) - (7)].string);
      (yyval.sql_node)->update.attribute_name = (yyvsp[(4) - (7)].string);
      (yyval.sql_node)->update.value = *(yyvsp[(6) - (7)].value);
      if ((yyvsp[(7) - (7)].condition_list) != nullptr) {
        (yyval.sql_node)->update.conditions.swap(*(yyvsp[(7) - (7)].condition_list));
        delete (yyvsp[(7) - (7)].condition_list);
      }
      free((yyvsp[(2) - (7)].string));
      free((yyvsp[(4) - (7)].string));
    ;}
    break;

  case 55:
#line 455 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SELECT);
      if ((yyvsp[(2) - (6)].expression_list) != nullptr) {
        (yyval.sql_node)->selection.expressions.swap(*(yyvsp[(2) - (6)].expression_list));
        delete (yyvsp[(2) - (6)].expression_list);
      }

      if ((yyvsp[(4) - (6)].relation_list) != nullptr) {
        (yyval.sql_node)->selection.relations.swap(*(yyvsp[(4) - (6)].relation_list));
        delete (yyvsp[(4) - (6)].relation_list);
      }

      if ((yyvsp[(5) - (6)].condition_list) != nullptr) {
        (yyval.sql_node)->selection.conditions.swap(*(yyvsp[(5) - (6)].condition_list));
        delete (yyvsp[(5) - (6)].condition_list);
      }

      if ((yyvsp[(6) - (6)].expression_list) != nullptr) {
        (yyval.sql_node)->selection.group_by.swap(*(yyvsp[(6) - (6)].expression_list));
        delete (yyvsp[(6) - (6)].expression_list);
      }
    ;}
    break;

  case 56:
#line 480 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_CALC);
      (yyval.sql_node)->calc.expressions.swap(*(yyvsp[(2) - (2)].expression_list));
      delete (yyvsp[(2) - (2)].expression_list);
    ;}
    break;

  case 57:
#line 489 "yacc_sql.y"
    {
      (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      (yyval.expression_list)->emplace_back((yyvsp[(1) - (1)].expression));
    ;}
    break;

  case 58:
#line 494 "yacc_sql.y"
    {
      if ((yyvsp[(3) - (3)].expression_list) != nullptr) {
        (yyval.expression_list) = (yyvsp[(3) - (3)].expression_list);
      } else {
        (yyval.expression_list) = new std::vector<std::unique_ptr<Expression>>;
      }
      (yyval.expression_list)->emplace((yyval.expression_list)->begin(), (yyvsp[(1) - (3)].expression));
    ;}
    break;

  case 59:
#line 504 "yacc_sql.y"
    {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::ADD, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), sql_string, &(yyloc));
    ;}
    break;

  case 60:
#line 507 "yacc_sql.y"
    {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::SUB, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), sql_string, &(yyloc));
    ;}
    break;

  case 61:
#line 510 "yacc_sql.y"
    {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::MUL, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), sql_string, &(yyloc));
    ;}
    break;

  case 62:
#line 513 "yacc_sql.y"
    {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::DIV, (yyvsp[(1) - (3)].expression), (yyvsp[(3) - (3)].expression), sql_string, &(yyloc));
    ;}
    break;

  case 63:
#line 516 "yacc_sql.y"
    {
      (yyval.expression) = (yyvsp[(2) - (3)].expression);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
    ;}
    break;

  case 64:
#line 520 "yacc_sql.y"
    {
      (yyval.expression) = create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, (yyvsp[(2) - (2)].expression), nullptr, sql_string, &(yyloc));
    ;}
    break;

  case 65:
#line 523 "yacc_sql.y"
    {
      (yyval.expression) = new ValueExpr(*(yyvsp[(1) - (1)].value));
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[(1) - (1)].value);
    ;}
    break;

  case 66:
#line 528 "yacc_sql.y"
    {
      RelAttrSqlNode *node = (yyvsp[(1) - (1)].rel_attr);
      (yyval.expression) = new UnboundFieldExpr(node->relation_name, node->attribute_name);
      (yyval.expression)->set_name(token_name(sql_string, &(yyloc)));
      delete (yyvsp[(1) - (1)].rel_attr);
    ;}
    break;

  case 67:
#line 534 "yacc_sql.y"
    {
      (yyval.expression) = new StarExpr();
    ;}
    break;

  case 68:
#line 541 "yacc_sql.y"
    {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->attribute_name = (yyvsp[(1) - (1)].string);
      free((yyvsp[(1) - (1)].string));
    ;}
    break;

  case 69:
#line 546 "yacc_sql.y"
    {
      (yyval.rel_attr) = new RelAttrSqlNode;
      (yyval.rel_attr)->relation_name  = (yyvsp[(1) - (3)].string);
      (yyval.rel_attr)->attribute_name = (yyvsp[(3) - (3)].string);
      free((yyvsp[(1) - (3)].string));
      free((yyvsp[(3) - (3)].string));
    ;}
    break;

  case 70:
#line 556 "yacc_sql.y"
    {
      (yyval.string) = (yyvsp[(1) - (1)].string);
    ;}
    break;

  case 71:
#line 561 "yacc_sql.y"
    {
      (yyval.relation_list) = new std::vector<std::string>();
      (yyval.relation_list)->push_back((yyvsp[(1) - (1)].string));
      free((yyvsp[(1) - (1)].string));
    ;}
    break;

  case 72:
#line 566 "yacc_sql.y"
    {
      if ((yyvsp[(3) - (3)].relation_list) != nullptr) {
        (yyval.relation_list) = (yyvsp[(3) - (3)].relation_list);
      } else {
        (yyval.relation_list) = new std::vector<std::string>;
      }

      (yyval.relation_list)->insert((yyval.relation_list)->begin(), (yyvsp[(1) - (3)].string));
      free((yyvsp[(1) - (3)].string));
    ;}
    break;

  case 73:
#line 580 "yacc_sql.y"
    {
      (yyval.condition_list) = nullptr;
    ;}
    break;

  case 74:
#line 583 "yacc_sql.y"
    {
      (yyval.condition_list) = (yyvsp[(2) - (2)].condition_list);  
    ;}
    break;

  case 75:
#line 589 "yacc_sql.y"
    {
      (yyval.condition_list) = nullptr;
    ;}
    break;

  case 76:
#line 592 "yacc_sql.y"
    {
      (yyval.condition_list) = new std::vector<ConditionSqlNode>;
      (yyval.condition_list)->emplace_back(*(yyvsp[(1) - (1)].condition));
      delete (yyvsp[(1) - (1)].condition);
    ;}
    break;

  case 77:
#line 597 "yacc_sql.y"
    {
      (yyval.condition_list) = (yyvsp[(3) - (3)].condition_list);
      (yyval.condition_list)->emplace_back(*(yyvsp[(1) - (3)].condition));
      delete (yyvsp[(1) - (3)].condition);
    ;}
    break;

  case 78:
#line 605 "yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 1;
      (yyval.condition)->left_attr = *(yyvsp[(1) - (3)].rel_attr);
      (yyval.condition)->right_is_attr = 0;
      (yyval.condition)->right_value = *(yyvsp[(3) - (3)].value);
      (yyval.condition)->comp = (yyvsp[(2) - (3)].comp);

      delete (yyvsp[(1) - (3)].rel_attr);
      delete (yyvsp[(3) - (3)].value);
    ;}
    break;

  case 79:
#line 617 "yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 0;
      (yyval.condition)->left_value = *(yyvsp[(1) - (3)].value);
      (yyval.condition)->right_is_attr = 0;
      (yyval.condition)->right_value = *(yyvsp[(3) - (3)].value);
      (yyval.condition)->comp = (yyvsp[(2) - (3)].comp);

      delete (yyvsp[(1) - (3)].value);
      delete (yyvsp[(3) - (3)].value);
    ;}
    break;

  case 80:
#line 629 "yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 1;
      (yyval.condition)->left_attr = *(yyvsp[(1) - (3)].rel_attr);
      (yyval.condition)->right_is_attr = 1;
      (yyval.condition)->right_attr = *(yyvsp[(3) - (3)].rel_attr);
      (yyval.condition)->comp = (yyvsp[(2) - (3)].comp);

      delete (yyvsp[(1) - (3)].rel_attr);
      delete (yyvsp[(3) - (3)].rel_attr);
    ;}
    break;

  case 81:
#line 641 "yacc_sql.y"
    {
      (yyval.condition) = new ConditionSqlNode;
      (yyval.condition)->left_is_attr = 0;
      (yyval.condition)->left_value = *(yyvsp[(1) - (3)].value);
      (yyval.condition)->right_is_attr = 1;
      (yyval.condition)->right_attr = *(yyvsp[(3) - (3)].rel_attr);
      (yyval.condition)->comp = (yyvsp[(2) - (3)].comp);

      delete (yyvsp[(1) - (3)].value);
      delete (yyvsp[(3) - (3)].rel_attr);
    ;}
    break;

  case 82:
#line 655 "yacc_sql.y"
    { (yyval.comp) = EQUAL_TO; ;}
    break;

  case 83:
#line 656 "yacc_sql.y"
    { (yyval.comp) = LESS_THAN; ;}
    break;

  case 84:
#line 657 "yacc_sql.y"
    { (yyval.comp) = GREAT_THAN; ;}
    break;

  case 85:
#line 658 "yacc_sql.y"
    { (yyval.comp) = LESS_EQUAL; ;}
    break;

  case 86:
#line 659 "yacc_sql.y"
    { (yyval.comp) = GREAT_EQUAL; ;}
    break;

  case 87:
#line 660 "yacc_sql.y"
    { (yyval.comp) = NOT_EQUAL; ;}
    break;

  case 88:
#line 666 "yacc_sql.y"
    {
      (yyval.expression_list) = nullptr;
    ;}
    break;

  case 89:
#line 672 "yacc_sql.y"
    {
      char *tmp_file_name = common::substr((yyvsp[(4) - (7)].string), 1, strlen((yyvsp[(4) - (7)].string)) - 2);
      
      (yyval.sql_node) = new ParsedSqlNode(SCF_LOAD_DATA);
      (yyval.sql_node)->load_data.relation_name = (yyvsp[(7) - (7)].string);
      (yyval.sql_node)->load_data.file_name = tmp_file_name;
      free((yyvsp[(7) - (7)].string));
      free(tmp_file_name);
    ;}
    break;

  case 90:
#line 685 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_EXPLAIN);
      (yyval.sql_node)->explain.sql_node = std::unique_ptr<ParsedSqlNode>((yyvsp[(2) - (2)].sql_node));
    ;}
    break;

  case 91:
#line 693 "yacc_sql.y"
    {
      (yyval.sql_node) = new ParsedSqlNode(SCF_SET_VARIABLE);
      (yyval.sql_node)->set_variable.name  = (yyvsp[(2) - (4)].string);
      (yyval.sql_node)->set_variable.value = *(yyvsp[(4) - (4)].value);
      free((yyvsp[(2) - (4)].string));
      delete (yyvsp[(4) - (4)].value);
    ;}
    break;


/* Line 1267 of yacc.c.  */
#line 2347 "yacc_sql.cpp"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (sql_string, sql_result, scanner, YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (sql_string, sql_result, scanner, yymsg);
	  }
	else
	  {
	    yyerror (sql_string, sql_result, scanner, YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }

  yyerror_range[0] = yylloc;

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  yyerror_range[0] = yylsp[1-yylen];
  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      yyerror_range[0] = *yylsp;
      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;

  yyerror_range[1] = yylloc;
  /* Using YYLLOC is tempting, but would change the location of
     the look-ahead.  YYLOC is available though.  */
  YYLLOC_DEFAULT (yyloc, (yyerror_range - 1), 2);
  *++yylsp = yyloc;

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
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

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (sql_string, sql_result, scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval, &yylloc, sql_string, sql_result, scanner);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, yylsp, sql_string, sql_result, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 705 "yacc_sql.y"

//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, ParsedSqlResult *sql_result) {
  yyscan_t scanner;
  yylex_init(&scanner);
  scan_string(s, scanner);
  int result = yyparse(s, sql_result, scanner);
  yylex_destroy(scanner);
  return result;
}

