#pragma once

#include <functional>
#include <cstddef>
#include <cassert>
#include <iostream>
#include "common/stl/pair.hpp"

template<
  class Key,
  class T,
  class Compare = std::less<Key>
>
class map {
private:
  enum class Color {
    black, red
  };

  enum class ChildType {
    left, right, root
  };

  class Node {
    friend class map;

  public:
    Node() = delete;

    Node(const Key &key, const T &value, Color color = Color::red,
         Node *parent = nullptr, Node *l = nullptr, Node *r = nullptr) :
      val_({key, value}), color_(color), l_(l), r_(r), parent_(parent) {}

    Node *Clone() {
      auto ret = new Node(val_.first, val_.second, color_);
      if (l_) {
        ret->l_ = l_->Clone();
        ret->l_->parent_ = ret;
      }
      if (r_) {
        ret->r_ = r_->Clone();
        ret->r_->parent_ = ret;
      }
      return ret;
    }

    enum ChildType ChildType() {
      if (parent_ == nullptr) {
        return ChildType::root;
      }
      // assert(this == parent_->l_ || this == parent_->r_);
      return (this == parent_->l_) ? ChildType::left : ChildType::right;
    }

    enum Color Color() {
      return color_;
    }

    bool IsRoot() {
      return parent_ == nullptr;
    }

    bool IsLeaf() {
      return l_ == nullptr && r_ == nullptr;
    }

    Node *Sibling() {
      return (ChildType() == ChildType::left) ? parent_->r_ : parent_->l_;
    }

    Node *Uncle() {
      return parent_->Sibling();
    }

    Node *GrandParent() {
      return parent_->parent_;
    }

    Node *CloseNephew() {
      if (ChildType() == ChildType::left) {
        if (!parent_->r_) {
          return nullptr;
        }
        return parent_->r_->l_;
      } else {
        if (!parent_->l_) {
          return nullptr;
        }
        return parent_->l_->r_;
      }
    }

    Node *DistantNephew() {
      if (ChildType() == ChildType::left) {
        if (!parent_->r_) {
          return nullptr;
        }
        return parent_->r_->r_;
      } else {
        if (!parent_->l_) {
          return nullptr;
        }
        return parent_->l_->l_;
      }
    }

  private:
    Node *l_, *r_, *parent_;
    enum Color color_;
    pair<const Key, T> val_;
  };

  Node *root_ = nullptr;
  size_t size_ = 0;
  Node *begin_ = nullptr, *end_ = nullptr;

  [[nodiscard]] bool IsEqual(const Key &a, const Key &b, const Compare &cmp) const {
    return !(cmp(a, b) ^ cmp(b, a));
  }

  void Destruct(Node *cur) {
    if (cur->l_) {
      Destruct(cur->l_);
    }
    if (cur->r_) {
      Destruct(cur->r_);
    }
    delete cur;
  }

  void RotateLeft(Node *cur) {
    auto tmp = cur->r_;
    cur->r_ = tmp->l_;
    if (cur->r_) {
      cur->r_->parent_ = cur;
    }
    if (cur->ChildType() == ChildType::left) {
      cur->parent_->l_ = tmp;
      tmp->parent_ = cur->parent_;
    } else if (cur->ChildType() == ChildType::right) {
      cur->parent_->r_ = tmp;
      tmp->parent_ = cur->parent_;
    } else {
      root_ = tmp;
      tmp->parent_ = nullptr;
    }
    cur->parent_ = tmp;
    tmp->l_ = cur;
  }

  void RotateRight(Node *cur) {
    auto tmp = cur->l_;
    cur->l_ = tmp->r_;
    if (cur->l_) {
      cur->l_->parent_ = cur;
    }
    if (cur->ChildType() == ChildType::left) {
      cur->parent_->l_ = tmp;
      tmp->parent_ = cur->parent_;
    } else if (cur->ChildType() == ChildType::right) {
      cur->parent_->r_ = tmp;
      tmp->parent_ = cur->parent_;
    } else {
      root_ = tmp;
      tmp->parent_ = nullptr;
    }
    cur->parent_ = tmp;
    tmp->r_ = cur;
  }

