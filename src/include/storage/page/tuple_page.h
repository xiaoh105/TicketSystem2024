#pragma once
#include "common/config.h"

#define TUPLE_HEADER_SIZE 4
#define TUPLE_MAX_SIZE ((BUSTUB_PAGE_SIZE - TUPLE_HEADER_SIZE) / sizeof(T))
#define LINKED_TUPLE_HEADER_SIZE 8
#define LINKED_TUPLE_MAX_SIZE ((BUSTUB_PAGE_SIZE - LINKED_TUPLE_HEADER_SIZE) / sizeof(T))

template <class T>
class TuplePage {
  static_assert(TUPLE_MAX_SIZE > 0);
public:
  TuplePage() = default;
  T &operator[](std::size_t id);
  T At(std::size_t id) const;
  int32_t Append(const T &val);
  [[nodiscard]] bool Full() const;
  [[nodiscard]] bool Empty() const;

private:
  T data_[TUPLE_MAX_SIZE]{};
  int32_t size_{0};
};

template <class T>
class LinkedTuplePage {
  static_assert(LINKED_TUPLE_MAX_SIZE > 0);
public:
  LinkedTuplePage() = default;
  int32_t GetNextPageId() const;
  void SetNextPageId(page_id_t id);

private:
  page_id_t next_page_id_{INVALID_PAGE_ID};
};