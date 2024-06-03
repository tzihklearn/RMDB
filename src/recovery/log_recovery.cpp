/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "log_recovery.h"

/**
 * @description: analyze阶段，需要获得脏页表（DPT）和未完成的事务列表（ATT）
 */
int RecoveryManager::analyze() {
    char log_buffer[LOG_BUFFER_SIZE];
    int disk_offset = 0;
    int lsn = 0;

    while (true) {
        int bytes = disk_manager_->read_log(log_buffer, LOG_BUFFER_SIZE, disk_offset);

        if (bytes <= 0) {
            break; // No more logs to read
        }

        int buf_offset = 0;

        while (buf_offset + OFFSET_LOG_TOT_LEN + sizeof(int) < bytes) {
            int log_size = *reinterpret_cast<uint32_t *>(log_buffer + buf_offset + OFFSET_LOG_TOT_LEN);

            if (buf_offset + log_size > bytes) {
                break;
            }
            LogType log_type = *reinterpret_cast<LogType *>(log_buffer + buf_offset + OFFSET_LOG_TYPE);

            switch (log_type) {
                case LogType::BEGIN: {
                    auto *begin_log = new BeginLogRecord();
                    begin_log->deserialize(log_buffer + buf_offset);

                    // Update lsn_offset_map_ and lsn_len_map_
                    lsn_offset_map_[begin_log->lsn_] = disk_offset;
                    lsn_len_map_[begin_log->lsn_] = begin_log->log_tot_len_;

                    // Move to the next log
                    buf_offset += begin_log->log_tot_len_;
                    disk_offset += begin_log->log_tot_len_;

                    // Update active_txn_table by adding the new transaction
                    active_txn_table[begin_log->log_tid_] = begin_log->lsn_;

                    lsn = begin_log->lsn_;
                    break;
                }
                case LogType::COMMIT: {
                    auto *commit_log = new CommitLogRecord();
                    commit_log->deserialize(log_buffer + buf_offset);
                    // Update active_txn_table by removing the transaction
                    active_txn_table.erase(commit_log->log_tid_);

                    // Update lsn_offset_map_ and lsn_len_map_
                    lsn_offset_map_[commit_log->lsn_] = disk_offset;
                    lsn_len_map_[commit_log->lsn_] = commit_log->log_tot_len_;

                    // Move to the next log
                    buf_offset += commit_log->log_tot_len_;
                    disk_offset += commit_log->log_tot_len_;

                    lsn = commit_log->lsn_;
                    break;
                }
                case LogType::ABORT: {
                    auto *abort_log = new AbortLogRecord();
                    abort_log->deserialize(log_buffer + buf_offset);

                    // Update active_txn_table by removing the transaction
                    active_txn_table.erase(abort_log->log_tid_);

                    // Update lsn_offset_map_ and lsn_len_map_
                    lsn_offset_map_[abort_log->lsn_] = disk_offset;
                    lsn_len_map_[abort_log->lsn_] = abort_log->log_tot_len_;

                    // Move to the next log
                    buf_offset += abort_log->log_tot_len_;
                    disk_offset += abort_log->log_tot_len_;

                    lsn = abort_log->lsn_;
                    break;
                }
                case LogType::INSERT: {
                    auto *insert_log = new InsertLogRecord();
                    insert_log->deserialize(log_buffer + buf_offset);

                    // Update lsn_offset_map_ and lsn_len_map_
                    lsn_offset_map_[insert_log->lsn_] = disk_offset;
                    lsn_len_map_[insert_log->lsn_] = insert_log->log_tot_len_;

                    // Move to the next log
                    buf_offset += insert_log->log_tot_len_;
                    disk_offset += insert_log->log_tot_len_;

                    // Update active_txn_table with the current lsn for the given transaction ID
                    active_txn_table[insert_log->log_tid_] = insert_log->lsn_;

                    // Check if the page is already in dirty_page_table
                    dirty_page_table.emplace_back(insert_log->lsn_);

                    lsn = insert_log->lsn_;
                    break;
                }

                case LogType::DELETE: {
                    auto *delete_log = new DeleteLogRecord();
                    delete_log->deserialize(log_buffer + buf_offset);

                    // Get the page ID directly from the Rid's page_no
                    lsn_offset_map_[delete_log->lsn_] = disk_offset;
                    lsn_len_map_[delete_log->lsn_] = delete_log->log_tot_len_;

                    buf_offset += delete_log->log_tot_len_;
                    disk_offset += delete_log->log_tot_len_;

                    // Update active_txn_table with the current lsn for the given transaction ID
                    active_txn_table[delete_log->log_tid_] = delete_log->lsn_;

                    // Check if the page is already in dirty_page_table
                    dirty_page_table.emplace_back(delete_log->lsn_);

                    lsn = delete_log->lsn_;
                    break;
                }

                case LogType::UPDATE: {
                    auto *update_log = new UpdateLogRecord();
                    update_log->deserialize(log_buffer + buf_offset);

                    // Get the page ID directly from the Rid's page_no
                    lsn_offset_map_[update_log->lsn_] = disk_offset;
                    lsn_len_map_[update_log->lsn_] = update_log->log_tot_len_;

                    buf_offset += update_log->log_tot_len_;
                    disk_offset += update_log->log_tot_len_;

                    // Update active_txn_table with the current lsn for the given transaction ID
                    active_txn_table[update_log->log_tid_] = update_log->lsn_;

                    // Check if the page is already in dirty_page_table
                    dirty_page_table.emplace_back(update_log->lsn_);

                    lsn = update_log->lsn_;
                    break;
                }
                default:
                    // Handle other log types if any
                    break;
            }
        }
        if (bytes < LOG_BUFFER_SIZE) {
            break;
        }
    }
    return ++lsn;
}

