
%{

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

%}

%define api.pure full
%define parse.error verbose
/** 启用位置标识 **/
%locations
%lex-param { yyscan_t scanner }
/** 这些定义了在yyparse函数中的参数 **/
%parse-param { const char * sql_string }
%parse-param { ParsedSqlResult * sql_result }
%parse-param { void * scanner }

//标识tokens
%token  SEMICOLON
        CREATE
        DROP
        TABLE
        VIEW
        TABLES
        INDEX
        CALC
        SELECT
        DESC
        SHOW
        SYNC
        INSERT
        DELETE
        UPDATE
        LBRACE
        RBRACE
        COMMA
        TRX_BEGIN
        TRX_COMMIT
        TRX_ROLLBACK
        INT_T
        STRING_T
        FLOAT_T
        DATE_T
        VECTOR_T
        HELP
        EXIT
        DOT //QUOTE
        INTO
        VALUES
        FROM
        WHERE
        AND
        SET
        ON
        LOAD
        DATA
        INFILE
        EXPLAIN
        EQ
        LT
        GT
        LE
        GE
        NE
        NOT
        LK
        INNER
        JOIN
        MAX_agg
        MIN_agg
        AVG_agg
        COUNT_agg
         SUM_agg
         UNIQUE
         NULLABLE
         IS
         ORDER
         BY
         ASC
         IN
         EXISTS
         OR
         AS
        LENGTH_func
        ROUND_func
        FORMAT_func
         TEXT_T
         GROUP
         HAVING

/** union 中定义各种数据类型，真实生成的代码也是union类型，所以不能有非POD类型的数据 **/
%union {
  ParsedSqlNode *                   sql_node;
  ConditionSqlNode *                condition;
  Value *                           value;
  enum CompOp                       comp;
  RelAttrSqlNode *                  rel_attr;
  std::vector<AttrInfoSqlNode> *    attr_infos;
  AttrInfoSqlNode *                 attr_info;
  ExpressionSqlNode *                      expression;
  std::vector<ExpressionSqlNode *> *       expression_list;
  std::vector<Value> *              value_list;
  std::vector<ConditionSqlNode> *   condition_list;
  std::vector<RelAttrSqlNode> *     rel_attr_list;
  InnerJoinSqlNode *                inner_join_list;
  std::vector<std::string> *        relation_list;
  char *                            string;
  int                               number;
  float                             floats;
  enum Agg                          agg;
  std::vector<UpdateValue>*         update_list;
  bool                              is_null;
  std::vector<OrderBySqlNode> *      order_by;
  LengthParam *                     length_func_param;
  RoundParam *                      round_func_param;
  FormatParam *                     format_func_param;
  GroupBySqlNode *                  group_by;
}

%token <number> NUMBER
%token <floats> FLOAT
%token <string> ID
%token <string> SSS
%token <string> DATE
%token <string> PATTERN
//非终结符

/** type 定义了各种解析后的结果输出的是什么类型。类型对应了 union 中的定义的成员变量名称 **/
%type <number>              type
%type <condition>           condition
%type <value>               value
%type <number>              number
%type <comp>                comp_op
%type <agg>                 agg
%type <rel_attr>            rel_attr
%type <attr_infos>          attr_def_list
%type <attr_info>           attr_def
%type <value_list>          value_list
%type <condition_list>      where
%type <condition_list>      on
%type <condition_list>      condition_list
%type <rel_attr_list>       select_attr
%type <inner_join_list>     rel_list
%type <rel_attr_list>       attr_list
%type <length_func_param>   length_func_param
%type <round_func_param>    round_func_param
%type <format_func_param>   format_func_param
%type <expression>          expression
%type <expression>          expression_list
%type <sql_node>            calc_stmt
%type <sql_node>            select_stmt
%type <sql_node>            insert_stmt
%type <sql_node>            update_stmt
%type <sql_node>            delete_stmt
%type <sql_node>            create_table_stmt
%type <sql_node>            drop_table_stmt
%type <sql_node>            show_tables_stmt
%type <sql_node>            desc_table_stmt
%type <sql_node>            create_index_stmt
%type <sql_node>            drop_index_stmt
%type <sql_node>            sync_stmt
%type <sql_node>            begin_stmt
%type <sql_node>            commit_stmt
%type <sql_node>            rollback_stmt
%type <sql_node>            load_data_stmt
%type <sql_node>            explain_stmt
%type <sql_node>            set_variable_stmt
%type <sql_node>            help_stmt
%type <sql_node>            exit_stmt
%type <sql_node>            command_wrapper
%type <relation_list>       arg_list
%type <update_list>         update_list
%type <relation_list>       id_list
%type <is_null>             nullable
%type <value_list>       insert_value
%type <value_list>       insert_values
%type <order_by>         order
%type <order_by>         order_by_list
%type <string>              as
%type <group_by>         group
%type <condition_list>    having
// commands should be a list but I use a single command instead
%type <sql_node>            commands

%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%%

commands: command_wrapper opt_semicolon  //commands or sqls. parser starts here.
  {
    std::unique_ptr<ParsedSqlNode> sql_node = std::unique_ptr<ParsedSqlNode>($1);
    sql_result->add_sql_node(std::move(sql_node));
  }
  ;

command_wrapper:
    calc_stmt
  | select_stmt
  | insert_stmt
  | update_stmt
  | delete_stmt
  | create_table_stmt
  | drop_table_stmt
  | show_tables_stmt
  | desc_table_stmt
  | create_index_stmt
  | drop_index_stmt
  | sync_stmt
  | begin_stmt
  | commit_stmt
  | rollback_stmt
  | load_data_stmt
  | explain_stmt
  | set_variable_stmt
  | help_stmt
  | exit_stmt
    ;

exit_stmt:      
    EXIT {
      (void)yynerrs;  // 这么写为了消除yynerrs未使用的告警。如果你有更好的方法欢迎提PR
      $$ = new ParsedSqlNode(SCF_EXIT);
    };

help_stmt:
    HELP {
      $$ = new ParsedSqlNode(SCF_HELP);
    };

sync_stmt:
    SYNC {
      $$ = new ParsedSqlNode(SCF_SYNC);
    }
    ;

begin_stmt:
    TRX_BEGIN  {
      $$ = new ParsedSqlNode(SCF_BEGIN);
    }
    ;

commit_stmt:
    TRX_COMMIT {
      $$ = new ParsedSqlNode(SCF_COMMIT);
    }
    ;

rollback_stmt:
    TRX_ROLLBACK  {
      $$ = new ParsedSqlNode(SCF_ROLLBACK);
    }
    ;

drop_table_stmt:    /*drop table 语句的语法解析树*/
    DROP TABLE ID {
      $$ = new ParsedSqlNode(SCF_DROP_TABLE);
      $$->drop_table.relation_name = $3;
      free($3);
    };

show_tables_stmt:
    SHOW TABLES {
      $$ = new ParsedSqlNode(SCF_SHOW_TABLES);
    }
    ;

desc_table_stmt:
    DESC ID  {
      $$ = new ParsedSqlNode(SCF_DESC_TABLE);
      $$->desc_table.relation_name = $2;
      free($2);
    }
    ;

