//
// Created by hxy on 23-10-20.
//
#include <vector>
#include <memory>

#include "sql/operator/logical_operator.h"
#include "sql/expr/expression.h"
#include "storage/field/field.h"
class OrderLogicalOperator : public LogicalOperator
{
public:
  OrderLogicalOperator(const std::vector<Field> &order_fields,const std::vector<OrderBySequence>&order_sequences,const std::vector<Field> &query_fields);
  virtual ~OrderLogicalOperator() = default;

  LogicalOperatorType type() const override
  {
    return LogicalOperatorType::ORDER_BY;
  }

  std::vector<std::unique_ptr<Expression>> &expressions()
  {
    return expressions_;
  }
  const std::vector<std::unique_ptr<Expression>> &expressions() const
  {
    return expressions_;
  }
  const std::vector<Field> &order_fields() const
  {
    return order_fields_;
  }
  const std::vector<Field> &query_fields() const
  {
    return query_fields_;
  }
  const std::vector<OrderBySequence> &order_sequences() const
  {
    return order_sequences_;
  }

private:
  //! 投影映射的字段名称
  //! 并不是所有的select都会查看表字段，也可能是常量数字、字符串，
  //! 或者是执行某个函数。所以这里应该是表达式Expression。
  //! 不过现在简单处理，就使用字段来描述
  std::vector<Field> order_fields_;
  std::vector<OrderBySequence>order_sequences_;
  std::vector<Field> query_fields_;
};



