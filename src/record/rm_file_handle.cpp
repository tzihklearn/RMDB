#include "rm_file_handle.h"
#include <shared_mutex>

/**
 * @description: 获取当前表中记录号为rid的记录
 * @param {Rid&} rid 记录号，指定记录的位置
 * @param {Context*} context
 * @return {unique_ptr<RmRecord>} rid对应的记录对象指针
 */
std::unique_ptr<RmRecord> RmFileHandle::get_record(const Rid &rid, Context *context, bool was_get_lock) const {
    // 加共享锁保护数据结构
    if (!was_get_lock) {
        std::shared_lock<std::shared_mutex> lock{latch_};
    }

//    // 对record加S锁
//    if (context != nullptr) {
//        context->lock_mgr_->lock_shared_on_record(context->txn_, rid, fd_);
//    }

    // 获取记录所在的page handle
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 初始化一个指向RmRecord的指针（赋值其内部的data和size）
    int record_size = file_hdr_.record_size;
    auto rm_rcd = std::make_unique<RmRecord>(record_size);

    // 赋值其内部的data和size
    char *data_ptr = page_hdl.get_slot(rid.slot_no);
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    std::memcpy(rm_rcd->data, data_ptr, record_size);
    rm_rcd->size = record_size;

    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), false);
    return rm_rcd;
}

/**
 * @description: 在当前表中插入一条记录，不指定插入位置
 * @param {char*} buf 要插入的记录的数据
 * @param {Context*} context
 * @return {Rid} 插入的记录的记录号（位置）
 */
Rid RmFileHandle::insert_record(char *buf, Context *context, bool is_abort) {
    std::shared_lock<std::shared_mutex> lock{latch_};
    // 1.获取当前未满的page handle
    RmPageHandle page_hdl = create_page_handle();

    // 在page_hdl中找到空闲slot位置
    int record_size = file_hdr_.record_size;
    int record_nums = file_hdr_.num_records_per_page;
    int slot_no = Bitmap::first_bit(false, page_hdl.bitmap, record_nums, *page_hdl.deleted);
    if (slot_no == record_nums) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw InvalidSlotNoError(slot_no, record_nums);
    }

//    // 对record加X锁
//    if (context != nullptr) {
//        auto rid = Rid{.page_no = page_hdl.page->get_page_id().page_no, .slot_no = slot_no};
//        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
//    }

    // 将buf复制到空闲slot位置
    char *slot_ptr = page_hdl.get_slot(slot_no);
    std::memcpy(slot_ptr, buf, record_size);
    Bitmap::set(page_hdl.bitmap, slot_no);

    // 更新page_handle.page_hdr的数据结构
    if (++page_hdl.page_hdr->num_records == record_nums) {
        file_hdr_.first_free_page_no = page_hdl.page_hdr->next_free_page_no;
    }

    Rid rid = Rid{page_hdl.page->get_page_id().page_no, slot_no};

    if(context != nullptr && !is_abort) {
        RmRecord rec(file_hdr_.record_size, buf);
        auto table_name = disk_manager_->get_file_name(fd_);

        // 记录事务日志
        Transaction *txn = context->txn_;
        auto *insert_log = new InsertLogRecord(txn->get_transaction_id(), rec, rid, table_name);
        insert_log->prev_lsn_ = txn->get_prev_lsn();

        //日志管理
        lsn_t lsn = context->log_mgr_->add_insert_log_record(context->txn_->get_transaction_id(), rec, rid, table_name);
        txn->set_prev_lsn(lsn);
        context->log_mgr_->flush_log_to_disk();

        auto *write_rcd = new TableWriteRecord(WType::INSERT_TUPLE, table_name, rid);
        context->txn_->append_table_write_record(write_rcd);


        //事务管理
//        RmRecord ins_rec(file_hdr_.record_size, buf);
//        WriteRecord *wr_rec = new WriteRecord(WType::INSERT_TUPLE, disk_manager_->get_file_name(fd_), rid, ins_rec);
////        context->txn_->append_write_record(wr_rec);


    } else if (context != nullptr) {
        RmRecord rec(file_hdr_.record_size, buf);
        auto table_name = disk_manager_->get_file_name(fd_);
        context->log_mgr_->add_insert_log_record(context->txn_->get_transaction_id(), rec, rid, table_name);
        context->log_mgr_->flush_log_to_disk();
    }
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);

    // 返回新插入的record的rid
    return Rid{page_hdl.page->get_page_id().page_no, slot_no};
    // 5.返回新插入的record的rid
    return rid;
}

