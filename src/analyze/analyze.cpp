/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "analyze.h"

#include <utility>
#include "optimizer/plan.h"
#include "optimizer/planner.h"
#include "portal.h"
#include "optimizer/optimizer.h"
#include "global_object/global_object.h"
#include  "execution/execution.h"

/**
 * @description: 分析器，进行语义分析和查询重写，需要检查不符合语义规定的部分
 * @param {shared_ptr<ast::TreeNode>} parse parser生成的结果集
 * @return {shared_ptr<Query>} Query 
 */
std::shared_ptr<Query> Analyze::do_analyze(std::shared_ptr<ast::TreeNode> parse, Context *context) {
    std::shared_ptr<Query> query = std::make_shared<Query>();

    if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(parse)) {
        query->tables = std::move(x->tabs);

        for (auto &table: query->tables) {
            if (!sm_manager_->db_.is_table(table)) {
                throw TableNotFoundError(table);
            }
        }

        std::vector<std::string> groupByColsSet;
        std::vector<std::shared_ptr<GroupByMete>> groupByMetes;

        for (const auto &groupByColsItem: x->group_by_cols) {
            for (const auto &item: groupByMetes) {
                if (item->tab_col.col_name == groupByColsItem->col->col_name) {
                    throw RMDBError("group by col can not repeat");
                }
            }

            TabCol groupByCol = {.tab_name = groupByColsItem->col->tab_name, .col_name = groupByColsItem->col->col_name};
            std::vector<HavingMete> havingMetes;

            for (auto &having: groupByColsItem->having) {
                if (having->lhs->ag_type == ast::SV_AGGREGATE_NULL) {
                    throw RMDBError("having col ag_type can not null");
                }

                TabCol havingCol = {.tab_name = having->lhs->tab_name, .col_name = having->lhs->col_name};
                AggregateMeta havingMeta = AggregateMeta(convert_sv_aggregate_op(having->lhs->ag_type), havingCol);
                Value value;

                if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(having->rhs)) {
                    value = convert_sv_value(rhs_val);
                } else {
                    throw RMDBError("having rhs is not value");
                }

                havingMetes.emplace_back(havingMeta, convert_sv_comp_op(having->op), value);
            }

            groupByMetes.push_back(std::make_shared<GroupByMete>(groupByCol, havingMetes));
            groupByColsSet.push_back(groupByColsItem->col->col_name);
        }

        query->group_by_cols = groupByMetes;

        int null_col_size = 0;
        std::vector<ColMeta> allColumns;
        get_all_cols(query->tables, allColumns);
        auto tabMeta = sm_manager_->db_.get_table(query->tables[0]);

        for (auto &selColumn: x->cols) {
            if (selColumn->ag_type == ast::SV_AGGREGATE_NULL) {
                if (selColumn->col_name.empty()) {
                    if (!groupByColsSet.empty() && groupByColsSet.size() != allColumns.size()) {
                        throw RMDBError("select list is not in group by clause");
                    }

                    for (auto &column: allColumns) {
                        TabCol selCol = {.tab_name = column.tab_name, .col_name = column.name};
                        query->cols.push_back(selCol);
                        query->aggregate_metas.emplace_back(convert_sv_aggregate_op(ast::SV_AGGREGATE_NULL), selCol);
                    }
                } else {
                    TabCol selCol = {.tab_name = selColumn->tab_name, .col_name = selColumn->col_name};
                    selCol = check_column(allColumns, selCol);
                    query->cols.push_back(selCol);
                    query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);

                    if (!groupByColsSet.empty()) {
                        if (std::find(groupByColsSet.begin(), groupByColsSet.end(), selColumn->col_name) ==
                            groupByColsSet.end()) {
                            throw RMDBError("col not in group by cols");
                        }
                    }
                }
                ++null_col_size;
            } else {
                x->has_ag = true;

                if (!selColumn->col_name.empty()) {
                    if ("*" != selColumn->col_name && !tabMeta.is_col(selColumn->col_name)) {
                        throw ColumnNotFoundError(selColumn->col_name);
                    }

                    auto clo = tabMeta.get_col(selColumn->col_name);
                    if (selColumn->ag_type == ast::SV_AGGREGATE_SUM && clo->type == TYPE_STRING) {
                        throw IncompatibleTypeError(coltype2str(clo->type), "string");
                    }
                }

                TabCol outputCol = {.tab_name = query->tables[0], .col_name = selColumn->as_name.empty()
                                                                              ? convert_sv_aggregate_to_str(
                                selColumn->ag_type) + "(" + selColumn->col_name + ")"
                                                                              : selColumn->as_name};

                TabCol selCol = {.tab_name = query->tables[0], .col_name = selColumn->col_name};
                query->cols.push_back(outputCol);
                query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);
            }
        }

        if (x->has_ag && groupByColsSet.empty() && null_col_size > 0) {
            throw RMDBError("select list is not in group by clause");
        }

        if (query->cols.empty()) {
            for (auto &column: allColumns) {
                TabCol selCol = {.tab_name = column.tab_name, .col_name = column.name};
                query->cols.push_back(selCol);
                query->aggregate_metas.emplace_back(convert_sv_aggregate_op(ast::SV_AGGREGATE_NULL), selCol);
            }
        }

        // 处理order by
        if (x->order.operator bool()) {
            TabCol tabCol = {.tab_name = x->order->col->tab_name, .col_name = x->order->col->col_name};
            query->sort_mete = std::make_shared<SortMete>(tabCol,
                                                          convert_sv_order_by_dir(x->order->orderby_dir));
        }

        get_clause(x->conds, query->conds, context, query->tables[0]);
        check_clause(query->tables, query->conds);

        std::map<std::string, TabCol> join_col;
        // 处理join
        if (query->tables.size() > 1) {
            for (const auto &cond : query->conds) {
                // cond.is_rhs is col
                if (!cond.is_rhs_val && !cond.is_rhs_in) {
                    // 处理 lhs_col
                    if (!cond.lhs_col.tab_name.empty()) {
                        join_col[cond.lhs_col.tab_name] = cond.lhs_col;
                    } else {
                        for (const auto &item: allColumns) {
                            if (item.name == cond.lhs_col.col_name) {
                                TabCol rhs_col = {.tab_name = item.tab_name, .col_name = cond.rhs_col.col_name};
                                join_col[item.tab_name] = rhs_col;
                            }
                        }
                    }
                    // 处理 rhs_col
                    if (!cond.rhs_col.tab_name.empty()) {
                        join_col[cond.rhs_col.tab_name] = cond.rhs_col;
                    } else {
                        for (const auto &item: allColumns) {
                            if (item.name == cond.rhs_col.col_name) {
                                TabCol rhs_col = {.tab_name = item.tab_name, .col_name = cond.rhs_col.col_name};
                                join_col[item.tab_name] = rhs_col;
                            }
                        }
                    }
                }
            }
        }
        query->join_col = join_col;

    } else if (auto x = std::dynamic_pointer_cast<ast::UpdateStmt>(parse)) {
        std::string tableName = x->tab_name;
        auto tableMeta = sm_manager_->db_.get_table(tableName);

        for (auto &clause: x->set_clauses) {
            SetClause setClause;

            TabCol selColumn = {.tab_name = tableName, .col_name = clause->col_name};
            setClause.lhs = selColumn;

            ColMeta columnMeta = *(tableMeta.get_col(selColumn.col_name));
            auto value = convert_sv_value(clause->val);

            if (columnMeta.type == TYPE_FLOAT && value.type == TYPE_INT) {
                Value tmp;
                tmp.set_float(value.int_val);
                value = tmp;
            }

            setClause.rhs = value;
            setClause.rhs.init_raw(columnMeta.type == TYPE_STRING ? columnMeta.len : sizeof(value.type));
            query->set_clauses.push_back(setClause);
        }

        get_clause(x->conds, query->conds, context, x->tab_name);
        check_clause({x->tab_name}, query->conds);

    } else if (auto x = std::dynamic_pointer_cast<ast::DeleteStmt>(parse)) {
        get_clause(x->conds, query->conds, context, x->tab_name);
        check_clause({x->tab_name}, query->conds);

    } else if (auto x = std::dynamic_pointer_cast<ast::InsertStmt>(parse)) {
        std::vector<ColMeta> columnMetas;
        get_all_cols({x->tab_name}, columnMetas);

        for (size_t i = 0; i < x->vals.size(); i++) {
            auto value = convert_sv_value(x->vals[i]);

            if (columnMetas[i].type == TYPE_FLOAT && value.type == TYPE_INT) {
                Value tmp;
                tmp.set_float(value.int_val);
                query->values.push_back(tmp);
            } else {
                query->values.push_back(value);
            }
        }
    } else if (auto x = std::dynamic_pointer_cast<ast::LoadStmt>(parse)) {
        std::string fileName = x->file_name;
        std::string tableName = x->tab_name;

        // 你可能需要在 Query 类中增加一些字段来存储 load 指令的信息
        query->file_url = fileName;
        query->tables.push_back(tableName);

        // 检查表是否存在
        bool isTable = sm_manager_->db_.is_table(tableName);
        if (!isTable) {
            throw TableNotFoundError(tableName);
        }
    }
    query->parse = std::move(parse);
    return query;
}

