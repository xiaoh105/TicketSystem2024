#include "common/utils.h"
#include "ticket/waitlist.h"
#include "storage/page/tuple_page.h"

WaitList::WaitList(shared_ptr<BufferPoolManager> bpm)
  : bpm_(std::move(bpm)),
    index_(new BPlusTree<pair<unsigned long long, Date>, signed int, std::less<>>(bpm_, std::less())){
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  next_tuple_id_ = cur_page->tuple_page_id_;
  timestamp_ = cur_page->dynamic_page_id_;
}

WaitList::~WaitList() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->tuple_page_id_ = next_tuple_id_;
  cur_page->dynamic_page_id_ = timestamp_;
}

WaitList::iterator::iterator(shared_ptr<BufferPoolManager> bpm, WritePageGuard guard,
                             int pos, const string &train_id, Date date)
 : bpm_(std::move(bpm)), guard_(std::move(guard)), pos_(pos),
   train_id_(train_id), date_(date) {}

WaitInfo& WaitList::iterator::operator*() {
  auto cur_page = guard_.AsMut<LinkedTuplePage<WaitInfo>>();
  return cur_page->operator[](pos_);
}

WaitList::iterator& WaitList::iterator::operator++() {
  if (IsEnd()) {
    assert(false);
  }
  auto cur_page = guard_.As<LinkedTuplePage<WaitInfo>>();
  if (pos_ == LinkedTuplePage<WaitInfo>::MaxSize() - 1) {
    if (cur_page->GetNextPageId() == INVALID_PAGE_ID) {
      assert(false);
    }
    guard_ = bpm_->FetchPageWrite(cur_page->GetNextPageId());
    pos_ = 0;
  } else {
    ++pos_;
  }
  return *this;
}

bool WaitList::iterator::IsEnd() {
  auto cur_page = guard_.As<LinkedTuplePage<WaitInfo>>();
  return pos_ == LinkedTuplePage<WaitList>::MaxSize() - 1 && cur_page->GetNextPageId() == INVALID_PAGE_ID;
}

WaitList::iterator WaitList::FetchWaitlist(const string& train_id, Date date) {
  vector<page_id_t> page_id;
  index_->GetValue(pair(StringHash(train_id), date), &page_id);
  if (page_id.empty()) {
    return {};
  }
  auto cur_guard = bpm_->FetchPageWrite(page_id[0]);
  RemoveEmptyPage(train_id, date, cur_guard);

  return {bpm_, std::move(cur_guard), 0, train_id, date};
}

void WaitList::RemoveEmptyPage(const string& train_id, Date date, WritePageGuard& cur_guard) {
  bool flag = true;
  do {
    auto cur_page = cur_guard.As<LinkedTuplePage<WaitInfo>>();
    for (int i = 0; i < cur_page->Size(); ++i) {
      if (cur_page->At(i).queue) {
        flag = false;
      }
    }
    if (flag) {
      index_->Remove(pair(StringHash(train_id), date));
      if (cur_page->GetNextPageId() == INVALID_PAGE_ID) {
        cur_guard = {};
        return;
      }
      index_->Insert(pair(StringHash(train_id), date), cur_page->GetNextPageId());
      cur_guard = bpm_->FetchPageWrite(cur_page->GetNextPageId());
    }
  } while (flag);
}

int32_t WaitList::Insert(const string& train_id, Date date, const string& username_,
                      int start_pos, int end_pos, int num) {
  vector<page_id_t> page_id_v{};
  index_->GetValue(pair(StringHash(train_id), date), &page_id_v);
  LinkedTuplePage<WaitInfo> *cur_page;
  if (page_id_v.empty()) {
    page_id_t page_id = INVALID_PAGE_ID;
    auto cur_guard = bpm_->NewPageGuarded(&page_id);
    cur_page = cur_guard.AsMut<LinkedTuplePage<WaitInfo>>();
    cur_page->SetNextPageId(INVALID_PAGE_ID);
    index_->Insert(pair(StringHash(train_id), date), page_id);
  } else {
    auto cur_guard = bpm_->FetchPageWrite(page_id_v[0]);
    auto tmp_page = cur_guard.As<LinkedTuplePage<WaitInfo>>();
    while (tmp_page->GetNextPageId() != INVALID_PAGE_ID) {
      cur_guard = bpm_->FetchPageWrite(tmp_page->GetNextPageId());
      tmp_page = cur_guard.As<LinkedTuplePage<WaitInfo>>();
    }
    cur_page = cur_guard.AsMut<LinkedTuplePage<WaitInfo>>();
    if (tmp_page->Full()) {
      page_id_t page_id = INVALID_PAGE_ID;
      auto new_guard = bpm_->NewPageGuarded(&page_id);
      cur_page->SetNextPageId(page_id);
      cur_page = new_guard.AsMut<LinkedTuplePage<WaitInfo>>();
      cur_page->SetNextPageId(INVALID_PAGE_ID);
    }
  }
  WaitInfo info{};
  info.start_pos_ = static_cast<int8_t>(start_pos);
  info.end_pos_ = static_cast<int8_t>(end_pos);
  username_.copy(info.username_, string::npos);
  info.num_ = num;
  info.timestamp_ = ++timestamp_;
  info.queue = true;
  cur_page->Append(info);
  return info.timestamp_;
}
