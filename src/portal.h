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

#include <cerrno>
#include <cstring>
#include <string>
#include "optimizer/plan.h"
#include "execution/executor_abstract.h"
#include "execution/executor_nestedloop_join.h"
#include "execution/executor_projection.h"
#include "execution/executor_seq_scan.h"
#include "execution/executor_index_scan.h"
#include "execution/executor_update.h"
#include "execution/executor_insert.h"
#include "execution/executor_delete.h"
#include "execution/execution_sort.h"
#include "common/common.h"
#include "execution/execution_aggregation.h"
#include "execution/execution_merge_sort.h"
#include "execution/execution_load.h"

typedef enum portalTag {
    PORTAL_Invalid_Query = 0,
    PORTAL_ONE_SELECT,
    PORTAL_DML_WITHOUT_SELECT,
    PORTAL_MULTI_QUERY,
    PORTAL_CMD_UTILITY,
    PORTAL_LOAD
} portalTag;


struct PortalStmt {
    portalTag tag;

    std::vector<TabCol> sel_cols;
    std::unique_ptr<AbstractExecutor> root;
    std::shared_ptr<Plan> plan;

    PortalStmt(portalTag tag_, std::vector<TabCol> sel_cols_, std::unique_ptr<AbstractExecutor> root_,
               std::shared_ptr<Plan> plan_) :
            tag(tag_), sel_cols(std::move(sel_cols_)), root(std::move(root_)), plan(std::move(plan_)) {}
};

class Portal {
private:
    SmManager *sm_manager_;

public:
    Portal(SmManager *sm_manager) : sm_manager_(sm_manager) {}

    ~Portal() {}

    // 将查询执行计划转换成对应的算子树
    std::shared_ptr<PortalStmt> start(std::shared_ptr<Plan> plan, Context *context) {
        // 这里可以将select进行拆分，例如：一个select，带有return的select等
        if (auto x = std::dynamic_pointer_cast<OtherPlan>(plan)) {
            return std::make_shared<PortalStmt>(PORTAL_CMD_UTILITY, std::vector<TabCol>(),
                                                std::unique_ptr<AbstractExecutor>(), plan);
        } else if (auto x = std::dynamic_pointer_cast<SetKnobPlan>(plan)) {
            return std::make_shared<PortalStmt>(PORTAL_CMD_UTILITY, std::vector<TabCol>(),
                                                std::unique_ptr<AbstractExecutor>(), plan);
        } else if (auto x = std::dynamic_pointer_cast<DDLPlan>(plan)) {
            return std::make_shared<PortalStmt>(PORTAL_MULTI_QUERY, std::vector<TabCol>(),
                                                std::unique_ptr<AbstractExecutor>(), plan);
        } else if (auto x = std::dynamic_pointer_cast<DMLPlan>(plan)) {
            switch (x->tag) {
                case T_Select: {
                    std::shared_ptr<ProjectionPlan> p = std::dynamic_pointer_cast<ProjectionPlan>(x->subplan_);
                    std::unique_ptr<AbstractExecutor> root = convert_plan_executor(p, context);
                    return std::make_shared<PortalStmt>(PORTAL_ONE_SELECT, std::move(p->sel_cols_), std::move(root),
                                                        plan);
                }

                case T_Update: {
                    std::unique_ptr<AbstractExecutor> scan = convert_plan_executor(x->subplan_, context);
                    std::vector<Rid> rids;
                    for (scan->beginTuple(); !scan->is_end(); scan->nextTuple()) {
                        rids.push_back(scan->rid());
                    }
                    std::unique_ptr<AbstractExecutor> root = std::make_unique<UpdateExecutor>(sm_manager_,
                                                                                              x->tab_name_,
                                                                                              x->set_clauses_,
                                                                                              x->conds_, rids, context);
                    return std::make_shared<PortalStmt>(PORTAL_DML_WITHOUT_SELECT, std::vector<TabCol>(),
                                                        std::move(root), plan);
                }
                case T_Delete: {
                    std::unique_ptr<AbstractExecutor> scan = convert_plan_executor(x->subplan_, context);
                    std::vector<Rid> rids;
                    for (scan->beginTuple(); !scan->is_end(); scan->nextTuple()) {
                        rids.push_back(scan->rid());
                    }

                    std::unique_ptr<AbstractExecutor> root =
                            std::make_unique<DeleteExecutor>(sm_manager_, x->tab_name_, x->conds_, rids, context);

                    return std::make_shared<PortalStmt>(PORTAL_DML_WITHOUT_SELECT, std::vector<TabCol>(),
                                                        std::move(root), plan);
                }
                case T_Insert: {
                    std::unique_ptr<AbstractExecutor> root =
                            std::make_unique<InsertExecutor>(sm_manager_, x->tab_name_, x->values_, context);

                    return std::make_shared<PortalStmt>(PORTAL_DML_WITHOUT_SELECT, std::vector<TabCol>(),
                                                        std::move(root), plan);
                }
                case T_SvAggregate: {
                    // 转为SeqScanExecutor
                    std::unique_ptr<AbstractExecutor> scan = convert_plan_executor(x->subplan_, context);

                    // 进行扫描，获取所有的rid
                    std::vector<Rid> rids;
                    for (scan->beginTuple(); !scan->is_end(); scan->nextTuple()) {
                        rids.push_back(scan->rid());
                    }

                    std::unique_ptr<AbstractExecutor> root = std::make_unique<AggregationExecutor>(sm_manager_,
                                                                                                   x->tab_name_,
                                                                                                   x->conds_,
                                                                                                   x->aggregationMetas_,
                                                                                                   x->group_by_cols_,
                                                                                                   rids, context);
                    // 返回一个select的算子
                    return std::make_shared<PortalStmt>(PORTAL_ONE_SELECT, std::move(x->output_col_), std::move(root),
                                                        plan);
                }
                default:
                    throw InternalError("Unexpected field type");
                    break;
            }
        } else if (auto x = std::dynamic_pointer_cast<LoadPlan>(plan)) {
            // 处理load指令的逻辑
            // 这里假设LoadExecutor是你定义的执行器类来处理load指令
            std::unique_ptr<AbstractExecutor> root = std::make_unique<LoadExecutor>(sm_manager_, x->getFileName(),
                                                                                    x->getTableName(), context);
            // 返回适当的PortalStmt。这里使用PORTAL_LOAD作为Portal类型
            return std::make_shared<PortalStmt>(PORTAL_LOAD, std::vector<TabCol>(), std::move(root), plan);
        } else {
            throw InternalError("Unexpected field type");
        }
        return nullptr;
    }

