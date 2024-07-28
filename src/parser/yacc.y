%{
#include "ast.h"
#include "yacc.tab.h"
#include <iostream>
#include <memory>

int yylex(YYSTYPE *yylval, YYLTYPE *yylloc);

void yyerror(YYLTYPE *locp, const char* s) {
    std::cerr << "Parser Error at line " << locp->first_line << " column " << locp->first_column << ": " << s << std::endl;
}

using namespace ast;
%}

// request a pure (reentrant) parser
%define api.pure full
// enable location in error handler
%locations
// enable verbose syntax error message
%define parse.error verbose

// keywords
%token SHOW TABLES CREATE TABLE DROP DESC INSERT INTO VALUES DELETE FROM ASC ORDER BY AS GROUP
WHERE UPDATE SET SELECT INT CHAR FLOAT DATETIME INDEX AND JOIN EXIT HELP TXN_BEGIN TXN_COMMIT TXN_ABORT TXN_ROLLBACK ORDER_BY ENABLE_NESTLOOP ENABLE_SORTMERGE
GROUP_BY HAVING IN STATIC_CHECKPOINT LOAD

// non-keywords
%token LEQ NEQ GEQ T_EOF

%token COUNT MAX MIN SUM

// type-specific tokens
%token <sv_str> IDENTIFIER VALUE_STRING
%token <sv_int> VALUE_INT
%token <sv_float> VALUE_FLOAT
%token <sv_bool> VALUE_BOOL

%token <sv_str> FILE_PATH_VALUE

%token <sv_str> TABLE_COL


// specify types for non-terminal symbol
%type <sv_node> stmt dbStmt ddl dml txnStmt setStmt
%type <sv_field> field
%type <sv_fields> fieldList
%type <sv_type_len> type
%type <sv_comp_op> op
%type <sv_expr> expr
%type <sv_val> value
%type <sv_vals> valueList
%type <sv_str> tbName colName
%type <sv_strs> tableList colNameList
%type <sv_col> col
%type <sv_cols> colList selector
%type <sv_set_clause> setClause
%type <sv_set_clauses> setClauses
%type <sv_cond> condition
%type <sv_conds> whereClause optWhereClause
%type <sv_orderby>  order_clause opt_order_clause
%type <sv_orderby_dir> opt_asc_desc
%type <sv_setKnobType> set_knob_type

%type <sv_aggregate_type> AGGREGATE_SUM AGGREGATE_COUNT AGGREGATE_MAX AGGREGATE_MIN
%type <sv_group_by_col> sv_group_by_col
%type <sv_group_by_cols> group_by_cols sv_group_by

%type <sub_select_stmt> sub_select_stmt
%type <in_op_value> in_op_vlaue
%type <in_sub_query> in_sub_query

//%token FILE_PATH
%type <sv_str> file_path

%type <sv_str> table_col_name

%%
start:
        stmt ';'
    {
        parse_tree = $1;
        YYACCEPT;
    }
    |   HELP
    {
        parse_tree = std::make_shared<Help>();
        YYACCEPT;
    }
    |   EXIT
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    |   T_EOF
    {
        parse_tree = nullptr;
        YYACCEPT;
    }
    ;

stmt:
        dbStmt
    |   ddl
    |   dml
    |   txnStmt
    |   setStmt
    ;

txnStmt:
        TXN_BEGIN
    {
        $$ = std::make_shared<TxnBegin>();
    }
    |   TXN_COMMIT
    {
        $$ = std::make_shared<TxnCommit>();
    }
    |   TXN_ABORT
    {
        $$ = std::make_shared<TxnAbort>();
    }
    | TXN_ROLLBACK
    {
        $$ = std::make_shared<TxnRollback>();
    }
    ;

dbStmt:
        SHOW TABLES
    {
        $$ = std::make_shared<ShowTables>();
    }
    |
	SHOW INDEX FROM tbName
    {
	$$ = std::make_shared<ShowIndex>($4);
    }
    ;

setStmt:
        SET set_knob_type '=' VALUE_BOOL
    {
        $$ = std::make_shared<SetStmt>($2, $4);
    }
    ;

ddl:
        CREATE TABLE tbName '(' fieldList ')'
    {
        $$ = std::make_shared<CreateTable>($3, $5);
    }
    |   DROP TABLE tbName
    {
        $$ = std::make_shared<DropTable>($3);
    }
    |   DESC tbName
    {
        $$ = std::make_shared<DescTable>($2);
    }
    |   CREATE INDEX tbName '(' colNameList ')'
    {
        $$ = std::make_shared<CreateIndex>($3, $5);
    }
    |   DROP INDEX tbName '(' colNameList ')'
    {
        $$ = std::make_shared<DropIndex>($3, $5);
    }
    |   CREATE STATIC_CHECKPOINT
    {
    	$$ = std::make_shared<StaticCheckpoint>();
    }
    |   LOAD file_path INTO tbName
    {
         $$ = std::make_shared<LoadStmt>($2, $4);
         std::cout << "Parsed file path: " << $2 << std::endl;
    }
    ;