TabCol Analyze::check_column(const std::vector<ColMeta> &all_cols, TabCol target) {
    if (target.tab_name.empty()) {
        std::string tab_name;
        for (auto &col: all_cols) {
            if (col.name == target.col_name) {
                if (!tab_name.empty()) {
                    throw AmbiguousColumnError(target.col_name);
                }
                tab_name = col.tab_name;
            }
        }
        if (tab_name.empty()) {
            throw ColumnNotFoundError(target.col_name);
        }
        target.tab_name = tab_name;
    } else {
        bool col_found = false;
        for (auto &col: all_cols) {
            if (col.tab_name == target.tab_name && col.name == target.col_name) {
                col_found = true;
                break;
            }
        }
        if (!col_found) {
            throw ColumnNotFoundError(target.col_name);
        }
    }
    return target;
}

void Analyze::get_all_cols(const std::vector<std::string> &tab_names, std::vector<ColMeta> &all_cols) {
    for (auto &sel_tab_name: tab_names) {
        const auto &sel_tab_cols = sm_manager_->db_.get_table(sel_tab_name).cols;
        all_cols.insert(all_cols.end(), sel_tab_cols.begin(), sel_tab_cols.end());
    }
}

void Analyze::get_clause(const std::vector<std::shared_ptr<ast::BinaryExpr>> &sv_conds, std::vector<Condition> &conds,
                         Context *context, std::string tab_name) {
    conds.clear();
    for (auto &expr: sv_conds) {
        Condition cond;
        cond.lhs_col = {.tab_name = expr->lhs->tab_name, .col_name = expr->lhs->col_name};
        cond.op = convert_sv_comp_op(expr->op);
        if (expr->lhs->ag_type != ast::SV_AGGREGATE_NULL) {
            throw RMDBError("lhs col can not aggregate");
        }
        if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs)) {
            cond.is_rhs_val = true;
            cond.is_rhs_in = false;
            cond.rhs_val = convert_sv_value(rhs_val);
        } else if (auto rhs_col = std::dynamic_pointer_cast<ast::Col>(expr->rhs)) {
            if (rhs_col->ag_type != ast::SV_AGGREGATE_NULL) {
                throw RMDBError("rhs col can not aggregate");
            }
            cond.is_rhs_val = false;
            cond.is_rhs_in = false;
            cond.rhs_col = {.tab_name = rhs_col->tab_name, .col_name = rhs_col->col_name};
        } else if (auto rhs_col = std::dynamic_pointer_cast<ast::SubSelectStmt>(expr->rhs)) {
            if (rhs_col->cols.size() > 1) {
                throw RMDBError("sub query result to more");
            }
            auto l_tabMeta = sm_manager_->db_.get_table(tab_name);
            auto r_tabMeta = sm_manager_->db_.get_table(rhs_col->tabs[0]);
            auto l_type = l_tabMeta.get_col(cond.lhs_col.col_name)->type;
            auto r_type = r_tabMeta.get_col(rhs_col->cols[0]->col_name)->type;
            if (rhs_col->cols[0]->ag_type == ast::SV_AGGREGATE_NULL) {
                if (l_type != r_type) {
                    if (l_type == TYPE_STRING || r_type == TYPE_STRING) {
                        throw RMDBError("l_col_type is not r_col_type");
                    }
                }
            } else {
                if (rhs_col->cols[0]->ag_type == ast::SV_AGGREGATE_MAX ||
                    rhs_col->cols[0]->ag_type == ast::SV_AGGREGATE_MIN || expr->op == ast::SV_OP_IN) {
                    if (l_type != r_type) {
                        if (l_type == TYPE_STRING || r_type == TYPE_STRING) {
                            throw RMDBError("l_col_type is not r_col_type");
                        }
                    }
                } else if (l_type == TYPE_STRING) {
                    throw RMDBError("l_col_type is not r_col_type");
                }
            }

            if (expr->op == ast::SV_OP_IN) {
                std::string r_tab_name = rhs_col->tabs[0];
                std::string r_col_name = rhs_col->cols[0]->col_name;
                cond.is_rhs_val = false;
                cond.is_rhs_in = true;
                auto values = sub_query_execution(rhs_col, context);
                cond.rhs_in_vals = values;
                cond.rhs_col = {.tab_name = r_tab_name, .col_name = r_col_name};
            } else {
                cond.is_rhs_val = true;
                cond.is_rhs_in = false;
                auto values = sub_query_execution(rhs_col, context);
                if (values.size() > 1) {
                    throw RMDBError("result to more");
                }
                if (values.size() == 0) {
                    Value value = Value();
                    value.is_null = true;
                    cond.rhs_val = value;
                } else {
                    cond.rhs_val = values[0];
                }
            }

        } else if (auto rhs_col = std::dynamic_pointer_cast<ast::InOpValue>(expr->rhs)) {
            auto l_tabMeta = sm_manager_->db_.get_table(tab_name);
            auto l_type = l_tabMeta.get_col(cond.lhs_col.col_name)->type;

            cond.is_rhs_val = false;
            cond.is_rhs_in = true;
            std::vector<Value> values;
            for (auto &val: rhs_col->values) {
                if (l_type == TYPE_STRING) {
                    if (auto str_val = std::dynamic_pointer_cast<ast::StringLit>(val)) {
                        // Do nothing, it's valid
                    } else {
                        throw RMDBError("in string type not int or float error");
                    }
                } else {
                    if (auto str_val = std::dynamic_pointer_cast<ast::StringLit>(val)) {
                        throw RMDBError("in int or float type not str error");
                    }
                }
                values.push_back(convert_sv_value(val));
            }
            cond.rhs_in_vals = values;
            cond.rhs_col = {.tab_name = tab_name, .col_name = cond.lhs_col.col_name};
        }
        conds.push_back(cond);
    }
}

