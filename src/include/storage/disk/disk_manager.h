#ifndef TICKETSYSTEM_DISK_MANAGER_H
#define TICKETSYSTEM_DISK_MANAGER_H

#include <fstream>
#include <mutex>
#include "common/config.h"

/**
 * @brief A thread-safe class for disk read and write.
 */
class DiskManager {
 public:
  DiskManager() = delete;
  /**
   * @brief Create and initialize a disk manager.
   * @param file_name The name of target file.
   * Create a disk manager according to file_name.
   * If the file exists, open it; Otherwise, create and open it.
   */
  DiskManager(const std::string &file_name);
  ~DiskManager();
  void ReadPage(page_id_t page_id, char *data);
  void WritePage(page_id_t page_id, const char *data);

 private:
  std::mutex io_latch_;
  std::fstream io_;
};

#endif //TICKETSYSTEM_DISK_MANAGER_H
