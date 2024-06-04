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
#include "common/common.h"
#include "index/ix.h"
#include "system/sm.h"
#include "defs.h"

class AbstractExecutor {
public:
    Rid _abstract_rid;

    Context *context_;

    virtual ~AbstractExecutor() = default;

    virtual size_t tupleLen() const { return 0; };

    virtual const std::vector<ColMeta> &cols() const {
        std::vector<ColMeta> *_cols = nullptr;
        return *_cols;
    };

    virtual std::string getType() { return "AbstractExecutor"; };

    virtual void beginTuple() {};

    virtual void nextTuple() {};

    virtual bool is_end() const { return true; };

    virtual Rid &rid() = 0;

    virtual std::unique_ptr<RmRecord> Next() = 0;

    virtual ColMeta get_col_offset(const TabCol &target) { return ColMeta(); };

    std::vector<ColMeta>::const_iterator get_col(const std::vector<ColMeta> &rec_cols, const TabCol &target) {
        auto pos = std::find_if(rec_cols.begin(), rec_cols.end(), [&](const ColMeta &col) {
            return col.tab_name == target.tab_name && col.name == target.col_name;
        });
        if (pos == rec_cols.end()) {
            throw ColumnNotFoundError(target.tab_name + '.' + target.col_name);
        }
        return pos;
    }

    // 从 Record 中取出某一列的 Value
    Value fetch_value(const std::unique_ptr<RmRecord> &record, const ColMeta &columnMeta) const {
        char *data = record->data + columnMeta.offset;
        size_t len = columnMeta.len;
        Value result;
        result.type = columnMeta.type;
        if (columnMeta.type == TYPE_INT) {
            int tmp;
            memcpy((char *) &tmp, data, len);
            result.set_int(tmp);
        } else if (columnMeta.type == TYPE_FLOAT) {
            float tmp;
            memcpy((char *) &tmp, data, len);
            result.set_float(tmp);
        } else if (columnMeta.type == TYPE_STRING) {
            std::string tmp(data, len);
            result.set_str(tmp);
        } else {
            throw InvalidTypeError();
        }
        result.init_raw(len);
        return result;
    }

    bool compare_value(const Value &leftValue, const Value &rightValue, CompOp op) const {
        if (!checkType(leftValue.type, rightValue.type)) {
            throw IncompatibleTypeError(coltype2str(leftValue.type), coltype2str(rightValue.type));
        }

        switch (op) {
            case OP_EQ:
                return leftValue == rightValue;
            case OP_NE:
                return leftValue != rightValue;
            case OP_LT:
                return leftValue < rightValue;
            case OP_GT:
                return leftValue > rightValue;
            case OP_LE:
                return leftValue <= rightValue;
            case OP_GE:
                return leftValue >= rightValue;
            default:
                throw IncompatibleTypeError(coltype2str(leftValue.type), coltype2str(rightValue.type));
        }
    }
};