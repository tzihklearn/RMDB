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

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include "defs.h"
#include "record/rm_defs.h"

struct ColDef {
    std::string name;       // 字段名称
    ColType type;           // 字段类型
    int len;                // 字段长度
};

struct TabCol {
    std::string tab_name;
    std::string col_name;

    friend bool operator<(const TabCol &x, const TabCol &y) {
        return std::make_pair(x.tab_name, x.col_name) < std::make_pair(y.tab_name, y.col_name);
    }
};

enum AggregationOp {
    AG_COUNT, AG_SUM, AG_MAX, AG_MIN
};

struct AggregationMeta {
    AggregationOp op;
    TabCol tableColumn;
};


struct OrderByColumn {
    TabCol tableColumn;
    bool isDesc;
};


struct DateTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;

    uint64_t encode() const {
        uint64_t ret = 0;
        ret |= static_cast<uint64_t>(year) << 40;
        ret |= static_cast<uint64_t>(month) << 32;
        ret |= static_cast<uint64_t>(day) << 24;
        ret |= static_cast<uint64_t>(hour) << 16;
        ret |= static_cast<uint64_t>(minute) << 8;
        ret |= static_cast<uint64_t>(second);
        return ret;
    }

    void decode(uint64_t code) {
        year = static_cast<int>((code >> 40) & 0xFFFF);
        month = static_cast<int>((code >> 32) & 0xFF);
        day = static_cast<int>((code >> 24) & 0xFF);
        hour = static_cast<int>((code >> 16) & 0xFF);
        minute = static_cast<int>((code >> 8) & 0xFF);
        second = static_cast<int>(code & 0xFF);
    }

    std::string encode_to_string() const {
        std::ostringstream oss;
        oss << year << "-";

        if (month < 10) {
            oss << "0";
        }
        oss << month << "-";

        if (day < 10) {
            oss << "0";
        }
        oss << day << " ";

        if (hour < 10) {
            oss << "0";
        }
        oss << hour << ":";

        if (minute < 10) {
            oss << "0";
        }
        oss << minute << ":";

        if (second < 10) {
            oss << "0";
        }
        oss << second;

        return oss.str();
    }

    void decode_from_string(const std::string &datetime_str) {
        std::istringstream iss(datetime_str);
        char delimiter;
        iss >> year >> delimiter >> month >> delimiter >> day >> hour >> delimiter >> minute >> delimiter >> second;
    }

    // 重载 > 运算符
    bool operator>(const DateTime &rhs) const {
        return encode() > rhs.encode();
    }

    // 重载 < 运算符
    bool operator<(const DateTime &rhs) const {
        return encode() < rhs.encode();
    }

    // 重载 == 运算符
    bool operator==(const DateTime &rhs) const {
        return encode() == rhs.encode();
    }

    // 重载 >= 运算符
    bool operator>=(const DateTime &rhs) const {
        return encode() >= rhs.encode();
    }

    // 重载 <= 运算符
    bool operator<=(const DateTime &rhs) const {
        return encode() <= rhs.encode();
    }
};

struct Value {
    ColType type;  // type of value
    union {
        int int_val;      // int value
        float float_val;  // float value
    };

    int64_t bigint_val;   // bigint value

    std::string str_val;  // string value

    DateTime datetime_val; // datetime value

    std::shared_ptr<RmRecord> raw;  // raw record buffer

    void set_int(int int_val_) {
        type = TYPE_INT;
        int_val = int_val_;
    }

    void set_float(float float_val_) {
        type = TYPE_FLOAT;
        float_val = float_val_;
    }

    void set_float(int int_val_) {
        type = TYPE_FLOAT;
        float_val = (float) int_val_;
    }

    void set_str(std::string str_val_) {
        type = TYPE_STRING;
        str_val = std::move(str_val_);
    }

    void set_bigint(int intVal) {
        type = TYPE_BIGINT;
        bigint_val = intVal;
    }

    // 这里要多重置一个初始化函数，不然会出问题
    void set_bigint(int64_t bigintVal) {
        type = TYPE_BIGINT;
        bigint_val = bigintVal;
    }

    void set_bigint(const std::string &bigintVal) {
        type = TYPE_BIGINT;
        try {
            bigint_val = std::stoll(bigintVal);
        } catch (std::out_of_range const &ex) {
            throw TypeOverflowError("BIGINT", bigintVal);
        }
    }

