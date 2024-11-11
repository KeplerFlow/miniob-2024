//
// Created by hxy on 23-10-20.
//
#pragma once

#include "sql/operator/physical_operator.h"

class OrderPhysicalOperator : public PhysicalOperator
{
public:
  OrderPhysicalOperator(std::vector<Field>order_fields,std::vector<OrderBySequence>order_sequences,std::vector<Field>query_fields): order_fields_(order_fields),order_sequences_(order_sequences),query_fields_(query_fields),index_(-1){
  }


  virtual ~OrderPhysicalOperator() = default;

  void add_expressions(std::vector<std::unique_ptr<Expression>> &&expressions)
  {

  }
  void add_projection(const Table *table, const FieldMeta *field);

  PhysicalOperatorType type() const override
  {
    return PhysicalOperatorType::ORDERBY;
  }

  RC open(Trx *trx) override;
  RC next() override;
  RC close() override;

  int cell_num() const
  {
    return query_fields_.size();
  }

  Tuple *current_tuple() override;

private:
  std::vector<ValueListTuple>value_list_;
  std::vector<Field>order_fields_;
  std::vector<OrderBySequence>order_sequences_;
  std::vector<Field>query_fields_;
  int index_;
};


class TupleSortUtil {
public:
  static bool cmp(const ValueListTuple &lhs, const ValueListTuple &rhs) {
    int ret = 0;
    for (int i=0;i<order_sequences_->size();i++ ) {
      // const map无法用下标取值
      int index = field_index_->at((*order_field_)[i].table_name()).at((*order_field_)[i].field_name());
      Value lvalue,rvalue;
      lhs.cell_at(index,lvalue);
      rhs.cell_at(index,rvalue);
      if(lvalue.attr_type()==NULLS&&rvalue.attr_type()==NULLS){
        continue;
      }
      if(lvalue.attr_type()==NULLS){
        if ((*order_sequences_)[i] == ASC_ORDER_BY) {
          return true;
        }else{
          return false;
        }
      }
      if(rvalue.attr_type()==NULLS){
        if ((*order_sequences_)[i] == ASC_ORDER_BY) {
          return false;
        }else{
          return true;
        }
      }
      ret = lvalue.compare(rvalue);
      if (ret != 0) {
        if ((*order_sequences_)[i] == ASC_ORDER_BY) {
          // ASC
          return ret < 0;
        } else {
          // DESC
          return ret > 0;
        }
      }
    }
    return ret < 0;
  }

  static void set(const std::map<std::string, std::map<std::string, int>> &field_index, const std::vector<Field> &order_field,const std::vector<OrderBySequence> &order_sequences) {
    field_index_ = &field_index;
    order_field_ = &order_field;
    order_sequences_=&order_sequences;
  }
private:
  // {table_name: {field_name, index}}
  static const std::map<std::string, std::map<std::string, int>> *field_index_;
  static const std::vector<Field> *order_field_;
  static const std::vector<OrderBySequence>*order_sequences_;
};