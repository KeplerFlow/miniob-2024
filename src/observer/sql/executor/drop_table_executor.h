//
// Created by hxy on 23-10-17.
//

#pragma once

#include "common/rc.h"

class SQLStageEvent;

/**
 * @brief 创建表的执行器
 * @ingroup Executor
 */
class DropTableExecutor
{
public:
  DropTableExecutor() = default;
  virtual ~DropTableExecutor() = default;

  RC execute(SQLStageEvent *sql_event);
};