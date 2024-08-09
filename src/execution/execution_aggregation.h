#ifndef RMDB_EXECUTION_AGGREGATION_H
#define RMDB_EXECUTION_AGGREGATION_H

#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"
#include <climits>
#include <cfloat>
#include <utility>

#endif //RMDB_EXECUTION_AGGREGATION_H

struct Key {
    std::vector<Value> value_list;

    Key(std::vector<Value> value_list_) : value_list(std::move(value_list_)) {}

    Key() = default;

    bool operator==(const Key &other) const {
        return value_list == other.value_list;
    }

    bool operator<(const Key &other) const {
        return value_list < other.value_list;
    }
};

class AggregationExecutor : public AbstractExecutor {
private:
    TabMeta tableMeta;                      // 表的元数据
    std::vector<Condition> conditions;      // delete的条件
    RmFileHandle *fh;                       // 表的数据文件句柄
    std::vector<AggregateMeta> aggregateMetas;        // 存储聚合操作的元数据
    std::vector<std::shared_ptr<GroupByMete>> group_by_cols;   // group by的列
    std::string tableName;                  // 表名称
    std::vector<Rid> rids;                  // 存储记录的Rid
    SmManager *smManager;                   // 存储系统管理器对象的指针
    Value val_;                             // 存储值
    std::vector<ColMeta> outputColumnMetas; // 存储输出列的元数据
    bool is_end_;                           // 标记是否已经执行完毕

    std::map<Key, std::vector<Rid>> group_by_map; // group by的后分好的rid的map
    std::map<Key, std::vector<Rid>>::iterator iter;   // group by的后分好的rid的map的迭代器
    std::unique_ptr<RmRecord> rm_record;    // 存储RmRecord的指针
    int r_r_size;                           // 存储RmRecord的大小

public:
    AggregationExecutor(SmManager *sm_manager_, const std::string &tab_name_, std::vector<Condition> conds_,
                        const std::vector<AggregateMeta> &aggregateMetas_,
                        std::vector<std::shared_ptr<GroupByMete>> group_by_cols_,
                        std::vector<Rid> rids, Context *context) {
        this->aggregateMetas = aggregateMetas_;
        this->group_by_cols = std::move(group_by_cols_);
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
        return std::move(rm_record);
    }

    Rid &rid() override { return _abstract_rid; }

    std::string getType() override { return "AggregationExecutor"; }