void Analyze::check_clause(const std::vector<std::string> &tab_names, std::vector<Condition> &conds) {
    std::vector<ColMeta> all_cols;
    get_all_cols(tab_names, all_cols);
    for (auto &cond: conds) {
        cond.lhs_col = check_column(all_cols, cond.lhs_col);
        if (!cond.is_rhs_val && !cond.is_rhs_in) {
            cond.rhs_col = check_column(all_cols, cond.rhs_col);
        }
        TabMeta &lhs_tab = sm_manager_->db_.get_table(cond.lhs_col.tab_name);
        auto lhs_col = lhs_tab.get_col(cond.lhs_col.col_name);
        ColType lhs_type = lhs_col->type;
        ColType rhs_type;
        if (cond.is_rhs_val) {
            if (lhs_col->type == TYPE_FLOAT && cond.rhs_val.type == TYPE_INT) {
                cond.rhs_val.set_float(cond.rhs_val.int_val);
            }
            if (cond.rhs_val.raw == nullptr) {
                cond.rhs_val.init_raw(lhs_col->len);
            }
            rhs_type = cond.rhs_val.type;
        } else if (!cond.is_rhs_in) {
            TabMeta &rhs_tab = sm_manager_->db_.get_table(cond.rhs_col.tab_name);
            auto rhs_col = rhs_tab.get_col(cond.rhs_col.col_name);
            rhs_type = rhs_col->type;

            if (lhs_type != rhs_type && !checkType(lhs_type, rhs_type)) {
                throw IncompatibleTypeError(coltype2str(lhs_type), coltype2str(rhs_type));
            }
        }
    }
}

