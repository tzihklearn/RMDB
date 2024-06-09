//
// Created by tzih on 24-6-8.
//

#ifndef RMDB_GLOBAL_OBJECT_H
#define RMDB_GLOBAL_OBJECT_H

#include "optimizer/optimizer.h"
#include "recovery/log_recovery.h"
#include "optimizer/plan.h"
#include "optimizer/planner.h"
#include "portal.h"
#include "analyze/analyze.h"

extern std::unique_ptr<DiskManager> disk_manager;
extern std::unique_ptr<BufferPoolManager> buffer_pool_manager;
extern std::unique_ptr<RmManager> rm_manager;
extern std::unique_ptr<IxManager> ix_manager;
extern std::unique_ptr<SmManager> sm_manager;
extern std::unique_ptr<LockManager> lock_manager;
extern std::unique_ptr<TransactionManager> txn_manager;
extern std::unique_ptr<Planner> planner;
extern std::unique_ptr<Optimizer> optimizer;
extern std::unique_ptr<QlManager> ql_manager;
extern std::unique_ptr<LogManager> log_manager;
extern std::unique_ptr<RecoveryManager> recovery;
extern std::unique_ptr<Portal> portal;
extern std::unique_ptr<Analyze> analyze;

#endif //RMDB_GLOBAL_OBJECT_H

// 构建全局所需的管理器对象
