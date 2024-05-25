#include "common/time.h"

#include <sstream>
#include <iomanip>

Date::Date(const string &format) : month_(), date_() {
  month_ = (format[0] - '0') * 10 + (format[1] - '0'); //NOLINT
  date_ = (format[3] - '0') * 10 + (format[4] - '0'); // NOLINT
}

string Date::ToString() const {
  std::ostringstream sbuf;
  sbuf << *this;
  return sbuf.str();
}

std::ostream &operator<<(std::ostream& os, const Date& date) {
  os << std::setfill('0');
  os << std::setw(2) << static_cast<int>(date.month_) << "-";
  os << std::setw(2) << static_cast<int>(date.date_);
  return os;
}

string Moment::ToString() const {
  std::ostringstream sbuf;
  sbuf << *this;
  return sbuf.str();
}

std::strong_ordering operator<=>(const Date &lhs, const Date &rhs) {
  return lhs.month_ != rhs.month_ ? lhs.month_ <=> rhs.month_ : lhs.date_ <=> rhs.date_;
}

Moment::Moment(const string &format) {
  hour_ = (format[0] - '0') * 10 + (format[1] - '0'); // NOLINT
  minute_ = (format[3] - '0') * 10 + (format[4] - '0'); // NOLINT
}

std::ostream &operator<<(std::ostream& os, const Moment& moment) {
  os << std::setfill('0');
  os << std::setw(2) << static_cast<int>(moment.hour_) << ":";
  os << std::setw(2) << static_cast<int>(moment.minute_);
  return os;
}

std::strong_ordering operator<=>(const Moment &lhs, const Moment &rhs) {
  return lhs.hour_ != rhs.hour_ ? lhs.hour_ <=> rhs.hour_ : lhs.minute_ <=> rhs.minute_;
}

std::ostream& operator<<(std::ostream& os, const Time& time) {
  os << time.date_ << " " << time.moment_;
  return os;
}

std::strong_ordering operator<=>(const Time& lhs, const Time& rhs) {
  return lhs.date_ != rhs.date_ ? lhs.date_ <=> rhs.date_ : lhs.moment_ <=> rhs.moment_;
}

Time::Time(Date date, Moment moment) : date_(date), moment_(moment) {}

Time Time::operator+(int minute) const {
  minute += moment_.minute_;
  auto ret_min = static_cast<int8_t>(minute % 60);
  minute /= 60;

  minute += moment_.hour_;
  auto ret_hour = static_cast<int8_t>(minute % 24);
  minute /= 24;

  minute += date_.date_;
  auto ret_date = static_cast<int8_t>(date_.month_ == 6 ? minute % 30 : minute % 31);
  minute /= (date_.month_ == 6 ? 30 : 31);

  minute += date_.month_;
  auto ret_month = static_cast<int8_t>(minute);
  return {{ret_month, ret_date}, {ret_hour, ret_min}};
}

Time& Time::operator+=(int minute) {
  return *this = *this + minute;
}

Time Time::operator-(int minute) const {
  int ret_month = date_.month_;
  int ret_date = date_.date_;
  int ret_hour = moment_.hour_;
  int ret_min = moment_.minute_;

  ret_min -= minute;
  while (ret_min < 0) {
    ret_min += 60;
    --ret_hour;
  }
  while (ret_hour < 0) {
    ret_hour += 24;
    --ret_date;
  }
  if (ret_date < 0) {
    ret_date += (ret_month == 7 ? 30 : 31);
    --ret_month;
  }
  return {{static_cast<int8_t>(ret_month), static_cast<int8_t>(ret_date)},
    {static_cast<int8_t>(ret_hour), static_cast<int8_t>(ret_min)}};
}

Time& Time::operator-=(int minute) {
  return *this = *this - minute;
}

string Time::ToString() const {
  std::ostringstream sbuf;
  sbuf << *this;
  return sbuf.str();
}

