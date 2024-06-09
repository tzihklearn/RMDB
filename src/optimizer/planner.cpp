/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "planner.h"

#include <memory>

#include "execution/executor_delete.h"
#include "execution/executor_index_scan.h"
#include "execution/executor_insert.h"
#include "execution/executor_nestedloop_join.h"
#include "execution/executor_projection.h"
#include "execution/executor_seq_scan.h"
#include "execution/executor_update.h"
#include "index/ix.h"
#include "record_printer.h"

// 根据已有索引调整 Where 条件顺序
std::pair<bool, IndexMeta> Planner::get_index_cols(const std::string &tab_name, std::vector<Condition> curr_conditions,
                                                   std::vector<std::string> &index_col_names) {
    index_col_names.clear();
    TabMeta &tab = sm_manager_->db_.get_table(tab_name);
    std::vector<IndexMeta> indexes = tab.get_indexes();

    // 构建索引条件映射表，记录每个条件匹配的索引列的位置
    std::unordered_map<size_t, std::vector<size_t>> conditionIndexMap;
    for (size_t i = 0; i < curr_conditions.size(); ++i) {
        const Condition &cond = curr_conditions[i];
        if (cond.is_rhs_val && cond.lhs_col.tab_name == tab_name) {
            for (size_t j = 0; j < indexes.size(); ++j) {
                if (indexes[j].contains_column(cond.lhs_col.col_name)) {
                    conditionIndexMap[j].push_back(i);
                    break;
                }
            }
        }
    }

    // 遍历索引，选择满足条件的最佳索引
    size_t bestIndex = 0;
    size_t bestMatchedCount = 0;
    for (const auto &entry: conditionIndexMap) {
        size_t indexId = entry.first;
        const std::vector<size_t> &matchedConditions = entry.second;
        if (matchedConditions.size() > bestMatchedCount &&
            matchedConditions.size() == indexes[indexId].get_column_count()) {
            bestIndex = indexId;
            bestMatchedCount = matchedConditions.size();
        }
    }

    // 根据最佳索引选择相应的条件顺序
    if (bestMatchedCount > 0) {
        const std::vector<size_t> &matchedConditions = conditionIndexMap[bestIndex];
        for (size_t i: matchedConditions) {
            index_col_names.push_back(curr_conditions[i].lhs_col.col_name);
        }
        return std::make_pair(true, indexes[bestIndex]);
    }

    return std::make_pair(false, IndexMeta());
}


/**
 * @brief 表算子条件谓词生成
 *
 * @param conditions 条件
 * @param tab_names 表名
 * @return std::vector<Condition>
 */
std::vector<Condition> pop_conds(std::vector<Condition> &conditions, std::string tab_names) {
    std::vector<Condition> solvedConditions;
    auto it = conditions.begin();
    while (it != conditions.end()) {
        // 将该条件添加到条件列表，如果该条件的左操作数的表名与当前表名相同
        if ((tab_names.compare(it->lhs_col.tab_name) == 0 && (it->is_rhs_val || it->is_rhs_in)) ||
            (it->lhs_col.tab_name.compare(it->rhs_col.tab_name) == 0)) {
            solvedConditions.emplace_back(std::move(*it));
            it = conditions.erase(it);
        } else {
            it++;
        }
    }
    return solvedConditions;
}

int push_conds(Condition *cond, std::shared_ptr<Plan> plan) {
    if (auto x = std::dynamic_pointer_cast<ScanPlan>(plan)) {
        if (x->tab_name_.compare(cond->lhs_col.tab_name) == 0) {
            return 1;
        } else if (x->tab_name_.compare(cond->rhs_col.tab_name) == 0) {
            return 2;
        } else {
            return 0;
        }
    } else if (auto x = std::dynamic_pointer_cast<JoinPlan>(plan)) {
        int left_res = push_conds(cond, x->left_);
        // 条件已经下推到左子节点
        if (left_res == 3) {
            return 3;
        }
        int right_res = push_conds(cond, x->right_);
        // 条件已经下推到右子节点
        if (right_res == 3) {
            return 3;
        }
        // 左子节点或右子节点有一个没有匹配到条件的列
        if (left_res == 0 || right_res == 0) {
            return left_res + right_res;
        }
        // 左子节点匹配到条件的右边
        if (left_res == 2) {
            // 需要将左右两边的条件变换位置
            std::map<CompOp, CompOp> swap_op = {
                    {OP_EQ, OP_EQ},
                    {OP_NE, OP_NE},
                    {OP_LT, OP_GT},
                    {OP_GT, OP_LT},
                    {OP_LE, OP_GE},
                    {OP_GE, OP_LE},
                    {OP_IN, OP_IN},
            };
            std::swap(cond->lhs_col, cond->rhs_col);
            cond->op = swap_op.at(cond->op);
        }
        x->conds_.emplace_back(std::move(*cond));
        return 3;
    }
    return false;
}

