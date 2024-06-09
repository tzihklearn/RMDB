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

class UpdateExecutor : public AbstractExecutor {
private:
    TabMeta tab_;                           // 表中的元数据
    std::vector<Condition> conds_;          // 更新的条件
    RmFileHandle *fh_;                      // 表的数据文件句柄
    std::vector<Rid> rids_;                 // 可能需要更新的位置
    std::string tab_name_;                  // 表名
    std::vector<SetClause> set_clauses_;    // 需要更新的字段和值
    SmManager *sm_manager_;
    Context *context_;                      // 上下文

public:
    UpdateExecutor(SmManager *sm_manager, const std::string &tab_name, std::vector<SetClause> set_clauses,
                   std::vector<Condition> conds, std::vector<Rid> rids, Context *context) {
        sm_manager_ = sm_manager;
        tab_name_ = tab_name;
        set_clauses_ = set_clauses;
        tab_ = sm_manager_->db_.get_table(tab_name);
        fh_ = sm_manager_->fhs_.at(tab_name).get();
        conds_ = conds;
        rids_ = rids;
        context_ = context;

        // IX
        context_->lock_mgr_->lock_IX_on_table(context_->txn_, fh_->GetFd());
    }

    std::unique_ptr<RmRecord> Next() override {
        // X
        context_->lock_mgr_->lock_exclusive_on_record(context_->txn_, rid(), fh_->GetFd());

        for (auto &rid: rids_) {
            auto rec = fh_->get_record(rid, context_);

            // 删除旧索引
            std::vector<std::unique_ptr<IndexWriteRecord>> index_write_records;
            std::vector<std::vector<char>> old_keys;
            for (auto &index: tab_.indexes) {
                auto ih = sm_manager_->ihs_.at(
                        sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

                std::vector<char> old_key(index.col_tot_len);
                int offset = 0;
                for (size_t i = 0; i < index.col_num; ++i) {
                    memcpy(old_key.data() + offset, rec->data + index.cols[i].offset, index.cols[i].len);
                    offset += index.cols[i].len;
                }
                ih->delete_entry(old_key.data(), context_->txn_);
                old_keys.push_back(std::move(old_key));
            }

            // 更新记录
            std::vector<char> newRecord(fh_->get_file_hdr().record_size);
            memcpy(newRecord.data(), rec->data, fh_->get_file_hdr().record_size);
            for (auto &setClause: set_clauses_) {
                auto lhsCol = tab_.get_col(setClause.lhs.col_name);
                memcpy(newRecord.data() + lhsCol->offset, setClause.rhs.raw->data, lhsCol->len);
            }

            // 记录更新前后的数据，用于日志记录
            RmRecord beforeUpdateRecord(rec->size);
            memcpy(beforeUpdateRecord.data, rec->data, rec->size);
            RmRecord afterUpdateRecord(newRecord.size());
            memcpy(afterUpdateRecord.data, newRecord.data(), newRecord.size());

            // 更新记录到文件
            fh_->update_record(rid, newRecord.data(), context_);

            // 写入更新日志
            auto writeRecord = new TableWriteRecord(WType::UPDATE_TUPLE, tab_name_, rid, beforeUpdateRecord,
                                                    afterUpdateRecord);
            context_->txn_->append_table_write_record(writeRecord);

            Transaction *txn = context_->txn_;
            auto update_log = new UpdateLogRecord(txn->get_transaction_id(), beforeUpdateRecord, afterUpdateRecord,
                                                  fh_->get_file_hdr().record_size, newRecord.data(), rid, tab_name_);
            update_log->prev_lsn_ = txn->get_prev_lsn();
            txn->set_prev_lsn(context_->log_mgr_->add_log_to_buffer(update_log));

            // 尝试插入新索引条目
            try {
                for (size_t idx = 0; idx < tab_.indexes.size(); ++idx) {
                    auto &index = tab_.indexes[idx];
                    auto &old_key = old_keys[idx];
                    auto ih = sm_manager_->ihs_.at(
                            sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

                    std::vector<char> new_key(index.col_tot_len);
                    int offset = 0;
                    for (size_t i = 0; i < index.col_num; ++i) {
                        memcpy(new_key.data() + offset, newRecord.data() + index.cols[i].offset, index.cols[i].len);
                        offset += index.cols[i].len;
                    }
                    ih->insert_entry(new_key.data(), rid, context_->txn_);

                    auto index_rcd = std::make_unique<IndexWriteRecord>(WType::UPDATE_TUPLE, tab_name_, rid,
                                                                        old_key.data(), new_key.data(),
                                                                        index.col_tot_len);
                    context_->txn_->append_index_write_record(index_rcd.get());
                    index_write_records.push_back(std::move(index_rcd));
                }
            } catch (InternalError &error) {
                // 回滚更新记录
                fh_->update_record(rid, rec->data, context_);
                // 重新插入旧索引条目
                for (size_t idx = 0; idx < tab_.indexes.size(); ++idx) {
                    auto &index = tab_.indexes[idx];
                    auto &old_key = old_keys[idx];
                    auto ih = sm_manager_->ihs_.at(
                            sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

                    ih->insert_entry(old_key.data(), rid, context_->txn_);
                }
                throw InternalError("index Error");
            }
        }
        return nullptr; // 返回nullptr表示没有更多元组
    }

    Rid &rid() override { return _abstract_rid; }
};