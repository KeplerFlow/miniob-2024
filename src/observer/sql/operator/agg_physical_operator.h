#pragma once

#include "sql/operator/physical_operator.h"


struct AggregateKey {
  ValueListTuple group_bys_;

  bool operator<(const AggregateKey &other) const {
    int ret = 0;
    for (uint32_t i = 0; i < group_bys_.cell_num(); i++) {
      Value left,right;
      group_bys_.cell_at(i,left);
      other.group_bys_.cell_at(i,right);
      if(left.attr_type()==NULLS&&right.attr_type()==NULLS){
        continue;
      }
      if(left.attr_type()==NULLS){
        return true;
      }
      if(right.attr_type()==NULLS){
        return false;
      }

      if ((ret = left.compare(right))!= 0) {
        return ret < 0;
      }
    }
    return ret < 0;
  }
};

struct AggregateValue {
  // max or min or sum or count
  ValueListTuple aggregates_;
  // 没什么好办法，由于avg的存在，一个值没法表示所有的聚合函数类型，所以多存一个count,用sum/count表示avg
  std::vector<int> count_;

};

/**
 * @brief 选择/投影物理算子
 * @ingroup PhysicalOperator
 */
class AggPhysicalOperator : public PhysicalOperator
{
public:
  AggPhysicalOperator(const std::vector<RelAttrSqlNode>&attributes, const std::vector<Field> &fields,std::vector<Expression*>&my_expressions, std::vector<Field>& group_fields,Expression * having_expression,std::vector<RelAttrSqlNode>&having_rels,std::vector<Field>&having_fields);

  virtual ~AggPhysicalOperator() = default;

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::AGG;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  int cell_num() const
  {
    if(!my_expressions_.empty()){
      return my_expressions_.size();
    }
    return cell_num_;
  }
  RC makeAggKey(Tuple *tuple,AggregateKey &aggregateKey);
  RC makeAggValue(AggregateValue &aggregateValue);
  Tuple *current_tuple() override;

private:
  ValueListTuple tuple_;
  std::vector<RelAttrSqlNode> attributes_;
  std::vector<Field>fields_;
  std::vector<int> count_;
  bool finish_;

  std::vector<Expression*>my_expressions_;
  ValueListTuple expression_tuple_;

  bool is_group_by_;
  std::map<AggregateKey,AggregateValue>agg_map_;

  std::vector<Field>group_fields_;
  Expression* having_expression_;
  bool is_group_by_first_=false;
  std::map<AggregateKey,AggregateValue>::iterator  iter_;
  std::vector<RelAttrSqlNode>having_rels_;
  std::vector<Field>having_fields_;
  int cell_num_;
};


