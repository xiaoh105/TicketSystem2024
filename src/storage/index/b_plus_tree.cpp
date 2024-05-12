#include <sstream>
#include <string>
#include <functional>

#include "storage/index/b_plus_tree.h"

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(shared_ptr<BufferPoolManager> buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
  : bpm_(std::move(buffer_pool_manager)),
    comparator_(std::move(comparator)),
    leaf_max_size_(leaf_max_size),
    internal_max_size_(internal_max_size),
    header_page_id_(0) {
  if (bpm_->IsFirstVisit()) {
    BasicPageGuard guard = bpm_->NewPageGuarded(&header_page_id_);
    auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
    root_page->root_page_id_ = INVALID_PAGE_ID;
  }
}

INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::~BPlusTree() {
  bpm_->FlushAllPages();
}
/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { return GetRootPageId() == 0; }
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, vector<ValueType> *result) -> bool {
  // Declaration of context instance.
  Context ctx;
  ctx.root_page_id_ = GetRootPageId();
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  auto cur = ctx.root_page_id_;
  auto cur_guard = bpm_->FetchPageRead(cur);
  auto cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  ctx.read_set_.push_back(std::move(cur_guard));
  while (!cur_page->IsLeafPage()) {
    auto pos = cur_page->UpperBound(key, comparator_) - 1;
    cur = cur_page->ValueAt(pos);
    cur_guard = bpm_->FetchPageRead(cur);
    cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    ctx.read_set_.push_back(std::move(cur_guard));
    ctx.read_set_.pop_front();
  }
  cur_guard = std::move(ctx.read_set_.back());
  ctx.read_set_.pop_back();
  auto leaf_page = cur_guard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  auto pos = leaf_page->LowerBound(key, comparator_);
  if (pos >= leaf_page->GetSize()) {
    return false;
  }
  if (leaf_page->KeyAt(pos) != key) {
    return false;
  }
  result->push_back(leaf_page->ValueAt(pos));
  return true;
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::LowerBound(const KeyType &key) -> INDEXITERATOR_TYPE {
  // Declaration of context instance.
  Context ctx;
  ctx.root_page_id_ = GetRootPageId();
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return {};
  }
  auto cur = ctx.root_page_id_;
  auto cur_guard = bpm_->FetchPageRead(cur);
  auto cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  ctx.read_set_.push_back(std::move(cur_guard));
  while (!cur_page->IsLeafPage()) {
    auto pos = cur_page->UpperBound(key, comparator_) - 1;
    cur = cur_page->ValueAt(pos);
    cur_guard = bpm_->FetchPageRead(cur);
    cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    ctx.read_set_.push_back(std::move(cur_guard));
    ctx.read_set_.pop_front();
  }
  cur_guard = std::move(ctx.read_set_.back());
  ctx.read_set_.pop_back();
  auto leaf_page = cur_guard.As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  auto pos = leaf_page->LowerBound(key, comparator_);
  if (pos >= leaf_page->GetSize()) {
    INDEXITERATOR_TYPE ret(bpm_, std::move(cur_guard), leaf_page->GetSize() - 1);
    if (ret.IsEnd()) {
      return {};
    } else {
      ++ret;
      return std::move(ret);
    }
  }
  INDEXITERATOR_TYPE ret(bpm_, std::move(cur_guard), pos);
  return std::move(ret);
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  // Declaration of context instance.
  Context ctx;
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  ctx.root_page_id_ = ctx.header_page_->As<BPlusTreeHeaderPage>()->root_page_id_;
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    page_id_t cur = 0;
    auto cur_guard = bpm_->NewPageGuarded(&cur);
    auto cur_page = cur_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
    cur_page->Init(leaf_max_size_);
    cur_page->SetSize(1);
    cur_page->SetKeyValue(0, key, value);
    cur_page->SetNextPageId(INVALID_PAGE_ID);
    auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = cur;
    return true;
  }
  auto cur = ctx.root_page_id_;
  auto cur_guard = bpm_->FetchPageWrite(cur);
  auto cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  while (!cur_page->IsLeafPage()) {
    if (cur_page->GetSize() < internal_max_size_) {
      // ctx.header_page_ = std::nullopt;
      ctx.write_set_.clear();
    }
    ctx.write_set_.push_back(std::move(cur_guard));
    auto pos = cur_page->UpperBound(key, comparator_) - 1;
    cur = cur_page->ValueAt(pos);
    cur_guard = bpm_->FetchPageWrite(cur);
    cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  }
  auto leaf_page = cur_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  if (leaf_page->GetSize() < leaf_max_size_) {
    // ctx.header_page_ = std::nullopt;
    ctx.write_set_.clear();
  }
  auto pos = leaf_page->LowerBound(key, comparator_);
  if (pos < leaf_page->GetSize() && leaf_page->KeyAt(pos) == key) {
    return false;
  }
  if (leaf_page->GetSize() < leaf_max_size_) {
    for (int i = leaf_page->GetSize() - 1; i >= pos; --i) {
      leaf_page->SetKeyValue(i + 1, leaf_page->KeyAt(i), leaf_page->ValueAt(i));
    }
    leaf_page->SetKeyValue(pos, key, value);
    leaf_page->IncreaseSize(1);
    return true;
  }
  auto cur_size = leaf_page->GetSize();
  pair<KeyType, ValueType> tmp[leaf_max_size_ + 1];
  for (int i = 0; i < pos; ++i) {
    tmp[i] = make_pair(leaf_page->KeyAt(i), leaf_page->ValueAt(i));
  }
  tmp[pos] = make_pair(key, value);
  for (int i = pos; i < cur_size; ++i) {
    tmp[i + 1] = make_pair(leaf_page->KeyAt(i), leaf_page->ValueAt(i));
  }
  cur_size = (cur_size + 1) >> 1;
  auto new_size = (leaf_max_size_ + 1) - cur_size;
  leaf_page->SetSize(cur_size);
  page_id_t new_id;
  auto new_guard = bpm_->NewPageGuarded(&new_id);
  auto new_page = new_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  new_page->Init(leaf_max_size_);
  new_page->SetSize(new_size);
  for (int i = 0; i < new_size; ++i) {
    new_page->SetKeyValue(i, tmp[i + cur_size].first, tmp[i + cur_size].second);
  }
  if (pos <= cur_size) {
    for (int i = pos; i < cur_size; ++i) {
      leaf_page->SetKeyValue(i, tmp[i].first, tmp[i].second);
    }
  }
  new_page->SetNextPageId(leaf_page->GetNextPageId());
  leaf_page->SetNextPageId(new_id);
  if (ctx.root_page_id_ == cur) {
    int root_id;
    auto root_guard = bpm_->NewPageGuarded(&root_id);
    auto root_page = root_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    root_page->Init(internal_max_size_);
    root_page->SetSize(2);
    root_page->SetKeyAt(1, new_page->KeyAt(0));
    root_page->SetValueAt(0, cur);
    root_page->SetValueAt(1, new_id);
    auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = root_id;
    return true;
  }
  InsertInternal(new_page->KeyAt(0), ctx, new_id);
  return true;
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertInternal(const KeyType &key, Context &ctx, int ch) {
  auto cur_guard = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();
  auto cur_page = cur_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  auto pos = cur_page->LowerBound(key, comparator_);
  if (cur_page->GetSize() < cur_page->GetMaxSize()) {
    for (int i = cur_page->GetSize() - 1; i >= pos; --i) {
      cur_page->SetKeyValue(i + 1, cur_page->KeyAt(i), cur_page->ValueAt(i));
    }
    cur_page->SetKeyValue(pos, key, ch);
    cur_page->IncreaseSize(1);
    return;
  }
  pair<KeyType, page_id_t> tmp[internal_max_size_ + 1];
  for (int i = 0; i < pos; ++i) {
    tmp[i] = make_pair(cur_page->KeyAt(i), cur_page->ValueAt(i));
  }
  tmp[pos] = make_pair(key, ch);
  for (int i = pos; i < cur_page->GetSize(); ++i) {
    tmp[i + 1] = make_pair(cur_page->KeyAt(i), cur_page->ValueAt(i));
  }
  int new_id;
  auto new_guard = bpm_->NewPageGuarded(&new_id);
  auto new_page = new_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  new_page->Init(internal_max_size_);
  auto cur_size = (internal_max_size_ + 1) >> 1;
  auto new_size = (internal_max_size_ + 1) - cur_size;
  new_page->SetSize(new_size);
  cur_page->SetSize(cur_size);
  for (int i = 0; i < new_size; ++i) {
    new_page->SetKeyValue(i, tmp[cur_size + i].first, tmp[cur_size + i].second);
  }
  if (pos < cur_size) {
    for (int i = pos; i < cur_size; ++i) {
      cur_page->SetKeyValue(i, tmp[i].first, tmp[i].second);
    }
  }
  if (ctx.root_page_id_ == cur_guard.PageId()) {
    int root_id;
    auto root_guard = bpm_->NewPageGuarded(&root_id);
    auto root_page = root_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    root_page->Init(internal_max_size_);
    root_page->SetSize(2);
    root_page->SetKeyAt(1, tmp[cur_size].first);
    root_page->SetValueAt(0, cur_guard.PageId());
    root_page->SetValueAt(1, new_id);
    auto header_page = ctx.header_page_->AsMut<BPlusTreeHeaderPage>();
    header_page->root_page_id_ = root_id;
    return;
  }
  InsertInternal(tmp[cur_size].first, ctx, new_id);
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immediately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  Context ctx;
  ctx.header_page_ = bpm_->FetchPageWrite(header_page_id_);
  ctx.root_page_id_ = ctx.header_page_->As<BPlusTreeHeaderPage>()->root_page_id_;
  if (ctx.root_page_id_ == INVALID_PAGE_ID) {
    return;
  }
  auto cur = ctx.root_page_id_;
  auto cur_guard = bpm_->FetchPageWrite(cur);
  auto cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  int lp;
  int rp;
  int ls;
  int rs;
  while (!cur_page->IsLeafPage()) {
    if (cur_page->GetSize() > std::max(internal_max_size_ >> 1, 2)) {
      // ctx.header_page_ = std::nullopt;
      ctx.write_set_.clear();
    }
    ctx.write_set_.push_back(std::move(cur_guard));
    auto pos = cur_page->UpperBound(key, comparator_) - 1;
    cur = cur_page->ValueAt(pos);
    lp = pos - 1;
    rp = (pos == cur_page->GetSize() - 1) ? -1 : pos + 1;
    if (lp != -1) {
      ls = cur_page->ValueAt(lp);
    }
    if (rp != -1) {
      rs = cur_page->ValueAt(rp);
    }
    cur_guard = bpm_->FetchPageWrite(cur);
    cur_page = cur_guard.As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  }
  auto leaf_page = cur_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  if (leaf_page->GetSize() > leaf_max_size_ >> 1) {
    // ctx.header_page_ = std::nullopt;
    ctx.write_set_.clear();
  }
  auto pos = leaf_page->LowerBound(key, comparator_);
  if (pos >= cur_page->GetSize() || leaf_page->KeyAt(pos) != key) {
    return;
  }
  auto cur_size = leaf_page->GetSize();
  for (int i = pos + 1; i < cur_size; ++i) {
    leaf_page->SetKeyValue(i - 1, leaf_page->KeyAt(i), leaf_page->ValueAt(i));
  }
  --cur_size;
  leaf_page->IncreaseSize(-1);
  if (cur_size >= leaf_max_size_ >> 1) {
    return;
  }
  if (ctx.IsRootPage(cur)) {
    if (cur_size == 0) {
      cur_guard.Drop();
      bpm_->DeletePage(cur);
      ctx.header_page_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = INVALID_PAGE_ID;
    }
    return;
  }
  WritePageGuard l_guard;
  WritePageGuard r_guard;
  B_PLUS_TREE_LEAF_PAGE_TYPE *l_page;
  B_PLUS_TREE_LEAF_PAGE_TYPE *r_page;
  if (lp != -1) {
    l_guard = bpm_->FetchPageWrite(ls);
    l_page = l_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
    if (l_page->GetSize() > l_page->GetMinSize()) {
      for (int i = cur_size - 1; i >= 0; --i) {
        leaf_page->SetKeyValue(i + 1, leaf_page->KeyAt(i), leaf_page->ValueAt(i));
      }
      leaf_page->IncreaseSize(1);
      auto l_size = l_page->GetSize();
      leaf_page->SetKeyValue(0, l_page->KeyAt(l_size - 1), l_page->ValueAt(l_size - 1));
      l_page->IncreaseSize(-1);
      auto parent_guard = std::move(ctx.write_set_.back());
      auto parent_page = parent_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
      parent_page->SetKeyAt(lp + 1, leaf_page->KeyAt(0));
      return;
    }
    l_guard.Drop();
  }
  if (rp != -1) {
    r_guard = bpm_->FetchPageWrite(rs);
    r_page = r_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
    if (r_page->GetSize() > r_page->GetMinSize()) {
      leaf_page->SetKeyValue(cur_size, r_page->KeyAt(0), r_page->ValueAt(0));
      leaf_page->IncreaseSize(1);
      for (int i = 1; i < r_page->GetSize(); ++i) {
        r_page->SetKeyValue(i - 1, r_page->KeyAt(i), r_page->ValueAt(i));
      }
      r_page->IncreaseSize(-1);
      auto parent_guard = std::move(ctx.write_set_.back());
      auto parent_page = parent_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
      parent_page->SetKeyAt(rp, r_page->KeyAt(0));
      return;
    }
    r_guard.Drop();
  }
  if (lp != -1) {
    l_guard = bpm_->FetchPageWrite(ls);
    l_page = l_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
    auto parent_page = ctx.write_set_.back().As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    if (lp != -1) {
      auto l_size = l_page->GetSize();
      for (int i = 0; i < cur_size; ++i) {
        l_page->SetKeyValue(l_size + i, leaf_page->KeyAt(i), leaf_page->ValueAt(i));
      }
      l_page->IncreaseSize(cur_size);
      l_page->SetNextPageId(leaf_page->GetNextPageId());
      cur_guard.Drop();
      l_guard.Drop();
      bpm_->DeletePage(cur);
      RemoveInternal(parent_page->KeyAt(lp + 1), ctx, cur);
      return;
    }
  }
  r_guard = bpm_->FetchPageWrite(rs);
  r_page = r_guard.AsMut<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  auto parent_page = ctx.write_set_.back().As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  auto r_size = r_page->GetSize();
  for (int i = 0; i < r_size; ++i) {
    leaf_page->SetKeyValue(cur_size + i, r_page->KeyAt(i), r_page->ValueAt(i));
  }
  leaf_page->IncreaseSize(r_size);
  leaf_page->SetNextPageId(r_page->GetNextPageId());
  r_guard.Drop();
  cur_guard.Drop();
  bpm_->DeletePage(rs);
  RemoveInternal(parent_page->KeyAt(rp), ctx, rs);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveInternal(const KeyType &key, Context &ctx, int ch) {
  auto cur_guard = std::move(ctx.write_set_.back());
  ctx.write_set_.pop_back();
  auto cur = cur_guard.PageId();
  auto cur_page = cur_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  auto cur_size = cur_page->GetSize();
  if (ctx.IsRootPage(cur) && cur_size == 2) {
    if (cur_page->ValueAt(0) == ch) {
      ctx.header_page_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = cur_page->ValueAt(1);
      cur_guard.Drop();
      bpm_->DeletePage(cur);
    } else {
      ctx.header_page_->AsMut<BPlusTreeHeaderPage>()->root_page_id_ = cur_page->ValueAt(0);
      cur_guard.Drop();
      bpm_->DeletePage(cur);
    }
    return;
  }
  auto pos = cur_page->LowerBound(key, comparator_);
  for (int i = pos + 1; i < cur_size; ++i) {
    cur_page->SetKeyValue(i - 1, cur_page->KeyAt(i), cur_page->ValueAt(i));
  }
  cur_page->IncreaseSize(-1);
  --cur_size;
  if (ctx.IsRootPage(cur) || cur_size >= std::max(internal_max_size_ >> 1, 2)) {
    return;
  }
  int ls;
  int lp;
  int rs;
  int rp;
  auto parent_page = ctx.write_set_.back().AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  for (int i = 0; i < parent_page->GetSize(); ++i) {
    if (parent_page->ValueAt(i) == cur) {
      pos = i;
      lp = i - 1;
      if (lp != -1) {
        ls = parent_page->ValueAt(lp);
      }
      rp = (i == parent_page->GetSize() - 1) ? -1 : i + 1;
      if (rp != -1) {
        rs = parent_page->ValueAt(rp);
      }
    }
  }
  WritePageGuard l_guard;
  WritePageGuard r_guard;
  BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *l_page;
  BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator> *r_page;
  if (lp != -1) {
    l_guard = bpm_->FetchPageWrite(ls);
    l_page = l_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    if (l_page->GetSize() > std::max(internal_max_size_ >> 1, 2)) {
      for (int i = cur_size - 1; i >= 0; --i) {
        cur_page->SetKeyValue(i + 1, cur_page->KeyAt(i), cur_page->ValueAt(i));
      }
      auto l_size = l_page->GetSize();
      cur_page->SetKeyAt(1, parent_page->KeyAt(pos));
      cur_page->SetValueAt(0, l_page->ValueAt(l_size - 1));
      parent_page->SetKeyAt(pos, l_page->KeyAt(l_size - 1));
      cur_page->IncreaseSize(1);
      l_page->IncreaseSize(-1);
      return;
    }
    l_guard.Drop();
  }
  if (rp != -1) {
    r_guard = bpm_->FetchPageWrite(rs);
    r_page = r_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    if (r_page->GetSize() > std::max(internal_max_size_ >> 1, 2)) {
      cur_page->SetKeyAt(cur_size, parent_page->KeyAt(rp));
      parent_page->SetKeyAt(rp, r_page->KeyAt(1));
      cur_page->SetValueAt(cur_size, r_page->ValueAt(0));
      for (int i = 1; i < r_page->GetSize(); ++i) {
        r_page->SetKeyValue(i - 1, r_page->KeyAt(i), r_page->ValueAt(i));
      }
      cur_page->IncreaseSize(1);
      r_page->IncreaseSize(-1);
      return;
    }
    r_guard.Drop();
  }
  if (lp != -1) {
    l_guard = bpm_->FetchPageWrite(ls);
    l_page = l_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
    auto l_size = l_page->GetSize();
    for (int i = 0; i < cur_size; ++i) {
      l_page->SetKeyValue(l_size + i, cur_page->KeyAt(i), cur_page->ValueAt(i));
    }
    l_page->SetKeyAt(l_size, parent_page->KeyAt(pos));
    l_page->IncreaseSize(cur_size);
    cur_guard.Drop();
    l_guard.Drop();
    bpm_->DeletePage(cur);
    RemoveInternal(parent_page->KeyAt(pos), ctx, cur);
    return;
  }
  r_guard = bpm_->FetchPageWrite(rs);
  r_page = r_guard.AsMut<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  auto r_size = r_page->GetSize();
  for (int i = 0; i < r_size; ++i) {
    cur_page->SetKeyValue(cur_size + i, r_page->KeyAt(i), r_page->ValueAt(i));
  }
  cur_page->SetKeyAt(cur_size, parent_page->KeyAt(rp));
  cur_page->IncreaseSize(r_size);
  r_guard.Drop();
  cur_guard.Drop();
  bpm_->DeletePage(rs);
  RemoveInternal(parent_page->KeyAt(rp), ctx, rs);
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE {
  auto cur = GetRootPageId();
  auto cur_guard = bpm_->FetchPageRead(cur);
  auto cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  while (!cur_page->IsLeafPage()) {
    cur = cur_page->ValueAt(0);
    auto tmp_guard = bpm_->FetchPageRead(cur);
    cur_guard = std::move(tmp_guard);
    cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  }
  return INDEXITERATOR_TYPE(bpm_, std::move(cur_guard), 0);
}

/*
 * Input parameter is low-key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  auto cur = GetRootPageId();
  auto cur_guard = bpm_->FetchPageRead(cur);
  auto cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  while (!cur_page->IsLeafPage()) {
    auto pos = cur_page->UpperBound(key, comparator_) - 1;
    cur = cur_page->ValueAt(pos);
    auto tmp_guard = bpm_->FetchPageRead(cur);
    cur_guard = std::move(tmp_guard);
    cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  }
  auto leaf_page = cur_guard.template As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  auto pos = leaf_page->LowerBound(key, comparator_);
  return INDEXITERATOR_TYPE(bpm_, std::move(cur_guard), pos);
}

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE {
  auto cur = GetRootPageId();
  auto cur_guard = bpm_->FetchPageRead(cur);
  auto cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  while (!cur_page->IsLeafPage()) {
    cur = cur_page->ValueAt(cur_page->GetSize() - 1);
    auto tmp_guard = bpm_->FetchPageRead(cur);
    cur_guard = std::move(tmp_guard);
    cur_page = cur_guard.template As<BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>>();
  }
  return INDEXITERATOR_TYPE(bpm_, std::move(cur_guard), cur_page->GetSize());
}

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() const -> page_id_t {
  auto page = bpm_->FetchPageRead(header_page_id_);
  return page.As<BPlusTreeHeaderPage>()->root_page_id_;
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::SetRootPageId(page_id_t id) {
  auto page = bpm_->FetchPageWrite(header_page_id_);
  page.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = id;
}

template class BPlusTree<int, int, std::less<>>;
template class BPlusTree<pair<unsigned long long, int>, int, std::less<>>;