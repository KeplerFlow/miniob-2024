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
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include <cmath>
#include "utils.h"

SelectStmt::~SelectStmt()
{
  if (nullptr != filter_stmt_) {
    //delete filter_stmt_;
    //filter_stmt_ = nullptr;
  }
}

static void wildcard_fields(Table *table, std::vector<Field> &field_metas)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num();
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    field_metas.push_back(Field(table, table_meta.field(i)));
  }
}

static void wildcard_expressions(Table *table, std::vector<Expression*> &all_expressions)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num();
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    all_expressions.push_back(static_cast<Expression*>(new FieldExpr(table, table_meta.field(i))));
  }
}

RC SelectStmt::create(Db *db, const SelectSqlNode &select_sql, Stmt *&stmt,bool is_sub_select,std::unordered_map<std::string, Table *>top_tables)
{
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }
  if(!select_sql.is_alias_right){
    return RC::INVALID_ARGUMENT;
  }

  // collect tables in `from` statement
  std::vector<Table *> tables;
  std::unordered_map<std::string, Table *> table_map;
  for (size_t i = 0; i < select_sql.relations.size(); i++) {
    const char *table_name = select_sql.relations[i].c_str();
    if (nullptr == table_name) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    tables.push_back(table);
    table_map.insert(std::pair<std::string, Table *>(table_name, table));
  }

  bool is_agg=false;
  int agg_num=0,group_by_num=0;
  {
    for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--){
      const RelAttrSqlNode &relation_attr = select_sql.attributes[i];
      if(relation_attr.agg!=NO_AGG){
        is_agg= true;
        agg_num++;
        if(relation_attr.attribute_name=="*"&&relation_attr.agg!=COUNT_AGG){
          LOG_WARN("invalid argument. agg_num  wrong. ");
          return RC::INVALID_ARGUMENT;
        }
      }
      if(!relation_attr.is_right){
        return  RC::INVALID_ARGUMENT;
      }
    }
  }


  // 将别名也添加到table_map中去，为了Filter中查找&
  std::string default_table_alas;
  std::unordered_map<std::string, std::string> alias_map(select_sql.alias_map);
  for (auto it = alias_map.begin(); it != alias_map.end(); it++) {
    if (table_map.find(it->second) != table_map.end()) {
      //table_map.insert(std::pair<std::string, Table *>(it->first, table_map[it->second]));
      if(it->second==select_sql.relations[0]){
        default_table_alas=it->first;
      }
    }
  }

  // 为列的别名生成别名映射关系
  std::unordered_map<std::string, std::string> col_alias_map;
  for (auto &attribute : select_sql.attributes) {
    if (!attribute.alias_name.empty()) {
      col_alias_map.insert(std::pair<std::string, std::string>(attribute.attribute_name, attribute.alias_name));
    }
  }

  if(select_sql.is_expression_select_attr){
    if(is_agg){
      if(agg_num!=static_cast<int>(select_sql.attributes.size())){
        LOG_WARN("invalid argument. agg_num  wrong. ");
        return RC::INVALID_ARGUMENT;
      }

      Table *default_table = nullptr;
      if (tables.size() == 1) {
        default_table = tables[0];
      }

      // collect query fields in `select` statement
      std::vector<Field> query_fields;
      for (int index = 0; index < static_cast<int>(select_sql.attributes.size()); ++index) {
        const RelAttrSqlNode &attr_node = select_sql.attributes[index];

        const char *rel_name = attr_node.relation_name.c_str();
        const char *attr_name = attr_node.attribute_name.c_str();

        if (common::is_blank(rel_name) && strcmp(attr_name, "*") == 0) {
          // Case: SELECT * without specifying a table
          FieldMeta *meta_field = new FieldMeta;
          meta_field->init("*");
          query_fields.push_back(Field(default_table, meta_field));

          for (auto sql_expr : select_sql.stringsqlExprs) {
            if (sql_expr->name().compare(attr_node.sqlString) == 0) {
              sql_expr->setType(INTS);
            }
          }
        } else if (!common::is_blank(rel_name)) {
          // Relation name is specified
          if (strcmp(rel_name, "*") == 0) {
            // Invalid case: relation name is '*' with non-'*' attribute
            if (strcmp(attr_name, "*") != 0) {
              LOG_WARN("Invalid field name when table is '*'. Attribute: %s", attr_name);
              return RC::SCHEMA_FIELD_MISSING;
            }
            // Case: SELECT * FROM *
            FieldMeta *meta_field = new FieldMeta;
            meta_field->init("*");
            query_fields.push_back(Field(default_table, meta_field));

            for (auto sql_expr : select_sql.stringsqlExprs) {
              if (sql_expr->name().compare(attr_node.sqlString) == 0) {
                sql_expr->setType(INTS);
              }
            }
          } else {
            // Valid table name specified
            auto tbl_iter = table_map.find(rel_name);
            if (tbl_iter == table_map.end()) {
              LOG_WARN("No such table in FROM clause: %s", rel_name);
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *tbl = tbl_iter->second;
            if (strcmp(attr_name, "*") == 0) {
              // Case: SELECT * FROM table
              FieldMeta *meta_field = new FieldMeta;
              meta_field->init("*");
              query_fields.push_back(Field(tbl, meta_field));

              for (auto sql_expr : select_sql.stringsqlExprs) {
                if (sql_expr->name().compare(attr_node.sqlString) == 0) {
                  sql_expr->setType(INTS);
                }
              }
            } else {
              // Specific attribute from a specific table
              const FieldMeta *meta_field = tbl->table_meta().field(attr_name);
              if (meta_field == nullptr) {
                LOG_WARN("No such field: %s.%s.%s", db->name(), tbl->name(), attr_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              query_fields.push_back(Field(tbl, meta_field));

              for (auto sql_expr : select_sql.stringsqlExprs) {
                if (sql_expr->name().compare(attr_node.sqlString) == 0) {
                  if (attr_node.agg == MAX_AGG || attr_node.agg == MIN_AGG || attr_node.agg == SUM_AGG) {
                    sql_expr->setType(meta_field->type());
                  } else if (attr_node.agg == COUNT_AGG) {
                    sql_expr->setType(INTS);
                  } else {
                    sql_expr->setType(FLOATS);
                  }
                }
              }
            }
          }
        } else {
          // Relation name is blank, attribute name is specified
          if (tables.size() != 1) {
            LOG_WARN("Ambiguous attribute without table name: %s", attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          Table *tbl = tables[0];
          const FieldMeta *meta_field = tbl->table_meta().field(attr_name);
          if (meta_field == nullptr) {
            LOG_WARN("No such field: %s.%s.%s", db->name(), tbl->name(), attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          query_fields.push_back(Field(tbl, meta_field));

          for (auto sql_expr : select_sql.stringsqlExprs) {
            if (sql_expr->name().compare(attr_node.sqlString) == 0) {
              if (attr_node.agg == MAX_AGG || attr_node.agg == MIN_AGG || attr_node.agg == SUM_AGG) {
                sql_expr->setType(meta_field->type());
              } else if (attr_node.agg == COUNT_AGG) {
                sql_expr->setType(INTS);
              } else {
                sql_expr->setType(FLOATS);
              }
            }
          }
        }
      }

      // create filter statement in `where` statement
      FilterStmt *filter_stmt = nullptr;
      RC rc = FilterStmt::create(db,
          default_table,default_table_alas,
          &table_map,
          select_sql.conditions.data(),
          static_cast<int>(select_sql.conditions.size()),
          top_tables,filter_stmt);
      if (rc != RC::SUCCESS) {
        LOG_WARN("cannot construct filter stmt");
        return rc;
      }
      // everything alright
      SelectStmt *select_stmt = new SelectStmt();
      // TODO add expression copy
      select_stmt->tables_.swap(tables);
      auto attributes(select_sql.attributes);
      std::reverse(attributes.begin(),attributes.end());
      select_stmt->attributes_.swap(attributes);
      select_stmt->filter_stmt_ = filter_stmt;
      select_stmt->query_fields_.swap(query_fields);
      select_stmt->is_agg_= true;
      select_stmt->top_tables_=top_tables;
      select_stmt->is_sub_select_=is_sub_select;
      select_stmt->alias_map_.swap(alias_map);
      select_stmt->col_alias_map_.swap(col_alias_map);
      select_stmt->is_expression_select_attr_=true;
      auto attributes_expression(select_sql.attributes_expression);
      select_stmt->attributes_expression_.swap(attributes_expression);
      select_stmt->is_group_=false;
      //auto stringsqlExprs(select_sql.stringsqlExprs);
      //select_stmt->stringsqlExprs.swap(stringsqlExprs);
      stmt = select_stmt;
      return RC::SUCCESS;

    }else{
      // collect query fields in `select` statement
      std::vector<Field> query_fields;
      for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
        const RelAttrSqlNode &relation_attr = select_sql.attributes[i];

        if (common::is_blank(relation_attr.relation_name.c_str()) &&
            0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
          return RC::INTERNAL;

        } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
          const char *table_name = relation_attr.relation_name.c_str();
          const char *field_name = relation_attr.attribute_name.c_str();

          if (0 == strcmp(table_name, "*")) {
            return RC::INTERNAL;
          } else {
            auto iter = table_map.find(table_name);
            if (iter == table_map.end()) {
              LOG_WARN("no such table in from list: %s", table_name);
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = iter->second;
            if (0 == strcmp(field_name, "*")) {
              return RC::INTERNAL;
            } else {
              const FieldMeta *field_meta = table->table_meta().field(field_name);
              if (nullptr == field_meta) {
                LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              query_fields.push_back(Field(table, field_meta));
              for(auto fieldExpr:select_sql.fieldExprs){
                if(fieldExpr->name().compare(relation_attr.sqlString)==0){
                  fieldExpr->setField(Field(table, field_meta));
                }
              }
            }
          }
        } else {
          if (tables.size() != 1) {
            LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          Table *table = tables[0];
          const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
          if (nullptr == field_meta) {
            LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          query_fields.push_back(Field(table, field_meta));
          for(auto fieldExpr:select_sql.fieldExprs){
            if(fieldExpr->name().compare(relation_attr.sqlString)==0){
              fieldExpr->setField(Field(table, field_meta));
            }
          }
        }
      }
      LOG_INFO("got %d tables in from stmt and %d fields in query stmt", tables.size(), query_fields.size());



      Table *default_table = nullptr;
      if (tables.size() == 1) {
        default_table = tables[0];
      }

      // collect order_by_fields in `select` statement
      std::vector<Field>order_by_fields;
      std::vector<OrderBySequence>order_by_sequences;
      for (int i = static_cast<int>(select_sql.order_by.size()) - 1; i >= 0; i--) {
        const RelAttrSqlNode relation_attr = select_sql.order_by[i].attrs;
        const OrderBySequence order_by_sequence =select_sql.order_by[i].orderBySequence;
        if (common::is_blank(relation_attr.relation_name.c_str()) &&
            0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
          LOG_WARN("invalid  ");
          return RC::SCHEMA_FIELD_MISSING;

        } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
          const char *table_name = relation_attr.relation_name.c_str();
          const char *field_name = relation_attr.attribute_name.c_str();

          if (0 == strcmp(table_name, "*")) {
            LOG_WARN("invalid field name while table is *. attr=%s", field_name);
            return RC::SCHEMA_FIELD_MISSING;
          } else {
            auto iter = table_map.find(table_name);
            if (iter == table_map.end()) {
              LOG_WARN("no such table in from list: %s", table_name);
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = iter->second;
            if (0 == strcmp(field_name, "*")) {
              LOG_WARN("invalid field name while table is *. attr=%s", field_name);
              return RC::SCHEMA_FIELD_MISSING;
            } else {
              const FieldMeta *field_meta = table->table_meta().field(field_name);
              if (nullptr == field_meta) {
                LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              order_by_fields.push_back(Field(table, field_meta));
            }
          }
        } else {
          if (tables.size() != 1) {
            LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          Table *table = tables[0];
          const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
          if (nullptr == field_meta) {
            LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          order_by_fields.push_back(Field(table, field_meta));
        }
        order_by_sequences.push_back(order_by_sequence);
      }

      // create filter statement in `where` statement
      FilterStmt *filter_stmt = nullptr;
      RC rc = FilterStmt::create(db,
          default_table,
          default_table_alas,
          &table_map,
          select_sql.conditions.data(),
          static_cast<int>(select_sql.conditions.size()),
          top_tables,
          filter_stmt);
      if (rc != RC::SUCCESS) {
        LOG_WARN("cannot construct filter stmt");
        return rc;
      }

      // everything alright
      SelectStmt *select_stmt = new SelectStmt();
      // TODO add expression copy
      select_stmt->tables_.swap(tables);
      auto attributes(select_sql.attributes);
      std::reverse(attributes.begin(),attributes.end());
      select_stmt->attributes_.swap(attributes);
      select_stmt->query_fields_.swap(query_fields);
      select_stmt->filter_stmt_ = filter_stmt;
      select_stmt->is_agg_= false;
      select_stmt->order_by_sequences_.swap(order_by_sequences);
      select_stmt->order_by_fields_.swap(order_by_fields);
      select_stmt->top_tables_=top_tables;
      select_stmt->is_sub_select_=is_sub_select;
      select_stmt->alias_map_.swap(alias_map);
      select_stmt->col_alias_map_.swap(col_alias_map);
      select_stmt->is_expression_select_attr_=true;
      auto attributes_expression(select_sql.attributes_expression);
      select_stmt->attributes_expression_.swap(attributes_expression);
      //auto fieldExprs(select_sql.fieldExprs);
      //select_stmt->fieldExprs.swap(fieldExprs);
      select_stmt->is_group_=false;
      stmt = select_stmt;
      return RC::SUCCESS;

    }

  }else{
    if(is_agg){
      if(select_sql.is_group_by){
        if((agg_num+group_by_num)!=static_cast<int>(select_sql.attributes.size())){
          LOG_WARN("invalid argument. agg_num  wrong. ");
          return RC::INVALID_ARGUMENT;
        }

        Table *default_table = nullptr;
        if (tables.size() == 1) {
          default_table = tables[0];
        }

        // collect query fields in `select` statement
          std::vector<Field> query_fields;
          for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
            const RelAttrSqlNode &relation_attr = select_sql.attributes[i];

          if (common::is_blank(relation_attr.relation_name.c_str()) &&
              0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
            FieldMeta *field_meta=new FieldMeta;
            field_meta->init("*");
            query_fields.push_back(Field(default_table, field_meta));

          } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
            const char *table_name = relation_attr.relation_name.c_str();
            const char *field_name = relation_attr.attribute_name.c_str();

            if (0 == strcmp(table_name, "*")) {
              if (0 != strcmp(field_name, "*")) {
                LOG_WARN("invalid field name while table is *. attr=%s", field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }
              FieldMeta *field_meta=new FieldMeta;
              field_meta->init("*");
              query_fields.push_back(Field(default_table, field_meta));
            } else {
              auto iter = table_map.find(table_name);
              if (iter == table_map.end()) {
                LOG_WARN("no such table in from list: %s", table_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              Table *table = iter->second;
              if (0 == strcmp(field_name, "*")) {
                FieldMeta *field_meta=new FieldMeta;
                field_meta->init("*");
                query_fields.push_back(Field(table, field_meta));
              } else {
                const FieldMeta *field_meta = table->table_meta().field(field_name);
                if (nullptr == field_meta) {
                  LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                  return RC::SCHEMA_FIELD_MISSING;
                }

                query_fields.push_back(Field(table, field_meta));
              }
            }
          } else {
            if (tables.size() != 1) {
              LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = tables[0];
            const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
            if (nullptr == field_meta) {
              LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            query_fields.push_back(Field(table, field_meta));
          }
        }

        // collect query fields in `select` statement
        std::vector<Field> group_fields;
        for (int i = static_cast<int>(select_sql.group_by.attrs.size()) - 1; i >= 0; i--) {
          const RelAttrSqlNode &relation_attr = select_sql.group_by.attrs[i];

          if (common::is_blank(relation_attr.relation_name.c_str()) &&
              0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
            FieldMeta *field_meta=new FieldMeta;
            field_meta->init("*");
            group_fields.push_back(Field(default_table, field_meta));

          } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
            const char *table_name = relation_attr.relation_name.c_str();
            const char *field_name = relation_attr.attribute_name.c_str();

            if (0 == strcmp(table_name, "*")) {
              if (0 != strcmp(field_name, "*")) {
                LOG_WARN("invalid field name while table is *. attr=%s", field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }
              FieldMeta *field_meta=new FieldMeta;
              field_meta->init("*");
              group_fields.push_back(Field(default_table, field_meta));
            } else {
              auto iter = table_map.find(table_name);
              if (iter == table_map.end()) {
                LOG_WARN("no such table in from list: %s", table_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              Table *table = iter->second;
              if (0 == strcmp(field_name, "*")) {
                FieldMeta *field_meta=new FieldMeta;
                field_meta->init("*");
                group_fields.push_back(Field(table, field_meta));
              } else {
                const FieldMeta *field_meta = table->table_meta().field(field_name);
                if (nullptr == field_meta) {
                  LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                  return RC::SCHEMA_FIELD_MISSING;
                }

                group_fields.push_back(Field(table, field_meta));
              }
            }
          } else {
            if (tables.size() != 1) {
              LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = tables[0];
            const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
            if (nullptr == field_meta) {
              LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            group_fields.push_back(Field(table, field_meta));
          }
        }



        // create filter statement in `where` statement
        FilterStmt *filter_stmt = nullptr;
        RC rc = FilterStmt::create(db,
            default_table,default_table_alas,
            &table_map,
            select_sql.conditions.data(),
            static_cast<int>(select_sql.conditions.size()),
            top_tables,filter_stmt);
        if (rc != RC::SUCCESS) {
          LOG_WARN("cannot construct filter stmt");
          return rc;
        }

        FilterStmt *having_stmt = nullptr;
        rc = FilterStmt::create(db,
            default_table,default_table_alas,
            &table_map,
            select_sql.group_by.conditions.data(),
            static_cast<int>(select_sql.group_by.conditions.size()),
            top_tables,having_stmt);
        if (rc != RC::SUCCESS) {
          LOG_WARN("cannot construct filter stmt");
          return rc;
        }
        std::vector<RelAttrSqlNode> having_rels;
        auto condition_num=select_sql.group_by.conditions.size();
        for (int i = 0; i < condition_num; i++) {
          auto condition=select_sql.group_by.conditions[i];
          if(condition.left_type==ATTR_TYPE){
            having_rels.push_back(condition.left_attr);
          }
          if(condition.right_type==ATTR_TYPE){
            having_rels.push_back(condition.right_attr);
          }
        }

        // collect query fields in `select` statement
        std::vector<Field> having_fields;
        for (int i = static_cast<int>(having_rels.size()) - 1; i >= 0; i--) {
           RelAttrSqlNode &relation_attr = having_rels[i];

          if (common::is_blank(relation_attr.relation_name.c_str()) &&
              0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
            FieldMeta *field_meta=new FieldMeta;
            field_meta->init("*");
            having_fields.push_back(Field(default_table, field_meta));

          } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
            const char *table_name = relation_attr.relation_name.c_str();
            const char *field_name = relation_attr.attribute_name.c_str();

            if (0 == strcmp(table_name, "*")) {
              if (0 != strcmp(field_name, "*")) {
                LOG_WARN("invalid field name while table is *. attr=%s", field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }
              FieldMeta *field_meta=new FieldMeta;
              field_meta->init("*");
              having_fields.push_back(Field(default_table, field_meta));
            } else {
              auto iter = table_map.find(table_name);
              if (iter == table_map.end()) {
                LOG_WARN("no such table in from list: %s", table_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              Table *table = iter->second;
              if (0 == strcmp(field_name, "*")) {
                FieldMeta *field_meta=new FieldMeta;
                field_meta->init("*");
                having_fields.push_back(Field(table, field_meta));
              } else {
                const FieldMeta *field_meta = table->table_meta().field(field_name);
                if (nullptr == field_meta) {
                  LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                  return RC::SCHEMA_FIELD_MISSING;
                }

                having_fields.push_back(Field(table, field_meta));
              }
            }
          } else {
            if (tables.size() != 1) {
              LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = tables[0];
            const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
            if (nullptr == field_meta) {
              LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            relation_attr.relation_name.assign(table->name());
            having_fields.push_back(Field(table, field_meta));
          }
        }


        // everything alright
        SelectStmt *select_stmt = new SelectStmt();
        // TODO add expression copy
        select_stmt->tables_.swap(tables);
        auto attributes(select_sql.attributes);
        std::reverse(attributes.begin(),attributes.end());
        select_stmt->attributes_.swap(attributes);
        select_stmt->filter_stmt_ = filter_stmt;
        select_stmt->query_fields_.swap(query_fields);
        select_stmt->is_agg_= true;
        select_stmt->top_tables_=top_tables;
        select_stmt->is_sub_select_=is_sub_select;
        select_stmt->alias_map_.swap(alias_map);
        select_stmt->col_alias_map_.swap(col_alias_map);
        select_stmt->is_expression_select_attr_=false;
        select_stmt->is_group_=true;
        select_stmt->group_by_fields_.swap(group_fields);
        select_stmt->having_filter_stmt_=having_stmt;
        select_stmt->having_rels_.swap(having_rels);
        select_stmt->having_fields_.swap(having_fields);
        stmt = select_stmt;
        return RC::SUCCESS;

      }else{
        if(agg_num!=static_cast<int>(select_sql.attributes.size())){
          LOG_WARN("invalid argument. agg_num  wrong. ");
          return RC::INVALID_ARGUMENT;
        }

        Table *default_table = nullptr;
        if (tables.size() == 1) {
          default_table = tables[0];
        }

        // collect query fields in `select` statement
        std::vector<Field> query_fields;
        for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
          const RelAttrSqlNode &relation_attr = select_sql.attributes[i];

          if (common::is_blank(relation_attr.relation_name.c_str()) &&
              0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
            FieldMeta *field_meta=new FieldMeta;
            field_meta->init("*");
            query_fields.push_back(Field(default_table, field_meta));

          } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
            const char *table_name = relation_attr.relation_name.c_str();
            const char *field_name = relation_attr.attribute_name.c_str();

            if (0 == strcmp(table_name, "*")) {
              if (0 != strcmp(field_name, "*")) {
                LOG_WARN("invalid field name while table is *. attr=%s", field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }
              FieldMeta *field_meta=new FieldMeta;
              field_meta->init("*");
              query_fields.push_back(Field(default_table, field_meta));
            } else {
              auto iter = table_map.find(table_name);
              if (iter == table_map.end()) {
                LOG_WARN("no such table in from list: %s", table_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              Table *table = iter->second;
              if (0 == strcmp(field_name, "*")) {
                FieldMeta *field_meta=new FieldMeta;
                field_meta->init("*");
                query_fields.push_back(Field(table, field_meta));
              } else {
                const FieldMeta *field_meta = table->table_meta().field(field_name);
                if (nullptr == field_meta) {
                  LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                  return RC::SCHEMA_FIELD_MISSING;
                }

                query_fields.push_back(Field(table, field_meta));
              }
            }
          } else {
            if (tables.size() != 1) {
              LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = tables[0];
            const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
            if (nullptr == field_meta) {
              LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
              return RC::SCHEMA_FIELD_MISSING;
            }

            query_fields.push_back(Field(table, field_meta));
          }
        }

        // create filter statement in `where` statement
        FilterStmt *filter_stmt = nullptr;
        RC rc = FilterStmt::create(db,
            default_table,default_table_alas,
            &table_map,
            select_sql.conditions.data(),
            static_cast<int>(select_sql.conditions.size()),
            top_tables,filter_stmt);
        if (rc != RC::SUCCESS) {
          LOG_WARN("cannot construct filter stmt");
          return rc;
        }
        // everything alright
        SelectStmt *select_stmt = new SelectStmt();
        // TODO add expression copy
        select_stmt->tables_.swap(tables);
        auto attributes(select_sql.attributes);
        std::reverse(attributes.begin(),attributes.end());
        select_stmt->attributes_.swap(attributes);
        select_stmt->filter_stmt_ = filter_stmt;
        select_stmt->query_fields_.swap(query_fields);
        select_stmt->is_agg_= true;
        select_stmt->top_tables_=top_tables;
        select_stmt->is_sub_select_=is_sub_select;
        select_stmt->alias_map_.swap(alias_map);
        select_stmt->col_alias_map_.swap(col_alias_map);
        select_stmt->is_expression_select_attr_=false;
        select_stmt->is_group_=false;
        stmt = select_stmt;
        return RC::SUCCESS;
      }


    }else{
      // collect query fields in `select` statement
      std::vector<Field> query_fields;
      std::vector<Expression*> all_expressions;
      for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
        const RelAttrSqlNode &relation_attr = select_sql.attributes[i];

        if (common::is_blank(relation_attr.relation_name.c_str()) &&
            0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
          for (Table *table : tables) {
            wildcard_fields(table, query_fields);
            wildcard_expressions(table, all_expressions);
          }

        } else if (!common::is_blank(relation_attr.relation_name.c_str())) {
          const char *table_name = relation_attr.relation_name.c_str();
          const char *field_name = relation_attr.attribute_name.c_str();

          if (0 == strcmp(table_name, "*")) {
            if (0 != strcmp(field_name, "*")) {
              LOG_WARN("invalid field name while table is *. attr=%s", field_name);
              return RC::SCHEMA_FIELD_MISSING;
            }
            for (Table *table : tables) {
              wildcard_fields(table, query_fields);
              wildcard_expressions(table, all_expressions);
            }
          } else {
            auto iter = table_map.find(table_name);
            if (iter == table_map.end()) {
              LOG_WARN("no such table in from list: %s", table_name);
              return RC::SCHEMA_FIELD_MISSING;
            }

            Table *table = iter->second;
            if (0 == strcmp(field_name, "*")) {
              wildcard_fields(table, query_fields);
              wildcard_expressions(table, all_expressions);
            } else {
              const FieldMeta *field_meta = table->table_meta().field(field_name);
              if (nullptr == field_meta) {
                LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              query_fields.push_back(Field(table, field_meta));
              if (relation_attr.func == NO_FUNC) {
                all_expressions.push_back(static_cast<Expression*>(new FieldExpr(table, field_meta)));
              } else {
                switch (relation_attr.func)
                {
                case LENGTH_FUNC:
                  all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), LENGTH_FUNC, relation_attr.lengthparam)));
                  break;
                case ROUND_FUNC:
                  all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), ROUND_FUNC, relation_attr.roundparam)));
                  break;
                case FORMAT_FUNC:
                  all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), FORMAT_FUNC, relation_attr.formatparam)));
                  break;
                default:
                  return RC::UNIMPLENMENT;
                }
                
              }
            }
          }
        } else {
          if (relation_attr.func != NO_FUNC && relation_attr.attribute_name.empty()) {
            // 输入为原始字符串  e.g. select length('this is a string') as len [from t];
            Value v;
            switch (relation_attr.func)
            {
            case LENGTH_FUNC:
            {
              v.set_int(relation_attr.lengthparam.raw_data.get_string().size());
            } break;
            case ROUND_FUNC:
            {
              float raw_data = relation_attr.roundparam.raw_data.get_float();
              if (relation_attr.roundparam.bits.length() == 0) {
                // 只有一个参数
                v.set_int(round(raw_data));
              } else if (relation_attr.roundparam.bits.attr_type() != INTS) {
                return RC::SQL_SYNTAX;
              } else {
                raw_data *= pow(10, relation_attr.roundparam.bits.get_int());
                v.set_float(round(raw_data)/pow(10, relation_attr.roundparam.bits.get_int()));
              }
            } break;
            case FORMAT_FUNC:
            {
              std::string raw_date = relation_attr.formatparam.raw_data.get_string();
              std::string format = relation_attr.formatparam.format.get_string();
              v.set_string(formatDate(raw_date.c_str(), format.c_str()).c_str());
            } break;
            
            default:
              return RC::UNIMPLENMENT;
            }
            all_expressions.push_back(static_cast<Expression*>(new ValueExpr(v)));
            continue;
          }

          if (tables.size() != 1) {
            LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          Table *table = tables[0];
          const FieldMeta *field_meta = table->table_meta().field(relation_attr.attribute_name.c_str());
          if (nullptr == field_meta) {
            LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), relation_attr.attribute_name.c_str());
            return RC::SCHEMA_FIELD_MISSING;
          }

          query_fields.push_back(Field(table, field_meta));
          if (relation_attr.func == NO_FUNC) {
            all_expressions.push_back(static_cast<Expression*>(new FieldExpr(table, field_meta)));
          } else {
            switch (relation_attr.func)
            {
            case LENGTH_FUNC:
              all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), LENGTH_FUNC, relation_attr.lengthparam)));
              break;
            case ROUND_FUNC:
              all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), ROUND_FUNC, relation_attr.roundparam)));
              break;
            case FORMAT_FUNC:
              all_expressions.push_back(static_cast<Expression*>(new FuncExpr(Field(table, field_meta), FORMAT_FUNC, relation_attr.formatparam)));
              break;
            default:
              return RC::UNIMPLENMENT;
            }          
          }

        }
      }
      LOG_INFO("got %d tables in from stmt and %d fields in query stmt", tables.size(), query_fields.size());



      Table *default_table = nullptr;
      if (tables.size() == 1) {
        default_table = tables[0];
      }

      // Collect sort_fields in the `select` statement
      std::vector<Field> sort_fields;
      std::vector<OrderBySequence> sort_orders;

      for (size_t idx = 0; idx < select_sql.order_by.size(); ++idx) {
        const RelAttrSqlNode attr_node = select_sql.order_by[idx].attrs;
        const OrderBySequence sort_order = select_sql.order_by[idx].orderBySequence;

        const char *rel_name = attr_node.relation_name.c_str();
        const char *attr_name = attr_node.attribute_name.c_str();

        bool is_rel_name_blank = common::is_blank(rel_name);
        bool is_attr_name_star = (strcmp(attr_name, "*") == 0);
        bool is_rel_name_star = (strcmp(rel_name, "*") == 0);

        if (is_rel_name_blank && is_attr_name_star) {
          LOG_WARN("Invalid field specification.");
          return RC::SCHEMA_FIELD_MISSING;
        }

        Table *table_ptr = nullptr;
        const FieldMeta *field_meta_ptr = nullptr;

        if (!is_rel_name_blank) {
          if (is_rel_name_star) {
            LOG_WARN("Invalid field name when table is '*'. Attribute: %s", attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          auto table_iter = table_map.find(rel_name);
          if (table_iter == table_map.end()) {
            LOG_WARN("No such table in FROM clause: %s", rel_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          table_ptr = table_iter->second;

          if (is_attr_name_star) {
            LOG_WARN("Invalid field name when attribute is '*'. Table: %s", rel_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          field_meta_ptr = table_ptr->table_meta().field(attr_name);
          if (field_meta_ptr == nullptr) {
            LOG_WARN("No such field: %s.%s.%s", db->name(), table_ptr->name(), attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }
        } else {
          if (tables.size() != 1) {
            LOG_WARN("Ambiguous attribute without table name: %s", attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }

          table_ptr = tables[0];
          field_meta_ptr = table_ptr->table_meta().field(attr_name);
          if (field_meta_ptr == nullptr) {
            LOG_WARN("No such field: %s.%s.%s", db->name(), table_ptr->name(), attr_name);
            return RC::SCHEMA_FIELD_MISSING;
          }
        }

        sort_fields.push_back(Field(table_ptr, field_meta_ptr));
        sort_orders.push_back(sort_order);
      }


      // create filter statement in `where` statement
      FilterStmt *filter_stmt = nullptr;
      RC rc = FilterStmt::create(db,
          default_table,
          default_table_alas,
          &table_map,
          select_sql.conditions.data(),
          static_cast<int>(select_sql.conditions.size()),
          top_tables,
          filter_stmt);
      if (rc != RC::SUCCESS) {
        LOG_WARN("cannot construct filter stmt");
        return rc;
      }

      // everything alright
      SelectStmt *select_stmt = new SelectStmt();
      // TODO add expression copy
      select_stmt->tables_.swap(tables);
      auto attributes(select_sql.attributes);
      std::reverse(attributes.begin(),attributes.end());
      select_stmt->col_alias_map_.swap(col_alias_map);
      select_stmt->is_expression_select_attr_=false;
      select_stmt->all_expressions_.swap(all_expressions);
      select_stmt->is_group_=false;
      select_stmt->attributes_.swap(attributes);
      select_stmt->query_fields_.swap(query_fields);
      select_stmt->filter_stmt_ = filter_stmt;
      select_stmt->is_agg_= false;
      select_stmt->order_by_fields_.swap(sort_fields);
      select_stmt->order_by_sequences_.swap(sort_orders);
      select_stmt->top_tables_=top_tables;
      select_stmt->is_sub_select_=is_sub_select;
      select_stmt->alias_map_.swap(alias_map);
      stmt = select_stmt;
      return RC::SUCCESS;

    }
  }





}
