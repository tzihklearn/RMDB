/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "yacc.y"

#include "ast.h"
#include "yacc.tab.h"
#include <iostream>
#include <memory>

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc);

void yyerror(YYLTYPE *locp, const char* s) {
    std::cerr << "Parser Error at line " << locp->first_line << " column " << locp->first_column << ": " << s << std::endl;
}

using namespace ast;

#line 86 "yacc.tab.cpp"

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

#include "yacc.tab.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SHOW = 3,                       /* SHOW  */
  YYSYMBOL_TABLES = 4,                     /* TABLES  */
  YYSYMBOL_CREATE = 5,                     /* CREATE  */
  YYSYMBOL_TABLE = 6,                      /* TABLE  */
  YYSYMBOL_DROP = 7,                       /* DROP  */
  YYSYMBOL_DESC = 8,                       /* DESC  */
  YYSYMBOL_INSERT = 9,                     /* INSERT  */
  YYSYMBOL_INTO = 10,                      /* INTO  */
  YYSYMBOL_VALUES = 11,                    /* VALUES  */
  YYSYMBOL_DELETE = 12,                    /* DELETE  */
  YYSYMBOL_FROM = 13,                      /* FROM  */
  YYSYMBOL_ASC = 14,                       /* ASC  */
  YYSYMBOL_ORDER = 15,                     /* ORDER  */
  YYSYMBOL_BY = 16,                        /* BY  */
  YYSYMBOL_AS = 17,                        /* AS  */
  YYSYMBOL_GROUP = 18,                     /* GROUP  */
  YYSYMBOL_WHERE = 19,                     /* WHERE  */
  YYSYMBOL_UPDATE = 20,                    /* UPDATE  */
  YYSYMBOL_SET = 21,                       /* SET  */
  YYSYMBOL_SELECT = 22,                    /* SELECT  */
  YYSYMBOL_INT = 23,                       /* INT  */
  YYSYMBOL_CHAR = 24,                      /* CHAR  */
  YYSYMBOL_FLOAT = 25,                     /* FLOAT  */
  YYSYMBOL_DATETIME = 26,                  /* DATETIME  */
  YYSYMBOL_INDEX = 27,                     /* INDEX  */
  YYSYMBOL_AND = 28,                       /* AND  */
  YYSYMBOL_JOIN = 29,                      /* JOIN  */
  YYSYMBOL_EXIT = 30,                      /* EXIT  */
  YYSYMBOL_HELP = 31,                      /* HELP  */
  YYSYMBOL_TXN_BEGIN = 32,                 /* TXN_BEGIN  */
  YYSYMBOL_TXN_COMMIT = 33,                /* TXN_COMMIT  */
  YYSYMBOL_TXN_ABORT = 34,                 /* TXN_ABORT  */
  YYSYMBOL_TXN_ROLLBACK = 35,              /* TXN_ROLLBACK  */
  YYSYMBOL_ORDER_BY = 36,                  /* ORDER_BY  */
  YYSYMBOL_ENABLE_NESTLOOP = 37,           /* ENABLE_NESTLOOP  */
  YYSYMBOL_ENABLE_SORTMERGE = 38,          /* ENABLE_SORTMERGE  */
  YYSYMBOL_GROUP_BY = 39,                  /* GROUP_BY  */
  YYSYMBOL_HAVING = 40,                    /* HAVING  */
  YYSYMBOL_IN = 41,                        /* IN  */
  YYSYMBOL_STATIC_CHECKPOINT = 42,         /* STATIC_CHECKPOINT  */
  YYSYMBOL_LOAD = 43,                      /* LOAD  */
  YYSYMBOL_LEQ = 44,                       /* LEQ  */
  YYSYMBOL_NEQ = 45,                       /* NEQ  */
  YYSYMBOL_GEQ = 46,                       /* GEQ  */
  YYSYMBOL_T_EOF = 47,                     /* T_EOF  */
  YYSYMBOL_COUNT = 48,                     /* COUNT  */
  YYSYMBOL_MAX = 49,                       /* MAX  */
  YYSYMBOL_MIN = 50,                       /* MIN  */
  YYSYMBOL_SUM = 51,                       /* SUM  */
  YYSYMBOL_IDENTIFIER = 52,                /* IDENTIFIER  */
  YYSYMBOL_VALUE_STRING = 53,              /* VALUE_STRING  */
  YYSYMBOL_VALUE_INT = 54,                 /* VALUE_INT  */
  YYSYMBOL_VALUE_FLOAT = 55,               /* VALUE_FLOAT  */
  YYSYMBOL_VALUE_BOOL = 56,                /* VALUE_BOOL  */
  YYSYMBOL_FILE_PATH_VALUE = 57,           /* FILE_PATH_VALUE  */
  YYSYMBOL_58_ = 58,                       /* ';'  */
  YYSYMBOL_59_ = 59,                       /* '='  */
  YYSYMBOL_60_ = 60,                       /* '('  */
  YYSYMBOL_61_ = 61,                       /* ')'  */
  YYSYMBOL_62_ = 62,                       /* ','  */
  YYSYMBOL_63_ = 63,                       /* '*'  */
  YYSYMBOL_64_ = 64,                       /* '.'  */
  YYSYMBOL_65_ = 65,                       /* '<'  */
  YYSYMBOL_66_ = 66,                       /* '>'  */
  YYSYMBOL_YYACCEPT = 67,                  /* $accept  */
  YYSYMBOL_start = 68,                     /* start  */
  YYSYMBOL_stmt = 69,                      /* stmt  */
  YYSYMBOL_txnStmt = 70,                   /* txnStmt  */
  YYSYMBOL_dbStmt = 71,                    /* dbStmt  */
  YYSYMBOL_setStmt = 72,                   /* setStmt  */
  YYSYMBOL_ddl = 73,                       /* ddl  */
  YYSYMBOL_dml = 74,                       /* dml  */
  YYSYMBOL_fieldList = 75,                 /* fieldList  */
  YYSYMBOL_colNameList = 76,               /* colNameList  */
  YYSYMBOL_field = 77,                     /* field  */
  YYSYMBOL_type = 78,                      /* type  */
  YYSYMBOL_valueList = 79,                 /* valueList  */
  YYSYMBOL_value = 80,                     /* value  */
  YYSYMBOL_condition = 81,                 /* condition  */
  YYSYMBOL_optWhereClause = 82,            /* optWhereClause  */
  YYSYMBOL_whereClause = 83,               /* whereClause  */
  YYSYMBOL_col = 84,                       /* col  */
  YYSYMBOL_colList = 85,                   /* colList  */
  YYSYMBOL_op = 86,                        /* op  */
  YYSYMBOL_expr = 87,                      /* expr  */
  YYSYMBOL_sub_select_stmt = 88,           /* sub_select_stmt  */
  YYSYMBOL_in_op_vlaue = 89,               /* in_op_vlaue  */
  YYSYMBOL_in_sub_query = 90,              /* in_sub_query  */
  YYSYMBOL_setClauses = 91,                /* setClauses  */
  YYSYMBOL_setClause = 92,                 /* setClause  */
  YYSYMBOL_selector = 93,                  /* selector  */
  YYSYMBOL_AGGREGATE_SUM = 94,             /* AGGREGATE_SUM  */
  YYSYMBOL_AGGREGATE_COUNT = 95,           /* AGGREGATE_COUNT  */
  YYSYMBOL_AGGREGATE_MAX = 96,             /* AGGREGATE_MAX  */
  YYSYMBOL_AGGREGATE_MIN = 97,             /* AGGREGATE_MIN  */
  YYSYMBOL_sv_group_by_col = 98,           /* sv_group_by_col  */
  YYSYMBOL_group_by_cols = 99,             /* group_by_cols  */
  YYSYMBOL_sv_group_by = 100,              /* sv_group_by  */
  YYSYMBOL_tableList = 101,                /* tableList  */
  YYSYMBOL_opt_order_clause = 102,         /* opt_order_clause  */
  YYSYMBOL_order_clause = 103,             /* order_clause  */
  YYSYMBOL_opt_asc_desc = 104,             /* opt_asc_desc  */
  YYSYMBOL_set_knob_type = 105,            /* set_knob_type  */
  YYSYMBOL_tbName = 106,                   /* tbName  */
  YYSYMBOL_colName = 107,                  /* colName  */
  YYSYMBOL_file_path = 108                 /* file_path  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
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


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

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
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
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
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  57
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   205

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  42
/* YYNRULES -- Number of rules.  */
#define YYNRULES  106
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  205

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   312


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      60,    61,    63,     2,    62,     2,    64,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    58,
      65,    59,    66,     2,     2,     2,     2,     2,     2,     2,
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
      55,    56,    57
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    74,    74,    79,    84,    89,    97,    98,    99,   100,
     101,   105,   109,   113,   117,   124,   129,   136,   143,   147,
     151,   155,   159,   163,   167,   175,   179,   183,   187,   194,
     198,   205,   209,   216,   223,   227,   231,   235,   242,   246,
     253,   257,   261,   265,   272,   276,   280,   287,   288,   295,
     299,   306,   310,   314,   318,   322,   326,   330,   334,   338,
     342,   346,   350,   354,   361,   365,   372,   376,   380,   384,
     388,   392,   399,   403,   407,   414,   421,   428,   432,   439,
     443,   450,   457,   461,   468,   475,   482,   489,   493,   500,
     504,   511,   512,   518,   522,   526,   533,   537,   541,   548,
     549,   550,   554,   555,   558,   560,   562
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SHOW", "TABLES",
  "CREATE", "TABLE", "DROP", "DESC", "INSERT", "INTO", "VALUES", "DELETE",
  "FROM", "ASC", "ORDER", "BY", "AS", "GROUP", "WHERE", "UPDATE", "SET",
  "SELECT", "INT", "CHAR", "FLOAT", "DATETIME", "INDEX", "AND", "JOIN",
  "EXIT", "HELP", "TXN_BEGIN", "TXN_COMMIT", "TXN_ABORT", "TXN_ROLLBACK",
  "ORDER_BY", "ENABLE_NESTLOOP", "ENABLE_SORTMERGE", "GROUP_BY", "HAVING",
  "IN", "STATIC_CHECKPOINT", "LOAD", "LEQ", "NEQ", "GEQ", "T_EOF", "COUNT",
  "MAX", "MIN", "SUM", "IDENTIFIER", "VALUE_STRING", "VALUE_INT",
  "VALUE_FLOAT", "VALUE_BOOL", "FILE_PATH_VALUE", "';'", "'='", "'('",
  "')'", "','", "'*'", "'.'", "'<'", "'>'", "$accept", "start", "stmt",
  "txnStmt", "dbStmt", "setStmt", "ddl", "dml", "fieldList", "colNameList",
  "field", "type", "valueList", "value", "condition", "optWhereClause",
  "whereClause", "col", "colList", "op", "expr", "sub_select_stmt",
  "in_op_vlaue", "in_sub_query", "setClauses", "setClause", "selector",
  "AGGREGATE_SUM", "AGGREGATE_COUNT", "AGGREGATE_MAX", "AGGREGATE_MIN",
  "sv_group_by_col", "group_by_cols", "sv_group_by", "tableList",
  "opt_order_clause", "order_clause", "opt_asc_desc", "set_knob_type",
  "tbName", "colName", "file_path", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-106)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-105)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      92,    12,    15,     9,   -43,    13,    24,   -43,     3,   116,
    -106,  -106,  -106,  -106,  -106,  -106,   -31,  -106,    34,    -5,
    -106,  -106,  -106,  -106,  -106,  -106,    42,   -43,   -43,  -106,
     -43,   -43,  -106,  -106,   -43,   -43,    43,  -106,  -106,     6,
    -106,  -106,  -106,  -106,    18,  -106,  -106,     8,    58,    27,
      29,    36,    38,    39,  -106,  -106,    95,  -106,  -106,   -43,
      46,    47,  -106,    50,    91,   102,    59,    76,   116,   -43,
      59,   -30,    59,    59,    59,   -43,  -106,    59,    59,    59,
      73,   116,  -106,  -106,   -11,  -106,    77,  -106,  -106,     1,
    -106,    79,    80,    90,    99,   108,  -106,  -106,   -12,  -106,
      20,     0,  -106,     7,    75,  -106,   109,   117,    59,  -106,
      75,   -43,   -43,   123,   135,   138,   139,   153,   154,  -106,
      59,  -106,   112,  -106,  -106,  -106,  -106,    59,  -106,  -106,
    -106,  -106,  -106,    22,  -106,   116,   113,  -106,  -106,  -106,
    -106,  -106,  -106,    25,  -106,  -106,  -106,  -106,   158,   157,
      59,    59,    59,    59,    59,  -106,   124,  -106,  -106,    75,
    -106,    64,   116,    94,  -106,  -106,  -106,  -106,   116,   164,
    -106,  -106,  -106,  -106,  -106,  -106,   120,  -106,   122,  -106,
    -106,   126,   -10,   127,    21,  -106,   116,  -106,  -106,   -43,
    -106,  -106,  -106,  -106,   145,  -106,   129,     1,   116,   116,
     123,   109,  -106,   157,  -106
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       4,     3,    11,    12,    13,    14,     0,     5,     0,     0,
       9,     6,    10,     7,     8,    15,     0,     0,     0,    23,
       0,     0,   104,    20,     0,     0,     0,   102,   103,     0,
      84,    85,    86,    83,   105,    51,    64,    82,     0,     0,
       0,     0,     0,     0,    53,   106,     0,     1,     2,     0,
       0,     0,    19,     0,     0,    47,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    16,     0,     0,     0,
       0,     0,    26,   105,    47,    79,     0,    17,    65,    47,
      93,     0,     0,     0,     0,     0,    52,    24,     0,    29,
       0,     0,    31,     0,     0,    49,    48,     0,     0,    27,
       0,     0,     0,    97,    59,    63,    62,    61,    60,    18,
       0,    34,     0,    36,    37,    33,    21,     0,    22,    42,
      40,    41,    43,     0,    38,     0,     0,    70,    69,    71,
      66,    67,    68,     0,    80,    81,    95,    94,     0,    91,
       0,     0,     0,     0,     0,    30,     0,    32,    25,     0,
      50,     0,     0,     0,    72,    73,    44,    74,     0,     0,
      28,    54,    58,    57,    56,    55,     0,    39,    76,    77,
      78,     0,     0,     0,   101,    96,     0,    35,    46,     0,
      45,   100,    99,    98,    87,    89,    92,    47,     0,     0,
      97,    88,    90,    91,    75
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,  -106,   110,
      72,  -106,    32,  -105,    60,   -82,    -4,    -9,    35,  -106,
      33,    37,  -106,  -106,  -106,    93,  -106,  -106,  -106,  -106,
    -106,     4,  -106,    -1,    10,     5,  -106,  -106,  -106,    -3,
     -60,  -106
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,    18,    19,    20,    21,    22,    23,    24,    98,   101,
      99,   125,   133,   134,   105,    82,   106,   107,    47,   143,
     166,   167,   180,   181,    84,    85,    48,    49,    50,    51,
      52,   195,   196,   170,    89,   149,   185,   193,    39,    53,
      54,    56
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      46,    33,   109,   189,    36,   145,    86,   113,    81,    32,
      91,    93,    94,    95,    96,    30,    25,   100,   102,   102,
      81,    27,    83,    34,    60,    61,    55,    62,    63,   191,
     111,    64,    65,    92,    57,   192,    31,    35,   164,    26,
      37,    38,    28,   121,   122,   123,   124,   162,    86,   119,
     120,   108,    68,    58,   177,    59,    76,    29,   164,    88,
     100,   126,   127,   112,    66,    67,    90,   157,   128,   127,
      68,    69,    97,    40,    41,    42,    43,    44,   129,   130,
     131,   132,  -104,   158,   159,   163,   162,    70,    45,    71,
     171,   172,   173,   174,   175,     1,    72,     2,    73,     3,
       4,     5,    80,    74,     6,    75,    77,    78,   146,   147,
      79,    83,     7,     8,     9,   200,   162,   129,   130,   131,
     132,    81,    10,    11,    12,    13,    14,    15,   129,   130,
     131,   132,    87,   104,   165,    16,   110,   135,   148,    17,
     114,   115,    40,    41,    42,    43,    44,   129,   130,   131,
     132,   116,   150,    46,   165,   151,   152,    45,   136,   184,
     117,   137,   138,   139,    40,    41,    42,    43,    44,   118,
     153,   154,   156,   161,   168,   169,   140,   194,   176,    45,
     186,   187,   141,   142,   159,   198,    90,   188,   190,   103,
     194,   199,   155,   178,   201,   160,   183,   182,   179,   197,
       0,   144,   204,   202,     0,   203
};

