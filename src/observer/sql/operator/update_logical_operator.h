#pragma once

#include "sql/operator/logical_operator.h"

/**
 * @brief 逻辑算子，用于执行update语句
 * @ingroup LogicalOperator
 */
class UpdateLogicalOperator : public LogicalOperator 
{
public:
  UpdateLogicalOperator(Table *table, std::vector<const FieldMeta *>field_meta, std::map<int,Value> value,std::map<int,int>select_map);
  virtual ~UpdateLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::UPDATE;
  }
  Table *table() const
  {
    return table_;
  }
  std::vector<const FieldMeta *>field_meta() { return field_meta_; }
  std::map<int,Value> value_map() const { return value_map_; }
  std::map<int,int> select_map() const { return select_map_; }
private:
  Table *table_ = nullptr;
  std::vector<const FieldMeta *>field_meta_;
  std::map<int,Value>value_map_;
  std::map<int,int>select_map_;//第n个child_opr对应那一个value.

};