dml:
        INSERT INTO tbName VALUES '(' valueList ')'
    {
        $$ = std::make_shared<InsertStmt>($3, $6);
    }
    |   DELETE FROM tbName optWhereClause
    {
        $$ = std::make_shared<DeleteStmt>($3, $4);
    }
    |   UPDATE tbName SET setClauses optWhereClause
    {
        $$ = std::make_shared<UpdateStmt>($2, $4, $5);
    }
    |   SELECT selector FROM tableList optWhereClause opt_order_clause sv_group_by
    {
        $$ = std::make_shared<SelectStmt>($2, $4, $5, $6, $7);
    }
    ;

fieldList:
        field
    {
        $$ = std::vector<std::shared_ptr<Field>>{$1};
    }
    |   fieldList ',' field
    {
        $$.push_back($3);
    }
    ;

colNameList:
        colName
    {
        $$ = std::vector<std::string>{$1};
    }
    | colNameList ',' colName
    {
        $$.push_back($3);
    }
    ;

field:
        colName type
    {
        $$ = std::make_shared<ColDef>($1, $2);
    }
    ;

type:
        INT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_INT, sizeof(int));
    }
    |   CHAR '(' VALUE_INT ')'
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_STRING, $3);
    }
    |   FLOAT
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_FLOAT, sizeof(float));
    }
    |   DATETIME
    {
        $$ = std::make_shared<TypeLen>(SV_TYPE_STRING, 30);
    }
    ;

valueList:
        value
    {
        $$ = std::vector<std::shared_ptr<Value>>{$1};
    }
    |   valueList ',' value
    {
        $$.push_back($3);
    }
    ;

value:
        VALUE_INT
    {
        $$ = std::make_shared<IntLit>($1);
    }
    |   VALUE_FLOAT
    {
        $$ = std::make_shared<FloatLit>($1);
    }
    |   VALUE_STRING
    {
        $$ = std::make_shared<StringLit>($1);
    }
    |   VALUE_BOOL
    {
        $$ = std::make_shared<BoolLit>($1);
    }
    ;

condition:
        col op expr
    {
        $$ = std::make_shared<BinaryExpr>($1, $2, $3);
    }
    |   col op '(' expr ')'
    {
    	$$ = std::make_shared<BinaryExpr>($1, $2, $4);
    }
    |   col IN '(' in_sub_query ')'
    {
    	$$ = std::make_shared<BinaryExpr>($1, SV_OP_IN, $4);
    }
    ;

optWhereClause:
        /* epsilon */ { /* ignore*/ }
    |   WHERE whereClause
    {
        $$ = $2;
    }
    ;

whereClause:
        condition
    {
        $$ = std::vector<std::shared_ptr<BinaryExpr>>{$1};
    }
    |   whereClause AND condition
    {
        $$.push_back($3);
    }
    ;

col:
	'*'
    {
        $$ = std::make_shared<Col>("", "", SV_AGGREGATE_NULL, "");
    }
    |    table_col_name
    {
        $$ = std::make_shared<Col>($1, SV_AGGREGATE_NULL, "");
    }
    |   colName
    {
        $$ = std::make_shared<Col>("", $1, SV_AGGREGATE_NULL, "");
    }
    |   AGGREGATE_SUM '(' colName ')' AS colName
    {
        $$ = std::make_shared<Col>("", $3, $1, $6);
    }
    |    AGGREGATE_MIN '(' colName ')' AS colName
    {
        $$ = std::make_shared<Col>("", $3, $1, $6);
    }
    |    AGGREGATE_MAX '(' colName ')' AS colName
    {
        $$ = std::make_shared<Col>("", $3, $1, $6);
    }
    |   AGGREGATE_COUNT '(' colName ')' AS colName
    {
        $$ = std::make_shared<Col>("", $3, $1, $6);
    }
    |   AGGREGATE_COUNT '(' '*' ')' AS colName
    {
        $$ = std::make_shared<Col>("", "", $1, $6);
    }
    |   AGGREGATE_SUM '(' colName ')'
    {
        $$ = std::make_shared<Col>("", $3, $1, "");
    }
    |    AGGREGATE_MIN '(' colName ')'
    {
         $$ = std::make_shared<Col>("", $3, $1, "");
    }
    |    AGGREGATE_MAX '(' colName ')'
    {
        $$ = std::make_shared<Col>("", $3, $1, "");
    }
    |   AGGREGATE_COUNT '(' colName ')'
    {
        $$ = std::make_shared<Col>("", $3, $1, "");
    }
    |   AGGREGATE_COUNT '(' '*' ')'
    {
        $$ = std::make_shared<Col>("", "", $1, "");
    }
    ;

