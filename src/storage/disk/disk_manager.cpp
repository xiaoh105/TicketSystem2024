#include <cassert>

#include "storage/disk/disk_manager.h"

DiskManager::DiskManager(const std::string &file_name) {
  io_.open(file_name);
  if (!io_) {
    io_.open(file_name, std::ios::out);
    io_.close();
    io_.open(file_name);
    first_flag_ = true;
  }
}

DiskManager::~DiskManager() {
  io_.close();
}

void DiskManager::ReadPage(page_id_t page_id, char *data) {
  std::size_t offset = page_id * BUSTUB_PAGE_SIZE;
  std::scoped_lock latch(io_latch_);
  io_.seekg(offset); // NOLINT
  io_.read(data, BUSTUB_PAGE_SIZE);
  if (io_.bad()) {
    assert("IO error when reading page.");
  }
}

void DiskManager::WritePage(page_id_t page_id, const char *data) {
  std::size_t offset = page_id * BUSTUB_PAGE_SIZE;
  std::scoped_lock latch(io_latch_);
  io_.seekp(offset); // NOLINT
  io_.write(data, BUSTUB_PAGE_SIZE);
  if (io_.bad()) {
    assert("IO error when writing page.");
  }
  io_.flush();
}