    void beginTuple() override {
        exit(1);
        std::map<Key, std::vector<Rid>> group_by_map_t;
        if (!group_by_cols.empty()) {
            std::vector<ColMeta> colMetes;
            unsigned int n = group_by_cols.size();
            for (const auto &item: group_by_cols) {
                auto col_meta = tableMeta.get_col(item->tab_col.col_name);
                colMetes.push_back(*col_meta);
            }

            for (const auto &rid: rids) {
                auto rec = fh->get_record(rid, context_);
                std::vector<Value> value_list;
                for (unsigned int i = 0; i < n; ++i) {
                    auto col_meta = colMetes[i];
                    int offset = col_meta.offset;
                    int len = col_meta.len;
                    char *data = rec->data + offset;
                    Value value;
                    switch (col_meta.type) {
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
                    value_list.push_back(value);
                }
                Key key = Key(value_list);
                group_by_map_t[key].push_back(rid);
            }

            int index = 0;
            for (const auto &item: group_by_cols) {
                if (!item->having_metes.empty()) {
                    for (auto it = group_by_map_t.begin(); it != group_by_map_t.end();) {
                        std::vector<Rid> rids_t = it->second;
                        bool op_r = true;
                        for (const auto &havingMete: item->having_metes) {
                            Value val;
                            switch (havingMete.lhs.op) {
                                case AG_COUNT: {
                                    val.set_int(rids_t.size());
                                    val.init_raw(sizeof(int));
                                    break;
                                }
                                case AG_MAX: {
                                    val = getMaxValue(rids_t, havingMete.lhs.table_column);
                                    break;
                                }
                                case AG_MIN: {
                                    val = getMinValue(rids_t, havingMete.lhs.table_column);
                                    break;
                                }
                                case AG_SUM: {
                                    val = getSumValue(rids_t, havingMete.lhs.table_column);
                                    break;
                                }
                                case AG_NULL: {
                                    val = it->first.value_list[index];
                                    ++index;
                                    break;
                                }
                                default: {
                                    throw InvalidTypeError();
                                }
                            }
                            if (!compareValue(val, havingMete.op, havingMete.rhs_val)) {
                                op_r = false;
                                break;
                            }
                        }
                        if (!op_r) {
                            it = group_by_map_t.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        } else {
            Key key;
            for (const auto &rid: rids) {
                group_by_map_t[key].push_back(rid);
            }
        }
        this->group_by_map = group_by_map_t;
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

        bool flag = true;
        if (iter == group_by_map.end()) {
            int rm_offset = 0;
            rm_record = std::make_unique<RmRecord>(r_r_size);

            for (const auto &aggregateMeta: aggregateMetas) {
                if (aggregateMeta.op != AG_COUNT) {
                    flag = false;
                    break;
                }
            }
            if (flag) {
                for (const auto &aggregateMeta: aggregateMetas) {
                    Value val;
                    switch (aggregateMeta.op) {
                        case AG_COUNT: {
                            val.set_int(rids.size());
                            val.init_raw(sizeof(int));
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                    if (aggregateMeta.op == AG_COUNT) {
                        memcpy(rm_record->data + rm_offset, val.raw->data, sizeof(int));
                        rm_offset += sizeof(int);
                    }
                }
            }
        } else {
            flag = false;
        }
        nextTuple();
        if (flag) {
            is_end_ = false;
        }
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
                    val.set_int(rids.size());
                    val.init_raw(sizeof(int));
                    break;
                }
                case AG_MAX: {
                    val = getMaxValue(rids, aggregateMeta.table_column);
                    break;
                }
                case AG_MIN: {
                    val = getMinValue(rids, aggregateMeta.table_column);
                    break;
                }
                case AG_SUM: {
                    val = getSumValue(rids, aggregateMeta.table_column);
                    break;
                }
                case AG_NULL: {
                    auto rec = fh->get_record(rids[0], context_);
                    auto col_meta = tableMeta.get_col(aggregateMeta.table_column.col_name);
                    int offset = col_meta->offset;
                    int len = col_meta->len;
                    char *data = rec->data + offset;
                    switch (col_meta->type) {
                        case TYPE_INT: {
                            int int_value = *reinterpret_cast<int *>(data);
                            val.set_int(int_value);
                            val.init_raw(sizeof(int));
                            break;
                        }
                        case TYPE_FLOAT: {
                            float float_value = *reinterpret_cast<float *>(data);
                            val.set_float(float_value);
                            val.init_raw(sizeof(float));
                            break;
                        }
                        case TYPE_STRING: {
                            std::string str_value(data, len);
                            val.set_str(str_value);
                            val.init_raw(sizeof(str_value));
                            break;
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    break;
                }
                default: {
                    break;
                }
            }
            if (aggregateMeta.op == AG_COUNT) {
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
    }

    const std::vector<ColMeta> &cols() const override {
        return outputColumnMetas;
    }

private:
    Value getMaxValue(const std::vector<Rid> &rids, const TabCol &tab_col) {
        auto col_meta = tableMeta.get_col(tab_col.col_name);
        int offset = col_meta->offset;
        int len = col_meta->len;
        Value val;

        if (rids.empty()) {
            throw RMDBError("No records to calculate MAX value.");
        }

        switch (col_meta->type) {
            case TYPE_INT: {
                int max_value = INT_MIN;
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    int value = *reinterpret_cast<int *>(rec->data + offset);
                    max_value = std::max(max_value, value);
                }
                val.set_int(max_value);
                val.init_raw(sizeof(int));
                break;
            }
            case TYPE_FLOAT: {
                float max_value = -FLT_MAX;
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    float value = *reinterpret_cast<float *>(rec->data + offset);
                    max_value = std::max(max_value, value);
                }
                val.set_float(max_value);
                val.init_raw(sizeof(float));
                break;
            }
            case TYPE_STRING: {
                auto rec = fh->get_record(rids.front(), context_);
                std::string max_value(rec->data + offset, len);
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    std::string value(rec->data + offset, len);
                    if (value > max_value) {
                        max_value = value;
                    }
                }
                val.set_str(max_value);
                val.init_raw(len);
                break;
            }
            default: {
                throw InvalidTypeError();
            }
        }
        return val;
    }

    Value getMinValue(const std::vector<Rid> &rids, const TabCol &tab_col) {
        auto col_meta = tableMeta.get_col(tab_col.col_name);
        int offset = col_meta->offset;
        int len = col_meta->len;
        Value val;

        if (rids.empty()) {
            throw RMDBError("No records to calculate MIN value.");
        }

        switch (col_meta->type) {
            case TYPE_INT: {
                int min_value = INT_MAX;
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    int value = *reinterpret_cast<int *>(rec->data + offset);
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
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    float value = *reinterpret_cast<float *>(rec->data + offset);
                    min_value = std::min(min_value, value);
                }
                val.set_float(min_value);
                val.init_raw(sizeof(float));
                break;
            }
            case TYPE_STRING: {
                auto rec = fh->get_record(rids.front(), context_);
                std::string min_value(rec->data + offset, len);
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    std::string value(rec->data + offset, len);
                    min_value = std::min(min_value, value);
                }
                val.set_str(min_value);
                val.init_raw(len);
                break;
            }
            default: {
                throw InvalidTypeError();
            }
        }
        return val;
    }

    Value getSumValue(const std::vector<Rid> &rids, const TabCol &tab_col) {
        auto col_meta = tableMeta.get_col(tab_col.col_name);
        int offset = col_meta->offset;
        Value val;
        double sum_value = 0.0;

        if (rids.empty()) {
            throw RMDBError("No records to calculate SUM value.");
        }

        switch (col_meta->type) {
            case TYPE_INT: {
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    int value = *reinterpret_cast<int *>(rec->data + offset);
                    if ((sum_value > 0 && value > 0 && sum_value > (std::numeric_limits<int>::max() - value)) ||
                        (sum_value < 0 && value < 0 && sum_value < (std::numeric_limits<int>::min() - value))) {
                        throw RMDBError("Sum value overflow/underflow");
                    }
                    sum_value += value;
                }
                val.set_int(static_cast<int>(sum_value));
                val.init_raw(sizeof(int));
                break;
            }
            case TYPE_FLOAT: {
                for (const auto &rid: rids) {
                    auto rec = fh->get_record(rid, context_);
                    float value = *reinterpret_cast<float *>(rec->data + offset);
                    if ((sum_value > 0 && value > 0 && sum_value > (std::numeric_limits<float>::max() - value)) ||
                        (sum_value < 0 && value < 0 && sum_value < (std::numeric_limits<float>::min() - value))) {
                        throw RMDBError("Sum value overflow/underflow");
                    }
                    sum_value += value;
                }
                val.set_float(static_cast<float>(sum_value));
                val.init_raw(sizeof(float));
                break;
            }
            default: {
                throw InvalidTypeError();
            }
        }
        return val;
    }

    bool compareValue(const Value &lhs, CompOp op, const Value &rhs) {
        switch (op) {
            case OP_EQ:
                return lhs == rhs;
            case OP_NE:
                return lhs != rhs;
            case OP_LT:
                return lhs < rhs;
            case OP_GT:
                return lhs > rhs;
            case OP_LE:
                return lhs <= rhs;
            case OP_GE:
                return lhs >= rhs;
            default:
                throw IncompatibleTypeError(coltype2str(lhs.type), coltype2str(rhs.type));
        }
    }
};