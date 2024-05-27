#pragma once

#include <string>

#include "common/time.h"
#include "storage/index/b_plus_tree.h"

using std::string;

enum class OrderStatus : int8_t { ksuccess = 0, kpending = 1, krefund = 2 };

struct OrderInfo {
  int32_t price_{};
  int32_t num_{};
  int32_t timestamp_{};
  char from_[41]{};
  char to_[41]{};
  char train_id_[21]{};
  OrderStatus status_{OrderStatus::ksuccess};
  Time leave_{};
  Time arrive_{};
};

class OrderList {
public:
  OrderList() = delete;
  explicit OrderList(shared_ptr<BufferPoolManager> bpm);
  ~OrderList();
  void QueryOrder(const string &username) const;
  bool RefundTicket(const string &username, std::size_t num, OrderInfo &info);
  void QueueSucceed(const string &username, std::size_t timestamp);
  void AppendOrder(const string &username, const OrderInfo &order_info);

private:
  shared_ptr<BufferPoolManager> bpm_;
  unique_ptr<BPlusTree<unsigned long long, page_id_t, std::less<>>> index_;
  page_id_t next_tuple_id_{};
};
