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
    RmFileHandle *fh_;                      // 表的数据文件句柄(句柄是一个指向对象的指针)
    std::vector<Rid> rids_;                 // 可能需要更新的位置
    std::string tab_name_;                  // 表名
    std::vector<SetClause> set_clauses_;    // 需要更新的字段和值
    SmManager *sm_manager_;

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

        // 加IX锁
        if (context_ != nullptr) {
            context_->lock_mgr_->lock_IX_on_table(context_->txn_, fh_->GetFd());
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        for (auto &rid: rids_) {
            auto rec = fh_->get_record(rid, context_);
            // 删除旧索引
            for (auto &index: tab_.indexes) {
                auto ih = sm_manager_->ihs_.at(
                        sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();
                char key[index.col_tot_len];
                int offset = 0;
                for (size_t i = 0; i < index.col_num; ++i) {
                    memcpy(key + offset, rec->data + index.cols[i].offset, index.cols[i].len);
                    offset += index.cols[i].len;
                }
                ih->delete_entry(key, context_->txn_);

                auto *index_rcd = new IndexWriteRecord(WType::DELETE_TUPLE, tab_name_, rid, key, index.col_tot_len);
                context_->txn_->append_index_write_record(index_rcd);
            }
            // 更新记录
            std::vector<char> newRecord(fh_->get_file_hdr().record_size);
            memcpy(newRecord.data(), rec->data, fh_->get_file_hdr().record_size);
            for (auto &setClauses: set_clauses_) {
                auto lhsCol = tab_.get_col(setClauses.lhs.col_name);
                memcpy(newRecord.data() + lhsCol->offset, setClauses.rhs.raw->data, lhsCol->len);
            }

            // 事务开始
            RmRecord beforeUpdateRecord(rec->size);
            RmRecord afterUpdateRecord(rec->size);
            memcpy(beforeUpdateRecord.data, rec->data, rec->size);

            fh_->update_record(rid, newRecord.data(), context_);

            memcpy(afterUpdateRecord.data, newRecord.data(), rec->size);
            auto *writeRecord = new TableWriteRecord(WType::UPDATE_TUPLE, tab_name_, rid, afterUpdateRecord);
            context_->txn_->append_table_write_record(writeRecord);

            Transaction *txn = context_->txn_;
            auto *update_log_ = new UpdateLogRecord(txn->get_transaction_id(), beforeUpdateRecord,
                                                    afterUpdateRecord, fh_->get_file_hdr().record_size,
                                                    newRecord.data(), rid, tab_name_);
            update_log_->prev_lsn_ = txn->get_prev_lsn();
            txn->set_prev_lsn(context_->log_mgr_->add_log_to_buffer(update_log_));
            // 事务结束

            // 删除旧索引条目并插入新索引条目
            try {
                for (size_t i = 0; i < tab_.indexes.size(); ++i) {
                    auto &index = tab_.indexes[i];
                    auto ih = sm_manager_->ihs_.at(
                            sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();

                    // 删除旧索引条目
                    ih->delete_entry(oldKeys[i].data(), context_->txn_);
                    auto *delete_index_rcd = new IndexWriteRecord(WType::DELETE_TUPLE, tab_name_, rid,
                                                                  oldKeys[i].data(), index.col_tot_len);
                    context_->txn_->append_index_write_record(delete_index_rcd);

                    // 构建新索引键
                    std::vector<char> newKey(index.col_tot_len);
                    int offset = 0;
                    for (size_t j = 0; j < index.col_num; ++j) {
                        memcpy(newKey.data() + offset, newRecord.data() + index.cols[j].offset, index.cols[j].len);
                        offset += index.cols[j].len;
                    }

                    // 插入新索引条目
                    ih->insert_entry(newKey.data(), rid, context_->txn_);
                    auto *insert_index_rcd = new IndexWriteRecord(WType::INSERT_TUPLE, tab_name_, rid, newKey.data(),
                                                                  index.col_tot_len);
                    context_->txn_->append_index_write_record(insert_index_rcd);
                }
            } catch (InternalError &error) {
                // 回滚更新记录
                fh_->update_record(rid, rec->data, context_);
                // 重新插入旧索引条目
                for (size_t i = 0; i < tab_.indexes.size(); ++i) {
                    auto &index = tab_.indexes[i];
                    auto ih = sm_manager_->ihs_.at(
                            sm_manager_->get_ix_manager()->get_index_name(tab_name_, index.cols)).get();
                    ih->insert_entry(oldKeys[i].data(), rid, context_->txn_);
                }
                throw InternalError("Index Update Error");
            }
        }
        return nullptr; // 返回 nullptr 表示没有更多元组
    }

    Rid &rid() override { return _abstract_rid; }
};