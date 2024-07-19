/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstring>
#include "log_manager.h"

/**
 * @description: 添加日志记录到日志缓冲区中，并返回日志记录号
 * @param {LogRecord*} log_record 要写入缓冲区的日志记录
 * @return {lsn_t} 返回该日志的日志记录号
 */
template <typename T>
lsn_t LogManager::add_log_to_buffer(T *log_record) {
    assert(log_record->log_tot_len_ != 0);
    if (log_buffer_.is_full(log_record->log_tot_len_)) {
        this->flush_log_to_disk(FlushReason::BUFFER_FULL);
    }

    log_record->serialize(log_buffer_.buffer_ + log_buffer_.offset_);
    log_buffer_.offset_ += log_record->log_tot_len_;

    return log_record->lsn_;
}

/**
 * @description: 把日志缓冲区的内容刷到磁盘中，由于目前只设置了一个缓冲区，因此需要阻塞其他日志操作
 */
void LogManager::flush_log_to_disk() {
    flush_log_to_disk(FlushReason::DEFAULT);
}

void LogManager::flush_log_to_disk(FlushReason flush_reason) {
    if (flush_reason != FlushReason::BUFFER_FULL) {
        this->latch_.lock();
    }
    //写回到磁盘
    disk_manager_->write_log(this->get_log_buffer()->buffer_, (int) this->get_log_buffer()->offset_);

    // 维护元数据
    this->persist_lsn_ = this->global_lsn_ - 1;
    this->get_log_buffer()->resetBuffer();

    if (flush_reason != FlushReason::BUFFER_FULL) {
        this->latch_.unlock();
    }
}

/**
 * @description: 用于分发lsn
 */
lsn_t LogManager::get_new_lsn() {
    return global_lsn_++;
}

/**
 * @description: 添加日志记录的辅助函数
 */
template <typename T>
lsn_t LogManager::add_log_record(T* log_record) {
    std::lock_guard<std::mutex> lg(this->latch_);
    log_record->lsn_ = get_new_lsn();
    lsn_t lsn = add_log_to_buffer(log_record);
    delete log_record;
    return lsn;
}

/**
 * @description: 添加一条insert_log_record
 */
lsn_t LogManager::add_insert_log_record(txn_id_t txn_id, RmRecord &insert_value, Rid &rid, const std::string &table_name) {
    return add_log_record(new InsertLogRecord(txn_id, insert_value, rid, table_name));
}

/**
 * @description: 添加一条delete_log_record
 */
lsn_t LogManager::add_delete_log_record(txn_id_t txn_id, RmRecord &delete_value, Rid &rid, const std::string &table_name) {
    return add_log_record(new DeleteLogRecord(txn_id, delete_value, rid, table_name));
}

/**
 * @description: 添加一条update_log_record
 */
lsn_t LogManager::add_update_log_record(txn_id_t txn_id, RmRecord &update_value, RmRecord &old_value, Rid &rid, const std::string &table_name) {
    return add_log_record(new UpdateLogRecord(txn_id, update_value, old_value, rid, table_name));
}

/**
 * @description: 添加一条begin_log_record
 */
lsn_t LogManager::add_begin_log_record(txn_id_t txn_id) {
    return add_log_record(new BeginLogRecord(txn_id));
}

/**
 * @description: 添加一条commit_log_record
 */
lsn_t LogManager::add_commit_log_record(txn_id_t txn_id) {
    return add_log_record(new CommitLogRecord(txn_id));
}

/**
 * @description: 添加一条abort_log_record
 */
lsn_t LogManager::add_abort_log_record(txn_id_t txn_id) {
    return add_log_record(new AbortLogRecord(txn_id));
}
// 静态检查点
void LogManager::static_checkpoint() {
    std::fstream static_checkpoint_outfile;
    static_checkpoint_outfile.open("static_checkpoint_my.txt", std::ios::out | std::ios::app);
    static_checkpoint_outfile << "static checkpoint" << std::endl;
    static_checkpoint_outfile.close();
}