std::shared_ptr<Plan> pop_scan(int *scantbl, std::string table, std::vector<std::string> &joined_tables,
                               std::vector<std::shared_ptr<Plan>> plans) {
    for (size_t i = 0; i < plans.size(); i++) {
        auto x = std::dynamic_pointer_cast<ScanPlan>(plans[i]);
        if (x->tab_name_.compare(table) == 0) {
            scantbl[i] = 1;
            joined_tables.emplace_back(x->tab_name_);
            return plans[i];
        }
    }
    return nullptr;
}


std::shared_ptr<Query> Planner::logical_optimization(std::shared_ptr<Query> query, Context *context) {
    //TODO 实现逻辑优化规则

    return query;
}

std::shared_ptr<Plan> Planner::physical_optimization(std::shared_ptr<Query> query, Context *context) {
    std::shared_ptr<Plan> plan = make_one_rel(query);
    // 其他物理优化

    plan = generate_sort_plan(query, std::move(plan));

    return plan;
}


std::shared_ptr<Plan> Planner::make_one_rel(std::shared_ptr<Query> query) {
    auto x = std::dynamic_pointer_cast<ast::SelectStmt>(query->parse);
    std::vector<std::string> tables = query->tables;
    std::vector<std::shared_ptr<Plan>> table_scan_executors(tables.size());
    for (size_t i = 0; i < tables.size(); i++) {
        auto curr_conds = pop_conds(query->conds, tables[i]);
        std::vector<std::string> index_col_names;
        auto [index_exist, index_meta] = get_index_cols(tables[i], curr_conds, index_col_names);
        if (!index_exist) {
            index_col_names.clear();
            table_scan_executors[i] =
                    std::make_shared<ScanPlan>(T_SeqScan, sm_manager_, tables[i], curr_conds, index_col_names);
        } else {
            auto index_scan = std::make_shared<ScanPlan>(T_IndexScan, sm_manager_, tables[i], curr_conds,
                                                         index_col_names);
            // 将找到的最匹配的索引赋给index_scan plan
            index_scan->index_meta_ = index_meta;
            table_scan_executors[i] = index_scan;
        }
    }
    // 只有一个表，不需要join。
    if (tables.size() == 1) {
        return table_scan_executors[0];
    }
    // 获取where条件
    auto conditions = std::move(query->conds);
    std::shared_ptr<Plan> table_join_executors;

    int scanTable[tables.size()];
    for (size_t i = 0; i < tables.size(); i++) {
        scanTable[i] = -1;
    }
    // 假设在ast中已经添加了jointree，这里需要修改的逻辑是，先处理jointree，然后再考虑剩下的部分
    if (conditions.size() >= 1) {
        // 有连接条件
        // 根据连接条件，生成第一层join
        std::vector<std::string> joined_tables(tables.size());
        auto it = conditions.begin();
        while (it != conditions.end()) {
            std::shared_ptr<Plan> left, right;
            left = pop_scan(scanTable, it->lhs_col.tab_name, joined_tables, table_scan_executors);
            right = pop_scan(scanTable, it->rhs_col.tab_name, joined_tables, table_scan_executors);
            std::vector<Condition> join_conds{*it};
            //建立join
            table_join_executors = std::make_shared<JoinPlan>(T_NestLoop, std::move(left), std::move(right),
                                                              join_conds);
            it = conditions.erase(it);
            break;
        }
        // 根据连接条件，生成第2-n层join
        it = conditions.begin();
        while (it != conditions.end()) {
            std::shared_ptr<Plan> left_need_to_join_executors = nullptr;
            std::shared_ptr<Plan> right_need_to_join_executors = nullptr;
            bool isNeedReverse = false;
            if (std::find(joined_tables.begin(), joined_tables.end(), it->lhs_col.tab_name) == joined_tables.end()) {
                left_need_to_join_executors = pop_scan(scanTable, it->lhs_col.tab_name, joined_tables,
                                                       table_scan_executors);
            }
            if (std::find(joined_tables.begin(), joined_tables.end(), it->rhs_col.tab_name) == joined_tables.end()) {
                right_need_to_join_executors = pop_scan(scanTable, it->rhs_col.tab_name, joined_tables,
                                                        table_scan_executors);
                isNeedReverse = true;
            }

            if (left_need_to_join_executors != nullptr && right_need_to_join_executors != nullptr) {
                std::vector<Condition> join_conds{*it};
                std::shared_ptr<Plan> temp_join_executors = std::make_shared<JoinPlan>(T_NestLoop,
                                                                                       std::move(
                                                                                               left_need_to_join_executors),
                                                                                       std::move(
                                                                                               right_need_to_join_executors),
                                                                                       join_conds);
                table_join_executors = std::make_shared<JoinPlan>(T_NestLoop, std::move(temp_join_executors),
                                                                  std::move(table_join_executors),
                                                                  std::vector<Condition>());
            } else if (left_need_to_join_executors != nullptr || right_need_to_join_executors != nullptr) {
                if (isNeedReverse) {
                    std::map<CompOp, CompOp> swap_op = {
                            {OP_EQ, OP_EQ},
                            {OP_NE, OP_NE},
                            {OP_LT, OP_GT},
                            {OP_GT, OP_LT},
                            {OP_LE, OP_GE},
                            {OP_GE, OP_LE},
                            {OP_IN, OP_IN},
                    };
                    std::swap(it->lhs_col, it->rhs_col);
                    it->op = swap_op.at(it->op);
                    left_need_to_join_executors = std::move(right_need_to_join_executors);
                }
                std::vector<Condition> join_conds{*it};
                table_join_executors = std::make_shared<JoinPlan>(T_NestLoop, std::move(left_need_to_join_executors),
                                                                  std::move(table_join_executors), join_conds);
            } else {
                push_conds(std::move(&(*it)), table_join_executors);
            }
            it = conditions.erase(it);
        }
    } else {
        table_join_executors = table_scan_executors[0];
        scanTable[0] = 1;
    }

    //连接剩余表
    for (size_t i = 0; i < tables.size(); i++) {
        if (scanTable[i] == -1) {
            table_join_executors = std::make_shared<JoinPlan>(T_NestLoop, std::move(table_scan_executors[i]),
                                                              std::move(table_join_executors),
                                                              std::vector<Condition>());
        }
    }

    return table_join_executors;

}