Value Analyze::convert_sv_value(const std::shared_ptr<ast::Value> &sv_val) {
    Value val;
    if (auto int_lit = std::dynamic_pointer_cast<ast::IntLit>(sv_val)) {
        val.set_int(int_lit->val);
    } else if (auto float_lit = std::dynamic_pointer_cast<ast::FloatLit>(sv_val)) {
        val.set_float(float_lit->val);
    } else if (auto str_lit = std::dynamic_pointer_cast<ast::StringLit>(sv_val)) {
        val.set_str(str_lit->val);
    } else {
        throw InternalError("Unexpected sv value type");
    }
    return val;
}

CompOp Analyze::convert_sv_comp_op(ast::SvCompOp op) {
    std::map<ast::SvCompOp, CompOp> m = {
            {ast::SV_OP_EQ, OP_EQ},
            {ast::SV_OP_NE, OP_NE},
            {ast::SV_OP_LT, OP_LT},
            {ast::SV_OP_GT, OP_GT},
            {ast::SV_OP_LE, OP_LE},
            {ast::SV_OP_GE, OP_GE},
            {ast::SV_OP_IN, OP_IN},
    };
    return m.at(op);
}

AggregateOp Analyze::convert_sv_aggregate_op(ast::SvAggregateType type) {
    std::map<ast::SvAggregateType, AggregateOp> m = {
            {ast::SV_AGGREGATE_COUNT, AG_COUNT},
            {ast::SV_AGGREGATE_SUM,   AG_SUM},
            {ast::SV_AGGREGATE_MAX,   AG_MAX},
            {ast::SV_AGGREGATE_MIN,   AG_MIN},
            {ast::SV_AGGREGATE_NULL,  AG_NULL},
    };
    return m.at(type);
}

