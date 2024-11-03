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
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/insert_stmt.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"
#include "storage/common/db.h"
#include "storage/common/table.h"
#include "util/date.h"
#include "util/util.h"
#include "util/ast_util.h"
#include <cassert>

RC InsertStmt::create(Db *db, Inserts &inserts, Stmt *&stmt)
{
  const char *target_table_name = inserts.relation_name;
  if (nullptr == db || nullptr == target_table_name || inserts.pair_num <= 0) {
    LOG_WARN("Invalid argument. db=%p, table_name=%p, pair_num=%d", 
             db, target_table_name, inserts.pair_num);
    return RC::INVALID_ARGUMENT;
  }

  // Check if the target table exists
  Table *target_table = db->find_table(target_table_name);
  if (nullptr == target_table) {
    LOG_WARN("No such table. db=%s, table_name=%s", db->name(), target_table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  for (size_t pair_idx = 0; pair_idx < inserts.pair_num; pair_idx++) {
    // Check the number of fields
    const int expr_num = inserts.pairs[pair_idx].expr_num;
    const TableMeta &table_meta = target_table->table_meta();
    const int field_num = table_meta.field_num() - table_meta.sys_field_num();
    
    if (field_num != expr_num) {
      LOG_WARN("Schema mismatch. Number of values=%d, number of fields in schema=%d", expr_num, field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }

    for (size_t expr_idx = 0; expr_idx < inserts.pairs[pair_idx].expr_num; expr_idx++) {
      // Check if the expression is valid
      if (!evaluate(inserts.pairs[pair_idx].exprs[expr_idx])) {
        LOG_WARN("Cannot evaluate expression in the insert statement");
        return RC::SQL_SYNTAX;
      }
    }
  }

  // Do typecasting
  for (size_t pair_idx = 0; pair_idx < inserts.pair_num; pair_idx++) {
    const int expr_num = inserts.pairs[pair_idx].expr_num;
    const TableMeta &table_meta = target_table->table_meta();
    const int sys_field_num = table_meta.sys_field_num();
    for (int field_idx = 0; field_idx < expr_num; field_idx++) {
      const FieldMeta *field_meta = table_meta.field(field_idx + sys_field_num);
      const AttrType field_type = field_meta->type();
      bool nullable = field_meta->nullable();
      Value &value = inserts.pairs[pair_idx].exprs[field_idx]->val;
      assert(value.type != DATES);
      assert(field_type != UNDEFINED);
      RC rc;
      if ((rc = try_to_cast_value(field_type, nullable, value)) != RC::SUCCESS) {
        return rc;
      }
    }
  }

  size_t expr_num = inserts.pairs[0].expr_num;
  // Create values
  std::vector<std::vector<Value>> value_pairs;
  value_pairs.resize(inserts.pair_num, std::vector<Value>{});
  for (size_t pair_idx = 0; pair_idx < inserts.pair_num; pair_idx++) {
    value_pairs[pair_idx].reserve(expr_num);
    for (size_t expr_idx = 0; expr_idx < expr_num; expr_idx++) {
      value_pairs[pair_idx].push_back(inserts.pairs[pair_idx].exprs[expr_idx]->val);
    }
  }

  // Everything is in order
  stmt = new InsertStmt(target_table, value_pairs, expr_num);
  return RC::SUCCESS;
}