static const yytype_int16 yycheck[] =
{
       9,     4,    84,    13,     7,   110,    66,    89,    19,    52,
      70,    71,    72,    73,    74,     6,     4,    77,    78,    79,
      19,     6,    52,    10,    27,    28,    57,    30,    31,     8,
      29,    34,    35,    63,     0,    14,    27,    13,   143,    27,
      37,    38,    27,    23,    24,    25,    26,    22,   108,    61,
      62,    62,    62,    58,   159,    13,    59,    42,   163,    68,
     120,    61,    62,    62,    21,    59,    69,   127,    61,    62,
      62,    13,    75,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    64,    61,    62,    60,    22,    60,    63,    60,
     150,   151,   152,   153,   154,     3,    60,     5,    60,     7,
       8,     9,    11,    64,    12,    10,    60,    60,   111,   112,
      60,    52,    20,    21,    22,   197,    22,    53,    54,    55,
      56,    19,    30,    31,    32,    33,    34,    35,    53,    54,
      55,    56,    56,    60,   143,    43,    59,    28,    15,    47,
      61,    61,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    61,    17,   162,   163,    17,    17,    63,    41,   168,
      61,    44,    45,    46,    48,    49,    50,    51,    52,    61,
      17,    17,    60,    60,    16,    18,    59,   186,    54,    63,
      16,    61,    65,    66,    62,    40,   189,    61,    61,    79,
     199,    62,   120,   161,   198,   135,   163,   162,   161,   189,
      -1,   108,   203,   199,    -1,   200
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     5,     7,     8,     9,    12,    20,    21,    22,
      30,    31,    32,    33,    34,    35,    43,    47,    68,    69,
      70,    71,    72,    73,    74,     4,    27,     6,    27,    42,
       6,    27,    52,   106,    10,    13,   106,    37,    38,   105,
      48,    49,    50,    51,    52,    63,    84,    85,    93,    94,
      95,    96,    97,   106,   107,    57,   108,     0,    58,    13,
     106,   106,   106,   106,   106,   106,    21,    59,    62,    13,
      60,    60,    60,    60,    64,    10,   106,    60,    60,    60,
      11,    19,    82,    52,    91,    92,   107,    56,    84,   101,
     106,   107,    63,   107,   107,   107,   107,   106,    75,    77,
     107,    76,   107,    76,    60,    81,    83,    84,    62,    82,
      59,    29,    62,    82,    61,    61,    61,    61,    61,    61,
      62,    23,    24,    25,    26,    78,    61,    62,    61,    53,
      54,    55,    56,    79,    80,    28,    41,    44,    45,    46,
      59,    65,    66,    86,    92,    80,   106,   106,    15,   102,
      17,    17,    17,    17,    17,    77,    60,   107,    61,    62,
      81,    60,    22,    60,    80,    84,    87,    88,    16,    18,
     100,   107,   107,   107,   107,   107,    54,    80,    79,    88,
      89,    90,    85,    87,    84,   103,    16,    61,    61,    13,
      61,     8,    14,   104,    84,    98,    99,   101,    40,    62,
      82,    83,    98,   102,   100
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    67,    68,    68,    68,    68,    69,    69,    69,    69,
      69,    70,    70,    70,    70,    71,    71,    72,    73,    73,
      73,    73,    73,    73,    73,    74,    74,    74,    74,    75,
      75,    76,    76,    77,    78,    78,    78,    78,    79,    79,
      80,    80,    80,    80,    81,    81,    81,    82,    82,    83,
      83,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    85,    85,    86,    86,    86,    86,
      86,    86,    87,    87,    87,    88,    89,    90,    90,    91,
      91,    92,    93,    94,    95,    96,    97,    98,    98,    99,
      99,   100,   100,   101,   101,   101,   102,   102,   103,   104,
     104,   104,   105,   105,   106,   107,   108
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     4,     4,     6,     3,
       2,     6,     6,     2,     4,     7,     4,     5,     7,     1,
       3,     1,     3,     2,     1,     4,     1,     1,     1,     3,
       1,     1,     1,     1,     3,     5,     5,     0,     2,     1,
       3,     1,     3,     1,     6,     6,     6,     6,     6,     4,
       4,     4,     4,     4,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     7,     1,     1,     1,     1,
       3,     3,     1,     1,     1,     1,     1,     1,     3,     1,
       3,     0,     3,     1,     3,     3,     3,     0,     2,     1,
       1,     0,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
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


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
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
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* start: stmt ';'  */
#line 75 "yacc.y"
    {
        parse_tree = (yyvsp[-1].sv_node);
        YYACCEPT;
    }
#line 1725 "yacc.tab.cpp"
    break;

  case 3: /* start: HELP  */
#line 80 "yacc.y"
    {
        parse_tree = std::make_shared<Help>();
        YYACCEPT;
    }
#line 1734 "yacc.tab.cpp"
    break;

  case 4: /* start: EXIT  */
#line 85 "yacc.y"
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
#line 1743 "yacc.tab.cpp"
    break;

  case 5: /* start: T_EOF  */
#line 90 "yacc.y"
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
#line 1752 "yacc.tab.cpp"
    break;

  case 11: /* txnStmt: TXN_BEGIN  */
#line 106 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<TxnBegin>();
    }
