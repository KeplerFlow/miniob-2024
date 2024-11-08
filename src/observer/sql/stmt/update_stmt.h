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

#pragma once

#include "common/rc.h"
#include "sql/stmt/stmt.h"
#include "storage/field/field_meta.h"
#include "sql/stmt/select_stmt.h"
#include <map>
class Table;
class FilterStmt;

/**
 * @brief 更新语句
 * @ingroup Statement
 */
class UpdateStmt : public Stmt 
{
public:
  UpdateStmt() = default;
  ~UpdateStmt(){
    /*for(auto select:select_map_){
      select.second->filter_stmt();
    }
    select_map_.clear();*/
  }
  // UpdateStmt(Table *table, Value *values, int value_amount);
  UpdateStmt(Table *table, std::map<int,Value>value_map, std::map<int,SelectStmt*> select_map,FilterStmt *filter_stmt, std:: vector<const FieldMeta *>field_meta);

  StmtType type() const override { return StmtType::UPDATE; }

public:
  static RC create(Db *db,  UpdateSqlNode &update_sql, Stmt *&stmt);

public:
  Table *table() const
  {
    return table_;
  }
  std::map<int,Value> value_map() const { return value_map_; }
  std::map<int,SelectStmt*> select_map() const { return select_map_; }

  FilterStmt *filter_stmt() const { return filter_stmt_; } 
  std:: vector<const FieldMeta *> field_meta() const { return field_meta_; }

private:
  Table *table_ = nullptr;
  std::map<int,SelectStmt*> select_map_;
  std::map<int,Value>value_map_;
  FilterStmt *filter_stmt_ = nullptr;
  std:: vector<const FieldMeta*> field_meta_;
};
