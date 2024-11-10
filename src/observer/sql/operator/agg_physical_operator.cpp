//
// Created by hxy on 23-10-17.
//
#include "sql/operator/agg_physical_operator.h"
#include "common/lang/string.h"
AggPhysicalOperator::AggPhysicalOperator(const std::vector<RelAttrSqlNode>&attributes, const std::vector<Field> &fields,std::vector<Expression*>&my_expressions, std::vector<Field>& group_fields,Expression * having_expression,std::vector<RelAttrSqlNode>&having_rels,std::vector<Field>&having_fields)
    : attributes_(attributes),fields_(fields),count_(0),finish_(false),my_expressions_(my_expressions),group_fields_(group_fields),having_expression_(having_expression),is_group_by_first_(true),having_rels_(having_rels),having_fields_(having_fields),cell_num_(attributes_.size())
{}

RC AggPhysicalOperator::open(Trx *trx)
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
  return RC::SUCCESS;
}

RC AggPhysicalOperator::next()
{
  if(group_fields_.empty()){
    if (children_.empty()||finish_) {
      return RC::RECORD_EOF;
    }
    std::vector<Value> values;
    values.resize(attributes_.size());
    count_.resize(attributes_.size());
    for(int i=0;i<attributes_.size();i++){
      values[i].set_null();
      count_[i]=0;
    }
    RC rc;
    for(;;){
      if(RC::SUCCESS==(rc=children_[0]->next())){
        auto tuple=children_[0]->current_tuple();
        for(int i=0;i<values.size();i++){
          if(attributes_[i].agg==COUNT_AGG){
            if(strcmp(fields_[i].field_name(),"*" )==0){
              if(values[i].attr_type()==NULLS){
                values[i].set_int(1);
              }else{
                values[i].set_int(values[i].get_int()+1);
              }
            }else{
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              if(value.attr_type()!=NULLS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_int(1);
                }else{
                  values[i].set_int(values[i].get_int()+1);
                }
              }

            }
          }else if(attributes_[i].agg==MAX_AGG){
            Value value;
            tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
            if(value.attr_type()!=NULLS){
              if(value.attr_type()==DATES){
                if(values[i].attr_type()==NULLS){
                  values[i].set_date(value.data());
                }else{
                  if(values[i].compare(value)<0){
                    values[i].set_date(value.data());
                  }
                }

              }else if(value.attr_type()==CHARS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_string(value.data());
                }else{
                  if(values[i].compare(value)<0){
                    values[i].set_string(value.data());
                  }
                }
              }else if(value.attr_type()==INTS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_int(value.get_int());
                }else{
                  if(values[i].compare(value)<0){
                    values[i].set_int(value.get_int());
                  }
                }
              }else if(value.attr_type()==FLOATS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_float(value.get_float());
                }else{
                  if(values[i].compare(value)<0){
                    values[i].set_float(value.get_float());
                  }
                }
              }
            }

          }else if(attributes_[i].agg==MIN_AGG){
            Value value;
            tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
            if(value.attr_type()!=NULLS){
              if(value.attr_type()==DATES){
                if(values[i].attr_type()==NULLS){
                  values[i].set_date(value.data());
                }else{
                  if(values[i].compare(value)>0){
                    values[i].set_date(value.data());
                  }
                }

              }else if(value.attr_type()==CHARS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_string(value.data());
                }else{
                  if(values[i].compare(value)>0){
                    values[i].set_string(value.data());
                  }
                }
              }else if(value.attr_type()==INTS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_int(value.get_int());
                }else{
                  if(values[i].compare(value)>0){
                    values[i].set_int(value.get_int());
                  }
                }
              }else if(value.attr_type()==FLOATS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_float(value.get_float());
                }else{
                  if(values[i].compare(value)>0){
                    values[i].set_float(value.get_float());
                  }
                }
              }
            }

          }else if(attributes_[i].agg==AVG_AGG){
            Value value;
            tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
            if(value.attr_type()!=NULLS){
              if(value.attr_type()==CHARS){
                if(values[i].attr_type()==NULLS){
                  auto f=common::string2float(value.data());
                  values[i].set_float(f);
                }else{
                  auto f=common::string2float(value.data());
                  values[i].set_float(values[i].get_float()+f);
                }

              }else if(value.attr_type()==INTS){
                if(values[i].attr_type()==NULLS){
                  auto f=common::int2float(value.get_int());
                  values[i].set_float(f);
                }else{
                  auto f=common::int2float(value.get_int());
                  values[i].set_float(values[i].get_float()+f);
                }
              }else if(value.attr_type()==FLOATS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_float(value.get_float());
                }else{
                  values[i].set_float(values[i].get_float()+value.get_float());
                }
              }
              count_[i]++;
            }
          }else if(attributes_[i].agg==SUM_AGG){
            Value value;
            tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
            if(value.attr_type()!=NULLS){
              if(value.attr_type()==CHARS){
                if(values[i].attr_type()==NULLS){
                  auto f=common::string2float(value.data());
                  values[i].set_float(f);
                }else{
                  auto f=common::string2float(value.data());
                  values[i].set_float(values[i].get_float()+f);
                }
              }else if(value.attr_type()==INTS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_int(value.get_int());
                }else{
                  values[i].set_int(values[i].get_int()+value.get_int());
                }
              }else if(value.attr_type()==FLOATS){
                if(values[i].attr_type()==NULLS){
                  values[i].set_float(value.get_float());
                }else{
                  values[i].set_float(values[i].get_float()+value.get_float());
                }
              }
            }
          }
        }

      }else if(rc!=RC::RECORD_EOF){
        return  rc;
      }
      else{
        for(int i=0;i<attributes_.size();i++){
          if(attributes_[i].agg==AVG_AGG){
            if(values[i].attr_type()!=NULLS){
              values[i].set_float(values[i].get_float()/count_[i]);
            }

          }else if(attributes_[i].agg==COUNT_AGG&&values[i].attr_type()==NULLS){
            values[i].set_int(0);
          }

        }
        tuple_.set_cells(values);
        finish_= true;
        if(!my_expressions_.empty()){
          std::vector<Value>my_values(my_expressions_.size());
          std::vector<std::string >sql_string;
          for(int i=0;i<attributes_.size();i++){
            sql_string.push_back(attributes_[i].sqlString);
          }
          ValueListForExpTuple valueListForExpTuple(sql_string);
          valueListForExpTuple.set_cells(&tuple_);
          for(int i=0;i<my_expressions_.size();i++){
            my_expressions_[i]->get_value(valueListForExpTuple,my_values[i]);
          }
          expression_tuple_.set_cells(my_values);
        }
        return RC::SUCCESS;
      }
    }

  }else{
    RC rc;
    if(is_group_by_first_){
      if(!having_rels_.empty()){
        attributes_.insert(attributes_.end(),having_rels_.begin(),having_rels_.end());
        fields_.insert(fields_.end(),having_fields_.begin(),having_fields_.end());
      }
      for(;;){
        if(RC::SUCCESS==(rc=children_[0]->next())){
          auto tuple=children_[0]->current_tuple();
          AggregateKey aggregateKey;
          rc=makeAggKey(tuple,aggregateKey);
          if(rc!=RC::SUCCESS){
            return rc;
          }
          if(agg_map_.find(aggregateKey)==agg_map_.end()){
            AggregateValue aggregateValue;
            makeAggValue(aggregateValue);
            agg_map_[aggregateKey]=aggregateValue;
          }
          std::vector<Value>values;
          std::vector<int>count;
          values.assign(agg_map_[aggregateKey].aggregates_.values().begin(),agg_map_[aggregateKey].aggregates_.values().end());
          count.assign(agg_map_[aggregateKey].count_.begin(),agg_map_[aggregateKey].count_.end());
          for(int i=0;i<values.size();i++){
            if(attributes_[i].agg==COUNT_AGG){
              if(strcmp(fields_[i].field_name(),"*" )==0){
                if(values[i].attr_type()==NULLS){
                  values[i].set_int(1);
                }else{
                  values[i].set_int(values[i].get_int()+1);
                }
              }else{
                Value value;
                tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
                if(value.attr_type()!=NULLS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_int(1);
                  }else{
                    values[i].set_int(values[i].get_int()+1);
                  }
                }

              }
            }else if(attributes_[i].agg==MAX_AGG){
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              if(value.attr_type()!=NULLS){
                if(value.attr_type()==DATES){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_date(value.data());
                  }else{
                    if(values[i].compare(value)<0){
                      values[i].set_date(value.data());
                    }
                  }

                }else if(value.attr_type()==CHARS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_string(value.data());
                  }else{
                    if(values[i].compare(value)<0){
                      values[i].set_string(value.data());
                    }
                  }
                }else if(value.attr_type()==INTS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_int(value.get_int());
                  }else{
                    if(values[i].compare(value)<0){
                      values[i].set_int(value.get_int());
                    }
                  }
                }else if(value.attr_type()==FLOATS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_float(value.get_float());
                  }else{
                    if(values[i].compare(value)<0){
                      values[i].set_float(value.get_float());
                    }
                  }
                }
              }

            }else if(attributes_[i].agg==MIN_AGG){
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              if(value.attr_type()!=NULLS){
                if(value.attr_type()==DATES){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_date(value.data());
                  }else{
                    if(values[i].compare(value)>0){
                      values[i].set_date(value.data());
                    }
                  }

                }else if(value.attr_type()==CHARS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_string(value.data());
                  }else{
                    if(values[i].compare(value)>0){
                      values[i].set_string(value.data());
                    }
                  }
                }else if(value.attr_type()==INTS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_int(value.get_int());
                  }else{
                    if(values[i].compare(value)>0){
                      values[i].set_int(value.get_int());
                    }
                  }
                }else if(value.attr_type()==FLOATS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_float(value.get_float());
                  }else{
                    if(values[i].compare(value)>0){
                      values[i].set_float(value.get_float());
                    }
                  }
                }
              }

            }else if(attributes_[i].agg==AVG_AGG){
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              if(value.attr_type()!=NULLS){
                if(value.attr_type()==CHARS){
                  if(values[i].attr_type()==NULLS){
                    auto f=common::string2float(value.data());
                    values[i].set_float(f);
                  }else{
                    auto f=common::string2float(value.data());
                    values[i].set_float(values[i].get_float()+f);
                  }

                }else if(value.attr_type()==INTS){
                  if(values[i].attr_type()==NULLS){
                    auto f=common::int2float(value.get_int());
                    values[i].set_float(f);
                  }else{
                    auto f=common::int2float(value.get_int());
                    values[i].set_float(values[i].get_float()+f);
                  }
                }else if(value.attr_type()==FLOATS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_float(value.get_float());
                  }else{
                    values[i].set_float(values[i].get_float()+value.get_float());
                  }
                }
                count[i]++;
              }
            }else if(attributes_[i].agg==SUM_AGG){
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              if(value.attr_type()!=NULLS){
                if(value.attr_type()==CHARS){
                  if(values[i].attr_type()==NULLS){
                    auto f=common::string2float(value.data());
                    values[i].set_float(f);
                  }else{
                    auto f=common::string2float(value.data());
                    values[i].set_float(values[i].get_float()+f);
                  }
                }else if(value.attr_type()==INTS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_int(value.get_int());
                  }else{
                    values[i].set_int(values[i].get_int()+value.get_int());
                  }
                }else if(value.attr_type()==FLOATS){
                  if(values[i].attr_type()==NULLS){
                    values[i].set_float(value.get_float());
                  }else{
                    values[i].set_float(values[i].get_float()+value.get_float());
                  }
                }
              }
            }else if(attributes_[i].agg==NO_AGG){
              Value value;
              tuple->find_cell(TupleCellSpec(fields_[i].table_name(), fields_[i].field_name()), value);
              values[i]=value;
            }
          }
          AggregateValue aggregateValue;
          aggregateValue.aggregates_.set_cells(values);
          aggregateValue.count_.assign(count.begin(),count.end());
          agg_map_[aggregateKey]=aggregateValue;

        }else if(rc!=RC::RECORD_EOF){
          return  rc;
        }else{
          for(auto iter=agg_map_.begin();iter!=agg_map_.end();iter++){
            auto aggValue=iter->second;
            auto values=aggValue.aggregates_.values();
            auto count=aggValue.count_;
            for(int i=0;i<attributes_.size();i++){
              if(attributes_[i].agg==AVG_AGG){
                if(values[i].attr_type()!=NULLS){
                  values[i].set_float(values[i].get_float()/count[i]);
                }

              }else if(attributes_[i].agg==COUNT_AGG&&values[i].attr_type()==NULLS){
                values[i].set_int(0);
              }

            }

            aggValue.aggregates_.set_cells(values);
            aggValue.count_=count;
            iter->second=aggValue;
          }
          break;
        }
      }
      is_group_by_first_=false;
      //iter_=agg_map_.begin();
      for(iter_=agg_map_.begin();iter_!=agg_map_.end();iter_++){
        if(having_expression_!= nullptr){
          Value my_value;
          std::vector<std::string >sql_string;
          //std::vector<TupleCellSpec>spec;
          for(int i=0;i<attributes_.size();i++){
            sql_string.push_back(attributes_[i].sqlString);
            //spec.push_back(TupleCellSpec(attributes_[i].relation_name.c_str(),attributes_[i].attribute_name.c_str()));
          }


          ValueListForExpTuple valueListForExpTuple(sql_string,attributes_);
          valueListForExpTuple.set_cells(&iter_->second.aggregates_);

          having_expression_->get_value(valueListForExpTuple,my_value);

          if(my_value.get_boolean()){
            return RC::SUCCESS;
          }
        }else{
          return RC::SUCCESS;
        }

      }
      return  RC::RECORD_EOF;
      /*if(iter_== agg_map_.end()){
        return RC::RECORD_EOF;
      }*/
      //return RC::SUCCESS;
    }else{
      iter_++;
      for(;iter_!=agg_map_.end();iter_++){
        if(having_expression_!= nullptr){
          Value my_value;
          std::vector<std::string >sql_string;
          for(int i=0;i<attributes_.size();i++){
            sql_string.push_back(attributes_[i].sqlString);
          }
          ValueListForExpTuple valueListForExpTuple(sql_string,attributes_);
          valueListForExpTuple.set_cells(&iter_->second.aggregates_);

          having_expression_->get_value(valueListForExpTuple,my_value);

          if(my_value.get_boolean()){
            return RC::SUCCESS;
          }
        }else{
          return RC::SUCCESS;
        }
      }
      return  RC::RECORD_EOF;
    }



  }


}

