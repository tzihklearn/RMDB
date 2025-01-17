/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class DeleteExecutor : public AbstractExecutor {
private:
    TabMeta tab_;
    std::vector<Condition> conds_;
    RmFileHandle *fh_;
    std::vector<Rid> rids_;
    std::string tab_name_;
    SmManager *sm_manager_;

public:
    DeleteExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<Condition> conds,
                   std::vector<Rid> rids, Context *context) {
        sm_manager_ = sm_manager;
        tab_name_ = tab_name;
        tab_ = sm_manager_->db_.get_table(tab_name);
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        conds_ = conds;
        rids_ = rids;
        context_ = context;

        // 申请表级意向写锁（IX）
        if (context_ != nullptr) {
            context_->lock_mgr_->lock_IX_on_table(context_->txn_, fh_->GetFd());
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        for (auto &rid: rids_) {
            // 申请行级排他锁（X锁）
            context_->lock_mgr_->lock_exclusive_on_record(context_->txn_, rid, fh_->GetFd());

            // 获取原记录
            auto record = fh_->get_record(rid, context_, false);

            // 删除索引条目
            for (const auto &index: tab_.indexes) {
                auto ih = sm_manager_->ihs_.at(
                        sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

                std::vector<char> key(index.col_tot_len);
                int offset = 0;
                for (size_t i = 0; i < index.col_num; ++i) {
                    memcpy(key.data() + offset, record->data + index.cols[i].offset, index.cols[i].len);
                    offset += index.cols[i].len;
                }

                ih->delete_entry(key.data(), context_->txn_);

                auto *index_rcd = new IndexWriteRecord(WType::DELETE_TUPLE, tab_name_, rid, key.data(),
                                                       index.col_tot_len);
                context_->txn_->append_index_write_record(index_rcd);
            }

            // 创建删除记录
            RmRecord delete_rcd(record->size);
            memcpy(delete_rcd.data, record->data, record->size);

            // 记录事务日志
//            Transaction *txn = context_->txn_;
//            auto *delete_log = new DeleteLogRecord(txn->get_transaction_id(), delete_rcd, rid, tab_name_);
//            delete_log->prev_lsn_ = txn->get_prev_lsn();
//            txn->set_prev_lsn(context_->log_mgr_->add_log_to_buffer(delete_log));

            // 删除记录
            fh_->delete_record(rid, context_);

//            auto *write_record = new TableWriteRecord(WType::DELETE_TUPLE, tab_name_, rid, delete_rcd);
//            context_->txn_->append_table_write_record(write_record);
        }
        return nullptr;
    }

    Rid &rid() override { return _abstract_rid; }
};