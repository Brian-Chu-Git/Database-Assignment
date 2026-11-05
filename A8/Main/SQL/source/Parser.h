/* A Bison parser, made by GNU Bison 3.7.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_STORAGE_HOME_B_BC116_A8_MAIN_SQL_SOURCE_PARSER_H_INCLUDED
# define YY_YY_STORAGE_HOME_B_BC116_A8_MAIN_SQL_SOURCE_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INTEGER = 258,                 /* INTEGER  */
    IDENTIFIER = 259,              /* IDENTIFIER  */
    DBL = 260,                     /* DBL  */
    STR = 261,                     /* STR  */
    SELECT = 262,                  /* SELECT  */
    FROM = 263,                    /* FROM  */
    WHERE = 264,                   /* WHERE  */
    AS = 265,                      /* AS  */
    BY = 266,                      /* BY  */
    AND = 267,                     /* AND  */
    OR = 268,                      /* OR  */
    NOT = 269,                     /* NOT  */
    SUM = 270,                     /* SUM  */
    AVG = 271,                     /* AVG  */
    GROUP = 272,                   /* GROUP  */
    INT = 273,                     /* INT  */
    BOOL = 274,                    /* BOOL  */
    BPLUSTREE = 275,               /* BPLUSTREE  */
    CREATE = 276,                  /* CREATE  */
    DOUBLE = 277,                  /* DOUBLE  */
    STRING = 278,                  /* STRING  */
    ON = 279,                      /* ON  */
    TABLE = 280                    /* TABLE  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 13 "/storage-home/b/bc116/A8/Main/SQL/source/Parser.y"

	struct SQLStatement *myStatement;
	struct SFWQuery *mySelectQuery;
	struct CreateTable *myCreateTable;
	struct FromList *myFromList;
	struct AttList *myAttList;
	struct Value *myValue;
	struct ValueList *allValues;
	struct CNF *myCNF;	
	int myInt;
	char *myChar;
	double myDouble;

#line 103 "/storage-home/b/bc116/A8/Main/SQL/source/Parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int yyparse (void *scanner, struct SQLStatement **myStatement);

#endif /* !YY_YY_STORAGE_HOME_B_BC116_A8_MAIN_SQL_SOURCE_PARSER_H_INCLUDED  */