    // 遍历算子树并执行算子生成执行结果
    void run(std::shared_ptr<PortalStmt> portal, QlManager *ql, txn_id_t *txn_id, Context *context) {
        switch (portal->tag) {
            case PORTAL_ONE_SELECT: {
                ql->select_from(std::move(portal->root), std::move(portal->sel_cols), context);
                break;
            }
            case PORTAL_DML_WITHOUT_SELECT: {
                ql->run_dml(std::move(portal->root));
                break;
            }
            case PORTAL_MULTI_QUERY: {
                ql->run_multi_query(portal->plan, context);
                break;
            }
            case PORTAL_CMD_UTILITY: {
                ql->run_cmd_utility(portal->plan, txn_id, context);
                break;
            }
            case PORTAL_LOAD: {
                portal->root->Next();
                break;
            }
            default: {
                throw InternalError("Unexpected field type");
            }
        }
    }

    // 清空资源
    void drop() {}

    std::unique_ptr<AbstractExecutor> convert_plan_executor(std::shared_ptr<Plan> plan, Context *context) {
        if (auto x = std::dynamic_pointer_cast<ProjectionPlan>(plan)) {
            return std::make_unique<ProjectionExecutor>(convert_plan_executor(x->subplan_, context),
                                                        x->sel_cols_);
        } else if (auto x = std::dynamic_pointer_cast<ScanPlan>(plan)) {
            if (x->tag == T_SeqScan) {
                // 顺序扫描
                return std::make_unique<SeqScanExecutor>(sm_manager_, x->tab_name_, x->conditions_, context);
            } else {
                // 索引扫描
                return std::make_unique<IndexScanExecutor>(sm_manager_, x->tab_name_, x->conditions_,
                                                           x->index_col_names_, x->index_meta_, context, x->is_sort_);
            }
        } else if (auto x = std::dynamic_pointer_cast<JoinPlan>(plan)) {
            std::unique_ptr<AbstractExecutor> left = convert_plan_executor(x->left_, context);
            std::unique_ptr<AbstractExecutor> right = convert_plan_executor(x->right_, context);
            std::unique_ptr<AbstractExecutor> join = std::make_unique<NestedLoopJoinExecutor>(
                    std::move(left),
                    std::move(right), std::move(x->conds_), x->is_reversal_join_, context, x->is_time_delay_);
            return join;
        } else if (auto x = std::dynamic_pointer_cast<SortPlan>(plan)) {
            if (context->js_->getSortMerge()) {
                context->js_->addEId();
                return std::make_unique<MergeSortExecutor>(convert_plan_executor(x->subplan_, context),
                                                           x->sel_col_, x->is_desc_, context, context->js_->getEId(), x->is_out_);
            }
            return std::make_unique<SortExecutor>(convert_plan_executor(x->subplan_, context),
                                                  x->sel_col_, x->is_desc_, context);
        }
        return nullptr;
    }
};