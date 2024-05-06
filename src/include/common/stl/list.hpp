#pragma once

#include <exception>
#include <cstddef>
/**
 * a data container like std::list
 * allocate random memory addresses for data and they are doubly-linked in a
 * list.
 */
template <typename T>
class list {
protected:
  class node {
  public:
    node *prev, *next;
    T val;
    node(): prev(nullptr), next(nullptr), val() {}
    node(const T &val_, node *prev_ = nullptr, node *next_ = nullptr): prev(prev_), next(next_), val(val_) {
      if (prev) { prev->next = this; }
      if (next) { next->prev = this; }
    }
    ~node() = default;
    void Insert(node *prev_, node *next_) {
      prev = prev_, next = next_;
      if (prev_) { prev->next = this; }
      if (next_) { next->prev = this; }
    }
  };

protected:
  node *head, *tail;
  int sz;

  /**
   * insert node cur before node pos
   * return the inserted node cur
   */
  node *insert(node *pos, node *cur) {
    cur->Insert(pos->prev, pos);
    return cur;
  }
  /**
   * remove node pos from list (no need to delete the node)
   * return the removed node pos
   */
  node *erase(node *pos) {
    pos->prev->next = pos->next;
    pos->next->prev = pos->prev;
    return pos;
  }

public:
  class const_iterator;
  class iterator {
    friend class list<T>;
    friend class const_iterator;
  public:
    iterator(): ptr(nullptr) {}
    iterator(node *ptr_): ptr(ptr_) {}
    iterator operator++(int) {
      auto ret = ptr;
      ptr = ptr->next;
      return ret;
    }
    iterator &operator++() {
      ptr = ptr->next;
      return *this;
    }
    iterator operator--(int) {
      auto ret = ptr;
      ptr = ptr->prev;
      return ret;
    }
    iterator &operator--() {
      ptr = ptr->prev;
      return *this;
    }

    /**
     * throw std::exception if iterator is invalid
     */
    T &operator*() const {
      if (!ptr) { throw std::exception(); }
      return ptr->val;
    }
    /**
     * throw std::exception if iterator is invalid
     */
    T *operator->() const noexcept {
      return &ptr->val;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    bool operator==(const iterator &rhs) const {
      return ptr == rhs.ptr;
    }
    bool operator==(const const_iterator &rhs) const {
      return ptr == rhs.ptr;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return ptr != rhs.ptr;
    }
    bool operator!=(const const_iterator &rhs) const {
      return ptr != rhs.ptr;
    }

  private:
    node *ptr;
  };

  /**
   * has same function as iterator, just for a const object.
   * should be able to construct from an iterator.
   */
  class const_iterator {
    friend class iterator;
  public:
    const_iterator(): ptr(nullptr) {}
    const_iterator(const node *ptr_): ptr(ptr_) {}
    const_iterator(const iterator &it): ptr(it.ptr) {}
    const_iterator operator++(int) {
      auto ret = ptr;
      ptr = ptr->next;
      return ret;
    }
    const_iterator &operator++() {
      ptr = ptr->next;
      return *this;
    }
    const_iterator operator--(int) {
      auto ret = ptr;
      ptr = ptr->prev;
      return ret;
    }
    const_iterator &operator--() {
      ptr = ptr->prev;
      return *this;
    }
    /**
     * throw std::exception if iterator is invalid
     */
    const T &operator*() const {
      if (!ptr) { throw std::exception(); }
      return ptr->val;
    }
    /**
     * throw std::exception if iterator is invalid
     */
    const T *operator->() const noexcept {
      return &ptr->val;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory).
     */
    bool operator==(const iterator &rhs) const {
      return ptr == rhs.ptr;
    }
    bool operator==(const const_iterator &rhs) const {
      return ptr == rhs.ptr;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return ptr != rhs.ptr;
    }
    bool operator!=(const const_iterator &rhs) const {
      return ptr != rhs.ptr;
    }