create_index_stmt:    /*create index 语句的语法解析树*/
    CREATE INDEX ID ON ID LBRACE ID id_list RBRACE
    {
      $$ = new ParsedSqlNode(SCF_CREATE_INDEX);
      CreateIndexSqlNode &create_index = $$->create_index;
      create_index.index_name = $3;
      create_index.relation_name = $5;
      create_index.is_unique=false;
      if($8==nullptr){
      create_index.attribute_name.push_back($7);

      }else{
      $8->push_back($7);
       create_index.attribute_name.swap(*$8);
      }
      std::reverse(create_index.attribute_name.begin(), create_index.attribute_name.end());
      free($3);
      free($5);
      free($7);
      free($8);
    }
    |CREATE UNIQUE INDEX ID ON ID LBRACE ID id_list RBRACE
         {
           $$ = new ParsedSqlNode(SCF_CREATE_INDEX);
           CreateIndexSqlNode &create_index = $$->create_index;
           create_index.index_name = $4;
           create_index.relation_name = $6;
           create_index.is_unique=true;
           if($9==nullptr){
           create_index.attribute_name.push_back($8);

           }else{
           $9->push_back($8);
            create_index.attribute_name.swap(*$9);
           }
           std::reverse(create_index.attribute_name.begin(), create_index.attribute_name.end());
           free($4);
           free($6);
           free($8);
           free($9);
         }
    ;
id_list:
  {
  $$=nullptr;
  }
  |COMMA ID id_list{
   if($3!=nullptr){
   $$=$3;
   }else{
   $$=new std::vector<std::string>;
   }
   $$->push_back($2);
   free($2);

  }
drop_index_stmt:      /*drop index 语句的语法解析树*/
    DROP INDEX ID ON ID
    {
      $$ = new ParsedSqlNode(SCF_DROP_INDEX);
      $$->drop_index.index_name = $3;
      $$->drop_index.relation_name = $5;
      free($3);
      free($5);
    }
    ;
create_table_stmt:    /*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE
    {
      $$ = new ParsedSqlNode(SCF_CREATE_TABLE);
      CreateTableSqlNode &create_table = $$->create_table;
      create_table.relation_name = $3;
      free($3);

      std::vector<AttrInfoSqlNode> *src_attrs = $6;

      if (src_attrs != nullptr) {
        create_table.attr_infos.swap(*src_attrs);
      }
      create_table.attr_infos.emplace_back(*$5);
      std::reverse(create_table.attr_infos.begin(), create_table.attr_infos.end());
      delete $5;
    }
    ;
attr_def_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA attr_def attr_def_list
    {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<AttrInfoSqlNode>;
      }
      $$->emplace_back(*$2);
      delete $2;
    }
    ;
    
attr_def:
    ID type LBRACE number RBRACE nullable
    {
      $$ = new AttrInfoSqlNode;
      $$->type = (AttrType)$2;
      $$->name = $1;
      $$->length = $4;
      $$->is_null=$6;
      free($1);
    }
    | ID type  nullable
    {
    if ((AttrType)$2==DATES){
    $$ = new AttrInfoSqlNode;
              $$->type = (AttrType)$2;
              $$->name = $1;
              $$->length = 10;
              $$->is_null=$3;
              free($1);
    }else if((AttrType)$2==TEXTS){
    $$ = new AttrInfoSqlNode;
              $$->type = (AttrType)$2;
              $$->name = $1;
              $$->length = 8;
              $$->is_null=$3;
              free($1);
    }else{
    $$ = new AttrInfoSqlNode;
          $$->type = (AttrType)$2;
          $$->name = $1;
          $$->length = 4;
          $$->is_null=$3;
          free($1);
    }

    }
    ;
nullable:
   {
   $$=true;
   }
   |NULLABLE{
   $$=true;
   }
   |NOT NULLABLE{
   $$=false;
   }
number:
    NUMBER {$$ = $1;}
    ;
type:
    INT_T      { $$=INTS; }
    | STRING_T { $$=CHARS; }
    | FLOAT_T  { $$=FLOATS; }
    | DATE_T  {$$=DATES;}
    | TEXT_T {$$=TEXTS;}
    ;
insert_stmt:        /*insert   语句的语法解析树*/
    INSERT INTO ID VALUES insert_value insert_values
    {
      $$ = new ParsedSqlNode(SCF_INSERT);
      $$->insertion.relation_name = $3;
      if ($6 != nullptr) {
        $$->insertion.values.swap(*$6);
      }
      $$->insertion.values.insert($$->insertion.values.end(),$5->begin(),$5->end());
      std::reverse($$->insertion.values.begin(), $$->insertion.values.end());
      delete $5;
      free($3);
    }
    ;
insert_value:
    LBRACE value value_list RBRACE {
	if ($3 != nullptr) {
        $$ = $3;
        } else {
        $$ = new std::vector<Value>;
        }
        $$->emplace_back(*$2);
         delete $2;
    }
insert_values:
    /* empty */
    {
    $$=nullptr;
    }
    |COMMA insert_value insert_values{
	if ($3 != nullptr) {
                $$ = $3;
              } else {
                $$ = new std::vector<Value>;
              }
              $$->insert($$->end(),$2->begin(),$2->end());
              delete $2;
    }
value_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA value value_list  { 
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<Value>;
      }
      $$->emplace_back(*$2);
      delete $2;
    }
    ;
value:
    NUMBER {
      $$ = new Value((int)$1);
      @$ = @1;
    }
    |'-' NUMBER {
      $$ = new Value(-(int)$2);
      @$ = @2;
    }
    |FLOAT {
      $$ = new Value((float)$1);
      @$ = @1;
    }
    |'-' FLOAT {
    $$ = new Value(-(float)$2);
    @$ = @2;
    }
    |SSS {
      char *tmp = common::substr($1,1,strlen($1)-2);
      $$ = new Value(tmp,CHARS);
      free(tmp);
    }
    |DATE {
           char *tmp = common::substr($1,1,strlen($1)-2);
           $$ = new Value(tmp,DATES);
           free(tmp);
         }
    |PATTERN {
         char *tmp = common::substr($1,1,strlen($1)-2);
         $$ = new Value(tmp,CHARS);
         free(tmp);
         }
    |NULLABLE{
      $$=new Value();
      $$->set_null();
    }
    ;
    
delete_stmt:    /*  delete 语句的语法解析树*/
    DELETE FROM ID where 
    {
      $$ = new ParsedSqlNode(SCF_DELETE);
      $$->deletion.relation_name = $3;
      if ($4 != nullptr) {
        $$->deletion.conditions.swap(*$4);
        delete $4;
      }
      free($3);
    }
    ;
update_stmt:      /*  update 语句的语法解析树*/
    UPDATE ID SET update_list where
    {
      $$ = new ParsedSqlNode(SCF_UPDATE);
      $$->update.relation_name = $2;
      $$->update.updateValue_list.swap(*$4);

      if ($5 != nullptr) {
        $$->update.conditions.swap(*$5);
        delete $5;
      }
      free($2);
      free($4);
    }
