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
    if (is_end()) {
        return;
    }
    auto &file_hdr = file_handle_->file_hdr_;

    // 提前获取当前页的页面句柄
    auto current_page = file_handle_->fetch_page_handle(rid_.page_no);

    for (int page_no = rid_.page_no; page_no < file_hdr.num_pages; ++page_no) {
        auto &bitmap = current_page.bitmap;
        int num_record = file_hdr.num_records_per_page;

        rid_.slot_no = Bitmap::next_bit(true, bitmap, num_record, rid_.slot_no);
        if (rid_.slot_no < num_record) {
            return;
        }
        current_page = file_handle_->fetch_page_handle(page_no + 1);
    }
    // 遍历页面都没找到
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