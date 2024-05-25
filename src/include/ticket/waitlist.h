#pragma once

#include "common/time.h"
#include "storage/index/b_plus_tree.h"

struct WaitInfo {
  int32_t timestamp_{};
  int32_t num_{};
  char username_[21]{};
  int8_t start_pos_{};
  int8_t end_pos_{};
  bool queue{true};
};

class WaitList {
public:
  WaitList() = delete;
  explicit WaitList(shared_ptr<BufferPoolManager> bpm);
  class iterator {
  public:
    iterator() = default;
    iterator(shared_ptr<BufferPoolManager> bpm, WritePageGuard guard, int pos);
    iterator &operator++();
    WaitInfo &operator*();
    [[nodiscard]] bool IsEnd() const;

  private:
    shared_ptr<BufferPoolManager> bpm_;
    WritePageGuard guard_{};
    int pos{};
  };

private:
  shared_ptr<BufferPoolManager> bpm_;
  BPlusTree<pair<unsigned long long, Date>, page_id_t, std::less<>> index_;
  page_id_t next_tuple_id_;
  int32_t timestamp_;
};