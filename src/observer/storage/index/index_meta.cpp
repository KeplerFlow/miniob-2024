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
// Created by Wangyunlai.wyl on 2021/5/18.
//

#include "storage/index/index_meta.h"
#include "storage/field/field_meta.h"
#include "storage/table/table_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "json/json.h"

const static Json::StaticString FIELD_NAME("name");
const static Json::StaticString INDEX_FIELD_NAMES("index_field_names");
const static Json::StaticString FIELD_FIELD_NAME("field_name");
const static Json::StaticString UNIQUE("unique");

RC IndexMeta::init(const char *name, std::vector<const FieldMeta *> &field,bool is_unique)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init index, name is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_ = name;
  for(int i=0;i<field.size();i++){
    field_.push_back(field[i]->name());
  }
  is_unique_=is_unique;
  return RC::SUCCESS;
}

RC IndexMeta::from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index)
{
  const Json::Value &name_value = json_value[FIELD_NAME];
  const Json::Value &fields_values = json_value[INDEX_FIELD_NAMES];
  const Json::Value & is_unique=json_value[UNIQUE];
  if (!name_value.isString()) {
    LOG_ERROR("Index name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  if (!fields_values.isArray()) {
    LOG_ERROR("Invalid index_fields_metas, json value=%s", fields_values.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if(!is_unique.isBool()){
    LOG_ERROR("Unique of index [%s] is not a bool.",
        name_value.asCString());
    return RC::INTERNAL;
  }
  bool unique=is_unique.asBool();
  RC rc = RC::SUCCESS;
  int field_num = fields_values.size();
  std::vector< const  FieldMeta *> fields;
  for (int i = 0; i < field_num; i++) {
    const Json::Value &field_name_value = fields_values[i];
    if (!field_name_value[FIELD_NAME].isString()) {
      LOG_ERROR("Field name is not a string. json value=%s", field_name_value.toStyledString().c_str());
      return RC::INTERNAL;
    }
    const FieldMeta *field = table.field(field_name_value[FIELD_NAME].asCString());
    fields.push_back(field);
  }

  return index.init(name_value.asCString(), fields,unique);
}

void IndexMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME] = name_;
  Json::Value fields_value;
  for (const std::string &field : field_) {
    Json::Value field_value;
    field_value[FIELD_NAME] = field;
    fields_value.append(std::move(field_value));
  }
  json_value[INDEX_FIELD_NAMES] = std::move(fields_value);
  json_value[UNIQUE]=is_unique_;
}

const char *IndexMeta::name() const
{
  return name_.c_str();
}

std::vector<std::string> IndexMeta::field() const
{
  return field_;
}
bool IndexMeta::is_unique()const{
  return is_unique_;
}
void IndexMeta::desc(std::ostream &os) const
{
 // os << "index name=" << name_ << ", field=" << field_<<", unique="<<is_unique_;
}