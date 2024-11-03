/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmt.h"
#include "sql/expr/expression.h"
#include "sql/parser/parse_defs.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "util/util.h"
#include "util/ast_util.h"
#include "util/check.h"
#include <set>

SelectStmt::~SelectStmt()
{
  for (Expression *expr : exprs_) {
    delete expr;
  }
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
  for (Expression * order_expr : order_exprs_) {
    delete order_expr;
  }
  for (Expression * group_by : group_bys_) {
    delete group_by;
  }
}

void SelectStmt::Print() const
{
  printf("------------------\n");
  auto print_units = [](const std::vector<FilterUnit *> &units) {
    for (FilterUnit *unit : units) {
      if (unit->condition_type() == ConditionType::COND_COMPARE) {
        printf("comp: %s\n", comp_to_string(unit->comp()).c_str());
        print_expr(unit->left());
        print_expr(unit->right());
      } else if (unit->condition_type() == ConditionType::COND_EXISTS) {
        printf("exists\n");
      } else {
        print_expr(unit->left());
        printf("in subquery\n");
      }
    }
  };
  printf("--- table info ---\n");
  for (Table *t : tables_) {
    printf("name: %s\n", t->name());
  }
  printf("--- join info  ---\n");
  for (const JoinStmt &join : join_stmts_) {
    printf("join on table: %s\n", join.join_table->name());
    print_units(join.filter_stmt->filter_units());
  }
  if (filter_stmt_ != nullptr) {
    printf("--- cond info  ---\n");
    print_units(filter_stmt_->filter_units());
  }
  printf("--- exprs info ---\n");
  for (Expression *expr : exprs_) {
    print_expr(expr);
  }
}

static RC check_selects(Selects &select_stmts, size_t &attr_count, size_t &aggr_count, ExprContext &context)
{
  attr_count = 0;
  aggr_count = 0;
  std::vector<ast *> attr_nodes;
  std::vector<ast *> aggr_nodes;
  
  for (size_t i = 0; i < select_stmts.group_by_length; i++) {
    ast *group_by = select_stmts.group_bys[i];
    if (group_by->nodetype != NodeType::ATTRN) {
      LOG_WARN("Using expressions other than attributes in group by is not allowed");
      return RC::SQL_SYNTAX;
    }
  }

  for (size_t i = 0; i < select_stmts.expr_num; i++) {
    size_t current_attr_count = 0;
    size_t current_aggr_count = 0;
    ast *t = select_stmts.exprs[i];
    RC rc = check_leaf_node(t, context, current_attr_count, current_aggr_count);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    if (current_attr_count > 0) {
      attr_nodes.push_back(select_stmts.exprs[i]);
    }
    if (current_aggr_count > 0) {
      aggr_nodes.push_back(select_stmts.exprs[i]);
    }
    attr_count += current_attr_count;
    aggr_count += current_aggr_count;
  }

  if (aggr_count > 0 && attr_count > 0) {
    // Check if all attributes are in group by
    for (ast *t : attr_nodes) {
      bool found = false;
      for (size_t i = 0; i < select_stmts.group_by_length; i++) {
        ast *group_by = select_stmts.group_bys[i];
        if (strcmp(group_by->attr.attribute_name, t->attr.attribute_name) == 0) {
          found = true;
          break;
        }
      }
      if (found == false) {
        LOG_WARN("Attributes (not in group by) cannot be selected with aggregates together");
        return RC::SQL_SYNTAX;
      }
    }
  }
  
  return RC::SUCCESS;
}

static void wildcard_fields(Table *table, std::vector<Expression *> &output_exprs, std::vector<const char *> &output_field_alias)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_count = table_meta.field_num();
  
  for (int i = table_meta.sys_field_num(); i < field_count; i++) {
    output_exprs.push_back(new FieldExpr(table, table_meta.field(i)));
    output_field_alias.push_back(nullptr);
  }
}

static void collect_attr_expr(ast *t, const ExprContext &ctx, std::vector<Expression *> &exprs, std::vector<const char *> &field_alias)
{
  assert(t->nodetype == NodeType::ATTRN);
  const RelAttr &attr = t->attr;
  if (strcmp(attr.attribute_name, "*") == 0) {
    if (common::is_blank(attr.relation_name) || strcmp(attr.relation_name, "*") == 0){
      for (Table *table : ctx.GetTables()) {
        wildcard_fields(table, exprs, field_alias);
      }
    } else {
      Table *table = ctx.GetTable(attr);
      wildcard_fields(table, exprs, field_alias);
    }
  } else {
    exprs.push_back(ExprFactory::create(t, ctx));
  }
}

