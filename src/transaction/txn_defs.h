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

#include <atomic>

#include "common/config.h"
#include "defs.h"
#include "record/rm_defs.h"

/* 标识事务状态 */
enum class TransactionState {
    DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
};

/* 系统的隔离级别，当前赛题中为可串行化隔离级别 */
enum class IsolationLevel {
    READ_UNCOMMITTED, REPEATABLE_READ, READ_COMMITTED, SERIALIZABLE
};

/* 事务写操作类型，包括插入、删除、更新三种操作 */
enum class WType {
    INSERT_TUPLE = 0, DELETE_TUPLE, UPDATE_TUPLE
};

/* 索引回滚类型 */
enum class IType {
    INSERT_INDEX = 0, DROP_INDEX
};

/**
 * @brief 事务的写操作记录，用于事务的回滚
 * INSERT
 * --------------------------------
 * | wtype | tableName | tuple_rid |
 * --------------------------------
 * DELETE / UPDATE
 * ----------------------------------------------
 * | wtype | tableName | tuple_rid | tuple_value |
 * ----------------------------------------------
 */
class TableWriteRecord {
public:
    TableWriteRecord() = default;

    // constructor for insert operation
    TableWriteRecord(WType wtype, const std::string &tab_name, const Rid &rid)
            : wtype_(wtype), tab_name_(tab_name), rid_(rid) {}

    // constructor for delete operation
    TableWriteRecord(WType wtype, const std::string &tab_name, const Rid &rid, const RmRecord &record)
            : wtype_(wtype), tab_name_(tab_name), rid_(rid), record_(record) {}

    // constructor for update operation
    TableWriteRecord(WType wtype, const std::string &tab_name, const Rid &rid, const RmRecord &before_record,
                     const RmRecord &record)
            : wtype_(wtype), tab_name_(tab_name), rid_(rid), before_record_(before_record), record_(record) {}

    ~TableWriteRecord() = default;

    inline RmRecord &GetRecord() { return record_; }

    inline Rid &GetRid() { return rid_; }

    inline WType &GetWriteType() { return wtype_; }

    inline std::string &GetTableName() { return tab_name_; }

    inline RmRecord &GetBeforeRecord() { return before_record_; }

private:
    WType wtype_;
    std::string tab_name_;
    Rid rid_;
    RmRecord before_record_;  // 新增成员变量，用于存储更新前的记录
    RmRecord record_;
};

class IndexWriteRecord {
public:
    IndexWriteRecord() = default;

    IndexWriteRecord(WType wtype, const std::string &tab_name, const Rid &rid, const char *key, int size)
            : wType(wtype), tab_name_(tab_name), rid_(rid) {
        key_ = new char[size];
        memcpy(key_, key, size);
    }

    // Constructor for update operations
    IndexWriteRecord(WType wtype, const std::string &tab_name, const Rid &rid, const char *old_key, const char *new_key,
                     int size)
            : wType(wtype), tab_name_(tab_name), rid_(rid) {
        old_key_ = new char[size];
        new_key_ = new char[size];
        memcpy(old_key_, old_key, size);
        memcpy(new_key_, new_key, size);
    }

    ~IndexWriteRecord() {
        delete[] key_;
        delete[] old_key_;
        delete[] new_key_;
    }

    inline char *GetKey() { return key_; }

    inline char *GetOldKey() { return old_key_; }

    inline char *GetNewKey() { return new_key_; }

    inline Rid &GetRid() { return rid_; }

    inline WType &GetWriteType() { return wType; }

    inline std::string &GetTableName() { return tab_name_; }

private:
    WType wType;
    std::string tab_name_;
    Rid rid_;
    char *key_ = nullptr;
    char *old_key_ = nullptr;
    char *new_key_ = nullptr;
};

class IndexCreateRecord {
public:
    IndexCreateRecord() = default;

    IndexCreateRecord(IType iType, const std::string &index_name, std::string tab_name,
                      std::vector<std::string> col_names)
            : iType(iType), index_name_(index_name), tab_name_(tab_name), col_names_(col_names) {
    }

    inline IType &GetCreateType() { return iType; }

    inline std::string &GetIndexName() { return index_name_; }

    inline std::string &GetTabName() { return tab_name_; }

    inline std::vector<std::string> &GetCalNames() { return col_names_; }

private:
    IType iType;
    std::string index_name_;
    std::string tab_name_;
    std::vector<std::string> col_names_;
};

/* 多粒度锁，加锁对象的类型，包括记录和表 */
enum class LockDataType {
    TABLE = 0, RECORD = 1
};

/**
 * @description: 加锁对象的唯一标识
 */
class LockDataId {
public:
    /* 表级锁 */
    LockDataId(int fd, LockDataType type) {
        assert(type == LockDataType::TABLE);
        fd_ = fd;
        type_ = type;
        rid_.page_no = -1;
        rid_.slot_no = -1;
    }

    /* 行级锁 */
    LockDataId(int fd, const Rid &rid, LockDataType type) {
        assert(type == LockDataType::RECORD);
        fd_ = fd;
        rid_ = rid;
        type_ = type;
    }

    inline int64_t Get() const {
        if (type_ == LockDataType::TABLE) {
            // fd_
            return static_cast<int64_t>(fd_);
        } else {
            // fd_, rid_.page_no, rid.slot_no
            return ((static_cast<int64_t>(type_)) << 63) | ((static_cast<int64_t>(fd_)) << 31) |
                   ((static_cast<int64_t>(rid_.page_no)) << 16) | rid_.slot_no;
        }
    }

    bool operator==(const LockDataId &other) const {
        if (type_ != other.type_) return false;
        if (fd_ != other.fd_) return false;
        return rid_ == other.rid_;
    }

    int fd_;
    Rid rid_;
    LockDataType type_;
};

template<>
struct std::hash<LockDataId> {
    size_t operator()(const LockDataId &obj) const { return std::hash<int64_t>()(obj.Get()); }
};

/* 事务回滚原因 */
enum class AbortReason {
    LOCK_ON_SHRINKING = 0, UPGRADE_CONFLICT, DEADLOCK_PREVENTION
};

/* 事务回滚异常，在rmdb.cpp中进行处理 */
class TransactionAbortException : public std::exception {
    txn_id_t txn_id_;
    AbortReason abort_reason_;

public:
    explicit TransactionAbortException(txn_id_t txn_id, AbortReason abort_reason)
            : txn_id_(txn_id), abort_reason_(abort_reason) {}

    txn_id_t get_transaction_id() { return txn_id_; }

    AbortReason GetAbortReason() { return abort_reason_; }

    std::string GetInfo() {
        switch (abort_reason_) {
            case AbortReason::LOCK_ON_SHRINKING: {
                return "Transaction " + std::to_string(txn_id_) +
                       " aborted because it cannot request locks on SHRINKING phase\n";
            }
                break;

            case AbortReason::UPGRADE_CONFLICT: {
                return "Transaction " + std::to_string(txn_id_) +
                       " aborted because another transaction is waiting for upgrading\n";
            }
                break;

            case AbortReason::DEADLOCK_PREVENTION: {
                return "Transaction " + std::to_string(txn_id_) + " aborted for deadlock prevention\n";
            }
                break;

            default: {
                return "Transaction aborted\n";
            }
                break;
        }
    }
};