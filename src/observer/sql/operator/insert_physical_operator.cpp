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
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/insert_physical_operator.h"
#include "sql/stmt/insert_stmt.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "storage/index/bplus_tree_index.h"

using namespace std;

InsertPhysicalOperator::InsertPhysicalOperator(Table *table, vector<Value> &&values)
    : table_(table), values_(std::move(values))
{}

RC InsertPhysicalOperator::open(Trx *trx)
{


  RC rc;
  const int field_num=table_->table_meta().field_num()-table_->table_meta().sys_field_num();
  for(int j=0;j<values_.size();j+=field_num) {
    Record record;
    rc = table_->make_record(field_num, values_.data(), record, j);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to make record. rc=%s", strrc(rc));
      return rc;
    }
    std::vector<IndexMeta> indexMeta = table_->get_all_index_meta();
    for (int i = 0; i < indexMeta.size(); i++) {
      if (indexMeta[i].is_unique()) {
        Index *index  = table_->find_index(indexMeta[i].name());
        int null_bitmap_len = align8(field_num) / 8;
        int null_bitmap_start=table_->table_meta().field(table_->table_meta().sys_field_num())->offset()-null_bitmap_len;
        common::Bitmap null_bitmap(record.data()+null_bitmap_start,null_bitmap_len);
        bool is_null=false;
        for(int x=0;x<index->field_meta().size();x++){
          if(null_bitmap.get_bit(table_->table_meta().find_field_index_by_name(index->field_meta()[x].name())-table_->table_meta().sys_field_num())){
            is_null=true;
            break;
          }
        }

        if(is_null){
          continue;
        }
        auto   offset = index->field_meta()[0].offset();
        auto   key    = record.data() + offset;
        auto   scan   = index->create_scanner(key, offset, true, key, offset, true);
        RID    rid;
        for (;;) {
          RC rc = scan->next_entry(&rid);
          if (rc == RC::SUCCESS) {
            Record record1;
            table_->get_record(rid, record1);
            bool same = true;
            for (int j = 0; j < index->field_meta().size(); j++) {
              for (int x = index->field_meta()[j].offset();
                   x < index->field_meta()[j].len() + index->field_meta()[j].offset();
                   x++) {
                if (record.data()[x] != record1.data()[x]) {
                  same = false;
                }
              }
            }
            if (same) {
              LOG_WARN("failed to insert  unique. rc=%s", strrc(rc));
              return RC::INTERNAL;
            }

          } else {
            break;
          }
        }
      }
    }
    rc = trx->insert_record(table_, record);
  }
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to insert record by transaction. rc=%s", strrc(rc));
  }
  return rc;
}

RC InsertPhysicalOperator::next()
{
  return RC::RECORD_EOF;
}

RC InsertPhysicalOperator::close()
{
  return RC::SUCCESS;
}
