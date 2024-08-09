/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */
#include <ctime>

#include "execution_manager.h"

#include "record_printer.h"

const char *help_info = "Supported SQL syntax:\n"
                        "  command ;\n"
                        "command:\n"
                        "  CREATE TABLE table_name (column_name type [, column_name type ...])\n"
                        "  DROP TABLE table_name\n"
                        "  CREATE INDEX table_name (column_name)\n"
                        "  DROP INDEX table_name (column_name)\n"
                        "  INSERT INTO table_name VALUES (value [, value ...])\n"
                        "  DELETE FROM table_name [WHERE where_clause]\n"
                        "  UPDATE table_name SET column_name = value [, column_name = value ...] [WHERE where_clause]\n"
                        "  SELECT selector FROM table_name [WHERE where_clause]\n"
                        "type:\n"
                        "  {INT | FLOAT | CHAR(n)}\n"
                        "where_clause:\n"
                        "  condition [AND condition ...]\n"
                        "condition:\n"
                        "  column op {column | value}\n"
                        "column:\n"
                        "  [table_name.]column_name\n"
                        "op:\n"
                        "  {= | <> | < | > | <= | >=}\n"
                        "selector:\n"
                        "  {* | column [, column ...]}\n";

// 主要负责执行DDL语句
void QlManager::run_multi_query(std::shared_ptr<Plan> plan, Context *context) {
    if (auto x = std::dynamic_pointer_cast<DDLPlan>(plan)) {
        switch (x->tag) {
            case T_CreateTable: {
                sm_manager_->create_table(x->tab_name_, x->cols_, context);
                break;
            }
            case T_DropTable: {
                sm_manager_->drop_table(x->tab_name_, context);
                break;
            }
            case T_CreateIndex: {
//                sm_manager_->create_index(x->tab_name_, x->tab_col_names_, context);
                break;
            }
            case T_DropIndex: {
                sm_manager_->drop_index(x->tab_name_, x->tab_col_names_, context);
                break;
            }
            default:
                throw InternalError("Unexpected field type");
        }
    }
}

// 执行help; show tables; desc table; begin; commit; abort;语句
void QlManager::run_cmd_utility(std::shared_ptr<Plan> plan, txn_id_t *txn_id, Context *context) {
    if (auto x = std::dynamic_pointer_cast<OtherPlan>(plan)) {
        switch (x->tag) {
            case T_Help: {
                memcpy(context->data_send_ + *(context->offset_), help_info, strlen(help_info));
                *(context->offset_) = strlen(help_info);
                break;
            }
            case T_ShowTable: {
                sm_manager_->show_tables(context);
                break;
            }
            case T_DescTable: {
                sm_manager_->desc_table(x->tab_name_, context);
                break;
            }
                // 索引相关
            case T_ShowIndex: {
                sm_manager_->show_index(x->tab_name_, context);
                break;
            }
                // 事务相关
            case T_Transaction_Begin: {
                context->txn_->set_txn_mode(true);
                break;
            }
            case T_Transaction_Commit: {
                context->txn_ = txn_mgr_->get_transaction(*txn_id);
                txn_mgr_->commit(context->txn_, context->log_mgr_);
                break;
            }
            case T_Transaction_Rollback: {
                context->txn_ = txn_mgr_->get_transaction(*txn_id);
                txn_mgr_->abort(context->txn_, context->log_mgr_);
                break;
            }
            case T_Transaction_Abort: {
                context->txn_ = txn_mgr_->get_transaction(*txn_id);
                txn_mgr_->abort(context->txn_, context->log_mgr_);
                break;
            }
            case T_Static_Checkpoint: {
                // 静态检查点
                context->log_mgr_->static_checkpoint();
                break;
            }
            default:
                throw InternalError("Unexpected field type");
        }
    } else if (auto x = std::dynamic_pointer_cast<SetKnobPlan>(plan)) {
        switch (x -> tag) {
            case T_SetKnob:
                switch (x->set_knob_type_) {
                    case ast::EnableNestLoop :
//                        context->js_->setSortMerge(!x->bool_value_);
                        context->js_->setSortMerge(true);
                        break;
                    case ast::EnableSortMerge:
//                        context->js_->setSortMerge(x->bool_value_);
                        context->js_->setSortMerge(true);
                        break;
                }
                break;
            default:
                throw RMDBError("");
        }
    }
}

