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

#include "parser/parser.h"
#include "system/sm.h"
#include "common/common.h"

class Query{
    public:
    std::shared_ptr<ast::TreeNode> parse;
    // TODO jointree
    // where条件
    std::vector<Condition> conds;
    // 投影列
    std::vector<TabCol> cols;
    std::vector<TabCol> output_cols;
    // 表名
    std::vector<std::string> tables;
    // update 的set 值
    std::vector<SetClause> set_clauses;
    //insert 的values值
    std::vector<Value> values;
    // 聚合函数meta
    std::vector<AggregateMeta> aggregate_metas;
    // 聚合函数的group by
    std::vector<std::shared_ptr<GroupByMete>> group_by_cols;
    // 排序
    std::shared_ptr<SortMete> sort_mete;
    // join_col
    std::map<std::string, TabCol> join_col;

    // 文件地址
    std::string file_url;

    Query()= default;

    // 自定义拷贝构造函数，进行深拷贝
    Query(const Query &query){
        parse = query.parse;
        conds = query.conds;
        cols = query.cols;
        tables = query.tables;
        set_clauses = query.set_clauses;
        values = query.values;
        aggregate_metas = query.aggregate_metas;
        group_by_cols = query.group_by_cols;
        sort_mete = query.sort_mete;
        join_col = query.join_col;
        file_url = query.file_url;
    }

};

class Analyze
{
private:
    SmManager *sm_manager_;
public:
    Analyze(SmManager *sm_manager) : sm_manager_(sm_manager){}
    ~Analyze(){}

    std::shared_ptr<Query> do_analyze(std::shared_ptr<ast::TreeNode> root, Context *context);

private:
    TabCol check_column(const std::vector<ColMeta> &all_cols, TabCol target);
    void get_all_cols(const std::vector<std::string> &tab_names, std::vector<ColMeta> &all_cols);
    void get_clause(const std::vector<std::shared_ptr<ast::BinaryExpr>> &sv_conds, std::vector<Condition> &conds, Context *context, std::string tab_name);
    void check_clause(const std::vector<std::string> &tab_names, std::vector<Condition> &conds);
    Value convert_sv_value(const std::shared_ptr<ast::Value> &sv_val);
    CompOp convert_sv_comp_op(ast::SvCompOp op);
    ArtOP convert_sv_art_op(ast::SvArtOp op);
    AggregateOp convert_sv_aggregate_op(ast::SvAggregateType type);
    std::string convert_sv_aggregate_to_str(ast::SvAggregateType type);
    bool convert_sv_order_by_dir(ast::OrderByDir orderByDir);
    std::vector<Value> sub_query_execution(std::shared_ptr<ast::SubSelectStmt> sub_select_stmt, Context *context);
    std::shared_ptr<Query> do_sub_query_analyze(std::shared_ptr<ast::SubSelectStmt> x, Context *context);
};