    void set_datetime(const std::string &datetime_str) {
        type = TYPE_DATETIME;
        datetime_val.decode_from_string(datetime_str);
        if (!check_datetime(datetime_val)) {
            throw TypeOverflowError("DateTime", datetime_str);
        }
    }

    void set_datetime(uint64_t datetime) {
        type = TYPE_DATETIME;
        datetime_val.decode(datetime);
        if (!check_datetime(datetime_val)) {
            throw TypeOverflowError("DateTime", datetime_val.encode_to_string());
        }
    }

    void init_raw(int len) {
        assert(raw == nullptr);
        raw = std::make_shared<RmRecord>(len);
        switch (type) {
            case TYPE_INT: {
                assert(len == sizeof(int));
                *(int *) (raw->data) = int_val;
                break;
            }
            case TYPE_FLOAT: {
                assert(len == sizeof(float));
                *(float *) (raw->data) = float_val;
                break;
            }
            case TYPE_STRING: {
                if (len < (int) str_val.size()) {
                    throw StringOverflowError();
                }
                memset(raw->data, 0, len);
                memcpy(raw->data, str_val.c_str(), str_val.size());
                break;
            }
            case TYPE_BIGINT: {
                assert(len == sizeof(int64_t));
                *(int64_t *) (raw->data) = bigint_val;
                break;
            }
            case TYPE_DATETIME: {
                assert(len == sizeof(uint64_t));
                uint64_t code = datetime_val.encode();
                *(uint64_t *) (raw->data) = code;
                break;
            }
            default: {
                throw InvalidTypeError();
                break;
            }
        }
    }

    // 重载value的比较运算符 > < == != >= <=
    bool operator>(const Value &rhs) const {
        if (!checkType(type, rhs.type)) {
            throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
        }

        switch (type) {
            case TYPE_INT:
                if (rhs.type == TYPE_INT) {
                    return int_val > rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return int_val > rhs.float_val;
                } else if (rhs.type == TYPE_BIGINT) {
                    return int_val > rhs.bigint_val;
                }
            case TYPE_FLOAT:
                if (rhs.type == TYPE_INT) {
                    return float_val > rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return float_val > rhs.float_val;
                }
            case TYPE_STRING:
                return strcmp(str_val.c_str(), rhs.str_val.c_str()) > 0;
            case TYPE_BIGINT:
                if (rhs.type == TYPE_BIGINT) {
                    return bigint_val > rhs.bigint_val;
                } else if (rhs.type == TYPE_INT) {
                    return bigint_val > rhs.int_val;
                }
            case TYPE_DATETIME:
                return datetime_val > rhs.datetime_val;
        }
        throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
    }

    bool operator<(const Value &rhs) const {
        if (!checkType(type, rhs.type)) {
            throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
        }

        switch (type) {
            case TYPE_INT:
                if (rhs.type == TYPE_INT) {
                    return int_val < rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return int_val < rhs.float_val;
                } else if (rhs.type == TYPE_BIGINT) {
                    return int_val < rhs.bigint_val;
                }
            case TYPE_FLOAT:
                if (rhs.type == TYPE_INT) {
                    return float_val < rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return float_val < rhs.float_val;
                }
            case TYPE_STRING:
                return strcmp(str_val.c_str(), rhs.str_val.c_str()) < 0;
            case TYPE_BIGINT:
                if (rhs.type == TYPE_BIGINT) {
                    return bigint_val < rhs.bigint_val;
                } else if (rhs.type == TYPE_INT) {
                    return bigint_val < rhs.int_val;
                }
            case TYPE_DATETIME:
                return datetime_val < rhs.datetime_val;
        }

        throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
    }

