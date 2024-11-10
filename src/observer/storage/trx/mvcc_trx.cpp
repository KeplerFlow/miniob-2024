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
// Created by Wangyunlai on 2023/04/24.
//

#include <limits>
#include "storage/trx/mvcc_trx.h"
#include "storage/field/field.h"
#include "storage/clog/clog.h"
#include "storage/db/db.h"
#include "storage/clog/clog.h"
#include "storage/index/index.h"
using namespace std;

MvccTrxKit::~MvccTrxKit()
{
  vector<Trx *> tmp_trxes;
  tmp_trxes.swap(trxes_);
  
  for (Trx *trx : tmp_trxes) {
    delete trx;
  }
}

RC MvccTrxKit::init()
{
  fields_ = vector<FieldMeta>{
    FieldMeta("__trx_xid_begin", AttrType::INTS, 0/*attr_offset*/, 4/*attr_len*/, false/*visible*/),
    FieldMeta("__trx_xid_end",   AttrType::INTS, 0/*attr_offset*/, 4/*attr_len*/, false/*visible*/)
  };

  LOG_INFO("init mvcc trx kit done.");
  return RC::SUCCESS;
}

const vector<FieldMeta> *MvccTrxKit::trx_fields() const
{
  return &fields_;
}

int32_t MvccTrxKit::next_trx_id()
{
  return ++current_trx_id_;
}

int32_t MvccTrxKit::max_trx_id() const
{
  return numeric_limits<int32_t>::max();
}

Trx *MvccTrxKit::create_trx(CLogManager *log_manager)
{
  Trx *trx = new MvccTrx(*this, log_manager);
  if (trx != nullptr) {
    lock_.lock();
    trxes_.push_back(trx);
    lock_.unlock();
  }
  return trx;
}

Trx *MvccTrxKit::create_trx(int32_t trx_id)
{
  Trx *trx = new MvccTrx(*this, trx_id);
  if (trx != nullptr) {
    lock_.lock();
    trxes_.push_back(trx);
    if (current_trx_id_ < trx_id) {
      current_trx_id_ = trx_id;
    }
    lock_.unlock();
  }
  return trx;
}

void MvccTrxKit::destroy_trx(Trx *trx)
{
  lock_.lock();
  for (auto iter = trxes_.begin(), itend = trxes_.end(); iter != itend; ++iter) {
    if (*iter == trx) {
      trxes_.erase(iter);
      break;
    }
  }
  lock_.unlock();

  delete trx;
}

Trx *MvccTrxKit::find_trx(int32_t trx_id)
{
  lock_.lock();
  for (Trx *trx : trxes_) {
    if (trx->id() == trx_id) {
      lock_.unlock();
      return trx;
    }
  }
  lock_.unlock();

  return nullptr;
}

RC MvccTrx::rollback()
{
  RC rc = RC::SUCCESS;
  started_ = false;
  
  for (const Operation &operation : operations_) {
    switch (operation.type()) {
      case Operation::Type::INSERT: {
        RID rid(operation.page_num(), operation.slot_num());
        Record record;
        Table *table = operation.table();
        rc = table->get_record(rid, record); 
        ASSERT(rc == RC::SUCCESS, "failed to get record while rollback. rid=%s, rc=%s", 
               rid.to_string().c_str(), strrc(rc));
        rc = table->delete_record(record);
        ASSERT(rc == RC::SUCCESS, "failed to delete record while rollback. rid=%s, rc=%s",
              rid.to_string().c_str(), strrc(rc));
      } break;

      case Operation::Type::DELETE: {
        Table *table = operation.table();
        RID rid(operation.page_num(), operation.slot_num());
        
        ASSERT(rc == RC::SUCCESS, "failed to get record while rollback. rid=%s, rc=%s",
              rid.to_string().c_str(), strrc(rc));
        Field begin_xid_field, end_xid_field;
        trx_fields(table, begin_xid_field, end_xid_field);

        auto record_updater = [this, &end_xid_field](Record &record) {
          ASSERT(end_xid_field.get_int(record) == -trx_id_, 
                "got an invalid record while rollback. end xid=%d, this trx id=%d", 
                end_xid_field.get_int(record), trx_id_);

          end_xid_field.set_int(record, trx_kit_.max_trx_id());
        };
        
        rc = table->visit_record(rid, false/*readonly*/, record_updater);
        ASSERT(rc == RC::SUCCESS, "failed to get record while committing. rid=%s, rc=%s",
               rid.to_string().c_str(), strrc(rc));
      } break;

      default: {
        ASSERT(false, "unsupported operation. type=%d", static_cast<int>(operation.type()));
      }
    }
  }

  operations_.clear();

  if (!recovering_) {
    rc = log_manager_->rollback_trx(trx_id_);
  }
  LOG_TRACE("append trx rollback log. trx id=%d, rc=%s", trx_id_, strrc(rc));
  return rc;
}