#line 1760 "yacc.tab.cpp"
    break;

  case 12: /* txnStmt: TXN_COMMIT  */
#line 110 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<TxnCommit>();
    }
#line 1768 "yacc.tab.cpp"
    break;

  case 13: /* txnStmt: TXN_ABORT  */
#line 114 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<TxnAbort>();
    }
#line 1776 "yacc.tab.cpp"
    break;

  case 14: /* txnStmt: TXN_ROLLBACK  */
#line 118 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<TxnRollback>();
    }
#line 1784 "yacc.tab.cpp"
    break;

  case 15: /* dbStmt: SHOW TABLES  */
#line 125 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<ShowTables>();
    }
#line 1792 "yacc.tab.cpp"
    break;

  case 16: /* dbStmt: SHOW INDEX FROM tbName  */
#line 130 "yacc.y"
    {
	(yyval.sv_node) = std::make_shared<ShowIndex>((yyvsp[0].sv_str));
    }
#line 1800 "yacc.tab.cpp"
    break;

  case 17: /* setStmt: SET set_knob_type '=' VALUE_BOOL  */
#line 137 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<SetStmt>((yyvsp[-2].sv_setKnobType), (yyvsp[0].sv_bool));
    }
#line 1808 "yacc.tab.cpp"
    break;

  case 18: /* ddl: CREATE TABLE tbName '(' fieldList ')'  */
