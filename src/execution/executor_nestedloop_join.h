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

class NestedLoopJoinExecutor : public AbstractExecutor {
private:
    std::unique_ptr<AbstractExecutor> left_;    // 左儿子节点（需要join的表）
    std::unique_ptr<AbstractExecutor> right_;   // 右儿子节点（需要join的表）
    size_t len_;                                // join后获得的每条记录的长度
    std::vector<ColMeta> cols_;                 // join后获得的记录的字段

    std::vector<Condition> fed_conds_;          // join条件
    // isend 变量表示当前连接操作是否已经完成。
    bool isend;

public:
    NestedLoopJoinExecutor(std::unique_ptr<AbstractExecutor> left, std::unique_ptr<AbstractExecutor> right,
                           std::vector<Condition> conds) {
        left_ = std::move(left);
        right_ = std::move(right);
        len_ = left_->tupleLen() + right_->tupleLen();
        cols_ = left_->cols();
        auto right_cols = right_->cols();
        for (auto &col: right_cols) {
            col.offset += left_->tupleLen();
        }

        cols_.insert(cols_.end(), right_cols.begin(), right_cols.end());
        isend = false;
        fed_conds_ = std::move(conds);

    }

    const std::vector<ColMeta> &cols() const override {
        return cols_;
    };

    // 记得重写isEnd方法
    bool is_end() const override {
        return isend;
    };

    size_t tupleLen() const override { return len_; };

    std::string getType() override { return "NestedLoopJoinExecutor"; };

    void beginTuple() override {
        // 完成左右子节点的元组重置
        left_->beginTuple();
        right_->beginTuple();
        // 每次开始新的元组处理之前，需要将 is_end 变量重置为 false
        isend = false;
        findNextValidTuple();
    }

    void nextTuple() override {
        // 如果右侧指针已经到达当前数据块的末尾
        if (right_->is_end()) {
            // 将左侧指针移动到下一个数据块
            left_->nextTuple();
            // 将右侧指针指向新的数据块的第一个元组
            right_->beginTuple();
        } else {
            // 将右侧指针移动到下一个元组
            right_->nextTuple();
        }
        // 找到下一个有效的元组
        findNextValidTuple();
    }

    std::unique_ptr<RmRecord> Next() override {
        // 递归取左右子树
        auto leftRecord = left_->Next();
        auto rightRecord = right_->Next();
        // 合并到一起
        auto result = std::make_unique<RmRecord>(len_);
        memcpy(result->data, leftRecord->data, leftRecord->size);
        memcpy(result->data + leftRecord->size, rightRecord->data, rightRecord->size);
        // 返回
        return result;
    }

    Rid &rid() override { return _abstract_rid; }

    void findNextValidTuple() {
        while (!left_->is_end()) {
            // 取左节点的record和列值
            auto leftRecord = left_->Next();
            auto leftCols = left_->cols();

            while (!right_->is_end()) {
                // 取右节点的record和列值
                auto rightRecord = right_->Next();
                auto rightCols = right_->cols();

                bool isFit = true;

                for (const auto &condition: fed_conds_) {
                    // 取左节点值
                    auto leftCol = *(left_->get_col(leftCols, condition.lhs_col));
                    auto leftValue = fetch_value(leftRecord, leftCol);

                    // 取右节点值
                    Value rightValue;
                    if (condition.is_rhs_val) {
                        rightValue = condition.rhs_val;
                    } else if (!condition.is_rhs_in){
                        auto rightCol = *(right_->get_col(rightCols, condition.rhs_col));
                        rightValue = fetch_value(rightRecord, rightCol);
                    }
                    // 比较是否符合条件
                    if (condition.is_rhs_in) {
                        for (const auto &rhs_val: condition.rhs_in_vals) {
                            isFit = false;
                            if (compare_value(leftValue, rhs_val, OP_EQ)) {
                                isFit = true;
                                break;
                            }
                        }
                    } else if (!compare_value(leftValue, rightValue, condition.op)) {
                        isFit = false;
                        break;
                    }

                }
                // 符合条件直接跳出
                if (isFit) {
                    return;
                }
                // 不符合则移动右节点到下一个位置
                right_->nextTuple();
            }
            // 移动左节点到下一个位置
            left_->nextTuple();
            // 重置右节点
            right_->beginTuple();
        }
        isend = true;
    }
};