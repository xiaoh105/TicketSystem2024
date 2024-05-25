#include <ticket/ticket_system.h>

#include "common/utils.h"

using std::cout, std::endl;

TicketSystem::TicketSystem(shared_ptr<BufferPoolManager> bpm,
                           shared_ptr<BufferPoolManager> station_bpm)
: bpm_(std::move(bpm)), station_bpm_(std::move(station_bpm)),
  index_(new BPlusTree<unsigned long long, RID, std::less<>>(bpm_, {})),
  station_index_(new BPlusTree<pair<unsigned long long, RID>, RID, std::less<>>(station_bpm_, {})) {
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  tuple_page_id_ = cur_page->tuple_page_id_;
  dynamic_page_id_ = cur_page->dynamic_page_id_;
}

TicketSystem::~TicketSystem() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->tuple_page_id_ = tuple_page_id_;
  cur_page->dynamic_page_id_ = dynamic_page_id_;
}

bool TicketSystem::FetchTrainInfo(const string& train_id, TrainInfo& info) const {
  vector<RID> train_rid;
  index_->GetValue(StringHash(train_id), &train_rid);
  if (train_rid.empty()) {
    return false;
  }
  auto cur_guard = bpm_->FetchPageRead(train_rid[0].page_id_);
  auto cur_page = cur_guard.As<TuplePage<TrainInfo>>();
  info = cur_page->At(train_rid[0].pos_);
  return true;
}

void TicketSystem::FetchDynamicInfo(const RID& rid, string& ret) const {
  auto cur_guard = bpm_->FetchPageRead(rid.page_id_);
  auto cur_page = cur_guard.As<DynamicTuplePage>();
  ret = cur_page->At(rid.pos_);
}

RID TicketSystem::WriteDynamicInfo(const string& data) {
  DynamicTuplePage *cur_page;
  if (dynamic_page_id_ == INVALID_PAGE_ID) {
    auto cur_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
    cur_page = cur_guard.AsMut<DynamicTuplePage>();
  } else {
    auto cur_guard = bpm_->FetchPageWrite(dynamic_page_id_);
    cur_page = cur_guard.AsMut<DynamicTuplePage>();
    if (cur_page->IsFull(data)) {
      auto new_guard = bpm_->NewPageGuarded(&dynamic_page_id_);
      cur_page = new_guard.AsMut<DynamicTuplePage>();
    }
  }
  auto pos = cur_page->Append(data);
  return {dynamic_page_id_, pos};
}


void TicketSystem::AddTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  const auto station_num = static_cast<int8_t>(std::stoi(para['n' - 'a']));
  int32_t seat_num = std::stoi(para['m' - 'a']);
  const string &station = para['s' - 'a'];
  auto stations = SplitString(para['s' - 'a']);
  auto prices = SplitAndConvert<int32_t>(para['p' - 'a'], station_num, 0);
  Moment start_time(para['x' - 'a']);
  auto travel_time = SplitAndConvert<int16_t>(para['t' - 'a'], station_num, 0);
  auto stopover_time =
    station_num == 2 ? new int16_t[2]{} : SplitAndConvert<int16_t>(para['o' - 'a'], station_num, 1);
  auto sales_date = SplitString(para['d' - 'a']);
  auto start_sale = Date(sales_date[0]);
  auto end_sale = Date(sales_date[1]);
  const char type = para['y' - 'a'][0];
  int32_t seats[station_num]{};
  for (int i = 0; i < station_num - 1; ++i) {
    seats[i] = seat_num;
  }

  TrainInfo data{};
  if (FetchTrainInfo(train_id, data)) {
    Fail();
    return;
  }
  train_id.copy(data.train_id_, string::npos);
  data.start_sale_ = start_sale;
  data.end_sale_ = end_sale;
  data.start_time_ = start_time;
  data.station_num_ = station_num;
  data.type_ = type;
  data.stations_ = WriteDynamicInfo(station);
  data.seat_num_ = WriteDynamicInfo(seats, station_num);
  data.prices_ = WriteDynamicInfo(prices, station_num);
  data.travel_time_ = WriteDynamicInfo(travel_time, station_num);
  data.stopover_time_ = WriteDynamicInfo(stopover_time, station_num);

  TuplePage<TrainInfo> *cur_page;
  if (tuple_page_id_ == INVALID_PAGE_ID) {
    auto cur_guard = bpm_->NewPageGuarded(&tuple_page_id_);
    cur_page = cur_guard.AsMut<TuplePage<TrainInfo>>();
  } else {
    auto cur_guard = bpm_->FetchPageWrite(tuple_page_id_);
    cur_page = cur_guard.AsMut<TuplePage<TrainInfo>>();
    if (cur_page->Full()) {
      auto new_guard = bpm_->NewPageGuarded(&tuple_page_id_);
      cur_page = new_guard.AsMut<TuplePage<TrainInfo>>();
    }
  }
  RID train_rid{tuple_page_id_, cur_page->Append(data)};

  index_->Insert(StringHash(train_id), train_rid);
  Succeed();
}

