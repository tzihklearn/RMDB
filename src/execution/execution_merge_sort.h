//
// Created by tzih on 24-6-27.
//

#ifndef RMDB_EXECUTION_MERGE_SORT_H
#define RMDB_EXECUTION_MERGE_SORT_H


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
#include "execution_defs.h"
#include "execution_manager.h"
#include "executor_abstract.h"
#include "index/ix.h"
#include "system/sm.h"
#include "rm_compare.h"
#include <vector>
#include <memory>
#include <queue>
#include <limits>
#include <functional>

class MergeSortExecutor : public AbstractExecutor {
private:
    std::unique_ptr<AbstractExecutor> prev;
    ColMeta col;                              // 框架中只支持一个键排序，需要自行修改数据结构支持多个键排序
    size_t tuple_num;
    size_t len_;
    bool is_desc;
    std::vector<size_t> used_tuple;
    std::unique_ptr<RmRecord> current_tuple;
    std::vector<std::unique_ptr<RmRecord>> tuples;
    int count;
    RmCompare cmp;
    bool end;

    std::string output_prefix = "merge_out_";
    std::string sort_prefix = "merge_sort_";

    int e_id;

    bool is_out;

    int i_num = 0;

//    using RecordWithIndex = std::pair<std::unique_ptr<RmRecord>, size_t>;
//    auto cmp_wrapper = [this](const RecordWithIndex& a, const RecordWithIndex& b) {
//        return !cmp(a.first, b.first);
//    };
//
//    std::priority_queue<RecordWithIndex, std::vector<RecordWithIndex>, decltype(cmp_wrapper)> min_heap(cmp_wrapper);

    using RecordWithIndex = std::pair<std::unique_ptr<RmRecord>, size_t>;
    std::function<bool(const RecordWithIndex&, const RecordWithIndex&)> cmp_wrapper;
    std::priority_queue<RecordWithIndex, std::vector<RecordWithIndex>, decltype(cmp_wrapper)> min_heap;
    unsigned long merge_idx;
    std::unique_ptr<RmRecord> merge_tuple;
    std::unique_ptr<std::ifstream> merge_sort_file;

public:
//    SortExecutor(std::unique_ptr<AbstractExecutor> prev_, const TabCol& sel_col_, bool is_desc_, Context *context) {
//        prev = std::move(prev_);
//        col = prev->get_col_offset(sel_col_);
//        is_desc = is_desc_;
//        tuple_num = 0;
//        used_tuple.clear();
//        cmp = RmCompare(std::make_shared<ColMeta>(col), is_desc);
//        context_ = context;
//
//        cmp_wrapper([this](const RecordWithIndex& a, const RecordWithIndex& b) {
//            return !cmp(a.first, b.first);
//        });
//        min_heap(cmp_wrapper);
//    }
    MergeSortExecutor(std::unique_ptr<AbstractExecutor> prev_, const TabCol& sel_col_, bool is_desc_, Context *context, int e_id_, bool is_out_)
            : prev(std::move(prev_)),
              col(prev->get_col_offset(sel_col_)),
              is_desc(is_desc_),
              tuple_num(0),
              cmp(std::make_shared<ColMeta>(col), is_desc_),
              cmp_wrapper([this](const RecordWithIndex& a, const RecordWithIndex& b) {
                  return !cmp(a.first, b.first);
              }),
              min_heap(cmp_wrapper),
              e_id(e_id_),
              is_out(is_out_) {
        end = false;
        context_ = context;
        used_tuple.clear();
        output_prefix = output_prefix + std::to_string(e_id) + "_";
        sort_prefix = sort_prefix + std::to_string(e_id) + "_";
        len_ = prev->tupleLen();
    }

    std::string getType() override {
        return "SortExecutor";
    }

//    using RecordWithIndex = std::pair<std::unique_ptr<RmRecord>, size_t>;
//    std::function<bool(const RecordWithIndex&, const RecordWithIndex&)> cmp_wrapper;

    void beginTuple() override {
        merge_sort();
    }

    size_t tupleLen() const override {
        return len_;
    };

