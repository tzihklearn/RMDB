/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "sm_manager.h"

#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

#include "index/ix.h"
#include "record/rm.h"
#include "record_printer.h"

/**
 * @description: 判断是否为一个文件夹
 * @return {bool} 返回是否为一个文件夹
 * @param {string&} db_name 数据库文件名称，与文件夹同名
 */
bool SmManager::is_dir(const std::string &db_name) {
    struct stat st;
    return stat(db_name.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

/**
 * @description: 创建数据库，所有的数据库相关文件都放在数据库同名文件夹下
 * @param {string&} db_name 数据库名称
 */
void SmManager::create_db(const std::string &db_name) {
    if (is_dir(db_name)) {
        throw DatabaseExistsError(db_name);
    }
    //为数据库创建一个子目录
    std::string cmd = "mkdir " + db_name;
    if (system(cmd.c_str()) < 0) {  // 创建一个名为db_name的目录
        throw UnixError();
    }
    if (chdir(db_name.c_str()) < 0) {  // 进入名为db_name的目录
        throw UnixError();
    }
    //创建系统目录
    auto *new_db = new DbMeta();
    new_db->name_ = db_name;

    // 注意，此处ofstream会在当前目录创建(如果没有此文件先创建)和打开一个名为DB_META_NAME的文件
    std::ofstream ofs(DB_META_NAME);

    // 将new_db中的信息，按照定义好的operator<<操作符，写入到ofs打开的DB_META_NAME文件中
    ofs << *new_db;  // 注意：此处重载了操作符<<

    delete new_db;

    // 创建日志文件
    disk_manager_->create_file(LOG_FILE_NAME);

    // 回到根目录
    if (chdir("..") < 0) {
        throw UnixError();
    }
}

/**
 * @description: 删除数据库，同时需要清空相关文件以及数据库同名文件夹
 * @param {string&} db_name 数据库名称，与文件夹同名
 */
void SmManager::drop_db(const std::string &db_name) {
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    std::string cmd = "rm -r " + db_name;
    if (system(cmd.c_str()) < 0) {
        throw UnixError();
    }
}

/**
 * @description: 打开数据库，找到数据库对应的文件夹，并加载数据库元数据和相关文件
 * @param {string&} db_name 数据库名称，与文件夹同名
 */
void SmManager::open_db(const std::string &db_name) {
    // 1. 找到对应文件夹，并进入该目录
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    if (chdir(db_name.c_str()) < 0) {  // 进入名为db_name的目录
        throw UnixError();
    }
    // 2.加载DbMeta和fhs_, ihs_?
    std::ifstream ifs(DB_META_NAME);
    ifs >> db_;
    ifs.close();
//    for (auto &[table_name, table_meta]: db_.tabs_) {
//        fhs_.emplace(table_name, rm_manager_->open_file(table_name));
//    }
    //加载当前数据库中每张表的数据文件和每个索引的文件
    for (auto & tab : db_.tabs_) {
        fhs_.emplace(tab.first, rm_manager_->open_file(tab.first));
        for (const auto &index: tab.second.indexes) {
            ihs_.emplace(ix_manager_->get_index_name(tab.first, index.cols), ix_manager_->open_index(tab.first, index.cols));
        }
//        //TODO:为什么是去打开字段信息而不是索引信息呢？
//        if (!tab.second.indexes.empty()) {
//            ihs_.emplace(ix_manager_->get_index_name(tab.first, tab.second.cols), ix_manager_->open_index(tab.first, tab.second.cols));
//        }
    }

}

/**
 * @description: 把数据库相关的元数据刷入磁盘中
 */
void SmManager::flush_meta() {
    // 默认清空文件
    std::ofstream ofs(DB_META_NAME);
    ofs << db_;
}

/**
 * @description: 关闭数据库并把数据落盘
 */
void SmManager::close_db() {
    //
    for (auto &[tab_name, tabfilehandle]: fhs_) {
        rm_manager_->close_file(&(*tabfilehandle));
        fhs_.erase(tab_name);
    }
    flush_meta();
    db_.name_.clear();
    db_.tabs_.clear();
}

/**
 * @description: 显示所有的表,通过测试需要将其结果写入到output.txt,详情看题目文档
 * @param {Context*} context
 */
void SmManager::show_tables(Context *context) {
    try {
        std::fstream outfile;
        if (context->write_out_){
            outfile.open("output.txt", std::ios::out | std::ios::app);
        } else {
            outfile.open("/dev/null", std::ios::out | std::ios::app);
            outfile.rdbuf()->pubsetbuf(nullptr, 0);  // 禁用缓冲
        }
        outfile << "| Tables |\n";
        RecordPrinter printer(1);
        printer.print_separator(context);
        printer.print_record({"Tables"}, context);
        printer.print_separator(context);
        for (auto &entry: db_.tabs_) {
            auto &tab = entry.second;
            printer.print_record({tab.name}, context);
            outfile << "| " << tab.name << " |\n";
        }
        printer.print_separator(context);
        outfile.close();
    } catch (std::exception &e) {
        std::cerr << "sm_manager show_tables() only pingcas can do" << e.what() << std::endl;
    }
}

/**
 * @description: 显示表的元数据
 * @param {string&} tableName 表名称
 * @param {Context*} context
 */
void SmManager::desc_table(const std::string &tab_name, Context *context) {
    TabMeta &tab = db_.get_table(tab_name);

    std::vector<std::string> captions = {"Field", "Type", "Index"};
    RecordPrinter printer(captions.size());
    // Print header
    printer.print_separator(context);
    printer.print_record(captions, context);
    printer.print_separator(context);
    // Print fields
    for (auto &col: tab.cols) {
        std::vector<std::string> field_info = {col.name, coltype2str(col.type), col.index ? "YES" : "NO"};
        printer.print_record(field_info, context);
    }
    // Print footer
    printer.print_separator(context);
}

/**
 * @description: 创建表
 * @param {string&} tableName 表的名称
 * @param {vector<ColDef>&} col_defs 表的字段
 * @param {Context*} context
 */
void SmManager::create_table(const std::string &tab_name, const std::vector<ColDef> &col_defs, Context *context) {
    if (db_.is_table(tab_name)) {
        throw TableExistsError(tab_name);
    }
    // Create table meta
    int curr_offset = 0;
    TabMeta tab;
    tab.name = tab_name;
    for (auto &col_def: col_defs) {
        ColMeta col = {.tab_name = tab_name,
                .name = col_def.name,
                .type = col_def.type,
                .len = col_def.len,
                .offset = curr_offset,
                .index = false};
        curr_offset += col_def.len;
        tab.cols.push_back(col);
    }
    // Create & open record file
    int record_size = curr_offset;  // record_size就是col meta所占的大小（表的元数据也是以记录的形式进行存储的）
    rm_manager_->create_file(tab_name, record_size);
    db_.tabs_[tab_name] = tab;
    // fhs_[tableName] = rm_manager_->open_file(tableName);
    fhs_.emplace(tab_name, rm_manager_->open_file(tab_name));

    flush_meta();
}

/**
 * @description: 删除表
 * @param {string&} tableName 表的名称
 * @param {Context*} context
 */
void SmManager::drop_table(const std::string &tab_name, Context *context) {
    // 1. 判断tab_name是否正确
    if (!db_.is_table(tab_name)) {
        throw TableNotFoundError(tab_name);
    }
    // 2. 删除db_中的对应的tabs_
    db_.tabs_.erase(tab_name);

    // 3. 删除buffer pool中的pages
    auto rm_file_hdl = fhs_.at(tab_name).get();
    buffer_pool_manager_->delete_all_pages(rm_file_hdl->GetFd());

    // 4. 记录管理器删除表中的数据文件
    rm_manager_->destroy_file(tab_name);
    fhs_.erase(tab_name);

    flush_meta();
}

/**
 * @description: 创建索引
 * @param {string&} tableName 表的名称
 * @param {vector<string>&} col_names 索引包含的字段名称
 * @param {Context*} context
 */
void SmManager::create_index(const std::string &tab_name, const std::vector<std::string> &col_names, Context *context) {
    if (!db_.is_table(tab_name)) {
        throw TableNotFoundError(tab_name);
    }

    // create index加S锁
    if (context != nullptr) {
        context->lock_mgr_->lock_shared_on_table(context->txn_, fhs_[tab_name]->GetFd());
    }

    // 1. 判断该索引是否已经被创建
    if (ix_manager_->exists(tab_name, col_names)) {
        throw IndexExistsError(tab_name, col_names);
    }
    std::string index_name = ix_manager_->get_index_name(tab_name, col_names);

    // 2. 将col_names中每一个string类型col,得到对应的ColMeta类型col(还要修改col_meta的index变量)
    std::vector<ColMeta> col_metas;
    for (auto &col_name: col_names) {
        ColMeta &col_meta = *db_.get_table(tab_name).get_col(col_name);
        col_meta.index = true;
        col_metas.push_back(col_meta);
    }

    // 3. 调用IxManager的createIndex方法初始化index文件
    ix_manager_->create_index(tab_name, col_metas);

    // 4. 更新TableMeta
    int fd = disk_manager_->open_file(index_name);

    IndexMeta index;
    index.tab_name = tab_name;
    auto page = buffer_pool_manager_->fetch_page(PageId{fd, IX_FILE_HDR_PAGE});

    IxFileHdr ix_file_hdl;
    ix_file_hdl.deserialize(page->get_data());
    index.col_tot_len = ix_file_hdl.col_tot_len_;
    index.col_num = ix_file_hdl.col_num_;
    for (auto &col_meta: col_metas) {
        index.cols.push_back(col_meta);
    }
    db_.get_table(tab_name).indexes.push_back(index);

    char *key = new char[index.col_tot_len];

    buffer_pool_manager_->unpin_page(page->get_page_id(), false);
    disk_manager_->close_file(fd);

    // 5. 更新sm_manager
    ihs_.emplace(index_name, ix_manager_->open_index(tab_name, col_names));
    auto ix_hdl = ihs_.at(index_name).get();

    // 6. 将表已存在的record创建索引
    auto tab = db_.get_table(tab_name);
    auto file_hdl = fhs_.at(tab_name).get();
    for (RmScan rm_scan(file_hdl); !rm_scan.is_end(); rm_scan.next()) {
        auto rec = file_hdl->get_record(rm_scan.rid(), context);
        int offset = 0;
        for (size_t i = 0; i < index.col_num; ++i) {
            memcpy(key + offset, rec->data + index.cols[i].offset, index.cols[i].len);
            offset += index.cols[i].len;
        }
//        ix_hdl->insert_entry(key, rm_scan.rid(), context->txn_);
        if (context != nullptr) {
            ix_hdl->insert_entry(key, rm_scan.rid(), context->txn_);
        } else {
            ix_hdl->insert_entry(key, rm_scan.rid(), nullptr);
        }

    }

//    if (nullptr != context) {
//        auto index_record = new IndexCreateRecord(IType::INSERT_INDEX, index_name, tab_name, col_names);
//        context->txn_->append_index_create_record(index_record);
//    }
    tab.indexes.push_back(index);

    flush_meta();
}

/**
 * @description: 删除索引
 * @param {string&} tableName 表名称
 * @param {vector<string>&} col_names 索引包含的字段名称
 * @param {Context*} context
 */
void SmManager::drop_index(const std::string &tab_name, const std::vector<std::string> &col_names, Context *context) {
    // drop index加S锁
    if (context != nullptr) {
        context->lock_mgr_->lock_shared_on_table(context->txn_, fhs_[tab_name]->GetFd());
    }

    if (!db_.is_table(tab_name)) {
        throw TableNotFoundError(tab_name);
    }

    TabMeta &tab = db_.tabs_[tab_name];
    for (auto col_name: col_names) {
        auto col = tab.get_col(col_name);
        if (!tab.is_col_belongs_to_index(col->name)) {
            col->index = false;
        }
    }

    auto index_name = ix_manager_->get_index_name(tab_name, col_names);
    ix_manager_->close_index(ihs_.at(index_name).get());
    ix_manager_->destroy_index(ihs_.at(index_name).get(), tab_name, col_names);

    auto ix_meta = db_.get_table(tab_name).get_index_meta(col_names);
    db_.get_table(tab_name).indexes.erase(ix_meta);
    ihs_.erase(index_name);

//    if (nullptr != context) {
//        auto index_record = new IndexCreateRecord(IType::DROP_INDEX, index_name, tab_name, col_names);
//        context->txn_->append_index_create_record(index_record);
//    }
}

/**
 * @description: 删除索引
 * @param {string&} tableName 表名称
 * @param {vector<ColMeta>&} 索引包含的字段元数据
 * @param {Context*} context
 */
void SmManager::drop_index(const std::string &tab_name, const std::vector<ColMeta> &cols, Context *context) {
    // drop index加S锁
    if (context != nullptr) {
        context->lock_mgr_->lock_shared_on_table(context->txn_, fhs_[tab_name]->GetFd());
    }
    std::vector<std::string> colNames;
    for (auto colMeta: cols) {
        colNames.emplace_back(colMeta.name);
    }
    drop_index(tab_name, colNames, context);
}

void SmManager::show_index(std::string tab_name, Context *context) {
    // show index加S锁
    if (context != nullptr) {
        context->lock_mgr_->lock_shared_on_table(context->txn_, fhs_[tab_name]->GetFd());
    }
    auto index_metas = db_.get_table(tab_name).indexes;
    try {
        std::fstream outfile;
        outfile.open("output.txt", std::ios::out | std::ios::app);
        RecordPrinter printer(3);
        for (auto &index: index_metas) {
            outfile << "| " << tab_name << " | unique | " << "(";
            std::stringstream ss;
            ss << "(";
            auto it = index.cols.begin();
            outfile << (*it).name;
            ss << (*it).name;
            it++;
            for (; it != index.cols.end(); ++it) {
                outfile << "," << (*it).name;
                ss << "," << (*it).name;
            }

            outfile << ") |\n";
            ss << ")";
            printer.print_record({tab_name, "unique", ss.str()}, context);
        }
        outfile.close();
    } catch (std::exception &e) {
        std::cerr << "sm_manager show_index() only pingcas can do" << e.what() << std::endl;
    }
}

void SmManager::load_csv_itermodel(const std::string& file_name, const std::string& table_name) {
    std::ifstream ifs(file_name);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file: " << file_name << std::endl;
        return;
    }

    TabMeta &tab_meta = this->db_.tabs_.at(table_name);

    bool tab_name_enable = false;
    Rid rid_{-1, -1};
    RmFileHandle* fh_ = this->fhs_.at(table_name).get();
    RmRecord buffer_load_record(fh_->get_file_hdr().record_size);
    RmPageHandle page_buffer = fh_->create_new_page_handle();

    std::vector<Value> row(tab_meta.cols.size());
    std::vector<size_t> col_lens(tab_meta.cols.size());

    // Initialize row and column lengths
    for (size_t i = 0; i < tab_meta.cols.size(); ++i) {
        const ColType& col_type = tab_meta.cols[i].type;
        col_lens[i] = tab_meta.cols[i].len;

        if (col_type == ColType::TYPE_INT) {
            row[i].set_int(0);
        } else if (col_type == ColType::TYPE_FLOAT) {
            row[i].set_float(0.0f);
        } else if (col_type == ColType::TYPE_STRING) {
            row[i].set_str("");
        } else {
            assert(0);
        }
        row[i].init_raw(col_lens[i]);
    }

    int line_num = 0;
    std::string s;

    while (std::getline(ifs, s)) {
        ++line_num;
        if (!tab_name_enable) {
            tab_name_enable = true;
            continue;
        }

        std::stringstream ss(s);
        std::string field;
        int ind = 0;

        while (std::getline(ss, field, ',') && ind < row.size()) {
            const ColType& col_type = row[ind].type;

            switch (col_type) {
                case TYPE_INT: {
                    row[ind].set_int(std::stoi(field));
                    break;
                }
                case TYPE_FLOAT: {
                    row[ind].set_float(std::stof(field));
                    break;
                }
                case TYPE_STRING: {
                    row[ind].set_str(field);
                    break;
                }
                default: {
                    assert(0);
                }
            }
            row[ind].cover_raw(col_lens[ind]);
            ++ind;
        }

        load_pre_insert(fh_, row, tab_meta, buffer_load_record, nullptr);

        rid_ = fh_->insert_record_for_load_data(buffer_load_record.data, page_buffer);
        insert_into_index(tab_meta, buffer_load_record, nullptr, rid_);
    }

    std::cout << "All line numbers: " << line_num << std::endl;
    ifs.close();
}


void SmManager::load_pre_insert(RmFileHandle* fh_, std::vector<Value>& values_, TabMeta& tab_, RmRecord& rec, Context* context_) {
    for (size_t i = 0; i < values_.size(); ++i) {
        auto &col = tab_.cols[i];
        auto &val = values_[i];
        if (col.type != val.type) {
            assert(0);
        }
        // if(col.type == TYPE_DATETIME) {
        //     if(!SmManager::check_datetime_(val.datetime_val)){
        //         throw InvalidValueError(val.datetime_val);
        //     }
        // }
        memcpy(rec.data + col.offset, val.raw->data, col.len);
    }
    // for(size_t i = 0; i < tab_.indexes.size(); ++i) {
    //     auto& index = tab_.indexes[i];
    //     auto ih = ihs_.at(get_ix_manager()->get_index_name(tab_.name, index.cols)).get();
    //     #ifndef ENABLE_LOCK_CRABBING
    //         std::lock_guard<std::mutex> lg(ih->root_latch_);
    //      #endif
    //     char* key = new char[index.col_tot_len];
    //     int offset = 0;
    //     for(size_t i = 0; i < (size_t)index.col_num; ++i) {
    //         memcpy(key + offset, rec.data + index.cols[i].offset, index.cols[i].len);
    //         offset += index.cols[i].len;
    //     }
    //     bool is_failed = ih->binary_search(key, context_).page_no != -1;
    //     if(is_failed){
    //         delete[] key;
    //         throw IndexInsertDuplicatedError();
    //     }
    //     delete[] key;
    // }
}

void SmManager::insert_into_index(TabMeta& tab_, RmRecord& rec, Context* context_, Rid& rid_){
    // Insert into index
//    for(size_t i = 0; i < tab_.indexes.size(); ++i) {
//        auto& index = tab_.indexes[i];
//        auto ih = ihs_.at(get_ix_manager()->get_index_name(tab_.name, index.cols)).get();
//        char* key = new char[index.col_tot_len];
//        int offset = 0;
//        for(size_t i = 0; i < (size_t)index.col_num; ++i) {
//            memcpy(key + offset, rec.data + index.cols[i].offset, index.cols[i].len);
//            offset += index.cols[i].len;
//        }
//        ih->insert_entry_load(key, rid_, context_==nullptr? nullptr:context_->txn_);
//        delete[] key;
//    }
    // 插入索引条目
    auto indexes = tab_.indexes;
    auto table_name = tab_.name;
    for (auto &index: indexes) {
        auto ih = ihs_.at(
                get_ix_manager()->get_index_name(table_name, index.cols)).get();

//        std::vector<char> key(index.col_tot_len);
        char* key = new char[index.col_tot_len];
        int offset = 0;
        for (size_t i = 0; i < index.col_num; ++i) {
            memcpy(key + offset, rec.data + index.cols[i].offset, index.cols[i].len);
            offset += index.cols[i].len;
        }

        try {
            ih->insert_entry_load(key, rid_, context_==nullptr? nullptr:context_->txn_);
            delete[] key;
        } catch (InternalError &error) {
            throw InternalError("Non-unique index!");
        }
    }
}

