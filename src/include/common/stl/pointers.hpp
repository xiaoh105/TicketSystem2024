#pragma once

#include <utility>

/**
 * This file implements the smart pointers needed for the project.
 */

/**
 * A simple self-implemented class template unique_ptr (without deleter).
 * @tparam T The typename of the pointer.
 */
template <class T>
class unique_ptr {
 public:
  unique_ptr();
  explicit unique_ptr(T *ptr);
  unique_ptr(const unique_ptr &other) = delete;
  unique_ptr(unique_ptr &&other) noexcept;
  ~unique_ptr();
  unique_ptr &operator=(const unique_ptr &other) = delete;
  unique_ptr &operator=(unique_ptr &&other) noexcept;
  T *get() const;
  T &operator*() const;
  T *operator->() const;
  explicit operator bool() const;

 private:
  T *ptr_;
};

template <class T, class... Args>
unique_ptr<T> make_unique(Args&&... args);

template <class T>
unique_ptr<T>::unique_ptr() : ptr_(nullptr) {}

template <class T>
unique_ptr<T>::unique_ptr(T *ptr) : ptr_(ptr) {}

template <class T>
unique_ptr<T>::unique_ptr(unique_ptr<T> &&other) noexcept {
  ptr_ = other.ptr_;
  other.ptr_ = nullptr;
}

template <class T>
unique_ptr<T>::~unique_ptr() {
  delete ptr_;
}

template <class T>
unique_ptr<T> &unique_ptr<T>::operator=(unique_ptr<T> &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  ptr_ = other.ptr_;
  other.ptr_ = nullptr;
  return *this;
}

template <class T>
T *unique_ptr<T>::get() const { return ptr_; }

template <class T>
T &unique_ptr<T>::operator*() const {
  return *ptr_;
}

template <class T>
T *unique_ptr<T>::operator->() const {
  return ptr_;
}

template <class T>
unique_ptr<T>::operator bool() const {
  return ptr_ != nullptr;
}

template <class T, class... Args>
unique_ptr<T> make_unique(Args&&... args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}
