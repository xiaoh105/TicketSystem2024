#pragma once

#include "common/rid.h"
#include "common/time.h"
#include "storage/index/b_plus_tree.h"
#include "ticket/train_system.h"

struct DetailedTrainInfo;

class TicketSystem {
public:
  TicketSystem() = delete;
  explicit TicketSystem(shared_ptr<BufferPoolManager> bpm);
  ~TicketSystem();
  void FetchTicket(Date date, int32_t seat_num, DetailedTrainInfo &info) const;
  void ModifyTicket(Date date, const DetailedTrainInfo &info);

private:
  shared_ptr<BufferPoolManager> bpm_;
  unique_ptr<BPlusTree<pair<unsigned long long, Date>, RID, std::less<>>> index_;
  page_id_t dynamic_page_id_{};
};
