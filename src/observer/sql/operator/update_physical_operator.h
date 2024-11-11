#pragma once

#include "sql/operator/physical_operator.h"

class Trx;
class DeleteStmt;

/**
 * @brief 物理算子，更新
 * @ingroup PhysicalOperator
 */
class UpdatePhysicalOperator : public PhysicalOperator
{
public:
  UpdatePhysicalOperator(Table *table, std::vector<const FieldMeta *>field_meta, std::map<int,Value> value,std::map<int,int>select_map)
      :table_(table), field_meta_(field_meta), value_map_(value),select_map_(select_map),is_muil_row_(false),is_fail_(false)
  {}

  virtual ~UpdatePhysicalOperator() = default;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::UPDATE;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override
  {
    return nullptr;
  }

private:
  Table *table_ = nullptr;


  Trx *trx_ = nullptr;
  std::vector<const FieldMeta *>field_meta_;
  std::map<int,Value>value_map_;
  std::map<int,int>select_map_;
  std::vector<Value>values_;
  bool is_muil_row_;
  bool is_fail_;
};