#include <cassert>

#include "storage/page/tuple_page.h"
#include "user/user_system.h"

template <class T>
T& TuplePage<T>::operator[](std::size_t id) {
  if (id >= TUPLE_MAX_SIZE) {
    assert(false);
  }
  return data_[id];
}

template <class T>
T TuplePage<T>::At(std::size_t id) const {
  return data_[id];
}

template <class T>
int32_t TuplePage<T>::Append(const T& val) {
  if (size_ == TUPLE_MAX_SIZE) {
    assert(false);
  }
  data_[size_] = val;
  ++size_;
  return size_ - 1;
}

template <class T>
bool TuplePage<T>::Full() const {
  return size_ == TUPLE_MAX_SIZE;
}

template <class T>
bool TuplePage<T>::Empty() const {
  return size_ == 0;
}

template <class T>
int32_t LinkedTuplePage<T>::GetNextPageId() const {
  return next_page_id_;
}


template <class T>
void LinkedTuplePage<T>::SetNextPageId(page_id_t id) {
  next_page_id_ = id;
}

template class TuplePage<UserProfile>;
