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

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "sm_defs.h"

/* 字段元数据 */
struct ColMeta {
    std::string tab_name;   // 字段所属表名称
    std::string name;       // 字段名称
    ColType type;           // 字段类型
    int len;                // 字段长度
    int offset;             // 字段位于记录中的偏移量
    bool index;             /** unused */

    friend std::ostream &operator<<(std::ostream &os, const ColMeta &col) {
        // ColMeta中有各个基本类型的变量，然后调用重载的这些变量的操作符<<（具体实现逻辑在defs.h）
        return os << col.tab_name << ' ' << col.name << ' ' << col.type << ' ' << col.len << ' ' << col.offset << ' '
                  << col.index;
    }

    friend std::istream &operator>>(std::istream &is, ColMeta &col) {
        return is >> col.tab_name >> col.name >> col.type >> col.len >> col.offset >> col.index;
    }
};

/* 索引元数据 */
struct IndexMeta {
    std::string tab_name;           // 索引所属表名称
    int col_tot_len;                // 索引字段长度总和
    int col_num;                    // 索引字段数量
    std::vector<ColMeta> cols;      // 索引包含的字段

    friend std::ostream &operator<<(std::ostream &os, const IndexMeta &index) {
        os << index.tab_name << " " << index.col_tot_len << " " << index.col_num;
        for (auto &col: index.cols) {
            os << "\n" << col;
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, IndexMeta &index) {
        is >> index.tab_name >> index.col_tot_len >> index.col_num;
        for (int i = 0; i < index.col_num; ++i) {
            ColMeta col;
            is >> col;
            index.cols.push_back(col);
        }
        return is;
    }

    bool contains_column(const std::string &column_name) const {
        for (const ColMeta &col: cols) {
            if (col.name == column_name) {
                return true;
            }
        }
        return false;
    }

    const std::string &get_column_name(size_t index) const {
        if (index >= cols.size()) {
            throw std::out_of_range("Index is out of range");
        }
        return cols[index].name;
    }

    unsigned long get_column_count() {
        return cols.size();
    }
};

/* 表元数据 */
struct TabMeta {
    std::string name;                   // 表名称
    std::vector<ColMeta> cols;          // 表包含的字段
    std::vector<IndexMeta> indexes;     // 表上建立的索引

    TabMeta() {}

    TabMeta(const TabMeta &other) {
        name = other.name;
        for (auto col: other.cols) cols.push_back(col);
    }

