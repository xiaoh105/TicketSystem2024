#pragma once

#include "buffer/buffer_pool_manager.h"
#include "common/rid.h"
#include "common/time.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/tuple_page.h"
#include "ticket/order_list.h"
#include "ticket/ticket_system.h"
#include "ticket/waitlist.h"
#include "user/user_system.h"

class TicketSystem;

struct TrainInfo {
  char train_id_[21]{};
  int32_t seat_num_{};
  RID stations_{}; // Type: string
  RID prices_{}; // Type: int32_t
  RID travel_time_{}; // Type: int16_t
  RID stopover_time_{}; // Type: int16_t
  Date start_sale_{}, end_sale_{};
  Moment start_time_{};
  int8_t station_num_{};
  char type_{};
  bool released_{false};
};

struct DetailedTrainInfo {
  string train_id_{};
  vector<string> stations_{};
  int32_t max_seat_{};
  int32_t seat_num_[100]{};
  int32_t prices_[100]{};
  int16_t travel_time_[100]{};
  int16_t stopover_time_[100]{};
  Date start_sale_{}, end_sale_{};
  Moment start_time_{};
  int8_t station_num_{};
  char type_;
};

class TrainSystem {
 public:
  TrainSystem() = delete;

  explicit TrainSystem(shared_ptr<BufferPoolManager> bpm,
                       shared_ptr<BufferPoolManager> station_bpm,
                       shared_ptr<BufferPoolManager> ticket_bpm,
                       shared_ptr<BufferPoolManager> waitlist_bpm,
                       shared_ptr<BufferPoolManager> orderlist_bpm);

  ~TrainSystem();

  void AddTrain(const string para[26]);

  void DeleteTrain(const string para[26]);

  void ReleaseTrain(const string para[26]);

  void QueryTrain(const string para[26]);

  void QueryTicket(const string para[26]);

  void QueryTransfer(const string para[26]);

  void BuyTicket(const string para[26], const shared_ptr<UserSystem> &user_system);

  void QueryOrder(const string para[26], const shared_ptr<UserSystem> &user_system) const;

  void RefundTicket(const string para[26], const shared_ptr<UserSystem> &user_system);

 private:
  bool FetchTrainInfo(const string &train_id, TrainInfo &info) const;

  void FetchTrainInfoStation(const string &station_name, vector<RID> &ret);

  void FetchDynamicInfo(const RID &rid, string &ret) const;

  RID WriteDynamicInfo(const string &data);

  void FetchDetailedTrainInfo(const RID &rid, DetailedTrainInfo &info) const;

  void FetchDetailedTrainInfo(const TrainInfo &brief, Date date, DetailedTrainInfo &info) const;

  void FetchDetailedTrainInfo(const TrainInfo &brief, DetailedTrainInfo &info) const;

  template <class T>
  void FetchDynamicInfo(const RID &rid, T *ret, std::size_t n) const {
    auto cur_guard = bpm_->FetchPageRead(rid.page_id_);
    auto cur_page = cur_guard.As<DynamicTuplePage>();
    cur_page->As<T>(rid.pos_, ret, n);
  };

  template <class T>
  RID WriteDynamicInfo(const T *data, std::size_t n) {
    DynamicTuplePage *cur_page;
    if (dynamic_page_id_ == INVALID_PAGE_ID) {
      auto cur_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
      cur_page = cur_guard.AsMut<DynamicTuplePage>();
    } else {
      auto cur_guard = bpm_->FetchPageWrite(dynamic_page_id_);
      cur_page = cur_guard.AsMut<DynamicTuplePage>();
      if (cur_page->IsFull(data, n)) {
        auto new_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
        cur_page = new_guard.AsMut<DynamicTuplePage>();
      }
    }
    auto pos = cur_page->Append(data, n);
    return {dynamic_page_id_, pos};
  }

  shared_ptr<BufferPoolManager> bpm_;
  shared_ptr<BufferPoolManager> station_bpm_;
  unique_ptr<BPlusTree<unsigned long long, RID, std::less<>>> index_;
  unique_ptr<BPlusTree<pair<unsigned long long, RID>, RID, std::less<>>> station_index_;
  unique_ptr<TicketSystem> ticket_system_;
  unique_ptr<WaitList> waitlist_;
  unique_ptr<OrderList> orderlist_;
  page_id_t tuple_page_id_;
  page_id_t dynamic_page_id_;
};
