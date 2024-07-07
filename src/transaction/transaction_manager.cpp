/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "transaction_manager.h"
#include "record/rm_file_handle.h"

std::unordered_map<txn_id_t, Transaction *> TransactionManager::txn_map = {};

/**
 * @description: 事务的开始方法
 * @return {Transaction*} 开始事务的指针
 * @param {Transaction*} txn 事务指针，空指针代表需要创建新事务，否则开始已有事务
 * @param {LogManager*} log_manager 日志管理器指针
 */
Transaction *TransactionManager::begin(Transaction *txn, LogManager *log_manager) {
    std::unique_lock<std::recursive_mutex> lock(latch_);

    if (txn == nullptr) {
        txn = new Transaction(next_txn_id_++);
        txn->set_state(TransactionState::GROWING);
    }

    txn_map.emplace(txn->get_transaction_id(), txn);

    BeginLogRecord begin_log(txn->get_transaction_id());
    begin_log.prev_lsn_ = txn->get_prev_lsn();
    txn->set_prev_lsn(log_manager->flush_log_to_disk(&begin_log));

    return txn;
}

/**
 * @description: 事务的提交方法
 * @param {Transaction*} txn 需要提交的事务
 * @param {LogManager*} log_manager 日志管理器指针
 */
void TransactionManager::commit(Transaction *txn, LogManager *log_manager) {
    std::unique_lock<std::recursive_mutex> lock(latch_);

    txn->set_state(TransactionState::SHRINKING);

    // 写入提交日志
    CommitLogRecord commit_log(txn->get_transaction_id());
    commit_log.prev_lsn_ = txn->get_prev_lsn();
    txn->set_prev_lsn(log_manager->flush_log_to_disk(&commit_log));

    // 释放所有锁
    std::shared_ptr<std::unordered_set<LockDataId>> lock_set_ = txn->get_lock_set();
    for (auto iter: *lock_set_) {
        this->lock_manager_->unlock(txn, iter);
    }

    txn->get_lock_set()->clear();
    txn->get_table_write_set()->clear();
    txn->get_index_write_set()->clear();

    txn->set_state(TransactionState::COMMITTED);
}

/**
 * @description: 事务的终止（回滚）方法
 * @param {Transaction *} txn 需要回滚的事务
 * @param {LogManager} *log_manager 日志管理器指针
 */
void TransactionManager::abort(Transaction *txn, LogManager *log_manager) {
    std::unique_lock<std::recursive_mutex> lock(latch_);

    auto table_write_set = txn->get_table_write_set();
    JoinStrategy js = JoinStrategy();
    Context context(lock_manager_, log_manager, txn, &js);
    while (!table_write_set->empty()) {
        auto write_rcd = table_write_set->back();
        auto &rm_file_hdl = sm_manager_->fhs_.at(write_rcd->GetTableName());
        switch (write_rcd->GetWriteType()) {
            case WType::INSERT_TUPLE: {
                rm_file_hdl->delete_record(write_rcd->GetRid(), &context);
                break;
            }
            case WType::DELETE_TUPLE: {
                rm_file_hdl->insert_record(write_rcd->GetRecord().data, &context);
                break;
            }
            case WType::UPDATE_TUPLE: {
                auto before_update_record = write_rcd->GetBeforeRecord();
                rm_file_hdl->update_record(write_rcd->GetRid(), before_update_record.data, &context);
                break;
            }
        }
        table_write_set->pop_back();
        delete write_rcd;
    }
    table_write_set->clear();

    auto index_write_set = txn->get_index_write_set();
    while (!index_write_set->empty()) {
        auto index_write_rcd = index_write_set->back();
        std::string table_name = index_write_rcd->GetTableName();
        auto tabs = sm_manager_->db_.get_table(table_name);
        auto indexes = sm_manager_->db_.get_table(table_name).indexes;
        for (auto &index: indexes) {
            auto ih = sm_manager_->ihs_.at(
                    sm_manager_->get_ix_manager()->get_index_name(table_name, index.cols)).get();
            switch (index_write_rcd->GetWriteType()) {
                case WType::INSERT_TUPLE: {
                    ih->delete_entry(index_write_rcd->GetKey(), txn);
                    break;
                }
                case WType::DELETE_TUPLE: {
                    ih->insert_entry(index_write_rcd->GetKey(), index_write_rcd->GetRid(), txn);
                    break;
                }
                case WType::UPDATE_TUPLE: {
                    ih->delete_entry(index_write_rcd->GetNewKey(), txn);
                    ih->insert_entry(index_write_rcd->GetOldKey(), index_write_rcd->GetRid(), txn);
                    break;
                }
            }
        }
        index_write_set->pop_back();
        delete index_write_rcd;
    }
    index_write_set->clear();

    auto index_create_set = txn->get_index_create_page_set();
    while (!index_create_set->empty()) {
        auto index_create_rcd = index_create_set->back();
        std::string index_name = index_create_rcd->GetIndexName();

        auto tab_name = index_create_rcd->GetTabName();
        auto col_names = index_create_rcd->GetCalNames();

        switch (index_create_rcd->GetCreateType()) {
            case IType::INSERT_INDEX: {
                sm_manager_->drop_index(tab_name, col_names, nullptr);
                break;
            }
            case IType::DROP_INDEX : {
                sm_manager_->create_index(tab_name, col_names, nullptr);
                break;
            }
        }
        index_create_set->pop_back();
    }

    std::shared_ptr<std::unordered_set<LockDataId>> lock_set_ = txn->get_lock_set();
    for (auto iter: *lock_set_) {
        this->lock_manager_->unlock(txn, iter);
    }

    txn->get_lock_set()->clear();

    AbortLogRecord abort_log(txn->get_transaction_id());
    abort_log.prev_lsn_ = txn->get_prev_lsn();
    txn->set_prev_lsn(log_manager->flush_log_to_disk(&abort_log));

    txn->set_state(TransactionState::ABORTED);
}