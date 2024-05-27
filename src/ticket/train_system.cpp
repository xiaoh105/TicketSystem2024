#include <ticket/train_system.h>

#include "common/utils.h"

using std::cout, std::endl;

TrainSystem::TrainSystem(shared_ptr<BufferPoolManager> bpm,
                         shared_ptr<BufferPoolManager> station_bpm,
                         shared_ptr<BufferPoolManager> ticket_bpm,
                         shared_ptr<BufferPoolManager> waitlist_bpm,
                         shared_ptr<BufferPoolManager> orderlist_bpm)
: bpm_(std::move(bpm)), station_bpm_(std::move(station_bpm)),
  index_(new BPlusTree<unsigned long long, RID, std::less<>>(bpm_, {})),
  station_index_(new BPlusTree<pair<unsigned long long, RID>, RID, std::less<>>(station_bpm_, {})),
  ticket_system_(new TicketSystem(std::move(ticket_bpm))),
  waitlist_(new WaitList(std::move(waitlist_bpm))),
  orderlist_(new OrderList(std::move(orderlist_bpm))) {
  auto cur_guard = bpm_->FetchPageRead(0);
  auto cur_page = cur_guard.As<BPlusTreeHeaderPage>();
  tuple_page_id_ = cur_page->tuple_page_id_;
  dynamic_page_id_ = cur_page->dynamic_page_id_;
}

TrainSystem::~TrainSystem() {
  auto cur_guard = bpm_->FetchPageWrite(0);
  auto cur_page = cur_guard.AsMut<BPlusTreeHeaderPage>();
  cur_page->tuple_page_id_ = tuple_page_id_;
  cur_page->dynamic_page_id_ = dynamic_page_id_;
}

