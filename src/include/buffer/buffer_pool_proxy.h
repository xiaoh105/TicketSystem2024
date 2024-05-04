#pragma once

#include <list>
#include <memory>
#include <unordered_map>
#include <condition_variable>

#include "buffer/replacer.h"
#include "common/config.h"
#include "storage/disk/disk_manager.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

class BufferPoolProxy {
public:
  explicit BufferPoolProxy(DiskManager *disk_manager);

  ~BufferPoolProxy();

  void AsyncWrite();

  void ReadPage(page_id_t page_id, char *page_data_);

  void WritePage(page_id_t page_id, const char *page_data);

private:
  size_t version_{0};
  SpinLock latch_;
  std::thread write_thread_;
  DiskManager *disk_manager_;
  std::unordered_map<page_id_t, const char *> request_page_;
  std::unordered_map<page_id_t, size_t> request_version_;
  std::mutex write_signal_latch_;
  std::condition_variable write_signal_;
  std::atomic_bool end_signal_{false};
  char write_temp_[BUSTUB_PAGE_SIZE]{};
};
