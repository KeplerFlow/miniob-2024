//
// Created by hxy on 23-10-20.
//
#include "sql/operator/order_physical_operator.h"

const std::map<std::string, std::map<std::string, int>> *TupleSortUtil::field_index_ = nullptr;
const std::vector<Field> *TupleSortUtil::order_field_= nullptr;
const std::vector<OrderBySequence>*TupleSortUtil::order_sequences_= nullptr;

RC OrderPhysicalOperator::open(Trx *trx)
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
  while(child->next()==RC::SUCCESS){
    ValueListTuple value_list;
    Tuple *tuple=child->current_tuple();
    std::vector<Value> values(tuple->cell_num());
    for(int i=0;i<tuple->cell_num();i++){
      tuple->cell_at(i,values[i]);
    }
    value_list.set_cells(values);
    value_list_.push_back(value_list);
  }
  std::map<std::string, std::map<std::string, int>> field_index;
  for(int i=0;i<order_fields_.size();i++){
    for(int j=0;j<query_fields_.size();j++){
      if(strcmp(order_fields_[i].table_name(),query_fields_[j].table_name())==0&&strcmp(order_fields_[i].field_name(),query_fields_[j].field_name())==0){
        field_index[order_fields_[i].table_name()][order_fields_[i].field_name()]=j;
      }
    }
  }
  TupleSortUtil::set(field_index,order_fields_,order_sequences_);
  std::sort(value_list_.begin(),value_list_.end(),TupleSortUtil::cmp);

  return RC::SUCCESS;
}

RC OrderPhysicalOperator::next()
{
  index_++;
  if(index_>=value_list_.size()){
     return RC::RECORD_EOF;
  }
  return RC::SUCCESS;
}

RC OrderPhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *OrderPhysicalOperator::current_tuple(){
  return &value_list_[index_];

}