/**
 * @description: 在当前表中的指定位置插入一条记录
 * @param {Rid&} rid 要插入记录的位置
 * @param {char*} buf 要插入记录的数据
 */
void RmFileHandle::insert_record(Rid &rid, char *buf, Context *context, bool is_abort) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

    // 判断指定位置是否空闲，若不空闲则返回异常
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);
    if (Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
//        auto recore = get_record(rid, nullptr, true);
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
//    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
//        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
//        throw RecordNotFoundError(rid.page_no, rid.slot_no);
//    }

    // 在指定位置插入记录
    std::memcpy(page_hdl.get_slot(rid.slot_no), buf, file_hdr_.record_size);

    // 更新page_handle中的数据结构
    Bitmap::set(page_hdl.bitmap, rid.slot_no);
    int record_nums = file_hdr_.num_records_per_page;
    if (++page_hdl.page_hdr->num_records == record_nums) {
        file_hdr_.first_free_page_no = page_hdl.page_hdr->next_free_page_no;
    }

    if(context != nullptr && !is_abort) {
        RmRecord rec(file_hdr_.record_size, buf);
        auto table_name = disk_manager_->get_file_name(fd_);

        // 记录事务日志
        Transaction *txn = context->txn_;
        auto *insert_log = new InsertLogRecord(txn->get_transaction_id(), rec, rid, table_name);
        insert_log->prev_lsn_ = txn->get_prev_lsn();

        //日志管理
        lsn_t lsn = context->log_mgr_->add_insert_log_record(context->txn_->get_transaction_id(), rec, rid, table_name);
        txn->set_prev_lsn(lsn);
        context->log_mgr_->flush_log_to_disk();

        auto *write_rcd = new TableWriteRecord(WType::INSERT_TUPLE, table_name, rid);
        context->txn_->append_table_write_record(write_rcd);


        //事务管理
//        RmRecord ins_rec(file_hdr_.record_size, buf);
//        WriteRecord *wr_rec = new WriteRecord(WType::INSERT_TUPLE, disk_manager_->get_file_name(fd_), rid, ins_rec);
////        context->txn_->append_write_record(wr_rec);


    } else if (context != nullptr) {
        RmRecord rec(file_hdr_.record_size, buf);
        auto table_name = disk_manager_->get_file_name(fd_);
        context->log_mgr_->add_insert_log_record(context->txn_->get_transaction_id(), rec, rid, table_name);
        context->log_mgr_->flush_log_to_disk();
    }

    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}

/**
 * @description: 在当前表中的指定位置插入一条记录
 * @param {Rid&} rid 要插入记录的位置
 * @param {char*} buf 要插入记录的数据
 */
void RmFileHandle::insert_record(const Rid &rid, char *buf) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

    // 判断指定位置是否空闲，若不空闲则返回异常
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);
    if (Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
//        auto recore = get_record(rid, nullptr, true);
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
//    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
//        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
//        throw RecordNotFoundError(rid.page_no, rid.slot_no);
//    }

    // 在指定位置插入记录
    std::memcpy(page_hdl.get_slot(rid.slot_no), buf, file_hdr_.record_size);

    // 更新page_handle中的数据结构
    Bitmap::set(page_hdl.bitmap, rid.slot_no);
    int record_nums = file_hdr_.num_records_per_page;
    if (++page_hdl.page_hdr->num_records == record_nums) {
        file_hdr_.first_free_page_no = page_hdl.page_hdr->next_free_page_no;
    }
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}

/**
 * @description: 删除记录文件中记录号为rid的记录
 * @param {Rid&} rid 要删除的记录的记录号（位置）
 * @param {Context*} context
 */
