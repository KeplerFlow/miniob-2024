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
// Created by Wangyunlai on 2022/07/05.
//

#include "sql/expr/expression.h"
#include "sql/expr/tuple.h"
#include <regex>
#include "sql/operator/logical_operator.h"
#include "sql/optimizer/logical_plan_generator.h"
#include "sql/operator/physical_operator.h"
#include "sql/optimizer/physical_plan_generator.h"
#include <cmath>
#include "sql/stmt/utils.h"
using namespace std;

RC FieldExpr::get_value(const Tuple &tuple, Value &value) const
{
   if(tuple.find_cell(TupleCellSpec(table_name(), field_name()), value)==RC::NOTFOUND){
     return globe_current_rows[table_name()].find_cell(TupleCellSpec(table_name(), field_name()), value);
   }
   return RC::SUCCESS;
}
RC StringSqlExpr::get_value(const Tuple &tuple, Value &value) const
{
   return tuple.find_cell(name(),value);

}

RC ValueExpr::get_value(const Tuple &tuple, Value &value) const
{
  value = value_;
  return RC::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
RC SubSelectExpr::get_values(const Tuple &tuple, std::vector<Value> &values)const {
  SelectStmt select_stmt = sub_select_;
  unique_ptr<LogicalOperator> logical_operator;
  LogicalPlanGenerator logicalPlanGenerator;
  RC rc = logicalPlanGenerator.create(&select_stmt, logical_operator);
  if(rc!=RC::SUCCESS){
    return rc;
  }
  unique_ptr<PhysicalOperator> physical_operator;
  PhysicalPlanGenerator physicalPlanGenerator;
  rc=physicalPlanGenerator.create(*logical_operator,physical_operator);
  if(rc!=RC::SUCCESS){
    return rc;
  }
  physical_operator->open(nullptr);
  while(RC::SUCCESS==(rc=physical_operator->next())){
    Tuple *tuple1;
    tuple1=physical_operator->current_tuple();
    Value value;
    tuple1->cell_at(0,value);
    values.push_back(value);
  }
  if (rc == RC::RECORD_EOF) {
    rc = RC::SUCCESS;
  }
  return rc;

}
////////////////////////////////////////////////////////////////////////////////////
RC ValueListExpr::get_values(const Tuple &tuple, std::vector<Value> &value)const  {
  value.insert(value.end(),value_list_.begin(),value_list_.end());
  return RC::SUCCESS;
}
////////////////////////////////////////////////////////////////////////////////////
CastExpr::CastExpr(unique_ptr<Expression> child, AttrType cast_type)
    : child_(std::move(child)), cast_type_(cast_type)
{}

CastExpr::~CastExpr()
{}

RC CastExpr::cast(const Value &value, Value &cast_value) const
{
  RC rc = RC::SUCCESS;
  if (this->value_type() == value.attr_type()) {
    cast_value = value;
    return rc;
  }

  switch (cast_type_) {
    case BOOLEANS: {
      bool val = value.get_boolean();
      cast_value.set_boolean(val);
    } break;
    default: {
      rc = RC::INTERNAL;
      LOG_WARN("unsupported convert from type %d to %d", child_->value_type(), cast_type_);
    }
  }
  return rc;
}

RC CastExpr::get_value(const Tuple &tuple, Value &cell) const
{
  RC rc = child_->get_value(tuple, cell);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(cell, cell);
}

RC CastExpr::try_get_value(Value &value) const
{
  RC rc = child_->try_get_value(value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(value, value);
}

////////////////////////////////////////////////////////////////////////////////

ComparisonExpr::ComparisonExpr(CompOp comp, unique_ptr<Expression> left, unique_ptr<Expression> right)
    : comp_(comp), left_(std::move(left)), right_(std::move(right))
{}

ComparisonExpr::~ComparisonExpr()
{}
std::string wildcardToRegex(const std::string& wildcard) {
  std::string regexPattern = std::regex_replace(wildcard, std::regex("%"), ".*");
  regexPattern = std::regex_replace(regexPattern, std::regex("_"), ".");
  return regexPattern;
}
RC ComparisonExpr::compare_value(const Value &left, const Value &right, bool &result) const
{
  if((left.attr_type()==NULLS||right.attr_type()==NULLS)&&(comp_!=IS_NOT_NULL&&comp_!=IS_NULL&&comp_!=NOT_IN_OP&&comp_!=IN_OP)){
    result=false;
    return RC::SUCCESS;
  }
  RC rc = RC::SUCCESS;
  if(comp_==LIKE||comp_==NOT_LIKE){
    std::string regexPattern = wildcardToRegex(right.data());
    std::regex reg(regexPattern);
    if (std::regex_match(left.data(), reg)&&comp_!=LIKE) {
      result=false;
    }else if(std::regex_match(left.data(), reg)&&comp_==LIKE){
      result=true;
    }else if(!std::regex_match(left.data(), reg)&&comp_==LIKE){
      result=false;
    }else if(!std::regex_match(left.data(), reg)&&comp_!=LIKE){
      result=true;
    }
  }else if(comp_==IS_NOT_NULL||comp_==IS_NULL){
    if(comp_==IS_NULL&&left.attr_type()==NULLS){
      result=true;
    }else if(comp_==IS_NULL&&left.attr_type()!=NULLS){
      result= false;
    }else if(comp_==IS_NOT_NULL&&left.attr_type()==NULLS){
      result= false;
    }else if(comp_==IS_NOT_NULL&&left.attr_type()!=NULLS){
      result= true;
    }
  }else if((comp_==NOT_IN_OP||comp_==IN_OP)&&(right.attr_type()==NULLS||left.attr_type()==NULLS)) {
    if(comp_==IN_OP&&left.attr_type()==NULLS){
      if(right.attr_type()==NULLS) {
        result=true;
      }else{
        result=false;
      }

    }else if(comp_==IN_OP&&left.attr_type()!=NULLS){
      if(right.attr_type()==NULLS) {
        result= false;
      }else {
        result=true;
      }
    }else if(comp_==NOT_IN_OP&&left.attr_type()==NULLS){
      if(right.attr_type()==NULLS) {
        result=false;
      }else{
        result=true;
      }
    }else if(comp_==NOT_IN_OP&&left.attr_type()!=NULLS){
      if(right.attr_type()==NULLS) {
        result= false;
      }else {
        result=true;
      }
    }
  }else{
      int cmp_result = left.compare(right);
      result = false;
      switch (comp_) {
      case EQUAL_TO:case IN_OP: {
          result = (0 == cmp_result);
        } break;
        case LESS_EQUAL: {
          result = (cmp_result <= 0);
        } break;
        case NOT_EQUAL:case NOT_IN_OP: {
          result = (cmp_result != 0);
        } break;
        case LESS_THAN: {
          result = (cmp_result < 0);
        } break;
        case GREAT_EQUAL: {
          result = (cmp_result >= 0);
        } break;
        case GREAT_THAN: {
          result = (cmp_result > 0);
        } break;
        default: {
          LOG_WARN("unsupported comparison. %d", comp_);
          rc = RC::INTERNAL;
        } break;
      }

  }


  return rc;
}

RC ComparisonExpr::try_get_value(Value &cell) const
{
  if (left_->type() == ExprType::VALUE && right_->type() == ExprType::VALUE) {
    ValueExpr *left_value_expr = static_cast<ValueExpr *>(left_.get());
    ValueExpr *right_value_expr = static_cast<ValueExpr *>(right_.get());
    const Value &left_cell = left_value_expr->get_value();
    const Value &right_cell = right_value_expr->get_value();

    bool value = false;
    RC rc = compare_value(left_cell, right_cell, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to compare tuple cells. rc=%s", strrc(rc));
    } else {
      cell.set_boolean(value);
    }
    return rc;
  }

  return RC::INVALID_ARGUMENT;
}

RC ComparisonExpr::get_value(const Tuple &tuple, Value &value) const
{
  if(left_->type()!=ExprType::SUBSELECT&&right_->type()!=ExprType::SUBSELECT&&left_->type()!=ExprType::VALUELIST&&right_->type()!=ExprType::VALUELIST){
    Value left_value;
    Value right_value;

    RC rc = left_->get_value(tuple, left_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
      return rc;
    }
    rc = right_->get_value(tuple, right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }

    bool bool_value = false;
    rc = compare_value(left_value, right_value, bool_value);
    if (rc == RC::SUCCESS) {
      value.set_boolean(bool_value);
    }
    return rc;
  }else{
    if(comp_==NOT_EXISTS_OP||comp_==EXISTS_OP){
      std::vector<Value>right_values;
      RC rc = right_->get_values(tuple, right_values);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
        return rc;
      }
      if(right_values.empty()){
        if(comp_==NOT_EXISTS_OP){
          value.set_boolean(true);
        }else{
          value.set_boolean(false);
        }
      }else{
        if(comp_==EXISTS_OP){
          value.set_boolean(true);
        }else{
          value.set_boolean(false);
        }
      }
    }else if(comp_==NOT_IN_OP||comp_==IN_OP){
      Value left_value,right_value;
      std::vector<Value>left_values,right_values;
      RC rc;
      if(left_->type()==ExprType::SUBSELECT||left_->type()==ExprType::VALUELIST){
         rc = left_->get_values(tuple, left_values);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return rc;
        }
        if(left_values.size()>1){
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return RC::INTERNAL;
        }
      }else {
         rc = left_->get_value(tuple, left_value);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return rc;
        }
      }
      if(right_->type()==ExprType::SUBSELECT||right_->type()==ExprType::VALUELIST){
         rc = right_->get_values(tuple, right_values);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
          return rc;
        }
      }else{
        /*RC rc = right_->get_value(tuple, right_value);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
          return rc;
        }*/
        LOG_WARN("failed to get value of right expression. ");
        return RC::INTERNAL;
      }
      if(left_values.size()>1){
        return  RC::INTERNAL;
      }else if(left_values.size()==1){
        left_value=left_values[0];
      }
      for(int i=0;i<right_values.size();i++){
        bool bool_value = false;
        rc = compare_value(left_value, right_values[i], bool_value);

        if(comp_==NOT_IN_OP){
          if(!bool_value){
            if (rc == RC::SUCCESS) {
              value.set_boolean(false);
              return rc;
            }
          }
        }else{
          if(bool_value){
            if (rc == RC::SUCCESS) {
              value.set_boolean(true);
              return rc;
            }
          }
        }
      }
      if(comp_==NOT_IN_OP){
        value.set_boolean(true);
        return rc;
      }else{
        value.set_boolean(false);
        return rc;
      }


    }else{
      Value left_value,right_value;
      std::vector<Value>left_values,right_values;
      RC rc;
      if(left_->type()==ExprType::SUBSELECT){
        rc = left_->get_values(tuple, left_values);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return rc;
        }
        if(left_values.size()>1){
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return RC::INTERNAL;
        }
      }else{
        rc = left_->get_value(tuple, left_value);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
          return rc;
        }
      }
      if(right_->type()==ExprType::SUBSELECT){
        rc = right_->get_values(tuple, right_values);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
          return rc;
        }
        if(right_values.size()>1){
          LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
          return RC::INTERNAL;
        }
      }else{
        RC rc = right_->get_value(tuple, right_value);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
          return rc;
        }

      }
      if(left_values.size()>1){
        return  RC::INTERNAL;
      }else if(left_values.size()==1){
        left_value=left_values[0];
      }
      if(right_values.size()>1){
        return  RC::INTERNAL;
      }else if(right_values.size()==1){
        right_value=right_values[0];
      }
      bool bool_value = false;
      rc = compare_value(left_value, right_value, bool_value);
      if (rc == RC::SUCCESS) {
        value.set_boolean(bool_value);
      }
      return rc;


    }
  }
  return RC::SUCCESS;

}

