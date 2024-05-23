/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "rm_file_handle.h"

/**
 * @description: 获取当前表中记录号为rid的记录
 * @param {Rid&} rid 记录号，指定记录的位置
 * @param {Context*} context
 * @return {unique_ptr<RmRecord>} rid对应的记录对象指针
 */
std::unique_ptr<RmRecord> RmFileHandle::get_record(const Rid &rid, Context *context) const {
    // 1. 获取指定记录所在的page handle
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 加S锁以获取记录
    if (context != nullptr) {
        context->lock_mgr_->lock_shared_on_record(context->txn_, rid, fd_);
    }

    // 2. 初始化一个指向RmRecord的指针（赋值其内部的data和size）
    int record_size = file_hdr_.record_size;
    auto rm_rcd = std::make_unique<RmRecord>(record_size);

    // 赋值其内部的data和size
    char *data_ptr = page_hdl.get_slot(rid.slot_no);
    std::memcpy(rm_rcd->data, data_ptr, record_size);
    rm_rcd->size = record_size;

    // 解锁并unpin页面
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), false);
    return rm_rcd;
}

/**
 * @description: 在当前表中插入一条记录，不指定插入位置
 * @param {char*} buf 要插入的记录的数据
 * @param {Context*} context
 * @return {Rid} 插入的记录的记录号（位置）
 */
Rid RmFileHandle::insert_record(char *buf, Context *context) {
    // 1. 获取当前未满的page handle
    RmPageHandle page_hdl = create_page_handle();

    // 2. 在page_hdl中找到空闲slot位置
    int record_size = file_hdr_.record_size;
    int record_nums = file_hdr_.num_records_per_page;
    int slot_no = Bitmap::first_bit(false, page_hdl.bitmap, record_nums);
    if (slot_no == record_nums) {
        throw InvalidSlotNoError(slot_no, record_nums);
    }

    // 3. 将buf复制到空闲slot位置
    char *slot_ptr = page_hdl.get_slot(slot_no);
    std::memcpy(slot_ptr, buf, record_size);
    Bitmap::set(page_hdl.bitmap, slot_no);

    // 4. 更新page_handle.page_hdr中的数据结构
    page_hdl.page_hdr->num_records++;

    // 5. 返回新插入的record的rid
    Rid rid{page_hdl.page->get_page_id().page_no, slot_no};

    // 6. 加X锁
    if (context != nullptr) {
        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
    }

    // 7. 当插入该record以后，如果页面满了，更新RmFileHdr的first_page_no
    if (page_hdl.page_hdr->num_records == record_nums) {
        file_hdr_.first_free_page_no = page_hdl.page_hdr->next_free_page_no;
    }

    // 8. 解锁并unpin页面
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
    return rid;
}

/**
 * @description: 在当前表中的指定位置插入一条记录
 * @param {Rid&} rid 要插入记录的位置
 * @param {char*} buf 要插入记录的数据
 */
void RmFileHandle::insert_record(const Rid &rid, char *buf) {
    // Todo:
    // 是否需要加锁

    // 获取指定位置的页面句柄
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 判断指定位置是否为空闲
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        throw std::runtime_error("Specified slot is not empty.");
    }

    // 在指定位置插入记录
    std::memcpy(page_hdl.get_slot(rid.slot_no), buf, file_hdr_.record_size);

    // 更新页面头中的数据结构
    Bitmap::set(page_hdl.bitmap, rid.slot_no);
    page_hdl.page_hdr->num_records++;

    // 更新文件头中的数据结构
    int record_nums = file_hdr_.num_records_per_page;
    if (page_hdl.page_hdr->num_records == record_nums) {
        file_hdr_.first_free_page_no = page_hdl.page_hdr->next_free_page_no;
    }

    // 解锁并unpin页面
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}

/**
 * @description: 删除记录文件中记录号为rid的记录
 * @param {Rid&} rid 要删除的记录的记录号（位置）
 * @param {Context*} context
 */