    bool operator==(const Value &rhs) const {
        if (!checkType(type, rhs.type)) {
            return false;
        }

        switch (type) {
            case TYPE_INT:
                if (rhs.type == TYPE_INT) {
                    return int_val == rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return int_val == rhs.float_val;
                } else if (rhs.type == TYPE_BIGINT) {
                    return int_val == rhs.bigint_val;
                }
            case TYPE_FLOAT:
                if (rhs.type == TYPE_INT) {
                    return float_val == rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return float_val == rhs.float_val;
                }
            case TYPE_STRING:
                return strcmp(str_val.c_str(), rhs.str_val.c_str()) == 0;
            case TYPE_BIGINT:
                if (rhs.type == TYPE_BIGINT) {
                    return bigint_val == rhs.bigint_val;
                } else if (rhs.type == TYPE_INT) {
                    return bigint_val == rhs.int_val;
                }
            case TYPE_DATETIME:
                return datetime_val == rhs.datetime_val;
        }

        throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
    }

    bool operator!=(const Value &rhs) const {
        return !(*this == rhs);
    }

    bool operator>=(const Value &rhs) const {
        if (!checkType(type, rhs.type)) {
            throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
        }

        switch (type) {
            case TYPE_INT:
                if (rhs.type == TYPE_INT) {
                    return int_val >= rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return int_val >= rhs.float_val;
                } else if (rhs.type == TYPE_BIGINT) {
                    return int_val >= rhs.bigint_val;
                }
            case TYPE_FLOAT:
                if (rhs.type == TYPE_INT) {
                    return float_val >= rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return float_val >= rhs.float_val;
                }
            case TYPE_STRING:
                return strcmp(str_val.c_str(), rhs.str_val.c_str()) >= 0;
            case TYPE_BIGINT:
                if (rhs.type == TYPE_BIGINT) {
                    return bigint_val >= rhs.bigint_val;
                } else if (rhs.type == TYPE_INT) {
                    return bigint_val >= rhs.int_val;
                }
            case TYPE_DATETIME:
                return datetime_val >= rhs.datetime_val;
        }

        throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
    }

    bool operator<=(const Value &rhs) const {
        if (!checkType(type, rhs.type)) {
            throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
        }

        switch (type) {
            case TYPE_INT:
                if (rhs.type == TYPE_INT) {
                    return int_val <= rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return int_val <= rhs.float_val;
                } else if (rhs.type == TYPE_BIGINT) {
                    return int_val <= rhs.bigint_val;
                }
            case TYPE_FLOAT:
                if (rhs.type == TYPE_INT) {
                    return float_val <= rhs.int_val;
                } else if (rhs.type == TYPE_FLOAT) {
                    return float_val <= rhs.float_val;
                }
            case TYPE_STRING:
                return strcmp(str_val.c_str(), rhs.str_val.c_str()) <= 0;
            case TYPE_BIGINT:
                if (rhs.type == TYPE_BIGINT) {
                    return bigint_val <= rhs.bigint_val;
                } else if (rhs.type == TYPE_INT) {
                    return bigint_val <= rhs.int_val;
                }
            case TYPE_DATETIME:
                return datetime_val <= rhs.datetime_val;
        }

        throw IncompatibleTypeError(coltype2str(type), coltype2str(rhs.type));
    }

    bool check_datetime(const DateTime &datetime) {
        // check year
        if (datetime.year < 1000 || datetime.year > 9999) {
            return false;
        }
        // check month
        if (datetime.month < 1 || datetime.month > 12) {
            return false;
        }
        // check day
        // check day
        int maxDay;
        switch (datetime.month) {
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                maxDay = 31;
                break;
            case 4:
            case 6:
            case 9:
            case 11:
                maxDay = 30;
                break;
            case 2:
                if ((datetime.year % 4 == 0 && datetime.year % 100 != 0) || datetime.year % 400 == 0) {
                    maxDay = 29;
                } else {
                    maxDay = 28;
                }
                break;
            default:
                return false;
        }
        if (datetime.day < 1 || datetime.day > maxDay) {
            return false;
        }

        if (datetime.hour < 0 || datetime.hour > 23) {
            return false;
        }

        if (datetime.minute < 0 || datetime.minute > 59) {
            return false;
        }

        if (datetime.second < 0 || datetime.second > 59) {
            return false;
        }

        return true;
    }

};

enum CompOp {
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE
};

struct Condition {
    TabCol lhs_col;   // left-hand side column
    CompOp op;        // comparison operator
    bool is_rhs_val;  // true if right-hand side is a value (not a column)
    TabCol rhs_col;   // right-hand side column
    Value rhs_val;    // right-hand side value
};

struct SetClause {
    TabCol lhs;
    Value rhs;
};