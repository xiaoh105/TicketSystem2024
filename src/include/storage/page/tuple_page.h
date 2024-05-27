#pragma once

#include <string>

#include "common/config.h"

#define TUPLE_HEADER_SIZE 4
#define TUPLE_MAX_SIZE ((BUSTUB_PAGE_SIZE - TUPLE_HEADER_SIZE) / sizeof(T))
#define LINKED_TUPLE_HEADER_SIZE 8
#define LINKED_TUPLE_MAX_SIZE ((BUSTUB_PAGE_SIZE - LINKED_TUPLE_HEADER_SIZE) / sizeof(T))
#define DYNAMIC_TUPLE_HEADER_SIZE 4

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
  [[nodiscard]] int32_t Size() const { return size_; }
  [[nodiscard]] static int32_t MaxSize() { return TUPLE_MAX_SIZE; }

private:
  T data_[TUPLE_MAX_SIZE]{};
  int32_t size_{0};
};

template <class T>
class LinkedTuplePage {
  static_assert(LINKED_TUPLE_MAX_SIZE > 0);
public:
  LinkedTuplePage() = default;
  void SetNextPageId(page_id_t id);
  T &operator[](std::size_t id);
  const T &At(std::size_t id) const;
  int32_t Append(const T &val);
  [[nodiscard]] bool Full() const;
  [[nodiscard]] bool Empty() const;
  [[nodiscard]] int32_t Size() const { return size_; }
  [[nodiscard]] int32_t GetNextPageId() const;
  [[nodiscard]] static int32_t MaxSize() { return LINKED_TUPLE_MAX_SIZE; }

private:
  T data_[LINKED_TUPLE_MAX_SIZE]{};
  int32_t size_{0};
  page_id_t next_page_id_{INVALID_PAGE_ID};
};

using std::string;

class DynamicTuplePage {
 public:
  DynamicTuplePage() = default;

  int32_t Append(const string &data);

  [[nodiscard]] string At(std::size_t pos) const;

  [[nodiscard]] bool IsFull(const string &data) const;

  template <class T>
  bool IsFull(const T *data, std::size_t n) const {
    return size_ + sizeof(T) * n > BUSTUB_PAGE_SIZE;
  }

  template <class T>
  int32_t Append(const T *data, std::size_t n) {
    auto ret = size_;
    memcpy(data_ + size_, data, sizeof(T) * n);
    size_ += sizeof(T) * n;
    return ret;
  }

  template <class T>
  void As(std::size_t pos, T *data, std::size_t n) const {
    memcpy(data, data_ + pos, sizeof(T) * n);
  }

  template <class T>
  void Modify(std::size_t pos, T *data, std::size_t n) {
    memcpy(data_ + pos, data, sizeof(T) * n);
  }

 private:
  char data_[BUSTUB_PAGE_SIZE - DYNAMIC_TUPLE_HEADER_SIZE]{};
  int32_t size_{0};
};
