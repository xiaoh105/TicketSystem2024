#include "common/rid.h"
#include "storage/page/b_plus_tree_internal_page.h"

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, and set max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
}
/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType { return array_[index].first; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) { array_[index].first = key; }

INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::UpperBound(const KeyType &key, const KeyComparator &cmp) const -> int {
  auto l = 1;
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
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::LowerBound(const KeyType &key, const KeyComparator &cmp) const -> int {
  auto l = 1;
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

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType { return array_[index].second; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValueAt(int index, const ValueType &val) { array_[index].second = val; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyValue(int index, const KeyType &key, const ValueType &val) {
  array_[index] = make_pair(key, val);
}

template class BPlusTreeInternalPage<pair<unsigned long long, RID>, page_id_t, std::less<>>;
template class BPlusTreeInternalPage<unsigned long long, page_id_t, std::less<>>;