static void collect_exprs(Selects &select_sql, const ExprContext &ctx, std::vector<Expression *> &exprs, std::vector<const char *> &field_alias)
{
  for (size_t i = 0; i < select_sql.expr_num; i++) {
    ast *t = select_sql.exprs[i];
    assert(t->nodetype != NodeType::UNDEFINEDN);
    if (t->nodetype == NodeType::ATTRN) {
      collect_attr_expr(t, ctx, exprs, field_alias);
      if (strcmp(t->attr.attribute_name, "*") != 0) {
        field_alias.push_back(select_sql.expr_alias[i]);
      }
    } else {
      exprs.push_back(ExprFactory::create(t, ctx));
      field_alias.push_back(select_sql.expr_alias[i]);
    }
  }
}

static RC collect_tables(Db *db, Selects &select_sql, ExprContext &ctx, std::unordered_map<std::string, std::string> &table_alias)
{
  std::set<std::string> aliases;
  for (int i = 0; i < select_sql.relation_num; i++) {
    const char *table_name = select_sql.relations[i];
    const char *alias = select_sql.relation_alias[i];
    if (nullptr == table_name) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }
    ctx.AddTable(table);

    if (nullptr != alias) {
      if (aliases.count(alias)) {
        LOG_WARN("duplicate table alias name");
        return RC::SQL_SYNTAX;
      }
      aliases.emplace(alias);
      ctx.SetAlias(table_name, alias);
      table_alias[table_name] = alias;
    }
  }
  return RC::SUCCESS;
}

static RC collect_join_stmts(Db *db, Selects &select_sql, Table *default_table, std::vector<JoinStmt> &join_stmts)
{
  ExprContext join_ctx(default_table);

  for (size_t i = 0; i < select_sql.join_num; i++) {
    Join &join = select_sql.joins[i];
    JoinStmt join_stmt;
    Table *join_table = db->find_table(join.join_table_name);
    if (join_table == nullptr) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }
    join_ctx.AddTable(join_table);
    join_stmt.join_table = join_table;

    FilterStmt *join_filter;
    RC rc = FilterStmt::create(db, join_ctx, join.conditions, join.condition_num, join_filter);
    if (rc != RC::SUCCESS) {
      LOG_WARN("fail to create filter for join on table %s\n", join.join_table_name);
      return rc;
    }
    join_stmt.filter_stmt = join_filter;

    join_stmts.push_back(join_stmt);
  }
  return RC::SUCCESS;
}

static RC check_only_attr(ast *t, ExprContext &ctx)
{
  size_t attr_cnt = 0;
  size_t aggr_cnt = 0;
  RC rc = check_leaf_node(t, ctx, attr_cnt, aggr_cnt);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  if (aggr_cnt > 0) {
    LOG_WARN("Cannot use aggregate in order by or group by");
    return RC::SQL_SYNTAX;
  }
  return RC::SUCCESS;
}

static RC check_having(Condition *condition)
{
  NodeType ltype = condition->left_ast->nodetype;
  NodeType rtype = condition->right_ast->nodetype;
  if (ltype != NodeType::AGGRN && rtype != NodeType::AGGRN) {
    LOG_WARN("having must have aggregate");
    return RC::SQL_SYNTAX;
  }
  return RC::SUCCESS;
}