std::string Analyze::convert_sv_aggregate_to_str(ast::SvAggregateType type) {
    std::map<ast::SvAggregateType, std::string> m = {
            {ast::SV_AGGREGATE_COUNT, "COUNT"},
            {ast::SV_AGGREGATE_SUM,   "SUM"},
            {ast::SV_AGGREGATE_MAX,   "MAX"},
            {ast::SV_AGGREGATE_MIN,   "MIN"},
            {ast::SV_AGGREGATE_NULL,  ""},
    };
    return m.at(type);
}

bool Analyze::convert_sv_order_by_dir(ast::OrderByDir orderByDir) {
    switch (orderByDir) {
        case ast::OrderBy_DEFAULT:
            return true;
        case ast::OrderBy_ASC:
            return true;
        case ast::OrderBy_DESC:
            return false;
        default:
            throw RMDBError("order by dir error");
    }
}

std::vector<Value> Analyze::sub_query_execution(std::shared_ptr<ast::SubSelectStmt> sub_select_stmt, Context *context) {
    auto query = do_sub_query_analyze(std::move(sub_select_stmt), context);

    std::shared_ptr<Plan> plan = optimizer->plan_query(query, context);

    std::shared_ptr<PortalStmt> portalStmt = portal->start(plan, context);
    auto values = ql_manager->sub_select_from(std::move(portalStmt->root));
    return values;
}

