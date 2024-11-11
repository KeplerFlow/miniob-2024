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
// Created by Wangyunlai on 2022/6/5.
//

#pragma once

#include <vector>
#include <memory>

#include "common/rc.h"
#include "sql/stmt/stmt.h"
#include "storage/field/field.h"

class FieldMeta;
class FilterStmt;
class Db;
class Table;

/**
 * @brief 表示select语句
 * @ingroup Statement
 */
class SelectStmt : public Stmt 
{
public:
  SelectStmt() = default;
  ~SelectStmt() override;

  StmtType type() const override
  {
    return StmtType::SELECT;
  }

public:
  static RC create(Db *db, const SelectSqlNode &select_sql, Stmt *&stmt,bool is_sub_select=false,std::unordered_map<std::string, Table *>top_tables=std::unordered_map<std::string, Table *>());

public:
  const std::vector<Table *> &tables() const
  {
    return tables_;
  }
  const std::vector<Field> &query_fields() const
  {
    return query_fields_;
  }
  const std::vector<RelAttrSqlNode> &attributes() const
  {
    return attributes_;
  }
  const std::vector<Field> &order_by_fields()const{
    return order_by_fields_;
  }
  const std::vector<OrderBySequence> &order_by_sequences()const{
    return  order_by_sequences_;
  }
  bool is_sub_select(){
    return is_sub_select_;
  }
  const  std::unordered_map<std::string, Table *>&top_tables()const{
    return  top_tables_;
  }
  bool is_agg(){
    return is_agg_;
  }
  FilterStmt *filter_stmt() const
  {
    return filter_stmt_;
  }
  std::unordered_map<std::string, std::string> alias_map()
  {
    return alias_map_;
  }
  std::unordered_map<std::string, std::string> col_alias_map()
  {
    return col_alias_map_;
  }
  std::vector<Expression*> &expression(){
    return attributes_expression_;
  }
  bool is_expression_select_attr(){
    return is_expression_select_attr_;
  }
  std::vector<Expression*> &all_expressions()
  {
    return all_expressions_;
  }
  bool is_group(){
    return is_group_;
  }
  std::vector<Field>&group_by_fields(){
    return group_by_fields_;
  }
  FilterStmt *having_filter_stmt()const{
    return having_filter_stmt_;
  }
  std::vector<RelAttrSqlNode> &having_rels(){
      return having_rels_;
  };
  std::vector<Field>&having_fields(){
      return having_fields_;
  }

private:
  std::vector<Field> query_fields_;
  std::vector<Table *> tables_;
  FilterStmt *filter_stmt_ = nullptr;
  bool is_agg_;
  std::vector<RelAttrSqlNode> attributes_;
  std::vector<Field>order_by_fields_;
  std::vector<OrderBySequence>order_by_sequences_;
  bool is_sub_select_;
  std::unordered_map<std::string, Table *>top_tables_;

  std::unordered_map<std::string, std::string> alias_map_;  //
  std::unordered_map<std::string, std::string> col_alias_map_;

  bool is_expression_select_attr_;
  std::vector<Expression*> attributes_expression_;
  std::vector<Expression*> all_expressions_;  // 用于无表达式无agg情况下的简单查询(Function)
  //std::vector<StringSqlExpr*>stringsqlExprs;//for agg
  //std::vector<FieldExpr*>fieldExprs; //for normal select

  bool is_group_;
  std::vector<Field>group_by_fields_;
  bool is_having_;
  FilterStmt *having_filter_stmt_= nullptr;
  std::vector<RelAttrSqlNode> having_rels_;
  std::vector<Field>having_fields_;
};
