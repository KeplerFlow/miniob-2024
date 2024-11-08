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

#include <vector>
#include <unordered_map>
#include "sql/parser/parse_defs.h"
#include "sql/stmt/stmt.h"
#include "sql/expr/expression.h"
#include "sql/stmt/select_stmt.h"
class Db;
class Table;
class FieldMeta;

struct FilterObj 
{
  ConditionValueType filter_type_;
  Field field;
  Value value;
  SelectStmt select;
  std::vector<Value>value_list;
  RelAttrSqlNode agg;
  Expression *expression;
  Func func_;
  LengthParam lengthparam_;
  RoundParam roundparam_;
  FormatParam formatparam_;

  void init_attr(const Field &field)
  {
    filter_type_ = ATTR_TYPE;
    this->func_ = NO_FUNC;
    this->field = field;
  }

  void init_value(const Value &value)
  {
    filter_type_ = VALUE_TYPE;
    this->func_ = NO_FUNC;
    this->value = value;
  }
  void init_select(const SelectStmt &selectStmt){
    filter_type_=SUB_SELECT_TYPE;
    this->func_ = NO_FUNC;
    this->select=selectStmt;
  }
  void init_value_list(const std::vector<Value>&value_list){
    filter_type_=VALUE_LIST_TYPE;
    this->func_ = NO_FUNC;
    this->value_list.insert(this->value_list.end(),value_list.begin(),value_list.end());
  }
  void init_expression( Expression *expression){
    filter_type_=EXPR_TYPE;
    this->func_ = NO_FUNC;
    this->expression=expression;
  }
  void init_func(const Field &field, const Func &func, const LengthParam &lengthparam)
  {
    filter_type_ = ATTR_TYPE;
    this->field = field;
    this->func_ = LENGTH_FUNC;
    this->lengthparam_ = lengthparam;
  }
  void init_func(const Field &field, const Func &func, const RoundParam &roundparam)
  {
    filter_type_ = ATTR_TYPE;
    this->field = field;
    this->func_ = ROUND_FUNC;
    this->roundparam_ = roundparam;
  }
  void init_func(const Field &field, const Func &func, const FormatParam &formatparam)
  {
    filter_type_ = ATTR_TYPE;
    this->field = field;
    this->func_ = FORMAT_FUNC;
    this->formatparam_ = formatparam;
  }
  void init_agg( const Field &field,const RelAttrSqlNode relAttrSqlNode){
    filter_type_=AGG_TYPE;
    this->func_ = NO_FUNC;
    this->field=field;
    this->agg=relAttrSqlNode;
  }
};

class FilterUnit 
{
public:
  FilterUnit() = default;
  ~FilterUnit()
  {}

  void set_comp(CompOp comp)
  {
    comp_ = comp;
  }

  CompOp comp() const
  {
    return comp_;
  }

  void set_left(const FilterObj &obj)
  {
    left_ = obj;
  }
  void set_right(const FilterObj &obj)
  {
    right_ = obj;
  }

  const FilterObj &left() const
  {
    return left_;
  }
  const FilterObj &right() const
  {
    return right_;
  }

private:
  CompOp comp_ = NO_OP;
  FilterObj left_;
  FilterObj right_;
};

/**
 * @brief Filter/谓词/过滤语句
 * @ingroup Statement
 */
class FilterStmt 
{
public:
  FilterStmt() = default;
  virtual ~FilterStmt();

public:
  const std::vector<FilterUnit *> &filter_units() const
  {
    return filter_units_;
  }
  bool is_and()const{
    return is_and_;
  }

public:
  static RC create(Db *db, Table *default_table, std::string default_table_alas,std::unordered_map<std::string, Table *> *tables,
      const ConditionSqlNode *conditions, int condition_num, std::unordered_map<std::string, Table *>top_tables,FilterStmt *&stmt);

  static RC create_filter_unit(Db *db, Table *default_table, std::string default_table_alas,std::unordered_map<std::string, Table *> *tables,
      const ConditionSqlNode &condition, std::unordered_map<std::string, Table *>top_tables,FilterUnit *&filter_unit);

private:
  std::vector<FilterUnit *> filter_units_;  // 默认当前都是AND关系
  bool is_and_;
};
