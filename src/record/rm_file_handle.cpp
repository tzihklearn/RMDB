#include "rm_file_handle.h"
#include <shared_mutex>

/**
 * @description: 获取当前表中记录号为rid的记录
 * @param {Rid&} rid 记录号，指定记录的位置
 * @param {Context*} context
 * @return {unique_ptr<RmRecord>} rid对应的记录对象指针
 */
std::unique_ptr<RmRecord> RmFileHandle::get_record(const Rid &rid, Context *context) const {
    // 加共享锁保护数据结构
    std::shared_lock<std::shared_mutex> lock{latch_};

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
Rid RmFileHandle::insert_record(char *buf, Context *context) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

    // 获取当前未满的page handle
    RmPageHandle page_hdl = create_page_handle();

    // 在page_hdl中找到空闲slot位置
    int record_size = file_hdr_.record_size;
    int record_nums = file_hdr_.num_records_per_page;
    int slot_no = Bitmap::first_bit(false, page_hdl.bitmap, record_nums);
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

    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);

    // 返回新插入的record的rid
    return Rid{page_hdl.page->get_page_id().page_no, slot_no};
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
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }

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
void RmFileHandle::delete_record(const Rid &rid, Context *context) {
    // 加独占锁保护数据结构
    std::unique_lock<std::shared_mutex> lock{latch_};

//    // 对record加X锁
//    if (context != nullptr) {
//        context->lock_mgr_->lock_exclusive_on_record(context->txn_, rid, fd_);
//    }

    // 获取指定记录所在的page handle
    RmPageHandle page_hdl = fetch_page_handle(rid.page_no);

    // 更新page_handle.page_hdr的数据结构
    if (!Bitmap::is_set(page_hdl.bitmap, rid.slot_no)) {
        buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    // 重置bitmap对应位
    Bitmap::reset(page_hdl.bitmap, rid.slot_no);

    // 如果page从满变为未满状态,调用release_page_handle()
    int record_nums = file_hdr_.num_records_per_page;
    if (page_hdl.page_hdr->num_records-- == record_nums) {
        release_page_handle(page_hdl);
    }
    buffer_pool_manager_->unpin_page(page_hdl.page->get_page_id(), true);
}

/**
 * @description: 更新记录文件中记录号为rid的记录
 * @param {Rid&} rid 要更新的记录的记录号（位置）
 * @param {char*} buf 新记录的数据
 * @param {Context*} context
 */
void RmFileHandle::update_record(const Rid &rid, char *buf, Context *context) {
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