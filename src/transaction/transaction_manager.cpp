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

    // 1. 判断传入事务参数是否为空指针
    if (txn == nullptr) {
        txn = new Transaction(next_txn_id_++);
        txn->set_state(TransactionState::GROWING);
    }

    txn_map.emplace(txn->get_transaction_id(), txn);

    BeginLogRecord begin_log(txn->get_transaction_id());
    begin_log.prev_lsn_ = txn->get_prev_lsn();
    lsn_t lsn = log_manager->add_begin_log_record(txn->get_transaction_id());
    txn->set_prev_lsn(lsn);
    log_manager->flush_log_to_disk();
    // 5. 返回当前事务指针
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

    // 4. 把事务提交日志刷入磁盘中
    CommitLogRecord commit_log(txn->get_transaction_id());
    commit_log.prev_lsn_ = txn->get_prev_lsn();
    lsn_t lsn = log_manager->add_commit_log_record(txn->get_transaction_id());
    txn->set_prev_lsn(lsn);
    log_manager->flush_log_to_disk();

    //释放所有txn加的锁
    std::shared_ptr<std::unordered_set<LockDataId>> lock_set_ = txn->get_lock_set();
    for (auto iter: *lock_set_) {
        this->lock_manager_->unlock(txn, iter);
    }

    // 3. 释放事务相关资源
    txn->get_lock_set()->clear();
    txn->get_table_write_set()->clear();
    txn->get_index_write_set()->clear();

    // 5. 更新事务状态
    txn->set_state(TransactionState::COMMITTED);
}

/**
 * @description: 事务的终止（回滚）方法
 * @param {Transaction *} txn 需要回滚的事务
 * @param {LogManager} *log_manager 日志管理器指针
 */
void TransactionManager::abort(Transaction *txn, LogManager *log_manager) {
    std::unique_lock<std::recursive_mutex> lock(latch_);

    // 1. 回滚所有写操作
    auto table_write_set = txn->get_table_write_set();
    JoinStrategy js = JoinStrategy();
    Context context(lock_manager_, log_manager, txn, &js);
    while (!table_write_set->empty()) {
        auto write_rcd = table_write_set->back();
        auto &rm_file_hdl = sm_manager_->fhs_.at(write_rcd->GetTableName());
        switch (write_rcd->GetWriteType()) {
            case WType::INSERT_TUPLE: {
                rm_file_hdl->delete_record(write_rcd->GetRid(), &context, true);
//                rm_file_hdl->delete_record(write_rcd->GetRid(), nullptr);
                break;
            }
            case WType::DELETE_TUPLE: {
                rm_file_hdl->insert_record(write_rcd->GetRid(), write_rcd->GetRecord().data, &context, true);
//                rm_file_hdl->insert_record(write_rcd->GetRecord().data, &context, true);
//                rm_file_hdl->insert_record(write_rcd->GetRecord().data, nullptr);
                break;
            }
            case WType::UPDATE_TUPLE: {
                // 获取更新前的记录并恢复
                auto before_update_record = write_rcd->GetBeforeRecord();
                rm_file_hdl->update_record(write_rcd->GetRid(), before_update_record.data, &context, true);
//                rm_file_hdl->update_record(write_rcd->GetRid(), before_update_record.data, nullptr);
                break;
            }
        }
        table_write_set->pop_back();
        delete write_rcd; // 避免内存泄露
    }
    table_write_set->clear();

    // 2. 回滚所有索引写操作
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
        delete index_write_rcd; // 避免内存泄露
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
//                sm_manager_->drop_index(tab_name, col_names, nullptr);
                sm_manager_->drop_index(tab_name, col_names, &context);
                break;
            }
            case IType::DROP_INDEX : {
//                sm_manager_->create_index(tab_name, col_names, nullptr);
                sm_manager_->create_index(tab_name, col_names, &context);
                break;
            }
        }
        index_create_set->pop_back();
    }

    // 3. 事务abort之后释放所有锁
    std::shared_ptr<std::unordered_set<LockDataId>> lock_set_ = txn->get_lock_set();
    for (auto iter: *lock_set_) {
        this->lock_manager_->unlock(txn, iter);
    }

    txn->get_lock_set()->clear();
//    txn->clear_write_set();
    lock_set_->clear();

    // 4. 写入abort日志
    AbortLogRecord abort_log(txn->get_transaction_id());
    abort_log.prev_lsn_ = txn->get_prev_lsn();
    lsn_t lsn = log_manager->add_abort_log_record(txn->get_transaction_id());
    txn->set_prev_lsn(lsn);
    log_manager->flush_log_to_disk();

    // 5. 更新事务状态
    txn->set_state(TransactionState::ABORTED);
}

