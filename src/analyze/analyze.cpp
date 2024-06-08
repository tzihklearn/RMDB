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

/**
 * @description: 分析器，进行语义分析和查询重写，需要检查不符合语义规定的部分
 * @param {shared_ptr<ast::TreeNode>} parse parser生成的结果集
 * @return {shared_ptr<Query>} Query 
 */
std::shared_ptr<Query> Analyze::do_analyze(std::shared_ptr<ast::TreeNode> parse) {
    std::shared_ptr<Query> query = std::make_shared<Query>();
    if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(parse)) {
        // 处理表名
        query->tables = std::move(x->tabs);
        // 检查表是否存在
        for (auto &table: query->tables) {
            bool isTable = sm_manager_->db_.is_table(table);
            if (!isTable) {
                throw TableNotFoundError(table);
            }
        }

        std::vector<std::string> set;
        // 处理group by 字段
        if (x->group_by_col.operator bool()) {
            TabCol groupByCol = {.tab_name = x->group_by_col->col->tab_name, .col_name = x->group_by_col->col->col_name};
            std::vector<HavingMete> havingMetes;
            if (x->group_by_col->having.size() > 0) {

                for (auto &having: x->group_by_col->having) {
                    TabCol havingCol = {.tab_name = having->lhs->tab_name, .col_name = having->lhs->col_name};
                    AggregateMeta havingMeta = AggregateMeta(convert_sv_aggregate_op(having->lhs->ag_type), havingCol);
                    Value value;
                    if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(having->rhs)) {
                         value = convert_sv_value(rhs_val);
                    }
                    auto havingMete = HavingMete(havingMeta, convert_sv_comp_op(having->op), value);
                    havingMetes.push_back(havingMete);
                }
            }
            query->group_by_col = std::make_shared<GroupByMete>(groupByCol, havingMetes);
            set.push_back(x->group_by_col->col->col_name);
        }

        for (auto &selColumn: x->cols) {
            // 处理普通字段
            if (selColumn->ag_type == ast::SV_AGGREGATE_NULL) {
                TabCol selCol = {.tab_name = selColumn->tab_name, .col_name = selColumn->col_name};
                query->cols.push_back(selCol);
                query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);
                if (!set.empty()) {
                    auto find = std::find(set.begin(), set.end(), selColumn->col_name);
                    if (find == set.end()) {
                        throw RMDBError("col not in group by cols");
                    }
                }
            } else {
                x->has_ag = true;
            }
        }



        std::vector<ColMeta> allColumns;
        get_all_cols(query->tables, allColumns);
        if (!query->cols.empty()) {
            // infer table name from column name
            for (auto &selColumn: query->cols) {
                // 防止处理count(*)
                if (!selColumn.col_name.empty()) {
                    selColumn = check_column(allColumns, selColumn);  // 列元数据校验
                }
            }
        }

        // 用于检查聚合函数和列类型是否匹配，以及检查列是否存在
        auto tabMeta = sm_manager_->db_.get_table(query->tables[0]);
        // 处理投影列
        if (x->has_ag) {
            for (auto &selColumn: x->cols) {
                // 处理聚合函数
                if (selColumn->ag_type != ast::SV_AGGREGATE_NULL) {
                    if (!selColumn->col_name.empty()) {
                        // 检查列是否存在
                        if ("*" != selColumn->col_name && !tabMeta.is_col(selColumn->col_name)) {
                            throw ColumnNotFoundError(selColumn->col_name);
                        }
                        // 检查聚合函数和列类型是否匹配
                        auto clo = tabMeta.get_col(selColumn->col_name);
                        if (selColumn->ag_type != ast::SV_AGGREGATE_COUNT && clo->type == TYPE_STRING) {
                            throw IncompatibleTypeError(coltype2str(clo->type), "string");
                        }
                    }

                    TabCol outputCol = {.tab_name = query->tables[0], .col_name = selColumn->as_name};
                    TabCol selCol = {.tab_name = query->tables[0], .col_name = selColumn->col_name};
                    if (selColumn->ag_type == ast::SV_AGGREGATE_NULL) {
                        outputCol.col_name = selColumn->col_name;
                    }
                    query->cols.push_back(outputCol);
                    query->aggregate_metas.emplace_back(convert_sv_aggregate_op(selColumn->ag_type), selCol);
                }
            }
        }

        // 处理*，没有指定列名，选择所有列
        if (query->cols.empty()) {
            // select all columns
            for (auto &column: allColumns) {
                TabCol selColumn = {.tab_name = column.tab_name, .col_name = column.name};
                query->cols.push_back(selColumn);
            }
        }