#line 144 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<CreateTable>((yyvsp[-3].sv_str), (yyvsp[-1].sv_fields));
    }
#line 1816 "yacc.tab.cpp"
    break;

  case 19: /* ddl: DROP TABLE tbName  */
#line 148 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<DropTable>((yyvsp[0].sv_str));
    }
#line 1824 "yacc.tab.cpp"
    break;

  case 20: /* ddl: DESC tbName  */
#line 152 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<DescTable>((yyvsp[0].sv_str));
    }
#line 1832 "yacc.tab.cpp"
    break;

  case 21: /* ddl: CREATE INDEX tbName '(' colNameList ')'  */
#line 156 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<CreateIndex>((yyvsp[-3].sv_str), (yyvsp[-1].sv_strs));
    }
#line 1840 "yacc.tab.cpp"
    break;

  case 22: /* ddl: DROP INDEX tbName '(' colNameList ')'  */
#line 160 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<DropIndex>((yyvsp[-3].sv_str), (yyvsp[-1].sv_strs));
    }
#line 1848 "yacc.tab.cpp"
    break;

  case 23: /* ddl: CREATE STATIC_CHECKPOINT  */
#line 164 "yacc.y"
    {
    	(yyval.sv_node) = std::make_shared<StaticCheckpoint>();
    }
