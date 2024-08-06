#pragma once

#include <future>
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"

class LoadExecutor : public AbstractExecutor {
private:
    TabMeta tab_;
    std::string file_name_;
    std::string table_name_;
    SmManager *sm_manager_;
    RmFileHandle *fh_;
    Rid rid_;
    std::string line_;

public:
    LoadExecutor(SmManager *sm_manager, const std::string &file_name, const std::string &table_name, Context *context) {
        file_name_ = file_name;
        table_name_ = table_name;
        sm_manager_ = sm_manager;
        fh_ = sm_manager->fhs_.at(table_name).get();
        tab_ = sm_manager_->db_.get_table(table_name);
        context_ = context;
    };

    // 开始执行
    std::unique_ptr<RmRecord> Next() override {
//        std::ifstream file(file_name_);
//        if (!file.is_open()) {
//            throw InternalError("Cannot open file: " + file_name_);
//        }
//        // 忽略第一行
//        std::string header;
//        std::getline(file, header);
//
//        std::string line;
//        while (std::getline(file, line)) {
//            // 解析CSV
//            std::vector<Value> values = parseCSVLine(line);
//            line_ = line;
//
//            // 插入数据到表中
//            insertIntoTable(table_name_, values);
//        }
//        std::cout << "load insert complete!" << std::endl;
//        return nullptr;
        // 使用std::async启动异步任务
        std::future<std::unique_ptr<RmRecord>> result = std::async(std::launch::async, [this]() -> std::unique_ptr<RmRecord> {
            std::ifstream file(file_name_);
            if (!file.is_open()) {
                char buffer[1024];
                // 使用getcwd获取当前工作目录
                if (getcwd(buffer, sizeof(buffer)) != nullptr) {
                    // 打印当前工作目录的路径
                    std::cout << "当前工作目录是: " << buffer << std::endl;
                } else {
                    std::cerr << "获取当前工作目录失败" << std::endl;
                }
                throw InternalError("Cannot open file: " + file_name_);
            }
            // 忽略第一行
            std::string header;
            std::getline(file, header);

            std::string line;
            while (std::getline(file, line)) {
                // 解析CSV
                std::vector<Value> values = parseCSVLine(line);
                line_ = line;

                // 插入数据到表中
                insertIntoTable(table_name_, values);
            }
            std::cout << "load insert complete!" << std::endl;
            return nullptr;
        });
        std::cout << "load ok!" << std::endl;

        // 返回一个空的std::unique_ptr，因为实际的返回值将在异步任务中处理
        return nullptr;
    }


    // 解析CSV行，返回值列表
    std::vector<Value> parseCSVLine(const std::string &line) {
        std::vector<Value> values;
        std::stringstream ss(line);
        std::string item;

        int columnIndex = 0;
        std::vector<ColType> expectedTypes;
        for (auto & col : tab_.cols) {
            expectedTypes.push_back(col.type);
        }
        int i = 0;
        while (std::getline(ss, item, ',')) {  // Assuming ',' as delimiter
            Value val;

            if (columnIndex >= expectedTypes.size()) {
                throw std::runtime_error("More columns in the CSV line than expected types provided.");
            }

            switch (expectedTypes[columnIndex]) {
                case TYPE_INT:
                    val.set_int(std::stoi(item));
                    break;
                case TYPE_FLOAT:
                    val.set_float(std::stof(item));
                    break;
                case TYPE_STRING:
                    val.set_str(item);
                    break;
                default:
                    throw std::runtime_error("Unknown column type");
            }

            values.push_back(val);
            columnIndex++;

            ++i;
            if (i >= 1000) {
                break;
            }
        }

        if (columnIndex != expectedTypes.size()) {
            throw std::runtime_error("Fewer columns in the CSV line than expected types provided.");
        }

        return values;
    }

    // 将数据插入到表中
    void insertIntoTable(const std::string &table_name, const std::vector<Value> &values) {
        // 使用数据库系统的API或方法将数据插入到表中
//        RmRecord rec(fh_->get_file_hdr().record_size);
//        for (size_t i = 0; i < values.size(); i++) {
//            auto &col = tab_.cols[i];
//            auto &val = (Value &) values[i];
//            if (col.type != val.type) {
//                throw IncompatibleTypeError(colType2str(col.type), colType2str(val.type));
//            }
//            val.init_raw(col.len);
//            memcpy(rec.data + col.offset, val.raw->data, col.len);
//        }
//        fh_->insert_record(rec.data, context_);
        //insert
        InsertExecutor insertExecutor(sm_manager_, table_name_, values, context_);
        insertExecutor.Next();
//        std::cout << "insert :" << line_ << std::endl;
    }

    Rid &rid() override { return rid_; }
};