colList:
        col
    {
        $$ = std::vector<std::shared_ptr<Col>>{$1};
    }
    |   colList ',' col
    {
        $$.push_back($3);
    }
    ;

op:
        '='
    {
        $$ = SV_OP_EQ;
    }
    |   '<'
    {
        $$ = SV_OP_LT;
    }
    |   '>'
    {
        $$ = SV_OP_GT;
    }
    |   NEQ
    {
        $$ = SV_OP_NE;
    }
    |   LEQ
    {
        $$ = SV_OP_LE;
    }
    |   GEQ
    {
        $$ = SV_OP_GE;
    }
    ;

expr:
        value
    {
        $$ = std::static_pointer_cast<Expr>($1);
    }
    |   col
    {
        $$ = std::static_pointer_cast<Expr>($1);
    }
    |  sub_select_stmt
    {
    	$$ = std::static_pointer_cast<Expr>($1);
    }
    ;

sub_select_stmt:
	SELECT colList FROM tableList optWhereClause opt_order_clause sv_group_by
    {
        $$ = std::make_shared<SubSelectStmt>($2, $4, $5, $6, $7);
    }
    ;

in_op_vlaue:
       valueList
    {
        $$ = std::make_shared<InOpValue>($1);
    }
    ;

in_sub_query:
	   sub_select_stmt
    {
	$$ = std::static_pointer_cast<Expr>($1);
    }
    |      in_op_vlaue
    {
    	$$ = std::static_pointer_cast<Expr>($1);
    }
    ;

setClauses:
        setClause
    {
        $$ = std::vector<std::shared_ptr<SetClause>>{$1};
    }
    |   setClauses ',' setClause
    {
        $$.push_back($3);
    }
    ;

setClause:
        colName '=' value
    {
        $$ = std::make_shared<SetClause>($1, $3);
    }
    ;

selector:
	colList
    ;

AGGREGATE_SUM:
        SUM
    {
        $$ = SV_AGGREGATE_SUM;
    }
    ;

AGGREGATE_COUNT:
        COUNT
    {
        $$ = SV_AGGREGATE_COUNT;
    }
    ;

AGGREGATE_MAX:
        MAX
    {
        $$ = SV_AGGREGATE_MAX;
    }
    ;

AGGREGATE_MIN:
        MIN
    {
        $$ = SV_AGGREGATE_MIN;
    }
    ;

sv_group_by_col:
	col
    {
	$$ = std::make_shared<GroupBy>($1, std::vector<std::shared_ptr<BinaryExpr>>{});
    }
    |   col HAVING whereClause
    {
	$$ = std::make_shared<GroupBy>($1, $3);
    }
    ;

group_by_cols:
	sv_group_by_col
    {
    	$$ = std::vector<std::shared_ptr<GroupBy>>{$1};
    }
    |   group_by_cols ',' sv_group_by_col
    {
    	$$.push_back($3);
    }
    ;

sv_group_by:
	/* epsilon */ { /* ignore*/ }
    |   GROUP BY group_by_cols
    {
    	$$ = $3;
    }

tableList:
        tbName
    {
        $$ = std::vector<std::string>{$1};
    }
    |   tableList ',' tbName
    {
        $$.push_back($3);
    }
    |   tableList JOIN tbName
    {
        $$.push_back($3);
    }
    ;

opt_order_clause:
    ORDER BY order_clause
    {
        $$ = $3;
    }
    |   /* epsilon */ { /* ignore*/ }
    ;

order_clause:
      col  opt_asc_desc
    {
        $$ = std::make_shared<OrderBy>($1, $2);
    }
    ;

opt_asc_desc:
    ASC          { $$ = OrderBy_ASC;     }
    |  DESC      { $$ = OrderBy_DESC;    }
    |       { $$ = OrderBy_DEFAULT; }
    ;

set_knob_type:
    ENABLE_NESTLOOP { $$ = EnableNestLoop; }
    |   ENABLE_SORTMERGE { $$ = EnableSortMerge; }
    ;

tbName: IDENTIFIER;

colName: IDENTIFIER;

file_path: FILE_PATH_VALUE

table_col_name: TABLE_COL
%%