////////////////////////////////////////////////////////////////////////////////
ConjunctionExpr::ConjunctionExpr(Type type, vector<unique_ptr<Expression>> &children)
    : conjunction_type_(type), children_(std::move(children))
{}

RC ConjunctionExpr::get_value(const Tuple &tuple, Value &value) const
{
  RC rc = RC::SUCCESS;
  if (children_.empty()) {
    value.set_boolean(true);
    return rc;
  }

  Value tmp_value;
  for (const unique_ptr<Expression> &expr : children_) {
    rc = expr->get_value(tuple, tmp_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value by child expression. rc=%s", strrc(rc));
      return rc;
    }
    bool bool_value = tmp_value.get_boolean();
    if ((conjunction_type_ == Type::AND && !bool_value) || (conjunction_type_ == Type::OR && bool_value)) {
      value.set_boolean(bool_value);
      return rc;
    }
  }

  bool default_value = (conjunction_type_ == Type::AND);
  value.set_boolean(default_value);
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, Expression *left, Expression *right)
    : arithmetic_type_(type), left_(left), right_(right)
{}
ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, unique_ptr<Expression> left, unique_ptr<Expression> right)
    : arithmetic_type_(type), left_(std::move(left)), right_(std::move(right))
{}

AttrType ArithmeticExpr::value_type() const
{
  if (!right_) {
    return left_->value_type();
  }

  if (left_->value_type() == AttrType::INTS &&
      right_->value_type() == AttrType::INTS &&
      arithmetic_type_ != Type::DIV) {
    return AttrType::INTS;
  }
  
  return AttrType::FLOATS;
}

