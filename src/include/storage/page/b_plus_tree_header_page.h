#pragma once

#include "common/config.h"

class BPlusTreeHeaderPage {
public:
  // Delete all constructor / destructor to ensure memory safety
  BPlusTreeHeaderPage() = delete;
  BPlusTreeHeaderPage(const BPlusTreeHeaderPage &other) = delete;

  page_id_t root_page_id_;
  int allocate_cnt_;
};