    void merge_sort() {

        used_tuple.clear();
        std::vector<std::unique_ptr<RmRecord>> rmRecords;

        // todo: std::vector<std::ifstream> file_pointers;同时打开的数量有限制，打开后要即使关闭
        // 所以现在要先将数据排好写到一个统一的文件，然后每次都去查那个文件
        // 正好题目也要求写到文件中
        size_t chunk_size = 3000; // 假设每个内存块可以存储1000个元组
        int i = 0;
        int run_id = 0;
        std::vector<std::string> run_files;

        for (prev->beginTuple(); !prev->is_end(); prev->nextTuple()) {
            auto tuple = prev->Next();
            rmRecords.push_back(std::move(tuple));
            if (++i == chunk_size) {
                std::sort(rmRecords.begin(), rmRecords.end(), cmp);
                auto t_record = std::make_unique<RmRecord>(**rmRecords.begin());
//                min_heap.emplace(std::move(t_record), run_id);
//                tuples.insert(tuples.end(), rmRecords.begin(), rmRecords.end());
                std::string file_name = output_prefix + std::to_string(run_id) + ".txt";
                run_files.push_back(file_name);
                std::unique_ptr<std::fstream> outfilePtr(new std::fstream(file_name));
//                std::ofstream outfilePtr(file_name);
                // 第一个元素以及在min_heap中，不需要写入文件
                rmRecords.erase(rmRecords.begin());

                for (auto &record : rmRecords) {
                    outfilePtr->write(reinterpret_cast<const char*>(&record->size), sizeof(record->size));
                    outfilePtr->write(record->data, record->size);
                }
//                outfilePtr->flush();
//                file_pointers.push_back(std::move(outfilePtr));
                rmRecords.clear();
                i = 0;
                outfilePtr->close();
                ++run_id;
            }
//            tuples.push_back(std::move(tuple));
        }

        if (!rmRecords.empty()) {
            std::sort(rmRecords.begin(), rmRecords.end(), cmp);
            auto t_record = std::make_unique<RmRecord>(**rmRecords.begin());
//            min_heap.emplace(std::move(t_record), run_id);
//                tuples.insert(tuples.end(), rmRecords.begin(), rmRecords.end());
            std::string file_name = output_prefix + std::to_string(run_id) + ".txt";
            run_files.push_back(file_name);
            std::unique_ptr<std::ofstream> outfilePtr(new std::ofstream(file_name, std::ios::out | std::ios::binary));
            if (!outfilePtr->is_open() || !outfilePtr->good()) {
                std::cerr << "Failed to open file: " << file_name << std::endl;
                return; // 或者其他适当的错误处理
            }
            for (auto &record : rmRecords) {
                outfilePtr->write(reinterpret_cast<const char*>(&record->size), sizeof(record->size));
                outfilePtr->write(record->data, record->size);
            }
//            outfilePtr->flush();
//            file_pointers.push_back(std::move(outfilePtr));
            rmRecords.clear();
            i = 0;
            outfilePtr->close();
            ++run_id;
        }


        // 归并排序输出到制定文件
        std::vector<std::unique_ptr<std::ifstream>> file_pointers;
        for (const auto& run_file : run_files) {
            std::unique_ptr<std::ifstream> file_ptr(new std::ifstream(run_file, std::ios::binary));
            file_pointers.push_back(std::move(file_ptr));
        }
        int t_size;
        for (i = 0; i < file_pointers.size(); ++i) {
            if (file_pointers[i]->read(reinterpret_cast<char*>(&t_size), sizeof(t_size))) {
                char *data = new char[t_size];
                file_pointers[i]->read(data, t_size);
                min_heap.emplace(std::make_unique<RmRecord>(t_size, data), i);
            }
        }

        // left
        if (is_out) {
            std::unique_ptr<std::fstream> sorted_results(new std::fstream("sorted_results.txt", std::ios::out | std::ios::app));
            auto cols = prev->cols();
            unsigned long col_size = cols.size();
            try {
//            sorted_results->open("sorted_results.txt", std::ios::out | std::ios::app);
                *sorted_results << "|";
                for (i = 0; i < col_size; ++i) {
                    *sorted_results << " " << cols[i].name << " |";
                }
                *sorted_results << "\n";
            } catch (std::exception &e) {
                std::cerr << "execution_manager select_from show_index() only pingcas can do" << e.what() << std::endl;
            }

            std::unique_ptr<std::ofstream> merge_sort_file_out(new std::ofstream(sort_prefix));
            while (!min_heap.empty()) {
                // 取出堆顶元素
                auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));

                // 输出到sorted_results.txt文件中
                std::unique_ptr<RmRecord>& tuple = top.first;
                std::vector<std::string> columns;
                for (i = 0; i < col_size; ++i) {
                    std::string col_str;
                    char *rec_buf = tuple->data + cols[i].offset;
                    switch (cols[i].type) {
                        case TYPE_INT: {
                            col_str = std::to_string(*(int *) rec_buf);
                            break;
                        }
                        case TYPE_FLOAT: {
                            col_str = std::to_string(*(float *) rec_buf);
                            break;
                        }
                        case TYPE_STRING: {
                            col_str = std::string((char *) rec_buf, cols[i].len);
                            col_str.resize(strlen(col_str.c_str()));
                            break;
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    columns.push_back(col_str);
                }
                // print record into file
                *sorted_results << "|";
                for (const auto &column: columns) {
                    *sorted_results << " " << column << " |";
                }
                *sorted_results << "\n";
                sorted_results->flush();

                // 输出到merge_sort文件中
                merge_sort_file_out->write(reinterpret_cast<const char*>(&tuple->size), sizeof(tuple->size));
                merge_sort_file_out->write(tuple->data, tuple->size);
                merge_sort_file_out->flush();

                size_t chunk_index = top.second;

                merge_idx = chunk_index;
                min_heap.pop();
                int size;
//            if (!file_pointers[merge_idx]->is_open() || !file_pointers[merge_idx]->good()) {
//                std::cerr << "Failed to open or invalid file stream at index: " << merge_idx << std::endl;
//                return;
//            }
                ++tuple_num;
                if (file_pointers[merge_idx]->read(reinterpret_cast<char*>(&size), sizeof(size))) {
                    char *data = new char[size];
                    file_pointers[merge_idx]->read(data, size);
                    min_heap.emplace(std::make_unique<RmRecord>(size, data), merge_idx);
                } else {
                    auto t_re = std::make_unique<RmRecord>();
                    for (i = 0; i < file_pointers.size(); ++i) {
                        if (file_pointers[i]->read(reinterpret_cast<char*>(&size), sizeof(size))) {
                            char *data = new char[size];
                            file_pointers[i]->read(data, size);
                            auto t = std::make_unique<RmRecord>(size, data);
                            if (!t_re->allocated_ || cmp(t_re, t)){
                                t_re = std::make_unique<RmRecord>(*t);
                                merge_idx = i;
                            }
                        }
                    }
                    if (t_re->allocated_) {
                        min_heap.emplace(std::move(t_re), merge_idx);
                    }
                }
            }

            // 关闭不需要的资源
            merge_sort_file_out->close();
            sorted_results->close();
        } else {
            auto cols = prev->cols();
            unsigned long col_size = cols.size();

            std::unique_ptr<std::ofstream> merge_sort_file_out(new std::ofstream(sort_prefix));
            while (!min_heap.empty()) {
                // 取出堆顶元素
                auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));

                // 输出到sorted_results.txt文件中
                std::unique_ptr<RmRecord>& tuple = top.first;
                std::vector<std::string> columns;
                for (i = 0; i < col_size; ++i) {
                    std::string col_str;
                    char *rec_buf = tuple->data + cols[i].offset;
                    switch (cols[i].type) {
                        case TYPE_INT: {
                            col_str = std::to_string(*(int *) rec_buf);
                            break;
                        }
                        case TYPE_FLOAT: {
                            col_str = std::to_string(*(float *) rec_buf);
                            break;
                        }
                        case TYPE_STRING: {
                            col_str = std::string((char *) rec_buf, cols[i].len);
                            col_str.resize(strlen(col_str.c_str()));
                            break;
                        }
                        default: {
                            throw InvalidTypeError();
                        }
                    }
                    columns.push_back(col_str);
                }
                // print record into file
//                sorted_results->flush();

                // 输出到merge_sort文件中
                merge_sort_file_out->write(reinterpret_cast<const char*>(&tuple->size), sizeof(tuple->size));
                merge_sort_file_out->write(tuple->data, tuple->size);
                merge_sort_file_out->flush();

                size_t chunk_index = top.second;

                merge_idx = chunk_index;
                min_heap.pop();
                int size;
//            if (!file_pointers[merge_idx]->is_open() || !file_pointers[merge_idx]->good()) {
//                std::cerr << "Failed to open or invalid file stream at index: " << merge_idx << std::endl;
//                return;
//            }
                ++tuple_num;
                if (file_pointers[merge_idx]->read(reinterpret_cast<char*>(&size), sizeof(size))) {
                    char *data = new char[size];
                    file_pointers[merge_idx]->read(data, size);
                    min_heap.emplace(std::make_unique<RmRecord>(size, data), merge_idx);
                } else {
                    auto t_re = std::make_unique<RmRecord>();
                    for (i = 0; i < file_pointers.size(); ++i) {
                        if (file_pointers[i]->read(reinterpret_cast<char*>(&size), sizeof(size))) {
                            char *data = new char[size];
                            file_pointers[i]->read(data, size);
                            auto t = std::make_unique<RmRecord>(size, data);
                            if (!t_re->allocated_ || cmp(t_re, t)){
                                t_re = std::make_unique<RmRecord>(*t);
                                merge_idx = i;
                            }
                        }
                    }
                    if (t_re->allocated_) {
                        min_heap.emplace(std::move(t_re), merge_idx);
                    }
                }
            }