update_list:
   {
   $$=nullptr;
   }
   |ID EQ value {

      $$=new std::vector<UpdateValue>;



      UpdateValue value;
      value.is_select=false;
      value.value=*$3;
      value.attribute_name=$1;
      $$->push_back(value);

      }
   |ID EQ value COMMA update_list{
   if($5==nullptr){
   $$=new std::vector<UpdateValue>;
   }else{
   $$=$5;
   }
   UpdateValue value;
   value.is_select=false;
   value.value=*$3;
   value.attribute_name=$1;
   $$->push_back(value);

   }
   |ID EQ  LBRACE select_stmt RBRACE {

         $$=new std::vector<UpdateValue>;

         UpdateValue value;
         value.is_select=true;
         value.selectSqlNode=$4->selection;
         value.attribute_name=$1;
         $$->push_back(value);

      }
   |ID EQ  LBRACE select_stmt RBRACE COMMA update_list{
   if($7==nullptr){
      $$=new std::vector<UpdateValue>;
      }else{
      $$=$7;
      }
      UpdateValue value;
      value.is_select=true;
      value.selectSqlNode=$4->selection;
      value.attribute_name=$1;
      $$->push_back(value);

   }
    ;
select_stmt:        /*  select 语句的语法解析树*/
    SELECT expression_list FROM ID as rel_list  where order group
         {
           $$ = new ParsedSqlNode(SCF_SELECT);
           if ($2 != nullptr) {
             $$->selection.attributes.swap($2->relAttrSqlNodes);
             $$->selection.attributes_expression.swap($2->expression);
             $$->selection.stringsqlExprs.swap($2->stringsqlExprs);
             $$->selection.fieldExprs.swap($2->fieldExprs);
             $$->selection.is_expression_select_attr=$2->is_expression;
             std::reverse($$->selection.attributes_expression.begin(), $$->selection.attributes_expression.end());
             delete $2;
           }
           $$->selection.is_alias_right=true;
           if ($6 != nullptr) {
             $$->selection.relations.swap($6->relations);
             $$->selection.conditions.swap($6->conditions);
             $$->selection.alias_map.insert($6->alias_map.begin(), $6->alias_map.end());
             $$->selection.is_alias_right=$6->is_alias_right;
             delete $6;
           }
           $$->selection.relations.push_back($4);
           std::reverse($$->selection.relations.begin(), $$->selection.relations.end());
           if ($5 != nullptr) {
           if($$->selection.alias_map.find($5)==$$->selection.alias_map.end()){
           $$->selection.alias_map.insert(std::pair<std::string, std::string>($5, $4));
           }else{
           $$->selection.is_alias_right=false;
            }
           }

           if ($7 != nullptr) {
             $$->selection.conditions.insert($$->selection.conditions.begin(),$7->begin(),$7->end());
             std::reverse($$->selection.conditions.begin(), $$->selection.conditions.end());
             delete $7;
           }
           if($8!=nullptr){
           $$->selection.order_by.insert($$->selection.order_by.begin(),$8->begin(),$8->end());
           std::reverse($$->selection.order_by.begin(), $$->selection.order_by.end());
                        delete $8;
           }
           $$->selection.is_sub_select=false;

           if($9!=nullptr){
           $$->selection.group_by=*$9;
           $$->selection.is_group_by=true;
           }else{
           $$->selection.is_group_by=false;
           }
           free($4);
         }
          | SELECT expression_list where {
            $$ = new ParsedSqlNode(SCF_SELECT);
            if ($2 != nullptr) {
                $$->selection.attributes.swap($2->relAttrSqlNodes);
                $$->selection.attributes_expression.swap($2->expression);
                $$->selection.stringsqlExprs.swap($2->stringsqlExprs);
                $$->selection.fieldExprs.swap($2->fieldExprs);
                $$->selection.is_expression_select_attr=$2->is_expression;
                std::reverse($$->selection.attributes_expression.begin(), $$->selection.attributes_expression.end());
                delete $2;
              }
            $$->selection.is_alias_right=true;
            if ($3 != nullptr) {
                $$->selection.conditions.insert($$->selection.conditions.begin(),$3->begin(),$3->end());
                std::reverse($$->selection.conditions.begin(), $$->selection.conditions.end());
                delete $3;
              }
            $$->selection.is_sub_select=false;
          }

    ;
group:
       {
        $$=nullptr;
       }
       |GROUP BY rel_attr attr_list having{
       $$=new GroupBySqlNode;
         if($4!=nullptr){
         $$->attrs.insert($$->attrs.end(),$4->begin(),$4->end());
         }
         $$->attrs.push_back(*$3);
         std::reverse($$->attrs.begin(),$$->attrs.end());
         if($5!=nullptr){
         $$->conditions.insert($$->conditions.end(),$5->begin(),$5->end());
         }

       }
       ;
having:
      {
       $$=nullptr;
      }
      |HAVING condition_list {
         $$ = $2;
      }
      ;

order:
  {
  $$=nullptr;
  }
  |ORDER BY rel_attr ASC order_by_list{
  if($5!=nullptr){
  $$=$5;
  }else{
  $$=new std::vector<OrderBySqlNode>;
  }
  OrderBySqlNode orderBySqlNode;
      orderBySqlNode.attrs=*$3;
      orderBySqlNode.orderBySequence=ASC_ORDER_BY;
      $$->push_back(orderBySqlNode);
   std::reverse($$->begin(),$$->end());
  }
  |ORDER BY rel_attr  order_by_list{
    if($4!=nullptr){
    $$=$4;
    }else{
    $$=new std::vector<OrderBySqlNode>;
    }
    OrderBySqlNode orderBySqlNode;
    orderBySqlNode.attrs=*$3;
    orderBySqlNode.orderBySequence=ASC_ORDER_BY;
    $$->push_back(orderBySqlNode);
     std::reverse($$->begin(),$$->end());
    }
  |ORDER BY rel_attr DESC order_by_list{
  if($5!=nullptr){
    $$=$5;
    }else{
    $$=new std::vector<OrderBySqlNode>;
    }
    $$->push_back({*$3,DESC_ORDER_BY});
     std::reverse($$->begin(),$$->end());

  };
order_by_list:
   {$$=nullptr;
   }
   |COMMA rel_attr ASC order_by_list{
   if($4!=nullptr){
     $$=$4;
     }else{
     $$=new std::vector<OrderBySqlNode>;
     }
     $$->push_back({*$2,ASC_ORDER_BY});
   }
   |COMMA rel_attr  order_by_list{
   if($3!=nullptr){
        $$=$3;
        }else{
        $$=new std::vector<OrderBySqlNode>;
        }
        $$->push_back({*$2,ASC_ORDER_BY});

   }|COMMA rel_attr DESC order_by_list{
       if($4!=nullptr){
            $$=$4;
            }else{
            $$=new std::vector<OrderBySqlNode>;
            }
            $$->push_back({*$2,DESC_ORDER_BY});
    }

calc_stmt:
    CALC expression_list
    {
      $$ = new ParsedSqlNode(SCF_CALC);
      std::reverse($2->expression.begin(), $2->expression.end());
      $$->calc.expressions.swap($2->expression);
      delete $2;
    }
    ;

