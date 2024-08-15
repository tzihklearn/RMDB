#include "lock_manager.h"

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

    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id() &&
            (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
             r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id() &&
                (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
                 r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    for (const auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    txn->get_lock_set()->emplace(lock_data_id);
    request.granted_ = true;
    request_queue.group_lock_mode_ = GroupLockMode::S;
    request_queue.request_queue_.emplace_back(request);
    request_queue.cv_.notify_all();
    return true;
}

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

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::SHARED) {
                bool can_upgrade = true;
                for (const auto &r: request_queue.request_queue_) {
                    if (r.txn_id_ != txn->get_transaction_id()) {
                        can_upgrade = false;
                        break;
                    }
                }

                if (can_upgrade) {
                    req.lock_mode_ = LockMode::EXCLUSIVE;
                    request_queue.group_lock_mode_ = GroupLockMode::X;
                    return true;
                } else {
                    wait_die(txn, request_queue, ul);
                    return false;
                }
            } else if (req.lock_mode_ == LockMode::EXCLUSIVE) {
                return true;
            }
        }
    }

    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id()) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id()) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    txn->get_lock_set()->emplace(lock_data_id);
    request.granted_ = true;
    request_queue.group_lock_mode_ = GroupLockMode::X;
    request_queue.request_queue_.emplace_back(request);
    request_queue.cv_.notify_all();
    return true;
}

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

    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id() &&
            (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
             r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id() &&
                (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
                 r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    txn->get_lock_set()->emplace(lock_data_id);
    request.granted_ = true;
    request_queue.group_lock_mode_ = GroupLockMode::S;
    request_queue.request_queue_.emplace_back(request);
    request_queue.cv_.notify_all();
    return true;
}

bool LockManager::lock_exclusive_on_table(Transaction *txn, int tab_fd) {
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

    LockRequest request(txn->get_transaction_id(), LockMode::EXCLUSIVE);
    auto &request_queue = lock_table_it->second;

    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::SHARED) {
                bool can_upgrade = true;
                for (const auto &r: request_queue.request_queue_) {
                    if (r.txn_id_ != txn->get_transaction_id()) {
                        can_upgrade = false;
                        break;
                    }
                }

                if (can_upgrade) {
                    req.lock_mode_ = LockMode::EXCLUSIVE;
                    request_queue.group_lock_mode_ = GroupLockMode::X;
                    return true;
                } else {
                    wait_die(txn, request_queue, ul);
                    return false;
                }
            } else if (req.lock_mode_ == LockMode::EXCLUSIVE) {
                return true;
            }
        }
    }

    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id()) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id()) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    txn->get_lock_set()->emplace(lock_data_id);
    request.granted_ = true;
    request_queue.group_lock_mode_ = GroupLockMode::X;
    request_queue.request_queue_.emplace_back(request);
    request_queue.cv_.notify_all();
    return true;
}

bool LockManager::lock_IS_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    // 检查事务是否有效
    if (!lock_check(txn)) {
        return false;
    }

    auto lock_data_id = LockDataId(tab_fd, LockDataType::TABLE);
    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        lock_table_it = lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id),
                                            std::forward_as_tuple()).first;
    }

    LockRequest request(txn->get_transaction_id(), LockMode::INTENTION_SHARED);
    auto &request_queue = lock_table_it->second;

    // 检查是否有冲突的锁请求
    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id() &&
            (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
             r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        // 再次检查冲突，因为wait_die可能改变队列状态
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id() &&
                (r.lock_mode_ == LockMode::EXCLUSIVE || r.lock_mode_ == LockMode::S_IX ||
                 r.lock_mode_ == LockMode::INTENTION_EXCLUSIVE)) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    // 检查当前事务是否已经持有锁
    for (const auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            return true;
        }
    }

    // 如果没有冲突，直接授予意向读锁
    if (request_queue.group_lock_mode_ != GroupLockMode::X && request_queue.group_lock_mode_ != GroupLockMode::SIX) {
        txn->get_lock_set()->emplace(lock_data_id);  // 将锁添加到事务的锁集
        request.granted_ = true;  // 标记锁请求已授予
        if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK) {
            request_queue.group_lock_mode_ = GroupLockMode::IS;  // 更新组锁模式
        }
        request_queue.request_queue_.emplace_back(request);  // 添加锁请求到队列中
        request_queue.cv_.notify_all();  // 唤醒所有等待锁的事务
        return true;
    } else {
        request_queue.cv_.notify_all();  // 唤醒所有等待锁的事务
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
}