void RmFileHandle::delete_record(Rid &rid, Context *context, bool is_abort) {
//void RmFileHandle::delete_record(const Rid &rid, Context *context) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

    // 1. 获取指定记录所在的page handle
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 2. 更新page_handle.page_hdr的数据结构
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    // 重置bitmap对应位

    // delete entry对record加X锁
    if (context != nullptr && !is_abort) {
        auto rec = get_record(rid, context, true);
        auto table_name = disk_manager_->get_file_name(fd_);

        Transaction *txn = context->txn_;
        auto *delete_log = new DeleteLogRecord(txn->get_transaction_id(), *rec, rid, table_name);
        delete_log->prev_lsn_ = txn->get_prev_lsn();

        //日志管理
        lsn_t lsn = context->log_mgr_->add_delete_log_record(context->txn_->get_transaction_id(), *rec, rid, disk_manager_->get_file_name(fd_));
        context->log_mgr_->flush_log_to_disk();
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);

        txn->set_prev_lsn(lsn);

        auto *write_record = new TableWriteRecord(WType::DELETE_TUPLE, table_name, rid, *rec);
        context->txn_->append_table_write_record(write_record);

//        auto rec = get_record(rid, context);
//        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
//
//        WriteRecord *wr_rec = new WriteRecord(WType::DELETE_TUPLE, disk_manager_->get_file_name(fd_), rid, *rec);
//        context->txn_->append_write_record(wr_rec);


    } else if (context != nullptr) {
        auto rec = get_record(rid, context, true);
        context->log_mgr_->add_delete_log_record(context->txn_->get_transaction_id(), *rec, rid, disk_manager_->get_file_name(fd_));
        context->log_mgr_->flush_log_to_disk();
    }

    // 2.1 测试bitmap对应位,测试成功则重置
    Bitmap::reset(page_hdl.bitmap, rid.slot_no);
    (*page_hdl.deleted)++;

    // 如果page从满变为未满状态,调用release_page_handle()
    int record_nums = file_hdr_.num_records_per_page;
//    if (page_hdl.page_hdr->num_records-- == record_nums) {
//        release_page_handle(page_hdl);
//    }
//    if (page_hdl.page_hdr->num_records == record_nums) {
//        release_page_handle(page_hdl);
//    }
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}


/**
 * @description: 更新记录文件中记录号为rid的记录
 * @param {Rid&} rid 要更新的记录的记录号（位置）
 * @param {char*} buf 新记录的数据
 * @param {Context*} context
 */
