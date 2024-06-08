//
// Created by tzih on 24-5-30.
//

#ifndef RMDB_EXECUTION_AGGREGATION_H
#define RMDB_EXECUTION_AGGREGATION_H

#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"
#include <climits>
#include <cfloat>

#endif //RMDB_EXECUTION_AGGREGATION_H


class AggregationExecutor : public AbstractExecutor {
private:
    TabMeta tableMeta;                      // 表的元数据
    std::vector<Condition> conditions;      // delete的条件
    RmFileHandle *fh;                       // 表的数据文件句柄
    std::vector<AggregateMeta> aggregateMetas;        // 存储聚合操作的元数据
    std::shared_ptr<GroupByMete> group_by_col;   // group by的列
    std::string tableName;                  // 表名称
    std::vector<Rid> rids;                  // 存储记录的Rid
    SmManager *smManager;                   // 存储系统管理器对象的指针
    Value val_;                             // 存储值
    std::vector<ColMeta> outputColumnMetas; // 存储输出列的元数据
    bool is_end_;                           // 标记是否已经执行完毕

    std::map<Value, std::vector<Rid>> group_by_map; // group by的后分好的rid的map
    std::map<Value, std::vector<Rid>>::iterator iter;   // group by的后分好的rid的map的迭代器
    std::unique_ptr<RmRecord> rm_record;    // 存储RmRecord的指针
    int r_r_size;                           // 存储RmRecord的大小

public:
    AggregationExecutor(SmManager *sm_manager_, const std::string &tab_name_, std::vector<Condition> conds_,
                        std::vector<AggregateMeta> aggregateMetas_, std::shared_ptr<GroupByMete> group_by_col_,
                        std::vector<Rid> rids, Context *context) {
        this->aggregateMetas = aggregateMetas_;
        this->group_by_col = group_by_col_;
        this->smManager = sm_manager_;
        this->tableName = tab_name_;
        this->tableMeta = smManager->db_.get_table(tab_name_);
        this->fh = smManager->fhs_.at(tab_name_).get();
        this->conditions = conds_;
        this->rids = rids;
        context_ = context;
        int offset_all = 0;
        for (const auto &aggregateMeta: aggregateMetas_) {
            if (aggregateMeta.op == AG_COUNT) {
                ColMeta col_meta_ = {.tab_name = tableName, .name = "*", .type = TYPE_INT, .len = sizeof(int), .offset = offset_all, .index = false};
                outputColumnMetas.push_back(col_meta_);
                offset_all += sizeof(int);
            } else {
                ColMeta col_meta_ = *tableMeta.get_col(aggregateMeta.table_column.col_name);
                col_meta_.offset = offset_all;
                outputColumnMetas.push_back(col_meta_);
                offset_all += col_meta_.len;
            }
        }

        is_end_ = false;
    }

    std::unique_ptr<RmRecord> Next() override {
//        return std::make_unique<RmRecord>(val_.raw->size, val_.raw->data);
        return std::move(rm_record);
    }

    Rid &rid() override { return _abstract_rid; }

    std::string getType() { return "AggregationExecutor"; };