void TicketSystem::DeleteTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  vector<RID> train_rid;
  if (index_->GetValue(StringHash(train_id), &train_rid)) {
    index_->Remove(StringHash(train_id));
    Succeed();
  } else {
    Fail();
  }
}

void TicketSystem::ReleaseTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  TrainInfo info{};
  vector<RID> train_rid;
  index_->GetValue(StringHash(train_id), &train_rid);
  if (train_rid.empty()) {
    Fail();
    return;
  }
  auto cur_guard = bpm_->FetchPageWrite(train_rid[0].page_id_);
  auto cur_page = cur_guard.AsMut<TuplePage<TrainInfo>>();
  info = cur_page->At(train_rid[0].pos_);
  if (info.released_ == true) {
    Fail();
    return;
  }
  cur_page->operator[](train_rid[0].pos_).released_ = true;
  string station;
  FetchDynamicInfo(info.stations_, station);
  for (const auto &i : SplitString(station)) {
    station_index_->Insert({StringHash(i), train_rid[0]}, train_rid[0]);
  }
  Succeed();
}

void TicketSystem::QueryTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  Date date(para['d' - 'a']);
  TrainInfo info;
  if (!FetchTrainInfo(train_id, info)) {
    Fail();
    return;
  }

  auto n = info.station_num_;
  string station;
  FetchDynamicInfo(info.stations_, station);
  auto stations = SplitString(station);
  int32_t seat_num[n];
  FetchDynamicInfo(info.seat_num_, seat_num, n);
  int32_t price[n];
  FetchDynamicInfo(info.prices_, price, n);
  int16_t travel_time[n];
  FetchDynamicInfo(info.travel_time_, travel_time, n);
  int16_t stopover_time[n];
  FetchDynamicInfo(info.stopover_time_, stopover_time, n);
  if (date < info.start_sale_ || date > info.end_sale_) {
    Fail();
    return;
  }

  cout << train_id << " " << info.type_ << endl;
  int total_price = 0;
  Time cur_time(date, info.start_time_);
  for (int i = 0; i < n; ++i) {
    cout << stations[i] << " "
         << (i == 0 ? "xx-xx xx:xx" : cur_time.ToString()) << " -> "
         << (i == n - 1 ? "xx-xx xx:xx" : (cur_time += stopover_time[i]).ToString()) << " "
         << total_price << " " << (i == n - 1 ? "x" : std::to_string(seat_num[i])) << endl;
    total_price += price[i];
    cur_time += travel_time[i];
  }
}

struct TicketInfo {
  string train_id_{};
  Time leave_{{0, 0}, {13, 61}};
  int duration_{};
  int price_{};
  int seat_{};
};

bool OrderByTime(const TicketInfo &lhs, const TicketInfo &rhs) {
  return lhs.duration_ != rhs.duration_ ? lhs.duration_ <= rhs.duration_ : lhs.train_id_ <= rhs.train_id_;
}

bool OrderByPrice(const TicketInfo &lhs, const TicketInfo &rhs) {
  return lhs.price_ != rhs.price_ ? lhs.price_ <= rhs.price_ : lhs.train_id_ <= rhs.train_id_;
}

