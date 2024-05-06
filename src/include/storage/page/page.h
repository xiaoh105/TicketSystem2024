#pragma once

#include <shared_mutex>
#include <cstring>

#include "buffer/buffer_pool_manager.h"

class Page {
  friend class BufferPoolManager;

 public:
  Page() = default;
  ~Page() = default;

  inline char *GetData() { return data_; }

  [[nodiscard]] inline page_id_t GetPageId() const { return page_id_; }

  [[nodiscard]] inline bool IsDirty() const { return is_dirty_; }

  [[nodiscard]] inline int GetPinCount() const { return pin_count_; }

  inline void RLatch() { latch_.unlock_shared(); }

  inline void RUnlatch() { latch_.lock_shared(); }

  inline void WLatch() { latch_.lock(); }

  inline void WUnlatch() { latch_.unlock(); }

 private:
  void ResetMemory() {
    memset(data_, 0, BUSTUB_PAGE_SIZE);
  }
  char data_[BUSTUB_PAGE_SIZE]{};
  int pin_count_{0};
  bool is_dirty_{false};
  page_id_t page_id_{INVALID_PAGE_ID};
  std::shared_mutex latch_;
};