/**
 * @description: 重做所有未落盘的操作
 */
void RecoveryManager::redo() {
    for (lsn_t lsn: dirty_page_table) {
        int log_len = lsn_len_map_[lsn];
        char *log_buf = new char[log_len];

        // Read the log entry from disk
        disk_manager_->read_log(log_buf, log_len, lsn_offset_map_[lsn]);
        LogType log_type = *(LogType *) log_buf;
        switch (log_type) {
            case LogType::INSERT: {
                auto *insert_log = new InsertLogRecord();

                insert_log->deserialize(log_buf);
                std::string tab_name(insert_log->table_name_, insert_log->table_name_size_);
                auto rm_file_hdl = sm_manager_->fhs_[tab_name].get();
                rm_file_hdl->insert_record(insert_log->insert_value_.data, nullptr);
                break;
            }
            case LogType::DELETE: {
                auto *delete_log = new DeleteLogRecord();

                delete_log->deserialize(log_buf);
                auto rm_file_hdl = sm_manager_->fhs_[delete_log->table_name_].get();
                auto page_hdl = rm_file_hdl->fetch_page_handle(delete_log->rid_.page_no);
                if (page_hdl.page->get_page_lsn() < delete_log->lsn_) {
                    rm_file_hdl->delete_record(delete_log->rid_, nullptr);
                }
                buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
                break;
            }
            case LogType::UPDATE: {
                auto *update_log = new UpdateLogRecord();

                update_log->deserialize(log_buf);
                std::string tab_name(update_log->table_name_, update_log->table_name_size_);
                auto rm_file_hdl = sm_manager_->fhs_[tab_name].get();
                auto page_hdl = rm_file_hdl->fetch_page_handle(update_log->rid_.page_no);
                if (page_hdl.page->get_page_lsn() < update_log->lsn_) {
                    rm_file_hdl->update_record(update_log->rid_, update_log->after_update_value_.data, nullptr);
                }
                buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
                break;
            }
            default:
                break;
        }
        delete[] log_buf; // Clean up
    }
}

/**
 * @description: 回滚未完成的事务
 */
void RecoveryManager::undo() {
    for (auto &it: active_txn_table) {
        lsn_t lsn = it.second;
        while (lsn != INVALID_LSN) {
            int log_len = lsn_len_map_[lsn];
            char *log_buf = new char[log_len];
            memset(log_buf, 0, log_len);

            // Read the log entry from disk
            disk_manager_->read_log(log_buf, log_len, lsn_offset_map_[lsn]);
            LogType log_type = *(LogType *) log_buf;
            switch (log_type) {
                case LogType::BEGIN: {
                    auto *begin_log_rcd = new BeginLogRecord();
                    begin_log_rcd->deserialize(log_buf);
                    lsn = begin_log_rcd->prev_lsn_;
                    break;
                }
                case LogType::INSERT: {
                    auto *insert_log_rcd = new InsertLogRecord();
                    insert_log_rcd->deserialize(log_buf);
                    auto rm_file_hdl = sm_manager_->fhs_[insert_log_rcd->table_name_].get();
                    rm_file_hdl->delete_record(insert_log_rcd->rid_, nullptr);
                    lsn = insert_log_rcd->prev_lsn_;
                    break;

                }
                case LogType::DELETE: {
                    auto *delete_log_rcd = new DeleteLogRecord();
                    delete_log_rcd->deserialize(log_buf);
                    auto rm_file_hdl = sm_manager_->fhs_[delete_log_rcd->table_name_].get();
                    rm_file_hdl->insert_record(delete_log_rcd->rid_, delete_log_rcd->delete_value_.data);
                    lsn = delete_log_rcd->prev_lsn_;
                    break;
                }
                case LogType::UPDATE: {
                    auto *update_log_rcd = new UpdateLogRecord();
                    update_log_rcd->deserialize(log_buf);
                    auto rm_file_hdl = sm_manager_->fhs_[update_log_rcd->table_name_].get();
                    rm_file_hdl->update_record(update_log_rcd->rid_, update_log_rcd->before_update_value_.data,
                                               nullptr);
                    lsn = update_log_rcd->prev_lsn_;
                    break;
                }
                default:
                    break;
            }
            delete[] log_buf; // Clean up
        }
    }
}