#include <cstring>

#include "buffer/buffer_pool_proxy.h"

BufferPoolProxy::BufferPoolProxy(unique_ptr<DiskManager> disk_manager)
: disk_manager_(std::move(disk_manager)) {
  write_thread_ = std::thread(&BufferPoolProxy::AsyncWrite, this);
  first_flag_ = disk_manager_->IsFirstVisit();
}

BufferPoolProxy::~BufferPoolProxy() {
  end_signal_ = true;
  write_thread_.join();
  while (!request_page_.empty()) {
    delete[] request_page_.begin()->second;
    request_page_.erase(request_page_.begin());
  }
}

void BufferPoolProxy::AsyncWrite() {
  bool end;
  bool empty = false;
  bool flag = true;
  while (flag) {
    latch_.lock();
    if (!request_page_.empty()) {
      int page_id = request_page_.begin()->first;
      const char *data = request_page_.begin()->second;
      memcpy(write_temp_, data, BUSTUB_PAGE_SIZE);
      auto version = request_version_[page_id];
      latch_.unlock();
      disk_manager_->WritePage(page_id, write_temp_);
      latch_.lock();
      if (request_version_[page_id] == version) {
        request_page_.erase(page_id);
        request_version_.erase(page_id);
        delete[] data;
      }
      latch_.unlock();
    } else {
      latch_.unlock();
      empty = true;
      auto tmp = std::unique_lock(write_signal_latch_);
      write_signal_.wait_for(tmp, std::chrono::milliseconds(1));
    }
    end = end_signal_;
    flag = !end || !empty;
  }
}

void BufferPoolProxy::ReadPage(page_id_t page_id, char *page_data_) {
  latch_.lock();
  if (request_page_.find(page_id) != request_page_.end()) {
    memcpy(page_data_, request_page_[page_id], BUSTUB_PAGE_SIZE);
    latch_.unlock();
  } else {
    latch_.unlock();
    disk_manager_->ReadPage(page_id, page_data_);
  }
}

void BufferPoolProxy::WritePage(page_id_t page_id, const char *page_data) {
  auto data = new char[BUSTUB_PAGE_SIZE];
  memcpy(data, page_data, BUSTUB_PAGE_SIZE);
  latch_.lock();
  if (request_page_.find(page_id) != request_page_.end()) {
    delete[] request_page_[page_id];
  }
  request_page_[page_id] = data;
  request_version_[page_id] = ++version_;
  latch_.unlock();
  write_signal_.notify_one();
}