            // 关闭不需要的资源
            merge_sort_file_out->close();
        }


        for (auto& file_ptr : file_pointers) {
            if (file_ptr->is_open()) {
                file_ptr->close();
            }
        }
        file_pointers.clear();

        merge_sort_file = std::make_unique<std::ifstream>(std::ifstream(sort_prefix));

//        for (i = 0; i < file_pointers.size(); ++i) {
//            int size;
//            if (file_pointers[i].read(reinterpret_cast<char*>(&size), sizeof(size))) {
//                char *data = new char[size];
//                file_pointers[i].read(data, size);
//                min_heap.emplace(std::make_unique<RmRecord>(size, data), i);
//            }
//        }


//        // 分割tuples数组
//        std::vector<std::vector<std::unique_ptr<RmRecord>>> chunks;
//        chunks.reserve(tuples.size() / chunk_size + 1);
//        auto item = tuples.begin();
//        while (item != tuples.end()) {
//            auto t = std::vector<std::unique_ptr<RmRecord>>();
//            t.reserve(chunk_size);
//
//            auto end = std::min(item + chunk_size, tuples.end());
//            while (item != end) {
//                t.push_back(std::move(*item));
//                ++item;
//            }
//            chunks.push_back(std::move(t));
//        }
//
//        // 内存中排序
//        for (auto& chunk : chunks) {
//            std::sort(chunk.begin(), chunk.end(), cmp);
//        }
//
//        // 外部合并排序
//        std::vector<std::unique_ptr<RmRecord>> result;
//        externalMergeSort(chunks, result);
////        while (!chunks.empty()) {
////            std::vector<std::unique_ptr<RmRecord>>* min_chunk = nullptr;
////            size_t min_chunk_size = std::numeric_limits<size_t>::max();
////            auto item = chunks.begin();
////            int i = 0;
////            for (auto& chunk : chunks) {
////                ++i;
////                if (chunk.empty()) continue;
////
////                if (min_chunk == nullptr || !cmp.operator()(min_chunk->back(), chunk.back())) {
////                    min_chunk = &chunk;
////                    min_chunk_size = chunk.size();
////                    while (i > 0) {
////                        ++item;
////                        --i;
////                    }
////                }
//////                if (chunk.back()->compare(min_chunk->back()) < 0) {
//////                    min_chunk = &chunk;
//////                    min_chunk_size = chunk.size();
//////                    while (i > 0) {
//////                        ++item;
//////                        --i;
//////                    }
//////                }
////            }
////            if (min_chunk) {
////                result.push_back(std::move(min_chunk->back()));
////                min_chunk->pop_back(CloudComputing);
////                if (min_chunk->empty()) {
////                    chunks.erase(item);
////                }
////            }
////        }
//
//
//        // 更新当前元组
////        current_tuple = std::make_unique<RmRecord>(std::move(result.front()));
////        current_tuple = std::move(result.front());
////        result.erase(result.begin());
//        tuples = std::move(result);
        count = 0;
