#include "ticket/ticket_system.h"

#include "common/utils.h"

TicketSystem::TicketSystem(shared_ptr<BufferPoolManager> bpm)
  : bpm_(std::move(bpm)),
    index_(new BPlusTree<pair<unsigned long long, Date>, RID, std::less<>>(bpm_, std::less())) {
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  dynamic_page_id_ = cur_page->dynamic_page_id_;
}

TicketSystem::~TicketSystem() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->dynamic_page_id_ = dynamic_page_id_;
}

void TicketSystem::FetchTicket(Date date, int32_t seat_num, DetailedTrainInfo& info) const {
  vector<RID> rid;
  index_->GetValue({StringHash(info.train_id_), date}, &rid);
  if (rid.empty()) {
    for (int i = 0; i < info.station_num_ - 1; ++i) {
      info.seat_num_[i] = seat_num;
    }
  } else {
    auto cur_guard = bpm_->FetchPageRead(rid[0].page_id_);
    auto cur_page = cur_guard.As<DynamicTuplePage>();
    cur_page->As(rid[0].pos_, info.seat_num_, info.station_num_);
  }
}

void TicketSystem::ModifyTicket(Date date, const DetailedTrainInfo& info) {
  vector<RID> rid;
  index_->GetValue({StringHash(info.train_id_), date}, &rid);
  if (rid.empty()) {
    if (dynamic_page_id_ == INVALID_PAGE_ID) {
      auto new_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
      auto cur_page = new_guard.AsMut<DynamicTuplePage>();
      auto new_rid = cur_page->Append(info.seat_num_, info.station_num_);
      index_->Insert(pair(StringHash(info.train_id_), date), {dynamic_page_id_, new_rid});
      return;
    }
    auto cur_guard = bpm_->FetchPageWrite(dynamic_page_id_);
    auto cur_page = cur_guard.AsMut<DynamicTuplePage>();
    if (cur_page->IsFull(info.seat_num_, info.station_num_)) {
      auto new_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
      cur_page = new_guard.AsMut<DynamicTuplePage>();
    }
    auto new_rid = cur_page->Append(info.seat_num_, info.station_num_);
    index_->Insert(pair(StringHash(info.train_id_), date), {dynamic_page_id_, new_rid});
  } else {
    auto cur_guard = bpm_->FetchPageWrite(rid[0].page_id_);
    auto cur_page = cur_guard.AsMut<DynamicTuplePage>();
    cur_page->Modify(rid[0].pos_, info.seat_num_, info.station_num_);
  }
}
