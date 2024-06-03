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
lsn_t LogManager::add_log_to_buffer(LogRecord *log_record) {
    std::scoped_lock lock(latch_);
    int log_size = log_record->log_tot_len_;

    assert(log_size < LOG_BUFFER_SIZE);
    // 1. 如果log_buffer已经满了，不能再装了，刷到磁盘
    if (log_buffer_.is_full(log_size)) {
        flush_buffer_to_disk();
    }
    log_record->lsn_ = global_lsn_;
    global_lsn_++;
    set_lsn(global_lsn_);
    // 2. 将log序列化后拷贝到buffer的相应位置
    char log_data[log_size];
    log_record->serialize(log_data);
    memcpy(log_buffer_.buffer_ + log_buffer_.offset_, log_data, log_size);
    log_buffer_.offset_ += log_record->log_tot_len_;
    // 3. 返回该日志的日志记录号
    return log_record->lsn_;
}

/**
 * @description: 把日志缓冲区的内容刷到磁盘中，由于目前只设置了一个缓冲区，因此需要阻塞其他日志操作
 */
lsn_t LogManager::flush_log_to_disk(LogRecord *log_record) {
    // 将锁的逻辑移至 add_log_to_buffer
    lsn_t lsn = add_log_to_buffer(log_record);
    flush_buffer_to_disk();
    return lsn;
}

/**
 * @description: 把日志缓冲区的内容刷到磁盘中
 */
void LogManager::flush_buffer_to_disk() {
    latch_.lock();
    disk_manager_->write_log(log_buffer_.buffer_, log_buffer_.offset_);

    persist_lsn_ = global_lsn_ - 1;
    memset(log_buffer_.buffer_, 0, log_buffer_.offset_);
    log_buffer_.offset_ = 0;

    latch_.unlock();
}

void LogManager::set_lsn(int lsn) {
    global_lsn_ = lsn;
}

void LogManager::init() {
}
