/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <climits>
#include "ix_index_handle.h"

#include "ix_scan.h"

/**
 * @brief 在当前node中查找第一个>=target的key_idx
 *
 * @return key_idx，范围为[0,num_key)，如果返回的key_idx=num_key，则表示target大于最后一个key
 * @note 返回key index（同时也是rid index），作为slot no
 */
int IxNodeHandle::lower_bound(const char *target) const {
    // 查找当前节点中第一个大于等于target的key，并返回key的位置给上层

    // 提示: 可以采用多种查找方式，如顺序遍历、二分查找等；使用ix_compare()函数进行比较

    // 顺序查找
    int key_idx = 0;
    for (; key_idx < page_hdr->num_key; key_idx++) {
        if (ix_compare(target, get_key(key_idx), file_hdr->col_types_, file_hdr->col_lens_) <= 0) {
            break;
        }
    }
    return key_idx;
}

/**
 * @brief 在当前node中查找第一个>target的key_idx
 *
 * @return key_idx，范围为[1,num_key)，如果返回的key_idx=num_key，则表示target大于等于最后一个key
 * @note 注意此处的范围从1开始
 */
int IxNodeHandle::upper_bound(const char *target) const {
    // 查找当前节点中第一个大于target的key，并返回key的位置给上层

    // 提示: 可以采用多种查找方式：顺序遍历、二分查找等；使用ix_compare()函数进行比较
    // 顺序查找
    int key_idx = 1;
    for (; key_idx < page_hdr->num_key; key_idx++) {
        if (ix_compare(target, get_key(key_idx), file_hdr->col_types_, file_hdr->col_lens_) < 0) {
            break;
        }
    }
    return key_idx;
}

/**
 * @brief 用于叶子结点根据key来查找该结点中的键值对
 * 值value作为传出参数，函数返回是否查找成功
 *
 * @param key 目标key
 * @param[out] value 传出参数，目标key对应的Rid
 * @return 目标key是否存在
 */
bool IxNodeHandle::leaf_lookup(const char *key, Rid **value) {
    int tar_idx = lower_bound(key);
    char *tar_key = get_key(tar_idx);
    if (tar_key == nullptr || std::strcmp(tar_key, key) != 0
        || ix_compare(tar_key, key, file_hdr->col_types_, file_hdr->col_lens_) != 0
        || tar_idx == get_size()) {
        return false;
    }
    *value = get_rid(tar_idx);

    return true;
}

/**
 * 用于内部结点（非叶子节点）查找目标key所在的孩子结点（子树）
 * @param key 目标key
 * @return page_id_t 目标key所在的孩子节点（子树）的存储页面编号
 */
page_id_t IxNodeHandle::internal_lookup(const char *key) {
    int key_idx = upper_bound(key);
    key_idx--;

    return value_at(key_idx);

}

/**
 * @brief 在指定位置插入n个连续的键值对
 * 将key的前n位插入到原来keys中的pos位置；将rid的前n位插入到原来rids中的pos位置
 *
 * @param pos 要插入键值对的位置
 * @param (key, rid) 连续键值对的起始地址，也就是第一个键值对，可以通过(key, rid)来获取n个键值对
 * @param n 键值对数量
 * @note [0,pos)           [pos,num_key)
 *                            key_slot
 *                            /      \
 *                           /        \
 *       [0,pos)     [pos,pos+n)   [pos+n,num_key+n)
 *                      key           key_slot
 */
void IxNodeHandle::insert_pairs(int pos, const char *key, const Rid *rid, int n) {
    // 1. 判断pos的合法性
    // 0 - get_size
    assert(pos <= get_size() && pos >= 0);
    // 2. 通过key获取n个连续键值对的key值，并把n个key值插入到pos位置

    int key_len = file_hdr->col_tot_len_;
    int rid_len = sizeof(Rid);
    int num = get_size() - pos;

    char *begin_key = get_key(pos);
    //memmove 能够保证同数组移动的时候不会重叠覆盖
    memmove(begin_key + n * key_len, begin_key, num * key_len);
    memcpy(begin_key, key, n * key_len);

    // 3. 通过rid获取n个连续键值对的rid值，并把n个rid值插入到pos位置

    Rid *begin_rid = get_rid(pos);
    memmove(begin_rid + n, begin_rid, num * rid_len);
    memcpy(begin_rid, rid, n * rid_len);
    // 4. 更新当前节点的键数量

    set_size(get_size() + n);

}

/**
 * @brief 用于在结点中插入单个键值对。
 * 函数返回插入后的键值对数量
 *
 * @param (key, value) 要插入的键值对
 * @return int 键值对数量
 */
int IxNodeHandle::insert(const char *key, const Rid &value) {
    int pos = lower_bound(key);
    if (pos == get_size() ||
        ix_compare(get_key(pos), key, file_hdr->col_types_, file_hdr->col_lens_) > 0) {
        insert_pair(pos, key, value);
    }
    return get_size();
}