#line 1856 "yacc.tab.cpp"
    break;

  case 24: /* ddl: LOAD file_path INTO tbName  */
#line 168 "yacc.y"
    {
         (yyval.sv_node) = std::make_shared<LoadStmt>((yyvsp[-2].sv_str), (yyvsp[0].sv_str));
         std::cout << "Parsed file path: " << (yyvsp[-2].sv_str) << std::endl;
    }
#line 1865 "yacc.tab.cpp"
    break;

  case 25: /* dml: INSERT INTO tbName VALUES '(' valueList ')'  */
#line 176 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<InsertStmt>((yyvsp[-4].sv_str), (yyvsp[-1].sv_vals));
    }
#line 1873 "yacc.tab.cpp"
    break;

  case 26: /* dml: DELETE FROM tbName optWhereClause  */
#line 180 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<DeleteStmt>((yyvsp[-1].sv_str), (yyvsp[0].sv_conds));
    }
#line 1881 "yacc.tab.cpp"
    break;

  case 27: /* dml: UPDATE tbName SET setClauses optWhereClause  */
#line 184 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<UpdateStmt>((yyvsp[-3].sv_str), (yyvsp[-1].sv_set_clauses), (yyvsp[0].sv_conds));
    }
#line 1889 "yacc.tab.cpp"
    break;

  case 28: /* dml: SELECT selector FROM tableList optWhereClause opt_order_clause sv_group_by  */
#line 188 "yacc.y"
    {
        (yyval.sv_node) = std::make_shared<SelectStmt>((yyvsp[-5].sv_cols), (yyvsp[-3].sv_strs), (yyvsp[-2].sv_conds), (yyvsp[-1].sv_orderby), (yyvsp[0].sv_group_by_cols));
    }
#line 1897 "yacc.tab.cpp"
    break;

  case 29: /* fieldList: field  */
#line 195 "yacc.y"
    {
        (yyval.sv_fields) = std::vector<std::shared_ptr<Field>>{(yyvsp[0].sv_field)};
    }
#line 1905 "yacc.tab.cpp"
    break;

  case 30: /* fieldList: fieldList ',' field  */
#line 199 "yacc.y"
    {
        (yyval.sv_fields).push_back((yyvsp[0].sv_field));
    }
#line 1913 "yacc.tab.cpp"
    break;

  case 31: /* colNameList: colName  */
#line 206 "yacc.y"
    {
        (yyval.sv_strs) = std::vector<std::string>{(yyvsp[0].sv_str)};
    }
#line 1921 "yacc.tab.cpp"
    break;

  case 32: /* colNameList: colNameList ',' colName  */
#line 210 "yacc.y"
    {
        (yyval.sv_strs).push_back((yyvsp[0].sv_str));
    }
#line 1929 "yacc.tab.cpp"
    break;

  case 33: /* field: colName type  */
#line 217 "yacc.y"
    {
        (yyval.sv_field) = std::make_shared<ColDef>((yyvsp[-1].sv_str), (yyvsp[0].sv_type_len));
    }
#line 1937 "yacc.tab.cpp"
    break;

  case 34: /* type: INT  */
#line 224 "yacc.y"
    {
        (yyval.sv_type_len) = std::make_shared<TypeLen>(SV_TYPE_INT, sizeof(int));
    }
#line 1945 "yacc.tab.cpp"
    break;

  case 35: /* type: CHAR '(' VALUE_INT ')'  */
#line 228 "yacc.y"
    {
        (yyval.sv_type_len) = std::make_shared<TypeLen>(SV_TYPE_STRING, (yyvsp[-1].sv_int));
    }