RC ArithmeticExpr::calc_value(const Value &left_value, const Value &right_value, Value &value) const
{
  RC rc = RC::SUCCESS;
  if(left_value.attr_type()==NULLS||right_value.attr_type()==NULLS){
     value.set_null();
     return RC::SUCCESS;
  }
  const AttrType target_type = value_type();

  switch (arithmetic_type_) {
    case Type::ADD: {
      if (target_type == AttrType::INTS) {
        value.set_int(left_value.get_int() + right_value.get_int());
      } else {
        value.set_float(left_value.get_float() + right_value.get_float());
      }
    } break;

    case Type::SUB: {
      if (target_type == AttrType::INTS) {
        value.set_int(left_value.get_int() - right_value.get_int());
      } else {
        value.set_float(left_value.get_float() - right_value.get_float());
      }
    } break;

    case Type::MUL: {
      if (target_type == AttrType::INTS) {
        value.set_int(left_value.get_int() * right_value.get_int());
      } else {
        value.set_float(left_value.get_float() * right_value.get_float());
      }
    } break;

    case Type::DIV: {
      if (target_type == AttrType::INTS) {
        if (right_value.get_int() == 0) {
          // NOTE: 设置为整数最大值是不正确的。通常的做法是设置为NULL，但是当前的miniob没有NULL概念，所以这里设置为整数最大值。
          value.set_null();
        } else {
          value.set_int(left_value.get_int() / right_value.get_int());
        }
      } else {
        if (right_value.get_float() > -EPSILON && right_value.get_float() < EPSILON) {
          // NOTE: 设置为浮点数最大值是不正确的。通常的做法是设置为NULL，但是当前的miniob没有NULL概念，所以这里设置为浮点数最大值。
          value.set_null();
        } else {
          value.set_float(left_value.get_float() / right_value.get_float());
        }
      }
    } break;

    case Type::NEGATIVE: {
      if (target_type == AttrType::INTS) {
        value.set_int(-left_value.get_int());
      } else {
        value.set_float(-left_value.get_float());
      }
    } break;

    default: {
      rc = RC::INTERNAL;
      LOG_WARN("unsupported arithmetic type. %d", arithmetic_type_);
    } break;
  }
  return rc;
}