expression_list:
    expression
    {
      $$ = $1;
    }
    | expression COMMA expression_list
    {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new ExpressionSqlNode ;
      }
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$1->stringsqlExprs.begin(),$1->stringsqlExprs.end());
      $$->fieldExprs.insert($$->fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
      $$->expression.insert($$->expression.end(),$1->expression.begin(),$1->expression.end());
      if($1->is_expression){
      $$->is_expression=true;
      }
    }
    ;
expression:
    expression '+' expression {
    $$=new ExpressionSqlNode;
      $$->expression.push_back(create_arithmetic_expression(ArithmeticExpr::Type::ADD, $1->expression[0], $3->expression[0], sql_string, &@$));
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$1->stringsqlExprs.begin(),$1->stringsqlExprs.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$3->stringsqlExprs.begin(),$3->stringsqlExprs.end());
      $$->fieldExprs.insert($$->fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
            $$->fieldExprs.insert($$->fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
            $$->is_expression=true;
    }
    | expression '-' expression {
    $$=new ExpressionSqlNode;
      $$->expression.push_back( create_arithmetic_expression(ArithmeticExpr::Type::SUB, $1->expression[0], $3->expression[0], sql_string, &@$));
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$1->stringsqlExprs.begin(),$1->stringsqlExprs.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$3->stringsqlExprs.begin(),$3->stringsqlExprs.end());
      $$->fieldExprs.insert($$->fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
                  $$->fieldExprs.insert($$->fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
                  $$->is_expression=true;
    }
    | expression '*' expression {
    $$=new ExpressionSqlNode;
      $$->expression.push_back( create_arithmetic_expression(ArithmeticExpr::Type::MUL, $1->expression[0], $3->expression[0], sql_string, &@$));
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$1->stringsqlExprs.begin(),$1->stringsqlExprs.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$3->stringsqlExprs.begin(),$3->stringsqlExprs.end());
      $$->fieldExprs.insert($$->fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
                  $$->fieldExprs.insert($$->fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
                  $$->is_expression=true;
    }
    | expression '/' expression {
    $$=new ExpressionSqlNode;
      $$->expression.push_back(create_arithmetic_expression(ArithmeticExpr::Type::DIV, $1->expression[0], $3->expression[0], sql_string, &@$));
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
      $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$1->stringsqlExprs.begin(),$1->stringsqlExprs.end());
      $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$3->stringsqlExprs.begin(),$3->stringsqlExprs.end());
      $$->fieldExprs.insert($$->fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
                  $$->fieldExprs.insert($$->fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
                  $$->is_expression=true;
    }
    | LBRACE expression RBRACE {
      $$ = $2;
      $$->expression[0]->set_name(token_name(sql_string, &@$));
      $$->is_expression=true;
    }
    | '-' expression %prec UMINUS {
    $$=new ExpressionSqlNode;
    $$->expression.push_back( create_arithmetic_expression(ArithmeticExpr::Type::NEGATIVE, $2->expression[0], nullptr, sql_string, &@$));
    $$->relAttrSqlNodes.insert($$->relAttrSqlNodes.end(),$2->relAttrSqlNodes.begin(),$2->relAttrSqlNodes.end());
    $$->stringsqlExprs.insert($$->stringsqlExprs.end(),$2->stringsqlExprs.begin(),$2->stringsqlExprs.end());
    $$->fieldExprs.insert($$->fieldExprs.end(),$2->fieldExprs.begin(),$2->fieldExprs.end());
    $$->is_expression=true;
    }
    | value {
      $$=new ExpressionSqlNode;
      $$->expression.push_back( new ValueExpr(*$1));
      $$->expression[0]->set_name(token_name(sql_string, &@$));
      $$->is_expression=false;
      $$->is_value=true;
      delete $1;
    }
    |ID as{
    $$=new ExpressionSqlNode;
           auto relAttrSqlNode = new RelAttrSqlNode;
           relAttrSqlNode->attribute_name = $1;
           relAttrSqlNode->agg=NO_AGG;
           relAttrSqlNode->func = NO_FUNC;
           relAttrSqlNode->is_right=true;
           if($2!=nullptr){
           relAttrSqlNode->alias_name=$2;
           }
    relAttrSqlNode->sqlString=token_name(sql_string, &@$);
    $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
    auto fieldExpr=new FieldExpr;
    fieldExpr->set_name(token_name(sql_string, &@$));
    $$->expression.push_back(fieldExpr);
    $$->fieldExprs.push_back(fieldExpr);
    $$->is_expression=false;
    $$->is_value=false;
           free($1);
         }
         |agg LBRACE arg_list RBRACE as{
         $$=new ExpressionSqlNode;
         auto relAttrSqlNode = new RelAttrSqlNode;
              if($3==nullptr||$3->size()!=1){
              relAttrSqlNode->is_right=false;
              }else{
              relAttrSqlNode->attribute_name = (*$3)[0];
              relAttrSqlNode->is_right=true;
              relAttrSqlNode->agg=$1;
              relAttrSqlNode->func = NO_FUNC;
              }
              if($5!=nullptr){
               relAttrSqlNode->alias_name=$5;
              }
              relAttrSqlNode->sqlString=token_name(sql_string, &@$);
              $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
                  auto stringSqlExpr=new StringSqlExpr;
                  stringSqlExpr->set_name(token_name(sql_string, &@$));
                  $$->expression.push_back(stringSqlExpr);
                  $$->stringsqlExprs.push_back(stringSqlExpr);
                  $$->is_expression=false;
                  $$->is_value=false;
               free($3);

              }
         | ID DOT ID as{
         $$=new ExpressionSqlNode;
         auto relAttrSqlNode = new RelAttrSqlNode;
           relAttrSqlNode->relation_name  = $1;
           relAttrSqlNode->attribute_name = $3;
           relAttrSqlNode->agg=NO_AGG;
           relAttrSqlNode->func = NO_FUNC;
           relAttrSqlNode->is_right=true;
           if($4!=nullptr){
           relAttrSqlNode->alias_name=$4;
           }
           relAttrSqlNode->sqlString=token_name(sql_string, &@$);
               $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
               auto fieldExpr=new FieldExpr;
               fieldExpr->set_name(token_name(sql_string, &@$));
               $$->expression.push_back(fieldExpr);
               $$->fieldExprs.push_back(fieldExpr);
               $$->is_expression=false;
               $$->is_value=false;
           free($1);
           free($3);
         }
         | agg LBRACE ID DOT ID RBRACE as{
         $$=new ExpressionSqlNode;
                  auto relAttrSqlNode = new RelAttrSqlNode;
                    relAttrSqlNode->relation_name  = $3;
                    relAttrSqlNode->attribute_name = $5;
                    relAttrSqlNode->is_right=true;
                    relAttrSqlNode->agg=$1;
                    relAttrSqlNode->func = NO_FUNC;
                    if($7!=nullptr){
                          relAttrSqlNode->alias_name=$7;
                          }
                    relAttrSqlNode->sqlString=token_name(sql_string, &@$);
                    $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
                    auto stringSqlExpr=new StringSqlExpr;
                    stringSqlExpr->set_name(token_name(sql_string, &@$));
                    $$->expression.push_back(stringSqlExpr);
                    $$->stringsqlExprs.push_back(stringSqlExpr);
                    $$->is_expression=false;
                    $$->is_value=false;
                    free($3);
                    free($5);
         }
         |'*' as {
         $$=new ExpressionSqlNode;
         auto relAttrSqlNode = new RelAttrSqlNode;
               relAttrSqlNode->relation_name  = "";
               relAttrSqlNode->attribute_name = "*";
               if($2!=nullptr){
               relAttrSqlNode->is_right=false;
               }else{
               relAttrSqlNode->is_right=true;
               }
               relAttrSqlNode->agg=NO_AGG;
               relAttrSqlNode->func = NO_FUNC;
               relAttrSqlNode->sqlString=token_name(sql_string, &@$);
               $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
               auto fieldExpr=new FieldExpr;
               fieldExpr->set_name(token_name(sql_string, &@$));
               $$->expression.push_back(fieldExpr);
               $$->fieldExprs.push_back(fieldExpr);
               $$->is_expression=false;
               $$->is_value=false;
             }
         |ID DOT '*' as {
         $$=new ExpressionSqlNode;
         auto relAttrSqlNode = new RelAttrSqlNode;
               relAttrSqlNode->relation_name  = $1;
               relAttrSqlNode->attribute_name = "*";
               if($4!=nullptr){
               relAttrSqlNode->is_right=false;
               }else{
               relAttrSqlNode->is_right=true;
               }

               relAttrSqlNode->agg=NO_AGG;
               relAttrSqlNode->func = NO_FUNC;
               relAttrSqlNode->sqlString=token_name(sql_string, &@$);
               $$->relAttrSqlNodes.push_back(*relAttrSqlNode);
               auto fieldExpr=new FieldExpr;
               fieldExpr->set_name(token_name(sql_string, &@$));
               $$->expression.push_back(fieldExpr);
               $$->fieldExprs.push_back(fieldExpr);
               $$->is_expression=false;
               $$->is_value=false;
         }
        | LENGTH_func LBRACE length_func_param RBRACE as {
          $$=new ExpressionSqlNode;
          auto relAttrSqlNode = new RelAttrSqlNode;
          relAttrSqlNode->relation_name  = $3->relation_name;
          relAttrSqlNode->attribute_name = $3->attribute_name;
          relAttrSqlNode->is_right = true;
          if($5!=nullptr){relAttrSqlNode->alias_name=$5;}
          relAttrSqlNode->agg = NO_AGG;
          relAttrSqlNode->func = LENGTH_FUNC;
          relAttrSqlNode->lengthparam = *$3;
          relAttrSqlNode->sqlString=token_name(sql_string, &@$);
          $$->relAttrSqlNodes.push_back(*relAttrSqlNode);

          /*auto funcExpr=new FuncExpr;
          funcExpr->set_name(token_name(sql_string, &@$));
          funcExpr->set_func(LENGTH_FUNC);
          funcExpr->set_lengthparam($3);
          $$->expression.push_back(funcExpr);
          $$->funcExprs.push_back(funcExpr);*/
          $$->is_expression=false;
          $$->is_value=false;
        }
        | ROUND_func LBRACE round_func_param RBRACE as {
          $$=new ExpressionSqlNode;
          auto relAttrSqlNode = new RelAttrSqlNode;
          relAttrSqlNode->relation_name  = $3->relation_name;
          relAttrSqlNode->attribute_name = $3->attribute_name;
          relAttrSqlNode->is_right = true;
          if($5!=nullptr){relAttrSqlNode->alias_name=$5;}
          relAttrSqlNode->agg = NO_AGG;
          relAttrSqlNode->func = ROUND_FUNC;
          relAttrSqlNode->roundparam = *$3;
          relAttrSqlNode->sqlString=token_name(sql_string, &@$);
          $$->relAttrSqlNodes.push_back(*relAttrSqlNode);

          /*auto funcExpr=new FuncExpr;
          funcExpr->set_name(token_name(sql_string, &@$));
          funcExpr->set_func(ROUND_FUNC);
          funcExpr->set_roundparam($3);
          $$->expression.push_back(funcExpr);
          $$->funcExprs.push_back(funcExpr);*/
          $$->is_expression=false;
          $$->is_value=false;

        }
        | FORMAT_func LBRACE format_func_param RBRACE as {
          $$=new ExpressionSqlNode;
          auto relAttrSqlNode = new RelAttrSqlNode;
          relAttrSqlNode->relation_name  = $3->relation_name;
          relAttrSqlNode->attribute_name = $3->attribute_name;
          relAttrSqlNode->is_right = true;
          if($5!=nullptr){relAttrSqlNode->alias_name=$5;}
          relAttrSqlNode->agg = NO_AGG;
          relAttrSqlNode->func = FORMAT_FUNC;
          relAttrSqlNode->formatparam = *$3;
          relAttrSqlNode->sqlString=token_name(sql_string, &@$);
          $$->relAttrSqlNodes.push_back(*relAttrSqlNode);

          /*auto funcExpr=new FuncExpr;
          funcExpr->set_name(token_name(sql_string, &@$));
          funcExpr->set_func(FORMAT_FUNC);
          funcExpr->set_formatparam($3);
          $$->expression.push_back(funcExpr);
          $$->funcExprs.push_back(funcExpr);*/
          $$->is_expression=false;
          $$->is_value=false;
        }
        ;

select_attr:
    /*'*' {
      $$ = new std::vector<RelAttrSqlNode>;
      RelAttrSqlNode attr;
      attr.relation_name  = "";
      attr.attribute_name = "*";
      attr.agg=NO_AGG;
      $$->emplace_back(attr);
    }*/
   /* | agg LBRACE arg_list RBRACE{
         $$ = new std::vector<RelAttrSqlNode>;
               RelAttrSqlNode attr;
               attr.relation_name  = "";
               attr.attribute_name = "*";
               attr.agg=$1;
               $$->emplace_back(attr);
         }*/
     rel_attr attr_list {
      if ($2 != nullptr) {
        $$ = $2;
      } else {
        $$ = new std::vector<RelAttrSqlNode>;
      }
      $$->emplace_back(*$1);
      delete $1;
    }
    ;
arg_list:
 /* empty */
    {
      $$ = nullptr;
    }
    |'*' {
    $$=new std::vector<std::string>;
    $$->push_back("*");

    }
    |'*' COMMA arg_list{
    if($3!=nullptr){
    $$=$3;
    }else{
    $$=new std::vector<std::string>;
    }

     $$->push_back("*");
    }
    | ID {
    $$=new std::vector<std::string>;
    $$->push_back($1);
    }
    |ID COMMA arg_list{
    if($3!=nullptr){
        $$=$3;
        }else{
        $$=new std::vector<std::string>;
        }
        $$->push_back($1);
    }
rel_attr:
    ID as{
      $$ = new RelAttrSqlNode;
      $$->attribute_name = $1;
      $$->agg=NO_AGG;
      $$->is_right=true;
      if($2!=nullptr){
      $$->alias_name=$2;
      }

      free($1);
    }
    |agg LBRACE arg_list RBRACE as{
         $$ = new RelAttrSqlNode;
         if($3==nullptr||$3->size()!=1){
         $$->is_right=false;
         }else{
         $$->attribute_name = (*$3)[0];
         $$->is_right=true;
         $$->agg=$1;
         }
         if($5!=nullptr){
          $$->alias_name=$5;
         }
          free($3);

         }
    | ID DOT ID as{
      $$ = new RelAttrSqlNode;
      $$->relation_name  = $1;
      $$->attribute_name = $3;
      $$->agg=NO_AGG;
      $$->is_right=true;
      if($4!=nullptr){
      $$->alias_name=$4;
      }
      free($1);
      free($3);
    }
    | agg LBRACE ID DOT ID RBRACE as{
               $$ = new RelAttrSqlNode;
               $$->relation_name  = $3;
               $$->attribute_name = $5;
               $$->is_right=true;
               $$->agg=$1;
               if($7!=nullptr){
                     $$->alias_name=$7;
                     }
               free($3);
               free($5);
    }
    |'*' as {
          $$ = new RelAttrSqlNode;
          $$->relation_name  = "";
          $$->attribute_name = "*";
          if($2!=nullptr){
          $$->is_right=false;
          }else{
          $$->is_right=true;
          }

          $$->agg=NO_AGG;
        }
    |ID DOT '*' as {
           $$ = new RelAttrSqlNode;
          $$->relation_name  = $1;
          $$->attribute_name = "*";
          if($4!=nullptr){
          $$->is_right=false;
          }else{
          $$->is_right=true;
          }

          $$->agg=NO_AGG;
    }
    ;

attr_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA rel_attr attr_list {
      if ($3 != nullptr) {
        $$ = $3;
      } else {
        $$ = new std::vector<RelAttrSqlNode>;
      }

      $$->emplace_back(*$2);
      delete $2;
    }
    ;

rel_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | COMMA ID as rel_list {
      if ($4 != nullptr) {
        $$ = $4;
      } else {
        $$ = new InnerJoinSqlNode;
      }
      if($3!=nullptr){
      if($$->alias_map.find($3)==$$->alias_map.end()){
      $$->alias_map[$3]=$2;
      }else{
      $$->is_alias_right=false;
      }
      }

      $$->relations.push_back($2);
      free($2);
    }
    |INNER JOIN ID as on rel_list {
           if ($6 != nullptr) {
             $$ = $6;
           } else {
             $$ = new InnerJoinSqlNode;
           }

           $$->relations.push_back($3);
           if($5!=nullptr){
           $$->conditions.insert($$->conditions.end(),$5->begin(),$5->end());
           }
           if($4!=nullptr){
           if($$->alias_map.find($4)==$$->alias_map.end()){
           $$->alias_map[$4]=$3;
           }else{
           $$->is_alias_right=false;
           }

           }

           free($3);
         }
    ;

/* Function length()的参数匹配 */
length_func_param:
  ID {
    $$ = new LengthParam;
    $$->attribute_name = $1;
    free($1);
  }
  | ID DOT ID {
    $$ = new LengthParam;
    $$->relation_name = $1;
    $$->attribute_name = $3;
    free($1);
    free($3);
  }
  | PATTERN {
    $$ = new LengthParam;

    char *tmp = common::substr($1,1,strlen($1)-2);
    $$->raw_data = Value(tmp,CHARS);
    free(tmp);
    free($1);
  }
  ;
/* Function round()的参数匹配 */
round_func_param:
  ID COMMA NUMBER {
    $$ = new RoundParam;
    $$->attribute_name = $1;
    $$->bits = Value((int)$3);
    free($1);
  }
  | ID {
    $$ = new RoundParam;
    $$->attribute_name = $1;
    free($1);
  }
  | ID DOT ID COMMA NUMBER {
    $$ = new RoundParam;
    $$->relation_name = $1;
    $$->attribute_name = $3;
    $$->bits = Value((int)$5);
    free($1);
    free($3);
  }
  | ID DOT ID {
    $$ = new RoundParam;
    $$->relation_name = $1;
    $$->attribute_name = $3;
    free($1);
    free($3);
  }
  | FLOAT COMMA NUMBER {
    $$ = new RoundParam;
    $$->raw_data = Value((float)$1);
    $$->bits = Value((int)$3);
  }
  | FLOAT {
    $$ = new RoundParam;
    $$->raw_data = Value((float)$1);
  }
  ;
/* Function date_format()的参数匹配 */
format_func_param:
  ID COMMA PATTERN {
    $$ = new FormatParam;
    $$->attribute_name = $1;

    char *tmp = common::substr($3,1,strlen($3)-2);
    $$->format = Value(tmp,CHARS);
    free(tmp); 
    free($1);
  }
  | ID DOT ID COMMA PATTERN {
    $$ = new FormatParam;
    $$->relation_name = $1;
    $$->attribute_name = $3;

    char *tmp = common::substr($5,1,strlen($5)-2);
    $$->format = Value(tmp,CHARS);
    free(tmp); 
    free($1);
    free($3);
  }
  | DATE COMMA PATTERN {
    $$ = new FormatParam;

    char *tmp = common::substr($1,1,strlen($1)-2);
    $$->raw_data = Value(tmp,DATES);
    free(tmp);

    char *tmp2 = common::substr($3,1,strlen($3)-2);
    $$->format = Value(tmp2,CHARS);
    free(tmp2);
    free($3);
  }
  ;

where:
    /* empty */
    {
      $$ = nullptr;
    }
    | WHERE condition_list {
      $$ = $2;  
    }
    ;
on:
     /* empty */
     {
        $$ = nullptr;
     }
     | ON condition_list {
        $$ = $2;
     }
     ;
condition_list:
    /* empty */
    {
      $$ = nullptr;
    }
    | condition {
      $$ = new std::vector<ConditionSqlNode>;
      $1->is_conjunction_or=false;
      $$->emplace_back(*$1);
      delete $1;
    }
    | condition AND condition_list {
      $$ = $3;
      $1->is_conjunction_or=false;
      $$->emplace_back(*$1);
      delete $1;
    }
    | condition OR condition_list {
           $$ = $3;
           $1->is_conjunction_or=true;
           $$->emplace_back(*$1);
           delete $1;
         }
    ;
condition:
    /*rel_attr comp_op value
    {
      $$ = new ConditionSqlNode;
      $$->left_type = ATTR_TYPE;
      $$->left_attr = *$1;
      $$->right_type = VALUE_TYPE;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op value 
    {
      $$ = new ConditionSqlNode;
      $$->left_type = VALUE_TYPE;
      $$->left_value = *$1;
      $$->right_type = VALUE_TYPE;
      $$->right_value = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | rel_attr comp_op rel_attr
    {
      $$ = new ConditionSqlNode;
      $$->left_type = ATTR_TYPE;
      $$->left_attr = *$1;
      $$->right_type = ATTR_TYPE;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op rel_attr
    {
      $$ = new ConditionSqlNode;
      $$->left_type = VALUE_TYPE;
      $$->left_value = *$1;
      $$->right_type = ATTR_TYPE;
      $$->right_attr = *$3;
      $$->comp = $2;

      delete $1;
      delete $3;
    }
    | value comp_op LBRACE select_stmt RBRACE
    {
    $$ = new ConditionSqlNode;
          $$->left_type = VALUE_TYPE;
          $$->left_value = *$1;
          $$->right_type = SUB_SELECT_TYPE;
          $$->right_select = &($4->selection);
          $$->right_select->is_sub_select=true;
          $$->comp = $2;
          delete $1;

    }
    | LBRACE select_stmt RBRACE comp_op value
    {
    $$ = new ConditionSqlNode;
              $$->left_type = SUB_SELECT_TYPE;
              $$->left_select =  &($2->selection);
              $$->left_select->is_sub_select=true;
              $$->right_type = VALUE_TYPE;
              $$->right_value = *$5;
              $$->comp = $4;
              delete $5;

    }
    |LBRACE select_stmt RBRACE comp_op LBRACE select_stmt RBRACE
    {
    $$ = new ConditionSqlNode;
              $$->left_type = SUB_SELECT_TYPE;
              $$->left_select =  &($2->selection);
              $$->left_select->is_sub_select=true;
              $$->right_type = SUB_SELECT_TYPE;
              $$->right_select =  &($6->selection);
              $$->right_select->is_sub_select=true;
              $$->comp = $4;


    }
    | rel_attr comp_op LBRACE select_stmt RBRACE
    {
    $$ = new ConditionSqlNode;
              $$->left_type = ATTR_TYPE;
              $$->left_attr = *$1;
              $$->right_type = SUB_SELECT_TYPE;
              $$->right_select =  &($4->selection);
              $$->right_select->is_sub_select=true;
              $$->comp = $2;
              delete $1;

    }
    | LBRACE select_stmt RBRACE comp_op rel_attr
     {
     $$ = new ConditionSqlNode;
               $$->left_type = SUB_SELECT_TYPE;
               $$->left_select =  &($2->selection);
               $$->left_select->is_sub_select=true;
               $$->right_type = ATTR_TYPE;
               $$->right_attr = *$5;
               $$->comp = $4;
               delete $5;

     }
     | EXISTS LBRACE select_stmt RBRACE{
     $$ = new ConditionSqlNode;
                    $$->right_type = SUB_SELECT_TYPE;
                    $$->right_select =  &($3->selection);
                    $$->right_select->is_sub_select=true;
                    $$->comp = EXISTS_OP;

     }
     | NOT EXISTS LBRACE select_stmt RBRACE{
$$ = new ConditionSqlNode;
                    $$->right_type = SUB_SELECT_TYPE;
                    $$->right_select =  &($4->selection);
                    $$->right_select->is_sub_select=true;
                    $$->comp = NOT_EXISTS_OP;
     }
     |rel_attr comp_op LBRACE value value_list RBRACE{
     $$ = new ConditionSqlNode;
     $$->left_type=ATTR_TYPE;
     $$->left_attr=*$1;
                         $$->right_type = VALUE_LIST_TYPE;
                         if($5!=nullptr){
                         $$->right_value_list.insert($$->right_value_list.end(),$5->begin(),$5->end());
                         }
                         $$->right_value_list.push_back(*$4);
                         std::reverse($$->right_value_list.begin(),$$->right_value_list.end());
                         $$->comp = $2;
     }
     |LBRACE select_stmt RBRACE comp_op LBRACE value value_list RBRACE{
           $$ = new ConditionSqlNode;
                               $$->left_type = SUB_SELECT_TYPE;
                               $$->left_select =  &($2->selection);
                               $$->left_select->is_sub_select=true;
                               if($7!=nullptr){
                               $$->right_value_list.insert($$->right_value_list.end(),$7->begin(),$7->end());
                               }
                               $$->right_value_list.push_back(*$6);
                               std::reverse($$->right_value_list.begin(),$$->right_value_list.end());
                               $$->comp = $4;
     }
     |value comp_op LBRACE value value_list RBRACE{
           $$ = new ConditionSqlNode;
           $$->left_type=VALUE_TYPE;
           $$->left_value=*$1;
                               $$->right_type = VALUE_LIST_TYPE;
                               if($5!=nullptr){
                               $$->right_value_list.insert($$->right_value_list.end(),$5->begin(),$5->end());
                               }
                               $$->right_value_list.push_back(*$4);
                               std::reverse($$->right_value_list.begin(),$$->right_value_list.end());
                               $$->comp = $2;
     }
     |expression comp_op value{
	$$ = new ConditionSqlNode;
              $$->left_type = EXPR_TYPE;
              $$->left_expr = $1->expression[0];
              $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
              $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
              $$->right_type = VALUE_TYPE;
              $$->right_value = *$3;
              $$->comp = $2;


              delete $3;
     }
     |expression comp_op rel_attr{
     $$ = new ConditionSqlNode;
                   $$->left_type = EXPR_TYPE;
                   $$->left_expr = $1->expression[0];
                   $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
                   $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
                   $$->right_type = ATTR_TYPE;
                   $$->right_attr = *$3;
                   $$->comp = $2;


                   delete $3;

     }
     |value comp_op expression{
       $$ = new ConditionSqlNode;
              $$->right_type = EXPR_TYPE;
              $$->right_expr = $3->expression[0];
              $$->right_relAttrSqlNodes.insert($$->right_relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
              $$->right_fieldExprs.insert($$->right_fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
              $$->left_type = VALUE_TYPE;
              $$->left_value = *$1;
              $$->comp = $2;


              delete $1;
     }
     |rel_attr comp_op expression{
     $$ = new ConditionSqlNode;
                        $$->right_type = EXPR_TYPE;
                        $$->right_expr = $3->expression[0];
                        $$->right_relAttrSqlNodes.insert($$->right_relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
                        $$->right_fieldExprs.insert($$->right_fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
                        $$->left_type = ATTR_TYPE;
                        $$->left_attr = *$1;
                        $$->comp = $2;


                        delete $1;

     }
     |expression comp_op expression{
     $$ = new ConditionSqlNode;
                             $$->right_type = EXPR_TYPE;
                             $$->right_expr = $3->expression[0];
                             $$->right_relAttrSqlNodes.insert($$->right_relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
                             $$->right_fieldExprs.insert($$->right_fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
                             $$->left_type = EXPR_TYPE;
                             $$->left_expr = $1->expression[0];
                             $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
                             $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
                             $$->comp = $2;



     }*/
     expression comp_op expression{
          $$ = new ConditionSqlNode;
          if($1->is_expression==false){
          if($1->is_value==true){
          $$->left_type=VALUE_TYPE;
          $$->left_value=((ValueExpr*)($1->expression[0]))->get_value();
          }else{
             $$->left_type = ATTR_TYPE;
             $$->left_attr = $1->relAttrSqlNodes[0];
          }
          }else{
          $$->left_type = EXPR_TYPE;
          $$->left_expr = $1->expression[0];
          $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
          $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
          }

          if($3->is_expression==false){
          if($3->is_value==true){
          $$->right_type=VALUE_TYPE;
          $$->right_value=((ValueExpr*)($3->expression[0]))->get_value();
          }else{
          $$->right_type = ATTR_TYPE;
          $$->right_attr = $3->relAttrSqlNodes[0];
          }
          }else{
          $$->right_type = EXPR_TYPE;
          $$->right_expr = $3->expression[0];
          $$->right_relAttrSqlNodes.insert($$->right_relAttrSqlNodes.end(),$3->relAttrSqlNodes.begin(),$3->relAttrSqlNodes.end());
          $$->right_fieldExprs.insert($$->right_fieldExprs.end(),$3->fieldExprs.begin(),$3->fieldExprs.end());
          }
          $$->comp = $2;



     }
     | expression comp_op LBRACE select_stmt RBRACE{
          $$ = new ConditionSqlNode;
          if($1->is_expression==false){
          if($1->is_value==true){
          $$->left_type=VALUE_TYPE;
          $$->left_value=((ValueExpr*)($1->expression[0]))->get_value();
          }else{
                 $$->left_type = ATTR_TYPE;
                 $$->left_attr = $1->relAttrSqlNodes[0];
          }
          }else{
          $$->left_type = EXPR_TYPE;
          $$->left_expr = $1->expression[0];
          $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
          $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
          }

          $$->right_type = SUB_SELECT_TYPE;
          $$->right_select = &($4->selection);
          $$->right_select->is_sub_select=true;
          $$->comp = $2;

     }
     | LBRACE select_stmt RBRACE comp_op expression{
          $$ = new ConditionSqlNode;

          if($5->is_expression==false){
          if($5->is_value==true){
          $$->right_type=VALUE_TYPE;
          $$->right_value=((ValueExpr*)($5->expression[0]))->get_value();
          }else{
          $$->right_type = ATTR_TYPE;
          $$->right_attr = $5->relAttrSqlNodes[0];
          }
          }else{
          $$->right_type = EXPR_TYPE;
          $$->right_expr = $5->expression[0];
          $$->right_relAttrSqlNodes.insert($$->right_relAttrSqlNodes.end(),$5->relAttrSqlNodes.begin(),$5->relAttrSqlNodes.end());
          $$->right_fieldExprs.insert($$->right_fieldExprs.end(),$5->fieldExprs.begin(),$5->fieldExprs.end());
          }


          $$->left_type = SUB_SELECT_TYPE;
          $$->left_select = &($2->selection);
          $$->left_select->is_sub_select=true;
          $$->comp = $4;

     }
     |LBRACE select_stmt RBRACE comp_op LBRACE select_stmt RBRACE{
         $$ = new ConditionSqlNode;
         $$->left_type = SUB_SELECT_TYPE;
         $$->left_select =  &($2->selection);
         $$->left_select->is_sub_select=true;
         $$->right_type = SUB_SELECT_TYPE;
         $$->right_select =  &($6->selection);
         $$->right_select->is_sub_select=true;
         $$->comp = $4;


     }
     |EXISTS LBRACE select_stmt RBRACE{
              $$ = new ConditionSqlNode;
              $$->right_type = SUB_SELECT_TYPE;
              $$->right_select =  &($3->selection);
              $$->right_select->is_sub_select=true;
              $$->comp = EXISTS_OP;

     }
     | NOT EXISTS LBRACE select_stmt RBRACE{
         $$ = new ConditionSqlNode;
         $$->right_type = SUB_SELECT_TYPE;
         $$->right_select =  &($4->selection);
         $$->right_select->is_sub_select=true;
         $$->comp = NOT_EXISTS_OP;
     }
     |expression comp_op LBRACE value COMMA value value_list RBRACE{
         $$ = new ConditionSqlNode;

         if($1->is_expression==false){
         if($1->is_value==true){
         $$->left_type=VALUE_TYPE;
         $$->left_value=((ValueExpr*)($1->expression[0]))->get_value();
         }else{
         $$->left_type = ATTR_TYPE;
         $$->left_attr = $1->relAttrSqlNodes[0];
         }
         }else{
         $$->left_type = EXPR_TYPE;
         $$->left_expr = $1->expression[0];
         $$->left_relAttrSqlNodes.insert($$->left_relAttrSqlNodes.end(),$1->relAttrSqlNodes.begin(),$1->relAttrSqlNodes.end());
         $$->left_fieldExprs.insert($$->left_fieldExprs.end(),$1->fieldExprs.begin(),$1->fieldExprs.end());
         }

         $$->right_type = VALUE_LIST_TYPE;
         if($7!=nullptr){
         $$->right_value_list.insert($$->right_value_list.end(),$7->begin(),$7->end());
          }
          $$->right_value_list.push_back(*$6);
          $$->right_value_list.push_back(*$4);
          std::reverse($$->right_value_list.begin(),$$->right_value_list.end());
          $$->comp = $2;
     }

    ;

comp_op:
      EQ { $$ = EQUAL_TO; }
    | LT { $$ = LESS_THAN; }
    | GT { $$ = GREAT_THAN; }
    | LE { $$ = LESS_EQUAL; }
    | GE { $$ = GREAT_EQUAL; }
    | NE { $$ = NOT_EQUAL; }
    | LK {$$ = LIKE;}
    | NOT LK {$$ = NOT_LIKE;}
    | IS    {$$=IS_NULL;}
    | IS NOT {$$=IS_NOT_NULL;}
    | IN   {$$=IN_OP;}
    | NOT IN {$$=NOT_IN_OP;}
    ;
agg:
    MAX_agg{$$=MAX_AGG;}
    |MIN_agg{$$=MIN_AGG;}
    |AVG_agg{$$=AVG_AGG;}
    |COUNT_agg{$$=COUNT_AGG;}
    |SUM_agg{$$=SUM_AGG;}
as:
  /* empty */
  {
    $$ = nullptr;
  }
  | AS ID {
    $$ = $2;
  }
  | ID {
    $$ = $1;
  }
  ;
load_data_stmt:
    LOAD DATA INFILE SSS INTO TABLE ID 
    {
      char *tmp_file_name = common::substr($4, 1, strlen($4) - 2);
      
      $$ = new ParsedSqlNode(SCF_LOAD_DATA);
      $$->load_data.relation_name = $7;
      $$->load_data.file_name = tmp_file_name;
      free($7);
      free(tmp_file_name);
    }
    ;

explain_stmt:
    EXPLAIN command_wrapper
    {
      $$ = new ParsedSqlNode(SCF_EXPLAIN);
      $$->explain.sql_node = std::unique_ptr<ParsedSqlNode>($2);
    }
    ;

set_variable_stmt:
    SET ID EQ value
    {
      $$ = new ParsedSqlNode(SCF_SET_VARIABLE);
      $$->set_variable.name  = $2;
      $$->set_variable.value = *$4;
      free($2);
      delete $4;
    }
    ;

opt_semicolon: /*empty*/
    | SEMICOLON
    ;
%%
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