RC SelectStmt::create_with_context(Db *db, Selects &select_sql, Stmt *&stmt, ExprContext &select_ctx)
{
  size_t outer_table_size = select_ctx.GetTableSize();

  if (db == nullptr)
  {
    LOG_WARN("Invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }

  std::unordered_map<std::string, std::string> table_alias;
  RC rc = collect_tables(db, select_sql, select_ctx, table_alias);
  if (rc != RC::SUCCESS)
  {
    return rc;
  }

  if (select_ctx.GetTableSize() == 0)
  {
    std::vector<Expression *> select_exprs;
    std::vector<const char *> select_field_alias;
    collect_exprs(select_sql, select_ctx, select_exprs, select_field_alias);
    assert(select_exprs.size() == select_field_alias.size());

    SelectStmt *select_stmt = new SelectStmt();
    select_stmt->exprs_.swap(select_exprs);
    select_stmt->table_alias_ = table_alias;
    select_stmt->field_alias_ = select_field_alias;
    stmt = select_stmt;

    select_stmt->Print();
    return RC::SUCCESS;
  }

  if (select_sql.join_num > 0 && select_ctx.GetTableSize() > 1)
  {
    LOG_ERROR("Joining with more than one select table is not implemented yet");
     return RC::UNIMPLENMENT;
  }

  std::vector<JoinStmt> join_stmts;
  rc = collect_join_stmts(db, select_sql, select_ctx.GetDefaultTable(), join_stmts);
  if (rc != RC::SUCCESS)
  {
    return rc;
  }

  for (JoinStmt &join : join_stmts)
  {
    select_ctx.AddTable(join.join_table);
  }

  size_t attribute_count = 0;
  size_t aggregate_count = 0;
  rc = check_selects(select_sql, attribute_count, aggregate_count, select_ctx);
  if (rc != RC::SUCCESS)
  {
    return rc;
  }

  bool is_selecting_attributes = (aggregate_count == 0);

  std::vector<Expression *> select_exprs;
  std::vector<const char *> select_field_alias;
  collect_exprs(select_sql, select_ctx, select_exprs, select_field_alias);
  assert(select_exprs.size() == select_field_alias.size());

  FilterStmt *where_filter_stmt = nullptr;
  rc = FilterStmt::create(db, select_ctx, select_sql.conditions, select_sql.condition_num, where_filter_stmt);
  if (rc != RC::SUCCESS)
  {
    LOG_WARN("Failed to construct filter statement");
    return rc;
  }

  for (size_t i = 0; i < select_sql.order_attr_length; i++)
  {
    rc = check_only_attr(select_sql.order_attr[i], select_ctx);
    if (rc != RC::SUCCESS)
    {
      return rc;
    }
  }

  std::vector<Expression *> order_exprs;
  std::vector<OrderPolicy> order_policies;
  for (size_t i = 0; i < select_sql.order_attr_length; i++)
  {
    order_exprs.push_back(ExprFactory::create(select_sql.order_attr[i], select_ctx));
    order_policies.push_back(select_sql.order_policy[i]);
  }

  for (size_t i = 0; i < select_sql.group_by_length; i++)
  {
    rc = check_only_attr(select_sql.group_bys[i], select_ctx);
    if (rc != RC::SUCCESS)
    {
      return rc;
    }
  }

  std::vector<Expression *> group_by_exprs;
  for (size_t i = 0; i < select_sql.group_by_length; i++)
  {
    group_by_exprs.push_back(ExprFactory::create(select_sql.group_bys[i], select_ctx));
  }

  FilterUnit *having_filter_unit = nullptr;
  if (select_sql.is_having)
  {
    RC rc = check_having(&select_sql.having);
    if (rc != RC::SUCCESS)
    {
      return rc;
    }
    rc = FilterStmt::create_filter_unit(db, select_ctx, select_sql.having, having_filter_unit, true);
    if (rc != RC::SUCCESS)
    {
      return rc;
    }
  }

  std::vector<Table *> select_tables;
  for (size_t i = outer_table_size; i < select_ctx.GetTableSize(); i++)
  {
    select_tables.push_back(select_ctx.GetTables()[i]);
  }

  SelectStmt *select_stmt = new SelectStmt();
  select_stmt->tables_.swap(select_tables);
  select_stmt->join_stmts_.swap(join_stmts);
  select_stmt->exprs_.swap(select_exprs);
  select_stmt->filter_stmt_ = where_filter_stmt;
  select_stmt->select_attributes_ = is_selecting_attributes;
  select_stmt->order_exprs_ = order_exprs;
  select_stmt->order_policies_ = order_policies;
  select_stmt->group_bys_ = group_by_exprs;
  select_stmt->having_ = having_filter_unit;
  select_stmt->table_alias_ = table_alias;
  select_stmt->field_alias_ = select_field_alias;
  stmt = select_stmt;

  select_stmt->Print();
  return RC::SUCCESS;
}

RC SelectStmt::create(Db *db, Selects &select_sql, Stmt *&stmt)
{
  ExprContext select_ctx;
  return create_with_context(db, select_sql, stmt, select_ctx);
}
