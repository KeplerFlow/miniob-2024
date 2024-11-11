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
// Created by Longda on 2021/4/13.
//

#include <string>
#include <sstream>

#include "sql/executor/execute_stage.h"

#include "common/log/log.h"
#include "session/session.h"
#include "event/storage_event.h"
#include "event/sql_event.h"
#include "event/session_event.h"
#include "sql/stmt/stmt.h"
#include "sql/stmt/select_stmt.h"
#include "storage/default/default_handler.h"
#include "sql/executor/command_executor.h"
#include "sql/operator/calc_physical_operator.h"

using namespace std;
using namespace common;

RC ExecuteStage::handle_request(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;
  const unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  if (physical_operator != nullptr) {
    return handle_request_with_physical_operator(sql_event);
  }

  SessionEvent *session_event = sql_event->session_event();

  Stmt *stmt = sql_event->stmt();
  if (stmt != nullptr) {
    CommandExecutor command_executor;
    rc = command_executor.execute(sql_event);
    session_event->sql_result()->set_return_code(rc);
  } else {
    return RC::INTERNAL;
  }
  return rc;
}

RC ExecuteStage::handle_request_with_physical_operator(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  Stmt *stmt = sql_event->stmt();
  ASSERT(stmt != nullptr, "SQL Statement shouldn't be empty!");

  unique_ptr<PhysicalOperator> &physical_operator = sql_event->physical_operator();
  ASSERT(physical_operator != nullptr, "physical operator should not be null");
  TupleSchema schema;
  switch (stmt->type()) {
    case StmtType::SELECT: {
      SelectStmt *select_stmt = static_cast<SelectStmt *>(stmt);
      std::unordered_map<std::string, std::string> alias_map(select_stmt->alias_map());
      if(select_stmt->is_agg()){
        if(select_stmt->is_expression_select_attr()){
          for(int i=0;i<select_stmt->expression().size();i++){
            schema.append_cell(select_stmt->expression()[i]->name().c_str());
          }
        }else{
          for(int i=0;i<select_stmt->attributes().size();i++){
            if (select_stmt->col_alias_map().contains(select_stmt->attributes()[i].attribute_name)) {
              string name=select_stmt->col_alias_map()[select_stmt->attributes()[i].attribute_name];
              schema.append_cell(name.c_str());
            }
            else if(select_stmt->attributes()[i].agg==MAX_AGG){
              string name="max("+select_stmt->attributes()[i].attribute_name+")";
              schema.append_cell(name.c_str());
            }else if(select_stmt->attributes()[i].agg==MIN_AGG){
              string name="min("+select_stmt->attributes()[i].attribute_name+")";
              schema.append_cell(name.c_str());
            }else if(select_stmt->attributes()[i].agg==AVG_AGG){
              string name="avg("+select_stmt->attributes()[i].attribute_name+")";
              schema.append_cell(name.c_str());
            }else if(select_stmt->attributes()[i].agg==COUNT_AGG){
              string name="count("+select_stmt->attributes()[i].attribute_name+")";
              schema.append_cell(name.c_str());
            }else if(select_stmt->attributes()[i].agg==SUM_AGG){
              string name="sum("+select_stmt->attributes()[i].attribute_name+")";
              schema.append_cell(name.c_str());
            }else if(select_stmt->attributes()[i].agg==NO_AGG){
              schema.append_cell(select_stmt->attributes()[i].attribute_name.c_str());
            }

          }
        }


      }else{
        if(select_stmt->is_expression_select_attr()){
          for(int i=0;i<select_stmt->expression().size();i++){
            schema.append_cell(select_stmt->expression()[i]->name().c_str());
          }
        }else{
          bool with_table_name = select_stmt->tables().size() > 1;

          bool has_asterisk = false;
          for (RelAttrSqlNode rel_attr:select_stmt->attributes()) {
            if (rel_attr.attribute_name == "*") {
              has_asterisk = true;
            }
          }

          if (has_asterisk) {
            for (const Field &field : select_stmt->query_fields()) {
              std::string field_name = field.field_name();
              if (select_stmt->col_alias_map().find(field_name) != select_stmt->col_alias_map().end()) {
                field_name = select_stmt->col_alias_map()[field_name];
              }

              if (with_table_name) {
                std::string table_name = field.table_name();
                for (auto it = alias_map.begin(); it!=alias_map.end(); it++) {
                  if (it->second == table_name) {
                    table_name = it->first;
                    break;
                  }
                }

                schema.append_cell(table_name.c_str(), field_name.c_str());
              } else {
                schema.append_cell(field_name.c_str());
              }
            }            
          } else {
            for (RelAttrSqlNode rel_attr:select_stmt->attributes()) {
              if (!rel_attr.alias_name.empty()) {
                schema.append_cell(rel_attr.alias_name.c_str());
              } else if (rel_attr.func != NO_FUNC) {
                switch (rel_attr.func)
                {
                case LENGTH_FUNC:
                  {
                    string name;
                    if (!rel_attr.attribute_name.empty()) {
                      name = rel_attr.relation_name.empty() ? rel_attr.attribute_name: 
                                rel_attr.relation_name+"."+rel_attr.attribute_name;
                    } else {
                      name = "\"" + rel_attr.lengthparam.raw_data.get_string() + "\"";
                    }
                    name = "length(" + name + ")";
                    schema.append_cell(name.c_str());
                  } break;
                case ROUND_FUNC:
                  {
                    string name;
                    if (!rel_attr.attribute_name.empty()) {
                      name = rel_attr.relation_name.empty() ? rel_attr.attribute_name: 
                                rel_attr.relation_name+"."+rel_attr.attribute_name;
                    } else {
                      name = rel_attr.roundparam.raw_data.to_string();
                    }
                    if (rel_attr.roundparam.bits.length() != 0) {
                      name = name + "," + rel_attr.roundparam.bits.to_string();
                    }
                    name = "round(" + name + ")";
                    schema.append_cell(name.c_str());
                  } break;
                case FORMAT_FUNC:
                  {
                    string name;
                    if (!rel_attr.attribute_name.empty()) {
                      name = rel_attr.relation_name.empty() ? rel_attr.attribute_name: 
                                rel_attr.relation_name+"."+rel_attr.attribute_name;
                    } else {
                      name = "\"" + rel_attr.formatparam.raw_data.get_string() + "\"";
                    }
                    name = name + "," + "\"" + rel_attr.formatparam.format.get_string() + "\"";
                    name = "date_format(" + name + ")";
                    schema.append_cell(name.c_str());
                  } break;
                default:
                  break;
                }
              } else {
                std::string field_name = rel_attr.attribute_name;
                if (with_table_name) {
                  std::string table_name = rel_attr.relation_name;
                  schema.append_cell(table_name.c_str(), field_name.c_str());
                } else {
                  schema.append_cell(field_name.c_str());
                }
              }
            }         
          }

        }

      }
    } break;

    case StmtType::CALC: {
      CalcPhysicalOperator *calc_operator = static_cast<CalcPhysicalOperator *>(physical_operator.get());
      for (const unique_ptr<Expression> & expr : calc_operator->expressions()) {
        schema.append_cell(expr->name().c_str());
      }
    } break;

    case StmtType::EXPLAIN: {
      schema.append_cell("Query Plan");
    } break;
    default: {
    } break;
  }
  SqlResult *sql_result = sql_event->session_event()->sql_result();
  sql_result->set_tuple_schema(schema);
  sql_result->set_operator(std::move(physical_operator));
  return rc;
}