  void SwapChildren(Node *a, Node *b) {
    switch (a->ChildType()) {
      case ChildType::left:
        a->parent_->l_ = b;
        break;
      case ChildType::right:
        a->parent_->r_ = b;
        break;
      case ChildType::root:
        root_ = b;
        break;
    }
    switch (b->ChildType()) {
      case ChildType::left:
        a->l_ = b->l_;
        if (a->l_) {
          a->l_->parent_ = a;
        }
        b->l_ = a;
        std::swap(a->r_, b->r_);
        if (a->r_) {
          a->r_->parent_ = a;
        }
        if (b->r_) {
          b->r_->parent_ = b;
        }
        break;
      case ChildType::right:
        a->r_ = b->r_;
        if (a->r_) {
          a->r_->parent_ = a;
        }
        b->r_ = a;
        std::swap(a->l_, b->l_);
        if (a->l_) {
          a->l_->parent_ = a;
        }
        if (b->l_) {
          b->l_->parent_ = b;
        }
        break;
      case ChildType::root:
        assert(false);
    }
    b->parent_ = a->parent_;
    a->parent_ = b;
    std::swap(a->color_, b->color_);
  }

  void SwapNode(Node *a, Node *b) {
    if (a->l_ == b || a->r_ == b) {
      SwapChildren(a, b);
      return;
    }
    if (b->l_ == a || b->r_ == a) {
      SwapChildren(b, a);
      return;
    }
    assert(a->parent_ != b->parent_);
    switch (a->ChildType()) {
      case ChildType::root:
        root_ = b;
        break;
      case ChildType::left:
        a->parent_->l_ = b;
        break;
      case ChildType::right:
        a->parent_->r_ = b;
        break;
    }
    switch (b->ChildType()) {
      case ChildType::root:
        root_ = a;
        break;
      case ChildType::left:
        b->parent_->l_ = a;
        break;
      case ChildType::right:
        b->parent_->r_ = a;
        break;
    }
    if (a->l_) {
      a->l_->parent_ = b;
    }
    if (a->r_) {
      a->r_->parent_ = b;
    }
    if (b->l_) {
      b->l_->parent_ = a;
    }
    if (b->r_) {
      b->r_->parent_ = a;
    }
    std::swap(a->l_, b->l_);
    std::swap(a->r_, b->r_);
    std::swap(a->parent_, b->parent_);
    std::swap(a->color_, b->color_);
  }

  [[nodiscard]] Node *Find(const Key &key) const {
    if (!root_) {
      return nullptr;
    }
    auto cur = root_;
    Compare cmp;
    while (cur != nullptr) {
      if (IsEqual(cur->val_.first, key, cmp)) {
        return cur;
      }
      if (cmp(key, cur->val_.first)) {
        cur = cur->l_;
      } else {
        cur = cur->r_;
      }
    }
    return nullptr;
  }

  Node *GetNext(Node *cur) {
    cur = cur->r_;
    while (cur->l_) {
      cur = cur->l_;
    }
    return cur;
  }

  Node *GetBegin() const {
    if (!root_) {
      return nullptr;
    }
    if (begin_) {
      return begin_;
    }
    auto cur = root_;
    while (cur->l_) {
      cur = cur->l_;
    }
    return cur;
  }

  Node *GetEnd() const {
    if (!root_) {
      return nullptr;
    }
    if (end_) {
      return end_;
    }
    auto cur = root_;
    while (cur->r_) {
      cur = cur->r_;
    }
    return cur;
  }

  void MaintainInsert(Node *cur) {
    bool flag = true;
    while (flag) {
      if (cur->IsRoot()) {
        assert(cur->color_ == Color::red);
        return;
      }
      if (cur->parent_->color_ == Color::black) {
        return;
      }
      if (cur->parent_->IsRoot()) {
        assert(cur->parent_->color_ == Color::red);
        cur->parent_->color_ = Color::black;
        return;
      }
      if (cur->Uncle() && cur->Uncle()->Color() == Color::red) {
        assert(cur->GrandParent()->color_ == Color::black);
        cur->parent_->color_ = cur->Uncle()->color_ = Color::black;
        cur->GrandParent()->color_ = Color::red;
        cur = cur->GrandParent();
      } else {
        assert(!cur->Uncle() || cur->Uncle()->color_ == Color::black);
        if (cur->ChildType() != cur->parent_->ChildType()) {
          auto parent = cur->parent_;
          if (cur->ChildType() == ChildType::left) {
            RotateRight(cur->parent_);
          } else {
            RotateLeft(cur->parent_);
          }
          cur = parent;
        }
        assert(cur->ChildType() == cur->parent_->ChildType());
        cur->GrandParent()->color_ = Color::red;
        cur->parent_->color_ = Color::black;
        if (cur->ChildType() == ChildType::left) {
          RotateRight(cur->GrandParent());
        } else {
          RotateLeft(cur->GrandParent());
        }
        flag = false;
      }
    }
  }

