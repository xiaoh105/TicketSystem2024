/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once

#include "common/stl/pointers.hpp"
#include "storage/page/b_plus_tree_leaf_page.h"

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
public:
  // you may define your own constructor based on your member variables
  IndexIterator();
  IndexIterator(shared_ptr<BufferPoolManager> bpm, ReadPageGuard guard, int index);
  ~IndexIterator();  // NOLINT

  IndexIterator(IndexIterator &&other) = default;
  IndexIterator &operator=(IndexIterator &&other) = default;

  auto IsEnd() -> bool;

  auto operator*() -> const MappingType &;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool {
    return cur_guard_.PageId() == itr.cur_guard_.PageId() && index_ == itr.index_;
  }

  auto operator!=(const IndexIterator &itr) const -> bool {
    return cur_guard_.PageId() != itr.cur_guard_.PageId() || index_ != itr.index_;
  }

  explicit operator bool() const {
    return static_cast<bool>(bpm_);
  }

private:
  shared_ptr<BufferPoolManager> bpm_;
  ReadPageGuard cur_guard_;
  int index_;
};
