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
#include "parser/ast.h"

#include "parser/parser.h"

typedef enum PlanTag {
    T_Invalid = 1,
    T_Help,
    // 表相关
    T_ShowTable,
    T_DescTable,
    T_CreateTable,
    T_DropTable,
    // 索引相关
    T_ShowIndex,
    T_CreateIndex,
    T_DropIndex,
    // 操作数据相关
    T_Insert,
    T_Update,
    T_Delete,
    T_Select,
    // 事务
    T_Transaction_Begin,
    T_Transaction_Commit,
    T_Transaction_Abort,
    T_Transaction_Rollback,
    // 扫描数据
    T_SeqScan,
    T_IndexScan,
    // 循环
    T_NestLoop,
    // 排序
    T_SortMerge,
    // 执行计划
    T_Projection,
    // 其他
    T_SetKnob,
    // 聚合函数
    T_SvAggregate
} PlanTag;

// 查询执行计划
class Plan {
public:
    PlanTag tag;

    virtual ~Plan() = default;
};

class ScanPlan : public Plan {
public:
    std::string tab_name_;
    std::vector<ColMeta> cols_;
    std::vector<Condition> conditions_;
    size_t len_;
    std::vector<Condition> fedConditions;
    std::vector<std::string> index_col_names_;
    IndexMeta index_meta_;

    ScanPlan(PlanTag tag, SmManager *sm_manager, std::string tab_name, std::vector<Condition> conds,
             std::vector<std::string> index_col_names) {
        Plan::tag = tag;
        tab_name_ = std::move(tab_name);
        conditions_ = std::move(conds);
        TabMeta &tab = sm_manager->db_.get_table(tab_name_);
        cols_ = tab.cols;
        len_ = cols_.back().offset + cols_.back().len;
        fedConditions = conditions_;
        index_col_names_ = index_col_names;
    }

    ~ScanPlan() {}
};

class JoinPlan : public Plan {
public:
    JoinPlan(PlanTag tag, std::shared_ptr<Plan> left, std::shared_ptr<Plan> right, std::vector<Condition> conds) {
        Plan::tag = tag;
        left_ = std::move(left);
        right_ = std::move(right);
        conds_ = std::move(conds);
        type = INNER_JOIN;
    }

    ~JoinPlan() {}

    // 左节点
    std::shared_ptr<Plan> left_;
    // 右节点
    std::shared_ptr<Plan> right_;
    // 连接条件
    std::vector<Condition> conds_;
    // future TODO: 后续可以支持的连接类型
    JoinType type;
};

class ProjectionPlan : public Plan {
public:
    ProjectionPlan(PlanTag tag, std::shared_ptr<Plan> subplan, std::vector<TabCol> sel_cols) {
        Plan::tag = tag;
        subplan_ = std::move(subplan);
        sel_cols_ = std::move(sel_cols);
    }

    ~ProjectionPlan() {}

    std::shared_ptr<Plan> subplan_;
    std::vector<TabCol> sel_cols_;

};

class SortPlan : public Plan {
public:
    SortPlan(PlanTag tag, std::shared_ptr<Plan> subplan, TabCol sel_col, bool is_desc) {
        Plan::tag = tag;
        subplan_ = std::move(subplan);
        sel_col_ = sel_col;
        is_desc_ = is_desc;
    }

    ~SortPlan() {}

    std::shared_ptr<Plan> subplan_;
    TabCol sel_col_;
    bool is_desc_;

};

// dml语句，包括insert; delete; update; select语句　
class DMLPlan : public Plan {
public:
    DMLPlan(PlanTag tag, std::shared_ptr<Plan> subplan, std::string tab_name,
            std::vector<Value> values, std::vector<Condition> conds,
            std::vector<SetClause> set_clauses) {
        Plan::tag = tag;
        subplan_ = std::move(subplan);
        tab_name_ = std::move(tab_name);
        values_ = std::move(values);
        conds_ = std::move(conds);
        set_clauses_ = std::move(set_clauses);
    }

    ~DMLPlan() {}

    std::shared_ptr<Plan> subplan_;
    std::string tab_name_;
    std::vector<Value> values_;
    std::vector<Condition> conds_;
    std::vector<SetClause> set_clauses_;
    // 聚合函数Mete
    std::vector<AggregateMeta> aggregationMetas_;
    // group by
    std::shared_ptr<GroupByMete> group_by_col_;

    // 聚合函数输出列
    // 聚合函数支持as语法
    std::vector<TabCol> output_col_;
};
// ddl语句, 包括create/drop table; create/drop index;
class DDLPlan : public Plan {
public:
    DDLPlan(PlanTag tag, std::string tab_name, std::vector<std::string> col_names, std::vector<ColDef> cols) {
        Plan::tag = tag;
        tab_name_ = std::move(tab_name);
        cols_ = std::move(cols);
        tab_col_names_ = std::move(col_names);
    }

    ~DDLPlan() {}

    std::string tab_name_;
    std::vector<std::string> tab_col_names_;
    std::vector<ColDef> cols_;
};

// help; show tables; desc tables; begin; abort; commit; rollback语句对应的plan
class OtherPlan : public Plan {
public:
    OtherPlan(PlanTag tag, std::string tab_name) {
        Plan::tag = tag;
        tab_name_ = std::move(tab_name);
    }

    ~OtherPlan() {}

    std::string tab_name_;
};

// Set Knob Plan
class SetKnobPlan : public Plan {
public:
    SetKnobPlan(ast::SetKnobType knob_type, bool bool_value) {
        Plan::tag = T_SetKnob;
        set_knob_type_ = knob_type;
        bool_value_ = bool_value;
    }

    ast::SetKnobType set_knob_type_;
    bool bool_value_;
};

class plannerInfo {
public:
    std::shared_ptr<ast::SelectStmt> parse;
    std::vector<Condition> where_conds;
    std::vector<TabCol> sel_cols;
    std::shared_ptr<Plan> plan;
    std::vector<std::shared_ptr<Plan>> table_scan_executors;
    std::vector<SetClause> set_clauses;

    plannerInfo(std::shared_ptr<ast::SelectStmt> parse_) : parse(std::move(parse_)) {}

};
