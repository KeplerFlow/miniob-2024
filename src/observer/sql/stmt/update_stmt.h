#pragma once

class Table;
class UpdateUnit;
struct UpdateStmt;

#include "sql/stmt/filter_stmt.h"
#include "sql/stmt/select_stmt.h"

struct UpdateUnit {
  const FieldMeta* update_field_meta{nullptr};
  SelectStmt *select_stmt{nullptr};
  bool is_select{false};
  Expression *update_expr{nullptr};

};

class UpdateStmt : public Stmt
{
public:

  UpdateStmt() = default;
  ~UpdateStmt() override;

  StmtType type() const override { return StmtType::UPDATE; }

private:
  Table *table_ = nullptr;
  FilterStmt *filter_stmt_ = nullptr;
  std::vector<UpdateUnit> units_;

public:
  static RC create(Db *db, Updates &update_sql, Stmt *&stmt);
  Table *table() const { return table_; }
  FilterStmt *filter_stmt() const { return filter_stmt_; }
  const std::vector<UpdateUnit> &units() const { return units_; }


};