    /* 判断当前表中是否存在名为col_name的字段 */
    bool is_col(const std::string &col_name) const {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const ColMeta &col) { return col.name == col_name; });
        return pos != cols.end();
    }

    /* 判断当前表上是否建有指定索引，索引包含的字段为col_names */
    // 最左匹配原则，自动调换col_names的顺序
    std::pair<bool, IndexMeta> have_index(const std::vector<std::string> &col_names) const {
        // 先统计cols
        std::map<std::string, bool> col2bool;
        for (auto col_name: col_names) {
            col2bool[col_name] = true;
        }

        // 找最匹配的index_meta
        int min_not_match_cols = INT32_MAX;
        IndexMeta const *most_match_index = nullptr;

        for (auto &index: indexes) {
            // 检查是否符合index需求
            // 1. 统计有多少连续的列用到了index
            size_t i = 0;
            int not_match_cols = 0;
            while (i < index.col_num) {
                if (col2bool[index.cols[i].name]) {
                    i++;
                } else {
                    break;
                }
            }
            // 2. 如果没有发现，那么该索引不匹配
            if (i == 0) {
                continue;
            }
            // 3. 检查之后的所有列是否都没有在索引中
            while (i < index.col_num) {
                if (!col2bool[index.cols[i].name]) {
                    i++;
                    not_match_cols++;
                } else {
                    break;
                }
            }
            // 4. 如果i == col_num那么说明之后的所有列都没有用到该索引，否则则说明用到了，该索引不符合最左匹配
            if (i == index.col_num && not_match_cols < min_not_match_cols) {
                most_match_index = &index;
                min_not_match_cols = not_match_cols;
            } else {
                continue;
            }
        }
        if (most_match_index != nullptr) {
            return {true, *most_match_index};
        } else {
            return {false, IndexMeta()};
        }
    }

    bool is_col_belongs_to_index(const std::string &colName) {
        for (auto &index: indexes) {
            auto &cols = index.cols;
            for (auto it = cols.begin(); it != cols.end(); ++it) {
                if (it->name == colName) {
                    return true;
                }
            }
        }
        return false;
    }

    /* 根据字段名称集合获取索引元数据 */
    std::vector<IndexMeta>::iterator get_index_meta(const std::vector<std::string> &col_names) {
        for (auto index = indexes.begin(); index != indexes.end(); ++index) {
            if ((*index).col_num != col_names.size()) continue;
            auto &index_cols = (*index).cols;
            size_t i = 0;
            for (; i < col_names.size(); ++i) {
                if (index_cols[i].name.compare(col_names[i]) != 0)
                    break;
            }
            if (i == col_names.size()) return index;
        }
        throw IndexNotFoundError(name, col_names);
    }

    /* 根据字段名称获取字段元数据 */
    std::vector<ColMeta>::iterator get_col(const std::string &col_name) {
        auto pos = std::find_if(cols.begin(), cols.end(), [&](const ColMeta &col) { return col.name == col_name; });
        if (pos == cols.end()) {
            throw ColumnNotFoundError(col_name);
        }
        return pos;
    }

    friend std::ostream &operator<<(std::ostream &os, const TabMeta &tab) {
        os << tab.name << '\n' << tab.cols.size() << '\n';
        for (auto &col: tab.cols) {
            os << col << '\n';  // col是ColMeta类型，然后调用重载的ColMeta的操作符<<
        }
        os << tab.indexes.size() << "\n";
        for (auto &index: tab.indexes) {
            os << index << "\n";
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, TabMeta &tab) {
        size_t n;
        is >> tab.name >> n;
        for (size_t i = 0; i < n; i++) {
            ColMeta col;
            is >> col;
            tab.cols.push_back(col);
        }
        is >> n;
        for (size_t i = 0; i < n; ++i) {
            IndexMeta index;
            is >> index;
            tab.indexes.push_back(index);
        }
        return is;
    }

    std::vector<IndexMeta> get_indexes() const {
        return indexes;
    }
};

// 注意重载了操作符 << 和 >>，这需要更底层同样重载TabMeta、ColMeta的操作符 << 和 >>
/* 数据库元数据 */
class DbMeta {
    friend class SmManager;

private:
    std::string name_;                      // 数据库名称
    std::map<std::string, TabMeta> tabs_;   // 数据库中包含的表

public:
    // DbMeta(std::string name) : name_(name) {}

    /* 判断数据库中是否存在指定名称的表 */
    bool is_table(const std::string &tab_name) const { return tabs_.find(tab_name) != tabs_.end(); }

    void SetTabMeta(const std::string &tab_name, const TabMeta &meta) {
        tabs_[tab_name] = meta;
    }

    /* 获取指定名称表的元数据 */
    TabMeta &get_table(const std::string &tab_name) {
        auto pos = tabs_.find(tab_name);
        if (pos == tabs_.end()) {
            throw TableNotFoundError(tab_name);
        }

        return pos->second;
    }

    std::map<std::string, TabMeta> &get_tables() {
        return tabs_;
    }

    // 重载操作符 <<
    friend std::ostream &operator<<(std::ostream &os, const DbMeta &db_meta) {
        os << db_meta.name_ << '\n' << db_meta.tabs_.size() << '\n';
        for (auto &entry: db_meta.tabs_) {
            os << entry.second << '\n';
        }
        return os;
    }

    friend std::istream &operator>>(std::istream &is, DbMeta &db_meta) {
        size_t n;
        is >> db_meta.name_ >> n;
        for (size_t i = 0; i < n; i++) {
            TabMeta tab;
            is >> tab;
            db_meta.tabs_[tab.name] = tab;
        }
        return is;
    }
};