  pair<Node *, bool> Insert(const Key &key, const T &value) {
    if (root_ == nullptr) {
      root_ = new Node(key, value, Color::black);
      begin_ = end_ = root_;
      return {root_, true};
    }
    Node *cur = root_;
    bool flag = true;
    Compare cmp;
    while (flag) {
      if (IsEqual(key, cur->val_.first, cmp)) {
        return {cur, false};
      }
      if (cmp(key, cur->val_.first)) {
        if (!cur->l_) {
          flag = false;
          cur->l_ = new Node(key, value, Color::red, cur);
        }
        cur = cur->l_;
      } else {
        if (!cur->r_) {
          flag = false;
          cur->r_ = new Node(key, value, Color::red, cur);
        }
        cur = cur->r_;
      }
    }
    MaintainInsert(cur);
    if (begin_ && cmp(key, begin_->val_.first)) {
      begin_ = cur;
    }
    if (end_ && cmp(end_->val_.first, key)) {
      end_ = cur;
    }
    return {cur, true};
  }

  void MaintainDeletion(Node *cur) {
    bool flag = true;
    while (flag) {
      if (cur == root_) {
        assert(cur->color_ == Color::black);
        return;
      }
      if (cur->Sibling()->color_ == Color::red) {
        assert(cur->CloseNephew()->color_ == Color::black);
        assert(cur->DistantNephew()->color_ == Color::black);
        assert(cur->parent_->color_ == Color::black);
        cur->Sibling()->color_ = Color::black;
        cur->parent_->color_ = Color::red;
        if (cur->ChildType() == ChildType::left) {
          RotateLeft(cur->parent_);
        } else {
          RotateRight(cur->parent_);
        }
      }
      assert(cur->Sibling()->color_ == Color::black);
      if ((!cur->CloseNephew() || cur->CloseNephew()->color_ == Color::black) &&
          (!cur->DistantNephew() || cur->DistantNephew()->color_ == Color::black) &&
          cur->parent_->color_ == Color::red) {
        cur->Sibling()->color_ = Color::red;
        cur->parent_->color_ = Color::black;
        flag = false;
      } else if ((!cur->CloseNephew() || cur->CloseNephew()->color_ == Color::black) &&
                 (!cur->DistantNephew() || cur->DistantNephew()->color_ == Color::black) &&
                 cur->parent_->color_ == Color::black) {
        cur->Sibling()->color_ = Color::red;
        cur = cur->parent_;
      } else {
        if (cur->CloseNephew() && cur->CloseNephew()->color_ == Color::red &&
            (!cur->DistantNephew() || cur->DistantNephew()->color_ == Color::black)) {
          cur->Sibling()->color_ = Color::red;
          cur->CloseNephew()->color_ = Color::black;
          if (cur->ChildType() == ChildType::left) {
            RotateRight(cur->Sibling());
          } else {
            RotateLeft(cur->Sibling());
          }
        }
        assert(cur->DistantNephew()->color_ == Color::red);
        cur->Sibling()->color_ = cur->parent_->color_;
        cur->parent_->color_ = Color::black;
        cur->DistantNephew()->color_ = Color::black;
        if (cur->ChildType() == ChildType::left) {
          RotateLeft(cur->parent_);
        } else {
          RotateRight(cur->parent_);
        }
        flag = false;
      }
    }
  }

