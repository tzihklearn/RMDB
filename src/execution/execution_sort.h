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

class SortExecutor : public AbstractExecutor {
   private:
    std::unique_ptr<AbstractExecutor> prev;
    ColMeta col;                              // 框架中只支持一个键排序，需要自行修改数据结构支持多个键排序
    size_t tuple_num;
    bool is_desc;
    std::vector<size_t> used_tuple;
    std::unique_ptr<RmRecord> current_tuple;
    std::vector<std::unique_ptr<RmRecord>> tuples;
    int count;
    RmCompare cmp;

    std::string output_prefix = "merge_out_";
    std::vector<std::string> run_files;

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
    std::vector<std::ifstream> file_pointers;

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
    SortExecutor(std::unique_ptr<AbstractExecutor> prev_, const TabCol& sel_col_, bool is_desc_, Context *context)
            : prev(std::move(prev_)),
              col(prev->get_col_offset(sel_col_)),
              is_desc(is_desc_),
              tuple_num(0),
              cmp(std::make_shared<ColMeta>(col), is_desc_),
              cmp_wrapper([this](const RecordWithIndex& a, const RecordWithIndex& b) {
                  return !cmp(a.first, b.first);
              }),
              min_heap(cmp_wrapper) {
        context_ = context;
        used_tuple.clear();
    }

    std::string getType() override {
        return "SortExecutor";
    }

//    using RecordWithIndex = std::pair<std::unique_ptr<RmRecord>, size_t>;
//    std::function<bool(const RecordWithIndex&, const RecordWithIndex&)> cmp_wrapper;

    void beginTuple() override {
        if (context_->js_->getSortMerge()) {
            merge_sort();
        } else {
            normal_sort();
        }
    }

    void normal_sort() {
        used_tuple.clear();
        for (prev->beginTuple(); !prev->is_end(); prev->nextTuple()) {
            auto tuple = prev->Next();
            tuples.push_back(std::move(tuple));
        }

        std::sort(tuples.begin(), tuples.end(), RmCompare(std::make_shared<ColMeta>(col), is_desc));
        count = 0;
    }

    void merge_sort() {

        used_tuple.clear();
        std::vector<std::unique_ptr<RmRecord>> rmRecords;

        size_t chunk_size = 100000; // 假设每个内存块可以存储1000个元组
        int i = 0;
        int run_id = 0;
        for (prev->beginTuple(); !prev->is_end(); prev->nextTuple()) {
            auto tuple = prev->Next();
            rmRecords.push_back(std::move(tuple));
            if (++i == chunk_size) {
                std::sort(rmRecords.begin(), rmRecords.end(), cmp);
                auto t_record = std::make_unique<RmRecord>(*rmRecords.back());
                min_heap.emplace(std::move(t_record), run_id);
//                tuples.insert(tuples.end(), rmRecords.begin(), rmRecords.end());
                std::string file_name = output_prefix + std::to_string(run_id) + ".txt";
                run_files.push_back(file_name);
                std::ofstream outfile(file_name);
                for (auto &record : rmRecords) {
                    outfile.write(reinterpret_cast<const char*>(&record->size), sizeof(record->size));
                    outfile.write(record->data, record->size);
                }
                rmRecords.clear();
                i = 0;
                outfile.close();
                ++run_id;
            }
//            tuples.push_back(std::move(tuple));
        }



//        std::vector<std::ifstream> file_pointers;
        for (const auto& run_file : run_files) {
            file_pointers.emplace_back(run_file, std::ios::binary);
        }

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
////                min_chunk->pop_back();
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
        auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
        auto record = std::make_unique<RmRecord>(*top.first);
        merge_idx = top.second;
        merge_tuple = std::move(record);
        min_heap.pop();
    }

    void externalMergeSort(std::vector<std::vector<std::unique_ptr<RmRecord>>>& chunks,
                           std::vector<std::unique_ptr<RmRecord>>& result) {

//        // 初始化最小堆，插入每个块的最后一个元素
//        for (size_t i = 0; i < chunks.size(); ++i) {
//            if (!chunks[i].empty()) {
//                min_heap.emplace(std::move(chunks[i].back()), i);
//                chunks[i].pop_back();
//            }
//        }

        // 初始化最小堆，插入每个块的第一个元素
        for (size_t i = 0; i < chunks.size(); ++i) {
            if (!chunks[i].empty()) {
                min_heap.emplace(std::move(chunks[i].front()), i);
                chunks[i].erase(chunks[i].begin());
            }
        }

        while (!min_heap.empty()) {
            // 取出堆顶元素
            auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
            min_heap.pop();
            std::unique_ptr<RmRecord>& min_record = top.first;
            size_t chunk_index = top.second;

            // 将最小元素加入结果
            result.push_back(std::move(min_record));

            // 从相同的块中插入下一个元素到堆中
            if (!chunks[chunk_index].empty()) {
                min_heap.emplace(std::move(chunks[chunk_index].front()), chunk_index);
                chunks[chunk_index].erase(chunks[chunk_index].begin());
            }
        }
    }

    void nextTuple() override {
        if (context_->js_->getSortMerge()) {
            if (!min_heap.empty()) {
                // 取出堆顶元素
                auto top = std::move(const_cast<RecordWithIndex&>(min_heap.top()));
                min_heap.pop();
                std::unique_ptr<RmRecord>& min_record = top.first;
                size_t chunk_index = top.second;

                // 将最小元素加入结果
                merge_tuple = std::move(min_record);
                int size;
                if (file_pointers[merge_idx].read(reinterpret_cast<char*>(&size), sizeof(size))) {
                    char *data = new char[size];
                    file_pointers[merge_idx].read(data, size);
                    min_heap.emplace(std::make_unique<RmRecord>(size, data), merge_idx);
                }

//                // 从相同的块中插入下一个元素到堆中
//                if (!chunks[chunk_index].empty()) {
//                    min_heap.emplace(std::move(chunks[chunk_index].front()), chunk_index);
//                    chunks[chunk_index].erase(chunks[chunk_index].begin());
//                }
            }
        } else {
            count++;
        }
    }

    std::unique_ptr<RmRecord> Next() override {
        if (context_->js_->getSortMerge()) {
            return std::move(merge_tuple);
        } else {
            auto rm_rcd = std::make_unique<RmRecord>(*tuples[count]);
            return rm_rcd;
        }
    }

    Rid &rid() override { return _abstract_rid; }

    // 实现cols方法，上层需要调用
    const std::vector<ColMeta> &cols() const {
        return prev->cols();
    };

    bool is_end() const override {
//        if (limit_ != -1 && count >= limit) {
//            return true;
//        }
        if (context_->js_->getSortMerge()) {

        } else {
            return count >= (int) tuples.size();
        }
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
          count = 0;
      }

};