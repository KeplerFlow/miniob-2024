#include "sql/stmt/drop_table_stmt.h"
#include "event/sql_debug.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/db/db.h"
#include "storage/table/table.h"
RC DropTableStmt::create(Db *db, const DropTableSqlNode &drop_table, Stmt *&stmt)
{
  const char *table_name = drop_table.relation_name.c_str();
  if (nullptr == db || nullptr == table_name ) {
    LOG_WARN("invalid argument. db=%p, table_name=%p",
        db, table_name);
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // everything alright
  stmt = new DropTableStmt(table);
  return RC::SUCCESS;
}
DropTableStmt::DropTableStmt(Table *table):table_(table)
{}
