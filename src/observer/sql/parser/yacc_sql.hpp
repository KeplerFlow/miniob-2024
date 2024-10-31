/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

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
/* Line 1529 of yacc.c.  */
#line 180 "yacc_sql.hpp"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

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

extern YYLTYPE yylloc;