///*
//    回滚WriteRecord.
//*/
//void TransactionManager::rollback_writeRecord(WriteRecord* to_rol, Transaction * txn, LogManager *log_manager){
//    //根据类型不同调用对应的反函数
//    RmFileHandle* fh_  = sm_manager_ -> fhs_[to_rol->GetTableName()].get(); //能否使用get保持互斥性?
//    TabMeta& tab_meta_ = sm_manager_->db_.get_table(to_rol->GetTableName());
//
//    if(to_rol->GetWriteType() == WType::INSERT_TUPLE){
//        //修改日志
//        log_manager->add_delete_log_record(txn->get_transaction_id(), to_rol->GetRecord(), to_rol->GetRid(), to_rol->GetTableName());
//
//        //调用delete
//        fh_->delete_record(to_rol->GetRid(), nullptr);
//
//        // set_page_lsn(to_rol->GetRid().page_no, now_lsn);
//
//        for(auto iter = tab_meta_.indexes.begin(); iter!= tab_meta_.indexes.end(); iter++){
//            std::string ix_file_name = sm_manager_ -> get_ix_manager() -> get_index_name(to_rol->GetTableName(), iter->cols);
//            IxIndexHandle* ih_ = sm_manager_->ihs_.at(ix_file_name).get();
//#ifndef ENABLE_LOCK_CRABBING
//            std::lock_guard<std::mutex> lg (*ih_->getMutex());
//#endif
//            auto& index = *iter;
//            char* key = new char[index.col_tot_len];
//            int offset = 0;
//            for(size_t i = 0; i < index.col_num; ++i) {
//                memcpy(key + offset, to_rol->GetRecord().data + index.cols[i].offset, index.cols[i].len);
//                offset += index.cols[i].len;
//            }
//
////            ih_ ->delete_entry(key, to_rol->GetRid(), txn);
//            ih_ ->delete_entry(key, txn);
//
//            delete[] key;
//        }
//    }
//    else if(to_rol->GetWriteType() == WType::DELETE_TUPLE){
//        //修改日志
//        log_manager->add_insert_log_record(txn->get_transaction_id(), to_rol->GetRecord(), to_rol->GetRid(), to_rol->GetTableName());
//        //调用insert
//        fh_->insert_record(to_rol->GetRid(),to_rol->GetRecord().data);
//        // set_page_lsn(to_rol->GetRid().page_no, now_lsn);
//        for(auto iter = tab_meta_.indexes.begin(); iter!= tab_meta_.indexes.end(); iter++){
//            std::string ix_file_name = sm_manager_ -> get_ix_manager() -> get_index_name(to_rol->GetTableName(), iter->cols);
//            IxIndexHandle* ih_ = sm_manager_->ihs_.at(ix_file_name).get();
//#ifndef ENABLE_LOCK_CRABBING
//            std::lock_guard<std::mutex> lg (*ih_->getMutex());
//#endif
//            auto& index = *iter;
//            char* key = new char[index.col_tot_len];
//            int offset = 0;
//            for(size_t i = 0; i < index.col_num; ++i) {
//                memcpy(key + offset, to_rol->GetRecord().data + index.cols[i].offset, index.cols[i].len);
//                offset += index.cols[i].len;
//            }
//            try{
//                ih_ ->insert_entry(key, to_rol->GetRid(),txn);
//            }
//            catch(IndexInsertDuplicatedError& e){
//                delete[] key;
//                throw e;
//            }
//            delete[] key;
//        }
//    }
//    else if(to_rol->GetWriteType() == WType::UPDATE_TUPLE){
//        auto old_rec = fh_->get_record(to_rol->GetRid(), nullptr);
//        record_unpin_guard rug({fh_->GetFd(), to_rol->GetRid().page_no}, true, sm_manager_->get_bpm());
//        log_manager->add_update_log_record(txn->get_transaction_id(), to_rol->GetRecord(), *(old_rec.get()), to_rol->GetRid(), to_rol->GetTableName());
//        //修改日志
//        //调用update
//        fh_ -> update_record(to_rol->GetRid(), to_rol->GetRecord().data,nullptr);
//        // set_page_lsn(to_rol->GetRid().page_no, now_lsn);
//        for(auto iter = tab_meta_.indexes.begin(); iter!= tab_meta_.indexes.end(); iter++){
//            std::string ix_file_name = sm_manager_ -> get_ix_manager() -> get_index_name(to_rol->GetTableName(), iter->cols);
//            IxIndexHandle* ih_ = sm_manager_->ihs_.at(ix_file_name).get();
//#ifndef ENABLE_LOCK_CRABBING
//            std::lock_guard<std::mutex> lg (*ih_->getMutex());
//#endif
//            auto& index = *iter;
//            char* key = new char[index.col_tot_len];
//            char* old_key = new char[index.col_tot_len];
//            int offset = 0;
//            for(size_t i = 0; i < index.col_num; ++i) {
//                memcpy(key + offset, to_rol->GetRecord().data + index.cols[i].offset, index.cols[i].len);
//                memcpy(old_key + offset, old_rec->data + index.cols[i].offset, index.cols[i].len);
//                offset += index.cols[i].len;
//            }
////            ih_ ->delete_entry(old_key,to_rol->GetRid(), txn);
//            ih_ ->delete_entry(old_key, txn);
//            try{
//                ih_ ->insert_entry(key, to_rol->GetRid(), txn);
//            }
//            catch(IndexInsertDuplicatedError& e){
//                delete[] key;
//                delete[] old_key;
//                throw e;
//            }
//            delete[] key;
//            delete[] old_key;
//        }
//    }
//    else{
//        assert(0);
//    }
//}