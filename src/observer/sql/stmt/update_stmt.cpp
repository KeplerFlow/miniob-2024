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
  for (int i=0; i<value_num; ++i) {
    field_meta = table_meta.field(updates[i].attribute_name.c_str());
    field_list.push_back(field_meta);
    if (field_meta == nullptr) {
      LOG_WARN("field does not exist. table=%s, field=%s", table_name, updates[i].attribute_name.c_str());
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }
    const AttrType field_type = field_meta->type();
    if(updates[i].is_select){
      Stmt * selectStmt= nullptr;
      RC rc = SelectStmt::create(db,
          updates[i].selectSqlNode,
          selectStmt);
      if (rc != RC::SUCCESS) {
        LOG_WARN("cannot construct select stmt");
        return rc;
      }
      select_map[i]=reinterpret_cast<SelectStmt*>(selectStmt);

    }else{
      const AttrType value_type = updates[i].value.attr_type();
      auto value=updates[i].value;
      if (field_type != value_type) {
        if(field_type==CHARS){
          if(value_type==INTS){
            auto s= common::int2string(value.get_int());
            value.set_type(CHARS);
            value.set_string(s.c_str());
          }else if(value_type==FLOATS){
            auto s= common::float2string(value.get_float());
            value.set_type(CHARS);
            value.set_string(s.c_str());
          }else if(value_type==NULLS){
            if(!field_meta->is_null()){
              LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
              return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            }
          }else{
            LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
            return RC::SCHEMA_FIELD_TYPE_MISMATCH;

          }
        }else if(field_type==INTS){
          if(value_type==CHARS){
            auto d= common::string2float(value.get_string());
            auto integer= common::float2int(d);
            value.set_type(INTS);
            value.set_int(integer);
          }else if(value_type==FLOATS){
            auto integer= common::float2int(value.get_float());
            value.set_type(INTS);
            value.set_int(integer);

          }else if(value_type==NULLS){
            if(!field_meta->is_null()){
              LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
              return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            }
          }else{
            LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
            return RC::SCHEMA_FIELD_TYPE_MISMATCH;
          }
        }else if(field_type==FLOATS){
          if(value_type==INTS){
            auto d= common::int2float(value.get_int());
            value.set_type(FLOATS);
            value.set_float(d);
          }else if(value_type==CHARS){
            auto d= common::string2float(value.get_string());
            value.set_type(FLOATS);
            value.set_float(d);
          }else if(value_type==NULLS){
            if(!field_meta->is_null()){
              LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
              return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            }
          }else{
            LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
            return RC::SCHEMA_FIELD_TYPE_MISMATCH;
          }
        }else if(field_type==DATES){
          if(value_type==NULLS){
            if(!field_meta->is_null()){
              LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
              return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            }
          }else{
            LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
              table_name, field_meta->name(), field_type, value_type);
            return RC::SCHEMA_FIELD_TYPE_MISMATCH;
          }
        }else if(field_type==TEXTS){
          if(value_type==CHARS){
            if(value.length()>65535){
              return RC::INVALID_ARGUMENT;
            }
            char *text=(char *)malloc(value.length()+1);
            memcpy(text,value.get_string().c_str(),value.length());
            text[value.length()]='\0';
            value.set_text(text);
          }
        }else{
          LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
            table_name, field_meta->name(), field_type, value_type);
          return RC::SCHEMA_FIELD_TYPE_MISMATCH;
        }


    }
    if(value_type==DATES&&!common::is_valid_date(value.data())){
        LOG_WARN("date error. table=%s, field=%s, field type=%d, value_type=%d",
          table_name, field_meta->name(), field_type, value_type);
        return RC::INVALID_ARGUMENT;
    }
    values_map[i]=value;

    }
    // if (field_meta->type() == DATES) {
    //   int time_distance = *(int *)value.data;
    //   if (!date_is_legal(time_distance)) {
    //     LOG_WARN("Insert value illegal for type \'%s\'.", "DATE");
    //     return RC::SCHEMA_FIELD_MISSING;
    //   }  
    // }
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
