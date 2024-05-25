#pragma once

#include <string_view>
#include <string>

#include "common/stl/vector.hpp"

constexpr unsigned long long mod = 19260817;

[[gnu::const]]
inline unsigned long long StringHash(std::string_view s) {
  unsigned long long ret = 0;
  for (int i = 0; i < s.length(); ++i) {
    ret = ret * mod + s[i];
  }
  return ret;
}

inline void Succeed() {
  std::cout << 0 << std::endl;
}

inline void Fail() {
  std::cout << -1 << std::endl;
}

inline vector<std::string> SplitString(const std::string &s) {
  vector<std::string> ret;
  std::string tmp{};
  for (const auto &ch : s) {
    if (ch == '|') {
      ret.push_back(tmp);
      tmp.clear();
    } else {
      tmp += ch;
    }
  }
  if (!tmp.empty()) {
    ret.push_back(tmp);
  }
  return ret;
}

template <class T>
T *SplitAndConvert(const std::string &s, std::size_t n, std::size_t st) {
  auto ret = new T[n]{};
  std::string tmp{};
  for (const auto &ch : s) {
    if (ch == '|') {
      ret[st++] = static_cast<T>(std::stoi(tmp));
      tmp.clear();
    } else {
      tmp += ch;
    }
  }
  if (!tmp.empty()) {
    ret[st++] = static_cast<T>(std::stoi(tmp));
  }
  return ret;
}

template <class T>
void sort(T *begin, T *end, bool(*cmp)(const T &, const T &))
{
  int n = static_cast<int>(end - begin);
  if (n == 0 || n == 1) return;
  int mid = n >> 1;
  int n1 = mid, n2 = n - mid;
  sort(begin, begin + mid, cmp);
  sort(begin + mid, end, cmp);
  T *tmp = new T[n];
  int p1 = 0, p2 = 0, p = 0;
  while (p1 < n1 || p2 < n2)
  {
    if (p2 == n2 || p1 < n1 && cmp(*(begin + p1), *(begin + mid + p2)))
    {
      tmp[p++] = *(begin + p1);
      ++p1;
    }
    else
    {
      tmp[p++] = *(begin + mid + p2);
      ++p2;
    }
  }
  for (int i = 0; i < n; ++i) *(begin + i) = tmp[i];
  delete [] tmp;
}

template <class T>
void sort(T *begin, T *end)
{
  int n = static_cast<int>(end - begin);
  if (n == 0 || n == 1) return;
  int mid = n >> 1;
  int n1 = mid, n2 = n - mid;
  sort(begin, begin + mid);
  sort(begin + mid, end);
  T *tmp = new T[n];
  int p1 = 0, p2 = 0, p = 0;
  while (p1 < n1 || p2 < n2)
  {
    if (p2 == n2 || p1 < n1 && *(begin + p1) <= *(begin + mid + p2))
    {
      tmp[p++] = *(begin + p1);
      ++p1;
    }
    else
    {
      tmp[p++] = *(begin + mid + p2);
      ++p2;
    }
  }
  for (int i = 0; i < n; ++i) *(begin + i) = tmp[i];
  delete [] tmp;
}