void MvccTrxKit::all_trxes(std::vector<Trx *> &trxes)
{
  lock_.lock();
  trxes = trxes_;
  lock_.unlock();
}

////////////////////////////////////////////////////////////////////////////////

MvccTrx::MvccTrx(MvccTrxKit &kit, CLogManager *log_manager) : trx_kit_(kit), log_manager_(log_manager)
{}

MvccTrx::MvccTrx(MvccTrxKit &kit, int32_t trx_id) : trx_kit_(kit), trx_id_(trx_id)
{
  started_ = true;
  recovering_ = true;
}

MvccTrx::~MvccTrx()
{
}

RC MvccTrx::insert_record(Table *table, Record &record)
{

  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  begin_field.set_int(record, -trx_id_);
  end_field.set_int(record, trx_kit_.max_trx_id());

  RC rc = table->insert_record(record);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to insert record into table. rc=%s", strrc(rc));
    return rc;
  }

  rc = log_manager_->append_log(CLogType::INSERT, trx_id_, table->table_id(), record.rid(), record.len(), 0/*offset*/, record.data());
  ASSERT(rc == RC::SUCCESS, "failed to append insert record log. trx id=%d, table id=%d, rid=%s, record len=%d, rc=%s",
      trx_id_, table->table_id(), record.rid().to_string().c_str(), record.len(), strrc(rc));


  operations_.push_back(Operation(Operation::Type::INSERT, table, record.rid()));
  return rc;
}

RC MvccTrx::update_record(Table *table, Record &record, std::vector<const FieldMeta *>field_meta, std::vector<Value> value)
{
  return RC::SUCCESS;
}

RC MvccTrx::delete_record(Table * table, Record &record)
{
  Field begin_field;
  Field end_field;
  trx_fields(table, begin_field, end_field);

  RC delete_result = RC::SUCCESS;

  operations_.push_back(Operation(Operation::Type::DELETE, table, record.rid()));

  return RC::SUCCESS;
}

RC MvccTrx::visit_record(Table *table_reference, Record &current_record, bool is_read_only) {
  Field start_field;
  Field finish_field;
  trx_fields(table_reference, start_field, finish_field);

  int32_t start_xid = start_field.get_int(current_record);
  int32_t finish_xid = finish_field.get_int(current_record);

  RC result_code = RC::SUCCESS;
  if (start_xid > 0 && finish_xid > 0) {
    if (trx_id_ >= start_xid && trx_id_ <= finish_xid) {
      result_code = RC::SUCCESS;
    } else {
      result_code = RC::RECORD_INVISIBLE;
    }
  } else if (start_xid < 0) {
    if (start_xid < 0 && finish_xid < 0) {
      result_code = is_read_only ? RC::RECORD_INVISIBLE : (-start_xid == trx_id_ ? RC::RECORD_INVISIBLE : RC::LOCKED_CONCURRENCY_CONFLICT);
    } else {
      result_code = (-start_xid == trx_id_) ? RC::SUCCESS : RC::RECORD_INVISIBLE;
    }
  } else if (finish_xid < 0) {
    if (is_read_only) {
      result_code = (-finish_xid != trx_id_) ? RC::SUCCESS : RC::RECORD_INVISIBLE;
    } else {
      result_code = (-finish_xid != trx_id_) ? RC::LOCKED_CONCURRENCY_CONFLICT : RC::RECORD_INVISIBLE;
    }
  }

  return result_code;
}

RC find_table(Db *db, const CLogRecord &log_record, Table *&table)
{
  switch (clog_type_from_integer(log_record.header().type_)) {
    case CLogType::INSERT:
    case CLogType::DELETE: {
      const CLogRecordData &data_record = log_record.data_record();
      table = db->find_table(data_record.table_id_);
      if (nullptr == table) {
        LOG_WARN("no such table to redo. table id=%d, log record=%s",
                 data_record.table_id_, log_record.to_string().c_str());
        return RC::SCHEMA_TABLE_NOT_EXIST;
      }
    } break;
    default:{
      // do nothing
    } break;
  }
  return RC::SUCCESS;
}