  void Remove(Node *cur) {
    if (begin_ == cur) {
      begin_ = nullptr;
    }
    if (end_ == cur) {
      end_ = nullptr;
    }
    if (size_ == 1) {
      assert(cur == root_);
      delete root_;
      root_ = nullptr;
      return;
    }
    if (cur->l_ && cur->r_) {
      auto tmp = GetNext(cur);
      SwapNode(cur, tmp);
    }
    assert(cur->l_ == nullptr || cur->r_ == nullptr);
    if (cur->IsLeaf()) {
      if (cur->color_ == Color::red) {
        switch (cur->ChildType()) {
          case ChildType::left:
            cur->parent_->l_ = nullptr;
            break;
          case ChildType::right:
            cur->parent_->r_ = nullptr;
            break;
          case ChildType::root:
            assert(false);
            break;
        }
        delete cur;
      } else {
        MaintainDeletion(cur);
        switch (cur->ChildType()) {
          case ChildType::left:
            cur->parent_->l_ = nullptr;
            break;
          case ChildType::right:
            cur->parent_->r_ = nullptr;
            break;
          case ChildType::root:
            assert(false);
            break;
        }
        delete cur;
      }
    } else {
      auto ch = cur->l_ ? cur->l_ : cur->r_;
      assert(ch->color_ == Color::red);
      assert(cur->color_ == Color::black);
      switch (cur->ChildType()) {
        case ChildType::left:
          cur->parent_->l_ = ch;
          ch->parent_ = cur->parent_;
          break;
        case ChildType::right:
          cur->parent_->r_ = ch;
          ch->parent_ = cur->parent_;
          break;
        case ChildType::root:
          root_ = ch;
          break;
        default:
          assert(false);
      }
      if (cur->color_ == Color::black) {
        ch->color_ = Color::black;
      }
      delete cur;
    }
  }

public:
  /**
   * the internal type of data.
   * it should have a default constructor, a copy constructor.
   * You can use sjtu::map as value_type by typedef.
   */
  typedef pair<const Key, T> value_type;

  /**
   * see BidirectionalIterator at CppReference for help.
   *
   * if there is anything wrong throw invalid_iterator.
   *     like it = map.begin(); --it;
   *       or it = map.end(); ++end();
   */
  class const_iterator;

  class iterator {
    friend class const_iterator;
    friend class map;
  private:
    map *base_ = nullptr;
    Node *ptr_ = nullptr;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = pair<const Key, T>;
    using pointer = pair<const Key, T>*;
    using reference = pair<const Key, T>&;
    using iterator_category = std::bidirectional_iterator_tag;
    using is_map_iterator = std::bool_constant<true>;

    iterator() = default;

    iterator(Node *ptr, map *base): ptr_(ptr), base_(base) {}

    iterator(const iterator &other) = default;