    void beginTuple() override {

        std::map<Value, std::vector<Rid>> group_by_map;
        if (group_by_col.operator bool()) {
            // group by
//            if (group_by_col->op == AG_NULL) {
            auto col_meta = tableMeta.get_col(group_by_col->tab_col.col_name);
            int offset = col_meta->offset;
            int len = col_meta->len;
            int a = rids.size();
            for (const auto &rid: rids) {
                auto rec = fh->get_record(rid, context_);
                char *data = rec->data + offset;
                Value value = Value();
                switch (col_meta->type) {
                    case TYPE_INT: {
                        int int_value = *reinterpret_cast<int *>(data);
                        value.set_int(int_value);
                        value.init_raw(sizeof(int));
                        break;
                    }
                    case TYPE_FLOAT: {
                        float float_value = *reinterpret_cast<float *>(data);
                        value.set_float(float_value);
                        value.init_raw(sizeof(float));
                        break;
                    }
                    case TYPE_STRING: {
                        std::string str_value(data, len);
                        value.set_str(str_value);
                        value.init_raw(sizeof(str_value));
                        break;
                    }
                    default: {
                        throw InvalidTypeError();
                    }
                }
                group_by_map[value].push_back(rid);
            }

            if (!group_by_col->having_metes.empty()) {

                for(auto it = group_by_map.begin(); it != group_by_map.end();) {
                    std::vector<Rid> rids = it->second;
                    bool op_r;
                    for (const auto &havingMete: group_by_col->having_metes) {
                        Value val;
                        switch (havingMete.lhs.op) {
                            case AG_COUNT: {
                                // todo: 待处理count(*)和count(col_name)，现在统一按照count(*)处理
                                val.set_int(rids.size());
                                val.init_raw(sizeof(int));
                                break;
                            }
                            case AG_MAX: {
                                auto col_meta = tableMeta.get_col(havingMete.lhs.table_column.col_name);
                                int offset = col_meta->offset;
                                int len = col_meta->len;
                                switch (col_meta->type) {
                                    case TYPE_INT: {
                                        int max_value = INT_MIN;
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            int value = *reinterpret_cast<int *>(data);
                                            if (value > max_value) {
                                                max_value = value;
                                            }
                                        }
                                        val.set_int(max_value);
                                        val.init_raw(sizeof(int));
                                        break;
                                    }
                                    case TYPE_FLOAT: {
                                        float max_value = FLT_MIN;
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            float value = *reinterpret_cast<float *>(data);
                                            if (value > max_value) {
                                                max_value = value;
                                            }
                                        }
                                        val.set_float(max_value);
                                        val.init_raw(sizeof(float));
                                        break;
                                    }
                                    case TYPE_STRING: {
                                        auto rec_begin = fh->get_record(*rids.begin(), context_);
                                        std::string max_value(rec_begin->data + offset, len);
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            std::string value(data, len);
                                            if (value > max_value) {
                                                max_value.assign(value);
                                            }
                                        }
                                        val.set_str(max_value);
                                        val.init_raw(sizeof(max_value));
                                        break;
                                    }
                                    default: {
                                        throw InvalidTypeError();
                                    }
                                }
                                break;
                            }
                            case AG_MIN: {
                                auto col_meta = tableMeta.get_col(havingMete.lhs.table_column.col_name);
                                int offset = col_meta->offset;
                                int len = col_meta->len;
                                switch (col_meta->type) {
                                    case TYPE_INT: {
                                        int min_value = INT_MAX;
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            int value = *reinterpret_cast<int *>(data);
                                            if (value < min_value) {
                                                min_value = value;
                                            }
                                        }
                                        val.set_int(min_value);
                                        val.init_raw(sizeof(int));
                                        break;
                                    }
                                    case TYPE_FLOAT: {
                                        float min_value = FLT_MAX;
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            float value = *reinterpret_cast<float *>(data);
                                            if (value < min_value) {
                                                min_value = value;
                                            }
                                        }
                                        val.set_float(min_value);
                                        val.init_raw(sizeof(float));
                                        break;
                                    }
                                    case TYPE_STRING: {
                                        auto rec_begin = fh->get_record(*rids.begin(), context_);
                                        std::string min_value(rec_begin->data + offset, len);
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;
                                            std::string value(data, len);
                                            if (value < min_value) {
                                                min_value.assign(value);
                                            }
                                        }
                                        val.set_str(min_value);
                                        val.init_raw(sizeof(min_value));
                                        break;
                                    }
                                    default: {
                                        throw InvalidTypeError();
                                    }
                                }
                                break;
                            }
                            case AG_SUM: {
                                float sum_value = 0;
                                auto col_meta = tableMeta.get_col(havingMete.lhs.table_column.col_name);
                                int offset = col_meta->offset;
                                switch (col_meta->type) {
                                    case TYPE_INT: {
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;

                                            int value = *reinterpret_cast<int *>(data);
                                            sum_value += value;
                                        }

                                        val.set_int((int) sum_value);
                                        val.init_raw(sizeof(int));
                                        break;
                                    }
                                    case TYPE_FLOAT: {
                                        for (auto &rid: rids) {
                                            auto rec = fh->get_record(rid, context_);
                                            char *data = rec->data + offset;

                                            float value = *reinterpret_cast<float *>(data);
                                            sum_value += value;
                                        }

                                        val.set_float(sum_value);
                                        val.init_raw(sizeof(float));
                                        break;
                                    }
                                    case TYPE_STRING : {
                                        throw RMDBError(
                                                "In execution_aggregation, we don't implement string and bigint and datetime type");
                                    }
                                    default: {
                                        throw InvalidTypeError();
                                    }
                                }
                                break;
                            }
                            case AG_NULL: {
                                val = iter->first;
                                break;
                            }
                            default: {

                            }
                        }

                        switch (havingMete.op) {
                            case OP_EQ:
                                op_r = val == havingMete.rhs_val;
                                break;
                            case OP_NE:
                                op_r = val != havingMete.rhs_val;
                                break;
                            case OP_LT:
                                op_r = val < havingMete.rhs_val;
                                break;
                            case OP_GT:
                                op_r = val > havingMete.rhs_val;
                                break;
                            case OP_LE:
                                op_r = val <= havingMete.rhs_val;
                                break;
                            case OP_GE:
                                op_r = val >= havingMete.rhs_val;
                                break;
                            default:
                                throw IncompatibleTypeError(coltype2str(val.type), coltype2str(havingMete.rhs_val.type));
                        }
                        if (!op_r) {
                            it = group_by_map.erase(it);
                            break;
                        }
                    }
                    if (op_r) {
                        it++;
                    }
                }
            }

        } else {
            Value value = Value();
            for (const auto &rid: rids) {
                group_by_map[value].push_back(rid);
            }
        }
        this->group_by_map = group_by_map;
        iter = this->group_by_map.begin();

        int size = 0;
        for (const auto &aggregateMeta: aggregateMetas) {
            if (aggregateMeta.op == AG_COUNT) {
                size += sizeof(int);
            } else {
                auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                size += col_meta->len;
            }
        }
        r_r_size = size;

        // 先执行一次nextTuple
        nextTuple();
    }

    void nextTuple() override {

        if (iter == group_by_map.end()) {
            is_end_ = true;
            return;
        }
        rm_record = std::make_unique<RmRecord>(r_r_size);
        std::vector<Rid> rids = iter->second;
        int rm_offset = 0;
        for (const auto &aggregateMeta: aggregateMetas) {
            Value val;
            switch (aggregateMeta.op) {
                case AG_COUNT: {
//                    // 处理count元组数量
//                    if (aggregateMeta.table_column.col_name.empty()) {
//                        val.set_int(tableMeta.cols.size());
//                        val.init_raw(sizeof (int));
//                    } else {
//
//                    }
                    // todo: 待处理count(*)和count(col_name)，现在统一按照count(*)处理
                    val.set_int(rids.size());
                    val.init_raw(sizeof(int));
                    break;
                }
                case AG_MAX: {
                    auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                    int offset = col_meta->offset;
                    int len = col_meta->len;
                    switch (col_meta->type) {
                        case TYPE_INT: {
                            int max_value = INT_MIN;
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                int value = *reinterpret_cast<int *>(data);
                                if (value > max_value) {
                                    max_value = value;
                                }
                            }
                            val.set_int(max_value);
                            val.init_raw(sizeof(int));
                            break;
                        }
                        case TYPE_FLOAT: {
                            float max_value = FLT_MIN;
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                float value = *reinterpret_cast<float *>(data);
                                if (value > max_value) {
                                    max_value = value;
                                }
                            }
                            val.set_float(max_value);
                            val.init_raw(sizeof(float));
                            break;
                        }
                        case TYPE_STRING: {
                            auto rec_begin = fh->get_record(*rids.begin(), context_);
                            std::string max_value(rec_begin->data + offset, len);
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                std::string value(data, len);
                                if (value > max_value) {
                                    max_value.assign(value);
                                }
                            }
                            val.set_str(max_value);
                            val.init_raw(sizeof(max_value));
                            break;
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    break;
                }
                case AG_MIN: {
                    auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                    int offset = col_meta->offset;
                    int len = col_meta->len;
                    switch (col_meta->type) {
                        case TYPE_INT: {
                            int min_value = INT_MAX;
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                int value = *reinterpret_cast<int *>(data);
                                if (value < min_value) {
                                    min_value = value;
                                }
                            }
                            val.set_int(min_value);
                            val.init_raw(sizeof(int));
                            break;
                        }
                        case TYPE_FLOAT: {
                            float min_value = FLT_MAX;
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                float value = *reinterpret_cast<float *>(data);
                                if (value < min_value) {
                                    min_value = value;
                                }
                            }
                            val.set_float(min_value);
                            val.init_raw(sizeof(float));
                            break;
                        }
                        case TYPE_STRING: {
                            auto rec_begin = fh->get_record(*rids.begin(), context_);
                            std::string min_value(rec_begin->data + offset, len);
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;
                                std::string value(data, len);
                                if (value < min_value) {
                                    min_value.assign(value);
                                }
                            }
                            val.set_str(min_value);
                            val.init_raw(sizeof(min_value));
                            break;
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    break;
                }
                case AG_SUM: {
                    float sum_value = 0;
                    auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                    int offset = col_meta->offset;
                    switch (col_meta->type) {
                        case TYPE_INT: {
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;

                                int value = *reinterpret_cast<int *>(data);
                                sum_value += value;
                            }

                            val.set_int((int) sum_value);
                            val.init_raw(sizeof(int));
                            break;
                        }
                        case TYPE_FLOAT: {
                            for (auto &rid: rids) {
                                auto rec = fh->get_record(rid, context_);
                                char *data = rec->data + offset;

                                float value = *reinterpret_cast<float *>(data);
                                sum_value += value;
                            }

                            val.set_float(sum_value);
                            val.init_raw(sizeof(float));
                            break;
                        }
                        case TYPE_STRING : {
                            throw RMDBError(
                                    "In execution_aggregation, we don't implement string and bigint and datetime type");
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    break;
                }
                case AG_NULL: {
                    val = iter->first;
                    break;
                }
                default: {

                }
            }
            if (aggregateMeta.op == AG_COUNT) {
                // count(*)
                memcpy(rm_record->data + rm_offset, val.raw->data, sizeof(int));
                rm_offset += sizeof(int);
            } else {
                auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                memcpy(rm_record->data + rm_offset, val.raw->data, col_meta->len);
                rm_offset += col_meta->len;
            }
        }
        ++iter;
    }

    bool is_end() const override {
        return is_end_;
    };

    const std::vector<ColMeta> &cols() const {
        return outputColumnMetas;
    };
};