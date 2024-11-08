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
// Created by Wangyunlai on 2022/12/15
//

#include "sql/operator/project_logical_operator.h"

ProjectLogicalOperator::ProjectLogicalOperator(const std::vector<Field> &fields, std::unordered_map<std::string, std::string> &col_alias_map, std::unordered_map<std::string, std::string> &alias_map,
            std::vector<Expression*>&my_expression,std::vector<Expression*>&all_expressions)
    : fields_(fields), col_alias_map_(col_alias_map), alias_map_(alias_map),my_expressions_(my_expression),all_expressions_(all_expressions)
{}