void RmFileHandle::delete_record(const Rid &rid, Context *context) {
    // delete entry对record加X锁，并获取指定记录所在的页面句柄
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 检查记录是否存在并获取锁
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // 加X锁
    if (context != nullptr) {
        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
    }

    // 更新页面数据结构
    Bitmap::reset(page_hdl.bitmap, rid.slot_no);
    page_hdl.page_hdr->num_records--;

    // 如果页面从满变为未满状态，则调用 release_page_handle
    int record_nums = file_hdr_.num_records_per_page;
    if (page_hdl.page_hdr->num_records == record_nums - 1) {
        release_page_handle(page_hdl);
    } else {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
    }
}


/**
 * @description: 更新记录文件中记录号为rid的记录
 * @param {Rid&} rid 要更新的记录的记录号（位置）
 * @param {char*} buf 新记录的数据
 * @param {Context*} context
 */
void RmFileHandle::update_record(const Rid &rid, char *buf, Context *context) {
    // 获取记录所在的页面句柄
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 加X锁
    if (context != nullptr) {
        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
    }

    // 检查记录是否存在
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

    // 更新记录
    std::memcpy(page_hdl.get_slot(rid.slot_no), buf, file_hdr_.record_size);

    // 解锁并unpin页面
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}

/**
 * 以下函数为辅助函数，仅提供参考，可以选择完成如下函数，也可以删除如下函数，在单元测试中不涉及如下函数接口的直接调用
*/
/**
 * @description: 获取指定页面的页面句柄
 * @param {int} page_no 页面号
 * @return {RmPageHandle} 指定页面的句柄
 */
RmPageHandle RmFileHandle::fetch_page_handle(int page_no) const {
    // 检查页面号是否有效
    if (page_no >= file_hdr_.num_pages) {
        throw PageNotExistError("", page_no);
    }

    // 使用缓冲池获取指定页面，并生成page_handle返回给上层
    PageId page_id;
    page_id.page_no = page_no;
    page_id.fd = fd_;
    Page *page = buffer_pool_manager_->fetch_page(page_id);

    // if page_no is invalid, throw PageNotExistError exception
    if (page == nullptr) {
        throw PageNotExistError("", page_no);
    }
    RmPageHandle page_hdl(&file_hdr_, page);
    return page_hdl;
}

/**
 * @description: 创建一个新的page handle
 * @return {RmPageHandle} 新的PageHandle
 */
RmPageHandle RmFileHandle::create_new_page_handle() {
    // 1.使用缓冲池来创建一个新page
    PageId page_id;
    page_id.fd = fd_;
    Page *page = buffer_pool_manager_->new_page(&page_id);
    if (page == nullptr) {
        throw std::runtime_error("Failed to create new page.");
    }

    // 2.更新page handle中的相关信息
    RmPageHandle page_hdl(&file_hdr_, page);
    Bitmap::init(page_hdl.bitmap, file_hdr_.bitmap_size);
    page_hdl.page_hdr->num_records = 0;
    page_hdl.page_hdr->next_free_page_no = RM_NO_PAGE;

    // 3.更新file_hdr_
    file_hdr_.num_pages++;
    file_hdr_.first_free_page_no = page_hdl.page->get_page_id().page_no;
    return page_hdl;
}

/**
 * @brief 创建或获取一个空闲的page handle
 *
 * @return RmPageHandle 返回生成的空闲page handle
 * @note pin the page, remember to unpin it outside!
 */
RmPageHandle RmFileHandle::create_page_handle() {
    // 1. 判断file_hdr_中是否还有空闲页
    int page_no = file_hdr_.first_free_page_no;

    // 1.1 没有空闲页：使用缓冲池来创建一个新page；可直接调用create_new_page_handle()
    if (page_no != RM_NO_PAGE) {
        return fetch_page_handle(page_no);
    }

    // 1.2 有空闲页：直接获取第一个空闲页
    // 2. 生成page handle并返回给上层
    return create_new_page_handle();
}

/**
 * @description: 当一个页面从没有空闲空间的状态变为有空闲空间状态时，更新文件头和页头中空闲页面相关的元数据
 */
void RmFileHandle::release_page_handle(RmPageHandle &page_handle) {
    // 当page从已满变成未满，考虑如何更新：
    // 1. page_handle.page_hdr->next_free_page_no
    PageId page_id = page_handle.page->get_page_id();
    page_handle.page_hdr->next_free_page_no = file_hdr_.first_free_page_no;

    // 2. file_hdr_.first_free_page_no
    file_hdr_.first_free_page_no = page_id.page_no;
}