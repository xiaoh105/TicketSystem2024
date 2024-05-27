#include "common/time.h"
#include "common/rid.h"
#include "storage/page/b_plus_tree_leaf_page.h"

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set next page id and set max size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetMaxSize(max_size);
  SetSize(0);
  next_page_id_ = INVALID_PAGE_ID;
}

/**
 * Helper methods to set/get next page id
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { return next_page_id_; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType { return array_[index].first; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> ValueType { return array_[index].second; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyValueAt(int index) const -> const MappingType & { return array_[index]; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::UpperBound(const KeyType &key, const KeyComparator &cmp) const -> int {
  auto l = 0;
  auto r = GetSize() - 1;
  if (array_[r].first <= key) {
    return r + 1;
  }
  while (l + 1 < r) {
    auto mid = (l + r) >> 1;
    if (array_[mid].first <= key) {
      l = mid;
    } else {
      r = mid;
    }
  }
  return array_[l].first > key ? l : r;
}

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::LowerBound(const KeyType &key, const KeyComparator &cmp) const -> int {
  auto l = 0;
  auto r = GetSize() - 1;
  if (array_[r].first < key) {
    return r + 1;
  }
  while (l + 1 < r) {
    auto mid = (l + r) >> 1;
    if (array_[mid].first < key) {
      l = mid;
    } else {
      r = mid;
    }
  }
  return array_[l].first >= key ? l : r;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetKeyValue(int index, const KeyType &key, const ValueType &value) {
  array_[index] = make_pair(key, value);
}

template class BPlusTreeLeafPage<pair<unsigned long long, RID>, RID, std::less<>>;
template class BPlusTreeLeafPage<unsigned long long, RID, std::less<>>;
template class BPlusTreeLeafPage<pair<unsigned long long, Date>, page_id_t, std::less<>>;
template class BPlusTreeLeafPage<pair<unsigned long long, Date>, RID, std::less<>>;
template class BPlusTreeLeafPage<unsigned long long, page_id_t, std::less<>>;
