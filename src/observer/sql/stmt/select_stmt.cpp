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
  if(select_sql.is_group_by){
    for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--){
      const RelAttrSqlNode &relation_attr = select_sql.attributes[i];
      if(relation_attr.agg!=NO_AGG){
        is_agg= true;
        agg_num++;
        if(relation_attr.attribute_name=="*"&&relation_attr.agg!=COUNT_AGG){
          LOG_WARN("invalid argument. agg_num  wrong. ");
          return RC::INVALID_ARGUMENT;
        }
      }else{
        bool is_found=false;
        for(int j=0;j<select_sql.group_by.attrs.size();j++){
          const RelAttrSqlNode &group_by_relation_attr = select_sql.group_by.attrs[j];
          if(common::is_blank(relation_attr.relation_name.c_str())){
            if(common::is_blank(group_by_relation_attr.relation_name.c_str())||group_by_relation_attr.relation_name==tables[0]->table_meta().name()){
              if(relation_attr.attribute_name.compare(group_by_relation_attr.attribute_name)==0){
                group_by_num++;
                is_found=true;
                break;
              }
            }
          }else{
            if(common::is_blank(group_by_relation_attr.relation_name.c_str())){
              if(relation_attr.relation_name==tables[0]->table_meta().name()){
                if(relation_attr.attribute_name.compare(group_by_relation_attr.attribute_name)==0){
                  group_by_num++;
                  is_found=true;
                  break;
                }
              }
            }else{
              if(relation_attr.relation_name==group_by_relation_attr.relation_name){
                if(relation_attr.attribute_name.compare(group_by_relation_attr.attribute_name)==0){
                  group_by_num++;
                  is_found=true;
                  break;
                }
              }
            }
          }

        }
        if(!is_found){
          return RC::INVALID_ARGUMENT;
        }
      }
      if(!relation_attr.is_right){
        return  RC::INVALID_ARGUMENT;
      }
    }
  }else{
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
      for (int i = static_cast<int>(select_sql.attributes.size()) - 1; i >= 0; i--) {
        const RelAttrSqlNode &relation_attr = select_sql.attributes[i];

        if (common::is_blank(relation_attr.relation_name.c_str()) &&
            0 == strcmp(relation_attr.attribute_name.c_str(), "*")) {
          FieldMeta *field_meta=new FieldMeta;
          field_meta->init("*");
          //query_fields.push_back(Field(default_table, field_meta));
          for(auto stringSql:select_sql.stringsqlExprs){
            if(stringSql->name().compare(relation_attr.sqlString)==0){
              stringSql->setType(INTS);
            }
          }

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
            for(auto stringSql:select_sql.stringsqlExprs){
              if(stringSql->name().compare(relation_attr.sqlString)==0){
                stringSql->setType(INTS);
              }
            }
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
              for(auto stringSql:select_sql.stringsqlExprs){
                if(stringSql->name().compare(relation_attr.sqlString)==0){
                  stringSql->setType(INTS);
                }
              }
            } else {
              const FieldMeta *field_meta = table->table_meta().field(field_name);
              if (nullptr == field_meta) {
                LOG_WARN("no such field. field=%s.%s.%s", db->name(), table->name(), field_name);
                return RC::SCHEMA_FIELD_MISSING;
              }

              query_fields.push_back(Field(table, field_meta));
              for(auto stringSql:select_sql.stringsqlExprs){
                if(stringSql->name().compare(relation_attr.sqlString)==0){
                  if(relation_attr.agg==MAX_AGG||relation_attr.agg==MIN_AGG||relation_attr.agg==SUM_AGG){
                    stringSql->setType(field_meta->type());
                  }else if(relation_attr.agg==COUNT_AGG){
                    stringSql->setType(INTS);
                  }else{
                    stringSql->setType(FLOATS);
                  }
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
          for(auto stringSql:select_sql.stringsqlExprs){
            if(stringSql->name().compare(relation_attr.sqlString)==0){
              if(relation_attr.agg==MAX_AGG||relation_attr.agg==MIN_AGG||relation_attr.agg==SUM_AGG){
                stringSql->setType(field_meta->type());
              }else if(relation_attr.agg==COUNT_AGG){
                stringSql->setType(INTS);
              }else{
                stringSql->setType(FLOATS);
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

              //order_by_fields.push_back(Field(table, field_meta));
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

        }
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

            //query_fields.push_back(Field(table, field_meta));
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

            //group_fields.push_back(Field(table, field_meta));
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

              //order_by_fields.push_back(Field(table, field_meta));
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

          //order_by_fields.push_back(Field(table, field_meta));
        }
        //order_by_sequences.push_back(order_by_sequence);
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
      select_stmt->is_expression_select_attr_=false;
      select_stmt->all_expressions_.swap(all_expressions);
      select_stmt->is_group_=false;
      stmt = select_stmt;
      return RC::SUCCESS;

    }
  }





}
