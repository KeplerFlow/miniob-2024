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

#include "common/log/log.h"
#include "sql/operator/project_physical_operator.h"
#include "storage/record/record.h"
#include "storage/table/table.h"
#include <cmath>
#include "sql/stmt/utils.h"

RC ProjectPhysicalOperator::open(Trx *trx)
{
  if (children_.empty()) {
    return RC::SUCCESS;
  }

  PhysicalOperator *child = children_[0].get();
  RC rc = child->open(trx);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC ProjectPhysicalOperator::next()
{
  RC rc;
  if (children_.empty() && expressions_.size() == 0) {
    return RC::RECORD_EOF;
  } else if (expressions_.size() != 0) {
    // 无表查询
    if (withoutTable_EOF_flag) {
      return RC::RECORD_EOF;
    }
    withoutTable_EOF_flag = true;
    if (children_.empty()) {
      // 无where
      return RC::SUCCESS; //直接在current_tuple()返回计算结果
    } else {
      return children_[0]->next();
    }
  }

  if(RC::SUCCESS!=(rc=children_[0]->next())){
    return rc;
  }else{
    if(!my_expressions_.empty()){
      std::vector<Value>values(my_expressions_.size());

      for(int i=0;i<my_expressions_.size();i++){
        if(my_expressions_[i]->get_value(*children_[0]->current_tuple(),values[i])!=RC::SUCCESS){
          return RC::INTERNAL;
        }
      }
      valueListTuple_.set_cells(values);
    }
    return RC::SUCCESS;
  }
}

RC ProjectPhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *ProjectPhysicalOperator::current_tuple()
{
  if (expressions_.size() == 0 && !children_.empty()) {
    if(my_expressions_.size()>0){
      return  &valueListTuple_;
    } else if (all_expressions_.size() > 0) {
      std::vector<Value> values;
      values.resize(all_expressions_.size());
      Tuple *cur_tuple = children_[0]->current_tuple();
      for (int i=0; i<all_expressions_.size(); ++i) {
        Value v;
        all_expressions_[i]->get_value(*cur_tuple, v);
        values[i] = v;
      }
      valueListTuple_.set_cells(values);
      return &valueListTuple_;
    }
    
  }

  // 无表查询
  if (expressions_.size() != 0) {
    std::vector<Value> values;
    values.resize(expressions_.size());
    for (int i=0; i<expressions_.size(); ++i) {
      FuncExpr *expr = static_cast<FuncExpr *>(expressions_[i].get());
      Value v;
      switch (expr->func())
      {
      case LENGTH_FUNC:
      {
        v.set_int(expr->lengthparam().raw_data.get_string().size());
      } break;
      case ROUND_FUNC:
      {
        float raw_data = expr->roundparam().raw_data.get_float();
        if (expr->roundparam().bits.length() == 0) {
          // 只有一个参数
          v.set_int(round(raw_data));
        } else if (expr->roundparam().bits.attr_type() != INTS) {
          break;
        } else {
          raw_data *= pow(10, expr->roundparam().bits.get_int());
          v.set_float(round(raw_data)/pow(10, expr->roundparam().bits.get_int()));
        }
      } break;
      case FORMAT_FUNC:
      {
        std::string raw_date = expr->formatparam().raw_data.get_string();
        std::string format = expr->formatparam().format.get_string();
        v.set_string(formatDate(raw_date.c_str(), format.c_str()).c_str());
      } break;
      
      default:
        break;
      }
      values[i] = v;
    }
    valueListTuple_.set_cells(values);
    return &valueListTuple_;
  }

  tuple_.set_tuple(children_[0]->current_tuple());
  return &tuple_;
}

void ProjectPhysicalOperator::add_projection(const Table *table, const FieldMeta *field_meta, const std::unordered_map<std::string, std::string> &col_alias_map, const std::unordered_map<std::string, std::string> &alias_map)
{
  // 对单表来说，展示的(alias) 字段总是字段名称，
  // 对多表查询来说，展示的alias 需要带表名字
  std::string table_name = table->name();
  std::string alias_name = field_meta->name();

  if (col_alias_map.find(alias_name) != col_alias_map.end()) {
    alias_name = col_alias_map.at(alias_name);
  }

  if (!table_name.empty()) {
    // 多表查询
    for (auto it = alias_map.begin(); it != alias_map.end(); it++) {
      if (it->second == table_name) {
        table_name = it->first;
        alias_name = table_name + "." + alias_name;
        break;
      }
    }
  }

  TupleCellSpec *spec = new TupleCellSpec(table->name(), field_meta->name(), alias_name.c_str());
  //TupleCellSpec *spec = new TupleCellSpec(table->name(), field_meta->name(), field_meta->name());
  tuple_.add_cell_spec(spec);

}
void ProjectPhysicalOperator::add_my_expressions(std::vector<Expression *> &my_expressions)
{ my_expressions_.insert(my_expressions_.end(),my_expressions.begin(),my_expressions.end());}
