/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
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

#include "sql/stmt/update_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "common/lang/string.h"
UpdateStmt::  UpdateStmt(Table *table, std::map<int,Value>value_map, std::map<int,SelectStmt*> select_map,FilterStmt *filter_stmt, std:: vector<const FieldMeta *>field_meta)
    : table_ (table), value_map_(value_map),select_map_(select_map), filter_stmt_(filter_stmt), field_meta_(field_meta)
{}

RC UpdateStmt::create(Db *db,  UpdateSqlNode &update, Stmt *&stmt)
{
  const char *table_name = update.relation_name.c_str();
  auto updates = update.updateValue_list;
  if (db == nullptr || table_name == nullptr || updates.empty()) {
    LOG_WARN("invalid argument. db=%p, table_name=%p",
              db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  std::map<int,Value>values_map;
  std::map<int,SelectStmt*>select_map;

  if (table == nullptr) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check fields type（目前只支持单个字段）
  //Value value = update.value;
  const TableMeta &table_meta = table->table_meta();
  const int value_num = update.updateValue_list.size();
  std::vector<const FieldMeta *>field_list;
  const FieldMeta *field_meta;
  // Iterating over updates to check field validity
  for (int index = 0; index < value_num; ++index) {
      const FieldMeta *current_field_meta = table_meta.field(updates[index].attribute_name.c_str());
      field_list.push_back(current_field_meta);

      if (current_field_meta == nullptr) {
          LOG_WARN("Missing field in schema. table=%s, field=%s", table_name, updates[index].attribute_name.c_str());
          return RC::SCHEMA_FIELD_NOT_EXIST;
      }

      AttrType current_field_type = current_field_meta->type();
      if (updates[index].is_select) {
          Stmt *select_statement = nullptr;
          RC status = SelectStmt::create(db, updates[index].selectSqlNode, select_statement);
          if (status != RC::SUCCESS) {
              LOG_WARN("Select statement construction failed");
              return status;
          }
          select_map[index] = reinterpret_cast<SelectStmt*>(select_statement);
      } else {
          AttrType update_value_type = updates[index].value.attr_type();
          Value current_value = updates[index].value;
          if (current_field_type != update_value_type) {
              if (current_field_type == CHARS) {
                  if (update_value_type == INTS) {
                      std::string converted_value = common::int2string(current_value.get_int());
                      current_value.set_string(converted_value.c_str());
                      current_value.set_type(CHARS);
                  } else if (update_value_type == FLOATS) {
                      std::string converted_value = common::float2string(current_value.get_float());
                      current_value.set_string(converted_value.c_str());
                      current_value.set_type(CHARS);
                  } else {
                      if (update_value_type != NULLS || !current_field_meta->is_null()) {
                          LOG_WARN("Type conflict detected. table=%s, field=%s, expected=%d, found=%d",
                                  table_name, current_field_meta->name(), current_field_type, update_value_type);
                          return RC::SCHEMA_FIELD_TYPE_MISMATCH;
                      }
                  }
              } else if (current_field_type == INTS || current_field_type == FLOATS) {
                  if (update_value_type == CHARS) {
                      float numeric_value = common::string2float(current_value.get_string());
                      current_value.set_type(current_field_type);
                      current_value.set_float(numeric_value);
                      if (current_field_type == INTS) {
                          current_value.set_int(static_cast<int>(numeric_value));
                      }
                  } else if ((current_field_type == INTS && update_value_type == FLOATS) || (current_field_type == FLOATS && update_value_type == INTS)) {
                      float numeric_value = (update_value_type == INTS) ? common::int2float(current_value.get_int()) : static_cast<float>(current_value.get_int());
                      current_value.set_float(numeric_value);
                      current_value.set_type(current_field_type);
                  } else {
                      if (update_value_type != NULLS || !current_field_meta->is_null()) {
                          LOG_WARN("Incompatible data type. table=%s, field=%s, expected=%d, found=%d",
                                  table_name, current_field_meta->name(), current_field_type, update_value_type);
                          return RC::SCHEMA_FIELD_TYPE_MISMATCH;
                      }
                  }
              } else if (current_field_type != DATES || (update_value_type == NULLS && !current_field_meta->is_null())) {
                  LOG_WARN("Unsupported or mismatched field type. table=%s, field=%s, expected=%d, found=%d",
                          table_name, current_field_meta->name(), current_field_type, update_value_type);
                  return RC::SCHEMA_FIELD_TYPE_MISMATCH;
              }
          }
          if (update_value_type == DATES && !common::is_valid_date(current_value.data())) {
              LOG_WARN("Invalid date format. table=%s, field=%s, expected=%d, actual=%d",
                      table_name, current_field_meta->name(), current_field_type, update_value_type);
              return RC::INVALID_ARGUMENT;
          }
          values_map[index] = current_value;
      }
  }

  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table*>(table_name, table));

  RC rc = FilterStmt::create(db,
      table,
      std::string(),
      &table_map,
      update.conditions.data(),
      static_cast<int>(update.conditions.size()),
      std::unordered_map<std::string, Table *>(),
      filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }

  stmt = new UpdateStmt(table, values_map, select_map,filter_stmt, field_list);
  return RC::SUCCESS;
}