RC ArithmeticExpr::get_value(const Tuple &tuple, Value &value) const
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  if(arithmetic_type_!=Type::NEGATIVE){
    rc = left_->get_value(tuple, left_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
      return rc;
    }
    rc = right_->get_value(tuple, right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }
  }else{
    rc = left_->get_value(tuple, left_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
      return rc;
    }
  }

  return calc_value(left_value, right_value, value);
}

RC ArithmeticExpr::try_get_value(Value &value) const
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  rc = left_->try_get_value(left_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }

  if (right_) {
    rc = right_->try_get_value(right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }
  }

  return calc_value(left_value, right_value, value);
}

////////////////////////////////////////////////////////////////////////////////
RC FuncExpr::get_value(const Tuple &tuple, Value &value) const
{
  Value tuple_value;
  tuple.find_cell(TupleCellSpec(table_name(), field_name()), tuple_value);
  switch (func_)
  {
  case LENGTH_FUNC:
  {
    int length = tuple_value.get_string().size();
    value.set_int(length);
  } break;
  case ROUND_FUNC:
  {
    float tuple_float = tuple_value.get_float();
    if (roundparam_.bits.length() == 0) {
      // 只有一个参数
      value.set_int(round(tuple_float));
    } else if (roundparam_.bits.attr_type() != INTS) {
      LOG_ERROR("round() Func bits error.");
      return RC::VARIABLE_NOT_VALID;
    } else {
      tuple_float *= pow(10, roundparam_.bits.get_int());
      value.set_float(round(tuple_float)/pow(10, roundparam_.bits.get_int()));
    }
  } break;
  case FORMAT_FUNC:
  {
    string tuple_date = tuple_value.get_string();
    string format = formatparam_.format.get_string();
    value.set_string(formatDate(tuple_date.c_str(), format.c_str()).c_str());
  } break;
  
  default:
    return RC::UNIMPLENMENT;
  }

  return RC::SUCCESS;
}