    iterator operator++(int) {
      if (!ptr_) {
        throw std::exception();
      }
      auto ret = *this;
      if (ptr_->r_) {
        ptr_ = ptr_->r_;
        while (ptr_->l_) {
          ptr_ = ptr_->l_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::right) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          ptr_ = nullptr;
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return ret;
    }

    iterator &operator++() {
      if (!ptr_) {
        throw std::exception();
      }
      if (ptr_->r_) {
        ptr_ = ptr_->r_;
        while (ptr_->l_) {
          ptr_ = ptr_->l_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::right) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          ptr_ = nullptr;
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return *this;
    }

    iterator operator--(int) {
      auto ret = *this;
      if (!ptr_) {
        ptr_ = base_->GetEnd();
        if (!ptr_) {
          throw std::exception();
        }
        return ret;
      }
      if (ptr_->l_) {
        ptr_ = ptr_->l_;
        while (ptr_->r_) {
          ptr_ = ptr_->r_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::left) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          throw std::exception();
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return ret;
    }

    iterator &operator--() {
      if (!ptr_) {
        ptr_ = base_->GetEnd();
        if (!ptr_) {
          throw std::exception();
        }
        return *this;
      }
      if (ptr_->l_) {
        ptr_ = ptr_->l_;
        while (ptr_->r_) {
          ptr_ = ptr_->r_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::left) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          throw std::exception();
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return *this;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    value_type &operator*() const {
      if (!ptr_) {
        throw std::exception();
      }
      return ptr_->val_;
    }

    bool operator==(const iterator &rhs) const {
      return ptr_ == rhs.ptr_ && base_ == rhs.base_;
    }

    bool operator==(const const_iterator &rhs) const {
      return ptr_ == rhs.ptr_ && base_ == rhs.base_;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return ptr_ != rhs.ptr_ || base_ != rhs.base_;
    }

    bool operator!=(const const_iterator &rhs) const {
      return ptr_ != rhs.ptr_ || base_ != rhs.base_;
    }

    /**
     * for the support of it->first.
     * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
     */
    value_type *operator->() const noexcept {
      return &ptr_->val_;
    }
  };

  class const_iterator {
    // it should has similar member method as iterator.
    //  and it should be able to construct from an iterator.
    friend class iterator;
    friend class map;
  private:
    Node *ptr_ = nullptr;
    const map *base_ = nullptr;
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = const pair<const Key, T>;
    using pointer = const pair<const Key, T>*;
    using reference = const pair<const Key, T>&;
    using iterator_category = std::bidirectional_iterator_tag;
    using is_map_iterator = std::bool_constant<true>;

    const_iterator() = default;

    const_iterator(Node *ptr, const map *base): ptr_(ptr), base_(base) {}

    const_iterator(const const_iterator &other) = default;

    explicit const_iterator(const iterator &other) {
      ptr_ = other.ptr_;
      base_ = other.base_;
    }

    const_iterator operator++(int) {
      if (!ptr_) {
        throw std::exception();
      }
      auto ret = *this;
      if (ptr_->r_) {
        ptr_ = ptr_->r_;
        while (ptr_->l_) {
          ptr_ = ptr_->l_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::right) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          ptr_ = nullptr;
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return ret;
    }

    const_iterator &operator++() {
      if (!ptr_) {
        throw std::exception();
      }
      if (ptr_->r_) {
        ptr_ = ptr_->r_;
        while (ptr_->l_) {
          ptr_ = ptr_->l_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::right) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          ptr_ = nullptr;
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return *this;
    }

    const_iterator operator--(int) {
      auto ret = *this;
      if (!ptr_) {
        ptr_ = base_->GetEnd();
        if (!ptr_) {
          throw std::exception();
        }
        return ret;
      }
      if (ptr_->l_) {
        ptr_ = ptr_->l_;
        while (ptr_->r_) {
          ptr_ = ptr_->r_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::left) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          throw std::exception();
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return ret;
    }

    const_iterator &operator--() {
      if (!ptr_) {
        ptr_ = base_->GetEnd();
        if (!ptr_) {
          throw std::exception();
        }
        return *this;
      }
      if (ptr_->l_) {
        ptr_ = ptr_->l_;
        while (ptr_->r_) {
          ptr_ = ptr_->r_;
        }
      } else {
        while (ptr_->ChildType() == ChildType::left) {
          ptr_ = ptr_->parent_;
        }
        if (ptr_->ChildType() == ChildType::root) {
          throw std::exception();
        } else {
          ptr_ = ptr_->parent_;
        }
      }
      return *this;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    const value_type &operator*() const {
      if (!ptr_) {
        throw std::exception();
      }
      return ptr_->val_;
    }

    bool operator==(const iterator &rhs) const {
      return ptr_ == rhs.ptr_ && base_ == rhs.base_;
    }

    bool operator==(const const_iterator &rhs) const {
      return ptr_ == rhs.ptr_ && base_ == rhs.base_;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return ptr_ != rhs.ptr_ || base_ != rhs.base_;
    }

    bool operator!=(const const_iterator &rhs) const {
      return ptr_ != rhs.ptr_ || base_ != rhs.base_;
    }

    const value_type *operator->() const noexcept {
      return &ptr_->val_;
    }
  };

  map() = default;

  map(const map &other) {
    if (other.root_) {
      root_ = other.root_->Clone();
    }
    begin_ = end_ = nullptr;
    size_ = other.size_;
  }

  map &operator=(const map &other) {
    if (this == &other) {
      return *this;
    }
    if (root_) {
      Destruct(root_);
    }
    root_ = nullptr;
    if (other.root_) {
      root_ = other.root_->Clone();
    }
    begin_ = end_ = nullptr;
    size_ = other.size_;
    return *this;
  }

  ~map() {
    if (root_) {
      Destruct(root_);
    }
  }

  /**
   * access specified element with bounds checking
   * Returns a reference to the mapped value of the element with key equivalent to key.
   * If no such element exists, an exception of type `index_out_of_bound'
   */
  T &at(const Key &key) {
    auto res = Find(key);
    if (res == nullptr) {
      throw std::exception();
    }
    return res->val_.second;
  }

  [[nodiscard]] const T &at(const Key &key) const {
    auto res = Find(key);
    if (res == nullptr) {
      throw std::exception();
    }
    return res->val_.second;
  }

  /**
   * access specified element
   * Returns a reference to the value that is mapped to a key equivalent to key,
   *   performing an insertion if such key does not already exist.
   */
  T &operator[](const Key &key) {
    auto res = Find(key);
    if (res == nullptr) {
      auto ret = Insert(key, T{});
      ++size_;
      assert(ret.second);
      return ret.first->val_.second;
    }
    return res->val_.second;
  }

  /**
   * behave like at() throw index_out_of_bound if such key does not exist.
   */
  const T &operator[](const Key &key) const {
    auto res = Find(key);
    if (res == nullptr) {
      throw std::exception();
    }
    return res->val_.second;
  }

  /**
   * return a iterator to the beginning
   */
  iterator begin() {
    return iterator(GetBegin(), this);
  }

  [[nodiscard]] const_iterator cbegin() const {
    return const_iterator(GetBegin(), this);
  }

  /**
   * return a iterator to the end
   * in fact, it returns past-the-end.
   */
  iterator end() {
    return iterator(nullptr, this);
  }

  [[nodiscard]] const_iterator cend() const {
    return const_iterator(nullptr, this);
  }

  /**
   * checks whether the container is empty
   * return true if empty, otherwise false.
   */
  [[nodiscard]] bool empty() const {
    return size_ == 0;
  }

  /**
   * returns the number of elements.
   */
  [[nodiscard]] size_t size() const {
    return size_;
  }

  /**
   * clears the contents
   */
  void clear() {
    if (root_) {
      Destruct(root_);
      begin_ = end_ = nullptr;
    }
    root_ = nullptr;
    size_ = 0;
  }

  /**
   * insert an element.
   * return a pair, the first of the pair is
   *   the iterator to the new element (or the element that prevented the insertion),
   *   the second one is true if insert successfully, or false.
   */
  pair<iterator, bool> insert(const value_type &value) {
    auto res = Insert(value.first, value.second);
    if (res.second) {
      ++size_;
    }
    return {iterator(res.first, this), res.second};
  }

  /**
   * erase the element at pos.
   *
   * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
   */
  void erase(iterator pos) {
    if (!pos.ptr_ || pos.base_ != this) {
      throw std::exception();
    }
    Remove(pos.ptr_);
    --size_;
  }

  void erase(const Key &key) {
    auto tmp = find(key);
    if (tmp != end()) {
      erase(tmp);
    }
  }

  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   *   which is either 1 or 0
   *     since this container does not allow duplicates.
   * The default method of check the equivalence is !(a < b || b > a)
   */
  [[nodiscard]] size_t count(const Key &key) const {
    return Find(key) == nullptr ? 0 : 1;
  }

  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is returned.
   */
  iterator find(const Key &key) {
    auto res = Find(key);
    return res == nullptr ? end() : iterator(res, this);
  }

  [[nodiscard]] const_iterator find(const Key &key) const {
    auto res = Find(key);
    return res == nullptr ? cend() : const_iterator(res, this);
  }
};

template <typename _Iter>
static constexpr bool is_map_iterator = requires {
  typename _Iter::is_map_iterator;
};

template <typename _Iter>
void sort(_Iter __first, _Iter __last) {
  using it_type = typename std::iterator_traits<_Iter>::iterator_category;
  if constexpr (is_map_iterator<_Iter>) {
    return;
  } else if constexpr (std::is_base_of_v<std::random_access_iterator_tag, it_type>) {
    std::sort(__first, __last);
  } else {
    static_assert(is_map_iterator<_Iter> || std::is_base_of_v<std::random_access_iterator_tag, it_type>,
                  "Not a random access iterator.");
  }
}

template <typename _Tp>
concept is_container = requires (const _Tp &a) {
  typename _Tp::iterator;
  a.cbegin();
  a.cend();
};

template <typename _Tp>
concept container_in_container = requires (const _Tp &a) {
  std::cbegin(*std::cbegin(a));
  std::cend(*std::cend(a));
};

template <typename _Tp>
void Print(const _Tp &__val, size_t len) {
  if constexpr (is_container<_Tp> || std::is_array_v<_Tp>) {
    for (int i = 1; i <= len; ++i) {
      std::cout << '\t';
    }
    std::cout << "[ ";
    for (auto it = std::cbegin(__val); it != std::cend(__val); ++it) {
      if (container_in_container<_Tp> && it == std::cbegin(__val)) {
        std::cout << std::endl;
      }
      Print(*it, len + 1);
      if (container_in_container<_Tp>) {
        std::cout << std::endl;
      } else {
        std::cout << " ";
      }
    }
    if (container_in_container<_Tp>) {
      for (int i = 1; i <= len; ++i) {
        std::cout << '\t';
      }
    }
    std::cout << ']';
  } else {
    std::cout << __val;
  }
}

template <typename _Tp>
void print(const _Tp &__val) {
  Print(__val, 0);
}