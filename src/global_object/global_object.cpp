//
// Created by tzih on 24-6-8.
//
#include "global_object.h"


std::unique_ptr<DiskManager> disk_manager = std::make_unique<DiskManager>();
std::unique_ptr<BufferPoolManager> buffer_pool_manager = std::make_unique<BufferPoolManager>(BUFFER_POOL_SIZE, disk_manager.get());
std::unique_ptr<RmManager> rm_manager = std::make_unique<RmManager>(disk_manager.get(), buffer_pool_manager.get());
std::unique_ptr<IxManager> ix_manager = std::make_unique<IxManager>(disk_manager.get(), buffer_pool_manager.get());
std::unique_ptr<SmManager> sm_manager = std::make_unique<SmManager>(disk_manager.get(), buffer_pool_manager.get(), rm_manager.get(), ix_manager.get());
std::unique_ptr<LockManager> lock_manager = std::make_unique<LockManager>();
std::unique_ptr<TransactionManager> txn_manager = std::make_unique<TransactionManager>(lock_manager.get(), sm_manager.get());
std::unique_ptr<Planner> planner = std::make_unique<Planner>(sm_manager.get());
std::unique_ptr<Optimizer> optimizer = std::make_unique<Optimizer>(sm_manager.get(), planner.get());
std::unique_ptr<QlManager> ql_manager = std::make_unique<QlManager>(sm_manager.get(), txn_manager.get());
std::unique_ptr<LogManager> log_manager = std::make_unique<LogManager>(disk_manager.get());
std::unique_ptr<RecoveryManager> recovery = std::make_unique<RecoveryManager>(disk_manager.get(), buffer_pool_manager.get(), sm_manager.get());
std::unique_ptr<Portal> portal = std::make_unique<Portal>(sm_manager.get());
std::unique_ptr<Analyze> analyze = std::make_unique<Analyze>(sm_manager.get());