std::shared_ptr<Query> Analyze::do_sub_query_analyze(std::shared_ptr<ast::SubSelectStmt> x, Context *context) {
    std::shared_ptr<Query> query = std::make_shared<Query>();

    query->tables = std::move(x->tabs);

    for (auto &table: query->tables) {
        if (!sm_manager_->db_.is_table(table)) {
            throw TableNotFoundError(table);
        }
    }

    std::vector<std::string> groupByColsSet;
    std::vector<std::shared_ptr<GroupByMete>> groupByMetes;

    for (const auto &groupByColsItem: x->group_by_cols) {
        TabCol groupByCol = {.tab_name = groupByColsItem->col->tab_name, .col_name = groupByColsItem->col->col_name};
        std::vector<HavingMete> havingMetes;

        for (auto &having: groupByColsItem->having) {
            if (having->lhs->ag_type == ast::SV_AGGREGATE_NULL) {
                throw RMDBError("having col ag_type can not null");
            }

            TabCol havingCol = {.tab_name = having->lhs->tab_name, .col_name = having->lhs->col_name};
            AggregateMeta havingMeta = AggregateMeta(convert_sv_aggregate_op(having->lhs->ag_type), havingCol);
            Value value;
            if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(having->rhs)) {
                value = convert_sv_value(rhs_val);
            } else {
                throw RMDBError("having rhs is not value");
            }
            auto havingMete = HavingMete(havingMeta, convert_sv_comp_op(having->op), value);
            havingMetes.push_back(havingMete);
        }

        groupByMetes.push_back(std::make_shared<GroupByMete>(groupByCol, havingMetes));
        groupByColsSet.push_back(groupByColsItem->col->col_name);
    }

    query->group_by_cols = groupByMetes;

    if (!x->cols.empty()) {
        for (const auto &selColumn: x->cols) {
            if (selColumn->ag_type == ast::SV_AGGREGATE_NULL) {
                TabCol selCol = {.tab_name = selColumn->tab_name, .col_name = selColumn->col_name};
                if (selCol.tab_name.empty()) {
                    selCol.tab_name = query->tables[0];
                }
                query->cols.push_back(selCol);
                query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);

                if (!groupByColsSet.empty()) {
                    if (std::find(groupByColsSet.begin(), groupByColsSet.end(), selColumn->col_name) ==
                        groupByColsSet.end()) {
                        throw RMDBError("col not in group by cols");
                    }
                }
            } else {
                x->has_ag = true;
                auto tabMeta = sm_manager_->db_.get_table(query->tables[0]);

                if (!selColumn->col_name.empty()) {
                    if ("*" != selColumn->col_name && !tabMeta.is_col(selColumn->col_name)) {
                        throw ColumnNotFoundError(selColumn->col_name);
                    }

                    auto clo = tabMeta.get_col(selColumn->col_name);
                    if (selColumn->ag_type != ast::SV_AGGREGATE_COUNT && clo->type == TYPE_STRING) {
                        throw IncompatibleTypeError(coltype2str(clo->type), "string");
                    }
                }

                TabCol outputCol = {.tab_name = query->tables[0], .col_name = selColumn->as_name.empty()
                                                                              ? convert_sv_aggregate_to_str(
                                selColumn->ag_type) + "(" + selColumn->col_name + ")"
                                                                              : selColumn->as_name};

                TabCol selCol = {.tab_name = query->tables[0], .col_name = selColumn->col_name};
                query->cols.push_back(outputCol);
                query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);
            }
        }
    } else {
        throw ColumnNotFoundError("col is null");
    }

    get_clause(x->conds, query->conds, context, query->tables[0]);
    check_clause(query->tables, query->conds);
    auto selectStmt = std::make_shared<ast::SelectStmt>(x->convert_to_select());
    query->parse = std::move(selectStmt);

    return query;
}
