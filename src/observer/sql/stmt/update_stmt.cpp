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
// Created by Wangyunlai on 2022/5/22.
//

#include "sql/stmt/update_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
#include "common/lang/string.h"
UpdateStmt::  UpdateStmt(Table *table, std::map<int,Value>value_map, std::map<int,SelectStmt*> select_map,FilterStmt *filter_stmt, std:: vector<const FieldMeta *>field_meta)
    : table_ (table), value_map_(value_map),select_map_(select_map), filter_stmt_(filter_stmt), field_meta_(field_meta)
{}

RC UpdateStmt::create(Db *db,  UpdateSqlNode &update, Stmt *&stmt)
{
  // Extracting the basic info and checking the initial conditions
const char *table_name = update.relation_name.c_str();
auto field_updates = update.updateValue_list;
if (db == nullptr || table_name == nullptr || field_updates.empty()) {
    LOG_WARN("Invalid input parameters. Database=%p, Table Name=%p", db, table_name);
    return RC::INVALID_ARGUMENT;
}

// Verifying the existence of the table in the database
Table *target_table = db->find_table(table_name);
if (!target_table) {
    LOG_WARN("Table not found in database. Database Name=%s, Table=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
}
const TableMeta &meta_info = target_table->table_meta();

// Preparing to process updates
std::map<int, Value> update_value_map;
std::map<int, SelectStmt*> select_stmt_map;
std::vector<const FieldMeta *> meta_fields;
const int num_updates = field_updates.size();

for (int idx = 0; idx < num_updates; ++idx) {
    const FieldMeta *meta_ptr = meta_info.field(field_updates[idx].attribute_name.c_str());
    meta_fields.push_back(meta_ptr);

    if (!meta_ptr) {
        LOG_WARN("Field does not exist. Table=%s, Field=%s", table_name, field_updates[idx].attribute_name.c_str());
        return RC::SCHEMA_FIELD_NOT_EXIST;
    }

    AttrType expected_type = meta_ptr->type();
    if (field_updates[idx].is_select) {
        Stmt *selection_stmt = nullptr;
        RC selection_rc = SelectStmt::create(db, field_updates[idx].selectSqlNode, selection_stmt);
        if (selection_rc != RC::SUCCESS) {
            LOG_WARN("Failed to create select statement");
            return selection_rc;
        }
        select_stmt_map[idx] = reinterpret_cast<SelectStmt*>(selection_stmt);
    } else {
        AttrType provided_type = field_updates[idx].value.attr_type();
        Value current_value = field_updates[idx].value;
        
        if (expected_type != provided_type) {
            switch (expected_type) {
                case CHARS:
                    if (provided_type == INTS || provided_type == FLOATS) {
                        std::string str_val = (provided_type == INTS) ? common::int2string(current_value.get_int()) : common::float2string(current_value.get_float());
                        current_value.set_string(str_val.c_str());
                        current_value.set_type(CHARS);
                    } else if (provided_type != NULLS || !meta_ptr->is_null()) {
                        LOG_WARN("Type mismatch. Table=%s, Field=%s, Expected=%d, Found=%d", table_name, meta_ptr->name(), expected_type, provided_type);
                        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
                    }
                    break;
                case INTS:
                case FLOATS:
                    if (provided_type == CHARS) {
                        float float_val = common::string2float(current_value.get_string());
                        current_value.set_type(expected_type);
                        if (expected_type == INTS) {
                            current_value.set_int(static_cast<int>(float_val));
                        } else {
                            current_value.set_float(float_val);
                        }
                    } else if (provided_type == NULLS && !meta_ptr->is_null()) {
                        LOG_WARN("Null value not allowed. Table=%s, Field=%s", table_name, meta_ptr->name());
                        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
                    }
                    break;
                default:
                    LOG_WARN("Unsupported field type. Table=%s, Field=%s", table_name, meta_ptr->name());
                    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
            }
        }
        if (provided_type == DATES && !common::is_valid_date(current_value.data())) {
            LOG_WARN("Invalid date format provided. Table=%s, Field=%s", table_name, meta_ptr->name());
            return RC::INVALID_ARGUMENT;
        }
        update_value_map[idx] = current_value;
    }
}

FilterStmt *filter_statement = nullptr;
std::unordered_map<std::string, Table *> table_map;
table_map.insert({table_name, target_table});
RC filter_rc = FilterStmt::create(db, target_table, std::string(), &table_map, update.conditions.data(), static_cast<int>(update.conditions.size()), std::unordered_map<std::string, Table *>(), filter_statement);
if (filter_rc != RC::SUCCESS) {
    LOG_WARN("Filter statement construction failed");
    return filter_rc;
}
stmt = new UpdateStmt(target_table, update_value_map, select_stmt_map, filter_statement, meta_fields);
return RC::SUCCESS;

}
