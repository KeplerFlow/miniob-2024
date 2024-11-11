//
// Created by hxy on 23-10-17.
//

#pragma once

#include <string>
#include <vector>
#include "common/rc.h"
#include "sql/stmt/stmt.h"

class Db;
class Table;
/**
 * @brief 表示创建表的语句
 * @ingroup Statement
 * @details 虽然解析成了stmt，但是与原始的SQL解析后的数据也差不多
 */
class DropTableStmt : public Stmt
{
public:
  DropTableStmt() = default;
  DropTableStmt(Table *table);
  virtual ~DropTableStmt() = default;

  StmtType type() const override { return StmtType::DROP_TABLE; }

  Table *table() const
  {
    return table_;
  }

  static RC create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt);

private:
  Table *table_ = nullptr;
};