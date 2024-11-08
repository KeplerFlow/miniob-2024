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
// Created by WangYunlai on 2022/12/08.
//

#pragma once

#include <vector>
#include <memory>

#include "sql/operator/logical_operator.h"
#include "sql/expr/expression.h"
#include "storage/field/field.h"

/**
 * @brief project 表示投影运算
 * @ingroup LogicalOperator
 * @details 从表中获取数据后，可能需要过滤，投影，连接等等。
 */
class ProjectLogicalOperator : public LogicalOperator 
{
public:
  ProjectLogicalOperator(const std::vector<Field> &fields, std::unordered_map<std::string, std::string> &col_alias_map, std::unordered_map<std::string, std::string> &alias_map,
            std::vector<Expression*>&my_expression, std::vector<Expression*> &all_expressions);
  virtual ~ProjectLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::PROJECTION;
  }

  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }
  const std::vector<std::unique_ptr<Expression>> &expressions() const
  {
    return expressions_;
  }
  const std::vector<Field> &fields() const
  {
    return fields_;
  }
  const std::unordered_map<std::string, std::string> &col_alias_map() const
  {
    return col_alias_map_;
  }
  const std::unordered_map<std::string, std::string> &alias_map() const
  {
    return alias_map_;
  }
   std::vector<Expression*>&my_expressions(){
    return my_expressions_;
  }
  std::vector<Expression*> &all_expressions()
  {
    return all_expressions_;
  }

private:
  //! 投影映射的字段名称
  //! 并不是所有的select都会查看表字段，也可能是常量数字、字符串，
  //! 或者是执行某个函数。所以这里应该是表达式Expression。
  //! 不过现在简单处理，就使用字段来描述
  std::vector<Field> fields_;
  std::unordered_map<std::string, std::string> col_alias_map_;
  std::unordered_map<std::string, std::string> alias_map_;
  std::vector<Expression*>my_expressions_; //hxy的
  std::vector<Expression*> all_expressions_; // 用于无表达式无agg情况下的简单查询(Function)
};
