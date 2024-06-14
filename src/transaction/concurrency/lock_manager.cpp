/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "lock_manager.h"

// 等待-死锁策略实现
void LockManager::wait_die(Transaction *txn, LockRequestQueue &request_queue, std::unique_lock<std::mutex> &ul) {
    while (true) {
        bool should_wait = false;
        for (const auto &existing_req: request_queue.request_queue_) {
            if (existing_req.txn_id_ == txn->get_transaction_id()) {
                continue;
            }
            if (existing_req.txn_id_ > txn->get_transaction_id()) {
                should_wait = true;
                break;
            } else {
                throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
            }
        }
        if (should_wait) {
            request_queue.cv_.wait(ul, [txn, &request_queue]() {
                return !is_waiting_required(txn, request_queue);
            });
        } else {
            break;
        }
    }
}

bool LockManager::is_waiting_required(Transaction *txn, const LockRequestQueue &request_queue) {
    for (const auto &req: request_queue.request_queue_) {
        if (req.txn_id_ != txn->get_transaction_id() && req.txn_id_ > txn->get_transaction_id()) {
            return true;
        }
    }
    return false;
}

// 加锁检查
bool LockManager::lock_check(Transaction *txn) {
    switch (txn->get_state()) {
        case TransactionState::COMMITTED:
        case TransactionState::ABORTED:
            return false;
        case TransactionState::DEFAULT:
            txn->set_state(TransactionState::GROWING);
        case TransactionState::GROWING:
            return true;
        case TransactionState::SHRINKING:
            throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHRINKING);
        default:
            throw RMDBError("Invalid transaction state in " + std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
}

// 释放锁检查
bool LockManager::unlock_check(Transaction *txn) {
    switch (txn->get_state()) {
        case TransactionState::COMMITTED:
        case TransactionState::ABORTED:
            return false;
        case TransactionState::DEFAULT:
            return true;
        case TransactionState::GROWING:
            txn->set_state(TransactionState::SHRINKING);
        case TransactionState::SHRINKING:
            return true;
        default:
            throw RMDBError("Invalid transaction state in " + std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
}

// 申请行级共享锁
bool LockManager::lock_shared_on_record(Transaction *txn, const Rid &rid, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, rid, LockDataType::RECORD);
    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        lock_table_it = lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id),
                                            std::forward_as_tuple()).first;
    }

    LockRequest request(txn->get_transaction_id(), LockMode::SHARED);
    auto &request_queue = lock_table_it->second;

//    wait_die(txn, request_queue, ul);

    for (const auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK ||
        request_queue.group_lock_mode_ == GroupLockMode::S || request_queue.group_lock_mode_ == GroupLockMode::IS) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::S;
        request_queue.request_queue_.emplace_back(request);
    } else {
        // 通知所有等待的线程
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }

    request_queue.cv_.notify_all();
    return true;
}

// 申请行级排他锁
bool LockManager::lock_exclusive_on_record(Transaction *txn, const Rid &rid, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, rid, LockDataType::RECORD);
    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        lock_table_it = lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id),
                                            std::forward_as_tuple()).first;
    }

    LockRequest request(txn->get_transaction_id(), LockMode::EXCLUSIVE);
    auto &request_queue = lock_table_it->second;

//    wait_die(txn, request_queue, ul);

    for (const auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::X;
        request_queue.request_queue_.emplace_back(request);
    } else {
        request_queue.request_queue_.emplace_back(request);
        request_queue.cv_.wait(ul, [&] { return request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK; });
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::X;
    }

    request_queue.cv_.notify_all();
    return true;
}

// 申请表级共享锁
bool LockManager::lock_shared_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        lock_table_it = lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id),
                                            std::forward_as_tuple()).first;
    }

    LockRequest request(txn->get_transaction_id(), LockMode::SHARED);
    auto &request_queue = lock_table_it->second;

//    wait_die(txn, request_queue, ul);

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::EXCLUSIVE || req.lock_mode_ == LockMode::S_IX ||
                req.lock_mode_ == LockMode::SHARED) {
                return true;
            } else if (req.lock_mode_ == LockMode::INTENTION_SHARED) {
                if (request_queue.group_lock_mode_ == GroupLockMode::IS ||
                    request_queue.group_lock_mode_ == GroupLockMode::S) {
                    req.lock_mode_ = LockMode::SHARED;
                    request_queue.group_lock_mode_ = GroupLockMode::S;
                    return true;
                } else {
                    // 通知所有等待的线程
                    request_queue.cv_.notify_all();
                    throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
                }
            } else {
                int num_ix = 0;
                for (const auto &req2: request_queue.request_queue_) {
                    if (req2.lock_mode_ == LockMode::INTENTION_EXCLUSIVE) {
                        num_ix++;
                    }
                }
                if (num_ix == 1) {
                    req.lock_mode_ = LockMode::S_IX;
                    request_queue.group_lock_mode_ = GroupLockMode::SIX;
                    return true;
                } else {
                    // 通知所有等待的线程
                    request_queue.cv_.notify_all();
                    throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
                }
            }
        }
    }

    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK ||
        request_queue.group_lock_mode_ == GroupLockMode::S || request_queue.group_lock_mode_ == GroupLockMode::IS ||
        request_queue.group_lock_mode_ == GroupLockMode::SIX) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::S;
        request_queue.request_queue_.emplace_back(request);
    } else {
        // 通知所有等待的线程
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }

    // 确保在任何情况下都会通知等待的事务
    request_queue.cv_.notify_all();
    return true;
}