std::shared_ptr<Plan> Planner::generate_sort_plan(std::shared_ptr<Query> query, std::shared_ptr<Plan> plan) {
    auto x = std::dynamic_pointer_cast<ast::SelectStmt>(query->parse);
    if (!x->has_sort) {
        return plan;
    }
    std::vector<std::string> tables = query->tables;
    std::vector<ColMeta> all_cols;
    for (auto &sel_tab_name: tables) {
        // 这里db_不能写成get_db(), 注意要传指针
        const auto &sel_tab_cols = sm_manager_->db_.get_table(sel_tab_name).cols;
        all_cols.insert(all_cols.end(), sel_tab_cols.begin(), sel_tab_cols.end());
    }
    TabCol sel_col;
    for (auto &col: all_cols) {
        if (col.name.compare(x->order->cols->col_name) == 0)
            sel_col = {.tab_name = col.tab_name, .col_name = col.name};
    }
    return std::make_shared<SortPlan>(T_SortMerge, std::move(plan), sel_col,
                                      x->order->orderby_dir == ast::OrderBy_DESC);
}


/**
 * @brief select plan 生成
 *
 * @param sel_cols select plan 选取的列
 * @param tab_names select plan 目标的表
 * @param conds select plan 选取条件
 */
std::shared_ptr<Plan> Planner::generate_select_plan(std::shared_ptr<Query> query, Context *context) {
    //逻辑优化
    query = logical_optimization(std::move(query), context);

    //物理优化
    auto sel_cols = query->cols;
    std::shared_ptr<Plan> plannerRoot = physical_optimization(query, context);
    plannerRoot = std::make_shared<ProjectionPlan>(T_Projection, std::move(plannerRoot),
                                                   std::move(sel_cols));

    return plannerRoot;
}