// 执行select语句，select语句的输出除了需要返回客户端外，还需要写入output.txt文件中
void QlManager::select_from(std::unique_ptr<AbstractExecutor> executorTreeRoot, std::vector<TabCol> sel_cols,
                            Context *context) {
    auto startTime = time(nullptr);
    std::vector<std::string> captions;
    captions.reserve(sel_cols.size());
    for (auto &sel_col: sel_cols) {
        captions.push_back(sel_col.col_name);
    }

    // Print header into buffer
    RecordPrinter rec_printer(sel_cols.size());
    rec_printer.print_separator(context);
    rec_printer.print_record(captions, context);
    rec_printer.print_separator(context);
    // print header into file
    std::fstream outfile;
    try {
        // 第一次是归并连接时，第二次
        outfile.open("output.txt", std::ios::out | std::ios::app);
        outfile << "|";
        for (const auto &caption: captions) {
            outfile << " " << caption << " |";
        }
        outfile << "\n";
    } catch (std::exception &e) {
        std::cerr << "execution_manager select_from show_index() only pingcas can do" << e.what() << std::endl;
    }
    outfile.flush();

    // Print records
    size_t num_rec = 0;
    // 执行query_plan
    for (executorTreeRoot->beginTuple(); !executorTreeRoot->is_end(); executorTreeRoot->nextTuple()) {
//        if (num_rec == 0) {
//            std::this_thread::sleep_for(std::chrono::seconds(100));
//        }
        auto Tuple = executorTreeRoot->Next();
        std::vector<std::string> columns;
        for (auto &col: executorTreeRoot->cols()) {
            std::string col_str;
            char *rec_buf = Tuple->data + col.offset;
            switch (col.type) {
                case TYPE_INT: {
                    col_str = std::to_string(*(int *) rec_buf);
                    break;
                }
                case TYPE_FLOAT: {
                    col_str = std::to_string(*(float *) rec_buf);
                    break;
                }
                case TYPE_STRING: {
                    col_str = std::string((char *) rec_buf, col.len);
                    col_str.resize(strlen(col_str.c_str()));
                    break;
                }
                default: {
                    throw InvalidTypeError();
                }
            }
            columns.push_back(col_str);
        }
        // print record into buffer
        rec_printer.print_record(columns, context);
        // print record into file
        outfile << "|";
        for (const auto &column: columns) {
            outfile << " " << column << " |";
        }
        outfile << "\n";
        outfile.flush();
        num_rec++;
    }
    outfile.close();
    executorTreeRoot->end_work();
    // Print footer into buffer
    rec_printer.print_separator(context);
    // Print record count into buffer
    RecordPrinter::print_record_count(num_rec, context);

    // 获取当前时间
    auto end_time = time(nullptr);
    std::cout << "Select using " << end_time - startTime << " seconds\n";
}

// 执行DML语句
void QlManager::run_dml(std::unique_ptr<AbstractExecutor> exec) {
    exec->Next();
}

std::vector<Value> QlManager::sub_select_from(std::unique_ptr<AbstractExecutor> executorTreeRoot) {
    std::vector<Value> values;
    for (executorTreeRoot->beginTuple(); !executorTreeRoot->is_end(); executorTreeRoot->nextTuple()) {
        auto Tuple = executorTreeRoot->Next();
        for (auto &col: executorTreeRoot->cols()) {
            Value value;

            char *rec_buf = Tuple->data + col.offset;
            switch (col.type) {
                case TYPE_INT: {
                    int int_value = *reinterpret_cast<int *>(rec_buf);
                    value.set_int(int_value);
                    value.init_raw(sizeof(int));
                    break;
                }
                case TYPE_FLOAT: {
                    float float_value = *reinterpret_cast<float *>(rec_buf);
                    value.set_float(float_value);
                    value.init_raw(sizeof(float));
                    break;
                }
                case TYPE_STRING: {
                    std::string col_str;
                    col_str = std::string((char *) rec_buf, col.len);
                    col_str.resize(strlen(col_str.c_str()));
//                    std::string str_value(rec_buf, col.len);
                    value.set_str(col_str);
                    value.init_raw(sizeof(col_str));
                    break;
                }
                default: {
                    throw InvalidTypeError();
                }
            }
            values.push_back(value);
        }

    }
    return values;
}