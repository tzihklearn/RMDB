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

void RecoveryManager::analyze() {
    int fsize = disk_manager_->get_file_size(LOG_FILE_NAME);
    this->buffer_ = new char[std::max(fsize, 1)];
    disk_manager_->read_log(this->buffer_, fsize, 0);
    this->parseLog();
}

void RecoveryManager::redo() {
    for (size_t i = 0; i < this->read_log_records.size(); ++i) {
        this->RedoLog(this->read_log_records[i], i);
    }
}

void RecoveryManager::undo() {
    for (auto iter = this->read_log_records.rbegin(); iter != this->read_log_records.rend(); ++iter) {
        if (this->undo_list.empty()) break;
        this->UndoLog(*iter, std::distance(iter, this->read_log_records.rend()) - 1);
    }

    for (auto log : this->read_log_records) {
        delete log;
    }

    for (auto &table : this->sm_manager_->db_.get_tables()) {
        std::vector<IndexMeta> tmp_indexes = table.second.indexes;
        for (auto &tmp_index : tmp_indexes) {
            sm_manager_->drop_index(tmp_index.tab_name, tmp_index.cols, nullptr);
            std::vector<std::string> col_names(tmp_index.cols.size());
            std::transform(tmp_index.cols.begin(), tmp_index.cols.end(), col_names.begin(),
                           [](const ColMeta &col) { return col.name; });
            sm_manager_->create_index(tmp_index.tab_name, col_names, nullptr);
        }
    }

    delete[] this->buffer_;
}

//判断落盘
bool RecoveryManager::is_record_stored(const std::string &file_name, int page_no, lsn_t now_lsn) {
    RmFileHandle *file_handle = this->sm_manager_->fhs_.at(file_name).get();
    if (page_no >= file_handle->get_file_hdr().num_pages) return false;
    RmPageHandle page_handle = file_handle->fetch_page_handle(page_no);
    unpin_page_guard upg({file_handle->GetFd(), page_no}, false, buffer_pool_manager_);
    return page_handle.page->get_page_lsn() >= now_lsn;
}

//Redo log
void RecoveryManager::RedoLog(LogRecord *log_record, lsn_t now_lsn) {
    switch (log_record->log_type_) {
        case LogType::begin: {
            auto *begin_rec = dynamic_cast<BeginLogRecord *>(log_record);
            this->undo_list.insert(begin_rec->log_tid_);
            break;
        }
        case LogType::commit: {
            break;
        }
        case LogType::ABORT: {
            auto *txn_rec = dynamic_cast<AbortLogRecord *>(log_record);
            this->undo_list.erase(txn_rec->log_tid_);
            break;
        }
        case LogType::DELETE: {
            auto *del_rec = dynamic_cast<DeleteLogRecord *>(log_record);
            std::string del_name = del_rec->get_tab_name();
            if (!is_record_stored(del_name, del_rec->rid_.page_no, now_lsn)) {
                this->sm_manager_->fhs_.at(del_name)->delete_record(del_rec->rid_, nullptr);
            }
            break;
        }
        case LogType::INSERT: {
            auto *ins_rec = dynamic_cast<InsertLogRecord *>(log_record);
            std::string ins_name = ins_rec->get_tab_name();
            if (!is_record_stored(ins_name, ins_rec->rid_.page_no, now_lsn)) {
                RmFileHandle *fh_ = this->sm_manager_->fhs_.at(ins_name).get();
                fh_->allocpage(ins_rec->rid_);
                fh_->insert_record(ins_rec->rid_, ins_rec->insert_value_.data);
            }
            break;
        }
        case LogType::UPDATE: {
            auto *up_rec = dynamic_cast<UpdateLogRecord *>(log_record);
            std::string up_name = up_rec->get_tab_name();
            if (!is_record_stored(up_name, up_rec->rid_.page_no, now_lsn)) {
                RmFileHandle *fh_ = this->sm_manager_->fhs_.at(up_name).get();
                fh_->allocpage(up_rec->rid_);
                fh_->update_record(up_rec->rid_, up_rec->update_value_.data, nullptr);
            }
            break;
        }
        default:
            assert(false);
    }
}