// 申请表级排他锁
bool LockManager::lock_exclusive_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    if (!lock_table_.count(lock_data_id)) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }

    LockRequest request(txn->get_transaction_id(), LockMode::EXCLUSIVE);
    auto &request_queue = lock_table_[lock_data_id];

//    wait_die(txn, request_queue, ul);

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::EXCLUSIVE) {
                return true;
            } else {
                if (request_queue.request_queue_.size() == 1) {
                    req.lock_mode_ = LockMode::EXCLUSIVE;
                    request_queue.group_lock_mode_ = GroupLockMode::X;
                    return true;
                } else {
                    // 通知所有等待的线程
                    request_queue.cv_.notify_all();
                    throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
                }
            }
        }
    }

    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::X;
        request_queue.request_queue_.emplace_back(request);
    } else {
        // 通知所有等待的线程
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }

    // 确保在任何情况下都会通知等待的事务
    request_queue.cv_.notify_all();
    return true;
}

// 申请表级意向读锁
bool LockManager::lock_IS_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    if (!lock_table_.count(lock_data_id)) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }

    LockRequest request(txn->get_transaction_id(), LockMode::INTENTION_SHARED);
    auto &request_queue = lock_table_[lock_data_id];

//    wait_die(txn, request_queue, ul);

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    if (request_queue.group_lock_mode_ != GroupLockMode::X) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK) {
            request_queue.group_lock_mode_ = GroupLockMode::IS;
        }
        request_queue.request_queue_.emplace_back(request);
        return true;
    } else {
        // 通知所有等待的线程
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
}

// 申请表级意向写锁
bool LockManager::lock_IX_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        lock_table_it = lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id),
                                            std::forward_as_tuple()).first;
    }

    LockRequest request(txn->get_transaction_id(), LockMode::INTENTION_EXCLUSIVE);
    auto &request_queue = lock_table_it->second;

    wait_die(txn, request_queue, ul);

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::INTENTION_EXCLUSIVE || req.lock_mode_ == LockMode::S_IX ||
                req.lock_mode_ == LockMode::EXCLUSIVE) {
                return true;
            } else if (req.lock_mode_ == LockMode::SHARED) {
                int num_s = 0;
                for (const auto &req2: request_queue.request_queue_) {
                    if (req2.lock_mode_ == LockMode::SHARED) {
                        num_s++;
                    }
                }
                if (num_s == 1) {
                    req.lock_mode_ = LockMode::S_IX;
                    request_queue.group_lock_mode_ = GroupLockMode::SIX;
                    return true;
                } else {
                    // 通知所有等待的线程
                    request_queue.cv_.notify_all();
                    throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
                }
            } else {
                if (request_queue.group_lock_mode_ == GroupLockMode::IS ||
                    request_queue.group_lock_mode_ == GroupLockMode::IX) {
                    req.lock_mode_ = LockMode::INTENTION_EXCLUSIVE;
                    request_queue.group_lock_mode_ = GroupLockMode::IX;
                    return true;
                } else {
                    // 通知所有等待的线程
                    request_queue.cv_.notify_all();
                    throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
                }
            }
        }
    }

    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK ||
        request_queue.group_lock_mode_ == GroupLockMode::IS || request_queue.group_lock_mode_ == GroupLockMode::IX) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::IX;
        request_queue.request_queue_.emplace_back(request);
        return true;
    } else {
        // 通知所有等待的线程
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
}

// 释放锁
bool LockManager::unlock(Transaction *txn, LockDataId lock_data_id) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!unlock_check(txn)) {
        return false;
    }

    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        return true;  // 没有找到对应的锁数据ID，直接返回true
    }

    auto &request_queue = lock_table_it->second;

    for (auto req = request_queue.request_queue_.begin(); req != request_queue.request_queue_.end(); ++req) {
        if (req->txn_id_ == txn->get_transaction_id()) {
            request_queue.request_queue_.erase(req);
            break;
        }
    }

    // 通知所有等待的线程
    request_queue.cv_.notify_all();

    // 更新group_lock_mode_并处理可能的锁升级
    int IS_lock_num = 0, IX_lock_num = 0, S_lock_num = 0, SIX_lock_num = 0, X_lock_num = 0;
    for (const auto &req: request_queue.request_queue_) {
        switch (req.lock_mode_) {
            case LockMode::INTENTION_SHARED:
                IS_lock_num++;
                break;
            case LockMode::INTENTION_EXCLUSIVE:
                IX_lock_num++;
                break;
            case LockMode::SHARED:
                S_lock_num++;
                break;
            case LockMode::EXCLUSIVE:
                X_lock_num++;
                break;
            case LockMode::S_IX:
                SIX_lock_num++;
                break;
            default:
                break;
        }
    }

    // 根据统计结果更新group_lock_mode_
    if (X_lock_num > 0) {
        request_queue.group_lock_mode_ = GroupLockMode::X;
    } else if (SIX_lock_num > 0) {
        request_queue.group_lock_mode_ = GroupLockMode::SIX;
    } else if (IX_lock_num > 0) {
        request_queue.group_lock_mode_ = GroupLockMode::IX;
    } else if (S_lock_num > 0) {
        request_queue.group_lock_mode_ = GroupLockMode::S;
    } else if (IS_lock_num > 0) {
        request_queue.group_lock_mode_ = GroupLockMode::IS;
    } else {
        request_queue.group_lock_mode_ = GroupLockMode::NON_LOCK;
    }

    return true;
}