// 生成DDL语句和DML语句的查询执行计划
std::shared_ptr<Plan> Planner::do_planner(std::shared_ptr<Query> query, Context *context) {
    std::shared_ptr<Plan> plannerRoot;
    if (auto x = std::dynamic_pointer_cast<ast::CreateTable>(query->parse)) {
        // create table;
        std::vector<ColDef> col_defs;
        for (auto &field: x->fields) {
            if (auto sv_col_def = std::dynamic_pointer_cast<ast::ColDef>(field)) {
                ColDef col_def = {.name = sv_col_def->col_name,
                        .type = interp_sv_type(sv_col_def->type_len->type),
                        .len = sv_col_def->type_len->len};
                col_defs.push_back(col_def);
            } else {
                throw InternalError("Unexpected field type");
            }
        }
        plannerRoot = std::make_shared<DDLPlan>(T_CreateTable, x->tab_name, std::vector<std::string>(), col_defs);
    } else if (auto x = std::dynamic_pointer_cast<ast::DropTable>(query->parse)) {
        // drop table;
        plannerRoot = std::make_shared<DDLPlan>(T_DropTable, x->tab_name, std::vector<std::string>(),
                                                std::vector<ColDef>());
    } else if (auto x = std::dynamic_pointer_cast<ast::CreateIndex>(query->parse)) {
        // create index;
        plannerRoot = std::make_shared<DDLPlan>(T_CreateIndex, x->tab_name, x->col_names, std::vector<ColDef>());
    } else if (auto x = std::dynamic_pointer_cast<ast::DropIndex>(query->parse)) {
        // drop index
        plannerRoot = std::make_shared<DDLPlan>(T_DropIndex, x->tab_name, x->col_names, std::vector<ColDef>());
    } else if (auto x = std::dynamic_pointer_cast<ast::InsertStmt>(query->parse)) {
        // insert;
        plannerRoot = std::make_shared<DMLPlan>(T_Insert, std::shared_ptr<Plan>(), x->tab_name,
                                                query->values, std::vector<Condition>(), std::vector<SetClause>());
    } else if (auto x = std::dynamic_pointer_cast<ast::DeleteStmt>(query->parse)) {
        // delete;
        // 生成表扫描方式
        std::shared_ptr<Plan> table_scan_executors;
        // 只有一张表，不需要进行物理优化了
        std::vector<std::string> index_col_names;
        auto [index_exist, index_meta] = get_index_cols(x->tab_name, query->conds, index_col_names);
        if (!index_exist) {  // 该表没有索引
            index_col_names.clear();
            table_scan_executors =
                    std::make_shared<ScanPlan>(T_SeqScan, sm_manager_, x->tab_name, query->conds, index_col_names);
        } else {  // 存在索引
            auto index_scan = std::make_shared<ScanPlan>(T_IndexScan, sm_manager_, x->tab_name, query->conds,
                                                         index_col_names);
            // 将找到的最匹配的索引赋给index_scan plan
            index_scan->index_meta_ = index_meta;
            table_scan_executors = index_scan;
        }

        plannerRoot = std::make_shared<DMLPlan>(T_Delete, table_scan_executors, x->tab_name,
                                                std::vector<Value>(), query->conds, std::vector<SetClause>());
    } else if (auto x = std::dynamic_pointer_cast<ast::UpdateStmt>(query->parse)) {
        // update;
        // 生成表扫描方式
        std::shared_ptr<Plan> table_scan_executors;
        // 只有一张表，不需要进行物理优化了
        std::vector<std::string> index_col_names;
        auto [index_exist, index_meta] = get_index_cols(x->tab_name, query->conds, index_col_names);
        if (!index_exist) {  // 该表没有索引
            index_col_names.clear();
            table_scan_executors =
                    std::make_shared<ScanPlan>(T_SeqScan, sm_manager_, x->tab_name, query->conds, index_col_names);
        } else {  // 存在索引
            auto index_scan = std::make_shared<ScanPlan>(T_IndexScan, sm_manager_, x->tab_name, query->conds,
                                                         index_col_names);
            // 将找到的最匹配的索引赋给index_scan plan
            index_scan->index_meta_ = index_meta;
            table_scan_executors = index_scan;
        }
        plannerRoot = std::make_shared<DMLPlan>(T_Update, table_scan_executors, x->tab_name,
                                                std::vector<Value>(), query->conds,
                                                query->set_clauses);
    } else if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(query->parse)) {

        if (x->has_ag) {
            // 聚合函数query转plan

            // 扫描表方式
            std::shared_ptr<Plan> table_scan_plan;

            // TODO: 未处理索引
            // 设置扫描表的执行计划
            table_scan_plan = std::make_shared<ScanPlan>(T_SeqScan, sm_manager_, query->tables[0], query->conds, std::vector<std::string>());

            // 设置聚合函数的执行计划，将表扫描的执行计划作为子计划
            auto aggregatePlan = std::make_shared<DMLPlan>(T_SvAggregate, table_scan_plan,  query->tables[0], std::vector<Value>() , query->conds, std::vector<SetClause>());
            aggregatePlan->aggregationMetas_ = query->aggregate_metas;
            aggregatePlan->group_by_col_ = query->group_by_col;

            aggregatePlan->output_col_ = query->cols;
            return aggregatePlan;
        } else {
            std::shared_ptr<plannerInfo> root = std::make_shared<plannerInfo>(x);
            // 生成select语句的查询执行计划
            std::shared_ptr<Plan> projection = generate_select_plan(std::move(query), context);
            plannerRoot = std::make_shared<DMLPlan>(T_Select, projection, std::string(), std::vector<Value>(),
                                                    std::vector<Condition>(), std::vector<SetClause>());
        }
    } else {
        throw InternalError("Unexpected AST root");
    }
    return plannerRoot;
}