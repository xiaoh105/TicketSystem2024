#pragma once

#include <climits>
#include <cstddef>
#include <memory>

/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */
template<typename T>
class vector {
private:
  size_t len, capacity;
  T *a;
  std::allocator<T> alloc;
  T *AssignMemory(size_t size) {
    return alloc.allocate(size);
  }
  void Release() {
    for (int i = 0; i < len; ++i) a[i].~T();
    alloc.deallocate(a, capacity);
  }
  void Expand(size_t target_size) {
    if (target_size >= capacity) {
      T *new_a = AssignMemory(capacity << 1);
      for (int i = 0; i < len; ++i) std::construct_at(new_a + i, a[i]);
      Release();
      a = new_a;
      capacity <<= 1;
    }
  }
  void Shrink() {
    if (capacity >= 2 && (capacity >> 2) > len) {
      while (capacity >= 2 && (capacity >> 2) > len) capacity >>= 1;
      T *new_a = AssignMemory(capacity);
      for (int i = 0; i < len; ++i) std::construct_at(new_a + i, a[i]);
      Release();
      a = new_a;
    }
  }

  void __sort(T *begin, T *end)
  {
    int n = static_cast<int>(end - begin);
    if (n == 0 || n == 1) return;
    int mid = n >> 1;
    int n1 = mid, n2 = n - mid;
    __sort(begin, begin + mid);
    __sort(begin + mid, end);
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

  void __sort(T *begin, T *end, bool(*cmp)(const T &, const T &))
  {
    int n = static_cast<int>(end - begin);
    if (n == 0 || n == 1) return;
    int mid = n >> 1;
    int n1 = mid, n2 = n - mid;
    __sort(begin, begin + mid, cmp);
    __sort(begin + mid, end, cmp);
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

public:
  /**
   * a type for actions of the elements of a vector, and you should write
   *   a class named const_iterator with same interfaces.
   */
  /**
   * you can see RandomAccessIterator at CppReference for help.
   */
  class const_iterator;

  class iterator {
    friend class vector;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::output_iterator_tag;
  private:
    T *head;
    int index;
    iterator(): head(), index() {}
    iterator(T *head_, int index_): head(head_), index(index_) {}
  public:
    /**
     * return a new iterator which pointer n-next elements
     * as well as operator-
     */
    iterator operator+(const int &n) const {
      return { head, index + n };
    }

    iterator operator-(const int &n) const {
      return { head, index - n };
    }

    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw invalid_iterator.
    int operator-(const iterator &rhs) const {
      if (head != rhs.head) throw std::exception();
      return index - rhs.index;
    }

    iterator &operator+=(const int &n) {
      index += n;
      return *this;
    }

    iterator &operator-=(const int &n) {
      index -= n;
      return *this;
    }

    iterator operator++(int) {
      auto ret = *this;
      ++index;
      return ret;
    }

    iterator &operator++() {
      ++index;
      return *this;
    }

    iterator operator--(int) {
      auto ret = *this;
      --index;
      return ret;
    }

    iterator &operator--() {
      --index;
      return *this;
    }

    T &operator*() const {
      return head[index];
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory address).
     */
    bool operator==(const iterator &rhs) const {
      return head == rhs.head && index == rhs.index;
    }

    bool operator==(const const_iterator &rhs) const {
      return head == rhs.head && index == rhs.index;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return head != rhs.head || index != rhs.index;
    }

    bool operator!=(const const_iterator &rhs) const {
      return head != rhs.head || index != rhs.index;
    }
  };

  class const_iterator {
    friend class vector;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::output_iterator_tag;
  private:
    T *head;
    int index;
    const_iterator(): head(), index() {}
    const_iterator(T *head_, int index_): head(head_), index(index_) {}
  public:
    /**
     * return a new iterator which pointer n-next elements
     * as well as operator-
     */
    const_iterator operator+(const int &n) const {
      return { head, index + n };
    }

    const_iterator operator-(const int &n) const {
      return { head, index - n };
    }

    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw invalid_iterator.
    int operator-(const const_iterator &rhs) const {
      if (head != rhs.head) throw std::exception();
      return index - rhs.index;
    }

    const_iterator &operator+=(const int &n) {
      index += n;
      return *this;
    }

    const_iterator &operator-=(const int &n) {
      index -= n;
      return *this;
    }

    const_iterator operator++(int) {
      auto ret = *this;
      ++index;
      return ret;
    }

    const_iterator &operator++() {
      ++index;
      return *this;
    }

    const_iterator operator--(int) {
      auto ret = *this;
      --index;
      return ret;
    }

    const_iterator &operator--() {
      --index;
      return *this;
    }

    const T &operator*() const {
      return head[index];
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory address).
     */
    bool operator==(const iterator &rhs) const {
      return head == rhs.head && index == rhs.index;
    }

    bool operator==(const const_iterator &rhs) const {
      return head == rhs.head && index == rhs.index;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return head != rhs.head || index != rhs.index;
    }

    bool operator!=(const const_iterator &rhs) const {
      return head != rhs.head || index != rhs.index;
    }
  };

  /**
   * At least two: default constructor, copy constructor
   */
  vector() {
    a = AssignMemory(1);
    capacity = 1;
    len = 0;
  }

  vector(const vector &other) {
    a = AssignMemory(other.capacity);
    capacity = other.capacity;
    len = other.len;
    for (int i = 0; i < len; ++i) std::construct_at(a + i, other.a[i]);
  }

  ~vector() {
    Release();
  }

  vector &operator=(const vector &other) {
    if (this == &other) { return *this; }
    Release();
    a = AssignMemory(other.capacity);
    capacity = other.capacity;
    len = other.len;
    for (int i = 0; i < len; ++i) std::construct_at(a + i, other.a[i]);
    return *this;
  }

  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   */
  T &at(const size_t &pos) {
    if (pos < 0 || pos >= len) {
      throw std::exception();
    }
    return a[pos];
  }

  const T &at(const size_t &pos) const {
    if (pos < 0 || pos >= len) {
      throw std::exception();
    }
    return a[pos];
  }

  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   * !!! Pay attentions
   *   In STL this operator does not check the boundary but I want you to do.
   */
  T &operator[](const size_t &pos) {
    if (pos < 0 || pos >= len) {
      throw std::exception();
    }
    return a[pos];
  }

  const T &operator[](const size_t &pos) const {
    if (pos < 0 || pos >= len) {
      throw std::exception();
    }
    return a[pos];
  }

  /**
   * access the first element.
   * throw container_is_empty if size == 0
   */
  const T &front() const {
    if (len == 0) {
      throw std::exception();
    }
    return a[0];
  }

  /**
   * access the last element.
   * throw container_is_empty if size == 0
   */
  const T &back() const {
    if (len == 0) {
      throw std::exception();
    }
    return a[len - 1];
  }

  /**
   * returns an iterator to the beginning.
   */
  iterator begin() {
    return iterator(a, 0);
  }

  const_iterator cbegin() const {
    return const_iterator(a, 0);
  }

  /**
   * returns an iterator to the end.
   */
  iterator end() {
    return iterator(a, len);
  }

  const_iterator cend() const {
    return const_iterator(a, len);
  }

  /**
   * checks whether the container is empty
   */
  bool empty() const {
    return len == 0;
  }

  /**
   * returns the number of elements
   */
  size_t size() const {
    return len;
  }

  /**
   * clears the contents
   */
  void clear() {
    Release();
    a = AssignMemory(1);
    len = 0;
    capacity = 1;
  }

  /**
   * inserts value before pos
   * returns an iterator pointing to the inserted value.
   */
  iterator insert(iterator pos, const T &value) {
    Expand(len + 1);
    std::construct_at(a + len, value);
    for (int i = len - 1; i >= pos.index; --i) a[i + 1] = a[i];
    a[pos.index] = value;
    ++len;
    return pos;
  }

  /**
   * inserts value at index ind.
   * after inserting, this->at(ind) == value
   * returns an iterator pointing to the inserted value.
   * throw index_out_of_bound if ind > size (in this situation ind can be size because after inserting the size will increase 1.)
   */
  iterator insert(const size_t &ind, const T &value) {
    if (ind > len) {
      throw std::exception();
    }
    Expand(len + 1);
    std::construct_at(a + len, value);
    for (int i = len - 1; i >= ind; --i) a[i + 1] = a[i];
    a[ind] = value;
    ++len;
    return iterator(a, ind);
  }

  /**
   * removes the element at pos.
   * return an iterator pointing to the following element.
   * If the iterator pos refers the last element, the end() iterator is returned.
   */
  iterator erase(iterator pos) {
    for (int i = pos.index + 1; i < len; ++i) a[i - 1] = a[i];
    a[len - 1].~T();
    --len;
    Shrink();
    return pos;
  }

  /**
   * removes the element with index ind.
   * return an iterator pointing to the following element.
   * throw index_out_of_bound if ind >= size
   */
  iterator erase(const size_t &ind) {
    if (ind >= len) {
      throw std::exception();
    }
    for (int i = ind + 1; i < len; ++i) a[i - 1] = a[i];
    a[len - 1].~T();
    --len;
    Shrink();
    return iterator(a, ind);
  }

  /**
   * adds an element to the end.
   */
  void push_back(const T &value) {
    Expand(len + 1);
    ++len;
    std::construct_at(a + len - 1, value);
  }

  /**
   * remove the last element from the end.
   * throw container_is_empty if size() == 0
   */
  void pop_back() {
    a[len - 1].~T();
    --len;
    Shrink();
  }

  void sort() {
    __sort(a, a + len);
  }

  void sort(bool(&cmp)(const T &, const T &)) {
    __sort(a, a + len, cmp);
  }
};
