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
// Created by wangyunlai on 2022/9/28
//

#pragma once

#include "sql/expr/expression.h"
#include <string>

// 字符串处理
std::string double2string(double v);
std::string value2string(const Value &value);
std::string expr_to_string(Expression *expr, bool is_multi_table);

// 值处理
Value value_copy(const Value &value);
RC try_to_cast_value(AttrType to_type, bool nullable, Value &value);
bool is_numeric_type(AttrType type);
size_t type_length(AttrType type, void *data);

// 聚合函数处理
std::string aggregate_func_string(AggrType aggr_type);

// 表达式处理
std::string mathop_to_string(MathOp mathop);
std::string comp_to_string(CompOp comp);
void print_expr(Expression *expr, int level=0);
