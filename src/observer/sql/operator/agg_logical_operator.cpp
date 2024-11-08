//
// Created by hxy on 23-10-17.
//
#include "sql/operator/agg_logical_operator.h"

AggLogicalOperator::AggLogicalOperator(const std::vector<RelAttrSqlNode>&attributes, const std::vector<Field> &fields,std::vector<Expression*>&my_expression, std::vector<Field>& group_fields,Expression * having_expression,std::vector<RelAttrSqlNode>having_rels,std::vector<Field>having_fields)
    : attributes_(attributes),fields_(fields),my_expressions_(my_expression),group_fields_(group_fields),having_expression_(having_expression),having_rels_(having_rels),having_fields_(having_fields)
{}