/**
 * @brief 用于在结点中的指定位置删除单个键值对
 *
 * @param pos 要删除键值对的位置
 */
void IxNodeHandle::erase_pair(int pos) {
    assert(pos >= 0 && pos < get_size());

    int n = get_size() - pos - 1;

    int key_size = file_hdr->col_tot_len_;
    char *key = get_key(pos);
    memmove(key, key + key_size, n * key_size);

    int rid_size = sizeof(Rid);
    Rid *rid = get_rid(pos);
    char *rid_ptr = reinterpret_cast<char *>(rid);
    memmove(rid_ptr, rid_ptr + rid_size, n * rid_size);

    page_hdr->num_key--;

}

/**
 * @brief 用于在结点中删除指定key的键值对。函数返回删除后的键值对数量
 *
 * @param key 要删除的键值对key值
 * @return 完成删除操作后的键值对数量
 */
int IxNodeHandle::remove(const char *key) {
    int key_idx = lower_bound(key);
    if (key_idx != page_hdr->num_key
        && (ix_compare(get_key(key_idx), key, file_hdr->col_types_, file_hdr->col_lens_) == 0)) {
        erase_pair(key_idx);
    }
    return page_hdr->num_key;
}

IxIndexHandle::IxIndexHandle(DiskManager *disk_manager, BufferPoolManager *buffer_pool_manager, int fd)
        : disk_manager_(disk_manager), buffer_pool_manager_(buffer_pool_manager), fd_(fd) {
    // init file_hdr_
    disk_manager_->read_page(fd, IX_FILE_HDR_PAGE, (char *) &file_hdr_, sizeof(file_hdr_));
    char buf[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    disk_manager_->read_page(fd, IX_FILE_HDR_PAGE, buf, PAGE_SIZE);
    file_hdr_ = new IxFileHdr();
    file_hdr_->deserialize(buf);

    // disk_manager管理的fd对应的文件中，设置从file_hdr_->num_pages开始分配page_no
    int now_page_no = disk_manager_->get_fd2pageno(fd);
    disk_manager_->set_fd2pageno(fd, now_page_no + 1);
}

// IxIndexHandle的析构函数
IxIndexHandle::~IxIndexHandle() {
    delete file_hdr_;
}

/**
 * @brief 用于查找指定键所在的叶子结点
 * @param key 要查找的目标key值
 * @param operation 查找到目标键值对后要进行的操作类型
 * @param transaction 事务参数，如果不需要则默认传入nullptr
 * @return [leaf node] and [root_is_latched] 返回目标叶子结点以及根结点是否加锁
 * @note need to Unlatch and unpin the leaf node outside!
 * 注意：用了FindLeafPage之后一定要unlatch叶结点，否则下次latch该结点会堵塞！
 */
std::pair<IxNodeHandle *, bool> IxIndexHandle::find_leaf_page(const char *key, Operation operation,
                                                              Transaction *transaction, bool find_first) {
    // 1. 获取根节点
    IxNodeHandle *root_hdl = fetch_node(file_hdr_->root_page_);
    IxNodeHandle *cur_hdl = root_hdl;

    // 2. 从根节点不断向下查找目标key
    while (!cur_hdl->is_leaf_page()) {
        page_id_t child_page_no = cur_hdl->internal_lookup(key);
        buffer_pool_manager_->unpin_page(cur_hdl->get_page_id(), false);
        delete cur_hdl;
        cur_hdl = fetch_node(child_page_no);
    }

    // 3. 找到包含该key值的叶子结点停止查找，并返回叶子节点
    return {cur_hdl, find_first};
//    for(int i = 0; i < cur_hdl->get_size(); i++){
//        if(cur_hdl->key_at(i) == *(int *)key) return {cur_hdl, find_first};
//    }
//    return {nullptr,false};
}

/**
 * @brief 用于查找指定键在叶子结点中的对应的值result
 *
 * @param key 查找的目标key值
 * @param result 用于存放结果的容器
 * @param transaction 事务指针
 * @return bool 返回目标键值对是否存在
 */
bool IxIndexHandle::get_value(const char *key, std::vector<Rid> *result, Transaction *transaction) {
    std::scoped_lock lock{root_latch_};

    // 1. 获取目标key值所在的叶子结点
    auto leaf_hdl = find_leaf_page(key, Operation::FIND, transaction).first;

    // 2. 在叶子节点中查找目标key值的位置，并读取key对应的rid
    Rid *rid;
    if (!leaf_hdl->leaf_lookup(key, &rid)) {
        return false;
    }

    // 提示：使用完buffer_pool提供的page之后，记得unpin page；记得处理并发的上锁
    buffer_pool_manager_->unpin_page(leaf_hdl->get_page_id(), false);

    // 3. 把rid存入result参数中
    result->push_back(*rid);

    delete leaf_hdl;

    return true;

}

/**
 * @brief  将传入的一个node拆分(Split)成两个结点，在node的右边生成一个新结点new node
 * @param node 需要拆分的结点
 * @return 拆分得到的new_node
 * @note need to unpin the new node outside
 * 注意：本函数执行完毕后，原node和new node都需要在函数外面进行unpin
 */
IxNodeHandle *IxIndexHandle::split(IxNodeHandle *node) {
    // 1. 将原结点的键值对平均分配，右半部分分裂为新的右兄弟结点
    IxNodeHandle *new_hdl = create_node();
    //初始化新节点 page_hdr
    new_hdl->page_hdr->is_leaf = node->page_hdr->is_leaf;
    new_hdl->page_hdr->num_key = 0;
    new_hdl->page_hdr->parent = node->get_parent_page_no();
    new_hdl->page_hdr->next_free_page_no = node->page_hdr->next_free_page_no;
    // 2. 如果新的右兄弟结点是叶子结点，更新新旧节点的prev_leaf和next_leaf指针
    if (new_hdl->is_leaf_page()) {
        new_hdl->page_hdr->prev_leaf = node->get_page_no();
        new_hdl->page_hdr->next_leaf = node->get_next_leaf();
        node->page_hdr->next_leaf = new_hdl->get_page_no();

        IxNodeHandle *next_node = fetch_node(new_hdl->page_hdr->next_leaf);

        next_node->page_hdr->prev_leaf = new_hdl->get_page_no();
        buffer_pool_manager_->unpin_page(next_node->get_page_id(), true);

        delete next_node;
    }

    int pos = node->page_hdr->num_key / 2;
    int n = node->get_size() - pos;
    new_hdl->insert_pairs(0, node->get_key(pos), node->get_rid(pos), n);
    node->page_hdr->num_key = pos;
    for (int i = 0; i < n; i++) {
        maintain_child(new_hdl, i);
    }
    return new_hdl;
}

/**
 * @brief Insert key & value pair into internal page after split
 * 拆分(Split)后，向上找到old_node的父结点
 * 将new_node的第一个key插入到父结点，其位置在 父结点指向old_node的孩子指针 之后
 * 如果插入后>=maxsize，则必须继续拆分父结点，然后在其父结点的父结点再插入，即需要递归
 * 直到找到的old_node为根结点时，结束递归（此时将会新建一个根R，关键字为key，old_node和new_node为其孩子）
 *
 * @param (old_page, new_page) 原结点为old_page，old_page被分裂之后产生了新的右兄弟结点new_page
 * @param key 要插入parent的key
 * @note 一个结点插入了键值对之后需要分裂，分裂后左半部分的键值对保留在原结点，在参数中称为old_page，
 * 右半部分的键值对分裂为新的右兄弟节点，在参数中称为new_page（参考Split函数来理解old_page和new_page）
 * @note 本函数执行完毕后，new page和old page都需要在函数外面进行unpin
 */
void IxIndexHandle::insert_into_parent(IxNodeHandle *old_page, const char *key, IxNodeHandle *new_page,
                                       Transaction *transaction) {

    // 1. 分裂前的结点（原结点, old_page）是否为根结点，如果为根结点需要分配新的root
    if (old_page->is_root_page()) {
        // 创建一个新的根结点
        IxNodeHandle *new_root_page = create_node();

        // 初始化新的根结点
        new_root_page->page_hdr->num_key = 0;
        new_root_page->page_hdr->is_leaf = 0;
        new_root_page->page_hdr->parent = INVALID_PAGE_ID;
        new_root_page->page_hdr->next_free_page_no = IX_NO_PAGE;

        // 将old_page和new_page插入新的根结点
        new_root_page->insert_pair(0, old_page->get_key(0), {old_page->get_page_no(), -1});
        new_root_page->insert_pair(1, key, {new_page->get_page_no(), -1});

        // 更新文件头部的根结点页号
        int new_root_page_no = new_root_page->get_page_no();
        file_hdr_->root_page_ = new_root_page_no;

        // 更新old_page和new_page的父节点
        new_page->page_hdr->parent = new_root_page_no;
        old_page->page_hdr->parent = new_root_page_no;

        // 释放new_root_page
        delete new_root_page;
    } else {

        // 2. 获取原结点（old_page）的父亲结点
        IxNodeHandle *parent_page = fetch_node(old_page->get_parent_page_no());

        // 3. 获取key对应的rid，并将(key, rid)插入到父亲结点
        int child_idx = parent_page->find_child(old_page);
        parent_page->insert_pair(child_idx + 1, key, {new_page->get_page_id().page_no, -1});

        // 4. 如果父亲结点仍需要继续分裂，则进行递归插入
        if (parent_page->get_size() == parent_page->get_max_size()) {
            // 分裂父节点
            IxNodeHandle *new_parent_page = split(parent_page);

            // 递归将new_parent_page的第一个key插入到其父节点中
            insert_into_parent(parent_page, new_parent_page->get_key(0), new_parent_page, transaction);

            // 解除固定新的父节点页面
            buffer_pool_manager_->unpin_page(new_parent_page->get_page_id(), true);

            // 释放new_parent_page
            delete new_parent_page;
        }
        // 解除固定父节点页面
        buffer_pool_manager_->unpin_page(parent_page->get_page_id(), true);

        // 释放parent_page
        delete parent_page;
    }
}


/**
 * @brief 将指定键值对插入到B+树中
 * @param (ins_key, ins_value) 要插入的键值对
 * @param txn 事务指针
 * @return page_id_t 插入到的叶结点的page_no
 */
page_id_t IxIndexHandle::insert_entry(const char *ins_key, const Rid &ins_value, Transaction *txn) {
    // 加锁以确保操作的原子性
    std::scoped_lock tree_lock{root_latch_};

    // 1. 查找ins_key值应该插入到哪个叶子节点
    std::pair<IxNodeHandle *, bool> leaf_page_result = find_leaf_page(ins_key, Operation::INSERT, txn);

    // 找不到应该插入哪个叶子节点
    if (!leaf_page_result.first) {
        throw IndexEntryNotFoundError();
    }

    // 获取叶子节点句柄
    IxNodeHandle *leaf_node_handle = leaf_page_result.first;

    // 检查是否存在重复的键
    if (leaf_node_handle->is_exist_key(ins_key)) {
        throw InternalError("Non-unique index!");
    }

    // 获取当前叶子节点的大小
    int current_size = leaf_node_handle->get_size();

    // 2. 在该叶子节点中插入键值对
    if (leaf_node_handle->insert(ins_key, ins_value) == current_size) {
        // 如果插入后键值对数量不变，解除页面固定并返回-1
        buffer_pool_manager_->unpin_page(leaf_node_handle->get_page_id(), false);
        return -1;
    } else if (leaf_node_handle->get_size() == leaf_node_handle->get_max_size()) {
        // 3. 如果结点已满，分裂结点，并把新结点的相关信息插入父节点
        IxNodeHandle *new_leaf_node = split(leaf_node_handle);

        // 如果该叶子是最后一片叶子，更新文件头部的最后叶子节点页号
        if (leaf_node_handle->get_page_no() == file_hdr_->last_leaf_) {
            file_hdr_->last_leaf_ = new_leaf_node->get_page_no();
        }

        // 将新叶子节点的第一个键插入到父节点
        insert_into_parent(leaf_node_handle, new_leaf_node->get_key(0), new_leaf_node, txn);

        // 解除页面固定
        buffer_pool_manager_->unpin_page(leaf_node_handle->get_page_id(), true);

        // 释放新叶子节点句柄
        delete new_leaf_node;
    }

    // 解除页面固定
    buffer_pool_manager_->unpin_page(leaf_node_handle->get_page_id(), false);

    // 获取叶子节点的页面编号
    auto leaf_page_id = leaf_node_handle->get_page_no();

    // 释放叶子节点句柄
    delete leaf_node_handle;

    return leaf_page_id;
}


/**
 * @brief 用于删除B+树中含有指定key的键值对
 * @param key 要删除的key值
 * @param transaction 事务指针
 * @return 如果删除成功返回true，否则返回false
 */
bool IxIndexHandle::delete_entry(const char *key, Transaction *transaction) {
    // 加锁以确保操作的原子性
    std::scoped_lock tree_lock{root_latch_};
    bool is_root_locked = true;

    // 1. 获取该键值对所在的叶子结点
    auto leaf_page_result = find_leaf_page(key, Operation::DELETE, transaction);
    IxNodeHandle *leaf_node_handle = leaf_page_result.first;

    // 2. 在该叶子结点中删除键值对
    if (leaf_node_handle->get_size() == leaf_node_handle->remove(key)) {
        // 2.1 删除失败，解锁并返回false
        buffer_pool_manager_->unpin_page(leaf_node_handle->get_page_id(), false);

        delete leaf_node_handle;
        return false;
    }

    // 3. 如果删除成功需要调用CoalesceOrRedistribute来进行合并或重分配操作，并根据函数返回结果判断是否有结点需要删除
    coalesce_or_redistribute(leaf_node_handle, transaction, &is_root_locked);
    buffer_pool_manager_->unpin_page(leaf_node_handle->get_page_id(), true);

    // 删除叶子结点句柄
    delete leaf_node_handle;
    return true;
}


/**
 * @brief 用于处理合并和重分配的逻辑，用于删除键值对后调用
 *
 * @param target_node 执行完删除操作的结点
 * @param txn 事务指针
 * @param is_root_locked 传出参数：根节点是否上锁，用于并发操作
 * @return 是否需要删除结点
 * @note User needs to first find the sibling of input page.
 * If sibling's size + input page's size >= 2 * page's minsize, then redistribute.
 * Otherwise, merge(Coalesce).
 */
bool IxIndexHandle::coalesce_or_redistribute(IxNodeHandle *target_node, Transaction *txn, bool *is_root_locked) {
    // 1. 判断target_node结点是否为根节点
    if (target_node->is_root_page()) {
        //    1.1 如果是根节点，需要调用adjust_root() 函数来进行处理，返回根节点是否需要被删除
        return adjust_root(target_node);
    } else {
        // 1.2 如果不是根节点，并且不需要执行合并或重分配操作，则直接返回false
        if (target_node->get_size() >= target_node->get_min_size()) {
            // 确保父节点的正确性
            maintain_parent(target_node);
            return false;
        }
    }
    // TODO:这个地方有问题，是否需要删除节点

//    // 2. 获取target_node结点的父亲结点
//    IxNodeHandle *parent_node = fetch_node(target_node->get_parent_page_no());
//
//    // 3. 寻找target_node结点的兄弟结点（优先选取前驱结点）
//    IxNodeHandle *sibling_node = nullptr;
//    bool is_left_sibling = false;
//    int sibling_index = parent_node->find_sibling(target_node->get_page_no(), &is_left_sibling);
//    if (is_left_sibling) {
//        sibling_node = fetch_node(parent_node->get_child_page_no(sibling_index));
//    } else {
//        sibling_node = fetch_node(parent_node->get_child_page_no(sibling_index + 1));
//    }
//
//    // 4. 如果target_node结点和兄弟结点的键值对数量之和，能够支撑两个B+树结点（即node.size + sibling.size >= 2 * NodeMinSize）
//    if (target_node->get_size() + sibling_node->get_size() >= 2 * target_node->get_min_size()) {
//        // 重新分配键值对（调用redistribute函数）
//        redistribute(target_node, sibling_node, parent_node, is_left_sibling);
//    } else {
//        // 5. 如果不满足上述条件，则需要合并两个结点，将右边的结点合并到左边的结点（调用coalesce函数）
//        coalesce(target_node, sibling_node, parent_node, is_left_sibling);
//    }
//
//    // 解除页面固定
//    buffer_pool_manager_->unpin_page(parent_node->get_page_id(), true);
//    buffer_pool_manager_->unpin_page(sibling_node->get_page_id(), true);
//
//    // 释放节点句柄
//    delete parent_node;
//    delete sibling_node;
//
//    return true;
}


/**
 * @brief 用于当根结点被删除了一个键值对之后的处理
 * @param original_root_node 原根节点
 * @return bool 根结点是否需要被删除
 * @note size of root page can be less than min size and this method is only called within coalesce_or_redistribute()
 */
bool IxIndexHandle::adjust_root(IxNodeHandle *original_root_node) {
    // 1. 如果original_root_node是内部结点，并且大小为1，则直接把它的孩子更新成新的根结点
    if (!original_root_node->is_leaf_page() && original_root_node->get_size() == 1) {
        // 获取新的根节点句柄
        IxNodeHandle *new_root_node_handle = fetch_node(original_root_node->value_at(0));
        // 设置新的根节点的父节点为空
        new_root_node_handle->set_parent_page_no(INVALID_PAGE_ID);
        // 更新文件头中的根节点页面号
        file_hdr_->root_page_ = new_root_node_handle->get_page_no();
        // 解除页面固定
        buffer_pool_manager_->unpin_page(new_root_node_handle->get_page_id(), true);
        // 释放旧的根节点句柄
        release_node_handle(*original_root_node);

        // 删除新的根节点句柄
        delete new_root_node_handle;

        return true;
    }
    // 2. 如果original_root_node是叶结点，且大小为0，则直接更新root page
    if (original_root_node->is_leaf_page() && original_root_node->get_size() == 0) {
        // 释放旧的根节点句柄
        release_node_handle(*original_root_node);
        // 将文件头中的根节点页面号设置为无效
        file_hdr_->root_page_ = INVALID_PAGE_ID;
        return true;
    }
    // 3. 除了上述两种情况，不需要进行操作
    return false;
}


/**
 * @brief 重新分配node和兄弟结点sibling_node的键值对
 * Redistribute key & value pairs from one page to its sibling page. If position == 0, move sibling page's first key
 * & value pair into end of input "node", otherwise move sibling page's last key & value pair into head of input "node".
 *
 * @param sibling_node sibling page of input "node"
 * @param target_node input from method coalesceOrRedistribute()
 * @param parent_node the parent of "target_node" and "sibling_node"
 * @param position target_node在parent_node中的rid_idx
 * @note target_node是之前刚被删除过一个key的结点
 * position=0，则sibling_node是target_node后继结点，表示：target_node(left)      sibling_node(right)
 * position>0，则sibling_node是target_node前驱结点，表示：sibling_node(left)  target_node(right)
 * 注意更新parent_node结点的相关kv对
 */
void IxIndexHandle::redistribute(IxNodeHandle *sibling_node, IxNodeHandle *target_node, IxNodeHandle *parent_node,
                                 int position) {
    // 1. 通过position判断sibling_node是否为target_node的前驱结点
    if (position != 0) {
        // 2. 从sibling_node中移动一个键值对到target_node结点中
        int sibling_last_index = sibling_node->get_size() - 1;
        target_node->insert_pair(0, sibling_node->get_key(sibling_last_index),
                                 *sibling_node->get_rid(sibling_last_index));
        sibling_node->erase_pair(sibling_last_index);

        // 3. 更新父节点中的相关信息，并且修改移动键值对对应孩字结点的父结点信息（maintain_child函数）
        // pair移到target_node里了，所以只需target_node维护child的parent
        maintain_child(target_node, target_node->get_size() - 1);
        // 插入到target_node的最前面，所以parent_node对应的key要改变
        maintain_parent(target_node);

    } else {
        // 2. 从sibling_node中移动一个键值对到target_node结点中
        target_node->insert_pair(target_node->get_size(), sibling_node->get_key(0), *sibling_node->get_rid(0));
        sibling_node->erase_pair(0);
        // 更新父节点中的相关信息，并且修改移动键值对对应孩字结点的父结点信息（maintain_child函数）
        maintain_parent(sibling_node);
        maintain_child(target_node, target_node->get_size() - 1);
    }
}


/**
 * @brief 合并(Coalesce)函数是将node和其直接前驱进行合并，也就是和它左边的neighbor_node进行合并；
 * 假设node一定在右边。如果上层传入的index=0，说明node在左边，那么交换node和neighbor_node，保证node在右边；合并到左结点，实际上就是删除了右结点；
 * Move all the key & value pairs from one page to its sibling page, and notify buffer pool manager to delete this page.
 * Parent page must be adjusted to take info of deletion into account. Remember to deal with coalesce or redistribute
 * recursively if necessary.
 *
 * @param neighbor_node sibling page of input "node" (neighbor_node是node的前结点)
 * @param node input from method coalesceOrRedistribute() (node结点是需要被删除的)
 * @param parent parent page of input "node"
 * @param idx node在parent中的rid_idx
 * @return true means parent node should be deleted, false means no deletion happend
 * @note Assume that *neighbor_node is the left sibling of *node (neighbor -> node)
 */
bool IxIndexHandle::coalesce(IxNodeHandle **neighbor_node, IxNodeHandle **node, IxNodeHandle **parent, int idx,
                             Transaction *transaction, bool *root_is_latched) {
    // Todo:
    // 1. 用index判断neighbor_node是否为node的前驱结点，若不是则交换两个结点，让neighbor_node作为左结点，node作为右结点
    // 2. 把node结点的键值对移动到neighbor_node中，并更新node结点孩子结点的父节点信息（调用maintain_child函数）
    // 3. 释放和删除node结点，并删除parent中node结点的信息，返回parent是否需要被删除
    // 提示：如果是叶子结点且为最右叶子结点，需要更新file_hdr_.last_leaf

    // 1. 用index判断neighbor_node是否为node的前驱结点，若不是则交换两个结点，让neighbor_node作为左结点，node作为右结点
    if (idx == 0) {
        IxNodeHandle **tmp = node;
        node = neighbor_node;
        neighbor_node = tmp;
        idx++;
    }

    // 2. 把node结点的键值对移动到neighbor_node中，并更新node结点孩子结点的父节点信息（调用maintain_child函数）
    int before_size = (*neighbor_node)->get_size();
    (*neighbor_node)->insert_pairs(before_size, (*node)->get_key(0), (*node)->get_rid(0), (*node)->get_size());
    int after_size = (*neighbor_node)->get_size();
    for (int i = before_size; i < after_size; ++i) {
        maintain_child(*neighbor_node, i);
    }

    // 提示：如果是叶子结点且为最右叶子结点，需要更新file_hdr_.last_leaf
    if ((*node)->get_page_no() == file_hdr_->last_leaf_) {
        file_hdr_->last_leaf_ = (*neighbor_node)->get_page_no();
    }

    // 3. 释放和删除node结点，并删除parent中node结点的信息，返回parent是否需要被删除
    erase_leaf(*node);
    release_node_handle(**node);
    (*parent)->erase_pair(idx);

    return coalesce_or_redistribute(*parent, transaction, root_is_latched);
}

/**
 * @brief 这里把iid转换成了rid，即iid的slot_no作为node的rid_idx(key_idx)
 * node其实就是把slot_no作为键值对数组的下标
 * 换而言之，每个iid对应的索引槽存了一对(key,rid)，指向了(要建立索引的属性首地址,插入/删除记录的位置)
 *
 * @param iid
 * @return Rid
 * @note iid和rid存的不是一个东西，rid是上层传过来的记录位置，iid是索引内部生成的索引槽位置
 */
Rid IxIndexHandle::get_rid(const Iid &iid) const {
    IxNodeHandle *node = fetch_node(iid.page_no);
    if (iid.slot_no >= node->get_size()) {
        throw IndexEntryNotFoundError();
    }
    buffer_pool_manager_->unpin_page(node->get_page_id(), false);  // unpin it!
    auto rid = *(node->get_rid(iid.slot_no));
    delete node;
    return rid;
}

/**
 * @brief FindLeafPage + lower_bound
 *
 * @param key 要查找的键值
 * @return NId 返回包含页号和槽号的Iid结构
 * @note 上层传入的key本来是int类型，通过(const char *)&key进行了转换
 * 可用*(int *)key转换回去
 */
Iid IxIndexHandle::lower_bound(const char *key) {
    // 先找到对应的叶子节点
    std::pair<IxNodeHandle *, bool> leaf_page_entry = find_leaf_page(key, Operation::FIND, nullptr);
    if (!leaf_page_entry.first) {
        // 如果找不到叶子节点，返回无效的Nid
        return Iid{-1, -1};
    }
    IxNodeHandle *leaf_node = leaf_page_entry.first;

    // 在叶子节点中找到第一个不小于key的键值对位置
    int lower_bound_idx = leaf_node->lower_bound(key);
    Iid result_iid = {.page_no = leaf_node->get_page_no(), .slot_no = lower_bound_idx};

    // 解锁并释放叶子节点
    buffer_pool_manager_->unpin_page(leaf_node->get_page_id(), false);
    delete leaf_node;

    return result_iid;
}

/**
 * @brief FindLeafPage + upper_bound
 *
 * @param key 要查找的键值
 * @return NId 返回包含页号和槽号的Iid结构
 */
Iid IxIndexHandle::upper_bound(const char *key) {
    // 先找到对应的叶子节点
    std::pair<IxNodeHandle *, bool> leaf_page_entry = find_leaf_page(key, Operation::FIND, nullptr);
    if (!leaf_page_entry.first) {
        // 如果找不到叶子节点，返回无效的Iid
        return Iid{-1, -1};
    }
    IxNodeHandle *leaf_node = leaf_page_entry.first;

    // 在叶子节点中找到第一个大于key的键值对位置
    int upper_bound_idx = leaf_node->upper_bound(key);
    Iid result_iid;

    // 如果upper_bound_idx等于节点中的键值对数量，说明key大于节点中的所有键值
    if (upper_bound_idx == leaf_node->get_size()) {
        result_iid = leaf_end();
    } else {
        result_iid = {.page_no = leaf_node->get_page_no(), .slot_no = upper_bound_idx};
    }

    // 解锁并释放叶子节点
    buffer_pool_manager_->unpin_page(leaf_node->get_page_id(), false);
    delete leaf_node;

    return result_iid;
}

/**
 * @brief 指向最后一个叶子的最后一个结点的后一个
 * 用处在于可以作为IxScan的最后一个
 *
 * @return Iid
 */
Iid IxIndexHandle::leaf_end() const {
    IxNodeHandle *node = fetch_node(file_hdr_->last_leaf_);
    Iid iid = {.page_no = file_hdr_->last_leaf_, .slot_no = node->get_size()};
    buffer_pool_manager_->unpin_page(node->get_page_id(), false);
    delete node;
    return iid;
}

/**
 * @brief 指向第一个叶子的第一个结点
 * 用处在于可以作为IxScan的第一个
 *
 * @return NId
 */
Iid IxIndexHandle::leaf_begin() const {
    Iid iid = {.page_no = file_hdr_->first_leaf_, .slot_no = 0};
    return iid;
}

/**
 * @brief 获取一个指定结点
 *
 * @param page_no
 * @return IxNodeHandle*
 * @note pin the page, remember to unpin it outside!
 */
IxNodeHandle *IxIndexHandle::fetch_node(int page_no) const {
    Page *page = buffer_pool_manager_->fetch_page(PageId{fd_, page_no});
    auto *node = new IxNodeHandle(file_hdr_, page);
    return node;
}

/**
 * @brief 获取一个指定结点
 *
 * @param page_no
 * @return IxNodeHandle*
 * @note pin the page, remember to unpin it outside!
 */
int IxIndexHandle::get_key(Iid iid) const {
    Page *page = buffer_pool_manager_->fetch_page(PageId{fd_, iid.page_no});
    auto *node = new IxNodeHandle(file_hdr_, page);
    if (iid.slot_no < 0) return INT_MIN;
    else return node->key_at(iid.slot_no);
}

/**
 * @brief 创建一个新结点
 *
 * @return IxNodeHandle*
 * @note pin the page, remember to unpin it outside!
 * 注意：对于Index的处理是，删除某个页面后，认为该被删除的页面是free_page
 * 而first_free_page实际上就是最新被删除的页面，初始为IX_NO_PAGE
 * 在最开始插入时，一直是create node，那么first_page_no一直没变，一直是IX_NO_PAGE
 * 与Record的处理不同，Record将未插入满的记录页认为是free_page
 */
IxNodeHandle *IxIndexHandle::create_node() {
    IxNodeHandle *node;
    file_hdr_->num_pages_++;

    PageId new_page_id = {.fd = fd_, .page_no = INVALID_PAGE_ID};
    // 从3开始分配page_no，第一次分配之后，new_page_id.page_no=3，file_hdr_.num_pages=4
    Page *page = buffer_pool_manager_->new_page(&new_page_id);
    node = new IxNodeHandle(file_hdr_, page);
    return node;
}

/**
 * @brief 从node开始更新其父节点的第一个key，一直向上更新直到根节点
 *
 * @param node
 */
void IxIndexHandle::maintain_parent(IxNodeHandle *node) {
    IxNodeHandle *curr = node;
    while (curr->get_parent_page_no() != IX_NO_PAGE) {
        IxNodeHandle *parent = fetch_node(curr->get_parent_page_no());
        int rank = parent->find_child(curr);
        char *parent_key = parent->get_key(rank);
        char *child_first_key = curr->get_key(0);

        if (memcmp(parent_key, child_first_key, file_hdr_->col_tot_len_) == 0) {
            assert(buffer_pool_manager_->unpin_page(parent->get_page_id(), true));
            break;
        }

        memcpy(parent_key, child_first_key, file_hdr_->col_tot_len_);  // 修改了parent node

        if (curr != node) {
            delete curr;
        }
        curr = parent;

        assert(buffer_pool_manager_->unpin_page(parent->get_page_id(), true));
    }
    if (curr != node) {
        delete curr;
    }
}

/**
 * @brief 要删除leaf之前调用此函数，更新leaf前驱结点的next指针和后继结点的prev指针
 *
 * @param leaf 要删除的leaf
 */
void IxIndexHandle::erase_leaf(IxNodeHandle *leaf) {
    assert(leaf->is_leaf_page());

    IxNodeHandle *prev = fetch_node(leaf->get_prev_leaf());
    prev->set_next_leaf(leaf->get_next_leaf());
    buffer_pool_manager_->unpin_page(prev->get_page_id(), true);

    delete prev;

    IxNodeHandle *next = fetch_node(leaf->get_next_leaf());
    next->set_prev_leaf(leaf->get_prev_leaf());  // 注意此处是SetPrevLeaf()
    buffer_pool_manager_->unpin_page(next->get_page_id(), true);

    delete next;
}

/**
 * @brief 删除node时，更新file_hdr_.num_pages
 *
 * @param node
 */
void IxIndexHandle::release_node_handle(IxNodeHandle &node) {
    file_hdr_->num_pages_--;
}

/**
 * @brief 将node的第child_idx个孩子结点的父节点置为node
 */
void IxIndexHandle::maintain_child(IxNodeHandle *node, int child_idx) {
    if (!node->is_leaf_page()) {
        //  Current node is inner node, load its child and set its parent to current node
        // 非叶子节点
        int child_page_no = node->value_at(child_idx);
        IxNodeHandle *child = fetch_node(child_page_no);
        child->set_parent_page_no(node->get_page_no());
        buffer_pool_manager_->unpin_page(child->get_page_id(), true);

        delete child;
    }
}

/**
 * @brief 判断当前节点中是否有对应key的pair
 *
 * @param key 要查找的键值
 * @return 是否有对应key的pair
 * @note 遍历当前节点的所有键，检查是否存在与输入key相等的键
 */
bool IxNodeHandle::is_exist_key(const char *key) const {
    // 获取当前节点中键的数量
    int num_keys = page_hdr->num_key;

    // 遍历当前节点中的所有键
    for (int key_index = 0; key_index < num_keys; key_index++) {
        // 获取当前键的地址
        char *current_key_addr = get_key(key_index);

        // 比较输入的key和当前键
        if (ix_compare(key, current_key_addr, file_hdr->col_types_, file_hdr->col_lens_) == 0) {
            // 如果相等，返回true
            return true;
        }
    }

    // 如果遍历完所有键都没有找到相等的，返回false
    return false;
}