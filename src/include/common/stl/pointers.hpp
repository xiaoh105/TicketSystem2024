#pragma once

#include <utility>

/**
 * A simple self-implemented class template unique_ptr (without deleter).
 * @tparam T The class of the pointer.
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

/**
 * A simple self-implemented class template shared_ptr (without deleter).
 * @tparam T The class of the pointer.
 */
template <class T>
class shared_ptr {
 public:
  shared_ptr();
  explicit shared_ptr(T *ptr);
  shared_ptr(const shared_ptr &other);
  shared_ptr(shared_ptr &&other) noexcept;
  ~shared_ptr();
  shared_ptr &operator=(const shared_ptr &other);
  shared_ptr &operator=(shared_ptr &&other) noexcept;
  explicit operator bool() const;
  T &operator*() const;
  T *operator->() const;
  T *get() const;

private:
  void drop();
  T *ptr_;
  std::size_t *counter_;
};

template <class T, class... Args>
unique_ptr<T> make_unique(Args&&... args);

template <class T, class... Args>
shared_ptr<T> make_shared(Args&&... args);

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

template <class T>
shared_ptr<T>::shared_ptr() : ptr_(nullptr), counter_(nullptr) {}

template <class T>
shared_ptr<T>::shared_ptr(T *ptr) : ptr_(ptr), counter_(new std::size_t(1)) {}

template <class T>
shared_ptr<T>::shared_ptr(const shared_ptr<T> &other)
: ptr_(other.ptr_), counter_(other.counter_) {
  ++*counter_;
}

template <class T>
shared_ptr<T>::shared_ptr(shared_ptr<T> &&other) noexcept
: ptr_(other.ptr_), counter_(other.counter_) {
  other.ptr_ = nullptr;
  other.counter_ = nullptr;
}

template <class T>
void shared_ptr<T>::drop() {
  if (ptr_) {
    --*counter_;
    if (*counter_ == 0) {
      delete ptr_;
      delete counter_;
    }
    counter_ = nullptr;
    ptr_ = nullptr;
  }
}

template <class T>
shared_ptr<T>::~shared_ptr() {
  drop();
}

template <class T>
shared_ptr<T> &shared_ptr<T>::operator=(const shared_ptr<T> &other) {
  if (this == &other) {
    return *this;
  }
  drop();
  if (other.ptr_) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    ++*counter_;
  }
}

template <class T>
shared_ptr<T> &shared_ptr<T>::operator=(shared_ptr<T> &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  drop();
  if (other.ptr_) {
    ptr_ = other.ptr_;
    counter_ = other.counter_;
    other.ptr_ = nullptr;
    other.counter_ = nullptr;
  }
}

template <class T>
T &shared_ptr<T>::operator*() const {
  return *ptr_;
}

template <class T>
T *shared_ptr<T>::operator->() const {
  return ptr_;
}

template <class T>
shared_ptr<T>::operator bool() const {
  return ptr_ != nullptr;
}

template <class T>
T *shared_ptr<T>::get() const {
  return ptr_;
}

template <class T, class... Args>
shared_ptr<T> make_shared(Args&&... args) {
  return shared_ptr<T>(new T(std::forward<Args>(args)...));
}