#line 1953 "yacc.tab.cpp"
    break;

  case 36: /* type: FLOAT  */
#line 232 "yacc.y"
    {
        (yyval.sv_type_len) = std::make_shared<TypeLen>(SV_TYPE_FLOAT, sizeof(float));
    }
#line 1961 "yacc.tab.cpp"
    break;

  case 37: /* type: DATETIME  */
#line 236 "yacc.y"
    {
        (yyval.sv_type_len) = std::make_shared<TypeLen>(SV_TYPE_STRING, 30);
    }
#line 1969 "yacc.tab.cpp"
    break;

  case 38: /* valueList: value  */
#line 243 "yacc.y"
    {
        (yyval.sv_vals) = std::vector<std::shared_ptr<Value>>{(yyvsp[0].sv_val)};
    }
#line 1977 "yacc.tab.cpp"
    break;

  case 39: /* valueList: valueList ',' value  */
#line 247 "yacc.y"
    {
        (yyval.sv_vals).push_back((yyvsp[0].sv_val));
    }
#line 1985 "yacc.tab.cpp"
    break;

  case 40: /* value: VALUE_INT  */
#line 254 "yacc.y"
    {
        (yyval.sv_val) = std::make_shared<IntLit>((yyvsp[0].sv_int));
    }
#line 1993 "yacc.tab.cpp"
    break;

  case 41: /* value: VALUE_FLOAT  */
#line 258 "yacc.y"
    {
        (yyval.sv_val) = std::make_shared<FloatLit>((yyvsp[0].sv_float));
    }
#line 2001 "yacc.tab.cpp"
    break;

  case 42: /* value: VALUE_STRING  */
#line 262 "yacc.y"
    {
        (yyval.sv_val) = std::make_shared<StringLit>((yyvsp[0].sv_str));
    }
#line 2009 "yacc.tab.cpp"
    break;

  case 43: /* value: VALUE_BOOL  */
#line 266 "yacc.y"
    {
        (yyval.sv_val) = std::make_shared<BoolLit>((yyvsp[0].sv_bool));
    }
#line 2017 "yacc.tab.cpp"
    break;

  case 44: /* condition: col op expr  */
#line 273 "yacc.y"
    {
        (yyval.sv_cond) = std::make_shared<BinaryExpr>((yyvsp[-2].sv_col), (yyvsp[-1].sv_comp_op), (yyvsp[0].sv_expr));
    }
#line 2025 "yacc.tab.cpp"
    break;

  case 45: /* condition: col op '(' expr ')'  */
#line 277 "yacc.y"
    {
    	(yyval.sv_cond) = std::make_shared<BinaryExpr>((yyvsp[-4].sv_col), (yyvsp[-3].sv_comp_op), (yyvsp[-1].sv_expr));
    }
#line 2033 "yacc.tab.cpp"
    break;

  case 46: /* condition: col IN '(' in_sub_query ')'  */
#line 281 "yacc.y"
    {
    	(yyval.sv_cond) = std::make_shared<BinaryExpr>((yyvsp[-4].sv_col), SV_OP_IN, (yyvsp[-1].in_sub_query));
    }
#line 2041 "yacc.tab.cpp"
    break;

  case 47: /* optWhereClause: %empty  */
#line 287 "yacc.y"
                      { /* ignore*/ }
#line 2047 "yacc.tab.cpp"
    break;

  case 48: /* optWhereClause: WHERE whereClause  */
#line 289 "yacc.y"
    {
        (yyval.sv_conds) = (yyvsp[0].sv_conds);
    }
#line 2055 "yacc.tab.cpp"
    break;

  case 49: /* whereClause: condition  */
#line 296 "yacc.y"
    {
        (yyval.sv_conds) = std::vector<std::shared_ptr<BinaryExpr>>{(yyvsp[0].sv_cond)};
    }
#line 2063 "yacc.tab.cpp"
    break;

  case 50: /* whereClause: whereClause AND condition  */
#line 300 "yacc.y"
    {
        (yyval.sv_conds).push_back((yyvsp[0].sv_cond));
    }
#line 2071 "yacc.tab.cpp"
    break;

  case 51: /* col: '*'  */
#line 307 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", "", SV_AGGREGATE_NULL, "");
    }
#line 2079 "yacc.tab.cpp"
    break;

  case 52: /* col: tbName '.' colName  */
#line 311 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>((yyvsp[-2].sv_str), (yyvsp[0].sv_str), SV_AGGREGATE_NULL, "");
    }
#line 2087 "yacc.tab.cpp"
    break;

  case 53: /* col: colName  */
#line 315 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[0].sv_str), SV_AGGREGATE_NULL, "");
    }
#line 2095 "yacc.tab.cpp"
    break;

  case 54: /* col: AGGREGATE_SUM '(' colName ')' AS colName  */
#line 319 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-3].sv_str), (yyvsp[-5].sv_aggregate_type), (yyvsp[0].sv_str));
    }
#line 2103 "yacc.tab.cpp"
    break;

  case 55: /* col: AGGREGATE_MIN '(' colName ')' AS colName  */
#line 323 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-3].sv_str), (yyvsp[-5].sv_aggregate_type), (yyvsp[0].sv_str));
    }
#line 2111 "yacc.tab.cpp"
    break;

  case 56: /* col: AGGREGATE_MAX '(' colName ')' AS colName  */
#line 327 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-3].sv_str), (yyvsp[-5].sv_aggregate_type), (yyvsp[0].sv_str));
    }
#line 2119 "yacc.tab.cpp"
    break;

  case 57: /* col: AGGREGATE_COUNT '(' colName ')' AS colName  */
#line 331 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-3].sv_str), (yyvsp[-5].sv_aggregate_type), (yyvsp[0].sv_str));
    }
