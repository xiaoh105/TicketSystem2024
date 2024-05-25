#pragma once

#include <string>
#include <cstdint>
#include <compare>

using std::string;

struct Date {
  int8_t month_;
  int8_t date_;
  Date(int8_t month, int8_t date) : month_(month), date_(date) {};
  explicit Date(const string &format);
  bool operator==(const Date&) const = default;
  [[nodiscard]] string ToString() const;
};

std::ostream &operator<<(std::ostream &os, const Date &date);

std::strong_ordering operator<=>(const Date &lhs, const Date &rhs);

struct Moment {
  int8_t hour_;
  int8_t minute_;
  Moment(int8_t hour, int8_t minute) : hour_(hour), minute_(minute) {};
  explicit Moment(const string &format);
  bool operator==(const Moment&) const = default;
  [[nodiscard]] string ToString() const;
};

std::ostream &operator<<(std::ostream &os, const Moment &moment);

std::strong_ordering operator<=>(const Moment &lhs, const Moment &rhs);

class Time {
  friend std::strong_ordering operator<=>(const Time &lhs, const Time&rhs);
  friend std::ostream &operator<<(std::ostream &os, const Time &time);

 public:
  Time() = delete;
  Time(Date date, Moment moment);
  Time(const Time &other) = default;
  Time &operator=(const Time &other) = default;
  [[nodiscard]] Date GetDate() const { return date_; }
  [[nodiscard]] Moment GetMoment() const { return moment_; }
  void SetMoment(Moment moment) { moment_ = moment; }
  Time operator+(int minute) const;
  Time operator-(int minute) const;
  Time &operator+=(int minute);
  Time &operator-=(int minute);
  string ToString() const;

 private:
  Date date_;
  Moment moment_;
};