void RecoveryManager::UndoLog(LogRecord *log_record, lsn_t now_lsn) {
    switch (log_record->log_type_) {
        case LogType::begin: {
            auto *begin_rec = dynamic_cast<BeginLogRecord *>(log_record);
            if (this->undo_list.count(begin_rec->log_tid_)) {
                log_manager_->add_abort_log_record(begin_rec->log_tid_);
                this->undo_list.erase(begin_rec->log_tid_);
            }
            break;
        }
        case LogType::commit:
            break;
        case LogType::ABORT:
            assert(this->undo_list.count(log_record->log_tid_) == 0);
            break;
        case LogType::DELETE: {
            auto *del_rec = dynamic_cast<DeleteLogRecord *>(log_record);
            std::string del_name = del_rec->get_tab_name();
            if (this->undo_list.count(del_rec->log_tid_)) {
                log_manager_->add_insert_log_record(del_rec->log_tid_, del_rec->delete_value_, del_rec->rid_, del_name);
                this->sm_manager_->fhs_.at(del_name)->insert_record(del_rec->rid_, del_rec->delete_value_.data);
            }
            break;
        }
        case LogType::INSERT: {
            auto *ins_rec = dynamic_cast<InsertLogRecord *>(log_record);
            std::string ins_name = ins_rec->get_tab_name();
            if (this->undo_list.count(ins_rec->log_tid_)) {
                log_manager_->add_delete_log_record(ins_rec->log_tid_, ins_rec->insert_value_, ins_rec->rid_, ins_name);
                this->sm_manager_->fhs_.at(ins_name)->delete_record(ins_rec->rid_, nullptr);
            }
            break;
        }
        case LogType::UPDATE: {
            auto *up_rec = dynamic_cast<UpdateLogRecord *>(log_record);
            std::string up_name = up_rec->get_tab_name();
            if (this->undo_list.count(up_rec->log_tid_)) {
                log_manager_->add_update_log_record(up_rec->log_tid_, up_rec->update_value_, up_rec->old_value_,
                                                    up_rec->rid_, up_name);
                this->sm_manager_->fhs_.at(up_name)->update_record(up_rec->rid_, up_rec->old_value_.data, nullptr);
            }
            break;
        }
        default:
            assert(false);
    }
}

void RecoveryManager::parseLog() {
    int offset = 0;
//    int file_size = disk_manager_->get_file_size(LOG_FILE_NAME);
//    auto log_ = new LogRecord();
//    while (offset < file_size) {
////        LogRecord *new_log = LogRecord::deserialize(this->buffer_ + offset);
//        LogRecord new_log = log_->deserialize(this->buffer_ + offset);
//        this->read_log_records.emplace_back(&new_log);
//        offset += new_log.log_tot_len_;
//    }
//开始解析
    auto *new_log = new LogRecord();
    while (disk_manager_->get_file_size(LOG_FILE_NAME) != 0) {
        new_log->deserialize(this->buffer_ + offset);
        if (new_log->log_tot_len_ <= 0) {
            exit(1);
        }
        switch (new_log->log_type_) {
            case LogType::begin: {
                auto *begin_log_rec = new BeginLogRecord();
                begin_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back((LogRecord *) begin_log_rec);
                offset += begin_log_rec->log_tot_len_;
                break;
            }
            case LogType::commit: {
                auto *commit_log_rec = new CommitLogRecord();
                commit_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back((LogRecord *) commit_log_rec);
                offset += commit_log_rec->log_tot_len_;
                break;
            }
            case LogType::ABORT: {
                auto *abort_log_rec = new AbortLogRecord();
                abort_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back((LogRecord *) abort_log_rec);
                offset += abort_log_rec->log_tot_len_;
                break;
            }
            case LogType::DELETE: {
                auto *del_log_rec = new DeleteLogRecord();
                del_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back((LogRecord *) del_log_rec);
                offset += del_log_rec->log_tot_len_;
                break;
            }
            case LogType::INSERT: {
                auto *ins_log_rec = new InsertLogRecord();
                ins_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back((LogRecord *) ins_log_rec);
                offset += ins_log_rec->log_tot_len_;
                break;
            }
            case LogType::UPDATE: {
                auto *up_log_rec = new UpdateLogRecord();
                up_log_rec->deserialize(this->buffer_ + offset);
                this->read_log_records.emplace_back(up_log_rec);
                offset += up_log_rec->log_tot_len_;
                break;
            }
            case LogType::IX_INSERT:
                break;
            case LogType::IX_DELETE:
                break;
            default:
                assert(0);
        }

        if (offset >= disk_manager_->get_file_size(LOG_FILE_NAME)) {
            break;
        }
    }
    delete new_log;
    this->log_manager_->set_global_lsn_(this->read_log_records.size());
    this->tmp_lsn_cnt = this->log_manager_->get_global_lsn_();
    this->log_manager_->set_persist_lsn_(this->read_log_records.size() - 1);
}