RC MvccTrx::redo(Db *db, const CLogRecord &log_record)
{
  Table *table = nullptr;
  RC rc = find_table(db, log_record, table);
  if (OB_FAIL(rc)) {
    return rc;
  }

  switch (log_record.log_type()) {
    case CLogType::INSERT: {
      const CLogRecordData &data_record = log_record.data_record();
      Record record;
      record.set_data(const_cast<char *>(data_record.data_), data_record.data_len_);
      record.set_rid(data_record.rid_);
      RC rc = table->recover_insert_record(record);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to recover insert. table=%s, log record=%s, rc=%s",
                 table->name(), log_record.to_string().c_str(), strrc(rc));
        return rc;
      }
      operations_.push_back(Operation(Operation::Type::INSERT, table, record.rid()));
    } break;

    case CLogType::DELETE: {
      const CLogRecordData &data_record = log_record.data_record();
      Field begin_field;
      Field end_field;
      trx_fields(table, begin_field, end_field);

      auto record_updater = [this, &end_field](Record &record) {
        (void)this;
        ASSERT(end_field.get_int(record) == trx_kit_.max_trx_id(), 
               "got an invalid record while committing. end xid=%d, this trx id=%d", 
               end_field.get_int(record), trx_id_);
                
        end_field.set_int(record, -trx_id_);
      };

      RC rc = table->visit_record(data_record.rid_, false/*readonly*/, record_updater);
      ASSERT(rc == RC::SUCCESS, "failed to get record while committing. rid=%s, rc=%s",
             data_record.rid_.to_string().c_str(), strrc(rc));
      
      //operations_.push_back(Operation(Operation::Type::DELETE, table, data_record.rid_));
    } break;

  }

  return RC::SUCCESS;
}

/**
 * @brief 获取指定表上的事务使用的字段
 * 
 * @param table 指定的表
 * @param begin_xid_field 返回处理begin_xid的字段
 * @param end_xid_field   返回处理end_xid的字段
 */
void MvccTrx::trx_fields(Table *table, Field &begin_xid_field, Field &end_xid_field) const
{
  const TableMeta &table_meta = table->table_meta();
  const std::pair<const FieldMeta *, int> trx_fields = table_meta.trx_fields();
  ASSERT(trx_fields.second >= 2, "invalid trx fields number. %d", trx_fields.second);

  begin_xid_field.set_table(table);
  begin_xid_field.set_field(&trx_fields.first[0]);
  end_xid_field.set_table(table);
  end_xid_field.set_field(&trx_fields.first[1]);
}

RC MvccTrx::start_if_need()
{
  if (!started_) {
    ASSERT(operations_.empty(), "try to start a new trx while operations is not empty");
    trx_id_ = trx_kit_.next_trx_id();
    LOG_DEBUG("current thread change to new trx with %d", trx_id_);
    RC rc = log_manager_->begin_trx(trx_id_);
    ASSERT(rc == RC::SUCCESS, "failed to append log to clog. rc=%s", strrc(rc));
    started_ = true;
  }
  return RC::SUCCESS;
}

RC MvccTrx::commit()
{
  int32_t commit_id = trx_kit_.next_trx_id();
  return commit_with_trx_id(commit_id);
}

RC MvccTrx::commit_with_trx_id(int32_t final_xid) {
  RC status = RC::SUCCESS;
  started_ = false;

  for (const Operation &op : operations_) {
    switch (op.type()) {
      case Operation::Type::INSERT: {
        RID record_id(op.page_num(), op.slot_num());
        Table *op_table = op.table();
        Field start_xid_field, stop_xid_field;
        trx_fields(op_table, start_xid_field, stop_xid_field);

        auto insert_commit = [ this, &start_xid_field, final_xid](Record &rec) {
          LOG_DEBUG("Pre-commit insertion: trx id=%d, start xid=%d, final xid=%d, lbt=%s",
                    trx_id_, start_xid_field.get_int(rec), final_xid, lbt());
          ASSERT(start_xid_field.get_int(rec) == -this->trx_id_, 
                 "Error during commit: start xid=%d, trx id=%d", 
                 start_xid_field.get_int(rec), trx_id_);

          start_xid_field.set_int(rec, final_xid);
        };

        status = op_table->visit_record(record_id, false/*readonly*/, insert_commit);
        ASSERT(status == RC::SUCCESS, "Record fetch failure during commit. rid=%s, rc=%s",
               record_id.to_string().c_str(), strrc(status));
      } break;

      case Operation::Type::DELETE: {
        Table *op_table = op.table();
        RID record_id(op.page_num(), op.slot_num());
        Field start_xid_field, stop_xid_field;
        trx_fields(op_table, start_xid_field, stop_xid_field);

        auto delete_commit = [this, &stop_xid_field, final_xid](Record &rec) {
          ASSERT(stop_xid_field.get_int(rec) == -trx_id_, 
                 "Error during commit: stop xid=%d, trx id=%d", 
                 stop_xid_field.get_int(rec), trx_id_);
                
          stop_xid_field.set_int(rec, final_xid);
        };

        status = op_table->visit_record(record_id, false/*readonly*/, delete_commit);
        ASSERT(status == RC::SUCCESS, "Record fetch failure during commit. rid=%s, rc=%s",
               record_id.to_string().c_str(), strrc(status));
      } break;

      default:
        ASSERT(false, "Encountered unsupported operation type=%d", static_cast<int>(op.type()));
    }
  }

  operations_.clear();

  if (!recovering_) {
    status = log_manager_->commit_trx(trx_id_, final_xid);
  }
  LOG_TRACE("Log entry for trx commit: trx id=%d, final_xid=%d, rc=%s", trx_id_, final_xid, strrc(status));
  return status;
}