#line 2127 "yacc.tab.cpp"
    break;

  case 58: /* col: AGGREGATE_COUNT '(' '*' ')' AS colName  */
#line 335 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", "", (yyvsp[-5].sv_aggregate_type), (yyvsp[0].sv_str));
    }
#line 2135 "yacc.tab.cpp"
    break;

  case 59: /* col: AGGREGATE_SUM '(' colName ')'  */
#line 339 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-1].sv_str), (yyvsp[-3].sv_aggregate_type), "");
    }
#line 2143 "yacc.tab.cpp"
    break;

  case 60: /* col: AGGREGATE_MIN '(' colName ')'  */
#line 343 "yacc.y"
    {
         (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-1].sv_str), (yyvsp[-3].sv_aggregate_type), "");
    }
#line 2151 "yacc.tab.cpp"
    break;

  case 61: /* col: AGGREGATE_MAX '(' colName ')'  */
#line 347 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-1].sv_str), (yyvsp[-3].sv_aggregate_type), "");
    }
#line 2159 "yacc.tab.cpp"
    break;

  case 62: /* col: AGGREGATE_COUNT '(' colName ')'  */
#line 351 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", (yyvsp[-1].sv_str), (yyvsp[-3].sv_aggregate_type), "");
    }
#line 2167 "yacc.tab.cpp"
    break;

  case 63: /* col: AGGREGATE_COUNT '(' '*' ')'  */
#line 355 "yacc.y"
    {
        (yyval.sv_col) = std::make_shared<Col>("", "", (yyvsp[-3].sv_aggregate_type), "");
    }
#line 2175 "yacc.tab.cpp"
    break;

  case 64: /* colList: col  */
#line 362 "yacc.y"
    {
        (yyval.sv_cols) = std::vector<std::shared_ptr<Col>>{(yyvsp[0].sv_col)};
    }
#line 2183 "yacc.tab.cpp"
    break;

  case 65: /* colList: colList ',' col  */
#line 366 "yacc.y"
    {
        (yyval.sv_cols).push_back((yyvsp[0].sv_col));
    }
#line 2191 "yacc.tab.cpp"
    break;

  case 66: /* op: '='  */
#line 373 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_EQ;
    }
#line 2199 "yacc.tab.cpp"
    break;

  case 67: /* op: '<'  */
#line 377 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_LT;
    }
#line 2207 "yacc.tab.cpp"
    break;

  case 68: /* op: '>'  */
#line 381 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_GT;
    }
#line 2215 "yacc.tab.cpp"
    break;

  case 69: /* op: NEQ  */
#line 385 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_NE;
    }
#line 2223 "yacc.tab.cpp"
    break;

  case 70: /* op: LEQ  */
#line 389 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_LE;
    }
#line 2231 "yacc.tab.cpp"
    break;

  case 71: /* op: GEQ  */
#line 393 "yacc.y"
    {
        (yyval.sv_comp_op) = SV_OP_GE;
    }
#line 2239 "yacc.tab.cpp"
    break;

  case 72: /* expr: value  */
#line 400 "yacc.y"
    {
        (yyval.sv_expr) = std::static_pointer_cast<Expr>((yyvsp[0].sv_val));
    }
#line 2247 "yacc.tab.cpp"
    break;

  case 73: /* expr: col  */
#line 404 "yacc.y"
    {
        (yyval.sv_expr) = std::static_pointer_cast<Expr>((yyvsp[0].sv_col));
    }
#line 2255 "yacc.tab.cpp"
    break;

  case 74: /* expr: sub_select_stmt  */
#line 408 "yacc.y"
    {
    	(yyval.sv_expr) = std::static_pointer_cast<Expr>((yyvsp[0].sub_select_stmt));
    }
#line 2263 "yacc.tab.cpp"
    break;

  case 75: /* sub_select_stmt: SELECT colList FROM tableList optWhereClause opt_order_clause sv_group_by  */
#line 415 "yacc.y"
    {
        (yyval.sub_select_stmt) = std::make_shared<SubSelectStmt>((yyvsp[-5].sv_cols), (yyvsp[-3].sv_strs), (yyvsp[-2].sv_conds), (yyvsp[-1].sv_orderby), (yyvsp[0].sv_group_by_cols));
    }
#line 2271 "yacc.tab.cpp"
    break;

  case 76: /* in_op_vlaue: valueList  */
#line 422 "yacc.y"
    {
        (yyval.in_op_value) = std::make_shared<InOpValue>((yyvsp[0].sv_vals));
    }
#line 2279 "yacc.tab.cpp"
    break;

  case 77: /* in_sub_query: sub_select_stmt  */
#line 429 "yacc.y"
    {
	(yyval.in_sub_query) = std::static_pointer_cast<Expr>((yyvsp[0].sub_select_stmt));
    }
#line 2287 "yacc.tab.cpp"
    break;

  case 78: /* in_sub_query: in_op_vlaue  */
#line 433 "yacc.y"
    {
    	(yyval.in_sub_query) = std::static_pointer_cast<Expr>((yyvsp[0].in_op_value));
    }
#line 2295 "yacc.tab.cpp"
    break;

  case 79: /* setClauses: setClause  */
#line 440 "yacc.y"
    {
        (yyval.sv_set_clauses) = std::vector<std::shared_ptr<SetClause>>{(yyvsp[0].sv_set_clause)};
    }
#line 2303 "yacc.tab.cpp"
    break;

  case 80: /* setClauses: setClauses ',' setClause  */
#line 444 "yacc.y"
    {
        (yyval.sv_set_clauses).push_back((yyvsp[0].sv_set_clause));
    }
#line 2311 "yacc.tab.cpp"
    break;

  case 81: /* setClause: colName '=' value  */
