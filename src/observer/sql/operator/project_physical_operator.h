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
// Created by WangYunlai on 2022/07/01.
//

#pragma once

#include "sql/operator/physical_operator.h"

/**
 * @brief 选择/投影物理算子
 * @ingroup PhysicalOperator
 */
class ProjectPhysicalOperator : public PhysicalOperator
{
public:
  ProjectPhysicalOperator()
  {}

  virtual ~ProjectPhysicalOperator() = default;

  void add_expressions(std::vector<std::unique_ptr<Expression>> &expressions)
  {
    expressions_ = std::move(expressions);
  }
  void add_projection(const Table *table, const FieldMeta *field, const std::unordered_map<std::string, std::string> &col_alias_map, const std::unordered_map<std::string, std::string> &alias_map);
  void add_my_expressions(std::vector<Expression*>&my_expressions);

  void add_all_expressions(std::vector<Expression*> &all_expressions)
  {
    all_expressions_ = all_expressions;
  } 

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::PROJECT;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  int cell_num() const
  {
    if(my_expressions_.size()>0){
      return my_expressions_.size();
    }
    return tuple_.cell_num();
  }

  Tuple *current_tuple() override;

private:
  ProjectTuple tuple_;
  std::vector<Expression*>my_expressions_;
  ValueListTuple valueListTuple_;
  std::vector<std::unique_ptr<Expression>> expressions_; //用于无表查询Function
  bool withoutTable_EOF_flag = false;
  // ValueListTuple generate_tuple_;
  std::vector<Expression*> all_expressions_; // 用于无表达式无agg情况下的简单查询(Function)

};
