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

class SeqScanExecutor : public AbstractExecutor {
private:
    std::string tab_name_;                 // 表的名称
    std::vector<Condition> conditions_;    // scan的条件
    RmFileHandle *fh_;                     // 表的数据文件句柄
    std::vector<ColMeta> cols_;            // scan后生成的记录的字段
    size_t len_;                           // scan后生成的每条记录的长度
    std::vector<Condition> fedConditions;  // 同Condition，两个字段相同

    Rid rid_;
    std::unique_ptr<RecScan> scan_;        // table_iterator

    SmManager *sm_manager_;
    Context *context_;

public:
    SeqScanExecutor(SmManager *sm_manager, std::string tab_name, std::vector<Condition> conds, Context *context) {
        sm_manager_ = sm_manager;
        tab_name_ = std::move(tab_name);
        conditions_ = std::move(conds);
        TabMeta &tab = sm_manager_->db_.get_table(tab_name_);
        fh_ = sm_manager_->fhs_.at(tab_name_).get();
        cols_ = tab.cols;
        len_ = cols_.back().offset + cols_.back().len;

        context_ = context;

        fedConditions = conditions_;

        // IS
        if (context != nullptr) {
            context->lock_mgr_->lock_IS_on_table(context->txn_, fh_->GetFd());
        }
    }

    bool is_fed_cond(const std::vector<ColMeta> &rec_cols, const Condition &cond, const RmRecord *target) {
        // 获取左操作数的colMeta
        auto lhsCol = cond.lhs_col;
        auto lhsColMeta = *get_col(rec_cols, lhsCol);

        // 如果右操作数是Col类型，获取右操作数的ColMeta
        ColMeta rhsColMeta;
        if (!cond.is_rhs_val && !cond.is_rhs_in) {
            auto rhsCol = cond.rhs_col;
            rhsColMeta = *get_col(rec_cols, rhsCol);
        }

        // 比较lhs和rhs的值
        auto rmRecord = std::make_unique<RmRecord>(*target);
        auto lhsVal = fetch_value(rmRecord, lhsColMeta);
        Value rhsVal;
        if (cond.is_rhs_val) {
            rhsVal = cond.rhs_val;
        } else if (!cond.is_rhs_in){
            rhsVal = fetch_value(rmRecord, rhsColMeta);
        }

        if (cond.is_rhs_in) {
            for (const auto &rhs_val: cond.rhs_in_vals) {
                if (compare_value(lhsVal, rhs_val, OP_EQ)) {
                    return true;
                }
            }
            return false;
        } else {
            return compare_value(lhsVal, rhsVal, cond.op);
        }
    }

    // 判断记录是否满足所有条件
    bool record_satisfies_conditions(const std::vector<ColMeta> &rec_cols, const std::vector<Condition> &conditions,
                                     const RmRecord *record) {
        return std::all_of(conditions.begin(), conditions.end(), [&](const Condition &cond) {
            return is_fed_cond(rec_cols, cond, record);
        });
    }

    void beginTuple() override {
        // S
        context_->lock_mgr_->lock_shared_on_record(context_->txn_, rid_, fh_->GetFd());

        // 获取一个RmScan对象的指针,赋值给算子的变量scan_
        scan_ = std::make_unique<RmScan>(fh_);

        // 用seq_scan来对表中的所有非空闲字段进行遍历，逐个判断是否满足所有条件
        while (!scan_->is_end()) {
            // 通过RmFileHandle获取到seq_scan扫描到的record并封装为RmRecord对象
            auto record = fh_->get_record(scan_->rid(), context_);

            // 检查是否满足所有条件
            if (record_satisfies_conditions(cols_, fedConditions, record.get())) {
                rid_ = scan_->rid();
                break;
            }

            scan_->next();
        }
    }

    void nextTuple() override {
        // S
        context_->lock_mgr_->lock_shared_on_record(context_->txn_, rid_, fh_->GetFd());

        assert(!is_end());
        // 继续查询下一个满足conds的record
        for (scan_->next(); !scan_->is_end(); scan_->next()) {
            // 通过RmFileHandle获取到seq_scan扫描到的record并封装为RmRecord对象
            auto record = fh_->get_record(scan_->rid(), context_);

            // 检查是否满足所有条件
            if (record_satisfies_conditions(cols_, fedConditions, record.get())) {
                rid_ = scan_->rid();
                break;
            }
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        if (is_end()) {
            return nullptr;
        }
        return fh_->get_record(rid_, context_);
    }

    Rid &rid() override { return rid_; }

    bool is_end() const override { return scan_->is_end(); }

    const std::vector<ColMeta> &cols() const override { return cols_; }

    size_t tupleLen() const override { return len_; }
};