//        auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
//        auto record = std::make_unique<RmRecord>(*top.first);
//        merge_idx = top.second;
//        merge_tuple = std::move(record);
//        min_heap.pop();
        nextTuple();
    }

//    void externalMergeSort(std::vector<std::vector<std::unique_ptr<RmRecord>>>& chunks,
//                           std::vector<std::unique_ptr<RmRecord>>& result) {
//
////        // 初始化最小堆，插入每个块的最后一个元素
////        for (size_t i = 0; i < chunks.size(); ++i) {
////            if (!chunks[i].empty()) {
////                min_heap.emplace(std::move(chunks[i].back()), i);
////                chunks[i].pop_back();
////            }
////        }
//
//        // 初始化最小堆，插入每个块的第一个元素
//        for (size_t i = 0; i < chunks.size(); ++i) {
//            if (!chunks[i].empty()) {
//                min_heap.emplace(std::move(chunks[i].front()), i);
//                chunks[i].erase(chunks[i].begin());
//            }
//        }
//
//        while (!min_heap.empty()) {
//            // 取出堆顶元素
//            auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
//            min_heap.pop();
//            std::unique_ptr<RmRecord>& min_record = top.first;
//            size_t chunk_index = top.second;
//
//            // 将最小元素加入结果
//            result.push_back(std::move(min_record));
//
//            // 从相同的块中插入下一个元素到堆中
//            if (!chunks[chunk_index].empty()) {
//                min_heap.emplace(std::move(chunks[chunk_index].front()), chunk_index);
//                chunks[chunk_index].erase(chunks[chunk_index].begin());
//            }
//        }
//    }

    void nextTuple() override {
        ++i_num;
        int size;
        if (merge_sort_file->read(reinterpret_cast<char*>(&size), sizeof(size))) {
            char *data = new char[size];
            merge_sort_file->read(data, size);
            merge_tuple = std::make_unique<RmRecord>(size, data);
        } else {
            end = true;
        }
//        if (!min_heap.empty()) {
//            // 取出堆顶元素
//            auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
//
//            std::unique_ptr<RmRecord>& min_record = top.first;
//            size_t chunk_index = top.second;
//
//            // 将最小元素加入结果
//            merge_tuple = std::make_unique<RmRecord>(*min_record);
////            merge_tuple = std::move(min_record);
//            merge_idx = chunk_index;
//            min_heap.pop();
//
//            if (file_pointers[merge_idx].read(reinterpret_cast<char*>(&size), sizeof(size))) {
//                char *data = new char[size];
//                file_pointers[merge_idx].read(data, size);
//                min_heap.emplace(std::make_unique<RmRecord>(size, data), merge_idx);
//            } else {
//                auto t_re = std::make_unique<RmRecord>();
//                for (int i = 0; i < file_pointers.size(); ++i) {
//                    if (file_pointers[i].read(reinterpret_cast<char*>(&size), sizeof(size))) {
//                        char *data = new char[size];
//                        file_pointers[merge_idx].read(data, size);
//                        auto t = std::make_unique<RmRecord>(size, data);
//                        if (!t_re->allocated_ || cmp(t_re, t)){
//                            t_re = std::make_unique<RmRecord>(*t);
//                            merge_idx = i;
//                        }
//                    }
//                }
//                if (t_re->allocated_) {
//                    min_heap.emplace(std::move(t_re), merge_idx);
//                }
//            }
//
//
////                // 从相同的块中插入下一个元素到堆中
////                if (!chunks[chunk_index].empty()) {
////                    min_heap.emplace(std::move(chunks[chunk_index].front()), chunk_index);
////                    chunks[chunk_index].erase(chunks[chunk_index].begin());
////                }
//        }
    }

    std::unique_ptr<RmRecord> Next() override {
        auto rm_rcd = std::make_unique<RmRecord>(*merge_tuple);
        return rm_rcd;
    }

    Rid &rid() override { return _abstract_rid; }

    // 实现cols方法，上层需要调用
    const std::vector<ColMeta> &cols() const {
        return prev->cols();
    };

    bool is_end() const override {
        return end;
//        return i_num >= tuple_num;
    };


//    void beginTuple() override {
//
//    }
//
//    void nextTuple() override {
//
//    }
//
//    std::unique_ptr<RmRecord> Next() override {
//        return nullptr;
//    }
//
//    Rid &rid() override { return _abstract_rid; }
    void set_begin() override {
//        file_pointers.clear();
//
//        for (const auto& run_file : run_files) {
//            file_pointers.emplace_back(run_file, std::ios::binary);
//        }
//        beginTuple();
        merge_sort_file->close();

        merge_sort_file = std::make_unique<std::ifstream>(std::ifstream(sort_prefix));
        merge_sort_file->seekg(0, std::ios::beg);
        end = false;
        i_num = 0;
        nextTuple();
    }

    void end_work() override {
        merge_sort_file->clear();
        merge_sort_file->close();
        prev->end_work();
    }

};

#endif //RMDB_EXECUTION_MERGE_SORT_H
