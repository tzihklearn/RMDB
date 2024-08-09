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
    std::string tab_name_;
    std::vector<Condition> conditions_;
    RmFileHandle *fh_;
    std::vector<ColMeta> cols_;
    size_t len_;
    std::vector<Condition> fedConditions;

    Rid rid_;
    std::unique_ptr<RecScan> scan_;

    SmManager *sm_manager_;

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
        exit(1);
        // 申请表级共享锁（S）
        if(context_!= nullptr){
            context_->lock_mgr_->lock_shared_on_table(context_->txn_, fh_->GetFd());
        }
    }

    bool is_fed_cond(const std::vector<ColMeta> &rec_cols, const Condition &cond, const RmRecord *target) {
        auto lhsColMeta = *get_col(rec_cols, cond.lhs_col);

        ColMeta rhsColMeta;
        if (!cond.is_rhs_val && !cond.is_rhs_in) {
            rhsColMeta = *get_col(rec_cols, cond.rhs_col);
        }

        auto rmRecord = std::make_unique<RmRecord>(*target);
        auto lhsVal = fetch_value(rmRecord, lhsColMeta);
        Value rhsVal;
        if (cond.is_rhs_val) {
            rhsVal = cond.rhs_val;
        } else if (!cond.is_rhs_in) {
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

    bool record_satisfies_conditions(const std::vector<ColMeta> &rec_cols, const std::vector<Condition> &conditions,
                                     const RmRecord *record) {
        return std::all_of(conditions.begin(), conditions.end(), [&](const Condition &cond) {
            return is_fed_cond(rec_cols, cond, record);
        });
    }

    void beginTuple() override {
        scan_ = std::make_unique<RmScan>(fh_);

        // 扫描表中所有记录，找到第一个满足条件的记录
        while (!scan_->is_end()) {
            // 申请行级共享锁（S锁）
            context_->lock_mgr_->lock_shared_on_record(context_->txn_, scan_->rid(), fh_->GetFd());

            auto record = fh_->get_record(scan_->rid(), context_);
            if (record_satisfies_conditions(cols_, conditions_, record.get())) {
                rid_ = scan_->rid();
                return;
            }
            scan_->next();
        }
    }

    void nextTuple() override {
        assert(!is_end());
        // 继续扫描下一个满足条件的记录
        for (scan_->next(); !scan_->is_end(); scan_->next()) {
            // 申请行级共享锁（S锁）
            context_->lock_mgr_->lock_shared_on_record(context_->txn_, scan_->rid(), fh_->GetFd());

            auto record = fh_->get_record(scan_->rid(), context_);
            if (record_satisfies_conditions(cols_, fedConditions, record.get())) {
                rid_ = scan_->rid();
                return;
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

    virtual ColMeta get_col_offset(const TabCol &target) {
        for (const auto &item: cols_) {
            if (item.name == target.col_name) {
                return item;
            }
        }
        return ColMeta();
    };

    void set_begin() {
        beginTuple();
    }
};