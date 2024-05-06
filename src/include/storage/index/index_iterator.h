/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "storage/page/b_plus_tree_leaf_page.h"

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
public:
  // you may define your own constructor based on your member variables
  IndexIterator();
  IndexIterator(BufferPoolManager *bpm, ReadPageGuard guard, int index);
  ~IndexIterator();  // NOLINT

  auto IsEnd() -> bool;

  auto operator*() -> const MappingType &;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool {
    return cur_guard_.PageId() == itr.cur_guard_.PageId() && index_ == itr.index_;
  }

  auto operator!=(const IndexIterator &itr) const -> bool {
    return cur_guard_.PageId() != itr.cur_guard_.PageId() || index_ != itr.index_;
  }

private:
  BufferPoolManager *bpm_;
  ReadPageGuard cur_guard_;
  int index_;
};
