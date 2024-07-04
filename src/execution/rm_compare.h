//
// Created by tzih on 24-6-22.
//

#ifndef RMDB_RMCOMPARE_H
#define RMDB_RMCOMPARE_H

#include "system/sm_meta.h"
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class RmCompare : AbstractExecutor {
    std::shared_ptr<ColMeta> order_col_mete;
    bool is_desc;

public:
    RmCompare(std::shared_ptr<ColMeta> order_col_mete_, bool is_desc_) :
            order_col_mete(order_col_mete_), is_desc(is_desc_) {}

    RmCompare() {}

    bool operator()(const std::unique_ptr<RmRecord> &lhs, const std::unique_ptr<RmRecord> &rhs) {
        if (order_col_mete.operator bool()) {

            auto left_value = fetch_value(lhs, *order_col_mete);
            auto right_value = fetch_value(rhs, *order_col_mete);
            bool flag = compare_value(left_value, right_value, OP_GT);

//            // 后面符合条件的元组根据排序顺序 (降序 / 升序)
//            if ((!flag && is_desc) || (flag && !is_desc)) {
//                return false;
//            } else {
//                return true;
//            }
            // 调整比较逻辑
            if (is_desc) {
                return flag; // 如果降序排序，则 lhs 应该大于 rhs
            } else {
                return !flag; // 如果升序排序，则 lhs 应该小于 rhs
            }
        }
        return true;
    }

    Rid &rid() override { return _abstract_rid; }

    std::unique_ptr<RmRecord> Next() override { return std::make_unique<RmRecord>(); }

};


#endif //RMDB_RMCOMPARE_H
