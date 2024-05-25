/**
 * index_iterator.cpp
 */
#include "common/rid.h"
#include "storage/index/index_iterator.h"

/*
 * NOTE: you can change the destructor/constructor method here
 * set your own input parameters
 */
INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator() = default;

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(shared_ptr<BufferPoolManager> bpm, ReadPageGuard guard, int index)
  : bpm_(std::move(bpm)), cur_guard_(std::move(guard)), index_(index) {}

INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool {
  auto cur_page = cur_guard_.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  return cur_page->GetNextPageId() == INVALID_PAGE_ID && index_ == cur_page->GetSize() - 1;
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> const MappingType & {
  auto cur_page = cur_guard_.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  return cur_page->KeyValueAt(index_);
}

INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
  auto cur_page = cur_guard_.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  ++index_;
  if (index_ >= cur_page->GetSize()) {
    auto next_id = cur_page->GetNextPageId();
    if (next_id == INVALID_PAGE_ID) {
      return *this;
    }
    index_ = 0;
    auto next_guard = bpm_->FetchPageRead(next_id);
    cur_guard_ = std::move(next_guard);
  }
  return *this;
}

template class IndexIterator<pair<unsigned long long, RID>, RID, std::less<>>;
template class IndexIterator<unsigned long long, RID, std::less<>>;
