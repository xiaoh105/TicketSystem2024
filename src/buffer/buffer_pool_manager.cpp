#include "buffer/buffer_pool_manager.h"
#include "storage/page/page_guard.h"
#include "storage/page/b_plus_tree_header_page.h"

BufferPoolManager::BufferPoolManager(size_t pool_size, unique_ptr<DiskManager> disk_manager, size_t replacer_k)
  : pool_size_(pool_size), disk_proxy_(make_unique<BufferPoolProxy>(std::move(disk_manager))) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_]{};
  page_lock_ = new SpinLock[pool_size]{};
  replacer_ = make_unique<LRUKReplacer>(pool_size, replacer_k);
  first_flag_ = disk_proxy_->IsFirstVisit();

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.push_back(static_cast<int>(i));
  }

  if (!first_flag_) {
    auto cur_guard = FetchPageRead(0);
    next_page_id_ = cur_guard.As<BPlusTreeHeaderPage>()->allocate_cnt_;
  }
}

BufferPoolManager::~BufferPoolManager() {
  auto cur_guard = FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->allocate_cnt_ = next_page_id_;
  cur_guard.Drop();
  FlushAllPages();
  delete[] pages_;
  delete[] page_lock_;
}

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  frame_id_t id;
  *page_id = AllocatePage();
  latch_.lock();
  if (!free_list_.empty()) {
    id = free_list_.front();
    replacer_->RecordAccess(id);
    replacer_->SetEvictable(id, false);
    free_list_.pop_front();
    page_table_[*page_id] = id;
    latch_.unlock();
  } else if (replacer_->Evict(&id)) {
    replacer_->RecordAccess(id);
    replacer_->SetEvictable(id, false);
    page_table_.erase(pages_[id].page_id_);
    page_table_[*page_id] = id;
    page_lock_[id].lock();
    if (pages_[id].IsDirty()) {
      disk_proxy_->WritePage(pages_[id].GetPageId(), pages_[id].GetData());
    }
    latch_.unlock();
  } else {
    latch_.unlock();
    return nullptr;
  }
  pages_[id].is_dirty_ = false;
  pages_[id].ResetMemory();
  pages_[id].page_id_ = *page_id;
  pages_[id].pin_count_ = 1;
  page_lock_[id].unlock();
  return &pages_[id];
}

auto BufferPoolManager::FetchPage(page_id_t page_id) -> Page * {
  latch_.lock();
  auto it = page_table_.find(page_id);
  auto id = (it == page_table_.end()) ? -1 : it->second;
  if (id != -1) {
    replacer_->SetEvictable(id, false);
    replacer_->RecordAccess(id);
    page_lock_[id].lock();
    latch_.unlock();
    ++pages_[id].pin_count_;
    page_lock_[id].unlock();
    return &pages_[id];
  }
  if (!free_list_.empty()) {
    id = free_list_.front();
    replacer_->RecordAccess(id);
    replacer_->SetEvictable(id, false);
    free_list_.pop_front();
    page_table_[page_id] = id;
    page_lock_[id].lock();
    latch_.unlock();
  } else if (replacer_->Evict(&id)) {
    replacer_->RecordAccess(id);
    replacer_->SetEvictable(id, false);
    page_lock_[id].lock();
    page_table_.erase(pages_[id].page_id_);
    page_table_[page_id] = id;
    if (pages_[id].IsDirty()) {
      disk_proxy_->WritePage(pages_[id].GetPageId(), pages_[id].GetData());
    }
    latch_.unlock();
  } else {
    latch_.unlock();
    return nullptr;
  }
  pages_[id].is_dirty_ = false;
  pages_[id].page_id_ = page_id;
  pages_[id].pin_count_ = 1;
  disk_proxy_->ReadPage(page_id, pages_[id].data_);
  page_lock_[id].unlock();
  return &pages_[id];
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) -> bool {
  latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    latch_.unlock();
    return false;
  }
  auto id = it->second;
  page_lock_[id].lock();
  if (pages_[id].pin_count_ <= 0) {
    latch_.unlock();
    page_lock_[id].unlock();
    return false;
  }
  pages_[id].is_dirty_ |= is_dirty;
  --pages_[id].pin_count_;
  if (pages_[id].pin_count_ == 0) {
    replacer_->SetEvictable(id, true);
  }
  latch_.unlock();
  page_lock_[id].unlock();
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  latch_.lock();
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    latch_.unlock();
    return false;
  }
  auto id = it->second;
  page_lock_[id].lock();
  latch_.unlock();
  disk_proxy_->WritePage(pages_[id].GetPageId(), pages_[id].data_);
  pages_[id].is_dirty_ = false;
  page_lock_[id].unlock();
  return true;
}

void BufferPoolManager::FlushAllPages() {
  latch_.lock();
  for (const auto &i : page_table_) {
    auto id = i.second;
    auto page_id = i.first;
    page_lock_[id].lock();
    disk_proxy_->WritePage(page_id, pages_[id].data_);
    pages_[id].is_dirty_ = false;
    page_lock_[id].unlock();
  }
  latch_.unlock();
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  latch_.lock();
  DeallocatePage(page_id);
  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    latch_.unlock();
    return true;
  }
  auto id = it->second;
  if (pages_[id].pin_count_ > 0) {
    latch_.unlock();
    return false;
  }
  replacer_->Remove(id);
  page_table_.erase(page_id);
  free_list_.push_back(id);
  page_lock_[id].lock();
  latch_.unlock();
  pages_[id].ResetMemory();
  pages_[id].page_id_ = INVALID_PAGE_ID;
  page_lock_[id].unlock();
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard {
  auto ret = FetchPage(page_id);
  while (ret == nullptr) {
    ret = FetchPage(page_id);
  }
  return {this, ret};
}

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  auto ret = FetchPage(page_id);
  while (ret == nullptr) {
    ret = FetchPage(page_id);
  }
  ret->RLatch();
  return {this, ret};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  auto ret = FetchPage(page_id);
  while (ret == nullptr) {
    ret = FetchPage(page_id);
  }
  ret->WLatch();
  return {this, ret};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard {
  auto ret = NewPage(page_id);
  while (ret == nullptr) {
    ret = NewPage(page_id);
  }
  return {this, ret};
}