bool TrainSystem::FetchTrainInfo(const string& train_id, TrainInfo& info) const {
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

void TrainSystem::FetchDynamicInfo(const RID& rid, string& ret) const {
  auto cur_guard = bpm_->FetchPageRead(rid.page_id_);
  auto cur_page = cur_guard.As<DynamicTuplePage>();
  ret = cur_page->At(rid.pos_);
}

RID TrainSystem::WriteDynamicInfo(const string& data) {
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


void TrainSystem::AddTrain(const string para[26]) {
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
  data.seat_num_ = seat_num;
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

void TrainSystem::DeleteTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  vector<RID> train_rid;
  if (index_->GetValue(StringHash(train_id), &train_rid)) {
    auto cur_guard = bpm_->FetchPageRead(train_rid[0].page_id_);
    auto cur_page = cur_guard.As<TuplePage<TrainInfo>>();
    if (cur_page->At(train_rid[0].pos_).released_ == true) {
      Fail();
    } else {
      index_->Remove(StringHash(train_id));
      Succeed();
    }
  } else {
    Fail();
  }
}

void TrainSystem::ReleaseTrain(const string para[26]) {
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

void TrainSystem::QueryTrain(const string para[26]) {
  const string &train_id = para['i' - 'a'];
  Date date(para['d' - 'a']);
  TrainInfo info;
  if (!FetchTrainInfo(train_id, info)) {
    Fail();
    return;
  }

  DetailedTrainInfo detailed_info{};
  FetchDetailedTrainInfo(info, date, detailed_info);
  if (date < info.start_sale_ || date > info.end_sale_) {
    Fail();
    return;
  }

  auto n = detailed_info.station_num_;
  cout << train_id << " " << info.type_ << endl;
  int total_price = 0;
  Time cur_time(date, info.start_time_);
  for (int i = 0; i < n; ++i) {
    cout << detailed_info.stations_[i] << " "
         << (i == 0 ? "xx-xx xx:xx" : cur_time.ToString()) << " -> "
         << (i == n - 1 ? "xx-xx xx:xx" : (cur_time += detailed_info.stopover_time_[i]).ToString()) << " "
         << total_price << " " << (i == n - 1 ? "x" : std::to_string(detailed_info.seat_num_[i])) << endl;
    total_price += detailed_info.prices_[i];
    cur_time += detailed_info.travel_time_[i];
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

void TrainSystem::FetchTrainInfoStation(const string& station_name, vector<RID>& ret) {
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


void TrainSystem::QueryTicket(const string para[26]) {
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
    DetailedTrainInfo detailed_info{};
    FetchDetailedTrainInfo(train_info, detailed_info);
    auto n = train_info.station_num_;

    int start_pos = -1, end_pos = -1;
    for (int j = 0; j < n; ++j) {
      if (detailed_info.stations_[j] == start) {
        start_pos = j;
      }
      if (detailed_info.stations_[j] == end) {
        end_pos = j;
      }
    }
    if (start_pos > end_pos) {
      continue;
    }

    int elapsed_time = 0;
    for (int j = 0; j < start_pos; ++j) {
      elapsed_time += detailed_info.travel_time_[j];
    }
    for (int j = 1; j <= start_pos; ++j) {
      elapsed_time += detailed_info.stopover_time_[j];
    }
    if ((Time(train_info.start_sale_, train_info.start_time_) + elapsed_time).GetDate() > date ||
        (Time(train_info.end_sale_, train_info.start_time_) + elapsed_time).GetDate() < date) {
      continue;
    }
    TicketInfo cur_ticket{};
    cur_ticket.train_id_ = train_info.train_id_;
    cur_ticket.seat_ = 100000;
    auto start_time = Time(date, {0, 0}) - elapsed_time;
    if (start_time.GetMoment() > train_info.start_time_) {
      start_time += 1440;
      start_time.SetMoment(train_info.start_time_);
      cur_ticket.leave_ = start_time + elapsed_time;
    } else {
      start_time.SetMoment(train_info.start_time_);
      cur_ticket.leave_ = start_time + elapsed_time;
    }
    ticket_system_->FetchTicket(start_time.GetDate(), train_info.seat_num_, detailed_info);

    for (int j = start_pos; j < end_pos; ++j) {
      cur_ticket.duration_ += detailed_info.travel_time_[j];
      cur_ticket.price_ += detailed_info.prices_[j];
      cur_ticket.seat_ = std::min(cur_ticket.seat_, detailed_info.seat_num_[j]);
    }
    if (cur_ticket.seat_ == 0) {
      continue;
    }
    for (int j = start_pos + 1; j < end_pos; ++j) {
      cur_ticket.duration_ += detailed_info.stopover_time_[j];
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

void TrainSystem::FetchDetailedTrainInfo(const RID& rid, DetailedTrainInfo& info) const {
  auto cur_guard = bpm_->FetchPageRead(rid.page_id_);
  auto cur_page = cur_guard.As<TuplePage<TrainInfo>>();
  auto brief = cur_page->At(rid.pos_);
  info.type_ = brief.type_;
  info.train_id_ = brief.train_id_;
  info.start_sale_ = brief.start_sale_;
  info.end_sale_ = brief.end_sale_;
  info.start_time_ = brief.start_time_;
  info.station_num_ = brief.station_num_;
  info.max_seat_ = brief.seat_num_;

  auto n = info.station_num_;
  FetchDynamicInfo(brief.prices_, info.prices_, n);
  FetchDynamicInfo(brief.stopover_time_, info.stopover_time_, n);
  FetchDynamicInfo(brief.travel_time_, info.travel_time_, n);
  string station;
  FetchDynamicInfo(brief.stations_, station);
  info.stations_ = SplitString(station);
}

void TrainSystem::FetchDetailedTrainInfo(const TrainInfo& brief, Date date, DetailedTrainInfo& info) const {
  info.type_ = brief.type_;
  info.train_id_ = brief.train_id_;
  info.start_sale_ = brief.start_sale_;
  info.end_sale_ = brief.end_sale_;
  info.start_time_ = brief.start_time_;
  info.station_num_ = brief.station_num_;
  info.max_seat_ = brief.seat_num_;

  auto n = info.station_num_;
  FetchDynamicInfo(brief.prices_, info.prices_, n);
  FetchDynamicInfo(brief.stopover_time_, info.stopover_time_, n);
  FetchDynamicInfo(brief.travel_time_, info.travel_time_, n);
  string station;
  FetchDynamicInfo(brief.stations_, station);
  info.stations_ = SplitString(station);
  ticket_system_->FetchTicket(date, brief.seat_num_, info);
}

void TrainSystem::FetchDetailedTrainInfo(const TrainInfo& brief, DetailedTrainInfo& info) const {
  info.type_ = brief.type_;
  info.train_id_ = brief.train_id_;
  info.start_sale_ = brief.start_sale_;
  info.end_sale_ = brief.end_sale_;
  info.start_time_ = brief.start_time_;
  info.station_num_ = brief.station_num_;
  info.max_seat_ = brief.seat_num_;

  auto n = info.station_num_;
  FetchDynamicInfo(brief.prices_, info.prices_, n);
  FetchDynamicInfo(brief.stopover_time_, info.stopover_time_, n);
  FetchDynamicInfo(brief.travel_time_, info.travel_time_, n);
  string station;
  FetchDynamicInfo(brief.stations_, station);
  info.stations_ = SplitString(station);
}

bool OrderByTime(const TicketInfo &lhs1, const TicketInfo &lhs2,
                 const TicketInfo &rhs1, const TicketInfo &rhs2) {
  if (lhs2.leave_ - lhs1.leave_ + lhs2.duration_ != rhs2.leave_ - rhs1.leave_ + rhs2.duration_) {
    return lhs2.leave_ - lhs1.leave_ + lhs2.duration_ < rhs2.leave_ - rhs1.leave_ + rhs2.duration_;
  }
  if (lhs1.price_ + lhs2.price_ != rhs1.price_ + rhs2.price_) {
    return lhs1.price_ + lhs2.price_ < rhs1.price_ + rhs2.price_;
  }
  return lhs1.train_id_ != rhs1.train_id_ ? lhs1.train_id_ < rhs1.train_id_ :
                                            lhs2.train_id_ < rhs2.train_id_;
}

bool OrderByPrice(const TicketInfo &lhs1, const TicketInfo &lhs2,
                 const TicketInfo &rhs1, const TicketInfo &rhs2) {
  if (lhs1.price_ + lhs2.price_ != rhs1.price_ + rhs2.price_) {
    return lhs1.price_ + lhs2.price_ < rhs1.price_ + rhs2.price_;
  }
  if (lhs2.leave_ - lhs1.leave_ + lhs2.duration_ != rhs2.leave_ - rhs1.leave_ + rhs2.duration_) {
    return lhs2.leave_ - lhs1.leave_ + lhs2.duration_ < rhs2.leave_ - rhs1.leave_ + rhs2.duration_;
  }
  return lhs1.train_id_ != rhs1.train_id_ ? lhs1.train_id_ < rhs1.train_id_ :
                                            lhs2.train_id_ < rhs2.train_id_;
}

void TrainSystem::QueryTransfer(const string para[26]) {
  const string &start = para['s' - 'a'];
  const string &end = para['t' - 'a'];
  Date date(para['d' - 'a']);
  bool order_by_cost = (para['p' - 'a'] == "cost");
  vector<RID> rid1;
  vector<RID> rid2;
  FetchTrainInfoStation(start, rid1);
  FetchTrainInfoStation(end, rid2);
  vector<DetailedTrainInfo> info1;
  vector<DetailedTrainInfo> info2;
  for (const auto &rid : rid1) {
    DetailedTrainInfo info;
    FetchDetailedTrainInfo(rid, info);
    info1.push_back(info);
  }
  for (const auto &rid : rid2) {
    DetailedTrainInfo info;
    FetchDetailedTrainInfo(rid, info);
    info2.push_back(info);
  }

  TicketInfo ticket1{}, ticket2{};
  string transfer{};
  for (auto &train1 : info1) {
    int start_pos = -1;
    for (int i = 0; i < train1.station_num_; ++i) {
      if (train1.stations_[i] == start) {
        start_pos = i;
        break;
      }
    }
    for (auto &train2 : info2) {
      if (train1.train_id_ == train2.train_id_) {
        continue;
      }
      int end_pos = -1;
      for (int i = 0; i < train2.station_num_; ++i) {
        if (train2.stations_[i] == end) {
          end_pos = i;
        }
      }
      int transfer_pos1, transfer_pos2;
      for (int i = start_pos + 1; i < train1.station_num_; ++i) {
        for (int j = 0; j < end_pos; ++j) {
          if (train1.stations_[i] != train2.stations_[j]) {
            continue;
          }
          transfer_pos1 = i;
          transfer_pos2 = j;
          int elapsed_time1 = 0;
          int elapsed_time2 = 0;
          int duration1 = 0;
          int duration2 = 0;
          int price1 = 0;
          int price2 = 0;
          int max_seat1 = 100000;
          int max_seat2 = 100000;
          for (int k = 0; k < start_pos; ++k) {
            elapsed_time1 += train1.travel_time_[k];
          }
          for (int k = 1; k <= start_pos; ++k) {
            elapsed_time1 += train1.stopover_time_[k];
          }
          for (int k = 0; k < transfer_pos2; ++k) {
            elapsed_time2 += train2.travel_time_[k];
          }
          for (int k = 1; k <= transfer_pos2; ++k) {
            elapsed_time2 += train2.stopover_time_[k];
          }
          if ((Time(train1.start_sale_, train1.start_time_) + elapsed_time1).GetDate() > date ||
              (Time(train1.end_sale_, train1.start_time_) + elapsed_time1).GetDate() < date) {
            continue;
          }
          Time start_time1 = Time(date, {0, 0}) - elapsed_time1;
          if (start_time1.GetMoment() <= train1.start_time_) {
            start_time1.SetMoment(train1.start_time_);
          } else {
            start_time1 += 1440;
            start_time1.SetMoment(train1.start_time_);
          }
          ticket_system_->FetchTicket(start_time1.GetDate(), train1.max_seat_, train1);
          for (int k = start_pos; k < transfer_pos1; ++k) {
            duration1 += train1.travel_time_[k];
            price1 += train1.prices_[k];
            max_seat1 = std::min(max_seat1, train1.seat_num_[k]);
          }
          for (int k = start_pos + 1; k < transfer_pos1; ++k) {
            duration1 += train1.stopover_time_[k];
          }
          Time arrive_time_1 = start_time1 + elapsed_time1 + duration1;
          if ((Time(train2.end_sale_, train2.start_time_) + elapsed_time2).GetDate() < date) {
            continue;
          }
          Time start_time2{};
          if (arrive_time_1 - elapsed_time2 < Time(train2.start_sale_, train2.start_time_)) {
            start_time2 = Time(train2.start_sale_, train2.start_time_);
          } else {
            start_time2 = arrive_time_1 - elapsed_time2;
            if (start_time2.GetMoment() <= train2.start_time_) {
              start_time2.SetMoment(train2.start_time_);
            } else {
              start_time2 += 1440;
              start_time2.SetMoment(train2.start_time_);
            }
          }
          ticket_system_->FetchTicket(start_time2.GetDate(), train2.max_seat_, train2);
          for (int k = transfer_pos2; k < end_pos; ++k) {
            duration2 += train2.travel_time_[k];
            price2 += train2.prices_[k];
            max_seat2 = std::min(max_seat2, train2.seat_num_[k]);
          }
          for (int k = transfer_pos2 + 1; k < end_pos; ++k) {
            duration2 += train2.stopover_time_[k];
          }


          TicketInfo cur_ticket1{train1.train_id_, start_time1 + elapsed_time1,
                                 duration1, price1, max_seat1};
          TicketInfo cur_ticket2{train2.train_id_, start_time2 + elapsed_time2,
                                 duration2, price2, max_seat2};
          if (order_by_cost && !ticket1.train_id_.empty() &&
              OrderByPrice(ticket1, ticket2, cur_ticket1, cur_ticket2)) {
            continue;
          }
          if (!order_by_cost && !ticket1.train_id_.empty() &&
              OrderByTime(ticket1, ticket2, cur_ticket1, cur_ticket2)) {
            continue;
          }
          ticket1 = cur_ticket1;
          ticket2 = cur_ticket2;
          transfer = train1.stations_[transfer_pos1];
        }
      }
    }
  }
  if (ticket1.train_id_.empty()) {
    cout << 0 << endl;
  } else {
    cout << ticket1.train_id_ << " " << start << " " << ticket1.leave_ << " -> "
         << transfer << " " << ticket1.leave_ + ticket1.duration_ << " "
         << ticket1.price_ << " " << ticket1.seat_ << endl;
    cout << ticket2.train_id_ << " " << transfer << " " << ticket2.leave_ << " -> "
         << end << " " << ticket2.leave_ + ticket2.duration_ << " "
         << ticket2.price_ << " " << ticket2.seat_ << endl;
  }
}

void TrainSystem::BuyTicket(const string para[26], const shared_ptr<UserSystem> &user_system) {
  const string &username = para['u' - 'a'];
  const string &train_id = para['i' - 'a'];
  Date date(para['d' - 'a']);
  int32_t num = std::stoi(para['n' - 'a']);
  const string &start = para['f' - 'a'];
  const string &end = para['t' - 'a'];
  bool queue = (para['q' - 'a'] == "true");

  if (!user_system->LoginStatus(username)) {
    Fail();
    return;
  }
  TrainInfo brief{};
  FetchTrainInfo(train_id, brief);
  if (brief.released_ == false) {
    Fail();
    return;
  }
  DetailedTrainInfo info{};
  FetchDetailedTrainInfo(brief, info);
  int elapsed_time = 0;
  int start_pos = -1, end_pos = -1;
  for (int i = 0; i < info.station_num_; ++i) {
    if (info.stations_[i] == start) {
      start_pos = i;
    } else if (info.stations_[i] == end) {
      end_pos = i;
    }
  }
  for (int i = 0; i < start_pos; ++i) {
    elapsed_time += info.travel_time_[i];
  }
  for (int i = 0; i <= start_pos; ++i) {
    elapsed_time += info.stopover_time_[i];
  }
  Time start_time = Time(date, {0, 0}) - elapsed_time;
  if (start_time.GetMoment() <= info.start_time_) {
    start_time.SetMoment(info.start_time_);
  } else {
    start_time += 1440;
    start_time.SetMoment(info.start_time_);
  }
  if (start_time.GetDate() < info.start_sale_ || start_time.GetDate() > info.end_sale_) {
    Fail();
    return;
  }
  ticket_system_->FetchTicket(start_time.GetDate(), brief.seat_num_, info);
  int price = 0;
  int max_seat = 100000;
  int duration = 0;
  for (int i = start_pos; i < end_pos; ++i) {
    price += info.prices_[i];
    max_seat = std::min(max_seat, info.seat_num_[i]);
    duration += info.travel_time_[i];
  }
  for (int i = start_pos + 1; i < end_pos; ++i) {
    duration += info.stopover_time_[i];
  }
  OrderInfo order{};
  start.copy(order.from_, string::npos);
  end.copy(order.to_, string::npos);
  train_id.copy(order.train_id_, string::npos);
  order.leave_ = start_time + elapsed_time;
  order.arrive_ = order.leave_ + duration;
  order.timestamp_ = -1;
  order.num_ = num;
  order.price_ = price;
  if (num <= max_seat) {
    for (int i = start_pos; i < end_pos; ++i) {
      info.seat_num_[i] -= num;
    }
    ticket_system_->ModifyTicket(start_time.GetDate(), info);
    cout << 1ll * num * price << endl;
    order.status_ = OrderStatus::ksuccess;
  } else if (queue) {
    cout << "queue" << endl;
    order.status_ = OrderStatus::kpending;
    order.timestamp_ = waitlist_->Insert(train_id, start_time.GetDate(), username, start_pos, end_pos, num);
  } else {
    Fail();
    return;
  }
  orderlist_->AppendOrder(username, order);
}

void TrainSystem::QueryOrder(const string para[26], const shared_ptr<UserSystem> &user_system) const {
  const string &username = para['u' - 'a'];
  if (!user_system->LoginStatus(username)) {
    Fail();
    return;
  }
  orderlist_->QueryOrder(username);
}

void TrainSystem::RefundTicket(const string para[26], const shared_ptr<UserSystem> &user_system) {
  const string &username = para['u' - 'a'];
  int num = para['n' - 'a'].empty() ? 1 : std::stoi(para['n' - 'a']);

  if (!user_system->LoginStatus(username)) {
    Fail();
    return;
  }
  OrderInfo order{};
  if (!orderlist_->RefundTicket(username, num, order)) {
    Fail();
    return;
  }

  TrainInfo brief{};
  FetchTrainInfo({order.train_id_}, brief);
  DetailedTrainInfo info{};
  FetchDetailedTrainInfo(brief, info);
  int elapsed_time = 0;
  int start_pos = -1, end_pos = -1;
  for (int i = 0; i < info.station_num_; ++i) {
    if (info.stations_[i] == order.from_) {
      start_pos = i;
    } else if (info.stations_[i] == order.to_) {
      end_pos = i;
      break;
    }
  }
  for (int i = 0; i < start_pos; ++i) {
    elapsed_time += info.travel_time_[i];
  }
  for (int i = 0; i <= start_pos; ++i) {
    elapsed_time += info.stopover_time_[i];
  }
  Time start_time = order.leave_ - elapsed_time;
  ticket_system_->FetchTicket(start_time.GetDate(), info.max_seat_, info);
  for (int i = start_pos; i < end_pos; ++i) {
    info.seat_num_[i] += order.num_;
  }

  auto it = waitlist_->FetchWaitlist(info.train_id_, start_time.GetDate());
  while (it) {
    auto &wait_info = *it;
    if (!wait_info.queue) {
      if (it.IsEnd()) {
        break;
      }
      ++it;
      continue;
    }
    int max_seat = 100000;
    for (int i = wait_info.start_pos_; i < wait_info.end_pos_; ++i) {
      max_seat = std::min(max_seat, info.seat_num_[i]);
    }
    if (max_seat >= wait_info.num_) {
      orderlist_->QueueSucceed(wait_info.username_, wait_info.timestamp_);
      for (int i = wait_info.start_pos_; i < wait_info.end_pos_; ++i) {
        info.seat_num_[i] -= wait_info.num_;
      }
      wait_info.queue = false;
    }
    if (it.IsEnd()) {
      break;
    }
    ++it;
  }
  ticket_system_->ModifyTicket(start_time.GetDate(), info);
  Succeed();
}
