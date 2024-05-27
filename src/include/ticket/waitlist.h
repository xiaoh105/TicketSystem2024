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
  class iterator {
  public:
    iterator() = default;
    iterator(shared_ptr<BufferPoolManager> bpm, WritePageGuard guard,
             int pos, const string &train_id, Date date);
    iterator &operator++();
    WaitInfo &operator*();
    [[nodiscard]] bool IsEnd();
    [[nodiscard]] const string &GetTrainId() const { return train_id_; }
    [[nodiscard]] Date GetDate() const { return date_; }
    explicit operator bool() const { return !train_id_.empty(); }

  private:
    shared_ptr<BufferPoolManager> bpm_;
    WritePageGuard guard_{};
    int pos_{};
    string train_id_{};
    Date date_{};
  };

  WaitList() = delete;

  explicit WaitList(shared_ptr<BufferPoolManager> bpm);

  ~WaitList();

  [[nodiscard]] iterator FetchWaitlist(const string &train_id, Date date);

  int32_t Insert(const string &train_id, Date date, const string &username_,
              int start_pos, int end_pos, int num);

  int32_t GetTimeStamp() { return ++timestamp_; };

private:
  void RemoveEmptyPage(const string &train_id, Date date, WritePageGuard &cur_guard);
  shared_ptr<BufferPoolManager> bpm_;
  unique_ptr<BPlusTree<pair<unsigned long long, Date>, page_id_t, std::less<>>> index_;
  page_id_t next_tuple_id_;
  int32_t timestamp_;
};