void RmFileHandle::update_record(Rid &rid, char *buf, Context *context, bool is_abort) {
    // update_entry对record加X锁
//    if (context != nullptr) {
//        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
//    }
//void RmFileHandle::update_record(const Rid &rid, char *buf, Context *context) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

//    // 对record加X锁
//    if (context != nullptr) {
//        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
//    }

    // 获取指定记录所在的page handle
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    // 更新记录

    //向get_record传递context
    if(context != nullptr && !is_abort) {
        //加互斥锁
        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);

        auto rec = get_record(rid, context, true);
        std::vector<char> newRecord(file_hdr_.record_size);
        memcpy(newRecord.data(), buf, file_hdr_.record_size);
        auto table_name = disk_manager_->get_file_name(fd_);

        // 记录更新前后的数据，用于日志记录
        RmRecord beforeUpdateRecord(rec->size);
        memcpy(beforeUpdateRecord.data, rec->data, rec->size);
        RmRecord afterUpdateRecord(newRecord.size());
        memcpy(afterUpdateRecord.data, newRecord.data(), newRecord.size());
        // 写入更新日志
        auto writeRecord = new TableWriteRecord(WType::UPDATE_TUPLE, table_name, rid, beforeUpdateRecord,
                                                    afterUpdateRecord);
        context->txn_->append_table_write_record(writeRecord);

        Transaction *txn = context->txn_;
        //日志处理
        lsn_t lsn = context->log_mgr_->add_update_log_record(context->txn_->get_transaction_id(), afterUpdateRecord, beforeUpdateRecord, rid, disk_manager_->get_file_name(fd_));
        context->log_mgr_->flush_log_to_disk();

        txn->set_prev_lsn(lsn);

//        WriteRecord *wr_rec = new WriteRecord(WType::UPDATE_TUPLE, disk_manager_->get_file_name(fd_), rid, *old_rec);
//        context->txn_->append_write_record(wr_rec);


//        RmRecord update_record(file_hdr_.record_size);
//        memcpy(update_record.data, buf, update_record.size);

#ifdef ENABLE_LSN
        update_page_handle.page->set_page_lsn(new_lsn);
#endif
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
    } else if (context != nullptr) {
        auto rec = get_record(rid, context, true);
        std::vector<char> newRecord(file_hdr_.record_size);
        memcpy(newRecord.data(), buf, file_hdr_.record_size);
        auto table_name = disk_manager_->get_file_name(fd_);

        // 记录更新前后的数据，用于日志记录
        RmRecord beforeUpdateRecord(rec->size);
        memcpy(beforeUpdateRecord.data, rec->data, rec->size);
        RmRecord afterUpdateRecord(newRecord.size());
        memcpy(afterUpdateRecord.data, newRecord.data(), newRecord.size());
        context->log_mgr_->add_update_log_record(context->txn_->get_transaction_id(), afterUpdateRecord, beforeUpdateRecord, rid, disk_manager_->get_file_name(fd_));
        context->log_mgr_->flush_log_to_disk();
    }

    // 2. 更新记录
    std::memcpy(page_hdl.get_slot(rid.slot_no), buf, file_hdr_.record_size);

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
    // 1.如果page_no invalid，抛出PageNotExistError异常
    if (page_no >= RmFileHandle::file_hdr_.num_pages || page_no < 0) {
        throw PageNotExistError("", page_no);
    }
    // 2.通过buffer pool manager获取指定pageId的page
    PageId page_id;
    page_id.page_no = page_no;
    page_id.fd = fd_;
    Page *page = buffer_pool_manager_->fetch_page(page_id);

    if (page == nullptr) {
        throw PageNotExistError("", page_no);
    }
    // 3.将获取到的page对象封装为RmPageHandle返回
    RmPageHandle page_hdl(&file_hdr_, page);
    return page_hdl;
}

/**
 * @description: 创建一个新的page handle
 * @return {RmPageHandle} 新的PageHandle
 */
RmPageHandle RmFileHandle::create_new_page_handle() {
    // 1. 使用缓冲池创建一个新的page
    PageId page_id;
    page_id.fd = fd_;
    Page *page = buffer_pool_manager_->new_page(&page_id);
    if (page == nullptr) {
        throw std::runtime_error("Failed to create new page");
    }

    // 2. 更新page handle的相关信息
    RmPageHandle page_hdl(&file_hdr_, page);
    Bitmap::init(page_hdl.bitmap, file_hdr_.bitmap_size);
    page_hdl.page_hdr->num_records = 0;
    page_hdl.page_hdr->next_free_page_no = RM_NO_PAGE;

    // 3. 更新file_hdr_
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
    if (page_no == RM_NO_PAGE) {
        return create_new_page_handle();
    }
    // 2. 生成page handle并返回给上层
    return fetch_page_handle(page_no);
}

/**
 * @description: 当一个页面从没有空闲空间的状态变为有空闲空间状态时，更新文件头和页头中空闲页面相关的元数据
 */
void RmFileHandle::release_page_handle(RmPageHandle &page_handle) {
    // 1. 更新文件头和页头中空闲页面相关的元数据
    page_handle.page_hdr->next_free_page_no = file_hdr_.first_free_page_no;
    file_hdr_.first_free_page_no = page_handle.page->get_page_id().page_no;
}

void RmFileHandle::allocpage(Rid& rid){
    while(rid.page_no >= get_file_hdr().num_pages) {
        RmPageHandle rm_page_hd = create_new_page_handle();
//        free_pageno_set.insert(rm_page_hd.page->get_page_id().page_no);
        release_page_handle(rm_page_hd);
        buffer_pool_manager_->unpin_page(rm_page_hd.page->get_page_id(), false);
    }
}