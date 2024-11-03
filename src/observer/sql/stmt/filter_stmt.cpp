/* Copyright (c) 2021T FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "util/ast_util.h"
#include "util/date.h"
#include "util/check.h"
#include "util/util.h"
#include <cassert>
#include "rc.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/expr/expression.h"
#include "sql/expr/tuple.h"
#include "sql/operator/subselect_operator.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/db.h"
#include "storage/common/table.h"


static RC check_condition(Condition &condition, ExprContext &ctx, bool is_having)
{
  if (condition.condition_type == COND_COMPARE) {
    CompOp comp = condition.comp;
    if (comp == IS || comp == IS_NOT) {
      if (condition.right_is_select || condition.left_is_select) {
        return RC::SQL_SYNTAX;
      }
      if (condition.left_ast->nodetype != NodeType::VALN) {
        return RC::SQL_SYNTAX;
      }
      if (condition.right_ast->nodetype != NodeType::VALN) {
        return RC::SQL_SYNTAX;
      }
      if (condition.left_ast->val.type != AttrType::NULLS) {
        return RC::SQL_SYNTAX;
      }
      if (condition.right_ast->val.type != AttrType::NULLS) {
        return RC::SQL_SYNTAX;
      }
    }
    size_t attr_count_left = 0;
    size_t aggr_count_left = 0;
    if (!condition.left_is_select) {
      ast *left_ast = condition.left_ast;
      RC rc = check_leaf_node(left_ast, ctx, attr_count_left, aggr_count_left);
      if (rc != RC::SUCCESS) {
        return rc;
      }
    }

    size_t attr_count_right = 0;
    size_t aggr_count_right = 0;
    if (!condition.right_is_select) {
      ast *right_ast = condition.right_ast;
      RC rc = check_leaf_node(right_ast, ctx, attr_count_right, aggr_count_right);
      if (rc != RC::SUCCESS) {
        return rc;
      }

      if ((aggr_count_left + aggr_count_right) > 0 && is_having == false) {
        LOG_WARN("Aggregation is not allowed in the condition\n");
        return RC::SQL_SYNTAX;
      }
    }
  } else if (condition.condition_type == COND_IN) {
    size_t attr_count_left = 0;
    size_t aggr_count_left = 0;
    ast *left_ast = condition.left_ast;
    RC rc = check_leaf_node(left_ast, ctx, attr_count_left, aggr_count_left);
    if (rc != RC::SUCCESS) {
      return rc;
    }
  }
  return RC::SUCCESS;
}

// SELECT * FROM date_table WHERE u_date='2017-2-29';
// FAILURE
static RC check_date_valid(Expression *left, Expression *right)
{
  if (left->type() == ExprType::VALUE && right->type() == ExprType::FIELD) {
    Expression *tmp = left;
    left = right;
    right = tmp;
  }
  if (left->type() == ExprType::FIELD && right->type() == ExprType::VALUE) {
    FieldExpr *left_field = static_cast<FieldExpr *>(left);
    ValueExpr *right_value = static_cast<ValueExpr *>(right);
    if (left_field->field().attr_type() == DATES) {
      RowTuple unused;
      TupleCell cell;
      right_value->get_value(unused, cell);
      int32_t date;
      RC rc;
      if (cell.attr_type() == CHARS && (rc = string_to_date(cell.data(), date)) != RC::SUCCESS) {
        return rc;
      }
    }
  }
  return RC::SUCCESS;
}

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, ExprContext &ctx, Condition *conditions, int condition_num, FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;
    rc = create_filter_unit(db, ctx, conditions[i], filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC FilterStmt::create_filter_unit(Db *db, ExprContext &ctx, Condition &condition, FilterUnit *&filter_unit, bool is_having)
{
  RC result = RC::SUCCESS;

  CompOp comparison_op = condition.comp;
  if (comparison_op < EQUAL_TO || comparison_op >= NO_OP) {
    LOG_WARN("Invalid compare operator: %d", comparison_op);
    return RC::INVALID_ARGUMENT;
  }

  result = check_condition(condition, ctx, is_having);
  if (result != RC::SUCCESS) {
    return result;
  }

  switch (condition.condition_type) {
    case COND_COMPARE:
      return create_filter_unit_compare(db, ctx, condition, filter_unit);
    case COND_IN:
      return create_filter_unit_in(db, ctx, condition, filter_unit);
    default:
      return create_filter_unit_exists(db, ctx, condition, filter_unit);
  }
}


RC FilterStmt::create_filter_unit_compare(Db *db, ExprContext &ctx, Condition &condition, FilterUnit *&filter_unit)
{
  RC result = RC::SUCCESS;
  Expression *left_expr = nullptr;
  Expression *right_expr = nullptr;
  Stmt *left_select_stmt = nullptr;
  Stmt *right_select_stmt = nullptr;
  bool use_left_select = false;
  bool use_right_select = false;

  if (condition.left_is_select) {
    if (condition.left_select->expr_num != 1) {
      LOG_WARN("Fetching more than 1 field in a subquery is incorrect");
      return RC::SQL_SYNTAX;
    }
    result = SelectStmt::create(db, *condition.left_select, left_select_stmt);
    if (result == RC::SUCCESS) {
      SubSelectOperator left_select_operator(static_cast<SelectStmt *>(left_select_stmt), nullptr);
      TupleCell left_cell;
      result = left_select_operator.GetOneResult(left_cell);
      if (result != RC::SUCCESS) {
        return result;
      }
      left_expr = new ValueExpr(left_cell.to_value());
      delete left_select_stmt;
    } else {
      ExprContext correlated_ctx;
      correlated_ctx.SetCorrelation(ctx);
      result = SelectStmt::create_with_context(db, *condition.left_select, left_select_stmt, correlated_ctx);
      if (result != RC::SUCCESS) {
        return result;
      }
      use_left_select = true;
    }
  } else {
    left_expr = ExprFactory::create(condition.left_ast, ctx);
  }
  if (condition.right_is_select) {
    if (condition.right_select->expr_num != 1) {
      LOG_WARN("Fetching more than 1 field in a subquery is incorrect");
      return RC::SQL_SYNTAX;
    }
    result = SelectStmt::create(db, *condition.right_select, right_select_stmt);
    if (result == RC::SUCCESS) {
      SubSelectOperator right_select_operator(static_cast<SelectStmt *>(right_select_stmt), nullptr);
      TupleCell right_cell;
      result = right_select_operator.GetOneResult(right_cell);
      if (result != RC::SUCCESS) {
        return result;
      }
      right_expr = new ValueExpr(right_cell.to_value());
      delete right_select_stmt;
    } else {
      ExprContext correlated_ctx;
      correlated_ctx.SetCorrelation(ctx);
      result = SelectStmt::create_with_context(db, *condition.right_select, right_select_stmt, correlated_ctx);
      if (result != RC::SUCCESS) {
        return result;
      }
      use_right_select = true;
    }
  } else {
    right_expr = ExprFactory::create(condition.right_ast, ctx);
  }

  if (left_expr && right_expr) {
    result = check_date_valid(left_expr, right_expr);
    if (result != RC::SUCCESS) {
      delete left_expr;
      delete right_expr;
      return result;
    }
  }

  filter_unit = new FilterUnit();
  if (use_left_select) {
    assert(left_select_stmt != nullptr);
    filter_unit->set_left_select(static_cast<SelectStmt *>(left_select_stmt));
  } else {
    filter_unit->set_left(left_expr);
  }
  if (use_right_select) {
    filter_unit->set_right_select(static_cast<SelectStmt *>(right_select_stmt));
  } else {
    filter_unit->set_right(right_expr);
  }
  filter_unit->set_comp(condition.comp);
  filter_unit->set_condition_type(condition.condition_type);
  filter_unit->set_condition_op(condition.condop);
  return RC::SUCCESS;
}


RC FilterStmt::create_filter_unit_exists(Db *db, ExprContext &ctx, Condition &condition, FilterUnit *&filter_unit)
{
  RC result = RC::SUCCESS;
  assert(condition.condition_type == ConditionType::COND_EXISTS);
  assert(condition.left_select != nullptr);
  Stmt *sub_select_stmt = nullptr;
  result = SelectStmt::create(db, *condition.left_select, sub_select_stmt);
  if (result == RC::SUCCESS) {
    SubSelectOperator sub_select_operator(static_cast<SelectStmt *>(sub_select_stmt), nullptr);
    std::vector<TupleCell> cells;
    bool exists;
    result = sub_select_operator.HasResult(exists);
    if (result != RC::SUCCESS) {
      return result;
    }
    filter_unit = new FilterUnit();
    filter_unit->set_exists(exists);
  } else {
    ExprContext correlated_ctx;
    correlated_ctx.SetCorrelation(ctx);
    result = SelectStmt::create_with_context(db, *condition.left_select, sub_select_stmt, correlated_ctx);
    if (result != RC::SUCCESS) {
      LOG_WARN("Not a correlated subquery, syntax error");
      return result;
    }
    filter_unit = new FilterUnit();
    filter_unit->set_sub_select(static_cast<SelectStmt *>(sub_select_stmt));
  }

  if (!filter_unit->is_correlated()) {
    delete sub_select_stmt;
  }
  filter_unit->set_comp(condition.comp);
  filter_unit->set_condition_type(condition.condition_type);
  filter_unit->set_condition_op(condition.condop);
  return RC::SUCCESS;
}


RC FilterStmt::create_filter_unit_in(Db *db, ExprContext &ctx, Condition &condition, FilterUnit *&filter_unit)
{
  RC result = RC::SUCCESS;
  std::vector<Value> new_values;
  bool using_sub_select = false;
  Stmt *sub_select_stmt = nullptr;
  assert(condition.left_ast != nullptr);
  if (condition.expr_num > 0) {
    assert(condition.left_select == nullptr);
    for (size_t i = 0; i < condition.expr_num; i++) {
      assert(condition.exprs[i] != nullptr);
      if (!evaluate(condition.exprs[i])) {
        LOG_WARN("Cannot evaluate %dth expression in IN value list", i);
        return SQL_SYNTAX;
      }
      new_values.reserve(condition.expr_num);
      for (size_t i = 0; i < condition.expr_num; i++) {
        new_values.push_back(value_copy(condition.exprs[i]->val));
      }
    }
  } else {
    // Use subquery
    assert(condition.expr_num == 0);
    assert(condition.left_select != nullptr);
    result = SelectStmt::create(db, *condition.left_select, sub_select_stmt);
    if (result == RC::SUCCESS) {
      SubSelectOperator sub_select_operator(static_cast<SelectStmt *>(sub_select_stmt), nullptr);
      std::vector<TupleCell> cells;
      result = sub_select_operator.GetResultList(cells);
      if (result != RC::SUCCESS) {
        return result;
      }
      new_values.reserve(cells.size());
      for (const TupleCell &cell : cells) {
        Value value = cell.to_value();
        new_values.push_back(value);
      }
    } else {
      // Correlated subquery
      ExprContext correlated_ctx;
      correlated_ctx.SetCorrelation(ctx);
      result = SelectStmt::create_with_context(db, *condition.left_select, sub_select_stmt, correlated_ctx);
      if (result != RC::SUCCESS) {
        LOG_WARN("Not a correlated subquery, syntax error");
        return result;
      }
      using_sub_select = true;
    }
  }
  filter_unit = new FilterUnit();
  if (using_sub_select) {
    filter_unit->set_sub_select(static_cast<SelectStmt *>(sub_select_stmt));
  } else {
    filter_unit->set_cells(new_values);
    delete sub_select_stmt;
  }
  filter_unit->set_left(ExprFactory::create(condition.left_ast, ctx));
  filter_unit->set_comp(condition.comp);
  filter_unit->set_condition_type(condition.condition_type);
  filter_unit->set_condition_op(condition.condop);
  return RC::SUCCESS;
}
