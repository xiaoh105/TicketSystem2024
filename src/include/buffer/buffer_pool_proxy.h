#pragma once

#include <memory>
#include <condition_variable>

#include "buffer/replacer.h"
#include "common/config.h"
#include "common/stl/list.hpp"
#include "common/stl/map.hpp"
#include "storage/disk/disk_manager.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

/**
 * @brief A proxy that controls disk reading and writing (to maximize bandwidth).
 */
class BufferPoolProxy {
public:
  /**
   * Create and initialize a buffer pool proxy.
   * @param disk_manager The disk manager the proxy controls.
   */
  explicit BufferPoolProxy(DiskManager *disk_manager);

  /**
   * Waits until all writing is done and destroys the proxy.
   */
  ~BufferPoolProxy();

  /**
   * @brief The background writing done by the writing thread.
   * The background writing of the writing thread:
   * It checks the waiting list for any possible writing. If it isn't empty, write it to the disk.
   */
  void AsyncWrite();

  /**
   * @brief Fetch a page from the disk.
   * @param page_id The id of the page to be fetched.
   * @param page_data_ The array for the fetched data.
   * Fetch a page with a certain id:
   * (1) It first checks its own wait buffer for the page id. If it exists, return directly.
   * (2) Otherwise, search the disk for the page and returns it.
   */
  void ReadPage(page_id_t page_id, char *page_data_);

  /**
   * @brief Write a page to the disk.
   * @param page_id The id of the page to be written.
   * @param page_data The page data to be written.
   * Write a page with a certain id to the disk:
   * (1) If the waiting list has data under the same id, replace it.
   * (2) Otherwise, append the page to the waiting list.
   */
  void WritePage(page_id_t page_id, const char *page_data);

private:
  size_t version_{0};
  SpinLock latch_;
  std::thread write_thread_;
  DiskManager *disk_manager_;
  map<page_id_t, const char *> request_page_;
  map<page_id_t, size_t> request_version_;
  std::mutex write_signal_latch_;
  std::condition_variable write_signal_;
  std::atomic_bool end_signal_{false};
  char write_temp_[BUSTUB_PAGE_SIZE]{};
};