//        } else {
//            // infer table name from column name
//            for (auto &selColumn: query->cols) {
//                // 防止处理count(*)
//                if (!selColumn.col_name.empty()) {
//                    selColumn = check_column(allColumns, selColumn);  // 列元数据校验
//                }
//            }
//        }
        //处理where条件
        get_clause(x->conds, query->conds);
        check_clause(query->tables, query->conds);
    } else if (auto x = std::dynamic_pointer_cast<ast::UpdateStmt>(parse)) {
        std::string tableName = x->tab_name;
        auto tableMeta = sm_manager_->db_.get_table(tableName);

        // 获取query->set_clauses
        for (auto &clause: x->set_clauses) {
            SetClause setClause;

            TabCol selColumn = {.tab_name = tableName, .col_name = clause->col_name};
            setClause.lhs = selColumn;

            ColMeta columnMeta = *(tableMeta.get_col(selColumn.col_name));
            auto value = convert_sv_value(clause->val);

            // 列类型是 float，新值类型是 int，需要做一个转换
            if (columnMeta.type == TYPE_FLOAT && value.type == TYPE_INT) {
                Value tmp;
                tmp.set_float(value.int_val);
                value = tmp;
            }

            setClause.rhs = value;
            switch (setClause.rhs.type) {
                case TYPE_INT:
                    setClause.rhs.init_raw(sizeof(int));
                    break;
                case TYPE_FLOAT:
                    setClause.rhs.init_raw(sizeof(float));
                    break;
                case TYPE_STRING:
                    setClause.rhs.init_raw(columnMeta.len);
                    break;
            }
            std::cout << setClause.rhs.raw->data << std::endl;
            query->set_clauses.push_back(setClause);
        }
        // 处理where条件
        get_clause(x->conds, query->conds);
        check_clause({x->tab_name}, query->conds);
    } else if (auto x = std::dynamic_pointer_cast<ast::DeleteStmt>(parse)) {
        //处理where条件
        get_clause(x->conds, query->conds);
        check_clause({x->tab_name}, query->conds);
    } else if (auto x = std::dynamic_pointer_cast<ast::InsertStmt>(parse)) {
        std::vector<ColMeta> columnMetas;
        get_all_cols({x->tab_name}, columnMetas);
        for (size_t i = 0; i < x->vals.size(); i++) {
            auto value = convert_sv_value(x->vals[i]);
            // 列与新值类型不匹配的话可以做个转换
            if (columnMetas[i].type == TYPE_FLOAT && value.type == TYPE_INT) {
                Value tmp;
                tmp.set_float(value.int_val);
                query->values.push_back(tmp);
            } else {
                query->values.push_back(value);
            }
        }
    } else {
        // do nothing
    }
    query->parse = std::move(parse);
    return query;
}

TabCol Analyze::check_column(const std::vector<ColMeta> &all_cols, TabCol target) {
    if (target.tab_name.empty()) {
        // Table name not specified, infer table name from column name
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
        /** TODO: Make sure target column exists */
        bool col_found = false;
        for (auto &col: all_cols) {
            if (col.tab_name == target.tab_name && col.name == target.col_name) {
                col_found = true;
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
        // 这里db_不能写成get_db(), 注意要传指针
        const auto &sel_tab_cols = sm_manager_->db_.get_table(sel_tab_name).cols;
        all_cols.insert(all_cols.end(), sel_tab_cols.begin(), sel_tab_cols.end());
    }
}

void Analyze::get_clause(const std::vector<std::shared_ptr<ast::BinaryExpr>> &sv_conds, std::vector<Condition> &conds) {
    conds.clear();
    for (auto &expr: sv_conds) {
        Condition cond;
        cond.lhs_col = {.tab_name = expr->lhs->tab_name, .col_name = expr->lhs->col_name};
        cond.op = convert_sv_comp_op(expr->op);
        if (auto rhs_val = std::dynamic_pointer_cast<ast::Value>(expr->rhs)) {
            cond.is_rhs_val = true;
            cond.rhs_val = convert_sv_value(rhs_val);
        } else if (auto rhs_col = std::dynamic_pointer_cast<ast::Col>(expr->rhs)) {
            cond.is_rhs_val = false;
            cond.rhs_col = {.tab_name = rhs_col->tab_name, .col_name = rhs_col->col_name};
        }
        conds.push_back(cond);
    }
}

void Analyze::check_clause(const std::vector<std::string> &tab_names, std::vector<Condition> &conds) {
    // auto all_cols = get_all_cols(tab_names);
    std::vector<ColMeta> all_cols;
    get_all_cols(tab_names, all_cols);
    // Get raw values in where clause
    for (auto &cond: conds) {
        // Infer table name from column name
        cond.lhs_col = check_column(all_cols, cond.lhs_col);
        if (!cond.is_rhs_val) {
            cond.rhs_col = check_column(all_cols, cond.rhs_col);
        }
        TabMeta &lhs_tab = sm_manager_->db_.get_table(cond.lhs_col.tab_name);
        auto lhs_col = lhs_tab.get_col(cond.lhs_col.col_name);
        ColType lhs_type = lhs_col->type;
        ColType rhs_type;
        if (cond.is_rhs_val) {
            // 列与新值类型不匹配的话可以做个转换
            if (lhs_col->type == TYPE_FLOAT && cond.rhs_val.type == TYPE_INT) {
                Value tmp;
                tmp.set_float(cond.rhs_val.int_val);
            }
            cond.rhs_val.init_raw(lhs_col->len);
            rhs_type = cond.rhs_val.type;
        } else {
            TabMeta &rhs_tab = sm_manager_->db_.get_table(cond.rhs_col.tab_name);
            auto rhs_col = rhs_tab.get_col(cond.rhs_col.col_name);
            rhs_type = rhs_col->type;
        }
        if (!checkType(lhs_type, rhs_type)) {
            throw IncompatibleTypeError(coltype2str(lhs_type), coltype2str(rhs_type));
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
    };
    return m.at(op);
}


AggregateOp Analyze::convert_sv_aggregate_op(ast::SvAggregateType type) {
    std::map<ast::SvAggregateType, AggregateOp> m = {
        {ast::SV_AGGREGATE_COUNT, AG_COUNT},
        {ast::SV_AGGREGATE_SUM, AG_SUM},
        {ast::SV_AGGREGATE_MAX, AG_MAX},
        {ast::SV_AGGREGATE_MIN, AG_MIN},
        {ast::SV_AGGREGATE_NULL, AG_NULL},
    };
    return m.at(type);
}