    const node *ptr;
  };

  /**
   * At least two: default constructor, copy constructor
   */
  list(): head(nullptr), tail(nullptr), sz(0) {
    head = new node();
    tail = new node();
    head->next = tail, tail->prev = head;
  }
  list(const list &other): list() {
    sz = other.sz;
    for (auto it = other.cbegin(); it != other.cend(); ++it) {
      new node(*it, tail->prev, tail);
    }
  }
  virtual ~list() {
    auto ptr = head;
    while (ptr) {
      auto tmp = ptr->next;
      delete ptr;
      ptr = tmp;
    }
  }
  list &operator=(const list &other) {
    if (this == &other) return *this;
    auto ptr = head;
    while (ptr) {
      auto tmp = ptr->next;
      delete ptr;
      ptr = tmp;
    }
    head = new node();
    tail = new node();
    head->next = tail, tail->prev = head;
    sz = other.sz;
    for (auto it = other.cbegin(); it != other.cend(); ++it) {
      new node(*it, tail->prev, tail);
    }
    return *this;
  }
  /**
   * access the first / last element
   * throw container_is_empty when the container is empty.
   */
  T &front() const {
    if (!sz) throw std::exception();
    return head->next->val;
  }
  T &back() const {
    if (!sz) throw std::exception();
    return tail->prev->val;
  }
  /**
   * returns an iterator to the beginning.
   */
  iterator begin() {
    return iterator(head->next);
  }
  const_iterator cbegin() const {
    return const_iterator(head->next);
  }
  /**
   * returns an iterator to the end.
   */
  iterator end() {
    return iterator(tail);
  }
  const_iterator cend() const {
    return const_iterator(tail);
  }
  /**
   * checks whether the container is empty.
   */
  virtual bool empty() const { return sz == 0; }
  /**
   * returns the number of elements
   */
  virtual size_t size() const { return sz; }

  /**
   * clears the contents
   */
  virtual void clear() {
    auto ptr = head;
    while (ptr) {
      auto tmp = ptr->next;
      delete ptr;
      ptr = tmp;
    }
    head = new node();
    tail = new node();
    head->next = tail, tail->prev = head;
    sz = 0;
  }
  /**
   * insert value before pos (pos may be the end() iterator)
   * return an iterator pointing to the inserted value
   * throw if the iterator is invalid
   */
  virtual iterator insert(iterator pos, const T &value) {
    if (!pos.ptr) throw std::exception();
    ++sz;
    return insert(pos.ptr, new node(value));
  }
  /**
   * remove the element at pos (the end() iterator is invalid)
   * returns an iterator pointing to the following element, if pos pointing to
   * the last element, end() will be returned. throw if the container is empty,
   * the iterator is invalid
   */
  virtual iterator erase(iterator pos) {
    if (empty() || pos == end() || !pos.ptr) throw std::exception();
    --sz;
    auto tmp = pos.ptr->next;
    delete erase(pos.ptr);
    return tmp;
  }
  /**
   * adds an element to the end
   */
  void push_back(const T &value) {
    ++sz;
    insert(tail, new node(value));
  }
  void push_back(T &&value) {
    ++sz;
    insert(tail, new node(std::forward<T>(value)));
  }
  /**
   * removes the last element
   * throw when the container is empty.
   */
  void pop_back() {
    if (empty()) {
      throw std::exception();
    }
    --sz;
    delete erase(tail->prev);
  }
  /**
   * inserts an element to the beginning.
   */
  void push_front(const T &value) {
    ++sz;
    insert(head->next, new node(value));
  }
  void push_front(T &&value) {
    ++sz;
    insert(head->next, new node(std::forward<T>(value)));
  }
  /**
   * removes the first element.
   * throw when the container is empty.
   */
  void pop_front() {
    if (empty()) {
      throw std::exception();
    }
    --sz;
    delete erase(head->next);
  }
};