void TicketSystem::FetchTrainInfoStation(const string& station_name, vector<RID>& ret) {
  auto station_hash = StringHash(station_name);
  auto it = station_index_->LowerBound({station_hash, RID_MIN});
  if (!it) {
    return;
  }
  while ((*it).first.first == station_hash) {
    ret.push_back((*it).second);
    if (it.IsEnd()) {
      break;
    }
    ++it;
  }
  ret.sort();
}


void TicketSystem::QueryTicket(const string para[26]) {
  const string &start = para['s' - 'a'];
  const string &end = para['t' - 'a'];
  bool order_by_cost = (para['p' - 'a'] == "cost");
  Date date(para['d' - 'a']);

  vector<RID> train_rid1;
  vector<RID> train_rid2;
  FetchTrainInfoStation(start, train_rid1);
  FetchTrainInfoStation(end, train_rid2);

  vector<TicketInfo> result;
  int pos = 0;
  for (const auto &i : train_rid1) {
    while (pos < train_rid2.size() && train_rid2[pos] < i) {
      ++pos;
    }
    if (pos == train_rid2.size()) {
      break;
    }
    if (train_rid2[pos] != i) {
      continue;
    }
    auto cur_guard = bpm_->FetchPageRead(i.page_id_);
    auto cur_page = cur_guard.As<TuplePage<TrainInfo>>();
    auto train_info = cur_page->At(i.pos_);
    string station;
    FetchDynamicInfo(train_info.stations_, station);
    auto stations = SplitString(station);
    auto n = train_info.station_num_;

    int start_pos = -1, end_pos = -1;
    for (int j = 0; j < n; ++j) {
      if (stations[j] == start) {
        start_pos = j;
      }
      if (stations[j] == end) {
        end_pos = j;
      }
    }
    if (start_pos > end_pos) {
      continue;
    }

    int32_t prices[n];
    FetchDynamicInfo(train_info.prices_, prices, n);
    int32_t seat_num[n];
    FetchDynamicInfo(train_info.seat_num_, seat_num, n);
    int16_t travel_time[n];
    FetchDynamicInfo(train_info.travel_time_, travel_time, n);
    int16_t stopover_time[n];
    FetchDynamicInfo(train_info.stopover_time_, stopover_time, n);

    int elapsed_time = 0;
    for (int j = 0; j < start_pos; ++j) {
      elapsed_time += travel_time[j];
    }
    for (int j = 1; j <= start_pos; ++j) {
      elapsed_time += stopover_time[j];
    }
    if ((Time(train_info.start_sale_, train_info.start_time_) + elapsed_time).GetDate() > date ||
        (Time(train_info.end_sale_, train_info.start_time_) + elapsed_time).GetDate() < date) {
      continue;
    }
    TicketInfo cur_ticket{};
    cur_ticket.train_id_ = train_info.train_id_;
    cur_ticket.seat_ = 100000;
    if ((Time(date, {0, 0}) - elapsed_time).GetMoment() > train_info.start_time_) {
      cur_ticket.leave_ = Time(date, {0, 0}) - elapsed_time + 1440;
      cur_ticket.leave_.SetMoment(train_info.start_time_);
      cur_ticket.leave_ += elapsed_time;
    } else {
      cur_ticket.leave_ = Time(date, {0, 0}) - elapsed_time;
      cur_ticket.leave_.SetMoment(train_info.start_time_);
      cur_ticket.leave_ += elapsed_time;
    }
    for (int j = start_pos; j < end_pos; ++j) {
      cur_ticket.duration_ += travel_time[j];
      cur_ticket.price_ += prices[j];
      cur_ticket.seat_ = std::min(cur_ticket.seat_, seat_num[j]);
    }
    for (int j = start_pos + 1; j < end_pos; ++j) {
      cur_ticket.duration_ += stopover_time[j];
    }
    result.push_back(cur_ticket);
  }
  if (order_by_cost) {
    result.sort(OrderByPrice);
  } else {
    result.sort(OrderByTime);
  }
  cout << result.size() << endl;
  for (const auto &ticket_info : result) {
    cout << ticket_info.train_id_ << " " << start << " " << ticket_info.leave_ << " -> "
         << end << " " << (ticket_info.leave_ + ticket_info.duration_) << " "
         << ticket_info.price_ << " " << ticket_info.seat_ << endl;
  }
}
