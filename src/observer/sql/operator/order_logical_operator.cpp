//
// Created by hxy on 23-10-20.
//

#include "order_logical_operator.h"
OrderLogicalOperator::OrderLogicalOperator(const std::vector<Field> &order_fields,const std::vector<OrderBySequence>&order_sequences,const std::vector<Field> &query_fields):order_fields_(order_fields),order_sequences_(order_sequences),query_fields_(query_fields){}