#line 451 "yacc.y"
    {
        (yyval.sv_set_clause) = std::make_shared<SetClause>((yyvsp[-2].sv_str), (yyvsp[0].sv_val));
    }
#line 2319 "yacc.tab.cpp"
    break;

  case 83: /* AGGREGATE_SUM: SUM  */
#line 462 "yacc.y"
    {
        (yyval.sv_aggregate_type) = SV_AGGREGATE_SUM;
    }
#line 2327 "yacc.tab.cpp"
    break;

  case 84: /* AGGREGATE_COUNT: COUNT  */
#line 469 "yacc.y"
    {
        (yyval.sv_aggregate_type) = SV_AGGREGATE_COUNT;
    }
#line 2335 "yacc.tab.cpp"
    break;

  case 85: /* AGGREGATE_MAX: MAX  */
#line 476 "yacc.y"
    {
        (yyval.sv_aggregate_type) = SV_AGGREGATE_MAX;
    }
#line 2343 "yacc.tab.cpp"
    break;

  case 86: /* AGGREGATE_MIN: MIN  */
#line 483 "yacc.y"
    {
        (yyval.sv_aggregate_type) = SV_AGGREGATE_MIN;
    }
#line 2351 "yacc.tab.cpp"
    break;

  case 87: /* sv_group_by_col: col  */
#line 490 "yacc.y"
    {
	(yyval.sv_group_by_col) = std::make_shared<GroupBy>((yyvsp[0].sv_col), std::vector<std::shared_ptr<BinaryExpr>>{});
    }
#line 2359 "yacc.tab.cpp"
    break;

  case 88: /* sv_group_by_col: col HAVING whereClause  */
#line 494 "yacc.y"
    {
	(yyval.sv_group_by_col) = std::make_shared<GroupBy>((yyvsp[-2].sv_col), (yyvsp[0].sv_conds));
    }
#line 2367 "yacc.tab.cpp"
    break;

  case 89: /* group_by_cols: sv_group_by_col  */
#line 501 "yacc.y"
    {
    	(yyval.sv_group_by_cols) = std::vector<std::shared_ptr<GroupBy>>{(yyvsp[0].sv_group_by_col)};
    }
#line 2375 "yacc.tab.cpp"
    break;

  case 90: /* group_by_cols: group_by_cols ',' sv_group_by_col  */
#line 505 "yacc.y"
    {
    	(yyval.sv_group_by_cols).push_back((yyvsp[0].sv_group_by_col));
    }
#line 2383 "yacc.tab.cpp"
    break;

  case 91: /* sv_group_by: %empty  */
#line 511 "yacc.y"
                      { /* ignore*/ }
#line 2389 "yacc.tab.cpp"
    break;

  case 92: /* sv_group_by: GROUP BY group_by_cols  */
#line 513 "yacc.y"
    {
    	(yyval.sv_group_by_cols) = (yyvsp[0].sv_group_by_cols);
    }
#line 2397 "yacc.tab.cpp"
    break;

  case 93: /* tableList: tbName  */
#line 519 "yacc.y"
    {
        (yyval.sv_strs) = std::vector<std::string>{(yyvsp[0].sv_str)};
    }
#line 2405 "yacc.tab.cpp"
    break;

  case 94: /* tableList: tableList ',' tbName  */
#line 523 "yacc.y"
    {
        (yyval.sv_strs).push_back((yyvsp[0].sv_str));
    }
#line 2413 "yacc.tab.cpp"
    break;

  case 95: /* tableList: tableList JOIN tbName  */
#line 527 "yacc.y"
    {
        (yyval.sv_strs).push_back((yyvsp[0].sv_str));
    }
#line 2421 "yacc.tab.cpp"
    break;

  case 96: /* opt_order_clause: ORDER BY order_clause  */
#line 534 "yacc.y"
    {
        (yyval.sv_orderby) = (yyvsp[0].sv_orderby);
    }
#line 2429 "yacc.tab.cpp"
    break;

  case 97: /* opt_order_clause: %empty  */
#line 537 "yacc.y"
                      { /* ignore*/ }
#line 2435 "yacc.tab.cpp"
    break;

  case 98: /* order_clause: col opt_asc_desc  */
#line 542 "yacc.y"
    {
        (yyval.sv_orderby) = std::make_shared<OrderBy>((yyvsp[-1].sv_col), (yyvsp[0].sv_orderby_dir));
    }
#line 2443 "yacc.tab.cpp"
    break;

  case 99: /* opt_asc_desc: ASC  */
#line 548 "yacc.y"
                 { (yyval.sv_orderby_dir) = OrderBy_ASC;     }
#line 2449 "yacc.tab.cpp"
    break;

  case 100: /* opt_asc_desc: DESC  */
#line 549 "yacc.y"
                 { (yyval.sv_orderby_dir) = OrderBy_DESC;    }
#line 2455 "yacc.tab.cpp"
    break;

  case 101: /* opt_asc_desc: %empty  */
#line 550 "yacc.y"
            { (yyval.sv_orderby_dir) = OrderBy_DEFAULT; }
#line 2461 "yacc.tab.cpp"
    break;

  case 102: /* set_knob_type: ENABLE_NESTLOOP  */
#line 554 "yacc.y"
                    { (yyval.sv_setKnobType) = EnableNestLoop; }
#line 2467 "yacc.tab.cpp"
    break;

  case 103: /* set_knob_type: ENABLE_SORTMERGE  */
#line 555 "yacc.y"
                         { (yyval.sv_setKnobType) = EnableSortMerge; }
#line 2473 "yacc.tab.cpp"
    break;


#line 2477 "yacc.tab.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
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
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
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
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 563 "yacc.y"