bool LockManager::lock_IX_on_table(Transaction *txn, int tab_fd) {
    std::unique_lock<std::mutex> ul{latch_};

    // 检查事务是否有效
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

    // 检查是否有冲突的锁请求
    bool has_conflict = false;
    for (const auto &r: request_queue.request_queue_) {
        if (r.txn_id_ != txn->get_transaction_id() &&
            (r.lock_mode_ == LockMode::SHARED || r.lock_mode_ == LockMode::EXCLUSIVE ||
             r.lock_mode_ == LockMode::S_IX || r.lock_mode_ == LockMode::INTENTION_SHARED)) {
            has_conflict = true;
            break;
        }
    }

    if (has_conflict) {
        wait_die(txn, request_queue, ul);
        // 再次检查冲突，因为wait_die可能改变队列状态
        has_conflict = false;
        for (const auto &r: request_queue.request_queue_) {
            if (r.txn_id_ != txn->get_transaction_id() &&
                (r.lock_mode_ == LockMode::SHARED || r.lock_mode_ == LockMode::EXCLUSIVE ||
                 r.lock_mode_ == LockMode::S_IX || r.lock_mode_ == LockMode::INTENTION_SHARED)) {
                has_conflict = true;
                break;
            }
        }
        if (has_conflict) {
            return false;
        }
    }

    // 检查当前事务是否已经有其他类型的锁，并进行锁模式升级
    for (auto &req: request_queue.request_queue_) {
        if (req.txn_id_ == txn->get_transaction_id()) {
            if (req.lock_mode_ == LockMode::INTENTION_EXCLUSIVE || req.lock_mode_ == LockMode::S_IX ||
                req.lock_mode_ == LockMode::EXCLUSIVE) {
                return true;
            } else if (req.lock_mode_ == LockMode::SHARED) {
                req.lock_mode_ = LockMode::S_IX;
                request_queue.group_lock_mode_ = GroupLockMode::SIX;
                request_queue.cv_.notify_all();
                return true;
            } else if (req.lock_mode_ == LockMode::INTENTION_SHARED) {
                req.lock_mode_ = LockMode::INTENTION_EXCLUSIVE;
                request_queue.group_lock_mode_ = GroupLockMode::IX;
                request_queue.cv_.notify_all();
                return true;
            }
        }
    }

    // 检查组锁模式是否允许授予意向排他锁
    if (request_queue.group_lock_mode_ == GroupLockMode::NON_LOCK ||
        request_queue.group_lock_mode_ == GroupLockMode::IS ||
        request_queue.group_lock_mode_ == GroupLockMode::S ||
        request_queue.group_lock_mode_ == GroupLockMode::IX) {
        txn->get_lock_set()->emplace(lock_data_id);
        request.granted_ = true;
        request_queue.group_lock_mode_ = GroupLockMode::IX;
        request_queue.request_queue_.emplace_back(request);
        request_queue.cv_.notify_all();
        return true;
    } else {
        request_queue.cv_.notify_all();
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
}

bool LockManager::unlock(Transaction *txn, LockDataId lock_data_id) {
    std::unique_lock<std::mutex> ul{latch_};

    if (!unlock_check(txn)) {
        return false;
    }

    auto lock_table_it = lock_table_.find(lock_data_id);
    if (lock_table_it == lock_table_.end()) {
        return true;
    }

    auto &request_queue = lock_table_it->second;

    for (auto req = request_queue.request_queue_.begin(); req != request_queue.request_queue_.end(); ++req) {
        if (req->txn_id_ == txn->get_transaction_id()) {
            request_queue.request_queue_.erase(req);
            break;
        }
    }

    request_queue.cv_.notify_all();

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

// 等待-死锁策略实现
void LockManager::wait_die(Transaction *txn, LockRequestQueue &request_queue, std::unique_lock<std::mutex> &ul) {
    exit(1);
    while (true) {
        bool should_wait = false;
        for (const auto &existing_req: request_queue.request_queue_) {
            if (existing_req.txn_id_ == txn->get_transaction_id()) {
                continue;
            }
            if (existing_req.txn_id_ > txn->get_transaction_id()) { // 当前事务是老事务，应等待
                should_wait = true;
                break;
            } else { // 新事务，当前事务应中止
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
        if (req.txn_id_ != txn->get_transaction_id() && req.txn_id_ > txn->get_transaction_id()) { // 老事务仍然存在
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
            return true;
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
            return true;
        case TransactionState::SHRINKING:
            return true;
        default:
            throw RMDBError("Invalid transaction state in " + std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
}