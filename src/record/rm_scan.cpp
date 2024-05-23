/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_scan.h"
#include "rm_file_handle.h"

/**
 * @brief 初始化file_handle和rid
 * @param file_handle
 */
RmScan::RmScan(const RmFileHandle *file_handle) : file_handle_(file_handle) {
    // 初始化file_handle和rid（指向第一个存放了记录的位置）
    rid_.page_no = RM_FIRST_RECORD_PAGE;
    rid_.slot_no = -1;
    next();
}

/**
 * @brief 找到文件中下一个存放了记录的位置
 */
void RmScan::next() {
    // 如果已经到达末尾，直接返回
    if (is_end()) {
        return;
    }
    // 获取文件总页数和每页记录数
    int num_pages = file_handle_->file_hdr_.num_pages;
    int num_records_per_page = file_handle_->file_hdr_.num_records_per_page;

    // 遍历每一页
    for (; rid_.page_no < num_pages; rid_.page_no++) {
        // 获取当前页的句柄和位图
        auto page_handle = file_handle_->fetch_page_handle(rid_.page_no);
        auto bitmap = page_handle.bitmap;

        // 从当前槽位开始搜索下一个为1的位
        rid_.slot_no = Bitmap::next_bit(true, bitmap, num_records_per_page, rid_.slot_no);

        // 如果找到了记录，直接返回
        if (rid_.slot_no < num_records_per_page) {
            return;
        }
        // 如果当前页没有记录，重置槽位号以便下一页检查
        rid_.slot_no = -1;
    }
    // 如果遍历完所有页仍未找到记录，则重置页号
    rid_.page_no = RM_NO_PAGE;
}

/**
 * @brief ​ 判断是否到达文件末尾
 */
bool RmScan::is_end() const {
    return rid_.page_no == RM_NO_PAGE;
}

/**
 * @brief RmScan内部存放的rid
 */
Rid RmScan::rid() const {
    return rid_;
}