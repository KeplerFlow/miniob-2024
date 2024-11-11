#pragma once

#include <vector>
#include <memory>

#include "sql/operator/logical_operator.h"
#include "sql/expr/expression.h"
#include "storage/field/field.h"

/**
 * @brief project 表示投影运算
 * @ingroup LogicalOperator
 * @details 从表中获取数据后，可能需要过滤，投影，连接等等。
 */
class AggLogicalOperator : public LogicalOperator
{
public:
  AggLogicalOperator(const std::vector<RelAttrSqlNode>&attributes, const std::vector<Field> &fields,std::vector<Expression*>&my_expression, std::vector<Field>& group_fields,Expression * having_expression,std::vector<RelAttrSqlNode>having_rels,std::vector<Field>having_fields);
  virtual ~AggLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::AGG;
  }

  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }
  const std::vector<std::unique_ptr<Expression>> &expressions() const
  {
    return expressions_;
  }
  const std::vector<RelAttrSqlNode> &attributes() const
  {
    return attributes_;
  }
  const std::vector<Field> &fields() const
  {
    return fields_;
  }
  std::vector<Expression*>&my_expressions(){
    return my_expressions_;
  }
  std::vector<Field>&group_fields(){
    return group_fields_;
  }
   Expression * having_expression(){
    return having_expression_;
  }
  std::vector<RelAttrSqlNode>&having_rels(){
    return  having_rels_;
  }
  std::vector<Field>&having_fields(){
    return having_fields_;
  }

private:
  //! 投影映射的字段名称
  //! 并不是所有的select都会查看表字段，也可能是常量数字、字符串，
  //! 或者是执行某个函数。所以这里应该是表达式Expression。
  //! 不过现在简单处理，就使用字段来描述
  std::vector<RelAttrSqlNode> attributes_;
  std::vector<Field>fields_;

  std::vector<Expression*>my_expressions_;

  std::vector<Field>group_fields_;
  Expression* having_expression_;
  std::vector<RelAttrSqlNode>having_rels_;
  std::vector<Field>having_fields_;
};