RC AggPhysicalOperator::close()
{
  if (!children_.empty()) {
    children_[0]->close();
  }
  return RC::SUCCESS;
}
Tuple *AggPhysicalOperator::current_tuple()
{

  if(!my_expressions_.empty()){
    return &expression_tuple_;
  }
  if(!group_fields_.empty()){
    iter_->second.aggregates_.set_num(cell_num_);
    return &iter_->second.aggregates_;
  }
  return &tuple_;
}


RC AggPhysicalOperator::makeAggKey(Tuple *tuple,AggregateKey &aggregateKey){
  std::vector<Value>values(group_fields_.size());
  for(int i=0;i<group_fields_.size();i++){
    RC rc=tuple->find_cell(TupleCellSpec(group_fields_[i].table_name(),group_fields_[i].field_name()),values[i]);
    if(rc!=RC::SUCCESS){
      return rc;
    }
  }
  aggregateKey.group_bys_.set_cells(values);
  return RC::SUCCESS;
}
RC AggPhysicalOperator::makeAggValue(AggregateValue &aggregateValue){
  std::vector<Value> values;
  vector<int>count;
  values.resize(attributes_.size());
  count.resize(attributes_.size());
  for(int i=0;i<attributes_.size();i++){
    values[i].set_null();
    count[i]=0;
  }
  aggregateValue.aggregates_.set_cells(values);
  aggregateValue.count_.insert(aggregateValue.count_.end(),count.begin(),count.end());
  return RC::SUCCESS;
}