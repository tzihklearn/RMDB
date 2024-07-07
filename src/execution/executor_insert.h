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

#include <cstdio>
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class InsertExecutor : public AbstractExecutor {
private:
    TabMeta tab_;
    std::vector<Value> values_;
    RmFileHandle *fh_;
    std::string tab_name_;
    Rid rid_;
    SmManager *sm_manager_;

public:
    InsertExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<Value> values, Context *context) {
        sm_manager_ = sm_manager;
        tab_ = sm_manager_->db_.get_table(tab_name);
        values_ = values;
        tab_name_ = tab_name;
        if (values.size() != tab_.cols.size()) {
            throw InvalidValueCountError();
        }
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        context_ = context;

        // 申请表级意向写锁（IX）
        if (context_ != nullptr) {
            context_->lock_mgr_->lock_IX_on_table(context_->txn_, fh_->GetFd());
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        // 构建记录
        RmRecord rec(fh_->get_file_hdr().record_size);
        for (size_t i = 0; i < values_.size(); i++) {
            auto &col = tab_.cols[i];
            auto &val = values_[i];
            if (col.type != val.type) {
                throw IncompatibleTypeError(coltype2str(col.type), coltype2str(val.type));
            }
            val.init_raw(col.len);
            memcpy(rec.data + col.offset, val.raw->data, col.len);
        }

        // 插入记录到文件并获取新的rid
        rid_ = fh_->insert_record(rec.data, context_);

        // 申请行级排他锁（X锁）
        context_->lock_mgr_->lock_exclusive_on_record(context_->txn_, rid_, fh_->GetFd());

        // 记录事务日志
        Transaction *txn = context_->txn_;
        auto *insert_log = new InsertLogRecord(txn->get_transaction_id(), rec, rid_, tab_name_);
        insert_log->prev_lsn_ = txn->get_prev_lsn();
        txn->set_prev_lsn(context_->log_mgr_->add_log_to_buffer(insert_log));

        auto *write_rcd = new TableWriteRecord(WType::INSERT_TUPLE, tab_name_, rid_);
        context_->txn_->append_table_write_record(write_rcd);

        // 插入索引条目
        for (auto &index: tab_.indexes) {
            auto ih = sm_manager_->ihs_.at(
                    sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

            std::vector<char> key(index.col_tot_len);
            int offset = 0;
            for (size_t i = 0; i < index.col_num; ++i) {
                memcpy(key.data() + offset, rec.data + index.cols[i].offset, index.cols[i].len);
                offset += index.cols[i].len;
            }

            try {
                ih->insert_entry(key.data(), rid_, context_->txn_);
                auto *index_rcd = new IndexWriteRecord(WType::INSERT_TUPLE, tab_name_, rid_, key.data(),
                                                       index.col_tot_len);
                context_->txn_->append_index_write_record(index_rcd);
            } catch (InternalError &error) {
                // 回滚操作
                fh_->delete_record(rid_, context_);
                context_->txn_->set_state(TransactionState::ABORTED);
                throw InternalError("Non-unique index!");
            }
        }

        return nullptr;
    }

    Rid &rid() override { return rid_; }
};