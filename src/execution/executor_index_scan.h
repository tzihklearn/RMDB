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

class IndexScanExecutor : public AbstractExecutor {
private:
    std::string tab_name_;                      // 表名称
    TabMeta tab_;                               // 表的元数据
    std::vector<Condition> conditions_;         // 扫描条件
    RmFileHandle *fh_;                          // 表的数据文件句柄
    std::vector<ColMeta> cols_;                 // 需要读取的字段
    size_t len_;                                // 选取出来的一条记录的长度
    std::vector<Condition> fedConditions;       // 扫描条件，和conditions_字段相同
    std::vector<std::string> index_col_names_;  // index scan涉及到的索引包含的字段
    IndexMeta index_meta_;                      // index scan涉及到的索引元数据

    Rid rid_;
    std::unique_ptr<RecScan> scan_;
    IxIndexHandle *ih_;

    SmManager *sm_manager_;

    bool is_end_;

public:
    IndexScanExecutor(SmManager *sm_manager, std::string tab_name, std::vector<Condition> conds,
                      std::vector<std::string> index_col_names,
                      IndexMeta index_meta, Context *context) {
        sm_manager_ = sm_manager;
        context_ = context;
        tab_name_ = std::move(tab_name);
        tab_ = sm_manager_->db_.get_table(tab_name_);
        conditions_ = std::move(conds);
        index_col_names_ = index_col_names;
        index_meta_ = index_meta;
        fh_ = sm_manager_->fhs_.at(tab_name_).get();
        auto index_name = sm_manager_->get_ix_manager()->get_index_name(tab_name_, index_meta_.cols);
        ih_ = sm_manager_->ihs_.at(index_name).get();
        cols_ = tab_.cols;
        len_ = cols_.back().offset + cols_.back().len;
        std::map<CompOp, CompOp> swap_op = {
                {OP_EQ, OP_EQ},
                {OP_NE, OP_NE},
                {OP_LT, OP_GT},
                {OP_GT, OP_LT},
                {OP_LE, OP_GE},
                {OP_GE, OP_LE},
        };

        for (auto &cond: conditions_) {
            if (cond.lhs_col.tab_name != tab_name_) {
                assert(!cond.is_rhs_val && cond.rhs_col.tab_name == tab_name_);
                std::swap(cond.lhs_col, cond.rhs_col);
                cond.op = swap_op.at(cond.op);
            }
        }
        fedConditions = conditions_;

        is_end_ = false;

        // index scan加S锁
        if (context_ != nullptr) {
            context_->lock_mgr_->lock_shared_on_table(context_->txn_, fh_->GetFd());
        }
    }

    void beginTuple() override {
        RmRecord lower_key(index_meta_.col_tot_len), upper_key(index_meta_.col_tot_len);

        size_t offset = 0;
        for (const auto &col: index_meta_.cols) {
            Value tmp_max, tmp_min;
            // 初始化tmp_max和tmp_min
            switch (col.type) {
                case TYPE_INT: {
                    tmp_max.set_int(INT32_MAX);
                    tmp_max.init_raw(col.len);
                    tmp_min.set_int(INT32_MIN);
                    tmp_min.init_raw(col.len);
                    break;
                }

                case TYPE_FLOAT: {
                    tmp_max.set_float(__FLT_MAX__);
                    tmp_max.init_raw(col.len);
                    tmp_min.set_float(-__FLT_MAX__);
                    tmp_min.init_raw(col.len);
                    break;
                }

                case TYPE_STRING : {
                    tmp_max.set_str(std::string(col.len, 255));
                    tmp_max.init_raw(col.len);
                    tmp_min.set_str(std::string(col.len, 0));
                    tmp_min.init_raw(col.len);
                    break;
                }
                case TYPE_DATETIME : {
                    tmp_max.set_datetime(std::string("9999-12-31 23:59:59"));
                    tmp_max.init_raw(col.len);
                    tmp_min.set_datetime(std::string("1000-01-01 00:00:00"));
                    tmp_min.init_raw(col.len);
                    break;
                }
                case TYPE_BIGINT : {
                    throw RMDBError("Unsupported type");
                }
                default: {
                    throw InvalidTypeError();
                }
            }

            for (auto cond: fedConditions) {
                if (cond.lhs_col.col_name == col.name && cond.is_rhs_val) {
                    switch (cond.op) {
                        case OP_EQ: {
                            if (cond.rhs_val > tmp_min) {
                                tmp_min = cond.rhs_val;
                            }
                            if (cond.rhs_val < tmp_max) {
                                tmp_max = cond.rhs_val;
                            }
                            break;
                        }
                            // >/>= rhs_val，更新tmp_min
                        case OP_GT: {
                        }
                        case OP_GE: {
                            if (cond.rhs_val > tmp_min) {
                                tmp_min = cond.rhs_val;
                            }
                            break;
                        }
                            // </<= rhs_val，更新tmp_max
                        case OP_LT: {
                        }
                        case OP_LE: {
                            if (cond.rhs_val < tmp_max) {
                                tmp_max = cond.rhs_val;
                            }
                            break;
                        }
                            // != rhs_val，不做处理
                        case OP_NE:
                        default:
                            break;
                    }
                }
            }

            // 检查tmp_min是否小于tmp_max
            assert(tmp_min <= tmp_max);
            memcpy(upper_key.data + offset, tmp_max.raw->data, col.len);
            memcpy(lower_key.data + offset, tmp_min.raw->data, col.len);
            offset += col.len;
        }

        auto lowerId = ih_->lower_bound(lower_key.data);
        auto upperId = ih_->upper_bound(upper_key.data);
        scan_ = std::make_unique<IxScan>(ih_, lowerId, upperId, sm_manager_->get_bpm());

        while (!scan_->is_end()) {
            rid_ = scan_->rid();
            auto record = fh_->get_record(rid_, context_);
            bool is_fit = true;
            for (const auto &fedCond: fedConditions) {
                auto col = *get_col(cols_, fedCond.lhs_col);
                auto value = fetch_value(record, col);
                if (fedCond.is_rhs_val && !compare_value(value, fedCond.rhs_val, fedCond.op)) {
                    is_fit = false;
                    break;
                }
            }
            if (is_fit) {
                break;
            } else {
                scan_->next();
            }
        }
    }

    void nextTuple() override {
        assert(!is_end());
        scan_->next();
        while (!scan_->is_end()) {
            rid_ = scan_->rid();
            auto record = fh_->get_record(rid_, context_);
            bool is_fit = true;
            for (const auto &fedCond: fedConditions) {
                auto col = *get_col(cols_, fedCond.lhs_col);
                auto value = fetch_value(record, col);
                if (fedCond.is_rhs_val && !compare_value(value, fedCond.rhs_val, fedCond.op)) {
                    is_fit = false;
                    break;
                }
            }
            if (is_fit) {
                break;
            } else {
                scan_->next();
            }
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        assert(!is_end());
        return fh_->get_record(rid_, context_);
    }

    Rid &rid() override { return rid_; }

    size_t tupleLen() const override { return len_; };

    const std::vector<ColMeta> &cols() const override {
        return tab_.cols;
    };

    std::string getType() override { return "IndexScanExecutor"; };

    bool is_end() const override { return scan_->is_end(); };
};