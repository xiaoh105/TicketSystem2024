#include "ticket/order_list.h"

#include "common/utils.h"
#include "storage/page/tuple_page.h"

OrderList::OrderList(shared_ptr<BufferPoolManager> bpm)
 : bpm_(std::move(bpm)),
   index_(new BPlusTree<unsigned long long, page_id_t, std::less<>>(bpm_, std::less())) {
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  next_tuple_id_ = cur_page->tuple_page_id_;
}

OrderList::~OrderList() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->tuple_page_id_ = next_tuple_id_;
}

void OrderList::QueryOrder(const string& username) const {
  using std::cout, std::endl;
  vector<page_id_t> page_id;
  index_->GetValue(StringHash(username), &page_id);
  if (page_id.empty()) {
    cout << 0 << endl;
    return;
  }
  auto cur_guard = bpm_->FetchPageRead(page_id[0]);
  auto cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
  vector<OrderInfo> result;
  bool flag = true;
  while (flag) {
    for (int i = cur_page->Size() - 1; i >= 0; --i) {
      const auto &info = cur_page->At(i);
      result.push_back(info);
    }
    if (cur_page->GetNextPageId() == INVALID_PAGE_ID) {
      flag = false;
    } else {
      cur_guard = bpm_->FetchPageRead(cur_page->GetNextPageId());
      cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
    }
  }
  cout << result.size() << endl;
  for (const auto &info : result) {
    string status;
    switch (info.status_) {
      case OrderStatus::kpending :
        status = "pending"; break;
      case OrderStatus::krefund :
        status = "refunded"; break;
      case OrderStatus::ksuccess :
        status = "success"; break;
      }
    cout << "[" << status << "] " << info.train_id_ << " " << info.from_ << " "
         << info.leave_ << " -> " << info.to_ << " " << info.arrive_ << " "
         << info.price_ << " " << info.num_ << endl;
  }
}

void OrderList::QueueSucceed(const string& username, std::size_t timestamp) {
  vector<page_id_t> page_id;
  index_->GetValue(StringHash(username), &page_id);
  auto cur_guard = bpm_->FetchPageWrite(page_id[0]);
  auto cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
  bool flag = true;
  while (flag) {
    if (cur_page->At(0).timestamp_ > timestamp) {
      cur_guard = bpm_->FetchPageWrite(cur_page->GetNextPageId());
      cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
    } else {
      flag = false;
      auto tmp_page = cur_guard.AsMut<LinkedTuplePage<OrderInfo>>();
      for (int i = 0; i < tmp_page->Size(); ++i) {
        if (tmp_page->operator[](i).timestamp_ == timestamp) {
          tmp_page->operator[](i).status_ = OrderStatus::ksuccess;
          break;
        }
      }
    }
  }
}

bool OrderList::RefundTicket(const string& username, std::size_t num, OrderInfo &info) {
  vector<page_id_t> page_id;
  index_->GetValue(StringHash(username), &page_id);
  auto cur_guard = bpm_->FetchPageWrite(page_id[0]);
  auto cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
  while (cur_page->Size() < num) {
    if (cur_page->GetNextPageId() == INVALID_PAGE_ID) {
      return false;
    }
    num -= cur_page->Size();
    cur_guard = bpm_->FetchPageWrite(cur_page->GetNextPageId());
    cur_page = cur_guard.As<LinkedTuplePage<OrderInfo>>();
  }
  auto tmp_page = cur_guard.AsMut<LinkedTuplePage<OrderInfo>>();
  auto size = tmp_page->Size();
  if (tmp_page->At(size - num).status_ == OrderStatus::krefund) {
    return false;
  }
  info = tmp_page->At(size - num);
  tmp_page->operator[](size - num).status_ = OrderStatus::krefund;
  return true;
}

void OrderList::AppendOrder(const string &username, const OrderInfo& order_info) {
  vector<page_id_t> page_id;
  index_->GetValue(StringHash(username), &page_id);
  if (page_id.empty()) {
    page_id_t new_id;
    auto cur_guard = bpm_->NewPageGuarded(&new_id);
    auto cur_page = cur_guard.AsMut<LinkedTuplePage<OrderInfo>>();
    cur_page->SetNextPageId(INVALID_PAGE_ID);
    cur_page->Append(order_info);
    index_->Insert(StringHash(username), new_id);
    return;
  }
  auto cur_guard = bpm_->FetchPageWrite(page_id[0]);
  auto cur_page = cur_guard.AsMut<LinkedTuplePage<OrderInfo>>();
  if (cur_page->Full()) {
    page_id_t new_id;
    auto new_guard = bpm_->NewPageGuarded(&new_id);
    auto new_page = new_guard.AsMut<LinkedTuplePage<OrderInfo>>();
    new_page->SetNextPageId(cur_guard.PageId());
    index_->Remove(StringHash(username));
    index_->Insert(StringHash(username), new_id);
    cur_page = new_page;
  }
  cur_page->Append(order_info);
}
